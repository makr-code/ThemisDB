# ThemisDB Vector Indexing Architecture

**Status**: Production-ready with FAISS integration ✅  
**Last Updated**: 2026-04-06

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      Client Application                          │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                     VectorIndexManager                           │
│                  (Entry Point & Orchestrator)                    │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             │ Configuration Check
                             ▼
                    ┌────────────────┐
                    │ advanced_index │
                    │   _enabled?    │
                    └────────┬───────┘
                             │
              ┌──────────────┴──────────────┐
              │ YES                         │ NO
              ▼                             ▼
┌─────────────────────────┐    ┌───────────────────────┐
│  AdvancedVectorIndex    │    │   Fallback Path       │
│   (FAISS NATIVE) ✅     │    │   (Legacy/Research)   │
│  PRIMARY PRODUCTION     │    │                       │
└─────────┬───────────────┘    └───────┬───────────────┘
          │                             │
          │                             │
          ▼                             ▼
┌─────────────────────────┐    ┌───────────────────────┐
│   FAISS GPU Backend     │    │  HNSW (hnswlib)       │
│   THEMIS_GPU_ENABLED    │    │  or                   │
│                         │    │  ProductQuantizer     │
│  • IVF+PQ (10-100x)    │    │  (custom fallback)    │
│  • IVF+Flat            │    │                       │
│  • HNSW+Flat           │    │                       │
│  • GPU: NVIDIA/AMD     │    │                       │
└─────────┬───────────────┘    └───────────────────────┘
          │
          │ Graceful Degradation
          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Degradation Chain                             │
│                                                                  │
│  GPU Available? → FAISS GPU (fastest)                          │
│       ↓ NO                                                      │
│  FAISS CPU → FAISS CPU (fast, optimized)                      │
│       ↓ NO                                                      │
│  HNSW Available? → HNSW (hnswlib, good)                       │
│       ↓ NO                                                      │
│  Custom Fallback → ProductQuantizer (basic)                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Component Status

### ✅ Production Components (FAISS Native)

| Component | Technology | Purpose | Status |
|-----------|-----------|---------|--------|
| **AdvancedVectorIndex** | FAISS IVF+PQ/HNSW | Primary production vector index | ✅ Active |
| **FAISS GPU Backend** | FAISS GPU | GPU acceleration (NVIDIA/AMD) | ✅ Active |
| **MultiGPUVectorIndex** | NCCL/RCCL | Multi-GPU vector indexing (v2.5+) | ✅ Active |
| **NCCL Backend** | NVIDIA NCCL | NVIDIA multi-GPU collectives | ✅ Active |
| **RCCL Backend** | AMD RCCL | AMD multi-GPU collectives | ✅ Active |
| **Build System** | CMake | Auto-detect FAISS, set THEMIS_GPU_ENABLED | ✅ Active |

### 🔄 Fallback Components

| Component | Technology | Purpose | Status |
|-----------|-----------|---------|--------|
| **HNSW Index** | hnswlib | Fallback when FAISS not available | ✅ Active |
| **ProductQuantizer** | Custom | Fallback quantization for non-FAISS paths | ✅ Active |

### ⚠️ Deprecated/Research Components

| Component | Technology | Purpose | Status |
|-----------|-----------|---------|--------|
| **BinaryQuantizer** | Custom | Binary hashing research | ⚠️ Deprecated |
| **LearnedQuantizer** | Custom | Learned quantization research | ⚠️ Deprecated |
| **ResidualQuantizer** | Custom | Multi-stage quantization research | 🔬 Research |

---

## Multi-GPU Architecture (v2.5+)

### Communication Backends

ThemisDB v2.5+ supports multi-GPU vector indexing with two communication backends:

**NCCL (NVIDIA GPUs)**:
- NVIDIA Collective Communications Library
- Optimized for NVIDIA GPUs with NVLink support
- Provides AllReduce, Broadcast, P2P transfers
- 25-50 GB/s inter-GPU bandwidth with NVLink
- Auto-detected for NVIDIA hardware

**RCCL (AMD GPUs)**:
- ROCm Communication Collectives Library
- Optimized for AMD GPUs with Infinity Fabric (XGMI)
- Provides AllReduce, Broadcast, P2P transfers
- 200 GB/s inter-GPU bandwidth with XGMI
- Auto-detected for AMD hardware

### Multi-GPU Features

- **Data Partitioning**: Distribute vectors across GPUs (round-robin, hash-based, range-based, balanced)
- **Query Fan-out**: Parallel query execution across all GPUs
- **Collective Top-K Merge**: Efficient result aggregation using NCCL/RCCL AllReduce
- **P2P Transfers**: Direct GPU-to-GPU data movement (no CPU involvement)
- **Fault Tolerance**: Graceful degradation when GPUs fail
- **Load Balancing**: Dynamic workload distribution

