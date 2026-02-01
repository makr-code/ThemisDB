# VRAM Configuration Tuning Guide

## Quick Reference

### GPU Selection Matrix

| GPU Model | VRAM | Best For | Max Model (FP16) | Max Model (Q4) |
|-----------|------|----------|------------------|----------------|
| RTX 4060 Ti | 16 GB | Development | 7B | 30B |
| RTX 4090 | 24 GB | Workstation | 13B | 70B |
| RTX 6000 Ada | 48 GB | Professional | 30B | 120B |
| A40 | 48 GB | Data Center | 30B | 120B |
| A100 40GB | 40 GB | Enterprise | 20B | 80B |
| A100 80GB | 80 GB | Enterprise | 50B | 180B |
| H100 | 80 GB | Cutting Edge | 50B+ | 180B+ |

## Hardware-Specific Configurations

### Consumer GPUs

#### RTX 4090 (24GB) - Optimal Settings

```yaml
# File: config/gpu_vram_configs/rtx4090_24gb.yaml

# Use Case: Development + Small Production
model: "Llama-2-7B"  # or Llama-2-13B with Q5

optimization:
  quantization: "FP16"  # Best quality for 7B
  batch_size: 8         # Sweet spot for throughput
  max_seq_length: 4096  # Standard context
  
  # Memory optimizations
  enable_flash_attention: true     # 2x faster attention
  enable_paged_kv_cache: true      # Reduce fragmentation
  enable_prefix_caching: true      # Share prompts
  kv_cache_block_size: 16          # Optimal block size
  
performance:
  expected_throughput: "320-380 tok/s"
  expected_latency: "22-25 ms/token"
  first_token_latency: "50-80 ms"
```

**Tuning Tips:**
- **Batch Size:** Start at 8, increase to 16 if memory allows
- **Context Length:** 4096 standard, can push to 8192 with batch_size=4
- **Quantization:** FP16 for quality, Q5 for 70B models (lower quality)
- **LoRA Adapters:** Can load 10-15 simultaneously with 8MB each

#### RTX 4060 Ti (16GB) - Budget Configuration

```yaml
model: "Llama-2-7B"

optimization:
  quantization: "Q5_K_M"  # Necessary for limited VRAM
  batch_size: 4           # Conservative
  max_seq_length: 2048    # Reduced context
  
  enable_flash_attention: true
  enable_paged_kv_cache: true
  enable_prefix_caching: true
```

**Tuning Tips:**
- **Model Size:** Stick to 7B models, Q4 quantization for 13B
- **Batch Size:** Keep at 4, max 8 with reduced context
- **Memory Trade-off:** Quality vs capacity - use Q5 for best balance

### Enterprise GPUs

#### A100 80GB - Production Configuration

```yaml
# File: config/gpu_vram_configs/a100_80gb.yaml

model: "Llama-2-70B"

inference:
  batch_size: 32          # High throughput
  max_seq_length: 8192    # Extended context
  
optimization:
  quantization: "FP16"    # Full precision
  enable_flash_attention: true
  enable_paged_kv_cache: true
  enable_prefix_caching: true
  continuous_batching: true  # Dynamic batching
  
vram_allocation:
  model_weights: "28 GB"
  kv_cache_static: "32 GB"
  kv_cache_dynamic: "8 GB"
  activations: "8 GB"
  overhead: "4 GB"
  
performance:
  expected_throughput: "800-1200 tok/s"
  expected_latency: "18-22 ms/token"
  max_concurrent_requests: 64
```

**Tuning Tips:**
- **Batch Size:** Scale from 32 to 64 for maximum throughput
- **Context Length:** Can handle 16K context with batch_size=16
- **Multi-GPU:** Use 2x A100 for Llama-405B (Q4 quantization)
- **NVLink:** Enable for multi-GPU with 600 GB/s bandwidth

### Multi-GPU Configurations

#### 2x RTX 4090 - Tensor Parallelism

