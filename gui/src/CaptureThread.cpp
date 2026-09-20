#include "CaptureThread.h"
#include "ThermalMath.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

struct MappedBuffer {
  void *start = nullptr;
  size_t length = 0;
};

int xioctl(int fd, unsigned long request, void *arg) {
  int r;
  do {
    r = ioctl(fd, request, arg);
  } while (r == -1 && errno == EINTR);
  return r;
}

}  // namespace

CaptureThread::CaptureThread(QObject *parent) : QThread(parent) {}

CaptureThread::~CaptureThread() {
  stop();
  wait();
}

void CaptureThread::setDevicePath(const QString &path) {
  QMutexLocker lock(&m_mutex);
  m_path = path;
}

void CaptureThread::stop() {
  QMutexLocker lock(&m_mutex);
  m_stop = true;
}

QVector<float> CaptureThread::latestCelsius() const {
  QMutexLocker lock(&m_mutex);
  return m_latest;
}

QVector<quint16> CaptureThread::latestKelvin() const {
  QMutexLocker lock(&m_mutex);
  return m_kelvin;
}

void CaptureThread::run() {
  QString path;
  {
    QMutexLocker lock(&m_mutex);
    m_stop = false;
    path = m_path;
  }

  const int fd = open(path.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    emit errorOccurred(QString("Cannot open %1: %2").arg(path, strerror(errno)));
    return;
  }

  v4l2_format fmt{};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = kThermalWidth;
  fmt.fmt.pix.height = kFrameHeight;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;
  if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
    emit errorOccurred("Camera rejected 256x384 YUYV.");
    close(fd);
    return;
  }
  if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV
      || fmt.fmt.pix.width != static_cast<unsigned>(kThermalWidth)
      || fmt.fmt.pix.height != static_cast<unsigned>(kFrameHeight)) {
    emit errorOccurred("Camera did not stay on 256x384 YUYV.");
    close(fd);
    return;
  }

  const int stride = static_cast<int>(fmt.fmt.pix.bytesperline);

  v4l2_requestbuffers req{};
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
    emit errorOccurred("VIDIOC_REQBUFS failed.");
    close(fd);
    return;
  }

  std::vector<MappedBuffer> bufs(req.count);
  for (unsigned i = 0; i < req.count; ++i) {
    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
      emit errorOccurred("VIDIOC_QUERYBUF failed.");
      close(fd);
      return;
    }
    bufs[i].length = buf.length;
    bufs[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, buf.m.offset);
    if (bufs[i].start == MAP_FAILED) {
      emit errorOccurred("mmap failed.");
      close(fd);
      return;
    }
    if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
      emit errorOccurred("VIDIOC_QBUF failed.");
      close(fd);
      return;
    }
  }

  v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
    emit errorOccurred("VIDIOC_STREAMON failed.");
    close(fd);
    return;
  }

  QVector<float> celsius(kThermalWidth * kThermalHeight);
  QVector<quint16> kelvin(kThermalWidth * kThermalHeight);
  QVector<quint8> visual(kThermalWidth * kThermalHeight);

  while (true) {
    {
      QMutexLocker lock(&m_mutex);
      if (m_stop) {
        break;
      }
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeval tv{};
    tv.tv_sec = 1;
    const int r = select(fd + 1, &fds, nullptr, nullptr, &tv);
    if (r <= 0) {
      continue;
    }

    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      emit errorOccurred("VIDIOC_DQBUF failed.");
      break;
    }

    const auto *data = static_cast<const uint8_t *>(bufs[buf.index].start);
    decodeRadiometricYuyv(data, stride, celsius.data(), kelvin.data());
    decodeVisualY(data, stride, visual.data());

    ThermalStats st;
    st.minC = 1e9f;
    st.maxC = -1e9f;
    double sum = 0;
    int count = 0;
    for (int y = 0; y < kThermalHeight; ++y) {
      for (int x = 0; x < kThermalWidth; ++x) {
        const float t = celsius[y * kThermalWidth + x];
        // Drop the empty/NUC sentinel around 239 C from a 0x8000 raw value.
        if (t > 200.0f || t < -40.0f) {
          continue;
        }
        sum += t;
        count++;
        if (t < st.minC) {
          st.minC = t;
          st.minX = x;
          st.minY = y;
        }
        if (t > st.maxC) {
          st.maxC = t;
          st.maxX = x;
          st.maxY = y;
        }
      }
    }
    if (count == 0) {
      st.minC = st.maxC = st.avgC = st.centerC = 0;
      st.valid = false;
    } else {
      st.avgC = static_cast<float>(sum / count);
      st.centerC = celsius[(kThermalHeight / 2) * kThermalWidth + (kThermalWidth / 2)];
      st.valid = (count > 100) && ((st.maxC - st.minC) > 0.4f) && (st.maxC < 200.0f);
    }

    {
      QMutexLocker lock(&m_mutex);
      m_latest = celsius;
      m_kelvin = kelvin;
    }
    QImage vis(visual.constData(), kThermalWidth, kThermalHeight, kThermalWidth,
               QImage::Format_Grayscale8);
    emit frameReady(celsius, vis.copy(), st);

    if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
      emit errorOccurred("VIDIOC_QBUF recycle failed.");
      break;
    }
  }

  xioctl(fd, VIDIOC_STREAMOFF, &type);
  for (auto &b : bufs) {
    if (b.start && b.start != MAP_FAILED) {
      munmap(b.start, b.length);
    }
  }
  close(fd);
}
