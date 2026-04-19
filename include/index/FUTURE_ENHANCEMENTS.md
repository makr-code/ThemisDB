# Index Module - Future Enhancements

## Scope

- API-level enhancements to `include/index/` public C++ headers
- HNSW builder API with async construction and progress callbacks
- Product quantization (PQ) compression interface for fp32 vector storage reduction
- GPU-accelerated ANN query API (compile-time optional via `THEMIS_ENABLE_GPU`)
- Composite index interface for multi-column query patterns
- Learned index API replacing B-tree with ML-predicted positions
- Unified `IIndex` polymorphic interface for all index types

## Design Constraints

- [ ] HNSW build API is asynchronous; callers receive a `std::future<Result<void>>` or progress callback
- [ ] GPU ANN query API is compile-time optional; guarded by `#ifdef THEMIS_ENABLE_GPU`
- [ ] Existing `IIndex` interface must not be modified in a breaking way; only additive changes permitted
- [ ] PQ compression interface operates only on fp32 vectors; other types require explicit cast
- [ ] Learned index API requires explicit `train()` call before `lookup()`; state machine enforced
- [ ] All new index APIs return `Result<T>`; no raw exceptions cross public header boundaries

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IIndex` | Query planner, benchmarks | Unified polymorphic index base |
| `HNSWIndex` | `VectorIndexManager` | Async build, async search |
| `GPUVectorIndex` | ANN query engine | `THEMIS_ENABLE_GPU` guarded |
| `LearnedIndex` | `SecondaryIndexManager` | Requires `train()` before use |
| `IndexRegistry` | Query optimizer, monitoring | Centralized index metadata |
| `IShardingStrategy` | `IndexManager` | Pluggable hash/range partitioning |

## Planned Features

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
- `include/index/index_compression.h` / `src/index/index_compression.cpp` — all five techniques
- `include/index/secondary_index.h` — `SecondaryIndexManager::Config` with compression fields

**Tests:** `tests/index/test_index_compression.cpp` — 30+ focused tests
**CI:** `.github/workflows/index-compression-ci.yml`

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

---

## Test Strategy

- Compliance suite for `IIndex`: every concrete implementation must pass insert/lookup/range/rebuild contract tests
- Unit tests for PQ compression: round-trip encode/decode error ≤ configured tolerance; ≥ 10× size reduction verified
- GPU index tests run conditionally when `THEMIS_ENABLE_GPU` is set; CPU fallback path always tested
- Regression tests confirming `IIndex` additions are backward-compatible with v1.4.x client code
- Property-based tests for `LearnedIndex`: lookup always returns a result within `error_bound` positions
- Header-only compilation tests: each planned header compiles in isolation without `src/` includes

## Performance Targets

- HNSW query API dispatch overhead: ≤ 500 ns (excluding actual ANN search time)
- PQ compression: ≥ 10× storage reduction for fp32 vectors at default quality settings
- `IIndex::lookup` virtual dispatch overhead: ≤ 100 ns above direct concrete-type call
- GPU ANN query throughput: ≥ 200K queries/sec on 8 GB VRAM with 768-dim vectors
- `IndexRegistry::getMetadata` lookup: ≤ 1 µs (hash-map backed)
- Learned index lookup: ≤ 50% of equivalent B-tree lookup latency for stable key distributions

## Security / Reliability

- Input dimension validated against registered schema on every `insert()` call; mismatches return `Error::DimensionMismatch`
- GPU memory bounds enforced via `memory_limit_mb` in `GPUVectorIndex::Config`; exceeding limit returns `Error::OutOfMemory`
- Index build from untrusted data validates all vector dimensions before any GPU/CPU memory allocation
- `LearnedIndex` model files validated for integrity (checksum) before `loadModel()` proceeds
- `IIndex::rebuild()` is idempotent and safe to call concurrently with read queries
- No raw pointers exposed in public index headers; all ownership expressed via `std::unique_ptr` or `std::shared_ptr`
