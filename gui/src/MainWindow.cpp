#include "MainWindow.h"
#include "DeviceFind.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

namespace {

QWidget *labeledSlider(const QString &title, QSlider *slider, QLabel **valueOut) {
  auto *w = new QWidget;
  auto *box = new QVBoxLayout(w);
  box->setContentsMargins(0, 0, 0, 0);
  auto *row = new QHBoxLayout;
  row->addWidget(new QLabel(title));
  auto *val = new QLabel;
  row->addStretch(1);
  row->addWidget(val);
  box->addLayout(row);
  box->addWidget(slider);
  *valueOut = val;
  return w;
}

/** Append a title label followed by a control to a column layout. */
void addLabeled(QVBoxLayout *box, const QString &title, QWidget *widget) {
  box->addWidget(new QLabel(title));
  box->addWidget(widget);
}

}  // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Super IRCam");
  setWindowIcon(QIcon::fromTheme("super-ircam", QIcon("/usr/share/icons/hicolor/128x128/apps/super-ircam.png")));
  resize(1480, 780);
  setMinimumSize(1180, 700);
  setStyleSheet(
      "QMainWindow, QWidget { background: #1b1d22; color: #e8e8e8; }"
      "QLabel { color: #e8e8e8; }"
      "QGroupBox { border: 1px solid #3a3f4b; border-radius: 4px; margin-top: 10px; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
      "QPushButton { background: #2d3340; color: #e8e8e8; border: 1px solid #444; "
      "  border-radius: 4px; padding: 6px 10px; }"
      "QPushButton:checked { background: #c45c26; border-color: #e07830; }"
      "QPushButton:hover { border-color: #888; }"
      "QSlider::groove:horizontal { height: 6px; background: #333; border-radius: 3px; }"
      "QSlider::handle:horizontal { width: 14px; background: #c45c26; margin: -5px 0; "
      "  border-radius: 7px; }"
      "QComboBox, QDoubleSpinBox { background: #2d3340; color: #e8e8e8; padding: 4px; }"
      "QScrollArea { background: #1b1d22; border: none; }"
      "QScrollBar:vertical {"
      "  background: #111318; width: 16px; margin: 0; border-left: 1px solid #5a6270; }"
      "QScrollBar::handle:vertical {"
      "  background: #e07830; min-height: 36px; margin: 2px; border-radius: 6px; }"
      "QScrollBar::handle:vertical:hover { background: #ff9a3c; }"
      "QScrollBar::handle:vertical:pressed { background: #c45c26; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0; background: none; border: none; }"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      "  background: #2a2e38; }"
      "QScrollBar:horizontal {"
      "  background: #111318; height: 16px; margin: 0; border-top: 1px solid #5a6270; }"
      "QScrollBar::handle:horizontal {"
      "  background: #e07830; min-width: 36px; margin: 2px; border-radius: 6px; }"
      "QScrollBar::handle:horizontal:hover { background: #ff9a3c; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0; background: none; border: none; }"
      "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
      "  background: #2a2e38; }");

  auto *central = new QWidget(this);
  setCentralWidget(central);
  auto *root = new QHBoxLayout(central);

  m_image = new ImageCanvas;
  m_image->setMinimumSize(700, 520);
  m_image->setAlignment(Qt::AlignCenter);
  m_image->setStyleSheet("background: #0b0c0f; color: #888; border: 1px solid #333;");
  m_image->setText("Waiting for camera…");
  connect(m_image, &ImageCanvas::canvasPressed, this, &MainWindow::onCanvasPressed);
  root->addWidget(m_image, 1);

  auto *side = new QWidget;
  side->setMinimumWidth(540);
  auto *box = new QVBoxLayout(side);
  box->setSpacing(6);
  root->addWidget(side, 0);

  auto *title = new QLabel("Super IRCam");
  title->setStyleSheet("font-weight: 600; font-size: 18px;");
  box->addWidget(title);

  m_deviceLabel = new QLabel("Device: searching…");
  m_deviceLabel->setWordWrap(true);
  box->addWidget(m_deviceLabel);

  m_minLabel = new QLabel("Min: —");
  m_avgLabel = new QLabel("Avg: —");
  m_maxLabel = new QLabel("Max: —");
  m_centerLabel = new QLabel("Center: —");
  m_spotsLabel = new QLabel("User spots: 0 (left-click add, right-click clear)");
  m_spotsLabel->setWordWrap(true);
  auto *stats = new QGridLayout;
  stats->setContentsMargins(0, 0, 0, 0);
  stats->addWidget(m_minLabel, 0, 0);
  stats->addWidget(m_avgLabel, 0, 1);
  stats->addWidget(m_maxLabel, 1, 0);
  stats->addWidget(m_centerLabel, 1, 1);
  box->addLayout(stats);
  box->addWidget(m_spotsLabel);

  auto *leftCol = new QWidget;
  auto *rightCol = new QWidget;
  auto *left = new QVBoxLayout(leftCol);
  auto *right = new QVBoxLayout(rightCol);
  left->setContentsMargins(0, 0, 0, 0);
  right->setContentsMargins(0, 0, 0, 0);
  left->setSpacing(6);
  right->setSpacing(6);

  m_cmap = new QComboBox;
  m_cmap->addItems(cmapNames());
  m_cmap->setCurrentIndex(defaultCmapIndex());
  addLabeled(left, "Colormap (Redux set)", m_cmap);

  m_layout = new QComboBox;
  m_layout->addItems({"Thermal", "Image", "Img+Therm wide", "Img+Therm high"});
  addLabeled(left, "Layout", m_layout);

  m_inter = new QComboBox;
  m_inter->addItems({"Nearest", "Linear", "Cubic", "Area", "Lanczos4", "Lin Exact", "Near Exact"});
  m_inter->setCurrentIndex(2);
  addLabeled(left, "Interpolation", m_inter);

  QLabel *zoomVal = nullptr;
  m_zoom = new QSlider(Qt::Horizontal);
  m_zoom->setRange(1, 5);
  m_zoom->setValue(3);
  left->addWidget(labeledSlider("Zoom", m_zoom, &zoomVal));
  zoomVal->setText("3x");
  connect(m_zoom, &QSlider::valueChanged, this, [zoomVal](int v) { zoomVal->setText(QString("%1x").arg(v)); });

  QLabel *blurVal = nullptr;
  m_blur = new QSlider(Qt::Horizontal);
  m_blur->setRange(0, 8);
  m_blur->setValue(0);
  left->addWidget(labeledSlider("Blur", m_blur, &blurVal));
  blurVal->setText("0");
  connect(m_blur, &QSlider::valueChanged, this, [blurVal](int v) { blurVal->setText(QString::number(v)); });

  QLabel *conVal = nullptr;
  m_contrast = new QSlider(Qt::Horizontal);
  m_contrast->setRange(5, 30);
  m_contrast->setValue(10);
  left->addWidget(labeledSlider("Contrast", m_contrast, &conVal));
  conVal->setText("1.0");
  connect(m_contrast, &QSlider::valueChanged, this,
          [conVal](int v) { conVal->setText(QString::number(v / 10.0, 'f', 1)); });
  left->addStretch(1);

  QLabel *thrVal = nullptr;
  m_threshold = new QSlider(Qt::Horizontal);
  m_threshold->setRange(0, 40);
  m_threshold->setValue(2);
  right->addWidget(labeledSlider("Threshold from avg", m_threshold, &thrVal));
  thrVal->setText("2.0");
  connect(m_threshold, &QSlider::valueChanged, this,
          [thrVal](int v) { thrVal->setText(QString::number(v, 'f', 1)); });

  QLabel *rotVal = nullptr;
  m_rotate = new QSlider(Qt::Horizontal);
  m_rotate->setRange(0, 3);
  m_rotate->setValue(0);
  right->addWidget(labeledSlider("Rotate (90° steps)", m_rotate, &rotVal));
  rotVal->setText("0°");
  connect(m_rotate, &QSlider::valueChanged, this,
          [rotVal](int v) { rotVal->setText(QString("%1°").arg(v * 90)); });

  m_rulers = new QComboBox;
  m_rulers->addItems({"Off", "One temp", "Cross hair", "Horizontal", "Vertical", "Both"});
  addLabeled(right, "Rulers", m_rulers);

  m_rangeMode = new QComboBox;
  m_rangeMode->addItems({"None", "Clip", "Grow"});
  addLabeled(right, "Locked range method", m_rangeMode);

  m_fahrenheit = new QCheckBox("Show Fahrenheit");
  m_freeze = new QCheckBox("Freeze frame");
  m_histogram = new QCheckBox("Histogram (gray maps)");
  m_lockRange = new QCheckBox("Lock colormap auto-range");
  m_alarmOn = new QCheckBox("High-temp alarm");
  right->addWidget(m_fahrenheit);
  right->addWidget(m_freeze);
  right->addWidget(m_histogram);
  right->addWidget(m_lockRange);
  right->addWidget(m_alarmOn);
  m_alarmC = new QDoubleSpinBox;
  m_alarmC->setRange(-20.0, 400.0);
  m_alarmC->setSuffix(" °C");
  m_alarmC->setValue(80.0);
  right->addWidget(m_alarmC);
  right->addStretch(1);

  auto *cols = new QHBoxLayout;
  cols->setContentsMargins(0, 0, 0, 0);
  cols->setSpacing(12);
  cols->addWidget(leftCol, 1);
  cols->addWidget(rightCol, 1);
  box->addLayout(cols);

  m_snapshot = new QPushButton("Snapshot (PNG + RAW)");
  m_record = new QPushButton("Record AVI");
  m_record->setCheckable(true);
  m_reset = new QPushButton("Reset defaults");
  auto *btns = new QHBoxLayout;
  btns->setContentsMargins(0, 0, 0, 0);
  btns->addWidget(m_snapshot);
  btns->addWidget(m_record);
  btns->addWidget(m_reset);
  box->addLayout(btns);
  m_recordLabel = new QLabel;
  m_recordLabel->setWordWrap(true);
  box->addWidget(m_recordLabel);

  m_status = new QLabel;
  m_status->setWordWrap(true);
  m_status->setStyleSheet("color: #9ad;");
  box->addWidget(m_status);
  box->addStretch(1);

  connect(m_snapshot, &QPushButton::clicked, this, &MainWindow::onSnapshot);
  connect(m_record, &QPushButton::clicked, this, &MainWindow::onRecordToggle);
  connect(m_reset, &QPushButton::clicked, this, &MainWindow::onReset);

  ThermalDevice dev;
  QString err;
  if (!findThermalCamera(&dev, &err)) {
    m_deviceLabel->setText("Device: not found");
    m_status->setText(err);
    return;
  }
  m_devicePath = dev.path;
  m_deviceLabel->setText(
      QString("Device: %1\nUSB %2:%3").arg(dev.path, dev.vendorId, dev.productId));

  m_capture = new CaptureThread(this);
  m_capture->setDevicePath(dev.path);
  connect(m_capture, &CaptureThread::frameReady, this, &MainWindow::onFrame);
  connect(m_capture, &CaptureThread::errorOccurred, this, &MainWindow::onError);
  m_capture->start();
}

