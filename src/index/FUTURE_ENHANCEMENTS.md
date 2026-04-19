> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Index Module - Future Enhancements

- HNSW vector similarity indexes (L2, Cosine, Dot Product) with GPU acceleration (Vulkan, CUDA, HIP)
- Product Quantization (PQ), Binary Quantization, and Residual Quantization compression
- B-tree, range, sparse, composite, and partial (filtered) secondary indexes
- R-tree spatial index with Z-order curve ordering
- Graph adjacency index for BFS/DFS traversal
- Learned index structures (ML-based B-tree replacement) and DiskANN/ScaNN on-disk ANN
- Multi-tenancy index isolation via RocksDB key-prefix scoping (`tenant:<id>:<index_name>`)
- Online index rebuild, cold/warm tier migration, and GPU-accelerated index build

## Design Constraints

- [ ] HNSW recall@10 must be ≥ 0.95 for cosine similarity on standard ANN benchmarks (e.g., SIFT-1M)
- [ ] GPU vector search must fall back to CPU automatically when GPU driver is unavailable
- [ ] Index writes must use RocksDB `WriteBatch` for atomicity; partial index states are not observable
- [ ] Online index rebuild must not degrade read QPS by more than 10% during rebuild
- [ ] Multi-tenancy key-prefix isolation must prevent cross-tenant key access at the `IndexManager` layer
- [ ] Learned index structures must maintain exact-match correctness (no false negatives)
- [ ] GPU memory budget per index is configurable; exceeding budget must fall back gracefully
- [ ] Cold/warm tier migration must be transparent to callers; no API change required

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IVectorIndex::search(query, k)` | Query engine, AQL | Returns top-k neighbors with distances; GPU or CPU backend |
| `IVectorIndex::add(id, vector)` | Ingestion pipeline | Persisted via RocksDB WriteBatch atomicity |
| `ISecondaryIndex::lookup(key)` | Query executor | Supports B-tree, range, spatial, and full-text backends |
| `IndexManager::createVectorIndex(config)` | Database core | Returns `VectorIndexAdapter`; enforces tenant isolation |
| `IndexManager::createSecondaryIndex(config)` | Database core | `config="partial:<predicate>"` for filtered indexes |
| `VectorIndexManager::incrementalReindex()` | Background maintenance | HNSW incremental rebuild without full index drop |
| `GPUVectorIndex::search(query, k)` | `IVectorIndex` impl | Requires Vulkan/CUDA/HIP; auto-fallback to CPU |
| `IndexAdvisor::recommend(workload)` | Query planner | Returns ranked index recommendations from workload replay |

## Planned Features

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

### Full-Text Search Index
**Priority:** High
**Original Target Version:** v1.7.0
**Status:** ✅ Implemented (v1.5.0)

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

### Distributed Index Partitioning
**Priority:** High
**Target Version:** v1.7.0

Shard indexes across multiple nodes for horizontal scalability.

**Features:**
- **Hash Partitioning**: Distribute keys by hash(pk) % num_shards
- **Range Partitioning**: Split by key ranges (e.g., A-M, N-Z)
- **Consistent Hashing**: Minimize data movement on resharding
- **Shard Rebalancing**: Automatic redistribution on node add/remove
- **Distributed Queries**: Scatter-gather for cross-shard queries

**Architecture:**
```
┌────────────────┐
│ Query Planner  │
└────────┬───────┘
         │ Scatter
         ├─────────────┬─────────────┬─────────────┐
         ▼             ▼             ▼             ▼
    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
    │Shard 0 │    │Shard 1 │    │Shard 2 │    │Shard 3 │
    │ (A-F)  │    │ (G-M)  │    │ (N-S)  │    │ (T-Z)  │
    └────────┘    └────────┘    └────────┘    └────────┘
         │             │             │             │
         └─────────────┴─────────────┴─────────────┘
                       │ Gather
                       ▼
              ┌────────────────┐
              │ Merge Results  │
              └────────────────┘
```

**API:**
```cpp
// Configure partitioning
IndexManager::ShardingConfig config;
config.num_shards = 4;
config.strategy = ShardingStrategy::HASH;
config.replication_factor = 2;  // Replicate each shard 2x

index_manager->enableSharding(config);

