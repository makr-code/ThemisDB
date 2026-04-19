> **Build + Test:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release && ctest --preset linux-ninja-release`

# Running LoRA Framework Tests

## Quick Start

### Build Tests

```bash
# From repository root
cmake -DTHEMIS_BUILD_TESTS=ON -B build
cmake --build build --target test_lora_framework_comprehensive
```

### Run Tests

```bash
cd build
ctest --output-on-failure -R LoRAFramework
```

## Test Files

1. **test_lora_framework.cpp** - Original integration tests (20 tests)
2. **test_lora_framework_comprehensive.cpp** - Comprehensive unit tests (50+ tests) ⭐

## Test Execution

### Run All LoRA Tests
```bash
ctest -R LoRA
```

### Run Comprehensive Tests Only
```bash
./tests/test_lora_framework_comprehensive
```

### Run with Filters
```bash
./tests/test_lora_framework_comprehensive --gtest_filter=StorageService*
./tests/test_lora_framework_comprehensive --gtest_filter=*ThreadSafety*
```

### Verbose Output
```bash
./tests/test_lora_framework_comprehensive --gtest_verbose
```

## Expected Output

```
╔══════════════════════════════════════════════════════════════════╗
║  ThemisDB LoRA Framework - Comprehensive Unit Test Suite        ║
╚══════════════════════════════════════════════════════════════════╝

Test Coverage:
  ✓ LoRAStorageService (save, load, delete, versioning)
  ✓ LoRAAdapterManager (lifecycle, caching, hot-swap)
  ✓ LoRATrainingService (training, callbacks, checkpoints)
  ✓ MultiLoRAManager (quantization, multi-GPU, fusion)
  ✓ Thread-safety (concurrent reads/writes)
  ✓ Error handling and edge cases
  ✓ Memory management and leak detection
  ✓ Performance benchmarks
  ✓ Integration scenarios

[==========] Running 50 tests from 1 test suite.
...
[  PASSED  ] 50 tests.
```

## Troubleshooting

### GTest Not Found
```bash
# Install via vcpkg
vcpkg install gtest

# Or via apt (Ubuntu/Debian)
sudo apt-get install libgtest-dev
```

### Build Fails
```bash
# Clean build
rm -rf build
cmake -DTHEMIS_BUILD_TESTS=ON -B build
cmake --build build
```

### Tests Timeout
Some tests may take longer on slower systems. Increase timeout:
```bash
ctest -R LoRA --timeout 600
```

## Documentation

See [LORA_FRAMEWORK_TEST_DOCUMENTATION.md](LORA_FRAMEWORK_TEST_DOCUMENTATION.md) for:
- Complete test coverage matrix
- Test categories and descriptions
- Development guidelines
- Adding new tests

## CI/CD Integration

Tests run automatically in GitHub Actions:
```yaml
- name: Build and Test
  run: |
    cmake -DTHEMIS_BUILD_TESTS=ON -B build
    cmake --build build
    cd build && ctest --output-on-failure -R LoRA
```
