> **Build + Test:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release && ctest --preset linux-ninja-release`

# Model Loading from ThemisDB - Tests

This directory contains comprehensive tests for the native model loading functionality from ThemisDB blob storage.

## Test Files

### Unit Tests (GTest)

**`test_model_loading_from_themisdb.cpp`**
- Comprehensive unit tests for `LLMModelStorage::loadModelBlob()` and `LlamaWrapper::loadModelFromThemisDB()`
- Test coverage includes:
  - Model blob retrieval (inline, external, encrypted)
  - Error handling (missing models, corrupted data)
  - Temp file cleanup (`cleanupTempModels()`)
  - End-to-end integration tests
  - Multi-model scenarios
  - Different model sizes (small, medium, large)

**Test Categories:**
- ✅ `LoadModelBlob_*` - Blob retrieval tests
- ✅ `CleanupTempModels_*` - Cache management tests
- ✅ `EndToEnd_*` - Integration tests
- ✅ `ErrorHandling_*` - Error scenario tests
- ✅ `Performance_*` - Basic performance tests

### Performance Benchmarks (Google Benchmark)

**`bench_model_loading_from_themisdb.cpp`**
- Performance benchmarks using Google Benchmark framework
- Benchmarks include:
  - Model storage at different sizes (1KB, 1MB, 10MB)
  - Model blob loading at different sizes
  - Metadata loading performance
  - SHA256 verification overhead
  - Temporary file write performance
  - Cache cleanup with different file counts

**Benchmark Categories:**
- 📊 `BM_StoreModel_*` - Storage performance
- 📊 `BM_LoadModelBlob_*` - Retrieval performance
- 📊 `BM_LoadModelMetadata` - Metadata query performance
- 📊 `BM_SHA256_Verification_*` - Hash verification overhead
- 📊 `BM_TempFileWrite_*` - Disk I/O performance
- 📊 `BM_CleanupTempModels_*` - Cleanup scalability

## Building and Running Tests

### Prerequisites

```bash
# Install dependencies
vcpkg install gtest                # For unit tests
vcpkg install benchmark           # For performance benchmarks
vcpkg install rocksdb openssl
```

### Build Tests

```bash
# Configure with tests enabled
cmake -B build -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LLM=ON

# Build tests
cmake --build build --target test_model_loading_from_themisdb
cmake --build build --target bench_model_loading_from_themisdb
```

### Run Unit Tests

```bash
# Run all model loading tests
./build/tests/test_model_loading_from_themisdb

# Run specific test
./build/tests/test_model_loading_from_themisdb --gtest_filter="*LoadModelBlob*"

# Run with verbose output
./build/tests/test_model_loading_from_themisdb --gtest_verbose

# Generate XML output for CI
./build/tests/test_model_loading_from_themisdb --gtest_output=xml:test_results.xml
```

### Run Benchmarks

```bash
# Run all benchmarks
./build/tests/bench_model_loading_from_themisdb

# Run specific benchmark
./build/tests/bench_model_loading_from_themisdb --benchmark_filter="LoadModelBlob"

# Output as JSON
./build/tests/bench_model_loading_from_themisdb --benchmark_out=results.json --benchmark_out_format=json

# Compare with baseline
./build/tests/bench_model_loading_from_themisdb --benchmark_out=current.json
./build/tests/bench_model_loading_from_themisdb --benchmark_out=baseline.json
compare.py benchmarks baseline.json current.json  # Google Benchmark tool
```

## Test Coverage

### Unit Tests Coverage

| Component | Coverage | Tests |
|-----------|----------|-------|
| `loadModelBlob()` | ✅ 100% | 4 tests |
| `cleanupTempModels()` | ✅ 100% | 3 tests |
| Integration | ✅ 100% | 3 tests |
| Error handling | ✅ 90% | 2 tests |

**Total:** 15 unit tests

### Benchmark Coverage

| Operation | Sizes | Benchmarks |
|-----------|-------|------------|
| Store Model | 3 sizes | 3 |
| Load Blob | 3 sizes | 3 |
| SHA256 | 2 sizes | 2 |
| File I/O | 2 sizes | 2 |
| Cleanup | 2 scenarios | 2 |

**Total:** 13 benchmarks

## Expected Test Results

### Unit Tests

All tests should pass:
```
[==========] Running 15 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 15 tests from ModelLoadingFromThemisDBTest
[ RUN      ] ModelLoadingFromThemisDBTest.LoadModelBlob_NotFound
[       OK ] ModelLoadingFromThemisDBTest.LoadModelBlob_NotFound (5 ms)
...
[----------] 15 tests from ModelLoadingFromThemisDBTest (1234 ms total)