// Queries automatically routed to correct shards
auto results = vim.search("embeddings", query_vector, 10);
// Internally: scatter to shards, merge top-k results
```

**Challenges:**
- Cross-shard joins and traversals
- Maintaining global statistics (IDF for full-text)
- Atomic multi-shard transactions
- Network latency for distributed queries

---

### GPU Memory Oversubscription
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Delivered (v1.7.0)

Support datasets larger than GPU VRAM via paging and streaming.

**Features:**
- **Unified Memory**: CUDA Unified Memory for automatic paging — ✅ via `GPUUnifiedMemoryAllocator` (CPU fallback on non-GPU builds)
- **Streaming**: Load index chunks from host RAM as needed — ✅ `GPUMemoryOversubscriptionManager::accessPartition()`
- **LRU Eviction**: Keep hot partitions in VRAM, evict cold — ✅ LRU doubly-linked list in `Impl`
- **Prefetching**: Predict next access patterns, prefetch to GPU — ✅ `PrefetchStrategy` (NONE/LRU/MRU/SEQUENTIAL)
- **Multi-GPU**: Distribute index across multiple GPUs — ✅ `MultiGPUVectorIndex` distributes partitions; oversubscription config available per-GPU

**Implementation:**
- `include/index/gpu_memory_oversubscription.h` — `PrefetchStrategy` enum + `GPUMemoryOversubscriptionManager` class
- `src/index/gpu_memory_oversubscription.cpp` — full LRU/streaming/prefetch implementation
- `include/index/gpu_vector_index.h` — `Config::enable_oversubscription`, `vram_budget_mb`, `prefetch_strategy`, `oversubscription_partition_vectors`
- `src/index/gpu_vector_index.cpp` — wires oversubscription manager into `initialize`/`addVector`/`search`/`getStatistics`

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

**Tests:** `tests/index/test_gpu_memory_oversubscription.cpp` — 26 focused tests
**CI:** `.github/workflows/gpu-memory-oversubscription-ci.yml`

---

### Index Compression
**Priority:** Medium
**Target Version:** v1.7.0
**Status:** ✅ Implemented (v1.7.0)

Reduce index storage footprint via compression.

**Techniques:**
- **Delta Encoding**: Store differences between adjacent keys
- **Prefix Compression**: Share common key prefixes
- **Bloom Filters**: Reduce false lookups (already in RocksDB)
- **Dictionary Encoding**: Map frequent strings to small integers
- **Run-Length Encoding**: Compress repeated values

**Example:**
```cpp
// Without compression
idx:users:country:USA:pk1
idx:users:country:USA:pk2
idx:users:country:USA:pk3

// With prefix compression
idx:users:country:USA:[pk1, pk2, pk3]

// 60% size reduction typical
```

**API:**
```cpp
SecondaryIndexManager::Config config;
config.enable_compression = true;
config.compression_algorithm = CompressionAlgorithm::ZSTD;
config.compression_level = 3;  // Balance: speed vs ratio

