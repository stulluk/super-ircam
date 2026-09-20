#include "Colormap.h"
#include "ThermalMath.h"

#include <opencv2/imgproc.hpp>

#include <cmath>

namespace {

void hslToRgb(float h, float s, float l, unsigned *r, unsigned *g, unsigned *b) {
  const float c = (1.0f - std::fabs(2.0f * l - 1.0f)) * s;
  const float hp = std::fmod(h / 60.0f + 6.0f, 6.0f);
  const float x = c * (1.0f - std::fabs(std::fmod(hp, 2.0f) - 1.0f));
  float rf = 0, gf = 0, bf = 0;
  if (hp < 1) {
    rf = c;
    gf = x;
  } else if (hp < 2) {
    rf = x;
    gf = c;
  } else if (hp < 3) {
    gf = c;
    bf = x;
  } else if (hp < 4) {
    gf = x;
    bf = c;
  } else if (hp < 5) {
    rf = x;
    bf = c;
  } else {
    rf = c;
    bf = x;
  }
  const float m = l - c / 2.0f;
  *r = static_cast<unsigned>(clampf((rf + m) * 255.0f, 0.0f, 255.0f));
  *g = static_cast<unsigned>(clampf((gf + m) * 255.0f, 0.0f, 255.0f));
  *b = static_cast<unsigned>(clampf((bf + m) * 255.0f, 0.0f, 255.0f));
}

cv::Mat makeHslLut(int direction) {
  cv::Mat lut(256, 1, CV_8UC3);
  for (int i = 0; i < 256; ++i) {
    const float t = (direction < 0) ? (255 - i) / 255.0f : i / 255.0f;
    const float h = 300.0f * t;  // violet -> red or reverse
    unsigned r, g, b;
    hslToRgb(h, 1.0f, 0.5f, &r, &g, &b);
    lut.at<cv::Vec3b>(i, 0) = cv::Vec3b(static_cast<uchar>(b), static_cast<uchar>(g),
                                        static_cast<uchar>(r));
  }
  return lut;
}

cv::Mat makeR2bLut(int ramp) {
  cv::Mat lut(256, 1, CV_8UC3);
  for (int i = 0; i < 256; ++i) {
    const float t = i / 255.0f;
    float h = 240.0f * (1.0f - t);
    float l = 0.5f;
    if (ramp > 0) {
      l = 0.4f + 0.5f * t;
    } else if (ramp < 0) {
      l = 0.6f * (1.0f - t * 0.5f);
    }
    if (std::abs(ramp) == 2) {
      const float ang = 180.0f * t;
      const float uc = std::sin(ang * static_cast<float>(M_PI) / 180.0f);
      l = (ramp > 0) ? (0.5f + 0.45f * uc) : (0.5f - 0.45f * uc);
      l = clampf(l, 0.05f, 0.95f);
    }
    unsigned r, g, b;
    hslToRgb(h, 1.0f, l, &r, &g, &b);
    lut.at<cv::Vec3b>(i, 0) = cv::Vec3b(static_cast<uchar>(b), static_cast<uchar>(g),
                                        static_cast<uchar>(r));
  }
  return lut;
}

cv::Mat makeIronbowLut() {
  static const cv::Vec3b stops[] = {
      {0, 0, 0}, {80, 0, 32}, {80, 0, 160}, {0, 80, 255}, {80, 220, 255}, {255, 255, 255}};
  cv::Mat lut(256, 1, CV_8UC3);
  const int n = 6;
  for (int i = 0; i < 256; ++i) {
    const float x = (i / 255.0f) * (n - 1);
    const int a = std::min(n - 2, static_cast<int>(x));
    const float f = x - static_cast<float>(a);
    const cv::Vec3b p = stops[a];
    const cv::Vec3b q = stops[a + 1];
    lut.at<cv::Vec3b>(i, 0) = cv::Vec3b(
        static_cast<uchar>(p[0] + (q[0] - p[0]) * f),
        static_cast<uchar>(p[1] + (q[1] - p[1]) * f),
        static_cast<uchar>(p[2] + (q[2] - p[2]) * f));
  }
  return lut;
}

CmapEntry stock(const char *name, int id, bool inv = false) {
  CmapEntry e;
  e.name = name;
  e.opencvId = id;
  e.invertBgr = inv;
  return e;
}

CmapEntry custom(const char *name, const cv::Mat &lut) {
  CmapEntry e;
  e.name = name;
  e.opencvId = -2;
  e.invertBgr = false;
  e.lut = lut;
  return e;
}

}  // namespace

