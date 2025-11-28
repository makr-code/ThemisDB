#!/usr/bin/env bash
set -euo pipefail
if [[ "${QNAP_BUILD_DEBUG:-0}" == "1" ]]; then
  set -x
fi

echo "=== ThemisDB QNAP Build (Ubuntu 20.04, GLIBC 2.31) [bash] ==="

# Ensure vcpkg root
VCPKG_ROOT="/opt/vcpkg"
TRIPLET="x64-linux"
MANIFEST_SRC="vcpkg.qnap.json"
MANIFEST_DST="vcpkg.json"

if [[ ! -d "$VCPKG_ROOT" ]]; then
  echo "ERROR: vcpkg root $VCPKG_ROOT nicht gefunden" >&2
  exit 1
fi

echo ">>> vcpkg Info"
git -C "$VCPKG_ROOT" rev-parse HEAD || true
"$VCPKG_ROOT/vcpkg" version || true

# Prepare manifest (copy qnap manifest to canonical name expected by vcpkg)
if [[ -f "$MANIFEST_SRC" ]]; then
  cp -f "$MANIFEST_SRC" "$MANIFEST_DST"
else
  echo "ERROR: Manifest $MANIFEST_SRC fehlt" >&2
  exit 1
fi

export VCPKG_FEATURE_FLAGS=manifests
export VCPKG_DEFAULT_TRIPLET="$TRIPLET"
export VCPKG_MANIFEST_DIR="$(pwd)"

echo ">>> Bootstrap vcpkg (idempotent)"
"$VCPKG_ROOT/bootstrap-vcpkg.sh"

echo ">>> Aktualisiere Ports Repository (git pull)"
git -C "$VCPKG_ROOT" pull --ff-only || echo "Warnung: git pull fehlgeschlagen, fahre fort"

echo ">>> vcpkg install (manifest + $TRIPLET, mit Retries)"
set +e
for attempt in 1 2 3; do
  echo "Versuch $attempt..."
  "$VCPKG_ROOT/vcpkg" install --triplet "$TRIPLET" --clean-after-build
  rc=$?
  if [ $rc -eq 0 ]; then
    echo "vcpkg install erfolgreich"
    break
  fi
  echo "vcpkg install fehlgeschlagen (rc=$rc)" >&2
  if [ $attempt -lt 3 ]; then
    echo "Warte 15s und retry..."
    sleep 15
  fi
done
set -e
if [ $rc -ne 0 ]; then
  echo "Abbruch nach 3 Fehlversuchen" >&2
  exit 1
fi

echo ">>> vcpkg list (kurz)"
"$VCPKG_ROOT/vcpkg" list | sed -n '1,50p' || true

echo ">>> Configure (CMake)"
rm -rf build-qnap
extra_flags=()
if [[ "${QNAP_VERBOSE:-0}" == "1" ]]; then
  extra_flags+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
fi
cmake -S . -B build-qnap -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_STATIC_BUILD=ON \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_TRACING=ON \
  -DVCPKG_MANIFEST_INSTALL=OFF \
  -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
  -DCMAKE_PREFIX_PATH="$(pwd)/vcpkg_installed/$TRIPLET" \
  -DRocksDB_DIR="$(pwd)/vcpkg_installed/$TRIPLET/share/rocksdb" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  "${extra_flags[@]:-}"

echo ">>> Build"
cmake --build build-qnap --parallel

echo ">>> Prüfe Binary"
ls -l build-qnap/themis_server || { echo "themis_server nicht gefunden" >&2; exit 1; }

echo ">>> GLIBC Prüfung (optional)"
ldd build-qnap/themis_server | grep GLIBC || true

echo "=== Fertig (QNAP bash build) ==="
