#!/bin/bash
# Runs inside the build container. Compiles both viewers into /work/dist.
set -eu
DEST=/work/dist
LIBDEST="${DEST}/lib"
mkdir -p "${LIBDEST}"

echo "=== compiler ==="
g++ --version | head -1
pkg-config --modversion opencv4
pkg-config --libs opencv4

echo "=== Thermal-Camera-Redux ==="
read -r -a opencv_cflags < <(pkg-config --cflags opencv4)
# Link only the modules Redux actually needs. Full opencv4 pulls GDAL/Qt/OpenEXR.
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
  "${opencv_cflags[@]}" \
  -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs \
  -lopencv_imgproc -lopencv_core

echo "=== ircam-viewer ==="
make -C /work/third_party/ircam-viewer -j"$(nproc)" clean
make -C /work/third_party/ircam-viewer -j"$(nproc)"
cp -f /work/third_party/ircam-viewer/ircam "${DEST}/ircam"

copy_non_glibc_deps() {
  binary="$1"
  ldd "${binary}" | awk '/=> \// { print $3 } /^\/.+ld-linux/ { print $1 }' \
    | while IFS= read -r so; do
      [ -n "${so}" ] || continue
      [ -f "${so}" ] || continue
      base="$(basename "${so}")"
      case "${base}" in
        libc.so*|libm.so*|libpthread.so*|libdl.so*|librt.so*|libresolv.so*|ld-linux*)
          continue
          ;;
      esac
      cp -a "${so}" "${LIBDEST}/"
    done
}

echo "=== collect shared libs ==="
copy_non_glibc_deps "${DEST}/redux"
copy_non_glibc_deps "${DEST}/ircam"

echo "=== outputs ==="
ls -l "${DEST}/redux" "${DEST}/ircam"
echo "lib count: $(find "${LIBDEST}" -maxdepth 1 -type f | wc -l)"
