# Performance Tuning Guide

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Target Audience:** ML Engineers, Performance Engineers

## Table of Contents

1. [Overview](#overview)
2. [Batch Size Selection](#batch-size-selection)
3. [Learning Rate Optimization](#learning-rate-optimization)
4. [Gradient Accumulation](#gradient-accumulation)
5. [Mixed Precision Training](#mixed-precision-training)
6. [VRAM Optimization](#vram-optimization)
7. [Multi-GPU Tuning](#multi-gpu-tuning)
8. [Throughput Optimization](#throughput-optimization)
9. [Latency Optimization](#latency-optimization)

---

## Overview

This guide covers performance optimization techniques for GPU-accelerated training and inference workloads in ThemisDB. Proper tuning can improve throughput by 2-5x and reduce memory usage by 40-60%.

### Performance Metrics

**Training Metrics:**
- Samples/second
- Tokens/second
- GPU utilization (target: >85%)
- VRAM usage
- Training loss convergence

**Inference Metrics:**
- Latency (P50, P95, P99)
- Throughput (requests/second)
- First token latency
- GPU utilization
- Batch processing time

---

## Batch Size Selection

### Impact Analysis

**Batch Size vs Performance:**

| Batch Size | Throughput | VRAM Usage | Training Stability | Convergence |
|------------|------------|------------|-------------------|-------------|
| 1-8 | Low | Low | High variance | Slow |
| 16-32 | Medium | Medium | Good | Good |
| 64-128 | High | High | Very stable | Fast |
| 256+ | Very High | Very High | Most stable | Fastest |

### Selection Strategy

**For Training:**

```python
# Calculate maximum batch size
max_batch_size = (available_vram - model_size - optimizer_overhead) / per_sample_memory

# Rule of thumb for LLaMA models:
# 7B model: 16-32 batch size on 24GB GPU
# 13B model: 8-16 batch size on 40GB GPU
# 70B model: 4-8 batch size on 80GB GPU

# Example configuration
training:
  batch_size: 32
  micro_batch_size: 8  # Split into 4 gradient accumulation steps
  sequence_length: 2048
```

**For Inference:**

```yaml
inference:
  # Single request (lowest latency)
  batch_size: 1
  continuous_batching: false
  
  # High throughput (batch multiple requests)
  batch_size: 32
  continuous_batching: true
  max_wait_time_ms: 100
```

### Dynamic Batch Sizing

```yaml
training:
  dynamic_batch_size:
    enabled: true
    min_batch_size: 8
    max_batch_size: 64
    target_vram_utilization: 0.9  # Target 90% VRAM usage
    adjustment_interval: 100  # Adjust every 100 steps
```

### Batch Size Calculation Tool

```bash
# Calculate optimal batch size
themisdb-cli calc batch-size \
  --model llama-2-7b \
  --gpu-memory 24576 \
  --sequence-length 2048 \
  --precision fp16

# Expected output:
# Model size: 7.0B parameters (14.0 GB in FP16)
# Optimizer states: 4.0 GB (AdamW)
# Gradients: 2.0 GB
# Available for activations: 4.0 GB
# Estimated batch size: 24-32
# Recommended: 24 (safe), 28 (optimal), 32 (aggressive)
```

---

## Learning Rate Optimization

### Learning Rate Selection

**Initial Learning Rate by Model Size:**

| Model Size | Batch Size 32 | Batch Size 64 | Batch Size 128 |
|------------|---------------|---------------|----------------|
| 7B params  | 3e-4 | 4e-4 | 5e-4 |
| 13B params | 2e-4 | 3e-4 | 4e-4 |
| 30B params | 1.5e-4 | 2e-4 | 3e-4 |
| 65B+ params | 1e-4 | 1.5e-4 | 2e-4 |

**Linear Scaling Rule:**

```python
# When doubling batch size, multiply learning rate by √2
new_lr = base_lr * sqrt(new_batch_size / base_batch_size)

# Example: base_lr=3e-4 for batch_size=32
# For batch_size=64: lr = 3e-4 * sqrt(64/32) = 4.24e-4
```

### Learning Rate Schedules

**Warmup + Cosine Decay (Recommended):**

```yaml
training:
  learning_rate:
    initial: 3e-4
    schedule: cosine_with_warmup
    warmup_steps: 2000
    min_lr: 3e-5
    total_steps: 100000
```

**Linear Warmup + Constant:**

```yaml
training:
  learning_rate:
    initial: 3e-4
    schedule: linear_warmup
    warmup_steps: 1000
    constant_after_warmup: true
```

**OneCycle Policy:**

```yaml
training:
  learning_rate:
    max_lr: 5e-4
    schedule: onecycle
    pct_start: 0.3  # 30% warmup
    div_factor: 25  # initial_lr = max_lr / 25
    final_div_factor: 10000
```

### Adaptive Learning Rates

```yaml
training:
  optimizer: adamw
  learning_rate:
    initial: 3e-4
    adaptive:
      enabled: true
      monitor: validation_loss
      factor: 0.5  # Reduce by 50% on plateau
      patience: 5  # Wait 5 evaluations before reducing
      min_lr: 1e-6
```

---

## Gradient Accumulation

### Why Gradient Accumulation?

Simulate larger batch sizes without increasing VRAM usage:

```
Effective batch size = micro_batch_size × gradient_accumulation_steps
```

### Configuration

```yaml
training:
  batch_size: 128  # Effective batch size
  micro_batch_size: 16  # Fits in VRAM
  gradient_accumulation_steps: 8  # 128 / 16 = 8
```

### Trade-offs

| Gradient Accumulation | Throughput | Memory | Training Speed |
|----------------------|------------|--------|----------------|
| 1 step (no accumulation) | Fastest | Highest | Fastest |
| 4 steps | -15% | -60% | Good |
| 8 steps | -25% | -75% | Acceptable |
| 16 steps | -40% | -87% | Slow |

### Optimal Configuration

```yaml
# For 24GB GPU with 7B model
training:
  micro_batch_size: 8
  gradient_accumulation_steps: 4
  # Effective batch size: 32

# For 40GB GPU with 13B model
training:
  micro_batch_size: 16
  gradient_accumulation_steps: 4
  # Effective batch size: 64
```

---

## Mixed Precision Training

### Precision Formats

| Format | Memory | Speed | Precision | Use Case |
|--------|--------|-------|-----------|----------|
| FP32 | 1.0x | 1.0x | Highest | Debugging, small models |
| FP16 | 0.5x | 2-3x | Good | Most training |
| BF16 | 0.5x | 2-3x | Better | Large models (A100+) |
| TF32 | 1.0x | 1.5x | Good | Automatic (A100+) |
| INT8 | 0.25x | 4x | Lower | Inference only |

### FP16 Training

```yaml
training:
  precision: fp16
  mixed_precision:
    enabled: true
    loss_scale: dynamic  # or: static with value like 1024.0
    loss_scale_window: 1000
    hysteresis: 2
```

### BF16 Training (A100/H100)

```yaml
training:
  precision: bf16
  mixed_precision:
    enabled: true
  # No loss scaling needed for BF16
```

### Automatic Mixed Precision (AMP)

```yaml
training:
  precision: auto  # Automatically choose FP16/BF16 based on GPU
  amp:
    enabled: true
    opt_level: O2  # O0=FP32, O1=mixed, O2=FP16, O3=full FP16
    keep_batchnorm_fp32: true
```

### Precision Selection Guide

**Use FP16 when:**
- GPU: RTX 3090, RTX 4090, A6000
- Model: <30B parameters
- Need: Maximum speed

**Use BF16 when:**
- GPU: A100, H100
- Model: >30B parameters
- Need: Numerical stability

**Use FP32 when:**
- Debugging convergence issues
- Final fine-tuning (optional)
- Small models where memory isn't a constraint

---

## VRAM Optimization

### Memory Breakdown

```
Total VRAM = Model Weights + Optimizer States + Gradients + Activations + Buffers
```

**Example: 7B model, FP16, AdamW, batch_size=16:**

```
Model weights: 14 GB (7B × 2 bytes)
Optimizer (AdamW): 28 GB (2 states × 14 GB)
Gradients: 14 GB
Activations: 8 GB (depends on batch size)
Buffers: 2 GB
---------------------------------
Total: 66 GB (doesn't fit in 24GB GPU!)
```

### Optimization Techniques

#### 1. Gradient Checkpointing

Trade computation for memory:

```yaml
training:
  gradient_checkpointing:
    enabled: true
    checkpoint_segments: 4  # More segments = less memory, slower
```

**Memory savings:** 60-80%  
**Speed penalty:** 20-30%

#### 2. CPU Offloading

```yaml
training:
  cpu_offload:
    enabled: true
    offload_optimizer: true  # Move optimizer to CPU
    offload_gradients: false  # Keep gradients on GPU
    pin_memory: true
```

**Memory savings:** 40-60%  
**Speed penalty:** 10-20%

#### 3. 8-bit Optimizers

```yaml
training:
  optimizer: adamw_8bit
  # Uses bitsandbytes for 8-bit optimizer
  # Saves 75% optimizer memory with minimal quality loss
```

**Memory savings:** 75%  
**Quality impact:** <1% degradation

#### 4. Flash Attention

```yaml
training:
  attention:
    implementation: flash_attention_2
    # 50% memory reduction for attention layers
```

**Memory savings:** 30-50%  
**Speed improvement:** 15-25%

#### 5. Model Parallelism

```yaml
training:
  model_parallel:
    enabled: true
    tensor_parallel_size: 4  # Split model across 4 GPUs
    pipeline_parallel_size: 1
```

### Complete Optimization Stack

```yaml
training:
  # Efficient configuration for 24GB GPU, 7B model
  precision: fp16
  micro_batch_size: 4
  gradient_accumulation_steps: 8  # Effective batch: 32
  
  gradient_checkpointing:
    enabled: true
    checkpoint_segments: 4
  
  optimizer: adamw_8bit
  
  attention:
    implementation: flash_attention_2
  
  cpu_offload:
    enabled: true
    offload_optimizer: true
```

---

## Multi-GPU Tuning

### Parallelism Strategies

#### Data Parallelism (Recommended)

Best for: Most training scenarios

```yaml
multi_gpu:
  strategy: data_parallel
  devices: [0, 1, 2, 3]
  backend: nccl  # Fastest for NVIDIA
  
  # Each GPU processes different data batches
  # Gradients synchronized after each step
```

**Scaling efficiency:** 85-95% with 4 GPUs, 70-80% with 8 GPUs

#### Tensor Parallelism

Best for: Models too large for single GPU

```yaml
multi_gpu:
  strategy: tensor_parallel
  tensor_parallel_size: 4
  
  # Model layers split across GPUs
  # All GPUs process same data
```

**Scaling efficiency:** 60-75%

#### Pipeline Parallelism

Best for: Very large models

```yaml
multi_gpu:
  strategy: pipeline_parallel
  pipeline_parallel_size: 4
  micro_batches: 16
  
  # Model split into stages
  # GPUs process different stages
```

**Scaling efficiency:** 50-70%

### NCCL Tuning

```bash
# Environment variables for optimal NCCL performance
export NCCL_DEBUG=WARN
export NCCL_IB_DISABLE=0  # Enable InfiniBand if available
export NCCL_P2P_LEVEL=NVL  # Use NVLink
export NCCL_ALGO=Ring
export NCCL_PROTO=Simple

# For systems with slow interconnects
export NCCL_BUFFSIZE=2097152
export NCCL_NTHREADS=4
```

### Load Balancing

```yaml
multi_gpu:
  # Uneven GPU distribution for mixed GPU types
  tensor_split: [0.4, 0.3, 0.2, 0.1]  # A100, A100, RTX4090, RTX4090
  
  # Or automatic balancing
  auto_balance: true
  balance_metric: memory  # or: compute, throughput
```

---

## Throughput Optimization

### Training Throughput

**Target:** >80% GPU utilization, >10K tokens/second

```yaml
training:
  # Maximize batch size
  micro_batch_size: 32  # As large as VRAM allows
  
  # Enable all speedup features
  precision: bf16
  attention: flash_attention_2
  compilation: torch_compile  # 10-30% speedup
  
  # Reduce I/O bottlenecks
  dataloader:
    num_workers: 8
    prefetch_factor: 4
    pin_memory: true
    persistent_workers: true
  
  # Optimize checkpointing
  checkpoint:
    save_interval: 1000  # Less frequent = faster
    async_save: true
```

### Inference Throughput

**Target:** <50ms P95 latency, >100 req/sec

```yaml
inference:
  # Continuous batching
  continuous_batching:
    enabled: true
    max_batch_size: 64
    max_wait_time_ms: 50
  
  # KV cache optimization
  kv_cache:
    enabled: true
    max_tokens: 65536
    block_size: 16
  
  # Speculative decoding
  speculative_decoding:
    enabled: true
    draft_model: llama-2-7b-draft
    num_candidates: 5
```

---

## Latency Optimization

### Reduce First Token Latency

```yaml
inference:
  # Preload models
  model_preload: true
  
  # Reduce batch size
  batch_size: 1
  
  # Skip waiting for batch fill
  continuous_batching: false
  
  # Fast attention
  attention: flash_attention_2
  
  # Reduce precision
  precision: fp16  # or int8 for even lower latency
```

### Reduce Per-Token Latency

```yaml
inference:
  # Speculative decoding (2-3x speedup)
  speculative_decoding:
    enabled: true
  
  # Smaller models for faster generation
  model_size: 7b  # vs 70b
  
  # Quantization
  quantization: gptq_4bit  # or: awq_4bit
```

---

## Benchmarking

### Built-in Benchmark Suite

```bash
# Training benchmark
themisdb-cli benchmark training \
  --model llama-2-7b \
  --batch-size 32 \
  --sequence-length 2048 \
  --steps 100 \
  --report-interval 10

# Inference benchmark
themisdb-cli benchmark inference \
  --model llama-2-7b \
  --batch-size 1,4,8,16,32 \
  --sequence-length 512 \
  --num-requests 1000 \
  --concurrency 10
```

### Performance Profiling

```bash
# Enable profiler
themisdb-cli profile \
  --mode training \
  --output profile.json \
  --duration 60

# View profile
themisdb-cli profile view profile.json
```

---

## Recommended Configurations

### Development (Single GPU)

```yaml
# 24GB GPU, 7B model, FP16
training:
  precision: fp16
  micro_batch_size: 8
  gradient_accumulation_steps: 4
  gradient_checkpointing: true
  optimizer: adamw_8bit
```

### Production Training (Multi-GPU)

```yaml
# 4× A100 40GB, 70B model, BF16
training:
  precision: bf16
  micro_batch_size: 16
  gradient_accumulation_steps: 4
  
  multi_gpu:
    strategy: data_parallel
    devices: [0, 1, 2, 3]
  
  gradient_checkpointing: false  # Enough memory
  optimizer: adamw
  attention: flash_attention_2
```

### High-Throughput Inference

```yaml
# Multiple GPUs, serving production traffic
inference:
  precision: fp16
  continuous_batching: true
  max_batch_size: 64
  
  kv_cache:
    enabled: true
    max_tokens: 131072
  
  speculative_decoding:
    enabled: true
```

---

## Next Steps

- **Monitoring**: Set up metrics to track performance ([MONITORING.md](MONITORING.md))
- **Troubleshooting**: Debug performance issues ([TROUBLESHOOTING.md](TROUBLESHOOTING.md))
- **Production**: Deploy optimized configuration ([RUNBOOKS.md](RUNBOOKS.md))

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026