MainWindow::~MainWindow() {
  stopRecorder();
  if (m_capture) {
    m_capture->stop();
    m_capture->wait(2000);
  }
}

void MainWindow::setCmapIndex(int index) {
  m_cmap->setCurrentIndex(index);
}

void MainWindow::setZoom(int zoom) {
  m_zoom->setValue(zoom);
}

void MainWindow::setBlur(int radius) {
  m_blur->setValue(radius);
}

bool MainWindow::saveWindowShot(const QString &path) {
  return grab().save(path);
}

void MainWindow::onReset() {
  m_cmap->setCurrentIndex(defaultCmapIndex());
  m_layout->setCurrentIndex(0);
  m_inter->setCurrentIndex(2);
  m_zoom->setValue(3);
  m_blur->setValue(0);
  m_contrast->setValue(10);
  m_threshold->setValue(2);
  m_rotate->setValue(0);
  m_rulers->setCurrentIndex(0);
  m_rangeMode->setCurrentIndex(0);
  m_fahrenheit->setChecked(false);
  m_freeze->setChecked(false);
  m_histogram->setChecked(false);
  m_lockRange->setChecked(false);
  m_alarmOn->setChecked(false);
  m_spots.clear();
  m_rulerX = kThermalWidth / 2;
  m_rulerY = kThermalHeight / 2;
  m_lockInit = false;
  m_status->setText("Defaults restored.");
}