### Configuration Example

```cpp
#include "index/multi_gpu_vector_index.h"

MultiGPUVectorIndex::Config config;
config.enableMultiGPU = true;
config.deviceIds = {0, 1, 2, 3};  // Use 4 GPUs
config.commBackend = MultiGPUVectorIndex::CommBackend::AUTO;  // NCCL or RCCL
config.enableP2P = true;
config.enableNVLink = true;   // For NVIDIA
config.enableXGMI = true;     // For AMD
config.partitionStrategy = MultiGPUVectorIndex::PartitionStrategy::BALANCED;

MultiGPUVectorIndex index(config);
index.initialize(128);  // 128-dimensional vectors
```

See `docs/NCCL_RCCL_INTEGRATION_GUIDE.md` for complete usage guide.

---

## Production Vector Search Flow

```
1. User Query: search(vector, k=10)
   │
   ▼
2. VectorIndexManager receives request
   │
   ▼
3. Route to index:
   │
   ├─ advanced_index_enabled=true
   │  └─ AdvancedVectorIndex.search() ✅ PRIMARY PATH
   │     │
   │     ├─ FAISS IVF+PQ.search()
   │     │  • Quantized search (10-100x compression)
   │     │  • GPU acceleration (if available)
   │     │  • SIMD optimizations
   │     │
   │     ├─ FAISS IVF+Flat.search()
   │     │  • Uncompressed, faster
   │     │  • More memory usage
   │     │
   │     └─ FAISS HNSW+Flat.search()
   │        • Best accuracy
   │        • Graph-based search
   │
   └─ advanced_index_enabled=false
      └─ HNSW (hnswlib) or BruteForce
         • Fallback for compatibility
         • ProductQuantizer if enabled

4. Return results: [(id, distance), ...]
```

---

## Performance Characteristics

### FAISS IVF+PQ (Production Default)

```
Dataset Size: 1M vectors (1536 dimensions)
Configuration: IVF=1024, PQ=8x8, GPU=NVIDIA A100

┌──────────────────┬────────────┬─────────────┐
│ Operation        │ Time       │ Notes       │
├──────────────────┼────────────┼─────────────┤
│ Training         │ ~30s       │ One-time    │
│ Indexing         │ ~5s        │ 1M vectors  │
│ Search (k=10)    │ ~2ms       │ Per query   │
│ Memory Usage     │ ~150MB     │ 10x compress│
│ GPU Memory       │ ~200MB     │ If GPU used │
└──────────────────┴────────────┴─────────────┘

Accuracy: ~95% recall@10 (vs brute force)
Throughput: ~500 QPS (single GPU)
```

### FAISS HNSW+Flat (Best Accuracy)

```
Dataset Size: 1M vectors (1536 dimensions)
Configuration: M=32, efConstruction=200

┌──────────────────┬────────────┬─────────────┐
│ Operation        │ Time       │ Notes       │
├──────────────────┼────────────┼─────────────┤
│ Training         │ N/A        │ No training │
│ Indexing         │ ~60s       │ 1M vectors  │
│ Search (k=10)    │ ~5ms       │ Per query   │
│ Memory Usage     │ ~6GB       │ Uncompressed│
└──────────────────┴────────────┴─────────────┘

Accuracy: ~99% recall@10 (vs brute force)
Throughput: ~200 QPS (single CPU core)
```

### Custom ProductQuantizer (Fallback)

```
Dataset Size: 100K vectors (1536 dimensions)
Configuration: 8 subquantizers, 256 centroids

┌──────────────────┬────────────┬─────────────┐
│ Operation        │ Time       │ Notes       │
├──────────────────┼────────────┼─────────────┤
│ Training         │ ~10s       │ K-means     │
│ Encoding         │ ~0.1ms     │ Per vector  │
│ Decoding         │ ~0.05ms    │ Per vector  │
│ Memory Usage     │ ~50MB      │ Codebook    │
└──────────────────┴────────────┴─────────────┘

Compression: ~8x (float32 → 8-bit codes)
Use Case: Standalone encode/decode (non-search)
```

---

## Configuration Examples

### Production: FAISS IVF+PQ with GPU

```cpp
VectorIndexManager::AdvancedIndexConfig config;
config.enabled = true;                    // Enable FAISS
config.index_type = AdvancedIndexConfig::Type::IVF_PQ;
config.nlist = 1024;                     // IVF clusters
config.nprobe = 64;                      // Search clusters
config.use_pq = true;                    // Enable PQ compression
config.pq_m = 8;                         // 8 subquantizers
config.pq_nbits = 8;                     // 8 bits per code
config.use_gpu = true;                   // Enable GPU
config.gpu_device = 0;                   // GPU 0
config.train_size = 100000;              // Training samples

vectorIndexManager.setAdvancedIndexConfig(config);
vectorIndexManager.init(objectName, dimension, metric);
```

