# GPU Memory Best Practices

## Do's and Don'ts

### ✅ DO

**Memory Management**
- ✅ Reserve 10% VRAM as safety margin
- ✅ Enable PagedAttention for KV-cache management
- ✅ Use prefix caching for shared prompts (30-50% savings)
- ✅ Monitor fragmentation and defragment when >15%
- ✅ Implement OOM recovery with CPU offloading

**Quantization**
- ✅ Use FP16 as default for production
- ✅ Profile INT8 vs FP16 on your specific tasks
- ✅ Test quality before deploying quantized models
- ✅ Document accuracy loss in production configs

**Multi-GPU**
- ✅ Use tensor parallelism for memory-bound models
- ✅ Enable P2P/NVLink when available
- ✅ Balance load based on GPU capacity
- ✅ Monitor per-GPU utilization

**Performance**
- ✅ Batch requests for higher throughput (8-16 optimal)
- ✅ Enable Flash Attention for 2x speedup
- ✅ Use continuous batching for variable load
- ✅ Profile before optimizing

### ❌ DON'T

**Memory Management**
- ❌ Don't allocate 100% of VRAM (leave 10% headroom)
- ❌ Don't ignore fragmentation warnings
- ❌ Don't mix models without checking compatibility
- ❌ Don't skip VRAM calculations before deployment

**Quantization**
- ❌ Don't use FP32 for inference (2x memory, no benefit)
- ❌ Don't use Q4 without quality testing
- ❌ Don't assume quantization has no impact
- ❌ Don't quantize without calibration data

**Multi-GPU**
- ❌ Don't use multi-GPU if single GPU fits
- ❌ Don't ignore inter-GPU communication costs
- ❌ Don't balance load equally across asymmetric GPUs
- ❌ Don't use pipeline parallelism with small batches

**Performance**
- ❌ Don't use batch_size=1 for production serving
- ❌ Don't over-provision context length
- ❌ Don't skip benchmarking on target hardware
- ❌ Don't optimize prematurely

## Common Pitfalls

### Pitfall 1: Over-allocating Context Length

**Problem:** Setting `max_seq_length=32768` when most requests use <4096

**Impact:**
- 8x memory waste
- Reduced batch size
- Lower throughput

**Solution:**
```yaml
# Bad
max_seq_length: 32768  # "Just in case"

# Good
max_seq_length: 4096   # 95th percentile of actual usage
context_expansion_enabled: true  # Dynamic for rare long contexts
```

### Pitfall 2: Ignoring Fragmentation

**Problem:** Running service for days without monitoring fragmentation

**Impact:**
- Gradual memory consumption increase
- Mysterious OOM errors
- Performance degradation

**Solution:**
```cpp
// Monitor and defragment
auto stats = cache_mgr.getMemoryStats();
if (stats.fragmentation_rate > 0.15) {  // >15%
    LOG(WARNING) << "High fragmentation: " << stats.fragmentation_rate;
    cache_mgr.defragment();
}
```

### Pitfall 3: Wrong Multi-GPU Strategy

**Problem:** Using pipeline parallelism with batch_size=1

**Impact:**
- Pipeline bubbles waste 75% of compute
- 4x GPUs → 1x performance

**Solution:**
```yaml
# For small batches: Use tensor parallelism
multi_gpu:
  strategy: "tensor_parallel"  # Better for small batches
  
# For large batches: Pipeline is OK
multi_gpu:
  strategy: "pipeline_parallel"
  micro_batch_size: 8  # Keep pipeline full
```

### Pitfall 4: Quantization Without Testing

**Problem:** Deploying Q4 model without quality verification

**Impact:**
- Silent quality degradation
- User complaints
- Reputational damage

**Solution:**
```python
# Always test before production
test_set = load_benchmark()
fp16_scores = evaluate(model_fp16, test_set)
q4_scores = evaluate(model_q4, test_set)

accuracy_loss = (fp16_scores - q4_scores) / fp16_scores
assert accuracy_loss < 0.05, f"Quality loss too high: {accuracy_loss}"
```

## Real-World Case Studies

### Case Study 1: Reducing OOM Errors by 95%

**Scenario:** RAG application with variable-length documents

**Initial Config:**
```yaml
max_seq_length: 8192  # Fixed allocation
batch_size: 16
enable_paged_kv_cache: false
```

**Problems:**
- OOM when documents exceeded 4096 tokens
- Fixed allocation wasted memory on short docs
- Only handled batch_size=8 reliably

**Solution:**
```yaml
max_seq_length: 16384  # Higher max
batch_size: 32         # Higher batch
enable_paged_kv_cache: true      # Dynamic allocation
enable_prefix_caching: true      # Share document prefixes
kv_cache_growth_factor: 0.3      # Allow growth
```

**Results:**
- OOM errors: 50/day → 2/day (95% reduction)
- Memory utilization: 85% → 92%
- Throughput: 2.3x improvement

### Case Study 2: Multi-GPU Optimization

**Scenario:** Llama-70B on 2x RTX 4090

