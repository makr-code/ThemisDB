# LLM Testing Guide

Complete guide for testing the ThemisDB LLM implementation.

## Overview

This guide covers unit tests, integration tests, and benchmarks for the LLaMA.cpp integration.

## Test Structure

```
tests/
├── test_embedded_llm.cpp           # Core EmbeddedLLM tests (38 tests)
├── test_voice_llm_integration.cpp  # Voice Assistant tests (10 tests)
└── test_content_llm_integration.cpp # Content Analysis tests (8 tests)

benchmarks/
├── bench_embedded_llm.cpp          # Core benchmarks (12 benchmarks)
├── bench_voice_llm.cpp             # Voice benchmarks (3 benchmarks)
└── bench_content_llm.cpp           # Content benchmarks (3 benchmarks)
```

## Prerequisites

### Required Files
- **Model File**: TinyLlama-1.1B GGUF format (~637MB)
- **Test Data**: Sample documents for content analysis

### Environment Setup

```bash
# Set model path
export THEMIS_LLM_MODEL_PATH="models/tinyllama-1.1b-q4_0.gguf"

# Optional: Set test data directory
export THEMIS_TEST_DATA_DIR="tests/data"
```

## Building Tests

### CMake Configuration

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_BUILD_TESTS=ON \
      -DTHEMIS_BUILD_BENCHMARKS=ON \
      .

cmake --build . --target all
```

### Build Targets

```bash
# Build all tests
cmake --build . --target tests

# Build specific test
cmake --build . --target test_embedded_llm

# Build all benchmarks
cmake --build . --target benchmarks
```

## Running Tests

### All Tests

```bash
# Run via CTest
ctest --output-on-failure

# Or directly
./build/tests/run_all_tests
```

### Specific Test Suites

```bash
# EmbeddedLLM core tests
./build/tests/test_embedded_llm

# Voice Assistant integration
./build/tests/test_voice_llm_integration

# Content Analysis integration
./build/tests/test_content_llm_integration

# With Google Test filters
./build/tests/test_embedded_llm --gtest_filter=EmbeddedLLMTest.BasicGeneration
```

### Test Output

```
[==========] Running 38 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 15 tests from EmbeddedLLMTest
[ RUN      ] EmbeddedLLMTest.BasicGeneration
[       OK ] EmbeddedLLMTest.BasicGeneration (145 ms)
...
[==========] 38 tests from 3 test suites ran. (15234 ms total)
[  PASSED  ] 38 tests.
```

## Running Benchmarks

### Basic Execution

```bash
# Run all benchmarks
./build/benchmarks/bench_embedded_llm

# Run with repetitions
./build/benchmarks/bench_embedded_llm --benchmark_repetitions=10

# Run specific benchmark
./build/benchmarks/bench_embedded_llm --benchmark_filter=BM_LLM_Generation
```

### Benchmark Options

```bash
# JSON output
./build/benchmarks/bench_embedded_llm \
    --benchmark_format=json \
    --benchmark_out=results.json

# CSV output
./build/benchmarks/bench_embedded_llm \
    --benchmark_format=csv \
    --benchmark_out=results.csv

# Custom time unit
./build/benchmarks/bench_embedded_llm \
    --benchmark_time_unit=ms
