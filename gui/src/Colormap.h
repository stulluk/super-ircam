#ifndef COLORMAP_H
#define COLORMAP_H

#include <QImage>
#include <QString>
#include <QStringList>
#include <QVector>

#include <opencv2/core.hpp>

/** Redux 0.9.3 colormap table (37 entries, Jet is the default). */
struct CmapEntry {
  QString name;
  int opencvId;  // -1 = none, -2 = custom LUT
  bool invertBgr;
  cv::Mat lut;   // 256x1 CV_8UC3 when opencvId == -2
};

QVector<CmapEntry> &cmapTable();
QStringList cmapNames();
int defaultCmapIndex();

/**
 * Colorize an 8-bit gray image with the Redux colormap at index.
 * src is CV_8UC1 256x192 (or any size).
 */
cv::Mat applyReduxCmap(const cv::Mat &gray8, int cmapIndex);

/** Build an 8-bit gray image from Celsius using [minC, maxC]. */
cv::Mat celsiusToGray(const QVector<float> &celsius, int width, int height,
                      float minC, float maxC);

QImage matToQImage(const cv::Mat &bgr);

#endif
