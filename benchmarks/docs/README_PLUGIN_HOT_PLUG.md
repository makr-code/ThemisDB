> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# Plugin Hot-Plug Monitoring Benchmarks

This directory contains performance benchmarks for the Plugin Hot-Plug Monitoring system.

## Overview

The benchmark suite (`bench_plugin_hot_plug.cpp`) measures:

- **Lifecycle overhead**: Enable/disable monitoring costs
- **Detection latency**: Time to detect filesystem events
- **Monitoring overhead**: CPU/memory impact during idle monitoring
- **Thread safety**: Performance under concurrent operations
- **Configuration impact**: Performance differences between config options

## Building

```bash
cmake -B build -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build --target bench_plugin_hot_plug
```

## Running Benchmarks

### Run All Benchmarks
```bash
./build/benchmarks/bench_plugin_hot_plug
```

### Run Specific Benchmark
```bash
./build/benchmarks/bench_plugin_hot_plug --benchmark_filter="EnableDisableMonitoring"
```

### Generate JSON Report
```bash
./build/benchmarks/bench_plugin_hot_plug \
    --benchmark_format=json \
    --benchmark_out=hot_plug_results.json
```

### Compare Results
```bash
# Baseline
./build/benchmarks/bench_plugin_hot_plug \
    --benchmark_format=json \
    --benchmark_out=baseline.json

# After changes
./build/benchmarks/bench_plugin_hot_plug \
    --benchmark_format=json \
    --benchmark_out=optimized.json

# Compare (requires compare.py from Google Benchmark)
python3 compare.py benchmarks baseline.json optimized.json
```

## Benchmark Descriptions

### EnableDisableMonitoring
Measures the overhead of starting and stopping filesystem monitoring.

**Expected**: < 10ms per enable/disable cycle

**What it tests**: 
- Thread creation/destruction overhead
- Platform-specific monitor initialization
- Resource allocation/cleanup

### MonitoringOverhead
Measures steady-state CPU usage while monitoring is active but idle.

**Expected**: < 0.1% CPU usage

**What it tests**:
- Background thread efficiency
- Platform API polling behavior
- Event queue management

### FileCreationDetectionLatency
Measures end-to-end latency from file creation to detection notification.

**Expected**: 500-700ms (includes 500ms debounce)

**What it tests**:
- Platform filesystem notification latency
- Event processing pipeline
- Debounce logic

### MultipleFileCreations
Tests batch file creation performance.

**Runs with**: 1, 5, 10, 50 files

**What it tests**:
- Event batching efficiency
- Queue saturation behavior
- Scaling characteristics

### AutoLoadDisabledVsEnabled
Compares performance with auto-load on vs off.

**Expected**: ~100ms difference (plugin loading overhead)

**What it tests**:
- Configuration impact
- Auto-load performance
- Plugin loading overhead

### ConcurrentFileCreations
Tests thread safety under concurrent file operations.

**Runs with**: 4 concurrent threads

**What it tests**:
- Lock contention
- Thread safety correctness
- Concurrent event handling

### MonitorMemoryFootprint
Measures memory usage scaling.

**Runs with**: 1, 10, 100 watch directories

**What it tests**:
- Memory usage per monitor
- Resource scaling
- Memory leak detection

### RapidEnableDisable
Stress tests enable/disable cycling.

**What it tests**:
- Race condition resilience
- Resource leak detection
- Cleanup correctness

### MixedOperations
Tests realistic mixed workload.

**What it tests**:
- Real-world performance
- Combined operation overhead
- Overall system behavior

## Performance Targets

| Metric | Target | Acceptable | Notes |
|--------|--------|------------|-------|
| Enable/Disable | < 5ms | < 10ms | One-time cost |
| Detection Latency | 500-550ms | < 700ms | Includes debounce |
| Monitoring Overhead | < 0.05% | < 0.1% | Idle CPU usage |
| Thread Safety | No contention | < 10ms lock wait | Under 4 threads |
| Memory per Monitor | < 500KB | < 1MB | Per instance |

## Interpreting Results

### Time Units
- **ns**: Nanoseconds (1e-9 seconds)
- **us**: Microseconds (1e-6 seconds)
- **ms**: Milliseconds (1e-3 seconds)

### Metrics
- **Time**: Mean execution time per iteration
- **CPU**: CPU time (excludes I/O wait)
- **Iterations**: Number of benchmark runs
- **Items/s**: Throughput metric

### Example Output
```
-----------------------------------------------------------------------
Benchmark                                 Time      CPU   Iterations
-----------------------------------------------------------------------
EnableDisableMonitoring                 5.2 ms   4.8 ms          142
MonitoringOverhead                      102 ns    98 ns   6984536
FileCreationDetectionLatency            524 ms   1.2 ms            1
MultipleFileCreations/1                 527 ms   1.3 ms            1
MultipleFileCreations/5                 701 ms   6.1 ms            1
AutoLoadDisabledVsEnabled/0             525 ms   1.2 ms            1
AutoLoadDisabledVsEnabled/1             628 ms   124 ms            1
ConcurrentFileCreations                 543 ms   18.2 ms           1
```

## Regression Detection

Run benchmarks before and after changes to detect performance regressions:

```bash
# Before changes
git checkout main
cmake -B build -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build --target bench_plugin_hot_plug
./build/benchmarks/bench_plugin_hot_plug --benchmark_format=json --benchmark_out=before.json

# After changes
git checkout feature-branch
cmake --build build --target bench_plugin_hot_plug
./build/benchmarks/bench_plugin_hot_plug --benchmark_format=json --benchmark_out=after.json

# Compare
python3 compare.py benchmarks before.json after.json
```

## Platform-Specific Notes

### Linux
- Uses inotify: Very efficient, minimal overhead
- Non-blocking poll allows clean shutdown
- Best performance of all platforms

### Windows
- Uses ReadDirectoryChangesW: Good performance
- Recursive subdirectory monitoring included
- Wide-char conversion adds minimal overhead

### macOS
- Uses kqueue: Requires directory scanning
- Slightly higher overhead due to polling approach
- Still acceptable for typical use cases

## Troubleshooting

### Benchmark Hangs
- Check file descriptor limits: `ulimit -n`
- Ensure /tmp has write permissions
- Verify no zombie threads from previous runs

### High Variance
- Close other applications during benchmarking
- Run multiple iterations: `--benchmark_repetitions=10`
- Use CPU governor: `cpupower frequency-set -g performance`

### Out of Memory
- Reduce batch sizes in benchmark code
- Check for leaks: `valgrind ./bench_plugin_hot_plug`
- Ensure cleanup runs between iterations

## Contributing

When adding new benchmarks:

1. Use the `HotPlugBenchmarkFixture` base class
2. Include proper setup/teardown
3. Use `state.PauseTiming()` for setup work
4. Document expected performance
5. Add entry to this README

## References

- Google Benchmark Documentation: https://github.com/google/benchmark
- Hot-Plug Monitoring Documentation: `docs/plugins/HOT_PLUG_MONITORING.md`
- Test Suite: `tests/test_plugin_hot_plug.cpp`
