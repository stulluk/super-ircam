#!/bin/bash
# Build the Ubuntu 22.04 AppImage image and produce dist/SuperIRCam-x86_64.AppImage
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE_NAME="${APPIMAGE_IMAGE:-super-ircam-appimage:22.04}"
mkdir -p "${ROOT}/dist"

docker build -t "${IMAGE_NAME}" -f "${ROOT}/Dockerfile.appimage" "${ROOT}"
docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp \
  -v "${ROOT}:/work" \
  -w /work \
  "${IMAGE_NAME}" \
  /bin/bash /work/scripts/build_appimage_in_container.sh

ls -l "${ROOT}/dist/SuperIRCam-x86_64.AppImage"
