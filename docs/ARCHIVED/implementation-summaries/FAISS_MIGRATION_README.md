# FAISS Migration Documentation

**Status**: ✅ Migration Complete  
**Date**: 2026-02-05

This directory contains comprehensive documentation about the FAISS migration assessment for ThemisDB's vector indexing components.

---

## Quick Links

### 📊 Executive Summary (Start Here)
**[FAISS_MIGRATION_EXECUTIVE_SUMMARY.md](FAISS_MIGRATION_EXECUTIVE_SUMMARY.md)**
- TL;DR: Migration goal achieved
- Quick facts and component status
- Key decisions and next steps
- **Best for**: Stakeholders, project managers, quick overview

### 🏗️ Architecture Guide
**[VECTOR_INDEXING_ARCHITECTURE.md](VECTOR_INDEXING_ARCHITECTURE.md)**
- Visual architecture diagrams
- Production flow and degradation chain
- Performance comparisons
- Configuration examples
- **Best for**: Developers, architects, implementation details

### 📝 Complete Migration Summary
**[FAISS_MIGRATION_COMPLETE.md](FAISS_MIGRATION_COMPLETE.md)**
- Comprehensive technical analysis
- Component-by-component status
- Why decisions were made
- Verification and testing details
- **Best for**: Technical deep dive, complete picture

### 📚 Supporting Documentation
- **[LIBRARY_USAGE_ANALYSIS.md](LIBRARY_USAGE_ANALYSIS.md)** - Original library usage analysis (German)
- **[LIBRARY_OPTIMIZATION_QUICKREF.md](LIBRARY_OPTIMIZATION_QUICKREF.md)** - Quick reference guide
- **[CHANGELOG.md](CHANGELOG.md)** - v1.5.0 migration entry

---

## Key Findings

### ✅ Migration Goal: ACHIEVED

ThemisDB **already uses FAISS as its primary production vector indexing solution** via the **AdvancedVectorIndex** component.

```
Production Architecture:
  User → VectorIndexManager
    ├─ AdvancedVectorIndex (FAISS IVF+PQ/HNSW) ✅ PRIMARY
    └─ Fallback: HNSW (hnswlib) or ProductQuantizer

Graceful Degradation:
  FAISS GPU → FAISS CPU → HNSW → Custom Fallback
```

### Component Status

| Component | Status | Role |
|-----------|--------|------|
| AdvancedVectorIndex | ✅ Production | Primary (FAISS IVF+PQ/HNSW) |
| FAISS GPU Backend | ✅ Production | GPU acceleration |
| ProductQuantizer | ✅ Fallback | Non-FAISS paths |
| BinaryQuantizer | ⚠️ Deprecated | Not used (-79 lines) |
| LearnedQuantizer | ⚠️ Deprecated | Research only |
| ResidualQuantizer | 🔬 Research | Multi-stage quantization |

### Performance (FAISS IVF+PQ)
- **Compression**: 10-100x (150MB vs 6GB for 1M vectors)
- **Search Speed**: ~2ms per query (GPU, 1M vectors)
- **Accuracy**: ~95% recall@10
- **Throughput**: ~500 QPS (single GPU)

---

## Reading Guide

### For Quick Overview
1. Read: **FAISS_MIGRATION_EXECUTIVE_SUMMARY.md**
2. Decision: Migration complete, no further work needed ✅

### For Implementation Details
1. Read: **VECTOR_INDEXING_ARCHITECTURE.md**
2. Focus: Configuration examples, performance characteristics
3. Apply: Use AdvancedVectorIndex for production workloads

### For Complete Understanding
1. Read all three main documents in order:
   - FAISS_MIGRATION_EXECUTIVE_SUMMARY.md (overview)
   - VECTOR_INDEXING_ARCHITECTURE.md (architecture)
   - FAISS_MIGRATION_COMPLETE.md (deep dive)
2. Reference: LIBRARY_USAGE_ANALYSIS.md (background)

---

## FAQ

### Q: Do I need to migrate my code?

**A: No.** If you're using AdvancedVectorIndex (the default for production), you're already using FAISS natively. No code changes needed.

### Q: Should I still use custom quantizers?

