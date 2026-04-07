# Build & Test Verification Guide

**Date:** December 15, 2025  
**Purpose:** Verify vector encryption implementation builds and tests correctly  
**Version:** v1.3.0  
**Kategorie:** 🔒 Security

---

## 📑 Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Build & Test](#quick-build--test)
- [Detailed Verification Steps](#detailed-verification-steps)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required

- CMake 3.20 or higher
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- vcpkg or system packages for dependencies

### Dependencies

- OpenSSL (for AES-256-GCM encryption)
- RocksDB (for storage)
- GoogleTest (for tests)
- Boost (for system utilities)

---

## Quick Build & Test

### 1. Configure

```bash
cd /path/to/ThemisDB
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON
```

**Expected output:**
```
-- The CXX compiler identification is ...
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

### 2. Build

```bash
cmake --build build -j$(nproc)
```

**Expected output:**
```
[100%] Built target themis_core
[100%] Built target themis_tests
[100%] Built target themis_server
```

### 3. Run Tests

```bash
cd build
ctest -R vector_encryption -V
```

**Expected output:**
```
Test project .../build
    Start 1: VectorEncryptionIntegrationTest.Phase1_VectorEncryptionOnly
1/8 Test #1: VectorEncryptionIntegrationTest.Phase1_VectorEncryptionOnly .......   Passed    0.15 sec
    Start 2: VectorEncryptionIntegrationTest.Phase2_HnswEncryptionOnly
2/8 Test #2: VectorEncryptionIntegrationTest.Phase2_HnswEncryptionOnly .........   Passed    0.12 sec
    Start 3: VectorEncryptionIntegrationTest.FullEncryption_BothPhases
3/8 Test #3: VectorEncryptionIntegrationTest.FullEncryption_BothPhases .........   Passed    0.18 sec
    Start 4: VectorEncryptionIntegrationTest.BackwardCompatibility_PlaintextIndex
4/8 Test #4: VectorEncryptionIntegrationTest.BackwardCompatibility_PlaintextIndex   Passed    0.10 sec
    Start 5: VectorEncryptionIntegrationTest.MixedMode_EncryptedAndPlaintext
5/8 Test #5: VectorEncryptionIntegrationTest.MixedMode_EncryptedAndPlaintext ...   Passed    0.14 sec
    Start 6: VectorEncryptionIntegrationTest.Performance_EncryptionOverhead
6/8 Test #6: VectorEncryptionIntegrationTest.Performance_EncryptionOverhead ....   Passed    2.45 sec
    Start 7: VectorEncryptionIntegrationTest.ErrorHandling_MissingEncryptionKey
7/8 Test #7: VectorEncryptionIntegrationTest.ErrorHandling_MissingEncryptionKey   Passed    0.05 sec
    Start 8: VectorEncryptionIntegrationTest.AutoSave_OnShutdown
8/8 Test #8: VectorEncryptionIntegrationTest.AutoSave_OnShutdown ...............   Passed    0.11 sec

100% tests passed, 0 tests failed out of 8
```

---

## Detailed Verification Steps

### Step 1: Verify Source Files

```bash
# Check all source files exist
ls -la include/index/vector_index.h
ls -la src/index/vector_index.cpp
ls -la src/security/encrypted_field.cpp
ls -la tests/test_vector_encryption_integration.cpp
ls -la tools/migrate_vector_encryption.cpp
ls -la examples/example_vector_encryption.cpp
```

**All files should exist.**

### Step 2: Verify CMakeLists Registration

```bash
# Check test is registered
grep -n "test_vector_encryption_integration" CMakeLists.txt
```

**Expected output:**
```
965:        tests/test_vector_encryption_integration.cpp
```

### Step 3: Build with Verbose Output

```bash
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --verbose 2>&1 | grep -i "vector_encryption"
```

**Expected: Compilation of test file**

### Step 4: Run Individual Tests

```bash
cd build

# Run specific test
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.Phase1_VectorEncryptionOnly"

# Run with verbose output
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.*" --gtest_verbose
```

### Step 5: Check Test Output

```bash
# Run tests and capture output
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.Performance_EncryptionOverhead" 2>&1 | tee test_output.txt

# Should see performance metrics:
grep "Encrypted insert" test_output.txt
grep "Encrypted HNSW save" test_output.txt
grep "Encrypted HNSW load" test_output.txt
```

**Expected output:**
```
Encrypted insert (1000 vectors): XXX ms
Per-vector: X.XX ms
Encrypted HNSW save: XXX ms
Encrypted HNSW load: XXX ms
```

---

## Troubleshooting

### Issue 1: CMake Can't Find Dependencies

**Error:**
```
CMake Error: Could not find a package configuration file provided by "OpenSSL"
```

**Solution:**
```bash
# Install via vcpkg
vcpkg install openssl rocksdb gtest boost-system

# Or via system package manager (Ubuntu/Debian)
sudo apt-get install libssl-dev librocksdb-dev libgtest-dev libboost-dev

# Or via system package manager (macOS)
brew install openssl rocksdb googletest boost
```

### Issue 2: Compilation Errors

**Error:**
```
error: 'EncryptedField' was not declared in this scope
```

**Solution:**
```bash
# Check if encryption header exists
ls -la include/security/encryption.h

# Verify include paths in CMakeLists.txt
grep -n "include_directories" CMakeLists.txt
```

### Issue 3: Linking Errors

**Error:**
```
undefined reference to `EncryptedField<std::vector<float>>::encrypt(...)`
```

**Solution:**
```bash
# Verify encrypted_field.cpp is in the build
grep -n "encrypted_field.cpp" CMakeLists.txt

# Rebuild from scratch
rm -rf build
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON
cmake --build build
```

### Issue 4: Tests Fail to Run

**Error:**
```
Test #X: VectorEncryptionIntegrationTest.Phase1_VectorEncryptionOnly .......***Failed
```

**Debug:**
```bash
# Run with verbose output
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.Phase1_VectorEncryptionOnly" --gtest_verbose

# Check logs
grep -i "error\|fail" test_output.txt

# Common issues:
# - FieldEncryption not initialized
# - RocksDB path permissions
# - Missing encryption keys
```

### Issue 5: Performance Tests Time Out

**Error:**
```
Test timeout exceeded
```

**Solution:**
```bash
# Increase timeout (in CTestTestfile.cmake or manually)
ctest -R vector_encryption --timeout 300

# Or skip performance tests for quick verification
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.*:-*.Performance*"
```

---

## Platform-Specific Notes

### Linux

```bash
# Standard build
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON
cmake --build build -j$(nproc)
cd build && ctest -R vector_encryption
```

**Works on:** Ubuntu 20.04+, Debian 11+, Fedora 35+, Arch Linux

### macOS

```bash
# May need to specify OpenSSL path
export OPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON
cmake --build build -j$(sysctl -n hw.ncpu)
cd build && ctest -R vector_encryption
```

**Works on:** macOS 11+ (Big Sur, Monterey, Ventura, Sonoma)

### Windows (MSVC)

```powershell
# Using vcpkg
vcpkg install openssl:x64-windows rocksdb:x64-windows gtest:x64-windows

# Build
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
cd build
ctest -C Release -R vector_encryption
```

**Works on:** Windows 10+, Server 2019+

### Windows (MinGW)

```bash
# Similar to Linux
cmake -B build -S . -G "MinGW Makefiles" -DTHEMIS_BUILD_TESTS=ON
cmake --build build
cd build && ctest -R vector_encryption
```

---

## Verification Checklist

Before marking as verified, ensure:

- [ ] CMake configuration succeeds without errors
- [ ] All source files compile without warnings (with `-Wall -Wextra`)
- [ ] Linking succeeds without undefined references
- [ ] All 8 integration tests pass
- [ ] Performance tests complete within reasonable time (<5 minutes)
- [ ] No memory leaks (run with valgrind or ASAN if available)
- [ ] Tests pass on target platforms (Linux/macOS/Windows)

---

## Performance Baseline

Expected performance on modern hardware (Intel i7/Ryzen 7, SSD):

| Test | Expected Time |
|------|---------------|
| Phase1_VectorEncryptionOnly | < 0.5 sec |
| Phase2_HnswEncryptionOnly | < 0.5 sec |
| FullEncryption_BothPhases | < 0.5 sec |
| BackwardCompatibility_PlaintextIndex | < 0.3 sec |
| MixedMode_EncryptedAndPlaintext | < 0.4 sec |
| Performance_EncryptionOverhead | < 5 sec |
| ErrorHandling_MissingEncryptionKey | < 0.1 sec |
| AutoSave_OnShutdown | < 0.3 sec |
| **Total** | **< 8 sec** |

If tests take significantly longer, check:
- Disk I/O speed (use SSD)
- CPU performance
- Debug vs Release build (use Release for performance)

---

## Advanced Verification

### Memory Leak Check (Linux)

```bash
# Install valgrind
sudo apt-get install valgrind

# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  ./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.Phase1_VectorEncryptionOnly"
```

**Expected:** "All heap blocks were freed -- no leaks are possible"

### Address Sanitizer (Linux/macOS)

```bash
# Build with ASAN
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_ASAN=ON
cmake --build build

# Run tests
cd build
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.*"
```

**Expected:** No ASAN errors

### Code Coverage

```bash
# Build with coverage
cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS="--coverage"
cmake --build build

# Run tests
cd build
./themis_tests --gtest_filter="VectorEncryptionIntegrationTest.*"

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html

# View coverage
open coverage_html/index.html
```

**Expected:** >90% coverage for encryption code

---

## Success Criteria

✅ **Build Success:**
- CMake configuration: PASS
- Compilation: PASS (no errors, minimal warnings)
- Linking: PASS

✅ **Test Success:**
- All 8 integration tests: PASS
- No crashes or segfaults
- No memory leaks
- Performance within baseline

✅ **Code Quality:**
- No ASAN errors
- No valgrind errors
- Clean static analysis (if available)

---

## Reporting Issues

If verification fails, collect:

1. **Build output:**
   ```bash
   cmake -B build -S . -DTHEMIS_BUILD_TESTS=ON 2>&1 | tee build.log
   cmake --build build 2>&1 | tee compile.log
   ```

2. **Test output:**
   ```bash
   cd build
   ctest -R vector_encryption -V 2>&1 | tee test.log
   ```

3. **System info:**
   ```bash
   uname -a
   cmake --version
   g++ --version  # or clang++ --version
   ```

4. **Create GitHub issue** with logs attached

---

## Next Steps

After successful verification:

1. ✅ Build passes
2. ✅ All tests pass
3. → **Run benchmarks** (see PERFORMANCE_OPTIMIZATION_NOTES.md)
4. → **Deploy to staging** (see VECTOR_ENCRYPTION_CONFIGURATION.md)
5. → **Production rollout** (see deployment checklist in PHASE1_FINAL_REPORT.md)

---

**Last Updated:** April 2026  
**Status:** Ready for Verification  
**Maintainer:** ThemisDB Security Team