SecondaryIndexManager sim(db, config);
```

**Implementation:**
- `include/index/index_compression.h` — `CompressionAlgorithm` enum, `BloomFilter`, `DictionaryCodec`, `PrefixCompressor`/`PrefixBlock`, `DeltaEncoder`/`DeltaBlock`, `RunLengthEncoder`/`RunLengthBlock`, `IndexCompressionCodec`
- `src/index/index_compression.cpp` — Full production implementation of all five techniques
- `include/index/secondary_index.h` — `SecondaryIndexManager::Config` with `enable_compression`, `compression_algorithm`, `compression_level`, and per-technique flags
- `src/index/secondary_index.cpp` — Config-based constructor wires `IndexCompressionCodec` into the manager

**Tests:** `tests/index/test_index_compression.cpp` — 30+ focused tests covering all 5 acceptance criteria
**CI:** `.github/workflows/index-compression-ci.yml`

---

### Matryoshka Representation Learning (MRL) Truncation
**Priority:** High
**Original Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0)

MRL (Kusupati et al., NeurIPS 2022) trains embeddings so that every prefix of
length d < D is itself a useful d-dimensional representation.  A single full-D
embedding can therefore be truncated to any supported granularity without
retraining, enabling:

- **Multi-stage retrieval**: cheap low-D ANN pre-filter → full-D re-rank.
- **Adaptive precision**: 64-D for rough candidate selection, 768-D for scoring.
- **Compact indexes**: smaller index fits in RAM → faster queries on large corpora.

#### ✅ Implemented Features

- `MatryoshkaTruncation` — stateless helper: `truncate(vector, full_dim)` and
  `truncate(std::vector<float>)` overloads; optional L2 normalisation so that
  dot-product equals cosine similarity on the result.
- `MatryoshkaTruncatedIndex` — `IAnnIndex` decorator that applies truncation
  transparently to **any** ANN backend (`ScaNN`, `DiskAnnAdapter`, HNSW, …).
  Methods: `build`, `add`, `search`, `save`, `load`, `size`, `truncation()`,
  `backend()` accessors.
- Standard MRL granularity constants: `kMRL_64 / 128 / 256 / 512 / 768 / 1024 / 1536`
  (matches OpenAI text-embedding-3-small/-large, Nomic Embed v1.5, BGE-M3).
- Zero-vector safety: no division-by-zero when normalisation is enabled.
- Zero-pad behaviour when `trunc_dim > full_dim` (graceful degradation).

**Files:**
- `include/index/matryoshka_truncation.h` — header-only implementation
- `src/index/matryoshka_truncation.cpp` — compilation unit (extension point)

**Tests:** `tests/index/test_matryoshka_truncation.cpp` — 25 focused tests (v1.8.0)
**CI:** `.github/workflows/matryoshka-truncation-ci.yml`

**References:**
- Kusupati, A. et al. "Matryoshka Representation Learning." NeurIPS 2022.
  https://arxiv.org/abs/2205.13147

---

### Learned Indexes
**Priority:** Medium
**Target Version:** v1.8.0

Replace B-tree with ML models for improved lookup performance.

**Concept:** Learn CDF (cumulative distribution function) of keys to predict position.

**Sources:**
- Paper: Kraska et al. (2018), "The Case for Learned Index Structures", SIGMOD
- Benefits: 2-3x faster lookups, 10-100x smaller indexes
- Tradeoffs: Requires training, less flexible for updates

**Architecture:**
```
┌──────────────┐
│   ML Model   │  Predicts: position = f(key)
│ (Neural Net) │
└──────┬───────┘
       │
       ▼
┌──────────────────┐
│  Correction      │  Binary search in local region
│  Layer (±ε)      │
└──────┬───────────┘
       │
       ▼
┌──────────────┐
│  Final Value │
└──────────────┘
```

**API:**
```cpp
// Enable learned index
sim.createLearnedIndex("users", "age", {
    .model_type = ModelType::NEURAL_NETWORK,
    .hidden_layers = {128, 64, 32},
    .error_bound = 100,  // Search within ±100 positions
    .retraining_interval = 3600  // Retrain hourly
});
```

**Use Cases:**
- Read-heavy workloads
- Stable key distributions
- Large indexes (> 1M keys)

---

### Approximate Index Maintenance
**Priority:** Medium
**Target Version:** v1.6.0

Trade consistency for write throughput via lazy index updates.

**Strategies:**
- **Batch Updates**: Buffer writes, flush periodically
- **Asynchronous Updates**: Update indexes in background thread
- **Eventually Consistent**: Accept temporary inconsistency
- **Merge Thresholds**: Only update index after N changes

**Configuration:**
```cpp
SecondaryIndexManager::Config config;
config.update_strategy = UpdateStrategy::BATCH;
config.batch_size = 1000;          // Flush every 1K writes
config.flush_interval_ms = 5000;    // Or every 5 seconds
config.consistency = ConsistencyLevel::EVENTUAL;

SecondaryIndexManager sim(db, config);
```

**Benefits:**
- 10-50x higher write throughput
- Lower write amplification
- Reduced lock contention

**Tradeoffs:**
- Index may lag behind data
- Queries may miss recent writes
- More complex recovery logic

---

## Performance Optimizations

### SIMD Optimization for Vector Distance
**Priority:** High
**Target Version:** v1.6.0

Use AVX-512/NEON instructions for faster distance computation.

**Current:** Generic C++ loops (~1 GB/s)
**Optimized:** AVX-512 intrinsics (~50 GB/s)

**Implementation:**
```cpp
// AVX-512 for L2 distance
float l2_distance_avx512(const float* a, const float* b, size_t dim) {
    __m512 sum = _mm512_setzero_ps();
    for (size_t i = 0; i < dim; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 diff = _mm512_sub_ps(va, vb);
        sum = _mm512_fmadd_ps(diff, diff, sum);
    }
    return _mm512_reduce_add_ps(sum);
}