```yaml
model: "Llama-2-70B"

multi_gpu:
  enabled: true
  devices: [0, 1]
  strategy: "tensor_parallel"
  
  tensor_parallel:
    shards: 2
    enable_peer_to_peer: true
    
optimization:
  quantization: "FP16"  # 35GB per GPU
  batch_size: 12
  max_seq_length: 4096
  
distribution:
  gpu0_allocation: "22 GB"  # Model shard + KV cache
  gpu1_allocation: "22 GB"
```

**Tuning Tips:**
- **P2P Performance:** Ensure GPUs on same PCIe switch
- **Batch Size:** 12-16 optimal to amortize communication
- **Load Balance:** Monitor per-GPU utilization, adjust sharding if needed
- **Alternative:** Use Q4 quantization (18GB per GPU) for more headroom

#### 4x A100 - Pipeline Parallelism

```yaml
model: "Llama-2-70B"

multi_gpu:
  enabled: true
  devices: [0, 1, 2, 3]
  strategy: "pipeline_parallel"
  
  pipeline_parallel:
    stages: 4
    micro_batch_size: 8
    
optimization:
  batch_size: 32  # 8 micro-batches × 4 stages
  max_seq_length: 8192
  nvlink_enabled: true
```

**Tuning Tips:**
- **Pipeline Depth:** Balance latency vs throughput
- **Micro-batching:** Smaller micro-batches reduce bubble time
- **NVLink:** Critical for pipeline - 600 GB/s vs 64 GB/s PCIe

## Performance Tuning Patterns

### Pattern 1: Maximize Throughput

**Goal:** Maximum tokens/second regardless of latency

```yaml
optimization:
  batch_size: 32          # Large batch
  enable_continuous_batching: true
  prefill_chunking: true
  dynamic_split_fuse: true
  
  # Aggressive caching
  enable_prefix_caching: true
  prefix_cache_size_gb: 8
```

**Expected:** 5-10x throughput increase vs batch_size=1

### Pattern 2: Minimize Latency

**Goal:** Fastest time-to-first-token

```yaml
optimization:
  batch_size: 1           # Single request
  enable_speculative_decoding: true
  kv_cache_prealloc: true
  
  # Reduce overhead
  skip_special_tokens: true
  early_stopping: true
```

**Expected:** 10-30ms first token latency

### Pattern 3: Memory Optimization

**Goal:** Fit largest model possible

```yaml
optimization:
  quantization: "Q4"      # 87.5% reduction
  enable_paged_kv_cache: true
  enable_prefix_caching: true
  cpu_offload_enabled: true  # Spill to RAM if needed
  
  # Conservative allocation
  kv_cache_growth_factor: 0.1  # 10% vs 20% default
```

**Expected:** Fit 4x larger model with 5% quality loss

### Pattern 4: Quality Focus

**Goal:** Best possible output quality

```yaml
optimization:
  quantization: "FP16"    # No quantization loss
  batch_size: 1           # No batching artifacts
  temperature: 0.7        # Optimal sampling
  
  # Full precision inference
  mixed_precision: false
```

**Expected:** 99.9%+ quality vs FP32 training

## Context Length Scaling

### Memory Requirements by Context Length

| Context | Batch 1 | Batch 8 | Batch 32 |
|---------|---------|---------|----------|
| 2K | 0.25 GB | 2 GB | 8 GB |
| 4K | 0.5 GB | 4 GB | 16 GB |
| 8K | 1 GB | 8 GB | 32 GB |
| 16K | 2 GB | 16 GB | 64 GB |
| 32K | 4 GB | 32 GB | 128 GB |

**Tuning Formula:**
```python
kv_cache_gb = context_length * batch_size * kv_bytes_per_token / (1024**3)

# Example (Llama-2-7B, FP16):
kv_bytes_per_token = 2 * 32 * 8 * 128 * 2 = 131,072 bytes ≈ 128 KB
kv_cache_8k_batch8 = 8192 * 8 * 128KB / (1024**3) ≈ 8 GB
```

