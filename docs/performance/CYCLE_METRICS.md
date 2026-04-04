# ThemisDB Cycle Metrics: Comprehensive Performance Measurement

## Table of Contents

1. [Overview](#overview)
2. [Why Clock Cycles?](#why-clock-cycles)
3. [Architecture](#architecture)
4. [CMake Configuration](#cmake-configuration)
5. [Build Presets](#build-presets)
6. [Runtime Configuration](#runtime-configuration)
7. [Expected Cycle Counts](#expected-cycle-counts)
8. [Grafana Integration](#grafana-integration)
9. [CHIMERA Integration](#chimera-integration)
10. [Troubleshooting](#troubleshooting)

## Overview

ThemisDB implements a comprehensive, cycle-based performance measurement system that:

- ✅ Measures CPU/GPU/PCIe/Memory operations in **clock cycles** instead of time
- ✅ Provides **zero-cost abstraction** when disabled (0% overhead)
- ✅ Integrates with **Grafana** (via Prometheus) and **CHIMERA**
- ✅ Demonstrates the efficiency of **heap pointer passing** from HNSW search to LLM inference
- ✅ Is **fully configurable** via CMake feature flags

## Why Clock Cycles?

### Time vs. Cycles Comparison

| Metric | Time-based (ms) | Cycle-based (Cycles) |
|--------|----------------|---------------------|
| **System comparability** | ❌ 3.2 GHz vs 4.8 GHz = not comparable | ✅ Normalized to CPU frequency |
| **Turbo Boost effects** | ❌ Highly variable | ✅ Consistent under same load |
| **CPU/GPU correlation** | ❌ Hard to correlate | ✅ Both measurable in cycles |
| **Precision** | ~1µs | ~1 cycle (sub-nanosecond @ 4GHz) |
| **PCIe/Memory bus** | ❌ Not directly measurable | ✅ Bus cycles countable |

### Key Benefits

1. **System-independent comparison**: Cycle counts are normalized to CPU frequency, making measurements comparable across different systems.
2. **Sub-nanosecond precision**: At 4 GHz, one cycle = 0.25 nanoseconds.
3. **Hardware correlation**: Can directly measure CPU, GPU, memory, and PCIe bus cycles.
4. **Turbo Boost stability**: Less affected by dynamic frequency scaling.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ThemisDB Core Process                    │
├─────────────────────────────────────────────────────────────┤
│  HNSW Search → [Heap Pointer] → LLM Inference              │
│      │                │              │                      │
│      └── Cycle Counter ──────────────┘                      │
│          (RDTSC/RDTSCP/GPU Events)                          │
└──────────────────┬──────────────────────────────────────────┘
                   │ Lock-Free Ring Buffer (per thread)
                   ↓
┌─────────────────────────────────────────────────────────────┐
│         Async Metrics Aggregator (Background Thread)        │
├─────────────────────────────────────────────────────────────┤
│  • Drains all thread-local buffers                         │
│  • Aggregates metrics                                       │
│  • Calculates efficiency ratios                            │
└──────────────────┬──────────────────────────────────────────┘
                   │ Prometheus/CHIMERA Export
                   ↓
┌─────────────────────────────────────────────────────────────┐
│              Grafana Dashboard / CHIMERA                    │
└─────────────────────────────────────────────────────────────┘
```

### Components

1. **HardwareCycleCounter**: Platform-specific cycle counter (RDTSC for x86_64, CNTVCT_EL0 for ARM64)
2. **Lock-Free Ring Buffer**: Thread-local SPSC buffer for zero-contention metrics collection
3. **Async Metrics Exporter**: Background thread that drains buffers and exports metrics
4. **Prometheus/CHIMERA Exporters**: Format metrics for external monitoring systems

## CMake Configuration

### Feature Flags

Add to your `CMakeLists.txt` or pass via command line:

```cmake
option(THEMIS_ENABLE_CYCLE_METRICS 
       "Enable CPU cycle measurement (low overhead: ~0.05%)" 
       OFF)

option(THEMIS_ENABLE_DETAILED_METRICS 
       "Enable detailed per-function metrics (overhead: ~5-10%)" 
       OFF)

option(THEMIS_ENABLE_GPU_CYCLE_METRICS 
       "Enable GPU cycle measurement via CUDA events (overhead: ~10-20%)" 
       OFF)

option(THEMIS_ENABLE_PMU_COUNTERS 
       "Enable hardware performance counters via perf (overhead: ~10-50%)" 
       OFF)

option(THEMIS_ENABLE_METRICS_EXPORT 
       "Enable Prometheus/CHIMERA export (adds async overhead)" 
       OFF)

option(THEMIS_BENCHMARK_MODE 
       "Enable all performance measurement features (FOR BENCHMARKING ONLY!)" 
       OFF)
```

### Building with Flags

```bash
# Production build (zero overhead)
cmake -B build -DTHEMIS_ENABLE_CYCLE_METRICS=OFF

# Monitored build (low overhead)
cmake -B build -DTHEMIS_ENABLE_CYCLE_METRICS=ON -DTHEMIS_ENABLE_METRICS_EXPORT=ON

# Benchmark build (all features)
cmake -B build -DTHEMIS_BENCHMARK_MODE=ON
```

## Build Presets

### Production Preset

**File**: `cmake/presets/ProductionPreset.cmake`

```cmake
set(THEMIS_ENABLE_CYCLE_METRICS OFF)
set(THEMIS_ENABLE_DETAILED_METRICS OFF)
set(THEMIS_ENABLE_GPU_CYCLE_METRICS OFF)
set(THEMIS_ENABLE_PMU_COUNTERS OFF)
set(THEMIS_ENABLE_METRICS_EXPORT OFF)
```

**Overhead**: 0% (zero cost)

**Use case**: Production deployment ✅

### Monitored Preset

**File**: `cmake/presets/MonitoredPreset.cmake`

```cmake
set(THEMIS_ENABLE_CYCLE_METRICS ON)
set(THEMIS_ENABLE_METRICS_EXPORT ON)
```

**Overhead**: <0.1% (with sampling rate 100-1000)

**Use case**: Grafana monitoring ✅

**Important**: Set sampling rate at runtime!

```cpp
RuntimeConfig::instance().setSamplingRate(100);  // Measure 1% of operations
```

### Benchmark Preset

**File**: `cmake/presets/BenchmarkPreset.cmake`

```cmake
set(THEMIS_BENCHMARK_MODE ON)  # Enables all features
```

**Overhead**: 5-50%

**Use case**: Performance testing only ⚠️

## Runtime Configuration

### Sampling Rate

Control measurement frequency:

```cpp
#include "performance/runtime_config.h"

auto& config = RuntimeConfig::instance();

// Measure all operations (100%)
config.setSamplingRate(1);

// Measure 1% of operations
config.setSamplingRate(100);

// Measure 0.1% of operations
config.setSamplingRate(1000);

// Disable measurement
config.setSamplingRate(0);
```

### Operation Filtering

Enable/disable specific operations:

```cpp
// Enable only specific operations
config.enableOperation("rag_pipeline");
config.enableOperation("hnsw_search");

// Disable specific operation
config.disableOperation("llm_inference");

// Clear all filters (enable all)
config.clearOperationFilters();
```

## Expected Cycle Counts

### Memory Operations

| Operation | Expected Cycles | Description |
|-----------|----------------|-------------|
| L1 cache hit | 4 | 1-2ns @ 3GHz |
| L2 cache hit | 12 | 3-4ns @ 3GHz |
| L3 cache hit | 50 | 12-15ns @ 3GHz |
| RAM access | 250 | 60-80ns @ 3GHz |

### Pointer Operations

| Operation | Expected Cycles | Notes |
|-----------|----------------|-------|
| Pointer passing | 150 | Registry + stack |
| Memory copy (1KB) | 800 | ~0.8 cycles/byte with SIMD |

**Speedup**: Pointer passing is **~53x faster** than copying 10KB!

### HNSW Search (CPU, 768-dim)

| Dataset Size | Expected Cycles | Time @ 3GHz |
|--------------|----------------|-------------|
| 1K vectors | 1,200,000 | ~0.4 ms |
| 10K vectors | 4,500,000 | ~1.5 ms |
| 100K vectors | 12,000,000 | ~4 ms |
| 1M vectors | 35,000,000 | ~11.7 ms |

### LLM Inference (CPU)

| Model Size | Cycles per Token | Time @ 3GHz |
|-----------|-----------------|-------------|
| 7B params | 80,000,000 | ~26.7 ms |
| 13B params | 145,000,000 | ~48.3 ms |

### RAG Pipeline Breakdown

**Scenario**: 10K vector search + 10 token generation (7B model)

| Component | Cycles | Percentage |
|-----------|--------|------------|
| HNSW search | 4,500,000 | 0.56% |
| **Pointer passing** | **150** | **0.000019%** ⭐ |
| LLM inference | 800,000,000 | 99.44% |
| **Total** | **804,500,150** | **100%** |

**Key Insight**: Pointer passing overhead is **negligible** (0.000019%)!

### Tolerance Levels

```cpp
// Normal variance: ±15%
ExpectedCycles::Tolerances::NORMAL_VARIANCE = 0.15;

// Warning threshold: ±30%
ExpectedCycles::Tolerances::WARNING_THRESHOLD = 0.30;

// Critical threshold: ±50%
ExpectedCycles::Tolerances::CRITICAL_THRESHOLD = 0.50;
```

## Grafana Integration

### Setup

1. **Configure Prometheus** to scrape ThemisDB metrics endpoint:

```yaml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:9090']
```

2. **Import Dashboard**: Load `grafana/cycle_metrics_dashboard.json`

3. **Configure Data Source**: Point to your Prometheus instance

### Metrics Available

- `themis_hnsw_search_cycles{operation="..."}`
- `themis_pointer_passing_cycles{operation="..."}`
- `themis_llm_inference_cycles{operation="..."}`
- `themis_cache_miss_cycles{operation="..."}`
- `themis_pcie_transfer_cycles{operation="...",direction="h2d|d2h"}`
- `themis_cpu_efficiency_ratio{operation="..."}`
- `themis_total_operation_cycles{operation="..."}`

### Dashboard Panels

1. **Pipeline Breakdown**: Stacked bar chart showing HNSW/Pointer/LLM cycles
2. **Pointer Passing Efficiency**: Stat panel (green if <500 cycles)
3. **Memory Hierarchy**: Pie chart of L1/L2/L3/RAM cycles
4. **PCIe Efficiency**: Transfer cycles per MB
5. **Deviation Heatmap**: % deviation from expected values
6. **System Comparison**: Normalized cycles across systems

## CHIMERA Integration

### JSON Format

CHIMERA export provides detailed JSON metrics:

```json
{
  "timestamp": 1706270400,
  "timestamp_iso": "2026-01-26T09:00:00Z",
  "system": {
    "cpu_model": "Intel(R) Xeon(R) Gold 6248R",
    "cpu_frequency_hz": 3000000000,
    "architecture": "x86_64"
  },
  "operations": [
    {
      "name": "rag_pipeline",
      "count": 1000,
      "cycles": {
        "hnsw_search": 4500000,
        "pointer_passing": 150,
        "llm_inference": 800000000,
        "total": 804500150
      },
      "expected": {
        "pointer_passing": 150
      },
      "deviation_percent": {
        "pointer_passing": 0.0
      },
      "breakdown_percent": {
        "hnsw_search": 0.56,
        "pointer_passing": 0.000019,
        "llm_inference": 99.44
      }
    }
  ]
}
```

### Accessing Metrics

```cpp
#include "performance/async_metrics_exporter.h"

auto& exporter = AsyncMetricsExporter::instance();

// Get Prometheus format
std::string prometheus = exporter.getPrometheusMetrics();

// Get CHIMERA JSON
std::string chimera = exporter.getCHIMERAMetrics();
```

## Troubleshooting

### Issue: Cycle counts are zero

**Cause**: Architecture not supported or metrics disabled

**Solution**:
1. Verify architecture: `uname -m` should show `x86_64` or `aarch64`
2. Check CMake flags: `THEMIS_ENABLE_CYCLE_METRICS=ON`
3. Rebuild project

### Issue: High variance in measurements

**Cause**: Turbo Boost, power management, or background processes

**Solution**:
1. Disable Turbo Boost:
   ```bash
   echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
   ```
2. Set CPU governor to performance:
   ```bash
   sudo cpupower frequency-set -g performance
   ```
3. Run on isolated cores

### Issue: Metrics buffer overflow

**Cause**: Too many operations, buffer too small

**Solution**:
1. Increase sampling rate: `setSamplingRate(1000)`
2. Check dropped metrics: `exporter.getDroppedMetrics()`
3. Increase buffer capacity in `lockfree_metrics_buffer.h`

### Issue: Zero-cost abstraction not working

**Cause**: Macros not being optimized out

**Solution**:
1. Verify all flags are OFF in production
2. Enable optimizations: `-O2` or `-O3`
3. Check assembly output: `objdump -d`

## Performance Impact

| Configuration | Overhead | Use Case |
|--------------|----------|----------|
| **Production (all OFF)** | 0% | Live system ✅ |
| **Monitored (sampling 1:100)** | <0.1% | Grafana monitoring ✅ |
| **Benchmark Mode** | 5-10% | Performance tests ✅ |
| **Profiling (with PMU)** | 10-50% | Development only ⚠️ |

## API Reference

### Recording Metrics

```cpp
#include "performance/cycle_metrics.h"
#include "performance/cycle_metrics_config.h"

// Method 1: Manual timing
uint64_t cycles;
THEMIS_MEASURE_CYCLES_START(cycles);
// ... code to measure ...
THEMIS_MEASURE_CYCLES_END(cycles);

// Method 2: RAII timer
{
    THEMIS_SCOPED_CYCLE_TIMER(cycles);
    // ... code to measure ...
}

// Method 3: Record operation
OperationCycleMetrics metrics;
metrics.hnsw_search_cycles = 1000000;
metrics.pointer_passing_cycles = 150;
THEMIS_RECORD_METRICS("my_operation", metrics);
```

### Starting Async Export

```cpp
#include "performance/async_metrics_exporter.h"

// Start background export thread (runs every 1 second)
AsyncMetricsExporter::instance().start(1);

// Stop export thread
AsyncMetricsExporter::instance().stop();
```

## Validation

### Running Benchmarks

```bash
cd build
./benchmarks/bench_cycle_metrics
```

Expected output:
```
ThemisDB Cycle Metrics Benchmark
=================================

System Information:
  CPU: Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz
  Frequency: 3600 MHz
  Cycle metrics: ENABLED

=== Pointer Passing vs Memory Copy ===
  Pointer passing: 147 cycles
  Memory copy (10KB): 7842 cycles
  Speedup: 53.3x
  Deviation from expected:
    Pointer: -2.0%
    Copy: -1.9%

=== Cache Hierarchy ===
  L1 access: ~4 cycles (expected: 4)
  L2 access: ~13 cycles (expected: 12)
  L3 access: ~52 cycles (expected: 50)
  RAM access: ~245 cycles (expected: 250)

=== RAG Pipeline Breakdown ===
  HNSW search: 4500000 cycles
  Pointer passing: 148 cycles
  LLM inference: 800000000 cycles
  Total: 804500148 cycles

  Breakdown:
    HNSW: 0.56%
    Pointer: 0.000018% (NEGLIGIBLE!)
    LLM: 99.44%

  Expected values:
    Pointer overhead: 0.000019%

=== Benchmark Complete ===
```

### Running Tests

```bash
cd build
./tests/performance/test_cycle_metrics
```

All tests should pass.

## Conclusion

The ThemisDB cycle metrics system provides:

✅ **Zero-cost abstraction** in production (0% overhead)
✅ **Sub-nanosecond precision** for accurate measurements
✅ **System-independent comparison** via cycle normalization
✅ **Comprehensive monitoring** via Grafana and CHIMERA
✅ **Demonstrates efficiency** of pointer passing in RAG pipeline

For questions or issues, please refer to the ThemisDB documentation or file an issue on GitHub.
