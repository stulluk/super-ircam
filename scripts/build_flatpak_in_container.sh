#!/bin/bash
# Runs inside the Flatpak build container (needs bwrap / privileged).
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="${ROOT}/dist"
STATE="${ROOT}/.flatpak-builder"
BUILD="${STATE}/build"
REPO="${STATE}/repo"
mkdir -p "${DEST}" "${STATE}" "${BUILD}" "${REPO}"

echo "=== flathub runtimes ==="
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install -y --noninteractive flathub \
  org.kde.Platform//6.10 \
  org.kde.Sdk//6.10

echo "=== flatpak-builder ==="
rm -rf "${BUILD}"
flatpak-builder --force-clean --ccache \
  --state-dir "${STATE}" \
  --repo="${REPO}" \
  "${BUILD}" \
  "${ROOT}/packaging/flatpak/com.github.stulluk.SuperIRCam.yml"

echo "=== bundle ==="
flatpak build-bundle \
  "${REPO}" \
  "${DEST}/SuperIRCam.flatpak" \
  com.github.stulluk.SuperIRCam
ls -l "${DEST}/SuperIRCam.flatpak"
