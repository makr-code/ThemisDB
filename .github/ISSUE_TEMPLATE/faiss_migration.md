---
name: FAISS Migration - Replace Custom Quantizers
about: Track migration of custom quantizers to FAISS native implementation
title: '[REFACTOR] Migrate [Component] to FAISS native implementation'
labels: refactoring, performance, technical-debt
assignees: ''
---

## Migration Overview

**Component to Migrate**: <!-- ProductQuantizer / BinaryQuantizer / ResidualQuantizer -->

**Current Implementation**: <!-- e.g., src/index/product_quantizer.cpp (309 lines) -->

**Target FAISS Implementation**: <!-- e.g., faiss::IndexIVFPQ -->

**Migration Phase**: <!-- Phase 1 / Phase 2 / Phase 3 -->

## Problem Statement

ThemisDB currently reimplements quantization functionality that already exists in FAISS library. This creates:
- ❌ ~800 lines of redundant code across 3 quantizers
- ❌ Double maintenance burden
- ❌ Missing FAISS optimizations (SIMD, GPU acceleration)
- ❌ Potentially worse performance compared to native FAISS

**Analysis Reference**: See `LIBRARY_USAGE_ANALYSIS.md` and `LIBRARY_OPTIMIZATION_QUICKREF.md`

## Current State

### Files Affected
<!-- List all files that need to be modified or removed -->
- [ ] `src/index/[quantizer].cpp` - Implementation to be removed
- [ ] `include/index/[quantizer].h` - Header to be removed
- [ ] `src/index/vector_index.cpp` - Usage to be updated
- [ ] `src/performance/rabitq.cpp` - Usage to be updated (if applicable)
- [ ] Tests: `tests/test_[quantizer].cpp` - To be updated

### Current Usage Locations
```bash
# Find current usage
grep -r "ProductQuantizer\|BinaryQuantizer\|ResidualQuantizer" src/ include/
```

## Migration Plan

### Phase 1: Preparation (Week 1)
- [ ] **Review current implementation**
  - Analyze current API and usage patterns
  - Document configuration parameters
  - Identify all usage sites
  
- [ ] **Understand FAISS equivalent**
  - Read FAISS documentation for equivalent index
  - Test FAISS implementation standalone
  - Verify feature parity
  
- [ ] **Create migration branch**
  ```bash
  git checkout -b refactor/faiss-[component]-migration
  ```

### Phase 2: Implementation (Week 1-2)

#### Step 1: Add FAISS Headers
```cpp
// Before
#include "index/product_quantizer.h"

// After
#include <faiss/IndexIVFPQ.h>      // For ProductQuantizer
#include <memory>                  // For std::unique_ptr
// OR
#include <faiss/IndexBinaryFlat.h> // For BinaryQuantizer
#include <memory>                  // For std::unique_ptr
// OR
#include <faiss/IndexResidual.h>   // For ResidualQuantizer
#include <memory>                  // For std::unique_ptr
```

#### Step 2: Replace Implementation

**Example: ProductQuantizer → FAISS**
```cpp
// ❌ OLD CODE (to be removed)
ProductQuantizer::Config config;
config.num_subquantizers = 8;
config.num_centroids = 256;
quantizer_ = std::make_unique<ProductQuantizer>(dim_, config);
quantizer_->train(training_vectors);
auto codes = quantizer_->encode(vector);

// ✅ NEW CODE (FAISS native)
auto index = std::make_unique<faiss::IndexIVFPQ>(
    dimension_,  // dim
    1024,        // nlist (clusters)
    8,           // m (subquantizers)
    8            // nbits per code
);
index->train(n_train, training_data);
index->add(n_vectors, vectors);
index->search(n_queries, queries, k, distances, labels);
```

#### Step 3: Update All Usage Sites
- [ ] Update `vector_index.cpp`
- [ ] Update `rabitq.cpp` (if applicable)
- [ ] Update any other files identified in preparation phase

#### Step 4: Update Tests
- [ ] Modify existing tests to work with FAISS API
- [ ] Verify test coverage is maintained
- [ ] Add performance comparison tests

### Phase 3: Validation (Week 2)

- [ ] **Functionality Tests**
  ```bash
  # Run specific tests
  ./build/tests/test_vector_index
  ./build/tests/test_[component]
  ```

- [ ] **Performance Benchmarks**
  ```bash
  # Compare before/after performance
  ./benchmarks/bench_vector_quantization --baseline
  ./benchmarks/bench_vector_quantization --faiss
  ```
  
- [ ] **Integration Tests**
  ```bash
  # Run full test suite
  ctest --output-on-failure
  ```

### Phase 4: Cleanup (Week 2)

- [ ] **Remove old files**
  ```bash
  git rm src/index/[quantizer].cpp
  git rm include/index/[quantizer].h
  git rm tests/test_[quantizer].cpp  # If no longer needed
  ```

- [ ] **Update documentation**
  - [ ] Update README if quantizers are documented
  - [ ] Update API documentation
  - [ ] Add migration notes to CHANGELOG.md

