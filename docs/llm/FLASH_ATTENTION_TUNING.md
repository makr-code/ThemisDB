# Flash Attention v3 Performance Tuning Guide

## Overview

This guide provides recommendations for tuning Flash Attention v3 performance across different hardware platforms and workloads.

## Hardware-Specific Tuning

### NVIDIA H100 (SM90 - Hopper Architecture)

**Optimal Configuration:**
```yaml
attention:
  backend: "cuda_sm90"
  enable_flash_v3: true
  enable_tensor_cores: true
  enable_async_copy: true
  enable_warp_specialization: true
  tile_size: 64
  block_size: 256
  num_warps: 8

hardware:
  h100:
    enable_tma: true              # Tensor Memory Accelerator
    enable_hopper_features: true
    sm_count: 132
```

**Expected Performance:**
- Throughput: 3000+ TFLOPs for attention
- Speedup: 30x vs standard attention
- Memory bandwidth: ~3.35 TB/s

**Tuning Tips:**
- Use FP16 or BF16 for maximum tensor core utilization
- Enable TMA for asynchronous memory copy
- Use warp specialization (producer/consumer warps)
- Prefill chunk size: 512-1024 tokens
- Decode batch size: 128-256 sequences

### NVIDIA A100 (SM86 - Ampere)

**Optimal Configuration:**
```yaml
attention:
  backend: "cuda_sm86"
  enable_flash_v2: true
  enable_tensor_cores: true
  tile_size: 64
  block_size: 256
  quantization: "fp16"

hardware:
  a100:
    enable_ampere_features: true
    sm_count: 108
```

**Expected Performance:**
- Throughput: 400-500 TFLOPs
- Speedup: 5x vs standard attention
- Memory bandwidth: ~1.6 TB/s

**Tuning Tips:**
- Use FP16 with tensor cores
- Batch size: 32-64 for optimal throughput
- Enable paged KV-cache to reduce memory pressure
- Prefill chunk size: 256-512 tokens

### NVIDIA RTX 4090 (SM89 - Ada Lovelace)

**Optimal Configuration:**
```yaml
attention:
  backend: "cuda_sm86"  # Use SM86 kernels (compatible)
  enable_flash_v2: true
  enable_tensor_cores: true
  tile_size: 64
  quantization: "fp16"

hardware:
  rtx4090:
    enable_ada_features: true
    sm_count: 128
```

**Expected Performance:**
- Throughput: 300-400 TFLOPs
- Speedup: 5x vs standard attention
- Memory bandwidth: ~1 TB/s

**Tuning Tips:**
- Consumer GPU - optimize for single user workloads
- Batch size: 8-16 for low latency
- Use aggressive KV-cache eviction
- Prefill chunk size: 256 tokens

### AMD MI300 (CDNA 3)

**Optimal Configuration:**
```yaml
attention:
  backend: "hip_mi300"
  tile_size: 64
  block_size: 256
  quantization: "fp16"

hardware:
  mi300:
    enable_wave64: true
    compute_units: 304
```

**Expected Performance:**
- Throughput: 600-800 TFLOPs (estimated)
- Speedup: 8x vs standard attention
- Memory bandwidth: ~5.3 TB/s

**Tuning Tips:**
- Use Wave64 optimization for maximum ALU utilization
- Enable LDS (Local Data Share) optimization
- Batch size: 32-64 sequences

### Vulkan (Cross-Platform)

**Optimal Configuration:**
```yaml
attention:
  backend: "vulkan"
  tile_size: 64
  block_size: 128
  quantization: "fp32"  # Vulkan compute precision

performance:
  prefill_chunk_size: 256
  decode_batch_size: 8
```

**Expected Performance:**
- Throughput: 150-200 TFLOPs
- Speedup: 3-4x vs standard attention
- Works on NVIDIA, AMD, Intel, ARM GPUs

**Tuning Tips:**
- Lower batch sizes for consumer GPUs
- Use smaller tile sizes (32-64)
- Enable descriptor set caching
- Profile with Vulkan validation layers

## Workload-Specific Tuning

