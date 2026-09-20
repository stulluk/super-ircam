#ifndef DEVICE_FIND_H
#define DEVICE_FIND_H

#include <QString>

struct ThermalDevice {
  QString path;
  int index = -1;
  QString vendorId;
  QString productId;
};

/**
 * Find the first V4L2 capture node with USB id 0bda:5830
 * (Qianli Super IRCam / Topdon TC001 / InfiRay P2 Pro).
 */
bool findThermalCamera(ThermalDevice *out, QString *error);

#endif
