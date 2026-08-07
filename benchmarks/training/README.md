# benchmarks/training

Phase 5 Performance Gates and Stress Test Benchmarks for Training Module

## Overview

This directory contains comprehensive performance benchmarks for ThemisDB's training module with Phase 5 hardening gates. All benchmarks validate performance against established thresholds and report regressions for continuous monitoring.

## Phase 5 Performance Gates

### CPU Training Performance Targets

| Operation | Target | Measurement | Notes |
|-----------|--------|-------------|-------|
| LoRA Layer Construction | <50µs | per-layer | Standard 768-dim layer |
| Forward Pass (LoRA) | <100µs/sample | single sample | Batch size = 1 |
| Forward Pass (Attention) | <150µs/sample | single sample | Batch size = 1 |
| Backward Pass (LoRA) | <150µs/sample | single sample | Batch size = 1 |
| Backward Pass (Attention) | <200µs/sample | single sample | Batch size = 1 |
| Training Step (32-sample batch) | <500ms | end-to-end | Forward + Backward + Optimizer |
| Adapter Merge | <100ms | operation | Merge 2+ adapters |
| Checkpoint Save (50MB) | <200ms | operation | Standard-size adapter |
| Checkpoint Load (50MB) | <500ms | operation | Standard-size adapter |

### GPU Training Performance Targets

| Operation | Target | Measurement | Notes |
|-----------|--------|-------------|-------|
| Forward Pass | ≤2x CPU baseline | GPU vs CPU | Batch size dependent |
| Backward Pass | ≤2x CPU baseline | GPU vs CPU | Batch size dependent |
| Min GPU Speedup (batch>1) | ≥1.5x | GPU/CPU ratio | Batch sizes 4+ recommended |
| Memory Cleanup | <10ms | after training step | GPU memory reclamation |
| GPU-to-CPU Transfer | <200ms | adapter data | Standard-size (50MB) |

### Memory Regression Detection

- **CPU Memory**: Flag increases >5% over baseline during sustained training
- **GPU Memory**: Flag increases >10% over baseline during training
- **Extended Sessions**: Monitor 1000+ training steps for memory leaks
- **Concurrent Adapters**: Test 4-8 simultaneous adapter training

## Benchmark Categories

### 1. Layer Construction Benchmarks

```bash
./build-*/bin_out_tests/bench_lora_training --benchmark_filter="Construction"
```

Measures:
- LoRA layer construction time with varying dimensions (256-4096)
- Attention LoRA construction
- Sequential layer composition overhead
- Per-layer construction time tracking

**Gate Validation**: Reports violations when construction exceeds 50µs for standard dimensions.

### 2. Forward Pass Benchmarks

```bash
./build-*/bin_out_tests/bench_lora_training --benchmark_filter="Forward"
```

Measures:
- Single-sample forward pass performance
- Per-sample throughput
- Layer dimension scaling
- Sequential pass composition overhead

**Gate Validation**: Reports if forward pass exceeds 100µs/sample (LoRA) or 150µs/sample (Attention).

### 3. Backward Pass Benchmarks

```bash
./build-*/bin_out_tests/bench_lora_training --benchmark_filter="Backward"
```

Measures:
- Single-sample backward pass performance
- Gradient computation overhead
- Per-sample throughput
- Sequential backward composition

**Gate Validation**: Reports if backward exceeds 150µs/sample (LoRA) or 200µs/sample (Attention).

### 4. Memory Efficiency Benchmarks

```bash
./build-*/bin_out_tests/bench_lora_training --benchmark_filter="Memory"
```

Measures:
- Layer memory footprint
- LoRA vs full fine-tuning memory comparison
- Rank impact on memory usage
- Memory baseline tracking

**Gate Validation**: Compares current memory with baseline and reports >5% regression.

### 5. Phase 5 Stress Test Benchmarks

```bash
./build-*/bin_out_tests/bench_lora_training --benchmark_filter="Extended|Concurrent|LargeBatch"
```

#### Extended Training Session (1000+ steps)

Validates:
- No memory accumulation over 1000 training steps
- Consistent per-step performance (no degradation)
- Proper gradient cleanup
- Checkpoint/restore cycles

**Gate Validation**: Ensures training step remains <500ms throughout session.

