#!/usr/bin/env bash
# Wave A GPU Test Execution Suite
# Executes GPU-FALLBACK-01..12, GPU-TIMEOUT-01..12, GPU-EXHAUST-01..12 tests

set -e

REPO_ROOT="/home/runner/work/ThemisDB/ThemisDB"
BUILD_DIR="${BUILD_DIR:-/tmp/themis-phase2-gpu-tests}"
TEST_OUTPUT_DIR="/tmp/wave-a-gpu-test-results"

echo "=========================================="
echo "Wave A GPU Test Execution Suite"
echo "=========================================="
echo "Repo: $REPO_ROOT"
echo "Build: $BUILD_DIR"
echo "Tests: $TEST_OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p "$TEST_OUTPUT_DIR"

# Configure and build with GPU support (or CPU fallback)
echo "[1/4] Configuring CMake..."
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_GPU=ON \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror=format" \
  || {
    echo "CMake configure failed. Trying with CPU-only fallback..."
    cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DTHEMIS_ENABLE_GPU=OFF \
      -DTHEMIS_ALLOW_MISSING_ROCKSDB=ON
  }

echo "[2/4] Building test targets..."
cmake --build "$BUILD_DIR" --target \
  test_gpu_fallback_all_paths \
  test_gpu_kernel_timeout_enforcer \
  test_gpu_resource_exhaustion \
  test_gpu_wave_a_timeout_closure \
  -j $(nproc) \
  2>&1 | tee "$TEST_OUTPUT_DIR/build.log"

echo ""
echo "[3/4] Running test suites..."

# GPU Fallback Tests (GPU-FALLBACK-01..12)
echo "  [3a] GPU-FALLBACK-01..12 (test_gpu_fallback_all_paths)..."
if [ -f "$BUILD_DIR/tests/gpu/test_gpu_fallback_all_paths" ]; then
  "$BUILD_DIR/tests/gpu/test_gpu_fallback_all_paths" \
    --gtest_output="json:$TEST_OUTPUT_DIR/gpu_fallback_results.json" \
    2>&1 | tee "$TEST_OUTPUT_DIR/gpu_fallback.log" || true
else
  echo "    ⚠️  Binary not found"
fi

# GPU Timeout Tests (GPU-TIMEOUT-01..12)
echo "  [3b] GPU-TIMEOUT-01..12 (test_gpu_kernel_timeout_enforcer)..."
if [ -f "$BUILD_DIR/tests/gpu/test_gpu_kernel_timeout_enforcer" ]; then
  "$BUILD_DIR/tests/gpu/test_gpu_kernel_timeout_enforcer" \
    --gtest_output="json:$TEST_OUTPUT_DIR/gpu_timeout_results.json" \
    2>&1 | tee "$TEST_OUTPUT_DIR/gpu_timeout.log" || true
else
  echo "    ⚠️  Binary not found"
fi

# GPU Resource Exhaustion Tests (GPU-EXHAUST-01..12)
echo "  [3c] GPU-EXHAUST-01..12 (test_gpu_resource_exhaustion)..."
if [ -f "$BUILD_DIR/tests/gpu/test_gpu_resource_exhaustion" ]; then
  "$BUILD_DIR/tests/gpu/test_gpu_resource_exhaustion" \
    --gtest_output="json:$TEST_OUTPUT_DIR/gpu_exhaustion_results.json" \
    2>&1 | tee "$TEST_OUTPUT_DIR/gpu_exhaustion.log" || true
else
  echo "    ⚠️  Binary not found"
fi

# GPU Wave A Timeout Closure Tests
echo "  [3d] GPU Wave A Closure (test_gpu_wave_a_timeout_closure)..."
if [ -f "$BUILD_DIR/tests/gpu/test_gpu_wave_a_timeout_closure" ]; then
  "$BUILD_DIR/tests/gpu/test_gpu_wave_a_timeout_closure" \
    --gtest_output="json:$TEST_OUTPUT_DIR/gpu_closure_results.json" \
    2>&1 | tee "$TEST_OUTPUT_DIR/gpu_closure.log" || true
else
  echo "    ⚠️  Binary not found"
fi

echo ""
echo "[4/4] Test execution complete"
echo "  Results: $TEST_OUTPUT_DIR"
echo "  Logs:"
for log in "$TEST_OUTPUT_DIR"/*.log; do
  if [ -f "$log" ]; then
    passed=$(grep -c "PASSED\|OK" "$log" || echo "0")
    failed=$(grep -c "FAILED\|FAIL" "$log" || echo "0")
    echo "    $(basename "$log"): $passed passed, $failed failed"
  fi
done

echo ""
echo "=========================================="
echo "Wave A GPU Test Execution Complete"
echo "=========================================="