// ARM NEON for mobile/edge
float l2_distance_neon(const float* a, const float* b, size_t dim) {
    float32x4_t sum = vdupq_n_f32(0.0f);
    for (size_t i = 0; i < dim; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        float32x4_t diff = vsubq_f32(va, vb);
        sum = vmlaq_f32(sum, diff, diff);
    }
    return vaddvq_f32(sum);
}
```

**Expected Improvement:**
- 50x faster distance computation
- 10x faster HNSW search
- Critical for CPU-based vector search

---

### Cache-Oblivious Indexes
**Priority:** Medium
**Target Version:** v1.7.0

Optimize index layout for CPU cache hierarchy (L1/L2/L3).

**Techniques:**
- **Cache-Oblivious B-trees**: Recursive blocking for cache efficiency
- **Eytzinger Layout**: Breadth-first array layout for binary search
- **Prefetch Hints**: Software prefetch for predictable access patterns

**Benefits:**
- 2-5x faster lookups (cache-friendly)
- Fewer cache misses
- Better memory bandwidth utilization

**References:**
- Paper: Brodal et al. (2002), "Cache-Oblivious Data Structures"
- Eytzinger layout: Used in `std::lower_bound` optimizations

---

### GPU Kernel Fusion
**Priority:** Medium
**Target Version:** v1.6.0

Combine multiple GPU operations into single kernel to reduce overhead.

**Example:**
```cpp
// Before: 3 kernel launches
distance_kernel<<<...>>>(query, database, distances);
top_k_kernel<<<...>>>(distances, indices, k);
rerank_kernel<<<...>>>(indices, refined_distances);

// After: 1 fused kernel (3x faster)
fused_search_kernel<<<...>>>(query, database, indices, distances, k);
```

**Benefits:**
- Eliminate intermediate memory transfers
- Better GPU occupancy
- 2-3x faster end-to-end

---

### Adaptive HNSW Parameters
**Priority:** Medium
**Target Version:** v1.6.0

Automatically tune HNSW parameters (M, efConstruction, efSearch) based on workload.

**Features:**
- Monitor query latency and recall
- Adjust efSearch dynamically (higher for slow queries)
- Retrain index with better M if recall drops
- Learn optimal parameters per use case

**Algorithm:**
```cpp
// Adaptive tuning
void AdaptiveHNSW::tune() {
    auto metrics = collectMetrics();

    if (metrics.avg_latency > target_latency_ms) {
        efSearch = std::max(efSearch - 10, 10);  // Reduce for speed
    }

    if (metrics.recall < target_recall) {
        efSearch = std::min(efSearch + 10, 500);  // Increase for recall
    }

    if (metrics.recall < 0.90 && metrics.dataset_size > 1e6) {
        // Rebuild with higher M
        rebuild_with_M(M + 4);
    }
}
```

---

## Refactoring Opportunities

### Unified Index Interface
**Priority:** High
**Target Version:** v1.6.0

Extract common interface for all index types to enable polymorphism.

**Current State:** Separate manager classes with different APIs
**Desired State:** Unified `IIndex` interface

**Interface:**
```cpp
class IIndex {
public:
    virtual ~IIndex() = default;

    // CRUD operations
    virtual Status insert(const Key& key, const Value& value) = 0;
    virtual Status remove(const Key& key) = 0;
    virtual Status update(const Key& key, const Value& new_value) = 0;

    // Query operations
    virtual std::vector<Result> lookup(const Query& query) = 0;
    virtual std::vector<Result> range(const Key& start, const Key& end) = 0;

    // Maintenance
    virtual Status rebuild() = 0;
    virtual Status optimize() = 0;
    virtual Statistics getStats() = 0;
};