#### Concurrent Adapter Training

Validates:
- Training 4+ adapters simultaneously
- No cross-adapter memory interference
- Independent gradient streams
- Resource contention handling

**Gate Validation**: Per-adapter throughput remains consistent.

#### Large Batch Training (Memory Pressure)

Validates:
- Training with batch sizes 64, 128, 256
- Graceful memory management under pressure
- No out-of-memory during training
- Backward pass stability

**Gate Validation**: Reports memory pressure scenarios and peak usage.

### 6. GPU Training Benchmarks

```bash
./build-*/bin_out_tests/bench_gpu_training_cycle [--benchmark_filter="CUDA|HIP|Vulkan"]
```

Measures (CUDA/HIP/Vulkan):
- CPU baseline (reference)
- GPU training cycle performance
- GPU vs CPU speedup ratio
- Complete training step (forward + backward + optimizer)
- Memory transfer overhead

**Gate Validation**:
- GPU performance within 2x CPU baseline
- Minimum 1.5x speedup for batch size > 1
- GPU memory cleanup < 10ms after step

## Interpreting Performance Gate Output

### Success Indication

```
[PERF_GATE] No violations - all operations within target envelopes
```

### Regression Detection

```
[PERF_GATE] BM_LoRALayer_Forward VIOLATION: measured=125.5µs gate=100.0µs (+25.5%)
```

Means:
- Operation exceeded target by 25.5%
- May indicate regression or system overload
- Investigate with profiling tools before committing

### GPU Speedup Violations

```
[GPU_SPEEDUP_GATE] BM_TrainingCycle_CUDA: speedup=1.2x (target min=1.5x)
```

Means:
- GPU not delivering expected speedup
- May indicate memory bottleneck or computation/comm ratio
- Review kernel efficiency

## Regression Thresholds by Hardware

### Baseline Hardware (Reference)
- CPU: Intel Core i7-12700K or equivalent
- GPU: NVIDIA RTX 4090 or equivalent
- Memory: 64GB+ system RAM, 24GB+ GPU VRAM

### Conservative Thresholds (Slower Hardware)
- Increase gate targets by 50%
- Report as "Adjusted for Hardware X"
- Document in benchmark run notes

### Performance Variance
- Single-run variance: ±10-15% expected
- Use multiple runs (5+) for gate decisions
- Use geometric mean for multi-run average

## Running All Phase 5 Benchmarks

```bash
# CPU benchmarks with all gates
cd build-release
./bin_out_tests/bench_lora_training \
  --benchmark_time_unit=us \
  --benchmark_repetitions=5 \
  --benchmark_display_aggregates_only=true

# GPU benchmarks (if CUDA available)
./bin_out_tests/bench_gpu_training_cycle \
  --benchmark_time_unit=ms \
  --benchmark_repetitions=5 \
  --benchmark_display_aggregates_only=true

# Generate JSON report for CI
./bin_out_tests/bench_lora_training \
  --benchmark_out=perf_gates_cpu.json \
  --benchmark_out_format=json

./bin_out_tests/bench_gpu_training_cycle \
  --benchmark_out=perf_gates_gpu.json \
  --benchmark_out_format=json
```

## Gate Violation Response

When a performance gate violation is detected:

1. **Immediate**: Run benchmark again (variance check)
2. **Profiling**: Use `perf`, `flamegraph`, or `vtune` to identify hot paths
3. **Analysis**: Compare with last passing run
4. **Decision**:
   - If regression: Identify commit and revert or optimize
   - If new baseline: Document change with justification
   - If variance: Use more repetitions for stability

## Documentation

- **Phase 5 Requirements**: See ROADMAP.md (lines 48-50)
- **Measurement Hygiene**: See benchmarks/MEASUREMENT_HYGIENE.md
- **Performance Expectations**: See PERFORMANCE_EXPECTATIONS.md
- **Hardware Requirements**: See HARDWARE_REQUIREMENTS.md

## Integration with CI/CD

Performance gates are validated in:
- `release_critical` CI workflow (gates blocking promotion)
- `nightly` performance monitoring (trend detection)
- Pull request benchmarks (regression detection before merge)

See `.github/workflows/` for integration details.
