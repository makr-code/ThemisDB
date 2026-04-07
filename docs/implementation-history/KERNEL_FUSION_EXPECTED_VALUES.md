# Kernel Fusion Expected Values and Performance Targets

## Overview

This document defines the expected performance values and validation criteria for the kernel fusion optimization implementation (Phase 10.3). These values are used for benchmarking, testing, and performance validation.

## Test Accuracy Tolerance

### Numerical Accuracy (gtest validation)
- **Tolerance**: `EPSILON = 1e-5f` (0.00001)
- **Applies to**: All fused vs unfused comparisons
- **Test files**: `tests/test_fused_kernels.cpp`

**Validation criteria**:
```cpp
// Forward pass output
assert(|fused_output[i] - unfused_output[i]| < 1e-5 for all i)

// Backward pass gradients
assert(|grad_A_fused[i] - grad_A_unfused[i]| < 1e-5 for all i)
assert(|grad_B_fused[i] - grad_B_unfused[i]| < 1e-5 for all i)
assert(|grad_input_fused[i] - grad_input_unfused[i]| < 1e-5 for all i)

// Optimizer updates
assert(|param_fused[i] - param_unfused[i]| < 1e-5 for all i)
```

## Performance Targets (from Phase 10.3 specification)

### 1. Forward Pass

**Current (Unfused)**:
- Kernel launches: 3
  1. `h = input @ B` (write h to global memory)
  2. `output = h @ A` (read h, write output)
  3. `output *= scaling` (read/write output)
- Memory traffic: 5 global memory accesses
- Baseline time: **1.0ms** (at 768x768, rank=8, batch=16)

**Target (Fused)**:
- Kernel launches: 1
  - `output = (input @ B) @ A * scaling` (h in shared memory)
- Memory traffic: 2 global memory accesses (input read, output write)
- **Memory reduction**: 66% (5 → 2 accesses)
- **Target speedup**: **1.5-1.8x**
- **Target time**: **0.55-0.67ms**

**Benchmark validation**:
```cpp
// Expected: BM_LoRA_Forward_Fused_CUDA should be 1.5-1.8x faster than Unfused
double speedup = time_unfused / time_fused;
assert(speedup >= 1.5 && speedup <= 1.8);
```

### 2. Backward Pass

**Current (Unfused)**:
- Kernel launches: 4
  1. `h^T @ grad_output` (compute grad_A intermediate)
  2. `grad_output @ A^T` (compute temp)
  3. `input^T @ temp` (compute grad_B)
  4. `temp @ B^T` (compute grad_input)
- Memory traffic: 7 global memory accesses
- Baseline time: **2.0ms** (at 768x768, rank=8, batch=16)

**Target (Fused)**:
- Kernel launches: 1
  - All gradients computed in single 3D-grid kernel
- Memory traffic: 4 global memory accesses (reduced intermediate writes)
- **Memory reduction**: 75% (7 → 4 accesses, accounting for gradient writes)
- **Target speedup**: **1.7-2.0x**
- **Target time**: **1.0-1.18ms**

**Benchmark validation**:
```cpp
double speedup = time_unfused / time_fused;
assert(speedup >= 1.7 && speedup <= 2.0);
```

### 3. Optimizer Step

**Current (Unfused)**:
- Operations: 3-4 separate kernels
  1. Add weight decay: `grad += weight_decay * param`
  2. Apply momentum: `v = momentum * v + (1-momentum) * grad`
  3. Update params: `param -= lr * v`
- Memory traffic: 4 global memory accesses
- Baseline time: **0.2ms** (for typical parameter count)

**Target (Fused)**:
- Operations: 1 fused kernel
  - `param -= lr * (grad + weight_decay * param + momentum * v)`
- Memory traffic: 2 global memory accesses (read params/grads, write params)
- **Memory reduction**: 50% (4 → 2 accesses)
- **Target speedup**: **1.3-1.5x**
- **Target time**: **0.13-0.15ms**

**Benchmark validation**:
```cpp
double speedup = time_unfused / time_fused;
assert(speedup >= 1.3 && speedup <= 1.5);
```

### 4. Full Training Step (End-to-End)

**Current (Unfused)**:
- Total kernels: 10-11
  - Forward: 3 kernels
  - Backward: 4 kernels
  - Optimizer: 3-4 kernels
- Total time: **3.2ms** (sum of components)

**Target (Fused)**:
- Total kernels: 3
  - Forward: 1 kernel
  - Backward: 1 kernel
  - Optimizer: 1 kernel
- **Overall speedup**: **1.5-2.0x**
- **Target time**: **1.5-2.0ms**

**Benchmark validation**:
```cpp
// Full training step: Forward + Backward + Optimizer
double speedup = time_unfused / time_fused;
assert(speedup >= 1.5 && speedup <= 2.0);
assert(time_fused >= 1.5 && time_fused <= 2.0);  // ms
```

## Memory Bandwidth Validation

### Expected Memory Traffic Reduction

**Test configuration**: 768x768, rank=8, batch=16

**Unfused memory traffic per training step**:
```
Forward:
  input read: 16 * 768 * 4 = 49,152 bytes
  B read: 768 * 8 * 4 = 24,576 bytes
  h write: 16 * 8 * 4 = 512 bytes
  h read: 512 bytes
  A read: 8 * 768 * 4 = 24,576 bytes
  output write: 16 * 768 * 4 = 49,152 bytes
  output read (scale): 49,152 bytes
  Total: ~197 KB

Backward:
  Multiple intermediate writes/reads
  Total: ~400 KB

Overall per step: ~600 KB
```