// Implementations
class BTreeIndex : public IIndex { ... };
class HNSWIndex : public IIndex { ... };
class RTreeIndex : public IIndex { ... };
```

**Benefits:**
- Generic query optimizer can choose best index
- Easier to add new index types
- Uniform testing and benchmarking

---

### Index Metadata Registry
**Priority:** Medium
**Target Version:** v1.6.0

Centralized registry for index metadata (type, stats, config).

**Current:** Metadata scattered across manager classes
**Desired:** Single source of truth

**Schema:**
```cpp
struct IndexMetadata {
    std::string name;
    std::string table;
    std::vector<std::string> columns;
    IndexType type;  // BTREE, HNSW, RTREE, etc.
    nlohmann::json config;

    // Statistics
    size_t num_entries;
    size_t size_bytes;
    Timestamp created_at;
    Timestamp last_updated;

    // Performance metrics
    uint64_t query_count;
    double avg_query_latency_ms;
    double cache_hit_rate;
};

class IndexRegistry {
public:
    void registerIndex(const IndexMetadata& metadata);
    std::optional<IndexMetadata> getMetadata(std::string_view name);
    std::vector<IndexMetadata> listIndexes(std::string_view table);
    void updateStats(std::string_view name, const Statistics& stats);
};
```

**Benefits:**
- Query planner can make informed decisions
- Easy to monitor index health
- Enables index usage analytics

---

### Zero-Copy Index Serialization
**Priority:** Medium
**Target Version:** v1.7.0

Eliminate memory copies when persisting/loading indexes.

**Current:** Serialize to temp buffer, then write to RocksDB
**Desired:** Direct memory-mapped I/O

**Techniques:**
- **Memory-Mapped Files**: `mmap()` for zero-copy reads
- **Shared Memory**: Cross-process index sharing
- **Cap'n Proto**: Zero-copy serialization format

**Example:**
```cpp
// Memory-mapped index file
class MmappedIndex {
    void* data_;
    size_t size_;

public:
    MmappedIndex(const std::string& path) {
        int fd = open(path.c_str(), O_RDONLY);
        struct stat sb;
        fstat(fd, &sb);
        size_ = sb.st_size;
        data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
    }

    // Direct access, no copies
    const VectorIndex* getIndex() const {
        return static_cast<const VectorIndex*>(data_);
    }
};
```

**Benefits:**
- 10-100x faster index loading
- Lower memory usage
- Better resource utilization

---

## Known Issues

### Issue: HNSW Vector Deletion Not Supported
**Severity:** Medium
**Impact:** Must rebuild index to remove vectors

**Current Workaround:** Mark as deleted, filter results
```cpp
// Soft delete: mark as deleted
deleted_ids_.insert(pk);

// Filter during search
auto raw_results = hnsw_index_.search(query, k + deleted_ids_.size());
std::vector<Result> filtered;
for (const auto& r : raw_results) {
    if (!deleted_ids_.contains(r.id)) {
        filtered.push_back(r);
    }
}
```

**Proper Solution (v1.7.0):**
- Implement HNSW node removal with graph repair
- Rebalance layers after deletion
- Trigger rebuild when >20% deleted

**References:**
- HNSWlib issue: https://github.com/nmslib/hnswlib/issues/93

---

### Issue: Spatial Index Polygon Intersection Approximation
**Severity:** Low
**Impact:** False positives in complex polygon queries

**Current Behavior:** Use MBR (bounding box) approximation
```cpp
// Query: Find polygons intersecting with polygon P
// Current: Returns all polygons whose MBR intersects P's MBR
// Problem: May return polygons that don't actually intersect P
```

**Workaround:** Post-filter with exact geometry check
```cpp
auto candidates = spatial.searchIntersects("polygons", query_mbr);

std::vector<SpatialResult> exact_matches;
for (const auto& candidate : candidates) {
    auto geom = loadGeometry(candidate.primary_key);
    if (exactIntersection(geom, query_polygon)) {
        exact_matches.push_back(candidate);
    }
}
```

**Proper Solution (v1.7.0):**
- Implement exact polygon intersection tests
- Use GEOS library for computational geometry
- Cache MBR checks, only exact-check close candidates

---

### Issue: Composite Index Column Order Matters
**Severity:** Medium
**Impact:** Query must match index column order for efficiency

**Problem:**
```cpp
// Index: (country, city, zip)
sim.createCompositeIndex("addresses", {"country", "city", "zip"});

// Efficient: Matches prefix
sim.lookupByIndex("addresses", {"country", "city"}, {"USA", "SF"});

