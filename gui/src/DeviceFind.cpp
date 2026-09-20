#include "DeviceFind.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

static QString readTrimmed(const QString &path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }
  return QString::fromUtf8(f.readAll()).trimmed();
}

static bool walkUsbIds(QString startDir, QString *vendor, QString *product) {
  // videoN/device is a relative symlink into the USB interface. Resolve it
  // first; otherwise cdUp() stays inside video4linux and never sees idVendor.
  const QString resolved = QFileInfo(startDir).canonicalFilePath();
  QDir dir(resolved.isEmpty() ? startDir : resolved);
  for (int i = 0; i < 8; ++i) {
    const QString v = readTrimmed(dir.filePath("idVendor"));
    const QString p = readTrimmed(dir.filePath("idProduct"));
    if (!v.isEmpty() && !p.isEmpty()) {
      *vendor = v.toLower();
      *product = p.toLower();
      return true;
    }
    if (!dir.cdUp()) {
      break;
    }
  }
  return false;
}

bool findThermalCamera(ThermalDevice *out, QString *error) {
  const QDir root("/sys/class/video4linux");
  const QStringList nodes = root.entryList(QStringList() << "video*",
                                           QDir::Dirs | QDir::NoDotAndDotDot,
                                           QDir::Name);
  for (const QString &name : nodes) {
    const QString sys = root.filePath(name);
    const QString cap = readTrimmed(sys + "/device/uevent");
    Q_UNUSED(cap);
    QString vendor;
    QString product;
    if (!walkUsbIds(sys + "/device", &vendor, &product)) {
      continue;
    }
    if (vendor != "0bda" || product != "5830") {
      continue;
    }
    bool ok = false;
    const int index = QString(name).mid(5).toInt(&ok);
    if (!ok) {
      continue;
    }
    const QString devPath = "/dev/" + name;
    if (!QFileInfo::exists(devPath)) {
      continue;
    }
    // Prefer the capture node: videoN+1 is usually the metadata sibling.
    // The capture node has a non-empty "index" of 0 in many UVC drivers.
    const QString idxFile = readTrimmed(sys + "/index");
    if (idxFile == "1") {
      continue;
    }
    out->path = devPath;
    out->index = index;
    out->vendorId = vendor;
    out->productId = product;
    return true;
  }
  if (error) {
    *error = "No thermal camera found (USB 0bda:5830).";
  }
  return false;
}