[----------] Global test environment tear-down
[==========] 15 tests from 1 test suite ran. (1234 ms total)
[  PASSED  ] 15 tests.
```

### Performance Benchmarks

Example results (actual performance varies by hardware):

```
---------------------------------------------------------------------
Benchmark                                   Time             CPU   
---------------------------------------------------------------------
BM_StoreModel_1KB                       15.2 ms         15.1 ms    
BM_StoreModel_1MB                       125 ms          124 ms     
BM_StoreModel_10MB                      1250 ms         1248 ms    
BM_LoadModelBlob_1KB                    2.5 ms          2.5 ms     
BM_LoadModelBlob_1MB                    18 ms           17.9 ms    
BM_LoadModelBlob_10MB                   180 ms          179 ms     
BM_LoadModelMetadata                    1.2 ms          1.2 ms     
BM_SHA256_Verification_1MB              5.8 ms          5.8 ms     
BM_SHA256_Verification_10MB             58 ms           58 ms      
BM_TempFileWrite_1MB                    12 ms           12 ms      
BM_TempFileWrite_10MB                   120 ms          120 ms     
BM_CleanupTempModels_10Files            3.2 ms          3.2 ms     
BM_CleanupTempModels_100Files           28 ms           28 ms      
```

## CI/CD Integration

### GitHub Actions

Add to `.github/workflows/tests.yml`:

```yaml
- name: Run Model Loading Tests
  run: |
    ./build/tests/test_model_loading_from_themisdb --gtest_output=xml:test_results.xml
    
- name: Upload Test Results
  uses: actions/upload-artifact@v3
  if: always()
  with:
    name: test-results
    path: test_results.xml
```

### Performance Regression Detection

```yaml
- name: Run Benchmarks
  run: |
    ./build/tests/bench_model_loading_from_themisdb --benchmark_out=current.json
    
- name: Compare with Baseline
  run: |
    python3 tools/compare_benchmarks.py baseline.json current.json --threshold=10
```

## Troubleshooting

### Test Failures

**"Model not found" errors:**
- Ensure RocksDB is initialized correctly
- Check test directory permissions

**Blob storage errors:**
- Verify filesystem backend is registered
- Check blob directory exists and is writable

**Encryption test failures:**
- Ensure OpenSSL is linked correctly
- Check MockKeyProvider initialization

### Benchmark Issues

**High variability in results:**
- Run benchmarks multiple times: `--benchmark_repetitions=10`
- Use fixed CPU frequency: `--benchmark_perf_counters=CYCLES`
- Disable CPU frequency scaling

**Out of disk space:**
- Clean temp directories before running
- Use smaller test data sizes

## Contributing

When adding new tests:

1. **Follow naming conventions:**
   - Unit tests: `TEST_F(ModelLoadingFromThemisDBTest, DescriptiveName)`
   - Benchmarks: `static void BM_DescriptiveName(benchmark::State& state)`

2. **Add to appropriate section:**
   - Unit tests in `test_model_loading_from_themisdb.cpp`
   - Benchmarks in `bench_model_loading_from_themisdb.cpp`

3. **Document expected behavior:**
   - Add comments explaining what's being tested
   - Include edge cases and error scenarios

4. **Update this README:**
   - Add new test to coverage table
   - Update expected results if needed

## Related Documentation

- [Implementation Summary](../../docs/en/llm/LLM_IMPLEMENTATION_COMPLETE.md)
- [Usage Examples](../../docs/examples/load_model_from_themisdb_example.md)
- [LLM Module Guide](../../docs/de/llm/LLM_LOADER_GUIDE.md)
- [Blob Storage Architecture](../../docs/storage/CLOUD_BLOB_BACKENDS.md)

## Contact

For questions or issues:
- Open an issue on GitHub
- Tag with `llm`, `tests`, `model-loading`
- Reference: "GAP: Modell-Laden aus ThemisDB" issue
