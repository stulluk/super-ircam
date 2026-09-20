#!/bin/bash
# Runs inside the build container. Builds redux + super-ircam and
# writes Debian packages into /work/dist.
set -eu

DEST=/work/dist
STAGING=/tmp/super-ircam-pkg
mkdir -p "${DEST}" "${STAGING}"

echo "=== compiler ==="
g++ --version | head -1
pkg-config --modversion opencv4
cmake --version | head -1

if [ -d /work/third_party/Thermal-Camera-Redux/src ] \
    && [ -d /work/packaging/thermal-camera-redux ]; then
  echo "=== Thermal-Camera-Redux ==="
  g++ -Wall -Wextra -O3 -ffast-math \
    -DBORDER_LAYOUT=0 \
    -DDEFAULT_FONT=0 \
    -DDEFAULT_COLORMAP=4 \
    -DROTATION=0 \
    -DDISPLAY_WIDTH=1920 \
    -DDISPLAY_HEIGHT=1080 \
    -DUSE_CELSIUS=1 \
    -DHUD_ALPHA=0.4 \
    -DUSE_ASSERT=0 \
    -I/usr/include/opencv4 \
    /work/third_party/Thermal-Camera-Redux/src/tc001.cpp \
    /work/third_party/Thermal-Camera-Redux/src/thread.cpp \
    -o "${DEST}/redux" \
    -lpthread \
    -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs \
    -lopencv_imgproc -lopencv_core

  echo "=== package thermal-camera-redux ==="
  REDUX_PKG="${STAGING}/thermal-camera-redux"
  rm -rf "${REDUX_PKG}"
  cp -a /work/packaging/thermal-camera-redux "${REDUX_PKG}"
  mkdir -p "${REDUX_PKG}/usr/libexec/thermal-camera-redux"
  cp -f "${DEST}/redux" "${REDUX_PKG}/usr/libexec/thermal-camera-redux/redux"
  chmod 0755 "${REDUX_PKG}/usr/bin/redux"
  chmod 0755 "${REDUX_PKG}/usr/libexec/thermal-camera-redux/redux"
  chmod 0644 "${REDUX_PKG}/DEBIAN/control"
  chmod 0644 "${REDUX_PKG}/usr/share/applications/thermal-camera-redux.desktop"
  find "${REDUX_PKG}/usr/share/doc" -type f -exec chmod 0644 {} \;
  dpkg-deb --root-owner-group --build "${REDUX_PKG}" \
    "${DEST}/thermal-camera-redux_0.9.3-1_amd64.deb"
else
  echo "=== skip Thermal-Camera-Redux (sources or packaging not in this tree) ==="
fi

if [ -d /work/gui ] && [ -d /work/packaging/super-ircam ]; then
  echo "=== super-ircam Qt GUI ==="
  rm -rf /tmp/super-ircam-build
  cmake -S /work/gui -B /tmp/super-ircam-build -DCMAKE_BUILD_TYPE=Release
  cmake --build /tmp/super-ircam-build -j"$(nproc)"
  cp -f /tmp/super-ircam-build/super-ircam "${DEST}/super-ircam"

  echo "=== package super-ircam ==="
  GUI_PKG="${STAGING}/super-ircam"
  rm -rf "${GUI_PKG}"
  cp -a /work/packaging/super-ircam "${GUI_PKG}"
  mkdir -p "${GUI_PKG}/usr/bin"
  cp -f "${DEST}/super-ircam" "${GUI_PKG}/usr/bin/super-ircam"
  chmod 0755 "${GUI_PKG}/usr/bin/super-ircam"
  chmod 0644 "${GUI_PKG}/DEBIAN/control"
  chmod 0755 "${GUI_PKG}/DEBIAN/postinst"
  chmod 0644 "${GUI_PKG}/usr/share/applications/super-ircam.desktop"
  find "${GUI_PKG}/usr/share/icons" -type f -exec chmod 0644 {} \;
  find "${GUI_PKG}/usr/share/pixmaps" -type f -exec chmod 0644 {} \; 2>/dev/null || true
  find "${GUI_PKG}/usr/share/doc" -type f -exec chmod 0644 {} \;
  dpkg-deb --root-owner-group --build "${GUI_PKG}" \
    "${DEST}/super-ircam_0.2.0-1_amd64.deb"
else
  echo "=== skip super-ircam (sources or packaging not in this tree) ==="
fi

echo "=== outputs ==="
ls -l "${DEST}"
if [ -f "${DEST}/redux" ]; then
  echo "redux NEEDED:"
  objdump -p "${DEST}/redux" | awk '/NEEDED/ { print $2 }'
fi
if [ -f "${DEST}/super-ircam" ]; then
  echo "super-ircam NEEDED:"
  objdump -p "${DEST}/super-ircam" | awk '/NEEDED/ { print $2 }'
fi