**Initial Config:**
```yaml
multi_gpu:
  strategy: "pipeline_parallel"  # Wrong choice
  batch_size: 4
```

**Problems:**
- Pipeline bubbles: 60% idle time
- Throughput: 80 tok/s (expected 300)
- P2P not enabled: CPU bottleneck

**Solution:**
```yaml
multi_gpu:
  strategy: "tensor_parallel"  # Better for memory-bound
  batch_size: 12               # Higher batch
  enable_peer_to_peer: true    # Direct GPU transfers
  
optimization:
  enable_flash_attention: true
  continuous_batching: true
```

**Results:**
- Throughput: 80 → 420 tok/s (5.25x)
- Latency: 50ms → 28ms
- GPU utilization: 40% → 85%

### Case Study 3: Quality vs Memory Trade-off

**Scenario:** Deploying Llama-13B on 16GB GPU (RTX 4060 Ti)

**Initial Attempt:**
```yaml
model: "Llama-13B"
quantization: "Q4"  # Only way to fit
```

**Problems:**
- Quality loss: 8% on benchmarks
- Hallucinations increased
- User satisfaction dropped

**Solution:**
```yaml
model: "Llama-7B"   # Smaller model
quantization: "FP16"  # Full quality
batch_size: 8         # Better throughput
enable_prefix_caching: true
```

**Results:**
- Quality: Q4 13B (92%) → FP16 7B (99%)
- User satisfaction: 78% → 94%
- Throughput: Similar (better batching compensated)

**Lesson:** Smaller high-quality model > larger low-quality model

## Advanced Patterns

### Pattern 1: Hybrid CPU-GPU Offloading

**When to Use:** Model barely fits in VRAM

```cpp
AdaptiveVRAMAllocator::Config config;
config.enable_cpu_offload = true;
config.offload_threshold = 0.95;  // Offload at 95% VRAM usage

// Keep hot layers on GPU, cold layers on CPU
std::vector<int> gpu_layers = {0, 1, 2, 30, 31};  // First/last layers hot
std::vector<int> cpu_layers = {3, 4, 5, ..., 29};  // Middle layers cold
```

**Benefits:**
- Fit larger models
- Maintain low latency on hot path
- Graceful degradation under memory pressure

### Pattern 2: Dynamic Batch Size Adjustment

**When to Use:** Variable request load

```cpp
class DynamicBatcher {
    size_t current_batch_size = 8;
    
    void adjust() {
        auto stats = gpu_mgr.getStats();
        
        if (stats.used_vram_bytes < stats.total_vram_bytes * 0.7) {
            current_batch_size = std::min(current_batch_size * 2, max_batch_size);
        } else if (stats.used_vram_bytes > stats.total_vram_bytes * 0.9) {
            current_batch_size = std::max(current_batch_size / 2, min_batch_size);
        }
    }
};
```

**Benefits:**
- Maximize throughput when memory available
- Prevent OOM under load
- Adapt to workload changes

### Pattern 3: Tiered Model Serving

**When to Use:** Different quality requirements per user/tier

```yaml
models:
  - name: "premium"
    model: "Llama-70B"
    quantization: "FP16"
    gpu_ids: [0, 1]  # Multi-GPU
    max_users: 100
    
  - name: "standard"
    model: "Llama-13B"
    quantization: "FP16"
    gpu_ids: [2]
    max_users: 500
    
  - name: "basic"
    model: "Llama-7B"
    quantization: "INT8"
    gpu_ids: [3]
    max_users: 2000
```

**Benefits:**
- Resource allocation matches value
- Prevent resource contention
- Clear capacity planning

### Pattern 4: Prefix Caching for RAG

**When to Use:** Document-based Q&A, retrieval-augmented generation

```cpp
// Cache document prefixes
PagedKVCacheManager cache_mgr(config);

// First query on document
uint64_t doc_seq_id = hash(document);
cache_mgr.addSequence(doc_seq_id, document_tokens);

// Subsequent queries share prefix
for (const auto& query : queries) {
    uint64_t query_seq_id = hash(document + query);
    cache_mgr.enablePrefixCaching(query_seq_id, doc_seq_id, document_tokens);
    // Only allocate new blocks for query-specific tokens
}
```

**Benefits:**
- 50-70% memory savings on repeated documents
- Faster inference (prefix pre-computed)
- Higher throughput

## Monitoring and Alerting

### Critical Metrics

```cpp
// Metric 1: VRAM Utilization
float vram_utilization = stats.used_vram_bytes / stats.total_vram_bytes;
if (vram_utilization > 0.90) {
    ALERT("VRAM utilization high: " << vram_utilization);
}

// Metric 2: Fragmentation
if (stats.fragmentation_pct > 15) {
    WARNING("Fragmentation high: " << stats.fragmentation_pct);
    cache_mgr.defragment();
}

// Metric 3: GPU Temperature
if (gpu_health.temperature_celsius > 80) {
    WARNING("GPU temperature high: " << gpu_health.temperature_celsius);
}

// Metric 4: OOM Rate
float oom_rate = oom_errors_last_hour / total_requests_last_hour;
if (oom_rate > 0.01) {  // 1% OOM rate
    ALERT("High OOM rate: " << oom_rate);
}
```

