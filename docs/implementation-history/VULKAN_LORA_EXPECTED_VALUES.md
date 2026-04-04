# Vulkan LoRA Backend - Expected Performance Values

## Overview

This document provides expected performance values and benchmarks for the Vulkan LoRA backend implementation. These values serve as targets for performance validation and regression testing.

## Test Environment Specifications

### Minimum Requirements
- **GPU**: Vulkan 1.2+ compatible GPU with compute shader support
- **VRAM**: 4GB minimum, 8GB+ recommended
- **Driver**: Latest Vulkan drivers
- **CPU**: 4+ cores for baseline comparison

### Reference Hardware Profiles

#### Profile 1: Integrated GPU (Entry-Level)
- **GPU**: Intel Iris Xe / AMD Radeon Vega
- **VRAM**: Shared system memory
- **Expected Speedup**: 10-20x over CPU

#### Profile 2: Discrete GPU (Mid-Range)
- **GPU**: NVIDIA GTX 1660 / AMD RX 5600
- **VRAM**: 6GB dedicated
- **Expected Speedup**: 30-40x over CPU

#### Profile 3: High-End GPU (Target)
- **GPU**: NVIDIA RTX 3070+ / AMD RX 6800+
- **VRAM**: 8GB+ dedicated
- **Expected Speedup**: 45-60x over CPU

## Performance Targets

### Matrix Multiplication

| Dimension | Target Time (Vulkan) | CPU Baseline | Expected Speedup |
|-----------|---------------------|--------------|------------------|
| 256×256   | 0.01 ms            | 1 ms         | 100x            |
| 768×768   | 0.1 ms             | 10 ms        | 100x            |
| 2048×2048 | 1.0 ms             | 100 ms       | 100x            |

**Formula**: Time ≈ (2 × M × N × K) / (GPU_TFLOPS × 10^12)

**Notes**:
- Includes CPU↔GPU transfer overhead
- Tiled 16×16 workgroups with shared memory
- Alpha scaling applied

### Element-wise Operations

| Operation | Size (elements) | Target Time | CPU Baseline | Speedup |
|-----------|----------------|-------------|--------------|---------|
| Add       | 64K            | 0.005 ms    | 0.5 ms       | 100x    |
| Add       | 1M             | 0.02 ms     | 2 ms         | 100x    |
| Add       | 4M             | 0.08 ms     | 8 ms         | 100x    |
| Multiply  | 1M             | 0.02 ms     | 2 ms         | 100x    |
| Scalar Mul| 1M             | 0.02 ms     | 2 ms         | 100x    |

**Notes**:
- 256-thread workgroups
- Memory bandwidth limited
- Minimal compute intensity

### Transpose Operations

| Matrix Size | Target Time | CPU Baseline | Speedup |
|-------------|-------------|--------------|---------|
| 256×256     | 0.01 ms     | 1 ms         | 100x    |
| 768×768     | 0.05 ms     | 5 ms         | 100x    |
| 1024×1024   | 0.1 ms      | 10 ms        | 100x    |
| 2048×2048   | 0.4 ms      | 40 ms        | 100x    |

**Notes**:
- 256-thread workgroups
- Coalesced memory access pattern
- Includes read and write operations

### LoRA Gradient Computations

#### grad_A Computation

| Configuration | Target Time | CPU Baseline | Speedup |
|--------------|-------------|--------------|---------|
| Batch=32, Rank=8, OutDim=256  | 0.05 ms | 2 ms   | 40x |
| Batch=32, Rank=8, OutDim=768  | 0.1 ms  | 5 ms   | 50x |
| Batch=32, Rank=8, OutDim=2048 | 0.3 ms  | 15 ms  | 50x |

#### grad_B Computation

| Configuration | Target Time | CPU Baseline | Speedup |
|--------------|-------------|--------------|---------|
| Batch=32, InDim=256, Rank=8  | 0.05 ms | 2 ms   | 40x |
| Batch=32, InDim=768, Rank=8  | 0.1 ms  | 5 ms   | 50x |
| Batch=32, InDim=2048, Rank=8 | 0.3 ms  | 15 ms  | 50x |

