# ThemisDB Vector Indexing Architecture

**Status**: Production-ready with FAISS integration ✅  
**Last Updated**: 2026-02-05

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

## References

### Documentation
- **FAISS_MIGRATION_COMPLETE.md** - Comprehensive migration summary
- **LIBRARY_USAGE_ANALYSIS.md** - Detailed library usage analysis
- **LIBRARY_OPTIMIZATION_QUICKREF.md** - Quick reference guide

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

**Document Version**: 1.0  
**Last Updated**: 2026-02-05  
**Status**: Production-ready ✅  
**Maintainer**: ThemisDB Core Team
