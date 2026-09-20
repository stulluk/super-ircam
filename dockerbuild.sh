#!/bin/bash
set -eu
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "${ROOT}"
IMAGE_NAME="${IMAGE_NAME:-super-ircam-build:latest}"
LOG="${ROOT}/dist/dockerbuild_$(date +%Y%m%d_%H%M%S).log"
mkdir -p "${ROOT}/dist"
echo "Building ${IMAGE_NAME}, log: ${LOG}"
docker build -t "${IMAGE_NAME}" -f "${ROOT}/Dockerfile" "${ROOT}" 2>&1 | tee "${LOG}"
echo "Image ${IMAGE_NAME} ready."