void MainWindow::onError(const QString &message) {
  m_status->setText(message);
}

void MainWindow::onSnapshot() {
  if (m_lastPixmap.isNull()) {
    m_status->setText("No frame to capture yet.");
    return;
  }
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
  QDir().mkpath(dir);
  const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
  const QString png = QString("%1/super-ircam-%2.png").arg(dir, stamp);
  const QString raw = QString("%1/super-ircam-%2.raw").arg(dir, stamp);
  const bool okPng = m_lastPixmap.save(png);
  bool okRaw = false;
  if (m_capture) {
    const QVector<quint16> kelvin = m_capture->latestKelvin();
    QFile f(raw);
    if (f.open(QIODevice::WriteOnly)) {
      f.write(reinterpret_cast<const char *>(kelvin.constData()),
              kelvin.size() * static_cast<int>(sizeof(quint16)));
      okRaw = true;
    }
  }
  m_status->setText(QString("Snapshot %1 / raw %2").arg(okPng ? png : "fail", okRaw ? raw : "fail"));
}

void MainWindow::onRecordToggle() {
  if (m_recording) {
    stopRecorder();
    m_record->setChecked(false);
    m_recordLabel->setText("Recording stopped. " + m_recordPath);
    return;
  }
  if (m_lastPixmap.isNull()) {
    m_record->setChecked(false);
    m_status->setText("No frame to record yet.");
    return;
  }
  startRecorder(m_lastPixmap.size());
}

