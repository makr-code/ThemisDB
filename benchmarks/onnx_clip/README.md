# ONNX CLIP v0.3.0 Benchmark Framework

## Phase 2A: Benchmark Infrastructure & Fixtures

This directory contains the benchmark infrastructure for ONNX CLIP v0.3.0 hardening.
Provides CPU and multi-backend latency measurements with standardized gate thresholds.

### Directory Structure

```
benchmarks/onnx_clip/
├── CMakeLists.txt                    # Benchmark target registration
├── README.md                          # This file
├── bench_onnx_clip_cpu.cpp            # CPU latency benchmarks
└── bench_onnx_clip_vit_backend.cpp    # Backend throughput/memory benchmarks
```

---

## Release Gates (Phase 2A)

All benchmarks enforce hard thresholds for Phase 2A promotion.
**Regression beyond 10% vs baseline blocks release.**

| Gate ID | Benchmark | Metric | Threshold | Description |
|---------|-----------|--------|-----------|-------------|
| **FCP-01** | `BM_SingleImageLatency_ViTB32_CPU` | p99 latency | ≤ 150 ms | Single image encoding (CPU) |
| **FCP-02** | `BM_BatchLatency_ViTB32_CPU_Batch16` | p99 latency | ≤ 2.4 sec | Batch of 16 images (CPU) |
| **FCP-03** | `BM_TextEmbedding_Latency_CPU` | p99 latency | ≤ 5 ms | Text encoding (CPU) |
| **FCP-04** | `BM_Initialization_Latency_CPU` | p99 latency | < 500 ms | Model initialization (CPU) |
| **FCP-05** | `BM_Throughput_ViTB32_CUDA_Batch64` | throughput | ≥ 6x single | Batch-64 vs single-image speedup (CUDA) |
| **FCP-06** | `BM_MemoryFootprint_ModelLoad` | memory | tracked | Model load memory footprint |

---

## Benchmark Files

### `bench_onnx_clip_cpu.cpp` (~200 lines)

CPU-only latency measurements for single-image and batch operations.

**Benchmarks:**

- `BM_SingleImageLatency_ViTB32_CPU` (target: ≤ 150 ms)
  - Measures wall-clock latency for encoding a single 224×224 image
  - Batch size: 1
  - Warmup: 10 iterations
  
- `BM_BatchLatency_ViTB32_CPU_Batch8` (target: ≤ 1.2 sec)
  - Measures wall-clock latency for batch of 8 images
  - Includes data loading, preprocessing, and model inference
  
- `BM_BatchLatency_ViTB32_CPU_Batch16` (target: ≤ 2.4 sec)
  - Measures wall-clock latency for batch of 16 images
  - Gate FCP-02: Hard threshold for CPU batch processing
  
- `BM_TextEmbedding_Latency_CPU` (target: ≤ 5 ms)
  - Measures latency for text-to-embedding pipeline
  - Single text prompt encoding
  - Gate FCP-03: Text encoding must be fast (< 5 ms)
  
- `BM_Initialization_Latency_CPU`
  - Measures model load time (weights + graph construction)
  - No cached inference; cold start
  - Gate FCP-04: Must initialize < 500 ms
  
- `BM_HealthCheck_Latency_CPU`
  - Measures overhead of model health-check operation
  - Smoke test for model availability

**Measurement Hygiene:**
- Canonical seed: `kCanonicalRngSeed = 42`
- Timing: `UseRealTime()` to capture wall-clock latency
- Warmup: 3-phase protocol (cold, warm, hot)
- RNG: Fixed seed for reproducible data sequences

---

### `bench_onnx_clip_vit_backend.cpp` (~250 lines)

Multi-backend throughput and memory benchmarks.

**Benchmarks:**

- `BM_Throughput_ViTB32_CUDA_Batch64`
  - Measures ops/sec for batch-64 CUDA inference
  - Target: ≥ 6x single-image throughput
  - Gate FCP-05: Batch parallelization efficiency
  - Falls back to CPU if CUDA unavailable
  
- `BM_Throughput_ViTB32_CPU_Batch16`
  - CPU throughput for batch-16 processing
  - Baseline for backend comparison
  
- `BM_MemoryFootprint_ModelLoad`
  - Tracks peak RSS during model loading
  - Captures model serialization + graph construction
  - Gate FCP-06: Memory footprint tracking
  
- `BM_MemoryFootprint_Runtime_State`
  - Tracks memory usage during batch inference
  - Includes input buffers, activation maps
  
- `BM_BatchSplitting_Performance`
  - Measures overhead of splitting large batches
  - Tests different split strategies (4, 8, 16 way splits)
  - Validates batch-splitting optimization

