# Quick Reference: Library Usage Optimization Opportunities

## TL;DR

**Problem**: ThemisDB reimplements ~800 lines of FAISS functionality
**Solution**: Replace custom quantizers with FAISS native functions
**Savings**: -800 LoC, +20-30% performance, less maintenance

---

## Identified Duplications

### FAISS Quantizers - ❌ REDUNDANT

| File | Lines | Replace With | Priority |
|------|-------|--------------|----------|
| `src/index/product_quantizer.cpp` | 309 | `faiss::IndexIVFPQ` | 🔴 HIGH |
| `src/index/binary_quantizer.cpp` | 231 | `faiss::IndexBinaryFlat` | 🔴 HIGH |
| `src/index/residual_quantizer.cpp` | 262 | `faiss::IndexResidual` | 🔴 HIGH |

**Used in:**
- `src/index/vector_index.cpp` (VectorIndexManager)
- `src/performance/rabitq.cpp` (RaBitQ)

---

## Migration Examples

### ProductQuantizer → FAISS

```cpp
// ❌ BEFORE: Custom implementation
#include "index/product_quantizer.h"
ProductQuantizer::Config config;
config.num_subquantizers = 8;
config.num_centroids = 256;
quantizer_ = std::make_unique<ProductQuantizer>(dim_, config);
quantizer_->train(training_vectors);
auto codes = quantizer_->encode(vector);

// ✅ AFTER: FAISS native
#include <faiss/IndexIVFPQ.h>
faiss::IndexIVFPQ* index = new faiss::IndexIVFPQ(
    dimension,  // dim
    1024,       // nlist (clusters)
    8,          // m (subquantizers)
    8           // nbits per code
);
index->train(n_train, training_data);
index->add(n_vectors, vectors);
index->search(n_queries, queries, k, distances, labels);
```

### BinaryQuantizer → FAISS

```cpp
// ❌ BEFORE: Custom implementation
#include "index/binary_quantizer.h"
BinaryQuantizer::Config config;
config.center_values = true;
auto quantizer = std::make_unique<BinaryQuantizer>(dim_, config);
quantizer->train(training_vectors);

// ✅ AFTER: FAISS native
#include <faiss/IndexBinaryFlat.h>
faiss::IndexBinaryFlat* index = new faiss::IndexBinaryFlat(dimension);
index->add(n_vectors, binary_vectors);
index->search(n_queries, queries, k, distances, labels);
```

### ResidualQuantizer → FAISS

```cpp
// ❌ BEFORE: Custom implementation
#include "index/residual_quantizer.h"
ResidualQuantizer::Config config;
config.num_stages = 2;
auto quantizer = std::make_unique<ResidualQuantizer>(dim_, config);

// ✅ AFTER: FAISS native
#include <faiss/IndexResidual.h>
faiss::IndexResidual* index = new faiss::IndexResidual(
    dimension,
    num_stages,
    num_centroids_per_stage
);
index->train(n_train, training_data);
```

---

## Correctly Used Libraries ✅

### llama.cpp - ✅ OPTIMAL
- Uses library for: model loading, inference, tokenization, sampling
- Adds: orchestration, async workers, caching, scheduling
- **No duplication** - appropriate abstraction layer

### hnswlib - ✅ OPTIMAL
- Uses library for: HNSW algorithm, search, insertion
- Adds: RocksDB persistence, audit logging, parameter optimization
- **No duplication** - appropriate wrapper

---

## Implementation Plan

### Phase 1: ProductQuantizer (Week 1-2)
1. Update `src/index/vector_index.cpp` to use FAISS
2. Update tests
3. Remove `src/index/product_quantizer.cpp` + header
4. Verify performance improvement

### Phase 2: BinaryQuantizer (Week 2-3)
1. Update usage sites to use FAISS
2. Update tests
3. Remove `src/index/binary_quantizer.cpp` + header

### Phase 3: ResidualQuantizer (Week 3-4)
1. Update `src/performance/rabitq.cpp` to use FAISS
2. Update tests
3. Remove `src/index/residual_quantizer.cpp` + header

---

## Expected Results

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Lines of Code (index/) | 3,304 | 2,504 | -24% |
| Custom Quantizers | 3 | 0 | -100% |
| Quantization Speed | Baseline | +20-30% | ✅ Better |
| Memory Usage | Baseline | Same | = |
| Maintenance | High | Low | ✅ Less |

---

## Verification

After migration, verify:
```bash
# 1. No references to old quantizers
grep -r "ProductQuantizer\|BinaryQuantizer\|ResidualQuantizer" src/ include/

# 2. FAISS usage in expected places
grep -r "faiss::IndexIVFPQ\|faiss::IndexBinaryFlat\|faiss::IndexResidual" src/

# 3. Performance comparison
# Run benchmarks before and after migration
./bench_vector_quantization --before  # Baseline
./bench_vector_quantization --after   # Should show +20-30% improvement
```

---

## Documentation

**Full Analysis:**
- 📄 `LIBRARY_USAGE_ANALYSIS.md` - Detailed analysis (German)
- 📄 `UNTERSUCHUNG_DOPPELSTRUKTUREN.md` - Investigation summary (German)
- 📄 `DUPLICATE_STRUCTURE_INVESTIGATION.md` - Original investigation (English)

**Quick Start:**
- 📄 `LIBRARY_OPTIMIZATION_QUICKREF.md` - This file

---

**Created**: 2026-02-02  
**Status**: Analysis complete, ready for implementation  
**Next**: Start Phase 1 (ProductQuantizer migration)
