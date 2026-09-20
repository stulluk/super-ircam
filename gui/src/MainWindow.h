#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "CaptureThread.h"
#include "Colormap.h"
#include "ThermalMath.h"

#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>
#include <QVector>

#include <opencv2/videoio.hpp>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSlider;

class ImageCanvas : public QLabel {
  Q_OBJECT
 public:
  explicit ImageCanvas(QWidget *parent = nullptr) : QLabel(parent) {
    setMouseTracking(true);
  }

 signals:
  void canvasPressed(const QPoint &localPos, Qt::MouseButton button);

 protected:
  void mousePressEvent(QMouseEvent *event) override {
    emit canvasPressed(event->position().toPoint(), event->button());
    QLabel::mousePressEvent(event);
  }
};

enum class LayoutMode { Thermal, Image, DualWide, DualHigh };
enum class RulerMode { Off, OneTemp, CrossHair, Horiz, Vert, Both };
enum class InterMode { Nearest, Linear, Cubic, Area, Lanczos, LinExact, NearExact };
enum class RangeMode { None, Clip, Grow };

struct UserSpot {
  int x = 0;
  int y = 0;
};

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  void setCmapIndex(int index);
  void setZoom(int zoom);
  void setBlur(int radius);
  bool saveWindowShot(const QString &path);
  bool hasFrame() const { return !m_celsius.isEmpty(); }

 public slots:
  void onSnapshot();
  void onRecordToggle();
  void onReset();

 signals:
  void frameReceived();

 private slots:
  void onFrame(const QVector<float> &celsius, const QImage &visualY,
               const ThermalStats &stats);
  void onError(const QString &message);
  void onCanvasPressed(const QPoint &localPos, Qt::MouseButton button);

 private:
  QImage renderView();
  void drawOverlays(QImage *img, int ox, int oy, int scale) const;
  void drawMarker(QImage *img, int x, int y, const QString &text, QRgb rgb) const;
  float displayTemp(float celsius) const;
  QString tempText(float celsius) const;
  QPoint labelToSensor(const QPoint &labelPos) const;
  void applyBlurInPlace(QVector<float> *celsius) const;
  void colorRange(float *minC, float *maxC) const;
  int opencvInter() const;
  void updateTempLabels();
  void startRecorder(const QSize &frameSize);
  void stopRecorder();

  CaptureThread *m_capture = nullptr;
  ImageCanvas *m_image = nullptr;
  QLabel *m_status = nullptr;
  QLabel *m_minLabel = nullptr;
  QLabel *m_avgLabel = nullptr;
  QLabel *m_maxLabel = nullptr;
  QLabel *m_centerLabel = nullptr;
  QLabel *m_spotsLabel = nullptr;
  QLabel *m_deviceLabel = nullptr;
  QLabel *m_recordLabel = nullptr;
  QComboBox *m_cmap = nullptr;
  QComboBox *m_layout = nullptr;
  QComboBox *m_inter = nullptr;
  QComboBox *m_rulers = nullptr;
  QComboBox *m_rangeMode = nullptr;
  QSlider *m_zoom = nullptr;
  QSlider *m_blur = nullptr;
  QSlider *m_contrast = nullptr;
  QSlider *m_threshold = nullptr;
  QSlider *m_rotate = nullptr;
  QCheckBox *m_fahrenheit = nullptr;
  QCheckBox *m_freeze = nullptr;
  QCheckBox *m_histogram = nullptr;
  QCheckBox *m_lockRange = nullptr;
  QCheckBox *m_alarmOn = nullptr;
  QDoubleSpinBox *m_alarmC = nullptr;
  QPushButton *m_snapshot = nullptr;
  QPushButton *m_record = nullptr;
  QPushButton *m_reset = nullptr;

  QPixmap m_lastPixmap;
  QImage m_visualY;
  QVector<float> m_celsius;
  ThermalStats m_stats;
  QList<UserSpot> m_spots;
  int m_rulerX = kThermalWidth / 2;
  int m_rulerY = kThermalHeight / 2;
  float m_lockMin = 0;
  float m_lockMax = 0;
  bool m_lockInit = false;
  QString m_devicePath;
  cv::VideoWriter m_writer;
  bool m_recording = false;
  QString m_recordPath;
  int m_recFrames = 0;
};

#endif