**Measurement Hygiene:**
- Uses steady_clock for stable timing
- Memory measurements via `/proc/self/status` (RSS)
- Canonical seed for synthetic image data
- Real-time mode for I/O and memory operations

---

## Building & Running

### Build All Benchmarks

```bash
# Configure with benchmarks enabled
cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON

# Build all ONNX CLIP benchmarks
cmake --build --preset linux-release --target \
    bench_onnx_clip_cpu \
    bench_onnx_clip_vit_backend
```

### Run CPU Benchmarks

```bash
# Run with JSON output for regression tracking
./build/linux-release/benchmarks/onnx_clip/bench_onnx_clip_cpu \
    --benchmark_out=results/bench_onnx_clip_cpu.json \
    --benchmark_out_format=json

# Run single benchmark
./build/linux-release/benchmarks/onnx_clip/bench_onnx_clip_cpu \
    --benchmark_filter=BM_SingleImageLatency_ViTB32_CPU
```

### Run Backend Benchmarks

```bash
# Run with JSON output
./build/linux-release/benchmarks/onnx_clip/bench_onnx_clip_vit_backend \
    --benchmark_out=results/bench_onnx_clip_vit_backend.json \
    --benchmark_out_format=json

# Run CUDA throughput benchmark
./build/linux-release/benchmarks/onnx_clip/bench_onnx_clip_vit_backend \
    --benchmark_filter=BM_Throughput_ViTB32_CUDA_Batch64
```

### Interpret Results

Gate regression detection:
```python
# Check if FCP-01 (single-image latency) regressed > 10%
baseline_p99 = 150  # ms
measured_p99 = 165  # ms
regression = (measured_p99 - baseline_p99) / baseline_p99
print(f"Regression: {regression:.1%}")  # 10.0% — at threshold
```

---

## Gate Acceptance Criteria

### Hard Gates (Blocking)

- **FCP-01**: Single-image latency p99 ≤ 150 ms
  - Blocks: GPU stream not available; model too large
  - Action: Profile inference path; reduce model size if needed

- **FCP-02**: Batch-16 latency p99 ≤ 2.4 sec
  - Blocks: Batch processing performance regression
  - Action: Check for increased per-image overhead; optimize loop

- **FCP-03**: Text encoding p99 ≤ 5 ms
  - Blocks: Text embedding too slow
  - Action: Verify tokenization path; check vocab lookups

- **FCP-04**: Initialization < 500 ms
  - Blocks: Startup time regression
  - Action: Profile model deserialization; check I/O

- **FCP-05**: Batch-64 throughput ≥ 6x single
  - Blocks: Insufficient parallelization
  - Action: Check CUDA kernel utilization; verify batch fusion

- **FCP-06**: Memory footprint tracked
  - Blocks: Memory regression > 10% vs baseline
  - Action: Profile allocations; check for leaks

---

## Measurement Protocols

### 3-Phase Warmup (All Benchmarks)

1. **Cold** (10 iterations)
   - Initial model load
   - Kernel JIT compilation
   
2. **Warm** (10 iterations)
   - Sequential inference to warm caches
   - Malloc arena initialization
   
3. **Hot** (10 iterations)
   - Random batch ordering
   - CPU cache + branch predictor stabilization

Measurement window starts **after** phase 3.

### Wall-Clock Timing (Latency Benchmarks)

```cpp
BENCHMARK_REGISTER_F(OnnxClipCpuFixture, BM_SingleImageLatency_ViTB32_CPU)
    ->UseRealTime()  // Capture I/O + scheduling wait
    ->Unit(benchmark::kMillisecond);
```

### Throughput Measurement (Backend Benchmarks)

```cpp
state.SetItemsProcessed(batch_size);  // ops/sec = items / elapsed_time
```

---

## Reference Material

- **Measurement Hygiene**: `benchmarks/MEASUREMENT_HYGIENE.md`
- **Benchmark Fixtures**: `benchmarks/bench_fixtures.h`
- **Phase 2A Spec**: `PHASE_2A_HARDENING_SPEC.md`
- **ONNX CLIP Module**: `src/onnx_clip/`

---

## Implementation Status

| File | Status | Lines |
|------|--------|-------|
| `bench_onnx_clip_cpu.cpp` | ✅ Implemented | ~200 |
| `bench_onnx_clip_vit_backend.cpp` | ✅ Implemented | ~250 |
| `CMakeLists.txt` | ✅ Implemented | ~40 |
| Gate Documentation | ✅ Complete | This file |

---

**Last Updated:** 2026-08-09  
**Author:** ThemisDB CI/CD  
**Version:** 1.0.0 (Phase 2A)