void MainWindow::startRecorder(const QSize &frameSize) {
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
  QDir().mkpath(dir);
  const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
  m_recordPath = QString("%1/super-ircam-%2_output.avi").arg(dir, stamp);
  const cv::Size sz(frameSize.width(), frameSize.height());
  bool ok = m_writer.open(m_recordPath.toStdString(),
                          cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), 25.0, sz, true);
  if (!ok) {
    ok = m_writer.open(m_recordPath.toStdString(),
                       cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 25.0, sz, true);
  }
  if (!ok) {
    m_status->setText("Could not open AVI writer.");
    m_recording = false;
    m_record->setChecked(false);
    return;
  }
  m_recording = true;
  m_recFrames = 0;
  m_recordLabel->setText("Recording " + m_recordPath);
}

void MainWindow::stopRecorder() {
  if (m_writer.isOpened()) {
    m_writer.release();
  }
  m_recording = false;
}

float MainWindow::displayTemp(float celsius) const {
  return m_fahrenheit->isChecked() ? celsiusToFahrenheit(celsius) : celsius;
}

QString MainWindow::tempText(float celsius) const {
  const QString unit = m_fahrenheit->isChecked() ? QString::fromUtf8("°F")
                                                 : QString::fromUtf8("°C");
  return QString("%1%2").arg(displayTemp(celsius), 0, 'f', 1).arg(unit);
}

void MainWindow::applyBlurInPlace(QVector<float> *celsius) const {
  const int r = m_blur->value();
  if (r <= 0 || celsius->size() != kThermalWidth * kThermalHeight) {
    return;
  }
  QVector<float> src = *celsius;
  for (int y = 0; y < kThermalHeight; ++y) {
    for (int x = 0; x < kThermalWidth; ++x) {
      float sum = 0;
      int n = 0;
      for (int dy = -r; dy <= r; ++dy) {
        const int yy = y + dy;
        if (yy < 0 || yy >= kThermalHeight) {
          continue;
        }
        for (int dx = -r; dx <= r; ++dx) {
          const int xx = x + dx;
          if (xx < 0 || xx >= kThermalWidth) {
            continue;
          }
          sum += src[yy * kThermalWidth + xx];
          n++;
        }
      }
      (*celsius)[y * kThermalWidth + x] = sum / static_cast<float>(n);
    }
  }
}

void MainWindow::colorRange(float *minC, float *maxC) const {
  float lo = m_stats.minC;
  float hi = m_stats.maxC;
  if (m_lockRange->isChecked()) {
    if (!m_lockInit) {
      return;  // caller sets lock on first valid
    }
    const RangeMode mode = static_cast<RangeMode>(m_rangeMode->currentIndex());
    if (mode == RangeMode::Grow) {
      lo = std::min(m_lockMin, m_stats.minC);
      hi = std::max(m_lockMax, m_stats.maxC);
    } else if (mode == RangeMode::Clip) {
      lo = m_lockMin;
      hi = m_lockMax;
    } else {
      lo = m_lockMin;
      hi = m_lockMax;
    }
  }
  *minC = lo;
  *maxC = hi;
}