### Low Latency (Real-time Chat)

**Goal:** Minimize time-to-first-token

```yaml
optimization:
  preset: "latency"

model:
  batch_size: 1
  max_seq_length: 2048

attention:
  tile_size: 32              # Smaller tiles for lower latency
  block_size: 128
  use_paged_kv_cache: true
  enable_prefix_caching: true

performance:
  prefill_chunk_size: 256
  decode_batch_size: 1
```

**Tuning Tips:**
- Batch size = 1 for lowest latency
- Use prefix caching for common prompts
- Smaller tile sizes reduce kernel launch overhead
- Aggressive GPU clock boosting

**Expected Results:**
- Time-to-first-token: 50-100ms (2K context)
- Decode latency: 10-20ms per token

### High Throughput (Batch Processing)

**Goal:** Maximize tokens/second

```yaml
optimization:
  preset: "throughput"

model:
  batch_size: 128            # Large batch
  max_seq_length: 4096

attention:
  tile_size: 64
  block_size: 256
  enable_kernel_fusion: true
  enable_continuous_batching: true

performance:
  prefill_chunk_size: 1024   # Large chunks
  decode_batch_size: 128
```

**Tuning Tips:**
- Large batch sizes (64-256)
- Large prefill chunks (512-1024 tokens)
- Enable continuous batching
- Use mixed precision (FP16)

**Expected Results:**
- Throughput: 10,000-50,000 tokens/second
- GPU utilization: 85-95%

### Memory Efficient (Long Contexts)

**Goal:** Support 16K-32K token contexts

```yaml
optimization:
  preset: "memory_efficient"

model:
  batch_size: 8              # Smaller batch
  max_seq_length: 32768      # Long context

attention:
  use_paged_kv_cache: true
  kv_block_size: 16
  num_kv_blocks: 8192
  enable_prefix_caching: true
  quantization: "int8"       # Quantized KV-cache

performance:
  prefill_chunk_size: 512
  enable_continuous_batching: false
```

**Tuning Tips:**
- Use paged KV-cache to reduce memory
- Quantize KV-cache (INT8 or Q4)
- Enable prefix caching aggressively
- Smaller batch sizes

**Expected Results:**
- Context length: 32K+ tokens
- Memory: 2-4x reduction vs standard
- Throughput: 2,000-5,000 tokens/second

## Quantization Trade-offs

### FP32 (Full Precision)

**Pros:**
- Maximum accuracy
- No calibration needed

**Cons:**
- 4x memory vs FP16
- 2x slower vs FP16 with tensor cores

**Use Case:** Research, debugging

### FP16 (Half Precision)

**Pros:**
- 2x memory savings vs FP32
- 2-8x speedup with tensor cores
- Minimal accuracy loss (<0.1%)

**Cons:**
- Requires tensor cores for speedup

**Use Case:** Production (recommended)

### BF16 (Brain Float 16)

**Pros:**
- Same dynamic range as FP32
- Better numerical stability than FP16
- 2x memory savings vs FP32

**Cons:**
- Requires Ampere+ (NVIDIA) or CDNA (AMD)

**Use Case:** Training, mixed precision

### INT8

**Pros:**
- 4x memory savings vs FP32
- 2x speedup vs FP16

**Cons:**
- Requires calibration
- 1-2% accuracy loss

**Use Case:** Edge deployment, long contexts

### Q4 (4-bit Quantization)

**Pros:**
- 8x memory savings vs FP32
- Enable 2x longer contexts

**Cons:**
- 3-5% accuracy loss
- Requires careful calibration

**Use Case:** Extreme memory constraints

## KV-Cache Tuning

### Block Size Selection

**Small Blocks (8-16 tokens):**
- Lower fragmentation
- Better memory utilization
- More allocation overhead

**Large Blocks (32-64 tokens):**
- Lower allocation overhead
- Higher fragmentation
- Simpler management

**Recommended:** 16 tokens per block

### Prefix Caching Strategy

**Aggressive Caching:**
```yaml
attention:
  enable_prefix_caching: true
  kv_block_size: 16
  prefix_sharing_threshold: 64  # Minimum tokens to share
```