### Dynamic Context Allocation

```cpp
// Allocate based on actual usage
AdaptiveVRAMAllocator::InferenceConfig config;
config.max_seq_length = 8192;  // Maximum
config.kv_cache_growth_factor = 0.3;  // Allow 30% growth

// Will only allocate as needed, not upfront
auto plan = allocator.calculateOptimalAllocation(model, hw, config);
```

## Batch Size Optimization

### Throughput vs Latency Trade-off

| Batch Size | Throughput (tok/s) | Latency (ms/tok) | VRAM (GB) |
|------------|-------------------|------------------|-----------|
| 1 | 45 | 22 | 16 |
| 4 | 160 | 25 | 18 |
| 8 | 320 | 25 | 20 |
| 16 | 580 | 28 | 23 |
| 32 | 960 | 33 | OOM |

**Optimal Batch Size:**
- **Interactive:** 1-4 (low latency)
- **Bulk Processing:** 16-32 (high throughput)
- **Balanced:** 8 (good throughput, acceptable latency)

### Dynamic Batching

```yaml
optimization:
  continuous_batching: true
  max_batch_size: 16
  batch_timeout_ms: 50  # Wait up to 50ms to fill batch
  
  # Batch scheduling
  priority_based: true
  fair_scheduling: true
```

**Benefits:**
- Automatically groups requests
- Maintains low latency for single requests
- Maximizes throughput when traffic is high

## Quantization Decision Tree

```
Start: What's your constraint?
│
├─ Memory: Use highest quantization that fits
│  ├─ 24GB GPU, 7B model? → FP16 (14GB)
│  ├─ 24GB GPU, 70B model? → Q4 (35GB won't fit)
│  └─ 80GB GPU, 70B model? → FP16 (140GB won't fit, use 2x GPU or Q4)
│
├─ Quality: Use lowest quantization acceptable
│  ├─ <1% loss acceptable? → FP16
│  ├─ <2% loss acceptable? → INT8
│  └─ <5% loss acceptable? → Q4
│
└─ Speed: Balance compression vs throughput
   ├─ CPU-bound? → Q4 (smaller transfers)
   ├─ Memory-bound? → FP16 (less overhead)
   └─ Balanced? → INT8 (good middle ground)
```

## Monitoring and Diagnostics

### Key Metrics to Track

```cpp
// Memory statistics
auto stats = cache_mgr.getMemoryStats();
std::cout << "Used blocks: " << stats.used_blocks << "/" << stats.total_blocks << "\n";
std::cout << "Fragmentation: " << (stats.fragmentation_rate * 100) << "%\n";
std::cout << "Prefix savings: " << (stats.prefix_sharing_ratio * 100) << "%\n";

// GPU health
auto health = gpu_mgr.getGPUHealth(0);
std::cout << "Temperature: " << health.temperature_celsius << "°C\n";
std::cout << "Utilization: " << health.utilization_percent << "%\n";
```

### Warning Thresholds

| Metric | Warning | Critical |
|--------|---------|----------|
| VRAM Usage | >85% | >95% |
| Fragmentation | >15% | >30% |
| Temperature | >75°C | >85°C |
| Utilization | >90% | >98% |

### Auto-tuning Script

```python
#!/usr/bin/env python3
# scripts/tune_vram_config.py

def find_optimal_batch_size(gpu_vram_gb, model_size_gb, context_length):
    """Find maximum batch size that fits in VRAM"""
    available = gpu_vram_gb - model_size_gb - 2  # 2GB reserve
    
    kv_cache_per_batch = context_length * 128 / 1024  # KB -> MB -> GB
    kv_cache_per_batch_gb = kv_cache_per_batch / 1024
    
    max_batch = int(available / kv_cache_per_batch_gb)
    return max(1, max_batch)

# Example
optimal_batch = find_optimal_batch_size(
    gpu_vram_gb=24,
    model_size_gb=14,  # Llama-2-7B FP16
    context_length=4096
)
print(f"Optimal batch size: {optimal_batch}")  # Output: 8
```