// Inefficient: Skips "city", can't use index
sim.lookupByIndex("addresses", {"country", "zip"}, {"USA", "94102"});
```

**Workaround:** Create multiple indexes for different query patterns
```cpp
sim.createCompositeIndex("addresses", {"country", "city", "zip"});
sim.createCompositeIndex("addresses", {"country", "zip"});
```

**Proper Solution (v1.7.0):**
- Permutation indexes: Automatically create multiple orderings
- Smarter query planner: Detect partial matches
- Index intersection: Merge results from multiple indexes

---

### Issue: GPU Index Limited by VRAM
**Severity:** High
**Impact:** Cannot index >10M vectors on 8GB GPU

**Current Limits:**
- 8GB VRAM: ~10M vectors (768D)
- 16GB VRAM: ~20M vectors (768D)
- 24GB VRAM: ~30M vectors (768D)

**Workaround:** Use CPU fallback or multi-GPU
```cpp
if (dataset_size * dim * sizeof(float) > gpu_vram_mb) {
    config.backend = GPUVectorIndex::Backend::CPU;
}
```

**Proper Solution (v1.6.0):**
- GPU memory oversubscription (see Planned Features)
- Multi-GPU sharding
- Quantization to reduce memory (PQ/BQ)

---

## Research Areas

### Quantum-Inspired Optimization
**Timeframe:** 2-3 years
**Potential:** 10-100x speedup for specific problems

Explore quantum-inspired algorithms for combinatorial optimization in indexing.

**Applications:**
- **Traveling Salesman**: Optimize index node insertion order
- **Graph Partitioning**: Shard graph indexes optimally
- **K-means Clustering**: Faster IVF training

**References:**
- Quantum annealing for combinatorial optimization
- Simulated annealing / Metropolis-Hastings
- Tensor network methods

---

### Neuromorphic Index Structures
**Timeframe:** 3-5 years
**Potential:** Ultra-low-power indexing for edge devices

Explore brain-inspired computing for energy-efficient indexing.

**Concept:**
- Spiking Neural Networks (SNNs) for approximate search
- Associative memory for content-addressable indexes
- Event-driven updates (only compute on change)

**Benefits:**
- 100-1000x lower power consumption
- Ideal for edge/IoT deployments
- Naturally approximate (trade accuracy for efficiency)

**Challenges:**
- Immature hardware (Intel Loihi, IBM TrueNorth)
- Difficult to program
- Limited tooling

---

### Homomorphic Encryption for Encrypted Indexes
**Timeframe:** 2-3 years
**Potential:** Enable querying without decryption

Allow clients to search encrypted data without server seeing plaintext.

**Use Cases:**
- Healthcare: Search medical records without HIPAA violations
- Finance: Query transactions without exposing sensitive data
- Privacy-preserving analytics

**Challenges:**
- 100-10000x slower than plaintext
- Limited operations (addition, multiplication only)
- Large ciphertext sizes

**References:**
- Microsoft SEAL library
- Homomorphic Encryption Standardization Consortium
- Fully Homomorphic Encryption (FHE)

---

## Migration Paths

### Migrating from v1.4.x to v1.5.x
**Breaking Changes:** FAISS integration changes API

**Steps:**
1. **Update dependencies**
   ```bash
   vcpkg install faiss
   cmake -DTHEMIS_HAS_FAISS=ON ..
   ```

2. **Update VectorIndexManager initialization**
   ```cpp
   // Old (v1.4.x)
   vim.init("embeddings", 1536, VectorIndexManager::Metric::COSINE);

   // New (v1.5.x) - same, but supports advanced config
   vim.init("embeddings", 1536, VectorIndexManager::Metric::COSINE);

   // Optional: Enable IVF+PQ
   VectorIndexManager::AdvancedIndexConfig adv;
   adv.enabled = true;
   vim.setAdvancedIndexConfig(adv);
   ```

3. **Rebuild indexes for FAISS support**
   ```cpp
   // Rebuild existing indexes to use FAISS
   vim.rebuildIndex("embeddings");
   ```

**Compatibility:** v1.4.x indexes still work, but won't use FAISS optimizations

---

### Migrating from Single-Node to Distributed
**Timeframe:** v1.7.0+
**Effort:** High (requires schema changes)

**Preparation:**
1. **Design shard key**
   ```cpp
   // Choose sharding strategy
   // Option 1: Hash-based (uniform distribution)
   size_t shard = hash(pk) % num_shards;

   // Option 2: Range-based (preserves locality)
   size_t shard = rangeMap(pk, shard_boundaries);

   // Option 3: Tenant-based (multi-tenancy)
   size_t shard = tenantIdToShard(tenant_id);
   ```

2. **Plan replication**
   ```cpp
   // Replicate each shard N times
   config.replication_factor = 3;
   config.consistency_level = ConsistencyLevel::QUORUM;  // 2/3 replicas
   ```

3. **Test distributed queries**
   ```cpp
   // Test scatter-gather performance
   auto results = vim.search(query_vector, 10);
   // Measure: network latency, merge overhead
   ```

**Rollout:**
1. Deploy sharded cluster in parallel
2. Backfill data to shards
3. Switch queries to sharded cluster
4. Decommission single-node

---

### Migrating to Learned Indexes
**Timeframe:** v1.8.0+
**Effort:** Medium (training required)

**Steps:**
1. **Collect training data**
   ```cpp
   // Sample keys for CDF learning
   auto keys = sim.sampleKeys("users", "age", 10000);
   std::sort(keys.begin(), keys.end());
   ```

2. **Train learned index model**
   ```cpp
   LearnedIndex::Config config;
   config.model_type = ModelType::NEURAL_NETWORK;
   config.hidden_layers = {128, 64, 32};

   auto learned_index = LearnedIndex::train(keys, config);
   ```

3. **A/B test performance**
   ```cpp
   // Compare: B-tree vs Learned Index
   auto btree_latency = benchmark(btree_index, queries);
   auto learned_latency = benchmark(learned_index, queries);

   if (learned_latency < btree_latency) {
       // Migrate to learned index
       sim.replaceIndex("users", "age", learned_index);
   }
   ```

**When to Use:**
- Read-heavy workloads (>90% reads)
- Stable key distributions
- Large datasets (>1M keys)
- Latency-sensitive queries

**When NOT to Use:**
- Write-heavy workloads
- Skewed/changing distributions
- Small datasets (<100K keys)
- Requires exact results

## Test Strategy

- Unit test coverage ≥ 80% across HNSW, secondary index manager, and spatial index (tracked by Issue #1882)
- HNSW recall@10 regression test: must be ≥ 0.95 on SIFT-1M cosine benchmark after every build (Issue #1883)
- Integration tests: vector insert → search round-trip, spatial containment queries, full-text inverted index search, partial index predicate filtering
- GPU/CPU parity tests: HNSW search results must match within float32 tolerance (≤ 1e-5 L2 distance delta)
- Multi-tenancy isolation test: cross-tenant key lookup must return empty result, not an error or another tenant's data
- Online rebuild test: concurrent reads during rebuild must return consistent results with no QPS drop > 10%
- Property-based tests: learned index must return exact matches for all inserted keys under randomized key distributions

## Performance Targets

- HNSW vector search (1M 128-dim float32 vectors, k=10): ≥ 5,000 QPS on CPU; ≥ 50,000 QPS on GPU (RTX-class)
- HNSW recall@10 on SIFT-1M (cosine): ≥ 0.95
- B-tree secondary index point lookup: < 500 µs p99 for 10M-key index
- R-tree spatial range query (1M points, 1% selectivity): < 10 ms p99
- GPU index build for 1M 128-dim vectors: < 60 s on RTX-class GPU
- Online index rebuild read QPS degradation: < 10% during rebuild
- RocksDB WriteBatch commit latency for vector add: < 2 ms p99

## Security / Reliability

- RocksDB key-prefix tenant isolation must be enforced at every `IndexManager` entry point; direct key access bypassing the manager layer is prohibited
- GPU memory safety: CUDA/Vulkan buffers must be bounds-checked; out-of-bounds access must raise a recoverable error, not a crash
- Audit log entries for all vector `add`, `delete`, and `search` operations must include tenant ID, timestamp, and operation type
- Index corruption detected via RocksDB checksum validation must trigger read-only mode and alert before serving queries
- Partial (filtered) index predicates must be validated at creation time to prevent injection via predicate strings
- Online rebuild must maintain a dual-index window (old + new) with atomic cutover; no gap in query coverage