int MainWindow::opencvInter() const {
  switch (static_cast<InterMode>(m_inter->currentIndex())) {
  case InterMode::Nearest:
    return cv::INTER_NEAREST;
  case InterMode::Linear:
    return cv::INTER_LINEAR;
  case InterMode::Cubic:
    return cv::INTER_CUBIC;
  case InterMode::Area:
    return cv::INTER_AREA;
  case InterMode::Lanczos:
    return cv::INTER_LANCZOS4;
  case InterMode::LinExact:
    return cv::INTER_LINEAR_EXACT;
  case InterMode::NearExact:
    return cv::INTER_NEAREST_EXACT;
  }
  return cv::INTER_CUBIC;
}

void MainWindow::updateTempLabels() {
  m_minLabel->setText("Min: " + tempText(m_stats.minC));
  m_avgLabel->setText("Avg: " + tempText(m_stats.avgC));
  m_maxLabel->setText("Max: " + tempText(m_stats.maxC));
  m_centerLabel->setText("Center: " + tempText(m_stats.centerC));
  m_spotsLabel->setText(QString("User spots: %1 (left-click add, right-click clear)")
                            .arg(m_spots.size()));
}

void MainWindow::drawMarker(QImage *img, int x, int y, const QString &text, QRgb rgb) const {
  if (x < 0 || y < 0 || x >= img->width() || y >= img->height()) {
    return;
  }
  QPainter p(img);
  p.setRenderHint(QPainter::Antialiasing, false);
  p.setPen(QPen(QColor(0, 0, 0), 3));
  p.drawLine(x - 9, y, x + 9, y);
  p.drawLine(x, y - 9, x, y + 9);
  p.setPen(QPen(QColor(rgb), 1));
  p.drawLine(x - 8, y, x + 8, y);
  p.drawLine(x, y - 8, x, y + 8);
  QFont f = p.font();
  f.setPixelSize(12);
  f.setBold(true);
  p.setFont(f);
  const QFontMetrics fm(f);
  const QRect br = fm.boundingRect(text).adjusted(-3, -1, 3, 2);
  int tx = x + 10;
  int ty = y - 6;
  if (tx + br.width() > img->width()) {
    tx = x - br.width() - 10;
  }
  if (ty < 0) {
    ty = y + 12;
  }
  if (ty + br.height() > img->height()) {
    ty = img->height() - br.height();
  }
  p.fillRect(tx, ty, br.width(), br.height(), QColor(0, 0, 0, 170));
  p.setPen(QColor(rgb));
  p.drawText(tx + 3, ty + fm.ascent(), text);
}

void MainWindow::drawOverlays(QImage *img, int ox, int oy, int scale) const {
  const int rot = m_rotate->value();
  const auto map = [&](int sx, int sy) {
    int x = sx;
    int y = sy;
    if (rot == 1) {
      x = kThermalHeight - 1 - sy;
      y = sx;
    } else if (rot == 2) {
      x = kThermalWidth - 1 - sx;
      y = kThermalHeight - 1 - sy;
    } else if (rot == 3) {
      x = sy;
      y = kThermalWidth - 1 - sx;
    }
    return QPoint(ox + x * scale + scale / 2, oy + y * scale + scale / 2);
  };
  const QPoint mx = map(m_stats.maxX, m_stats.maxY);
  const QPoint mn = map(m_stats.minX, m_stats.minY);
  const QPoint ch = map(kThermalWidth / 2, kThermalHeight / 2);
  drawMarker(img, mx.x(), mx.y(), QString("Max %1").arg(tempText(m_stats.maxC)), qRgb(255, 80, 80));
  drawMarker(img, mn.x(), mn.y(), QString("Min %1").arg(tempText(m_stats.minC)), qRgb(80, 220, 255));
  drawMarker(img, ch.x(), ch.y(), QString("%1").arg(tempText(m_stats.centerC)), qRgb(255, 255, 255));

  const RulerMode rm = static_cast<RulerMode>(m_rulers->currentIndex());
  if (rm != RulerMode::Off) {
    QPainter p(img);
    p.setPen(QPen(QColor(255, 255, 255, 180), 1, Qt::DashLine));
    const QPoint rp = map(m_rulerX, m_rulerY);
    if (rm == RulerMode::CrossHair || rm == RulerMode::Both || rm == RulerMode::Horiz) {
      p.drawLine(ox, rp.y(), ox + kThermalWidth * scale, rp.y());
    }
    if (rm == RulerMode::CrossHair || rm == RulerMode::Both || rm == RulerMode::Vert) {
      p.drawLine(rp.x(), oy, rp.x(), oy + kThermalHeight * scale);
    }
    const float t = m_celsius[m_rulerY * kThermalWidth + m_rulerX];
    drawMarker(img, rp.x(), rp.y(), QString("R %1").arg(tempText(t)), qRgb(255, 220, 80));
  }

  for (const UserSpot &s : m_spots) {
    const QPoint pt = map(s.x, s.y);
    const float t = m_celsius[s.y * kThermalWidth + s.x];
    drawMarker(img, pt.x(), pt.y(), tempText(t), qRgb(255, 180, 40));
  }
}

