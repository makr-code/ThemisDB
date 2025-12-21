#!/usr/bin/env bash
set -euo pipefail

# Cross-platform setup for Linux/macOS (vcpkg bootstrap)
# Windows users can use setup.ps1

# Detect or install vcpkg
if [[ -z "${VCPKG_ROOT:-}" ]]; then
  echo "VCPKG_ROOT not set. Installing vcpkg locally to ./vcpkg ..."
  if [[ ! -d vcpkg ]]; then
    git clone https://github.com/microsoft/vcpkg.git vcpkg
  fi
  export VCPKG_ROOT="$(pwd)/vcpkg"
  echo "VCPKG_ROOT set to ${VCPKG_ROOT}"
fi

# Bootstrap vcpkg
if [[ ! -f "${VCPKG_ROOT}/vcpkg" ]]; then
  pushd "$VCPKG_ROOT" >/dev/null
  ./bootstrap-vcpkg.sh
  popd >/dev/null
fi

echo "vcpkg ready at: ${VCPKG_ROOT}"