#ifndef CAPTURE_THREAD_H
#define CAPTURE_THREAD_H

#include <QImage>
#include <QMetaType>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QVector>

#include <cstdint>

struct ThermalStats {
  float minC = 0;
  float maxC = 0;
  float avgC = 0;
  float centerC = 0;
  int minX = 0;
  int minY = 0;
  int maxX = 0;
  int maxY = 0;
  bool valid = false;
};

Q_DECLARE_METATYPE(ThermalStats)

class CaptureThread : public QThread {
  Q_OBJECT

 public:
  explicit CaptureThread(QObject *parent = nullptr);
  ~CaptureThread() override;

  void setDevicePath(const QString &path);
  void stop();
  QVector<float> latestCelsius() const;
  QVector<quint16> latestKelvin() const;

 signals:
  void frameReady(const QVector<float> &celsius, const QImage &visualY,
                  const ThermalStats &stats);
  void errorOccurred(const QString &message);

 protected:
  void run() override;

 private:
  QString m_path;
  bool m_stop = false;
  mutable QMutex m_mutex;
  QVector<float> m_latest;
  QVector<quint16> m_kelvin;
};

#endif