QImage MainWindow::renderView() {
  if (m_celsius.size() != kThermalWidth * kThermalHeight) {
    return {};
  }
  float minC = m_stats.minC;
  float maxC = m_stats.maxC;
  if (m_lockRange->isChecked()) {
    if (!m_lockInit) {
      m_lockMin = minC;
      m_lockMax = maxC;
      m_lockInit = true;
    }
    colorRange(&minC, &maxC);
    m_lockMin = minC;
    m_lockMax = maxC;
  } else {
    m_lockInit = false;
  }

  cv::Mat gray = celsiusToGray(m_celsius, kThermalWidth, kThermalHeight, minC, maxC);
  if (m_histogram->isChecked()) {
    cv::equalizeHist(gray, gray);
  }
  cv::Mat color = applyReduxCmap(gray, m_cmap->currentIndex());
  const double alpha = m_contrast->value() / 10.0;
  color.convertTo(color, -1, alpha, 0);

  if (m_alarmOn->isChecked()) {
    const float alarmC = static_cast<float>(m_alarmC->value());
    for (int y = 0; y < kThermalHeight; ++y) {
      for (int x = 0; x < kThermalWidth; ++x) {
        if (m_celsius[y * kThermalWidth + x] >= alarmC) {
          color.at<cv::Vec3b>(y, x) = cv::Vec3b(255, 255, 255);
        }
      }
    }
  }

  cv::Mat visual;
  if (!m_visualY.isNull()) {
    cv::Mat v(kThermalHeight, kThermalWidth, CV_8UC1,
              const_cast<uchar *>(m_visualY.constBits()), m_visualY.bytesPerLine());
    visual = applyReduxCmap(v, m_cmap->currentIndex());
    visual.convertTo(visual, -1, alpha, 0);
  } else {
    visual = color.clone();
  }

  const LayoutMode layout = static_cast<LayoutMode>(m_layout->currentIndex());
  cv::Mat composed;
  if (layout == LayoutMode::Image) {
    composed = visual;
  } else if (layout == LayoutMode::DualWide) {
    cv::hconcat(visual, color, composed);
  } else if (layout == LayoutMode::DualHigh) {
    cv::vconcat(visual, color, composed);
  } else {
    composed = color;
  }

  const int z = m_zoom->value();
  cv::Mat scaled;
  cv::resize(composed, scaled, cv::Size(composed.cols * z, composed.rows * z), 0, 0,
             opencvInter());

  const int rot = m_rotate->value();
  if (rot == 1) {
    cv::rotate(scaled, scaled, cv::ROTATE_90_CLOCKWISE);
  } else if (rot == 2) {
    cv::rotate(scaled, scaled, cv::ROTATE_180);
  } else if (rot == 3) {
    cv::rotate(scaled, scaled, cv::ROTATE_90_COUNTERCLOCKWISE);
  }

  QImage img = matToQImage(scaled);
  // Overlays sit on the thermal pane. Dual-wide: thermal is the right half.
  int ox = 0;
  int oy = 0;
  if (layout == LayoutMode::DualWide) {
    ox = kThermalWidth * z;
  } else if (layout == LayoutMode::DualHigh) {
    oy = kThermalHeight * z;
  }
  if (layout != LayoutMode::Image) {
    drawOverlays(&img, ox, oy, z);
  } else {
    drawOverlays(&img, 0, 0, z);
  }
  return img;
}

