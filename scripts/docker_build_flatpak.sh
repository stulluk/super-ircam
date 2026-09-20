#!/bin/bash
# Build the Flatpak bundle in a privileged container (bwrap).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE_NAME="${FLATPAK_IMAGE:-super-ircam-flatpak:latest}"
mkdir -p "${ROOT}/dist"

docker build -t "${IMAGE_NAME}" -f "${ROOT}/Dockerfile.flatpak" "${ROOT}"
docker run --rm --privileged \
  -v "${ROOT}:/work" \
  -w /work \
  "${IMAGE_NAME}" \
  /bin/bash /work/scripts/build_flatpak_in_container.sh

ls -l "${ROOT}/dist/SuperIRCam.flatpak"