### Prometheus Metrics

```cpp
// Export metrics for Grafana/Prometheus
DEFINE_gauge(gpu_vram_used_bytes, "GPU VRAM used in bytes");
DEFINE_gauge(gpu_vram_total_bytes, "GPU VRAM total in bytes");
DEFINE_gauge(gpu_fragmentation_percent, "GPU memory fragmentation %");
DEFINE_counter(gpu_oom_errors_total, "Total GPU OOM errors");
DEFINE_histogram(gpu_allocation_latency_ms, "GPU allocation latency in ms");

// Update metrics
gpu_vram_used_bytes.Set(stats.used_vram_bytes);
gpu_fragmentation_percent.Set(stats.fragmentation_pct);
```

## Testing and Validation

### Unit Tests

```cpp
TEST(VRAMAllocation, CalculateOptimalAllocation_RTX4090) {
    AdaptiveVRAMAllocator allocator;
    
    // Configure 7B model on RTX 4090
    auto model = createLlama7BConfig();
    auto hw = createRTX4090Hardware();
    auto config = createInferenceConfig(8, 4096);
    
    auto plan = allocator.calculateOptimalAllocation(model, hw, config);
    
    EXPECT_TRUE(plan.fits_in_vram);
    EXPECT_LE(plan.total, hw.available_vram_bytes);
    EXPECT_GE(plan.model_weights, 13ULL * 1024 * 1024 * 1024);  // ~14 GB
}
```

### Integration Tests

```cpp
TEST(VRAMAllocation, MultiGPUDistribution) {
    MultiGPUMemoryCoordinator coordinator;
    coordinator.initialize({0, 1});
    
    size_t model_size = 140ULL * 1024 * 1024 * 1024;  // 140 GB
    auto plan = coordinator.distributeModelWeights({0, 1}, model_size);
    
    EXPECT_EQ(plan.tensor_parallel_size, 2);
    EXPECT_EQ(plan.shard_sizes.size(), 2);
    EXPECT_NEAR(plan.shard_sizes[0], model_size / 2, 1e9);
}
```

### Benchmarks

```cpp
BENCHMARK_F(VRAMBench, ModelLoading_7B_FP16)(benchmark::State& state) {
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        void* ptr = gpu_mgr->allocateGPU("llama-7b", 14ULL * 1024 * 1024 * 1024);
        auto end = std::chrono::high_resolution_clock::now();
        
        state.SetIterationTime(std::chrono::duration<double>(end - start).count());
        gpu_mgr->freeGPU("llama-7b", ptr);
    }
}
```

## Deployment Checklist

### Pre-Production

- [ ] Profile model on target hardware
- [ ] Calculate VRAM requirements with 10% buffer
- [ ] Test OOM recovery mechanisms
- [ ] Benchmark throughput/latency
- [ ] Validate quantization quality (if used)
- [ ] Test multi-GPU coordination (if applicable)
- [ ] Set up monitoring and alerting
- [ ] Document configuration decisions

### Production

- [ ] Monitor VRAM utilization (alert >90%)
- [ ] Monitor fragmentation (alert >15%)
- [ ] Monitor OOM rate (alert >1%)
- [ ] Monitor GPU temperature (alert >80°C)
- [ ] Track inference latency P50/P95/P99
- [ ] Track throughput (tokens/second)
- [ ] Log memory statistics hourly
- [ ] Review metrics weekly

### Post-Deployment

- [ ] Analyze actual vs expected performance
- [ ] Tune batch size based on traffic patterns
- [ ] Adjust context length limits if needed
- [ ] Optimize quantization settings
- [ ] Update capacity planning
- [ ] Document lessons learned

## Resources

### Documentation
- [GPU_VRAM_ALLOCATION_GUIDE.md](GPU_VRAM_ALLOCATION_GUIDE.md) - Architecture and API
- [VRAM_CONFIGURATION_TUNING.md](VRAM_CONFIGURATION_TUNING.md) - Hardware-specific tuning
- [VRAM_ALLOCATION_BEST_PRACTICES.md](VRAM_ALLOCATION_BEST_PRACTICES.md) - Existing best practices

### Configuration Templates
- `config/gpu_vram_configs/rtx4090_24gb.yaml` - Consumer GPU
- `config/gpu_vram_configs/a100_80gb.yaml` - Enterprise GPU
- `config/gpu_vram_configs/multi_gpu_hybrid.yaml` - Multi-GPU setup

### Code Examples
- `examples/llm/adaptive_vram_example.cpp` - Allocation examples
- `tests/test_gpu_vram_allocation.cpp` - Unit tests
- `benchmarks/bench_gpu_vram_allocation.cpp` - Performance benchmarks

---

**For questions:** [ThemisDB GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
