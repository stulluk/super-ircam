#!/bin/bash
# Build Thermal-Camera-Redux, the Qt GUI, and both .deb packages
# inside Docker. Artifacts land in dist/ on the host.
set -eu
ROOT="$(cd "$(dirname "$0")" && pwd)"
IMAGE_NAME="${IMAGE_NAME:-super-ircam-build:latest}"
mkdir -p "${ROOT}/dist"

if ! docker image inspect "${IMAGE_NAME}" >/dev/null 2>&1; then
  echo "Image ${IMAGE_NAME} missing; run ./dockerbuild.sh first."
  exit 1
fi

docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp \
  -v "${ROOT}:/work" \
  -w /work \
  "${IMAGE_NAME}" \
  /bin/bash /work/scripts/build_debs_in_container.sh

echo "Build outputs:"
ls -l "${ROOT}/dist"