**Use Cases:**
- Few-shot prompting (share examples)
- Chat with system prompts
- RAG with common context

**Expected Savings:** 50-90% memory reduction

### Cache Eviction Policies

**LRU (Least Recently Used):**
- Best for interactive workloads
- Evict old sequences first

**LFU (Least Frequently Used):**
- Best for batch processing
- Keep hot sequences in cache

**TTL (Time-To-Live):**
- Evict after fixed time
- Predictable memory usage

## Performance Monitoring

### Key Metrics

1. **Attention Throughput (TFLOPs)**
   - Target: 200+ TFLOPs (consumer), 1000+ TFLOPs (datacenter)

2. **Memory Bandwidth Utilization (%)**
   - Target: 60-80% of peak bandwidth

3. **KV-Cache Hit Rate (%)**
   - Target: 70-90% with prefix caching

4. **Kernel Launch Overhead (μs)**
   - Target: <100μs per kernel

5. **Time-to-First-Token (ms)**
   - Target: <100ms for 2K context

### Profiling Tools

**NVIDIA:**
```bash
# NSight Compute (kernel profiling)
ncu --set full --target-processes all ./themis_server

# NSight Systems (timeline profiling)
nsys profile --trace=cuda,nvtx ./themis_server
```

**AMD:**
```bash
# ROCm Profiler
rocprof --hip-trace --sys-trace ./themis_server

# ROCm SMI (real-time monitoring)
watch -n 1 rocm-smi
```

**Vulkan:**
```bash
# RenderDoc (frame capture)
renderdoc --capture ./themis_server

# Vulkan validation layers
VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d \
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
./themis_server
```

## Common Issues and Solutions

### Issue: Low GPU Utilization (<50%)

**Causes:**
- Batch size too small
- CPU-GPU transfer bottleneck
- Kernel launch overhead

**Solutions:**
- Increase batch size
- Enable continuous batching
- Use async copy
- Pin host memory

### Issue: Out of Memory (OOM)

**Causes:**
- Batch size too large
- Context length too long
- KV-cache not evicting

**Solutions:**
- Reduce batch size
- Enable paged KV-cache
- Use quantization (INT8/Q4)
- Implement cache eviction

### Issue: Slow Prefill Phase

**Causes:**
- Large context length
- Small chunk size
- CPU tokenization bottleneck

**Solutions:**
- Increase prefill_chunk_size
- Use larger tiles (128)
- Parallelize tokenization
- Enable prefix caching

### Issue: Accuracy Degradation

**Causes:**
- Over-aggressive quantization
- Numerical instability
- Wrong precision settings

**Solutions:**
- Use FP16 instead of INT8
- Enable mixed precision (FP32 accumulation)
- Validate with reference implementation
- Check for NaN/Inf values

## Best Practices

1. **Always profile before optimizing**
   - Measure baseline performance
   - Identify bottlenecks
   - Validate improvements

2. **Start with conservative settings**
   - Use FP16 precision
   - Moderate batch sizes (16-32)
   - Standard tile sizes (64)

3. **Enable caching for common patterns**
   - System prompts
   - Few-shot examples
   - RAG context

4. **Monitor memory usage**
   - Track KV-cache growth
   - Set memory limits
   - Implement eviction policies

5. **Validate accuracy**
   - Compare with reference implementation
   - Test edge cases
   - Monitor drift over time

6. **Automate performance testing**
   - CI/CD benchmarks
   - Regression detection
   - A/B testing

## References

- [Flash Attention Paper](https://arxiv.org/abs/2205.14135)
- [Flash Attention 2 Paper](https://arxiv.org/abs/2307.08691)
- [vLLM PagedAttention](https://arxiv.org/abs/2309.06180)
- [NVIDIA Tensor Cores](https://www.nvidia.com/en-us/data-center/tensor-cores/)

## Getting Help

For performance tuning assistance:
1. Check ThemisDB documentation
2. Join community forums
3. File GitHub issues with profiling data
4. Contact support for enterprise customers
