### Context

This issue implements the roadmap item 'GPU Vector Index: CUDA and HIP Backend Implementation' for the index domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.4.0.

Primary detail section: GPU Vector Index: CUDA and HIP Backend Implementation

### Goal

Deliver the scoped changes for GPU Vector Index: CUDA and HIP Backend Implementation in src/index/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### GPU Vector Index: CUDA and HIP Backend Implementation
**Priority:** High
**Target Version:** v1.4.0

`src/index/gpu_vector_index.cpp` has 2 unimplemented GPU backends:
- Line 711: `// HIP backend not implemented - fallback to CPU`
- Line 722: `// CUDA backend not implemented in this PR`

Both paths fall through to the CPU implementation, making GPU-accelerated ANN search non-functional.

**Implementation Notes:**
- `[ ]` Implement the CUDA backend (line 722): use cuVS/RAFT `raft::neighbors::hnsw` for graph construction and search; allocate device memory via `GpuMemoryPool` from `src/gpu/memory_pool.cpp`.
- `[ ]` Implement the HIP backend (line 711): use `hipblas` + ROCm equivalent of RAFT or a custom HIP HNSW kernel; mirror the CUDA backend interface.
- `[ ]` `advanced_vector_index.cpp` (line 146): replace the "FAISS not available - using stub" warning path with a compile-time `#error` requiring either FAISS or HNSW to be enabled; stubs should not silently succeed in production builds.
- `[ ]` `learned_quantizer.cpp` (line 353): implement the TODO "compute distance directly from codes/centroids without full decoding" — this is an asymmetric distance computation (ADC) optimization that can deliver 3–5× speedup for product quantization search.

**Performance Targets:**
- GPU ANN search (1 M 768-dim vectors, k=10): ≥ 10× throughput vs. CPU HNSW on RTX 3080.
- ADC quantized distance: ≥ 3× speedup vs. full decode path on CPU.

---


**Priority:** High  
**Target Version:** v1.7.0

Add inverted index for full-text search with stemming, stop words, and relevance ranking.

**Features:**
- **Tokenization**: Language-aware tokenizers (English, German, French, etc.)
- **Stemming**: Porter/Snowball stemmers for root word extraction
- **Stop Words**: Configurable stop word lists
- **TF-IDF Scoring**: Term frequency-inverse document frequency ranking
- **Phrase Search**: Quoted phrase matching
- **Fuzzy Search**: Levenshtein distance for typo tolerance
- **Highlighting**: Result snippet generation with keyword highlighting

**API:**
```cpp
// Create full-text index
sim.createFullTextIndex("articles", "content", {
    .language = "en",
    .stemming = true,
    .stop_words = {"the", "a", "an"},
    .min_word_length = 3
});

// Full-text search
auto results = sim.fullTextSearch(
    "articles", "content",
    "machine learning algorithms",
    /*limit=*/10
);

// Results sorted by relevance score
for (const auto& result : results) {
    std::cout << "Score: " << result.score << std::endl;
    std::cout << "Snippet: " << result.snippet << std::endl;
}
```

**Implementation:**
- RocksDB key schema: `fts:table:term:pk`
- Term frequency storage: `fts_tf:table:pk:term -> frequency`
- Document frequency: `fts_df:table:term -> doc_count`
- BM25 ranking algorithm for relevance scoring

**Use Cases:**
- Document search
- Log analysis
- Content discovery
- Semantic search (combined with vector search)

---

### Acceptance Criteria

- [ ] Line 711: `// HIP backend not implemented - fallback to CPU`
- [ ] Line 722: `// CUDA backend not implemented in this PR`
- [ ] Implement the CUDA backend (line 722): use cuVS/RAFT `raft::neighbors::hnsw` for graph construction and search; allocate device memory via `GpuMemoryPool` from `src/gpu/memory_pool.cpp`.
- [ ] Implement the HIP backend (line 711): use `hipblas` + ROCm equivalent of RAFT or a custom HIP HNSW kernel; mirror the CUDA backend interface.
- [ ] `advanced_vector_index.cpp` (line 146): replace the "FAISS not available - using stub" warning path with a compile-time `#error` requiring either FAISS or HNSW to be enabled; stubs should not silently succeed in production builds.
- [ ] `learned_quantizer.cpp` (line 353): implement the TODO "compute distance directly from codes/centroids without full decoding" — this is an asymmetric distance computation (ADC) optimization that can deliver 3–5× speedup for product quantization search.
- [ ] GPU ANN search (1 M 768-dim vectors, k=10): ≥ 10× throughput vs. CPU HNSW on RTX 3080.
- [ ] ADC quantized distance: ≥ 3× speedup vs. full decode path on CPU.
- [ ] **Tokenization**: Language-aware tokenizers (English, German, French, etc.)
- [ ] **Stemming**: Porter/Snowball stemmers for root word extraction
- [ ] **Stop Words**: Configurable stop word lists
- [ ] **TF-IDF Scoring**: Term frequency-inverse document frequency ranking
- [ ] **Phrase Search**: Quoted phrase matching
- [ ] **Fuzzy Search**: Levenshtein distance for typo tolerance
- [ ] **Highlighting**: Result snippet generation with keyword highlighting
- [ ] RocksDB key schema: `fts:table:term:pk`
- [ ] Term frequency storage: `fts_tf:table:pk:term -> frequency`
- [ ] Document frequency: `fts_df:table:term -> doc_count`
- [ ] BM25 ranking algorithm for relevance scoring
- [ ] Document search
- [ ] Log analysis
- [ ] Content discovery
- [ ] Semantic search (combined with vector search)

### Relationships

- Roadmap row: #29 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/index/FUTURE_ENHANCEMENTS.md#gpu-vector-index-cuda-and-hip-backend-implementation
- Source key: roadmap:29:index:v1.4.0:gpu-vector-index-cuda-and-hip-backend-implementation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:29:index:v1.4.0:gpu-vector-index-cuda-and-hip-backend-implementation -->
<!-- roadmap-ref: row=29;module=index;target=v1.4.0 -->
<!-- roadmap-detail: src/index/FUTURE_ENHANCEMENTS.md#gpu-vector-index-cuda-and-hip-backend-implementation -->