- [ ] **Update build system**
  - [ ] Remove references from CMakeLists.txt (if explicitly listed)
  - [ ] Update dependency documentation

## Expected Results

### Metrics
| Metric | Before | After (Expected) | Improvement |
|--------|--------|------------------|-------------|
| Lines of Code | ~[300] | 0 | -100% |
| Training Time | Baseline | -20-30% | ✅ Faster |
| Quantization Time | Baseline | -20-30% | ✅ Faster |
| Memory Usage | Baseline | Same | = |
| Maintenance Effort | High | Low | ✅ Reduced |

### Performance Benchmarks
<!-- To be filled after implementation -->
```
Baseline (Custom):
- Training: [X] ms
- Encoding: [Y] ms/vector
- Search: [Z] ms

FAISS Native:
- Training: [X'] ms ([+/-]%)
- Encoding: [Y'] ms/vector ([+/-]%)
- Search: [Z'] ms ([+/-]%)
```

## Testing Checklist

### Unit Tests
- [ ] All existing unit tests pass
- [ ] New FAISS-specific tests added (if needed)
- [ ] Edge cases covered

### Integration Tests
- [ ] VectorIndexManager integration works
- [ ] RaBitQ integration works (if applicable)
- [ ] End-to-end vector search works

### Performance Tests
- [ ] Quantization performance measured
- [ ] Search performance measured
- [ ] Memory usage validated
- [ ] Results meet or exceed expectations

### Regression Tests
- [ ] No existing functionality broken
- [ ] API compatibility maintained (if applicable)
- [ ] All benchmarks pass

## Documentation Updates

- [ ] **Code Comments**
  - Migration rationale documented
  - FAISS usage patterns commented
  
- [ ] **README Updates**
  - Remove references to custom quantizers
  - Add FAISS dependency notes
  
- [ ] **Migration Guide**
  - Update `LIBRARY_USAGE_ANALYSIS.md` status
  - Mark component as completed in `LIBRARY_OPTIMIZATION_QUICKREF.md`
  
- [ ] **CHANGELOG.md**
  ```markdown
  ### Changed
  - Migrated [Component] to FAISS native implementation
  - Removed ~[300] lines of redundant quantization code
  - Improved quantization performance by ~[25]%
  ```

## Rollback Plan

If migration fails or causes issues:

1. **Immediate Rollback**
   ```bash
   git checkout main
   git branch -D refactor/faiss-[component]-migration
   ```

2. **Partial Rollback**
   ```bash
   git revert [commit-hash]
   ```

3. **Keep both implementations** (temporary fallback)
   - Add feature flag to switch between implementations
   - Gather more data before final decision

## Dependencies

### Required
- [ ] FAISS library available (already in vcpkg.json under 'gpu' feature)
- [ ] GPU feature enabled (if using GPU acceleration)

### Optional
- [ ] Benchmark infrastructure ready
- [ ] Performance monitoring in place

## Related Issues/PRs

- Investigation PR: #[PR_NUMBER] - Library usage analysis
- Related Documentation: `LIBRARY_USAGE_ANALYSIS.md`
- Quick Reference: `LIBRARY_OPTIMIZATION_QUICKREF.md`

## Success Criteria

- ✅ All tests pass
- ✅ Performance meets or exceeds baseline
- ✅ Code reduction achieved (~[300] lines)
- ✅ No regressions in functionality
- ✅ Documentation updated
- ✅ Clean git history (squashed commits)

## Additional Notes

### FAISS Configuration Examples

**ProductQuantizer:**
```cpp
auto index = std::make_unique<faiss::IndexIVFPQ>(dimension, nlist, m, nbits);
// dimension: vector dimensionality
// nlist: number of clusters (try sqrt(N))
// m: number of subquantizers (dim % m must be 0)
// nbits: bits per code (typically 8)
```

**BinaryQuantizer:**
```cpp
auto index = std::make_unique<faiss::IndexBinaryFlat>(dimension);
// dimension: vector dimensionality (in bits)
```

**ResidualQuantizer:**
```cpp
auto index = std::make_unique<faiss::IndexResidual>(dimension, num_stages, num_centroids);
// dimension: vector dimensionality
// num_stages: number of residual stages
// num_centroids: centroids per stage
```

### Resources

- FAISS Documentation: https://github.com/facebookresearch/faiss/wiki
- ThemisDB Analysis: `LIBRARY_USAGE_ANALYSIS.md`
- Quick Reference: `LIBRARY_OPTIMIZATION_QUICKREF.md`
- Migration Examples: See Quick Reference document

---

**Assignee Checklist:**
- [ ] I understand the migration plan
- [ ] I have access to required resources
- [ ] I have reviewed FAISS documentation
- [ ] I have estimated the effort (1-2 weeks)
- [ ] I am ready to start implementation

**Reviewer Checklist:**
- [ ] Code quality is acceptable
- [ ] All tests pass
- [ ] Performance benchmarks show improvement
- [ ] Documentation is updated
- [ ] No regressions identified
- [ ] Ready to merge