QVector<CmapEntry> &cmapTable() {
  static QVector<CmapEntry> table;
  if (!table.isEmpty()) {
    return table;
  }
  table.push_back(stock("None", -1));
  table.push_back(stock("Autumn", cv::COLORMAP_AUTUMN));
  table.push_back(stock("Inv Autumn", cv::COLORMAP_AUTUMN, true));
  table.push_back(stock("Bone", cv::COLORMAP_BONE));
  table.push_back(stock("Jet", cv::COLORMAP_JET));
  table.push_back(stock("Inv Jet", cv::COLORMAP_JET, true));
  table.push_back(stock("Winter", cv::COLORMAP_WINTER));
  table.push_back(stock("Rainbow", cv::COLORMAP_RAINBOW));
  table.push_back(stock("Inv Rainbow", cv::COLORMAP_RAINBOW, true));
  table.push_back(stock("Ocean", cv::COLORMAP_OCEAN));
  table.push_back(stock("Inv Ocean", cv::COLORMAP_OCEAN, true));
  table.push_back(stock("Summer", cv::COLORMAP_SUMMER));
  table.push_back(stock("Spring", cv::COLORMAP_SPRING));
  table.push_back(stock("Cool", cv::COLORMAP_COOL));
  table.push_back(stock("HSV", cv::COLORMAP_HSV));
  table.push_back(stock("Inv HSV", cv::COLORMAP_HSV, true));
  table.push_back(stock("Pink", cv::COLORMAP_PINK));
  table.push_back(stock("Hot", cv::COLORMAP_HOT));
  table.push_back(stock("Cold", cv::COLORMAP_HOT, true));
  table.push_back(stock("Parula", cv::COLORMAP_PARULA));
  table.push_back(stock("Magma", cv::COLORMAP_MAGMA));
  table.push_back(stock("Inferno", cv::COLORMAP_INFERNO));
  table.push_back(stock("Plasma", cv::COLORMAP_PLASMA));
  table.push_back(stock("Viridis", cv::COLORMAP_VIRIDIS));
  table.push_back(stock("Cividis", cv::COLORMAP_CIVIDIS));
  table.push_back(stock("Twilight", cv::COLORMAP_TWILIGHT));
  table.push_back(stock("Twilight Shift", cv::COLORMAP_TWILIGHT_SHIFTED));
  table.push_back(stock("Turbo", cv::COLORMAP_TURBO));
  table.push_back(stock("Inv Turbo", cv::COLORMAP_TURBO, true));
  table.push_back(stock("Deepgreen", cv::COLORMAP_DEEPGREEN));
  table.push_back(custom("HSL", makeHslLut(-1)));
  table.push_back(custom("Inv HSL", makeHslLut(1)));
  table.push_back(custom("R2B", makeR2bLut(0)));
  table.push_back(custom("C2BlackL", makeR2bLut(-1)));
  table.push_back(custom("C2BlackUC", makeR2bLut(-2)));
  table.push_back(custom("C2WhiteL", makeR2bLut(1)));
  table.push_back(custom("C2WhiteUC", makeR2bLut(2)));
  table.push_back(custom("Ironbow", makeIronbowLut()));
  return table;
}

QStringList cmapNames() {
  QStringList names;
  for (const auto &e : cmapTable()) {
    names << e.name;
  }
  return names;
}

int defaultCmapIndex() {
  const auto &t = cmapTable();
  for (int i = 0; i < t.size(); ++i) {
    if (t[i].name == "Jet") {
      return i;
    }
  }
  return 0;
}

cv::Mat applyReduxCmap(const cv::Mat &gray8, int cmapIndex) {
  const auto &table = cmapTable();
  if (cmapIndex < 0 || cmapIndex >= table.size()) {
    cmapIndex = defaultCmapIndex();
  }
  const CmapEntry &e = table[cmapIndex];
  if (e.opencvId == -1) {
    cv::Mat bgr;
    cv::cvtColor(gray8, bgr, cv::COLOR_GRAY2BGR);
    return bgr;
  }
  cv::Mat color;
  if (e.opencvId == -2) {
    cv::applyColorMap(gray8, color, e.lut);
  } else {
    cv::applyColorMap(gray8, color, e.opencvId);
    if (e.invertBgr) {
      cv::cvtColor(color, color, cv::COLOR_BGR2RGB);
    }
  }
  return color;
}

cv::Mat celsiusToGray(const QVector<float> &celsius, int width, int height,
                      float minC, float maxC) {
  cv::Mat gray(height, width, CV_8UC1);
  const float span = std::max(0.5f, maxC - minC);
  for (int y = 0; y < height; ++y) {
    auto *row = gray.ptr<uint8_t>(y);
    for (int x = 0; x < width; ++x) {
      const float n = clampf((celsius[y * width + x] - minC) / span, 0.0f, 1.0f);
      row[x] = static_cast<uint8_t>(n * 255.0f);
    }
  }
  return gray;
}

QImage matToQImage(const cv::Mat &bgr) {
  cv::Mat rgb;
  cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
  return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                QImage::Format_RGB888)
      .copy();
}