**Notes**:
- 16×16 workgroups
- Batch processing with accumulation
- Scaling factor applied

## End-to-End Training Performance

### Complete Training Step (Batch=32, Dim=768, Rank=8)

| Operation | Target Time | CPU Baseline | Notes |
|-----------|-------------|--------------|-------|
| **Forward Pass** | **1.0 ms** | **50 ms** | |
| - input @ B | 0.4 ms | 20 ms | Batch 32, 768→8 |
| - h @ A | 0.4 ms | 20 ms | Batch 32, 8→768 |
| - Output scaling | 0.2 ms | 10 ms | Element-wise |
| **Backward Pass** | **2.5 ms** | **110 ms** | |
| - grad_A | 0.1 ms | 50 ms | |
| - grad_B | 0.1 ms | 50 ms | |
| - grad_input | 0.3 ms | 10 ms | Optional |
| **Total** | **3.5 ms** | **160 ms** | **45.7x speedup** |

### Training Throughput

| Metric | Target | CPU Baseline | Notes |
|--------|--------|--------------|-------|
| Steps/second | 285 | 6.25 | Batch 32, Dim 768 |
| Samples/second | 9,120 | 200 | 32 samples per step |
| Time per epoch (10K samples) | 1.1 s | 50 s | Assuming continuous training |

## Memory Performance

### Buffer Upload/Download (CPU↔GPU Transfer)

| Data Size | Target Time | Bandwidth | Notes |
|-----------|-------------|-----------|-------|
| 4 KB      | 0.01 ms     | 400 MB/s  | Small overhead |
| 1 MB      | 0.1 ms      | 10 GB/s   | Staging buffer |
| 4 MB      | 0.3 ms      | 13 GB/s   | Typical batch |
| 16 MB     | 1.0 ms      | 16 GB/s   | Large model weights |

**Notes**:
- PCIe 3.0 x16 theoretical: ~16 GB/s
- PCIe 4.0 x16 theoretical: ~32 GB/s
- Includes staging buffer overhead

### Device-Local Memory Operations

| Operation | Target | Notes |
|-----------|--------|-------|
| Buffer-to-buffer copy (1MB) | 0.02 ms | GPU-only, no CPU |
| Buffer allocation (1MB) | 0.05 ms | Device-local |
| Buffer deallocation | < 0.01 ms | RAII cleanup |

## Numerical Accuracy Targets

### Tolerance Levels

| Operation Type | Maximum Error | Notes |
|----------------|---------------|-------|
| Matrix Multiplication | < 1e-3 | Relative to CPU reference |
| Element-wise Ops | < 1e-5 | Simple operations |
| Gradients | < 1e-3 | Accumulated errors |
| Full Training Step | < 1e-2 | End-to-end validation |

### Validation Method

```cpp
bool validate_result(const std::vector<float>& vulkan_result,
                     const std::vector<float>& cpu_reference,
                     float tolerance = 1e-3f) {
    for (size_t i = 0; i < vulkan_result.size(); i++) {
        float diff = std::abs(vulkan_result[i] - cpu_reference[i]);
        float relative_error = diff / (std::abs(cpu_reference[i]) + 1e-10f);
        if (relative_error > tolerance) {
            return false;
        }
    }
    return true;
}
```

## Test Data Generation

### Random Matrix Generation

```cpp
std::vector<float> generate_test_matrix(int rows, int cols, int seed = 42) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    std::vector<float> matrix(rows * cols);
    for (auto& val : matrix) {
        val = dist(gen);
    }
    return matrix;
}
```

**Properties**:
- Values in range [-1.0, 1.0]
- Uniform distribution
- Reproducible with fixed seed
- No special patterns (edge cases tested separately)

### Edge Case Test Data

