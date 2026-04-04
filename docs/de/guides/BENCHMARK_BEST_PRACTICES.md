# Benchmark Best Practices for ThemisDB

This document outlines best practices for writing and maintaining benchmarks in the ThemisDB project. Following these guidelines ensures consistent, accurate, and reproducible benchmark results.

## Table of Contents

1. [General Principles](#general-principles)
2. [Python Benchmarks](#python-benchmarks)
3. [C++ Benchmarks](#cpp-benchmarks)
4. [Complexity Analysis](#complexity-analysis)
5. [Reproducibility](#reproducibility)
6. [Common Pitfalls](#common-pitfalls)

## General Principles

### Accuracy Over Speed
- Prioritize accurate measurements over fast benchmark execution
- Ensure setup/teardown phases don't pollute timing measurements
- Use appropriate warmup iterations to eliminate cold-start effects

### Minimal Interference
- Isolate benchmarks from external factors (network, disk I/O, etc.)
- Use fixed random seeds for reproducible data generation
- Disable unnecessary background services during benchmarking

### Clear Documentation
- Document what each benchmark measures
- Specify expected performance characteristics (O(n), O(log n), etc.)
- Include context about hardware requirements or constraints

## Python Benchmarks

### HTTP Error Handling

**Always wrap HTTP requests in try-except blocks and handle errors appropriately:**

```python
# ❌ BAD: Silently ignores errors
try:
    response = client.get("/api/endpoint")
except:
    pass

# ✅ GOOD: Proper error handling with logging (Pattern 1 - timing inside try)
for _ in range(iterations):
    try:
        start = time.perf_counter()
        response = client.get("/api/endpoint")
        response.raise_for_status()
        latency_ms = (time.perf_counter() - start) * 1000
        result.latencies_ms.append(latency_ms)
    except httpx.HTTPError as e:
        print(f"[Benchmark] HTTP error skipped: {e}")
    except Exception as e:
        print(f"[Benchmark] Error skipped: {e}")

# ✅ ALSO GOOD: Alternative pattern (Pattern 2 - timing outside try)
for _ in range(iterations):
    start = time.perf_counter()
    try:
        response = client.get("/api/endpoint")
        response.raise_for_status()
        result.latencies_ms.append((time.perf_counter() - start) * 1000)
    except httpx.HTTPError as e:
        print(f"[Benchmark] HTTP error skipped: {e}")
    except Exception as e:
        print(f"[Benchmark] Error skipped: {e}")
```

**Important Notes:**
- Both patterns are correct and measure the actual operation time
- Pattern 1 (timing inside try): Measures from operation start to completion
- Pattern 2 (timing outside try): Also measures from operation start, records only on success
- Pattern 2 is slightly preferred as it makes timing scope more explicit
- On failure, no timing is recorded in either pattern (iteration is skipped)
- The goal is to measure successful operations only, excluding failed attempts

**Key points:**
- Use `raise_for_status()` to catch HTTP errors (4xx, 5xx)
- Only record timing for successful requests
- Log errors for debugging but don't let them crash the benchmark
- Skip failed requests rather than recording them with incorrect timing
- The timing should include the full HTTP request/response cycle

### Warmup Phases

Always include warmup iterations before actual measurements:

```python
# Warmup (not measured)
for _ in range(5):
    try:
        response = client.get("/api/endpoint")
        response.raise_for_status()
    except Exception as e:
        print(f"[Warmup] Error: {e}")

# Benchmark (measured)
for _ in range(50):
    start = time.perf_counter()
    try:
        response = client.get("/api/endpoint")
        response.raise_for_status()
        result.latencies_ms.append((time.perf_counter() - start) * 1000)
    except Exception as e:
        print(f"[Benchmark] Error skipped: {e}")
```

### Reproducibility Flags

Set consistent parameters for reproducible results:

```python
# Recommended settings
WARMUP_ITERATIONS = 5      # Stabilize performance
BENCHMARK_ITERATIONS = 50  # Sufficient for statistical significance
TIMEOUT = 30.0             # Reasonable timeout for operations
```

## C++ Benchmarks

### Timing Isolation with PauseTiming/ResumeTiming

**Isolate setup and teardown phases from measurements:**

```cpp
// ❌ BAD: Setup is included in timing
static void BM_Example(benchmark::State& state) {
    for (auto _ : state) {
        // Setup (measured - BAD!)
        auto data = generateTestData(1000);
        
        // Actual operation
        processData(data);
    }
}

// ✅ GOOD: Setup is excluded from timing
static void BM_Example(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup (not measured)
        auto data = generateTestData(1000);
        
        state.ResumeTiming();
        
        // Actual operation (measured)
        processData(data);
    }
}
```

**When to use PauseTiming/ResumeTiming:**
- Memory allocation for test data
- Database connection setup
- File I/O for loading test fixtures
- Random data generation
- Creating complex test structures

**When NOT to use it:**
- Operations that are part of the measured code path
- Lightweight operations (< 1% of total time)
- Operations that would normally be cached in production

### BENCHMARK_MAIN() Usage

**Always include BENCHMARK_MAIN() at the end of benchmark files:**

```cpp
// At the end of your benchmark file
BENCHMARK_MAIN();
```

This ensures the benchmark can be run as a standalone executable. Without it, you'll need to link against `benchmark_main` library explicitly.

### Fixture Usage

Use fixtures for complex setup/teardown:

```cpp
class MyBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // One-time setup before all iterations
        db_ = std::make_unique<Database>(test_config_);
        db_->initialize();
    }
    
    void TearDown(const ::benchmark::State& state) override {
        // One-time cleanup after all iterations
        db_->close();
        db_.reset();
    }
    
protected:
    std::unique_ptr<Database> db_;
};

BENCHMARK_DEFINE_F(MyBenchmarkFixture, QueryTest)(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark code using db_
        auto result = db_->query("SELECT * FROM table");
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_REGISTER_F(MyBenchmarkFixture, QueryTest)
    ->Threads(1)
    ->Threads(4)
    ->Unit(benchmark::kMillisecond);
```

## Complexity Analysis

### Adding Complexity Tracking

For algorithms with expected complexity (O(n), O(log n), etc.), add complexity analysis:

```cpp
BENCHMARK_F(MyBenchmark, StringLength)(benchmark::State& state) {
    std::string testStr(state.range(0), 'x');
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(performOperation(testStr));
    }
    
    state.SetComplexityN(state.range(0));
}

BENCHMARK_REGISTER_F(MyBenchmark, StringLength)
    ->Range(8, 8<<10)              // Test with 8 to 8192 elements
    ->Complexity();                  // Auto-detect complexity
    // Or specify explicitly:
    // ->Complexity(benchmark::oN)   // O(n)
    // ->Complexity(benchmark::oNLogN) // O(n log n)
```

### When to Add Complexity Analysis

Add `.Complexity()` when:
- The algorithm has known theoretical complexity
- Performance scales with input size
- You want to verify implementation matches theoretical expectations
- Comparing different algorithm implementations

Example benchmarks that benefit from complexity analysis:
- Sorting algorithms
- Search operations
- String operations (length-dependent)
- Array/vector operations
- Graph traversal algorithms

## Reproducibility

### Fixed Random Seeds

Use deterministic random generation:

```cpp
// C++
std::mt19937 rng(42);  // Fixed seed

// Python
import random
random.seed(42)
```

### Environment Documentation

Document expected environment in benchmark comments:

```cpp
/**
 * @brief Benchmark for vector operations
 * 
 * Requirements:
 * - CPU: x86_64 with AVX2 support
 * - Memory: At least 4GB RAM
 * - OS: Linux/macOS (uses POSIX features)
 * 
 * Expected performance:
 * - O(n) complexity
 * - ~100ns per element on modern hardware
 */
```

### Reporting Requirements

Include in benchmark results:
- Hardware specs (CPU, RAM, disk type)
- Software versions (compiler, libraries, OS)
- Configuration parameters
- Date and time of run
- Git commit hash

## Common Pitfalls

### 1. Compiler Optimization Eliminating Code

```cpp
// ❌ BAD: Compiler may optimize away the operation
for (auto _ : state) {
    int result = expensiveComputation();
    // result is unused, may be optimized away
}

// ✅ GOOD: Force compiler to keep the result
for (auto _ : state) {
    int result = expensiveComputation();
    benchmark::DoNotOptimize(result);
}
```

### 2. Insufficient Warmup

```cpp
// ❌ BAD: No warmup, first iterations slower
BENCHMARK(BM_ColdStart);

// ✅ GOOD: Warmup outside measurement
void BM_ProperWarmup(benchmark::State& state) {
    if (state.thread_index() == 0) {
        // Warmup phase
        for (int i = 0; i < 10; i++) {
            performOperation();
        }
    }
    
    for (auto _ : state) {
        performOperation();
    }
}
```

### 3. Measuring Allocation Instead of Algorithm

```cpp
// ❌ BAD: Includes allocation time
for (auto _ : state) {
    std::vector<int> data(1000000);  // Allocation measured!
    processVector(data);
}

// ✅ GOOD: Allocate outside measurement
for (auto _ : state) {
    state.PauseTiming();
    std::vector<int> data(1000000);
    state.ResumeTiming();
    
    processVector(data);
}
```

### 4. Insufficient Iterations

```python
# ❌ BAD: Too few iterations for statistical significance
BENCHMARK_ITERATIONS = 3

# ✅ GOOD: Enough iterations for stable results
BENCHMARK_ITERATIONS = 50  # or more for noisy operations
```

### 5. External Dependencies

```cpp
// ❌ BAD: Depends on external service availability
for (auto _ : state) {
    auto result = httpClient.get("http://example.com/api");
}

// ✅ GOOD: Use mocked/local services or document dependencies
for (auto _ : state) {
    auto result = httpClient.get("http://localhost:8765/api");
}
```

## Running Benchmarks

### C++ Benchmarks

```bash
# Run a single benchmark
./bench_transaction_throughput

# Run with specific parameters
./bench_aql_functions --benchmark_filter=String.*

# Generate JSON output
./bench_compliance_security_governance --benchmark_format=json --benchmark_out=results.json

# Control iterations and repetitions
./bench_distributed_coordinator --benchmark_min_time=5.0 --benchmark_repetitions=10
```

### Python Benchmarks

```bash
# Run a benchmark script
python3 benchmarks/comparative/scripts/fair_benchmark.py

# With timing control
python3 benchmarks/comparative/scripts/simple_benchmark.py --min-time=5 --repetitions=10
```

## Review Checklist

Before submitting benchmark code, ensure:

- [ ] HTTP requests have proper error handling (Python)
- [ ] Setup/teardown use PauseTiming/ResumeTiming (C++)
- [ ] BENCHMARK_MAIN() is present (C++)
- [ ] Appropriate warmup iterations are included
- [ ] Complexity analysis is added for O(n) algorithms
- [ ] Fixed random seeds are used for reproducibility
- [ ] DoNotOptimize is used to prevent optimization
- [ ] Comments document expected performance characteristics
- [ ] Benchmark name clearly describes what is measured

## Contributing

When adding new benchmarks:

1. Follow the patterns in existing benchmarks
2. Add documentation explaining what is measured
3. Include expected complexity and performance characteristics
4. Test on representative hardware
5. Verify results are reproducible across multiple runs

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Writing Good Benchmarks](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [C++ Benchmarking Best Practices](https://www.youtube.com/watch?v=zWxSZcpeS8Q)