```

### Benchmark Output

```
Run on (8 X 3400 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 1.23, 1.45, 1.67
--------------------------------------------------------------------
Benchmark                           Time           CPU   Iterations
--------------------------------------------------------------------
BM_LLM_Generation_Latency/10     45.2 ms       44.8 ms          16
BM_LLM_Generation_Latency/50      186 ms        185 ms           4
BM_LLM_Embeddings/10             11.5 μs       11.4 μs       61234
BM_LLM_Embeddings/100            45.2 μs       44.9 μs       15543
```

## Test Categories

### 1. Initialization Tests
- Singleton pattern
- Configuration loading
- Resource initialization

### 2. Text Generation Tests
- Basic generation
- Parameter variations (max_tokens, temperature)
- Edge cases (empty prompt, very long)
- Error handling

### 3. Embeddings Tests
- Basic embedding generation
- Normalization verification
- Consistency checks
- Batch processing

### 4. Chat Tests
- Single-turn conversations
- Multi-turn conversations
- Chat formats (ChatML, Llama-2, etc.)
- Role handling (system, user, assistant)

### 5. Streaming Tests
- Token-by-token delivery
- Callback functionality
- SSE format validation
- MCP format validation

### 6. Thread Safety Tests
- Concurrent generation
- Concurrent embeddings
- Race condition detection
- Lock contention

### 7. Integration Tests
- AQL integration
- MCP server integration
- HTTP API integration
- Voice Assistant integration
- Content Analysis integration

## Test Coverage

Current test coverage:

| Component | Tests | Coverage |
|-----------|-------|----------|
| EmbeddedLLM Core | 15 | 95% |
| Voice Assistant | 10 | 90% |
| Content Analysis | 8 | 85% |
| Integration | 5 | 80% |
| **Total** | **38** | **88%** |

## Performance Baselines

### Text Generation
- **First Token Latency**: < 50ms
- **Throughput**: > 50 tokens/sec (CPU), > 100 tokens/sec (GPU)
- **Total Time (50 tokens)**: < 1s (CPU), < 500ms (GPU)

### Embeddings
- **Latency**: < 15ms (10 words), < 50ms (100 words)
- **Throughput**: > 2000 embeddings/sec

### Chat
- **Latency**: < 1s (single turn), < 2s (5 turns)
- **Overhead**: ~180ms per additional turn

### Concurrent
- **Linear Scaling**: Up to 4 threads
- **Sublinear Scaling**: 4-16 threads
- **Throughput**: 50 req/sec (1 thread), 150-200 req/sec (8 threads)

## Troubleshooting

### Common Issues

#### 1. Model Not Found
```
Error: Failed to load model: File not found
```

**Solution:**
```bash
export THEMIS_LLM_MODEL_PATH=/path/to/model.gguf
```

#### 2. Out of Memory
```
Error: Failed to allocate context
```

**Solution:**
- Reduce `n_ctx` in config
- Reduce `n_gpu_layers` to use less VRAM
- Use quantized model (Q4_0 instead of F16)

#### 3. Tests Timing Out
```
Test timeout after 300 seconds
```

**Solution:**
```bash
# Increase timeout
ctest --timeout 600

# Or skip slow tests
ctest -E "SlowTest"
```

#### 4. Benchmark Variance
```
High variance in benchmark results
```

**Solution:**
```bash
# Run more repetitions
./bench --benchmark_repetitions=20

# Use median instead of mean
./bench --benchmark_report_aggregates_only=true
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: LLM Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Download Model
        run: |
          mkdir -p models
          wget https://example.com/tinyllama.gguf -O models/tinyllama.gguf
      
      - name: Build
        run: |
          cmake -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_BUILD_TESTS=ON .
          cmake --build .
      
      - name: Run Tests
        run: ctest --output-on-failure
        env:
          THEMIS_LLM_MODEL_PATH: models/tinyllama.gguf
```

## Best Practices

### 1. Test Independence
- Each test should be independent
- No shared state between tests
- Clean up resources in TearDown()

### 2. Fast Tests
- Use small max_tokens in tests
- Cache model loading across tests
- Mock heavy operations when possible

### 3. Meaningful Assertions
- Test behavior, not implementation
- Use appropriate matchers (EXPECT_GT, EXPECT_NEAR)
- Add descriptive failure messages

### 4. Benchmark Accuracy
- Run with `--benchmark_repetitions=10`
- Report both mean and median
- Note hardware specifications

## Advanced Topics

### Memory Leak Detection

```bash
# With Valgrind
valgrind --leak-check=full ./build/tests/test_embedded_llm

# With Address Sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" .
cmake --build .
./build/tests/test_embedded_llm
```

### Profiling

```bash
# CPU profiling with perf
perf record ./build/benchmarks/bench_embedded_llm
perf report

# Memory profiling with massif
valgrind --tool=massif ./build/benchmarks/bench_embedded_llm
ms_print massif.out.*
```

### Custom Test Data

```bash
# Generate test documents
python scripts/generate_test_data.py --count=100 --output=tests/data

# Run tests with custom data
THEMIS_TEST_DATA_DIR=tests/data ./build/tests/test_content_llm_integration
```

## References

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [llama.cpp Documentation](https://github.com/ggerganov/llama.cpp)
- [ThemisDB LLM Architecture](./LLAMA_IMPLEMENTATION_SUMMARY.md)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: docs/llm/

---

**Last Updated**: January 2026  
**Version**: 1.0  
**Author**: ThemisDB Team / GitHub Copilot