### High Accuracy: FAISS HNSW+Flat

```cpp
VectorIndexManager::AdvancedIndexConfig config;
config.enabled = true;
config.index_type = AdvancedIndexConfig::Type::HNSW_FLAT;
config.use_gpu = false;                  // HNSW is CPU-only
// HNSW parameters configured via FAISS defaults

vectorIndexManager.setAdvancedIndexConfig(config);
vectorIndexManager.init(objectName, dimension, metric);
```

### Fallback: HNSW (hnswlib)

```cpp
// Don't call setAdvancedIndexConfig() or set enabled=false
vectorIndexManager.init(objectName, dimension, metric);
// Uses HNSW from hnswlib automatically
```

---

## Build Configuration

### CMake: Detect FAISS

```cmake
# From cmake/CMakeLists.txt
find_package(faiss CONFIG)

if(NOT faiss_FOUND)
    message(WARNING "Faiss not found - GPU support will be automatically disabled.")
    set(THEMIS_ENABLE_GPU OFF CACHE BOOL "Enable GPU acceleration" FORCE)
endif()

# Set flag for conditional compilation
if(THEMIS_ENABLE_GPU)
    target_compile_definitions(themisdb PRIVATE THEMIS_GPU_ENABLED)
endif()
```

### Conditional Compilation

```cpp
// In code
#ifdef THEMIS_GPU_ENABLED
    // Use AdvancedVectorIndex with FAISS
    if (advanced_config_.enabled) {
        advanced_index_ = std::make_unique<AdvancedVectorIndex>(...);
    }
#else
    // Fallback to HNSW
    THEMIS_WARN("Advanced indexing requires FAISS support");
#endif
```

---

## Migration History

### Timeline

- **2026-01**: Initial library usage analysis identified custom quantizers
- **2026-02-02**: BinaryQuantizer simplified (-79 lines), LearnedQuantizer deprecated
- **2026-02-05**: **Migration assessment complete** ✅
  - Documented that AdvancedVectorIndex uses FAISS natively
  - Clarified FAISS is PRIMARY production solution
  - Custom quantizers serve as fallback/research only

### Decision Rationale

**Why keep ProductQuantizer?**
- FAISS IndexIVFPQ doesn't expose standalone encode/decode API
- ThemisDB needs: `encode(vector) → codes` for external use
- Production workloads use AdvancedVectorIndex (FAISS) instead
- ProductQuantizer serves as fallback for compatibility

**Why deprecate BinaryQuantizer & LearnedQuantizer?**
- Never used in production code paths
- Research implementations only
- FAISS provides better alternatives
- Simplified/deprecated to reduce maintenance

**Why keep ResidualQuantizer?**
- Research component for multi-stage quantization
- Not production-critical
- May inform future optimizations


---

## Workload-Specific Optimization

### Overview

ThemisDB provides workload-specific index tuning to optimize performance for different use cases. Each workload type has distinct characteristics and requirements:

| Workload | Characteristics | Optimization Goal |
|----------|----------------|-------------------|
| **OLTP** | High QPS, low latency, small k | Minimize query latency, maximize throughput |
| **Analytics** | Large k, batch queries, complex aggregations | Maximize recall, tolerate higher latency |
| **RAG** | Medium k, high-dimensional embeddings | Balance speed and accuracy for LLM retrieval |
| **Mixed** | Varying query patterns | Balanced configuration |
| **Batch Insert** | Bulk data loading | Optimize construction speed |

### HNSW Workload Tuning

**Parameter Adjustments by Workload:**

```
OLTP Configuration:
├── M: 8-16 (lower for faster writes)
├── ef_construction: 96-192 (faster build)
├── ef_search: 16-128 (lower for speed)
├── target_latency: 5ms
└── target_recall: 90-93%

Analytics Configuration:
├── M: 24-48 (higher for connectivity)
├── ef_construction: 288-600 (quality build)
├── ef_search: 64-512 (higher for recall)
├── target_latency: 50ms
└── target_recall: 97-99%

RAG Configuration:
├── M: 16-32 (balanced)
├── ef_construction: 192-384 (good quality)
├── ef_search: 32-256 (balanced)
├── target_latency: 15ms
└── target_recall: 95-97%
```

**Usage Example:**