**A: No, use AdvancedVectorIndex instead.** Custom quantizers (ProductQuantizer, etc.) are fallback implementations for compatibility. Production code should use AdvancedVectorIndex with FAISS.

### Q: How do I enable FAISS?

**A: It's automatic.** If FAISS is found during build (via vcpkg or system), `THEMIS_GPU_ENABLED` is set and AdvancedVectorIndex uses FAISS automatically. See VECTOR_INDEXING_ARCHITECTURE.md for configuration examples.

### Q: Why keep ProductQuantizer?

**A: API compatibility.** FAISS IndexIVFPQ doesn't expose standalone encode/decode methods. ProductQuantizer serves as fallback for non-FAISS paths. However, production workloads should use AdvancedVectorIndex (FAISS) instead.

### Q: What about GPU acceleration?

**A: Available via FAISS.** Set `use_gpu=true` in AdvancedIndexConfig to enable GPU acceleration (requires NVIDIA/AMD GPU and FAISS with GPU support).

### Q: Can I use HNSW without FAISS?

**A: Yes.** Don't call `setAdvancedIndexConfig()` or set `enabled=false`. VectorIndexManager will use HNSW from hnswlib automatically.

---

## Configuration Example

```cpp
// Production: FAISS IVF+PQ with GPU
VectorIndexManager::AdvancedIndexConfig config;
config.enabled = true;                    // Enable FAISS
config.index_type = AdvancedIndexConfig::Type::IVF_PQ;
config.nlist = 1024;                     // IVF clusters
config.nprobe = 64;                      // Search clusters
config.use_pq = true;                    // Enable PQ
config.pq_m = 8;                         // Subquantizers
config.pq_nbits = 8;                     // Bits per code
config.use_gpu = true;                   // Enable GPU
config.gpu_device = 0;                   // GPU 0

vectorIndexManager.setAdvancedIndexConfig(config);
vectorIndexManager.init(objectName, dimension, metric);
```

See VECTOR_INDEXING_ARCHITECTURE.md for more examples.

---

## Document Map

```
FAISS Migration Documentation/
│
├── FAISS_MIGRATION_README.md (this file)
│   └── Entry point and navigation guide
│
├── FAISS_MIGRATION_EXECUTIVE_SUMMARY.md
│   ├── TL;DR and key findings
│   ├── Component status at a glance
│   └── Next steps and decisions
│
├── VECTOR_INDEXING_ARCHITECTURE.md
│   ├── Visual architecture diagrams
│   ├── Production flow and degradation
│   ├── Performance comparisons
│   └── Configuration examples
│
├── FAISS_MIGRATION_COMPLETE.md
│   ├── Comprehensive technical analysis
│   ├── Component-by-component details
│   ├── Why decisions were made
│   └── Verification procedures
│
├── LIBRARY_USAGE_ANALYSIS.md
│   └── Original analysis (background)
│
└── LIBRARY_OPTIMIZATION_QUICKREF.md
    └── Quick reference (summary)
```

---

## Related Resources

### Internal Documentation
- `include/index/advanced_vector_index.h` - FAISS wrapper interface
- `src/index/advanced_vector_index.cpp` - FAISS integration
- `src/acceleration/faiss_gpu_backend.cpp` - GPU backend
- `cmake/CMakeLists.txt` - Build configuration

### External Resources
- [FAISS GitHub](https://github.com/facebookresearch/faiss)
- [FAISS Wiki](https://github.com/facebookresearch/faiss/wiki)
- [FAISS Paper: "Billion-scale similarity search with GPUs"](https://arxiv.org/abs/1702.08734)
- [hnswlib](https://github.com/nmslib/hnswlib)

---

## Conclusion

The FAISS migration assessment is **COMPLETE**. ThemisDB uses FAISS as its primary production vector indexing solution. No further migration work is required.

**For questions or issues**, refer to:
1. This README for navigation
2. FAISS_MIGRATION_EXECUTIVE_SUMMARY.md for quick answers
3. VECTOR_INDEXING_ARCHITECTURE.md for implementation details
4. FAISS_MIGRATION_COMPLETE.md for complete technical analysis

---

**Created**: 2026-02-05  
**Status**: Complete  
**Maintainer**: ThemisDB Core Team
