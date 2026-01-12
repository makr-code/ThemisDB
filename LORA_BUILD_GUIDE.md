# LoRA Framework Build Guide

Complete guide for building and testing the ThemisDB LoRA Adapter Framework with CMake.

## 📋 Table of Contents

1. [Docker Quick Start](#docker-quick-start) ⭐ **New!**
2. [Prerequisites](#prerequisites)
3. [Quick Start](#quick-start)
4. [Build Options](#build-options)
5. [Dependencies](#dependencies)
6. [Building](#building)
7. [Running Tests](#running-tests)
8. [Running Benchmarks](#running-benchmarks)
9. [Troubleshooting](#troubleshooting)

---

## Docker Quick Start

🐳 **Fastest way to get started** - Complete environment in < 5 minutes!

### Using Docker Compose

```bash
# Navigate to docker directory
cd docker

# Start all services (ThemisDB + Prometheus + Grafana)
./scripts/start.sh

# Access services:
# - ThemisDB:   http://localhost:8529
# - Prometheus: http://localhost:9091
# - Grafana:    http://localhost:3000 (admin/admin)
```

### Development Mode

```bash
# Start with hot-reload and debug support
./scripts/start.sh dev

# Run tests in Docker
./scripts/test.sh

# View logs
./scripts/logs.sh themisdb
```

**Full Docker documentation**: See [docker/README.md](docker/README.md)

---

---

## Prerequisites

### Required

- **CMake** ≥ 3.20
- **C++17** compatible compiler (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 2019)
- **vcpkg** (recommended for dependency management)

### Required Dependencies

```bash
# Core dependencies (always required)
vcpkg install openssl zlib rocksdb fmt spdlog nlohmann-json boost-system boost-filesystem protobuf grpc
```

### Optional Dependencies

```bash
# For unit tests
vcpkg install gtest

# For benchmarks
vcpkg install benchmark

# For metrics (Prometheus/Grafana integration)
vcpkg install prometheus-cpp
```

---

## Quick Start

### 1. Build with Default Options

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Create build directory
mkdir build && cd build

# Configure with vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build . -j$(nproc)
```

### 2. Build with Tests and Benchmarks

```bash
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_BUILD_TESTS=ON \
    -DTHEMIS_BUILD_BENCHMARKS=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --target test_lora_framework bench_lora_framework
```

---

## Build Options

### Core Options

| Option | Default | Description |
|--------|---------|-------------|
| `THEMIS_BUILD_TESTS` | `ON` | Build unit tests |
| `THEMIS_BUILD_BENCHMARKS` | `ON` | Build performance benchmarks |
| `THEMIS_ENABLE_LLM` | `OFF` | Enable LLM features |
| `CMAKE_BUILD_TYPE` | `Debug` | Build type: Debug, Release, RelWithDebInfo |

### Example Configurations

**Development Build** (fast compilation, debugging):
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_BUILD_TESTS=ON
```

**Production Build** (optimized):
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DTHEMIS_STRICT_BUILD=ON
```

**Testing Build** (with tests and benchmarks):
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DTHEMIS_BUILD_TESTS=ON \
    -DTHEMIS_BUILD_BENCHMARKS=ON
```

---

## Dependencies

### Dependency Detection

CMake will automatically detect available dependencies:

✅ **Found**: Feature enabled with full functionality  
⚠️ **Not Found (Optional)**: Feature disabled gracefully  
❌ **Not Found (Required)**: Build fails with clear error message

### Checking Dependencies

```bash
# Run CMake with verbose output
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Look for these messages:
# ✅ "GTest found - tests enabled"
# ✅ "Google Benchmark found - benchmarks enabled"
# ✅ "Prometheus C++ client found - metrics enabled"
# ⚠️ "Prometheus C++ client not found - metrics collection disabled"
```

### Installing Missing Dependencies

#### Using vcpkg (Recommended)

```bash
# Install all optional dependencies
vcpkg install gtest benchmark prometheus-cpp

# Or install individually
vcpkg install gtest           # For unit tests
vcpkg install benchmark       # For performance benchmarks
vcpkg install prometheus-cpp  # For metrics collection
```

#### Using System Package Manager

**Ubuntu/Debian:**
```bash
sudo apt-get install libgtest-dev libbenchmark-dev
```

**macOS (Homebrew):**
```bash
brew install googletest google-benchmark prometheus-cpp
```

**Windows:**
Use vcpkg (recommended)

---

## Building

### Build Targets

#### All Targets

```bash
# Build everything
cmake --build . -j$(nproc)
```

#### Specific Targets

```bash
# Build only LoRA framework tests
cmake --build . --target test_lora_framework

# Build only LoRA framework benchmarks
cmake --build . --target bench_lora_framework

# Build with verbose output
cmake --build . --target test_lora_framework --verbose
```

### Build Output

```
build/
├── tests/
│   └── test_lora_framework          # Unit test executable
├── benchmarks/
│   └── bench_lora_framework         # Benchmark executable
└── compile_commands.json            # For IDE integration
```

---

## Running Tests

### Basic Test Execution

```bash
# Run all LoRA framework tests
./tests/test_lora_framework

# Run with verbose output
./tests/test_lora_framework --gtest_verbose=true

# Run specific test suite
./tests/test_lora_framework --gtest_filter="*StorageService*"

# List all available tests
./tests/test_lora_framework --gtest_list_tests
```

### Using CTest

```bash
# Run all tests through CTest
ctest --output-on-failure

# Run with verbose output
ctest -V

# Run specific test
ctest -R LoRAFramework

# Run tests in parallel
ctest -j$(nproc)
```

### Test Output Formats

**XML Output** (for CI/CD):
```bash
./tests/test_lora_framework --gtest_output=xml:test_results.xml
```

**JSON Output**:
```bash
./tests/test_lora_framework --gtest_output=json:test_results.json
```

### Custom Test Target

```bash
# Run tests through custom target
make run_tests
# or
cmake --build . --target run_tests
```

---

## Running Benchmarks

### Basic Benchmark Execution

```bash
# Run all benchmarks
./benchmarks/bench_lora_framework

# Run specific benchmark
./benchmarks/bench_lora_framework --benchmark_filter="Manager_HotSwap"

# Run with minimal time per benchmark
./benchmarks/bench_lora_framework --benchmark_min_time=0.1

# Run with repetitions for statistical analysis
./benchmarks/bench_lora_framework --benchmark_repetitions=10
```

### Benchmark Output Formats

**JSON Output** (for analysis tools):
```bash
./benchmarks/bench_lora_framework --benchmark_out=results.json --benchmark_out_format=json
```

**CSV Output**:
```bash
./benchmarks/bench_lora_framework --benchmark_out=results.csv --benchmark_out_format=csv
```

### Custom Benchmark Targets

```bash
# Run all benchmarks
make run_benchmarks

# Run quick benchmarks (< 10 seconds)
make run_benchmarks_quick

# Run detailed benchmarks with statistics
make run_benchmarks_detailed
```

### Analyzing Results

```bash
# Compare two benchmark runs
./benchmarks/bench_lora_framework --benchmark_out=baseline.json
# ... make changes ...
./benchmarks/bench_lora_framework --benchmark_out=current.json

# Compare (requires benchmark tools)
python3 compare.py benchmarks baseline.json current.json
```

---

## Troubleshooting

### Common Issues

#### 1. GTest Not Found

**Error:**
```
GTest not found - tests will not be built
Install with: vcpkg install gtest
```

**Solution:**
```bash
vcpkg install gtest
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

#### 2. Google Benchmark Not Found

**Error:**
```
Google Benchmark not found - benchmarks will not be built
Install with: vcpkg install benchmark
```

**Solution:**
```bash
vcpkg install benchmark
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

#### 3. Prometheus Not Found (Non-Critical)

**Warning:**
```
Prometheus C++ client not found - metrics collection disabled
```

**Solution (Optional):**
```bash
vcpkg install prometheus-cpp
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

This is optional - metrics features will be disabled but framework will still work.

#### 4. Missing Headers in Tests/Benchmarks

**Error:**
```
fatal error: llm/lora_framework/lora_adapter_manager.h: No such file or directory
```

**Solution:**
Ensure include directories are correct:
```bash
# Check if headers exist
ls -la include/llm/lora_framework/

# Reconfigure CMake
rm -rf build/*
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

#### 5. Linker Errors

**Error:**
```
undefined reference to `themis::llm::lora::LoRAAdapterManager::loadAdapter`
```

**Solution:**
Implementation files are not yet compiled. This is expected until the main library is built.
The tests and benchmarks currently contain placeholder/mock implementations.

---

## Advanced Configuration

### Cross-Compilation

```bash
# For ARM64
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=arm64-linux
```

### Static Linking

```bash
cmake .. \
    -DTHEMIS_STATIC_BUILD=ON \
    -DCMAKE_BUILD_TYPE=Release
```

### With Sanitizers (Debug)

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DTHEMIS_ENABLE_ASAN=ON
```

---

## CI/CD Integration

### GitHub Actions Example

```yaml
name: LoRA Framework Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install dependencies
        run: |
          vcpkg install gtest benchmark prometheus-cpp
      
      - name: Configure
        run: |
          mkdir build && cd build
          cmake .. \
            -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
            -DTHEMIS_BUILD_TESTS=ON \
            -DTHEMIS_BUILD_BENCHMARKS=ON
      
      - name: Build
        run: cmake --build build --target test_lora_framework
      
      - name: Test
        run: |
          cd build
          ctest --output-on-failure
```

---

## Performance Tips

### Build Performance

1. **Use Ninja instead of Make:**
   ```bash
   cmake .. -G Ninja
   ninja test_lora_framework
   ```

2. **Parallel builds:**
   ```bash
   cmake --build . -j$(nproc)
   ```

3. **Ccache for faster rebuilds:**
   ```bash
   cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

### Test Performance

1. **Run tests in parallel:**
   ```bash
   ctest -j$(nproc)
   ```

2. **Skip slow tests during development:**
   ```bash
   ./tests/test_lora_framework --gtest_filter=-*Slow*
   ```

---

## Next Steps

1. **Explore Tests**: See `LORA_TESTING_AND_METRICS_GUIDE.md`
2. **View Examples**: See `LORA_USAGE_EXAMPLES.md`
3. **Learn Architecture**: See `LLM_LORA_UNIFIED_ARCHITECTURE.md`
4. **Integration**: See `LLM_LORA_LLAMACPP_INTEGRATION.md`

---

## Support

For issues or questions:
- 📖 Check documentation: `docs/`
- 🐛 Report bugs: GitHub Issues
- 💬 Discuss: GitHub Discussions
