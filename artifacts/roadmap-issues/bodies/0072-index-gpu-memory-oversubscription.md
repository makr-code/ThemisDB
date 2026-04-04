### Context

This issue implements the roadmap item 'GPU Memory Oversubscription' for the index domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: GPU Memory Oversubscription

### Goal

Deliver the scoped changes for GPU Memory Oversubscription in src/index/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### GPU Memory Oversubscription
**Priority:** High  
**Target Version:** v1.6.0

Support datasets larger than GPU VRAM via paging and streaming.

**Features:**
- **Unified Memory**: CUDA Unified Memory for automatic paging
- **Streaming**: Load index chunks from host RAM as needed
- **LRU Eviction**: Keep hot partitions in VRAM, evict cold
- **Prefetching**: Predict next access patterns, prefetch to GPU
- **Multi-GPU**: Distribute index across multiple GPUs

**Configuration:**
```cpp
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;
config.enable_oversubscription = true;
config.vram_budget_mb = 8192;  // 8GB VRAM limit
config.prefetch_strategy = PrefetchStrategy::LRU;

// Works with 50M vectors (200GB) on 8GB GPU
auto gpu_index = std::make_unique<GPUVectorIndex>(config);
```

**Performance:**
- Hot data: Full GPU speed (200K queries/sec)
- Cold data: CPU speed with PCIe overhead (10K queries/sec)
- Prefetch hit rate: 80-90% (workload-dependent)

---

### Acceptance Criteria

- [ ] **Unified Memory**: CUDA Unified Memory for automatic paging
- [ ] **Streaming**: Load index chunks from host RAM as needed
- [ ] **LRU Eviction**: Keep hot partitions in VRAM, evict cold
- [ ] **Prefetching**: Predict next access patterns, prefetch to GPU
- [ ] **Multi-GPU**: Distribute index across multiple GPUs
- [ ] Hot data: Full GPU speed (200K queries/sec)
- [ ] Cold data: CPU speed with PCIe overhead (10K queries/sec)
- [ ] Prefetch hit rate: 80-90% (workload-dependent)

### Relationships

- Roadmap row: #72 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/index/FUTURE_ENHANCEMENTS.md#gpu-memory-oversubscription
- Source key: roadmap:72:index:v1.7.0:gpu-memory-oversubscription

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:72:index:v1.7.0:gpu-memory-oversubscription -->
<!-- roadmap-ref: row=72;module=index;target=v1.7.0 -->
<!-- roadmap-detail: src/index/FUTURE_ENHANCEMENTS.md#gpu-memory-oversubscription -->