**Fused memory traffic per training step**:
```
Forward:
  input read: 49,152 bytes
  B read: 24,576 bytes
  A read: 24,576 bytes
  output write: 49,152 bytes
  Total: ~147 KB (25% reduction)

Backward:
  Reduced intermediate traffic
  Total: ~250 KB (37.5% reduction)

Overall per step: ~400 KB (33% overall reduction)
```

**Validation**:
```cpp
// Benchmark should report bytes processed
double bandwidth_reduction = (bytes_unfused - bytes_fused) / bytes_unfused;
assert(bandwidth_reduction >= 0.25 && bandwidth_reduction <= 0.40);  // 25-40% reduction
```

## Test Configurations

### Standard Test Sizes

| Configuration | in_dim | out_dim | rank | batch_size | Use Case |
|---------------|--------|---------|------|------------|----------|
| Small         | 256    | 256     | 4    | 16         | Quick validation |
| Medium        | 768    | 768     | 8    | 16         | BERT-base fine-tuning |
| Large         | 1024   | 1024    | 16   | 16         | Larger transformers |
| XLarge        | 2048   | 2048    | 16   | 16         | GPT-3, LLaMA-7B |

### Platform-Specific Expectations

#### CUDA (NVIDIA GPUs)
- **GPU**: Volta, Turing, Ampere, Ada, Hopper
- **Expected speedup**: 1.5-2.0x (as specified)
- **Optimal tile size**: 16x16
- **Shared memory usage**: ~2-4 KB per thread block

#### HIP (AMD GPUs)
- **GPU**: RDNA2, RDNA3, CDNA
- **Expected speedup**: 1.4-1.9x (slightly lower due to architecture differences)
- **Optimal tile size**: 16x16
- **Wave64 optimization**: Enabled

## Benchmark Execution

### Running Benchmarks

```bash
# Build with CUDA
cmake -B build -DTHEMIS_ENABLE_CUDA=ON -DTHEMIS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target bench_fused_kernels

# Run benchmarks
./build/benchmarks/bench_fused_kernels --benchmark_repetitions=10

# Expected output format:
# BM_LoRA_Forward_Unfused_CUDA/768/8    1000 us
# BM_LoRA_Forward_Fused_CUDA/768/8       600 us  (1.67x speedup) ✓
# BM_LoRA_Backward_Unfused_CUDA/768/8   2000 us
# BM_LoRA_Backward_Fused_CUDA/768/8     1100 us  (1.82x speedup) ✓
```

### Validation Script

```python
import re

def validate_benchmark_results(output_file):
    """Validate that fused kernels meet performance targets."""
    
    results = {}
    
    # Parse benchmark output
    with open(output_file) as f:
        for line in f:
            match = re.match(r'BM_LoRA_(\w+)_(Fused|Unfused)_CUDA.*\s+(\d+)\s+us', line)
            if match:
                operation, variant, time_us = match.groups()
                key = f"{operation}_{variant}"
                results[key] = int(time_us)
    
    # Validate forward pass
    forward_speedup = results['Forward_Unfused'] / results['Forward_Fused']
    assert 1.5 <= forward_speedup <= 1.8, f"Forward speedup {forward_speedup} not in [1.5, 1.8]"
    
    # Validate backward pass
    backward_speedup = results['Backward_Unfused'] / results['Backward_Fused']
    assert 1.7 <= backward_speedup <= 2.0, f"Backward speedup {backward_speedup} not in [1.7, 2.0]"
    
    # Validate training step
    step_speedup = results['TrainingStep_Unfused'] / results['TrainingStep_Fused']
    assert 1.5 <= step_speedup <= 2.0, f"Training step speedup {step_speedup} not in [1.5, 2.0]"
    
    print("✓ All performance targets met!")
    print(f"  Forward: {forward_speedup:.2f}x (target: 1.5-1.8x)")
    print(f"  Backward: {backward_speedup:.2f}x (target: 1.7-2.0x)")
    print(f"  Training step: {step_speedup:.2f}x (target: 1.5-2.0x)")
```

## Success Criteria Summary

### Must Pass (Required)
- ✅ Numerical accuracy: fused == unfused within 1e-5
- ✅ No crashes or errors during benchmarks
- ✅ Forward speedup: 1.5-1.8x
- ✅ Backward speedup: 1.7-2.0x
- ✅ Overall training step: 1.5-2.0x

### Should Pass (Goals)
- 🎯 Memory bandwidth reduction: 66-75%
- 🎯 Kernel count reduction: 10-11 → 3
- 🎯 Consistent speedup across different tensor sizes

### Optional (Nice to Have)
- 💡 Speedup exceeds upper bound (>1.8x forward, >2.0x backward)
- 💡 Works on older GPU architectures (Pascal, Polaris)
- 💡 Scales well with batch size

## Troubleshooting

### If benchmarks fail to meet targets:

1. **Check GPU utilization**: Use `nvidia-smi` or `rocm-smi`
2. **Profile with nsight/rocprof**: Identify bottlenecks
3. **Verify tile sizes**: Try 8x8, 16x16, 32x32
4. **Check occupancy**: May need to reduce shared memory usage
5. **Temperature throttling**: Ensure GPU is not thermal throttling

### Common issues:

- **Lower than expected speedup**: May indicate memory bottleneck, not compute
- **Higher than expected speedup**: Baseline may have been suboptimal
- **Inconsistent results**: Check for thermal throttling or background processes

## References

- Phase 10.3 Implementation: `LORA_GPU_PHASE10_PLAN.md`
- Kernel Fusion Design: `KERNEL_FUSION_IMPLEMENTATION.md`
- Test Implementation: `tests/test_fused_kernels.cpp`
- Benchmark Implementation: `benchmarks/bench_fused_kernels.cpp`

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Status**: Reference for validation