```cpp
// OLTP: Real-time product recommendations
auto config = HnswParameterTuner::getWorkloadOptimizedConfig(
    100000, HnswParameterTuner::WorkloadType::OLTP);

HnswParameterTuner tuner(config);
// tuner will automatically adapt ef_search for optimal performance

// Analytics: Batch similarity analysis
auto config = HnswProductionDefaults::getWorkloadOptimizedParams(
    1000000, 768, HnswProductionDefaults::WorkloadType::ANALYTICS);

// RAG: Document retrieval for LLM
auto config = AdvancedVectorIndex::getWorkloadOptimizedConfig(
    500000, 1536, AdvancedVectorIndex::WorkloadType::RAG);
```

### FAISS Workload Tuning

**IVF Configuration by Workload:**

```
OLTP:
├── nlist: dataset_size / 200 (fewer clusters)
├── nprobe: 32 (lower probe)
├── index_type: IVF_FLAT (no compression)
└── Expected: 2-5ms latency, 90-95% recall

Analytics:
├── nlist: dataset_size / 50 (more clusters)
├── nprobe: 128 (higher probe)
├── index_type: IVF_PQ (compression)
└── Expected: 20-50ms latency, 97-99% recall

RAG:
├── nlist: sqrt(dataset_size) (balanced)
├── nprobe: 64 (balanced)
├── index_type: IVF_PQ (compression)
└── Expected: 5-15ms latency, 95-97% recall
```

### Adaptive Runtime Tuning

**How It Works:**

1. **Monitoring**: Tracks query latency and recall (if available)
2. **Analysis**: Analyzes recent queries in sliding window
3. **Adaptation**: Adjusts ef_search to meet targets
4. **Feedback Loop**: Continuously improves based on results

**Adaptive Algorithm:**

```
if (avg_latency > target_latency):
    ef_search = ef_search * 0.9  // Reduce for speed
elif (avg_recall < target_recall):
    ef_search = ef_search * 1.1  // Increase for accuracy
elif (avg_recall > target_recall + 0.02):
    ef_search = ef_search * 0.95 // Optimize (slightly reduce)
```

**Configuration:**

```cpp
HnswParameterTuner::Config config;
config.adaptive = true;  // Enable adaptation
config.target_recall = 0.95;
config.target_latency = std::chrono::milliseconds(10);
config.stats_window_size = 1000;  // Track last 1000 queries
config.workload = HnswParameterTuner::WorkloadType::RAG;

HnswParameterTuner tuner(config);
```

### Performance Monitoring

**Key Metrics:**

```cpp
auto stats = tuner.getStats();

// Monitor these metrics:
stats.queries_processed;      // Total queries
stats.avg_latency_ms;         // Average query latency
stats.avg_recall;             // Average recall (if measured)
stats.current_ef_search;      // Current adapted ef_search
stats.adaptations_count;      // Number of adaptations
```

**Performance Comparison:**

```
Dataset: 1M vectors, 768 dimensions

Workload    | Latency (p95) | Recall@10 | Memory  | QPS
------------|---------------|-----------|---------|-------
OLTP        | 3-5ms         | 92%       | 8 GB    | 8,000
Analytics   | 35-50ms       | 98%       | 25 GB   | 800
RAG         | 10-15ms       | 96%       | 12 GB   | 3,000
Mixed       | 15-25ms       | 95%       | 15 GB   | 2,000
```

### Best Practices

1. **Profile First**: Measure current workload before optimizing
2. **Start Conservative**: Begin with MIXED workload, then specialize
3. **Enable Adaptive**: Let the system tune ef_search automatically
4. **Monitor Metrics**: Track latency and recall in production
5. **Rebuild Periodically**: Rebuild index when data grows 5x
6. **Use GPU**: Enable GPU for Analytics workloads on large datasets
7. **Test Thoroughly**: Benchmark with production-like queries
8. **Document Changes**: Record configuration and performance impact

---

## References

### Documentation
- **FAISS_MIGRATION_COMPLETE.md** - Comprehensive migration summary
- **LIBRARY_USAGE_ANALYSIS.md** - Detailed library usage analysis
- **LIBRARY_OPTIMIZATION_QUICKREF.md** - Quick reference guide
- **PERFORMANCE_TIPS.md** - Performance optimization guidelines
- **Workload Optimization** - This document, section on workload-specific tuning

### Code
- **include/index/advanced_vector_index.h** - FAISS wrapper interface
- **src/index/advanced_vector_index.cpp** - FAISS integration implementation
- **src/acceleration/faiss_gpu_backend.cpp** - GPU acceleration backend
- **cmake/CMakeLists.txt** - Build configuration with FAISS detection

### External
- **FAISS Documentation**: https://github.com/facebookresearch/faiss/wiki
- **FAISS Paper**: "Billion-scale similarity search with GPUs" (2017)
- **hnswlib**: https://github.com/nmslib/hnswlib

---

**Document Version**: 1.1  
**Last Updated**: 2026-04-06  
**Status**: Production-ready ✅  
**Maintainer**: ThemisDB Core Team