```cpp
// All zeros
std::vector<float> zeros(size, 0.0f);

// All ones
std::vector<float> ones(size, 1.0f);

// Identity matrix
auto identity = generate_identity_matrix(dim);

// Very small values
std::vector<float> small_values(size, 1e-6f);

// Very large values
std::vector<float> large_values(size, 1e6f);
```

## Performance Regression Thresholds

### Acceptable Variance

| Metric | Acceptable Range | Action if Exceeded |
|--------|-----------------|-------------------|
| Execution time | ±10% | Warning |
| Execution time | ±25% | Investigation required |
| Execution time | > +50% | Regression detected |
| Numerical accuracy | < 1e-3 | Pass |
| Numerical accuracy | 1e-3 to 1e-2 | Warning |
| Numerical accuracy | > 1e-2 | Failure |

### Performance Baseline Recording

```bash
# Run benchmarks and save baseline
./bench_vulkan_lora --benchmark_out=baseline.json --benchmark_out_format=json

# Compare against baseline
./bench_vulkan_lora --benchmark_out=current.json --benchmark_out_format=json
python3 compare_benchmarks.py baseline.json current.json
```

## Platform-Specific Expectations

### Windows

- **Driver**: NVIDIA/AMD latest drivers
- **Expected Variance**: ±5% vs Linux
- **Known Issues**: Validation layer overhead higher

### Linux

- **Driver**: Mesa 22.0+ or proprietary
- **Expected Variance**: Baseline platform
- **Known Issues**: None

### macOS (MoltenVK)

- **Driver**: MoltenVK 1.2+
- **Expected Variance**: +10-20% overhead due to Metal translation
- **Known Issues**: Some validation layers unavailable

### Android

- **Driver**: Vulkan 1.2+ (API level 29+)
- **Expected Variance**: Highly device-dependent
- **Known Issues**: Power management may affect performance

## Continuous Integration Expectations

### CI Environment

- **Hardware**: GitHub Actions runners (no GPU)
- **Expected Behavior**: Tests skip with "Vulkan not available"
- **Validation**: Compile-time only

### Local Testing

- **Hardware**: Developer machines with GPU
- **Expected Behavior**: All tests pass
- **Validation**: Full performance + accuracy testing

## Troubleshooting Performance Issues

### Symptom: Slower than CPU

**Possible Causes**:
1. No GPU available (falling back to CPU)
2. Validation layers enabled in release build
3. Excessive CPU↔GPU transfers
4. Small workload (overhead dominates)

**Solution**:
```bash
# Check Vulkan availability
vulkaninfo

# Disable validation layers
export VK_INSTANCE_LAYERS=""

# Profile with validation layers off
./bench_vulkan_lora
```

### Symptom: High variance

**Possible Causes**:
1. GPU thermal throttling
2. Background GPU workload
3. Power management
4. Driver issue

**Solution**:
- Run multiple iterations
- Warmup before timing
- Check GPU temperature
- Update drivers

### Symptom: Memory errors

**Possible Causes**:
1. Out of VRAM
2. Memory leak
3. Buffer overflow
4. Validation layer catching real bugs

**Solution**:
```bash
# Run with validation layers
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./test_vulkan_lora

# Check VRAM usage
nvidia-smi  # For NVIDIA GPUs
```

## References

- Vulkan Spec: https://registry.khronos.org/vulkan/specs/1.3/html/
- Google Benchmark: https://github.com/google/benchmark
- Performance Analysis: VULKAN_IMPLEMENTATION_SUMMARY.md
- Test Code: tests/test_vulkan_lora.cpp
- Benchmark Code: benchmarks/bench_vulkan_lora.cpp

## Version History

- **v1.0** (2026-01-16): Initial expected values document
  - Matrix multiplication targets
  - Element-wise operation targets
  - LoRA gradient targets
  - End-to-end training step targets
  - Accuracy thresholds
  - Platform-specific expectations

---

**Note**: All values in this document are **targets** based on design specifications and theoretical performance. Actual performance will vary based on hardware, drivers, and workload characteristics. Values should be validated against real hardware and updated based on empirical results.