QPoint MainWindow::labelToSensor(const QPoint &labelPos) const {
  if (m_lastPixmap.isNull()) {
    return {-1, -1};
  }
  const QRect cr = m_image->contentsRect();
  const int x0 = cr.x() + (cr.width() - m_lastPixmap.width()) / 2;
  const int y0 = cr.y() + (cr.height() - m_lastPixmap.height()) / 2;
  int ix = labelPos.x() - x0;
  int iy = labelPos.y() - y0;
  ix = std::clamp(ix, 0, m_lastPixmap.width() - 1);
  iy = std::clamp(iy, 0, m_lastPixmap.height() - 1);
  const int z = std::max(1, m_zoom->value());
  const LayoutMode layout = static_cast<LayoutMode>(m_layout->currentIndex());
  if (layout == LayoutMode::DualWide) {
    ix -= kThermalWidth * z;
    if (ix < 0) {
      ix += kThermalWidth * z;  // clicked visual half: still map into 256
    }
  } else if (layout == LayoutMode::DualHigh) {
    iy -= kThermalHeight * z;
    if (iy < 0) {
      iy += kThermalHeight * z;
    }
  }
  int sx = ix / z;
  int sy = iy / z;
  const int rot = m_rotate->value();
  if (rot == 1) {  // 90 CW: display(x,y) <- src(y, h-1-x) inverse
    const int nsx = sy;
    const int nsy = kThermalWidth - 1 - sx;
    sx = nsx;
    sy = nsy;
  } else if (rot == 2) {
    sx = kThermalWidth - 1 - sx;
    sy = kThermalHeight - 1 - sy;
  } else if (rot == 3) {
    const int nsx = kThermalHeight - 1 - sy;
    const int nsy = sx;
    sx = nsx;
    sy = nsy;
  }
  sx = std::clamp(sx, 0, kThermalWidth - 1);
  sy = std::clamp(sy, 0, kThermalHeight - 1);
  return {sx, sy};
}

void MainWindow::onCanvasPressed(const QPoint &localPos, Qt::MouseButton button) {
  if (m_celsius.isEmpty()) {
    return;
  }
  const QPoint sensor = labelToSensor(localPos);
  if (sensor.x() < 0) {
    return;
  }
  if (button == Qt::RightButton) {
    m_spots.clear();
    m_spotsLabel->setText("User spots: 0 (left-click add, right-click clear)");
    return;
  }
  if (button == Qt::LeftButton) {
    const RulerMode rm = static_cast<RulerMode>(m_rulers->currentIndex());
    if (rm != RulerMode::Off) {
      m_rulerX = sensor.x();
      m_rulerY = sensor.y();
    } else if (m_spots.size() < 12) {
      m_spots.push_back({sensor.x(), sensor.y()});
    } else {
      m_spots.pop_front();
      m_spots.push_back({sensor.x(), sensor.y()});
    }
    updateTempLabels();
  }
}

void MainWindow::onFrame(const QVector<float> &celsius, const QImage &visualY,
                         const ThermalStats &stats) {
  if (!m_freeze->isChecked()) {
    m_celsius = celsius;
    applyBlurInPlace(&m_celsius);
    m_stats = stats;
    m_visualY = visualY;
  }
  if (!m_stats.valid) {
    m_status->setText("Calibrating (NUC)…");
    m_image->setText("Calibrating…");
    return;
  }
  if (m_status->text().startsWith("Calibrating")) {
    m_status->clear();
  }
  updateTempLabels();
  const QImage img = renderView();
  m_lastPixmap = QPixmap::fromImage(img);
  m_image->setPixmap(m_lastPixmap);

  if (m_recording && m_writer.isOpened()) {
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgb.height(), rgb.width(), CV_8UC3, rgb.bits(), rgb.bytesPerLine());
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_RGB2BGR);
    if (bgr.size() != cv::Size(static_cast<int>(m_writer.get(cv::CAP_PROP_FRAME_WIDTH)),
                               static_cast<int>(m_writer.get(cv::CAP_PROP_FRAME_HEIGHT)))) {
      cv::resize(bgr, bgr,
                 cv::Size(static_cast<int>(m_writer.get(cv::CAP_PROP_FRAME_WIDTH)),
                          static_cast<int>(m_writer.get(cv::CAP_PROP_FRAME_HEIGHT))));
    }
    m_writer.write(bgr);
    ++m_recFrames;
    m_recordLabel->setText(QString("Recording %1 frames → %2").arg(m_recFrames).arg(m_recordPath));
  }
  emit frameReceived();
}