## Configuration Examples

### Example 1: Cost-Optimized (RTX 4060 Ti)

```yaml
hardware:
  gpu_model: "RTX 4060 Ti"
  vram_gb: 16
  
model: "Llama-2-7B"

optimization:
  quantization: "Q5_K_M"  # 9GB model
  batch_size: 4
  max_seq_length: 2048
  
  # Aggressive memory saving
  enable_paged_kv_cache: true
  enable_prefix_caching: true
  cpu_offload_threshold: 0.9
  
cost:
  hardware: "$500"
  power: "160W"
  cost_per_1m_tokens: "$0.50"
```

### Example 2: Balanced (RTX 4090)

```yaml
hardware:
  gpu_model: "RTX 4090"
  vram_gb: 24
  
model: "Llama-2-13B"

optimization:
  quantization: "FP16"  # 26GB with optimizations
  batch_size: 8
  max_seq_length: 4096
  
  # Standard optimizations
  enable_flash_attention: true
  enable_paged_kv_cache: true
  enable_prefix_caching: true
  
performance:
  throughput: "240-320 tok/s"
  latency: "25-30 ms/tok"
```

### Example 3: High-Performance (4x A100)

```yaml
hardware:
  gpus: ["A100 80GB", "A100 80GB", "A100 80GB", "A100 80GB"]
  total_vram_gb: 320
  
model: "Llama-2-70B"

multi_gpu:
  strategy: "tensor_parallel"
  shards: 4
  nvlink: true
  
optimization:
  quantization: "FP16"
  batch_size: 64
  max_seq_length: 8192
  
performance:
  throughput: "3000+ tok/s"
  latency: "<5 ms/tok"
  concurrent_requests: 128
```

## Troubleshooting Scenarios

### Scenario 1: OOM During Inference

**Symptoms:** CUDA out of memory mid-batch

**Diagnosis:**
```cpp
auto plan = allocator.calculateOptimalAllocation(model, hw, config);
if (!plan.fits_in_vram) {
    std::cout << "Required: " << (plan.total / 1e9) << " GB\n";
    std::cout << "Available: " << (hw.available_vram_bytes / 1e9) << " GB\n";
    std::cout << plan.recommendation << "\n";
}
```

**Solutions:**
1. Reduce batch size by 50%
2. Switch to INT8 quantization
3. Enable CPU offloading
4. Add second GPU

### Scenario 2: Low Throughput

**Symptoms:** 10x slower than expected

**Diagnosis:**
```cpp
auto stats = gpu_mgr.getStats();
if (stats.utilization_percent < 50) {
    // GPU is idle - CPU bottleneck
} else if (stats.fragmentation_pct > 20) {
    // Memory fragmentation
}
```

**Solutions:**
1. Increase batch size
2. Enable continuous batching
3. Defragment memory
4. Check for CPU bottlenecks

### Scenario 3: Quality Degradation

**Symptoms:** Poor output quality

**Diagnosis:**
- Check quantization level
- Verify model loaded correctly
- Compare with FP16 baseline

**Solutions:**
1. Use higher precision (Q4 → INT8 → FP16)
2. Verify quantization calibration
3. Check for corrupted weights

## Best Practices Checklist

- [ ] Use FP16 for production inference (best quality/speed)
- [ ] Enable PagedAttention to reduce fragmentation
- [ ] Enable prefix caching for shared prompts (30-50% savings)
- [ ] Set batch_size to 8-16 for good throughput
- [ ] Monitor VRAM usage and stay below 90%
- [ ] Use multi-GPU for models >50B parameters
- [ ] Enable Flash Attention for 2x speedup
- [ ] Reserve 10% VRAM headroom for safety
- [ ] Defragment memory periodically
- [ ] Profile and tune for your specific workload

---

**Next:** See [GPU_MEMORY_BEST_PRACTICES.md](GPU_MEMORY_BEST_PRACTICES.md) for advanced patterns
