#!/bin/bash
# Runs inside the Ubuntu 22.04 AppImage build container, or on GitHub Actions.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="${ROOT}/dist"
APPDIR=/tmp/SuperIRCam.AppDir
TOOLS=/tmp/appimage-tools
mkdir -p "${DEST}" "${TOOLS}"

echo "=== build super-ircam (Ubuntu 22.04 / old glibc) ==="
rm -rf /tmp/super-ircam-appimage-build
cmake -S "${ROOT}/gui" -B /tmp/super-ircam-appimage-build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build /tmp/super-ircam-appimage-build -j"$(nproc)"
rm -rf "${APPDIR}"
DESTDIR="${APPDIR}" cmake --install /tmp/super-ircam-appimage-build

if [ ! -x "${TOOLS}/linuxdeploy/AppRun" ]; then
  echo "=== fetch linuxdeploy ==="
  wget -q -O "${TOOLS}/linuxdeploy.AppImage" \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
  wget -q -O "${TOOLS}/linuxdeploy-plugin-qt.AppImage" \
    https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
  chmod +x "${TOOLS}/linuxdeploy.AppImage" "${TOOLS}/linuxdeploy-plugin-qt.AppImage"
  cd "${TOOLS}"
  ./linuxdeploy.AppImage --appimage-extract >/dev/null
  mv squashfs-root linuxdeploy
  ./linuxdeploy-plugin-qt.AppImage --appimage-extract >/dev/null
  mv squashfs-root linuxdeploy-plugin-qt
fi

export APPIMAGE_EXTRACT_AND_RUN=1
export LINUXDEPLOY_OUTPUT_VERSION=0.2.0
WAYLAND_PLUGIN="/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/libqwayland-generic.so"
if [ -f "${WAYLAND_PLUGIN}" ]; then
  export EXTRA_PLATFORM_PLUGINS="libqwayland-generic.so;libqwayland-egl.so"
fi
export QMAKE="${QMAKE:-/usr/lib/qt6/bin/qmake}"
if [ ! -x "${QMAKE}" ]; then
  QMAKE="$(command -v qmake6 || command -v qmake || true)"
fi
export QMAKE
export PATH="${TOOLS}/linuxdeploy-plugin-qt/usr/bin:${PATH}"
export LDAI_OUTPUT="${DEST}/SuperIRCam-x86_64.AppImage"
export LINUXDEPLOY_OUTPUT_APP_NAME=SuperIRCam

cd /tmp
rm -f SuperIRCam-x86_64.AppImage "${DEST}/SuperIRCam-x86_64.AppImage"
"${TOOLS}/linuxdeploy/AppRun" \
  --appdir "${APPDIR}" \
  --desktop-file "${APPDIR}/usr/share/applications/super-ircam.desktop" \
  --icon-file "${APPDIR}/usr/share/icons/hicolor/128x128/apps/super-ircam.png" \
  --plugin qt \
  --output appimage

# linuxdeploy writes the AppImage next to the cwd.
if [ -f /tmp/SuperIRCam-x86_64.AppImage ]; then
  mv -f /tmp/SuperIRCam-x86_64.AppImage "${DEST}/SuperIRCam-x86_64.AppImage"
elif [ -f /tmp/Super_IRCam-x86_64.AppImage ]; then
  mv -f /tmp/Super_IRCam-x86_64.AppImage "${DEST}/SuperIRCam-x86_64.AppImage"
fi
chmod +x "${DEST}/SuperIRCam-x86_64.AppImage"
ls -l "${DEST}/SuperIRCam-x86_64.AppImage"
file "${DEST}/SuperIRCam-x86_64.AppImage"
