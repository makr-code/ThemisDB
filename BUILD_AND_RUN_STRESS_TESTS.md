# Building and Running Tensor Stress Test Suite (STREAM B BLOCK B2)

This guide provides step-by-step instructions for building and running the comprehensive stress testing suite for the TensorFingerprintGraph component.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Build Instructions](#build-instructions)
4. [Running Tests](#running-tests)
5. [Test Output Analysis](#test-output-analysis)
6. [Troubleshooting](#troubleshooting)
7. [CI Integration](#ci-integration)

---

## Quick Start

For rapid builds and testing:

```bash
# Configure (Linux)
cmake --preset community-release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_CUDA=OFF

# Build just the stress test target
cmake --build --preset community-release \
  --target module_tensor_test_tensor_stress_suite_focused \
  --parallel 8

# Run all stress tests
ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure \
  -V
```

---

## Prerequisites

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU Cores | 4 | 8+ |
| RAM | 4 GB | 8 GB+ |
| Disk Space | 2 GB | 5 GB+ |
| Build Time | N/A | ~120s for stress test target |

### Required Dependencies

- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: 3.21+
- **Build Tools**: 
  - Linux: make, ninja
  - Windows: Visual Studio 2019+
  - macOS: Xcode 12+

### ThemisDB Dependencies

Required:
- GoogleTest (gtest)
- spdlog (logging)
- fmt (formatting)
- OpenSSL 3.0+
- ZLIB 1.3+

Optional (for full build):
- RocksDB (or use the `community-release-allow-missing-rocksdb` configure preset for diagnostic builds)
- CUDA (disable with `-DTHEMIS_ENABLE_CUDA=OFF`)
- TensorRT, TBB (not needed for stress tests)

### Installation (Ubuntu/Debian)

```bash
# Core dependencies
sudo apt-get install -y \
  build-essential cmake git \
  libgtest-dev libfmt-dev spdlog \
  libssl-dev zlib1g-dev

# Optional: RocksDB (if not using system packages)
git clone https://github.com/facebook/rocksdb.git
cd rocksdb && make shared_lib -j8 && sudo make install
```

---

## Build Instructions

### Step 1: Clone and Prepare Repository

```bash
cd /path/to/ThemisDB
git checkout feature/tensor-q4-determinism  # or your feature branch
```

### Step 2: Configure CMake

#### Linux (Recommended)

```bash
# Community Release (default, system packages)
cmake --preset community-release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_CUDA=OFF
```

#### Windows

```bash
# Windows Release with vcpkg
cmake --preset windows-release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_CUDA=OFF
```

#### macOS

```bash
# Requires Homebrew: brew install gcc cmake gtest fmt spdlog openssl zlib
cmake -S . -B build_mac \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_CUDA=OFF
```

### Step 3: Build Stress Test Target

```bash
# Build just the stress test (fast iteration)
cmake --build --preset community-release \
  --target module_tensor_test_tensor_stress_suite_focused \
  --parallel 8

# Or full build (if needed)
cmake --build --preset community-release --parallel 16
```

### Build Output

Successful build produces:
```
build-community-release/bin_out_module_tensor_tests/
  ├── module_tensor_test_tensor_stress_suite_focused
  └── [other tensor test binaries]
```

---

## Running Tests

### Basic Test Execution

```bash
# Run all stress tests via ctest
ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure \
  -V
```

### Run Specific Test Patterns

```bash
# Run only throughput tests (TSTRESS-01..03)
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*TSTRESS01*:*TSTRESS02*:*TSTRESS03*"

# Run only memory stability tests (TSTRESS-04..06)
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*TSTRESS04*:*TSTRESS05*:*TSTRESS06*"

# Run only chaos injection tests (TSTRESS-13..15)
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*TSTRESS13*:*TSTRESS14*:*TSTRESS15*"

# Skip long-running tests (TSTRESS-18)
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*:-*TSTRESS18*"
```

### Direct Execution (for debugging)

```bash
# Find the test binary
TEST_BIN=./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused

# Run with gtest filters
${TEST_BIN} --gtest_filter="*TSTRESS01*"

# Run with verbose output
${TEST_BIN} --gtest_filter="*TSTRESS*" --gtest_repeat=3
```

### Test Categories

#### Quick Validation (~30 seconds)

```bash
# Run fast throughput + latency tests only
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*TSTRESS01*:*TSTRESS02*:*TSTRESS07*:*TSTRESS10*"
```

#### Comprehensive (~5 minutes)

```bash
# Run all stress tests except SustainedLoad
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*:-*TSTRESS18*" \
  --gtest_repeat=1
```

#### Full Validation (~10 minutes)

```bash
# Run all stress tests including SustainedLoad
ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure \
  -V
```

---

## Test Output Analysis

### Expected Output Example

```
Running main() from gmock_main.cc
[==========] Running 20 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 20 tests from TensorStressTest
[ RUN      ] TensorStressTest.TSTRESS01_BasicThroughput10kOps
[       OK ] TensorStressTest.TSTRESS01_BasicThroughput10kOps (345 ms)
[ RUN      ] TensorStressTest.TSTRESS02_BasicThroughput50kOps
[       OK ] TensorStressTest.TSTRESS02_BasicThroughput50kOps (1520 ms)
[ RUN      ] TensorStressTest.TSTRESS03_BasicThroughput100kOps
[       OK ] TensorStressTest.TSTRESS03_BasicThroughput100kOps (3047 ms)
...
[==========] 20 passed, 0 failed in 45678 ms
```

### Performance Metrics Interpretation

#### Throughput

```
Expected: >= 2,000 ops/sec
Example: 2,450 ops/sec ✓ PASS

Formula: ops/sec = (total_operations * 1e9) / elapsed_ns
```

#### Memory

```
Expected: < 5% growth per 1M ops
Example: Graph size 1,234 (< 1M ops) ✓ PASS
```

#### Latency

```
P99 < 100ms (QueryHeavy)
P99 < 500ms (Mixed)
P99 < 1s (StoreHeavy)
```

### Test Result Summary

Save results to file:
```bash
ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure \
  > stress_test_results.txt 2>&1

# Extract summary
grep -E "PASS|FAIL|Throughput|Latency" stress_test_results.txt
```

---

## Troubleshooting

### Build Failures

#### Error: "gtest/gtest.h: No such file or directory"

**Cause**: GoogleTest not found or not linked.

**Solution**:
```bash
# Install gtest
sudo apt-get install libgtest-dev

# Or ensure it's linked in CMakeLists.txt
# (already done in stress test target)
```

#### Error: "tensor_fingerprint_graph.h: No such file or directory"

**Cause**: Include path not set correctly.

**Solution**:
```bash
# Verify include directories in CMakeLists.txt
cmake --preset community-release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DCMAKE_VERBOSE_MAKEFILE=ON  # Show build details
```

#### Error: "undefined reference to `themis::tensor::TensorFingerprintGraph::...`"

**Cause**: Implementation files not linked.

**Solution**: Verify CMakeLists.txt includes:
- `tensor_fingerprint_graph.cpp`
- `adapter_repository.cpp`
- `tensor_error_handling.cpp`

(Already configured in our update)

### Runtime Failures

#### Test timeout (e.g., "Test exceeded timeout")

**Cause**: Test running longer than expected (default: 300s).

**Solution**:
```bash
# Increase timeout for long-running tests
ctest --preset community-release \
  --timeout 600  # 10 minutes

# Or run without timeout (not recommended for CI)
ctest --preset community-release \
  --no-timeout
```

#### Assertion failure: "Throughput regression"

**Cause**: Performance below 2,000 ops/sec threshold.

**Diagnosis**:
```bash
# Run test in isolation
./build-community-release/bin_out_module_tensor_tests/module_tensor_test_tensor_stress_suite_focused \
  --gtest_filter="*TSTRESS01*" \
  --gtest_repeat=5

# Check system load
top -bn1 | head -10

# Check CPU frequency scaling
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver
```

**Solution**:
- Disable CPU frequency scaling for consistent results
- Close other applications
- Use dedicated machine for benchmarking
- Verify no hyperthreading contention

#### Memory assertion failure: "Graph should remain bounded"

**Cause**: Graph size growing beyond expected limits.

**Diagnosis**:
```bash
# Check removeAdapter() implementation
grep -n "removeAdapter" src/tensor/tensor_fingerprint_graph.cpp

# Verify RAII cleanup in destructors
grep -n "~\|delete\|clear" include/tensor/tensor_fingerprint_graph.h
```

**Solution**:
- Implement proper cleanup in removeAdapter()
- Verify shared_mutex doesn't hold references
- Check for reference cycles in fingerprint storage

### Compilation Warnings

Suppress non-critical warnings:
```bash
cmake --preset community-release \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"

# Or ignore specific warnings
-Wno-deprecated-declarations -Wno-unused-parameter
```

---

## CI Integration

### GitHub Actions Configuration

Add to `.github/workflows/tensor-stress-tests.yml`:

```yaml
name: Tensor Stress Tests (Stream B Block 2)

on: [push, pull_request]

jobs:
  stress-tests:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    
    steps:
      - uses: actions/checkout@v3
        with:
          ref: feature/tensor-q4-determinism
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            libgtest-dev libfmt-dev spdlog \
            libssl-dev zlib1g-dev
      
      - name: Configure CMake
        run: |
          cmake --preset community-release \
            -DTHEMIS_BUILD_TESTS=ON \
            -DTHEMIS_ENABLE_CUDA=OFF
      
      - name: Build stress test target
        run: |
          cmake --build --preset community-release \
            --target module_tensor_test_tensor_stress_suite_focused \
            --parallel 8
      
      - name: Run stress tests
        run: |
          ctest --preset community-release \
            -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
            --output-on-failure \
            -V 2>&1 | tee stress_test_results.txt
      
      - name: Upload results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: stress-test-results
          path: |
            build-community-release/Testing/Temporary/LastTest.log
            stress_test_results.txt
```

### Local Pre-Commit Validation

Create `.git/hooks/pre-push` script:

```bash
#!/bin/bash
echo "Running tensor stress tests..."

ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure

if [ $? -ne 0 ]; then
    echo "Stress tests failed. Push aborted."
    exit 1
fi
```

Make executable:
```bash
chmod +x .git/hooks/pre-push
```

---

## Performance Baseline Establishment

### First Run (Baseline)

```bash
# Run once to establish baseline
ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure \
  -V \
  > baseline_results.txt 2>&1

# Extract metrics
grep -E "Throughput|Latency|ops/sec|ms" baseline_results.txt > baseline_metrics.txt
```

### Regression Testing

```bash
# Run and compare to baseline
ctest --preset community-release \
  -R "^test_tensor_stress_suite_focused_tensor_FocusedTests$" \
  --output-on-failure \
  -V \
  > current_results.txt 2>&1

# Simple comparison (automated)
diff baseline_metrics.txt <(grep -E "Throughput|Latency" current_results.txt)
```

---

## Documentation

- **STRESS_COVERAGE.md**: Detailed workload profiles and measurement methodology
- **test_tensor_stress_suite_focused.cpp**: Test implementation with inline documentation
- **CMakeLists.txt**: Build configuration with timeout settings

---

## References

- [STREAM_B_Q4_2026_PLANNING.md](../../STREAM_B_Q4_2026_PLANNING.md) - Project planning
- [STRESS_COVERAGE.md](./STRESS_COVERAGE.md) - Workload profiles and strategies
- [include/tensor/tensor_fingerprint_graph.h](../../include/tensor/tensor_fingerprint_graph.h) - API reference
- [benchmarks/MEASUREMENT_HYGIENE.md](../../benchmarks/MEASUREMENT_HYGIENE.md) - Measurement best practices
