> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-05-13 -->
<!-- Audit: see §Review / Audit Trail at end of document -->

# Vector Advanced Features

> **Scope of this document:** Advanced vector-index features beyond the baseline
> `VectorIndexManager` API.
> For basic usage (`init`, `addVector`, `search`, `deleteVector`) refer to
> [README.md](README.md) — the *src/index* implementation guide — and
> [include/index/README.md](../../include/index/README.md) — the public-API
> reference.

## Status

| Dimension | Value |
|---|---|
| Module version | v1.8.0 |
| Last validated | 2026-05-13 |
| Overall status | ✅ Production-Ready |
| Baseline doc | [README.md](README.md) |
| Public-API doc | [include/index/README.md](../../include/index/README.md) |

## Basic vs. Advanced: Feature Boundary

| Feature | Where documented |
|---|---|
| `VectorIndexManager::init / addVector / search / deleteVector` | [README.md](README.md) |
| HNSW parameters (`M`, `efConstruction`, `efSearch`) | [README.md](README.md) |
| RocksDB persistence, `WriteBatch` atomicity | [README.md](README.md) |
| Audit logging for vector operations | [README.md](README.md) |
| **FAISS IVF+PQ (`AdvancedVectorIndex`)** | **This document** |
| **GPU vector search (`GPUVectorIndex`, `MultiGPUVectorIndex`)** | **This document** |
| **GPU memory oversubscription** | **This document** |
| **Quantization (PQ, Binary, Residual, Learned)** | **This document** |
| **Approximate radius search** | **This document** |
| **Multi-vector / hybrid search** | **This document** |
| **Matryoshka embedding truncation (MRL)** | **This document** |
| **HNSW parameter auto-tuning** | **This document** |
| **Distributed vector index** | **This document** |

---

## 1. FAISS Advanced Vector Index (`AdvancedVectorIndex`)

**Header:** `include/index/advanced_vector_index.h`
**Source:** `src/index/advanced_vector_index.cpp`
**Status:** ✅ Production-Ready
**Build flag:** `THEMIS_GPU_ENABLED` — required for real FAISS; without it a
`StubCallbacks` bridge must be registered (see §1.5).

### 1.1 Description

`AdvancedVectorIndex` wraps FAISS with IVF (Inverted File) + PQ (Product
Quantization) for production-scale approximate nearest-neighbor search:

- **10–100× memory reduction** vs. flat index
- **2–10× faster search** on datasets larger than 1 M vectors
- GPU acceleration via FAISS GPU index
- ADC (Asymmetric Distance Computation) tables for ~40 % faster CPU search (v1.5.x)
- Workload-optimized configuration factory

### 1.2 Index Types

| Type | Memory | Speed | Accuracy | Best For |
|---|---|---|---|---|
| `IVF_FLAT` | High | High | Exact | Medium datasets, recall-critical |
| `IVF_PQ` | Low | Very High | Approximate | Large datasets, memory-constrained |
| `HNSW_FLAT` | High | Highest | Exact | Best accuracy, unlimited memory |
| `IVF_HNSW_PQ` | Medium | High | High | Best speed/accuracy trade-off |

### 1.3 Configuration Guide

```cpp
#include "index/advanced_vector_index.h"

// --- Workload-optimized factory (recommended) ---
// Dataset: 5 M documents, 768-dim embeddings, RAG workload
auto config = AdvancedVectorIndex::getWorkloadOptimizedConfig(
    5'000'000,                           // dataset size
    768,                                 // dimension
    AdvancedVectorIndex::WorkloadType::RAG
);

AdvancedVectorIndex adv_index(768, config);

// --- Manual configuration ---
AdvancedVectorIndex::Config cfg;
cfg.index_type   = AdvancedVectorIndex::Config::Type::IVF_PQ;
cfg.nlist        = 2048;    // IVF clusters — sqrt(N) is a good default
cfg.nprobe       = 128;     // Clusters searched per query (speed vs. recall)
cfg.use_pq       = true;
cfg.pq_m         = 8;       // Sub-quantizers (dimension % pq_m == 0)
cfg.pq_nbits     = 8;       // Bits per sub-quantizer (8 or 16)
cfg.use_adc_tables = true;  // ~40 % faster search on CPU (v1.5.x ADC)
cfg.use_gpu      = false;   // Set true to use GPU (requires FAISS GPU build)
cfg.train_size   = 150'000; // Training vectors — minimum 30 × nlist

AdvancedVectorIndex adv_index(768, cfg);
// Verify construction succeeded (GPU/FAISS availability affects initialization)
auto init_stats = adv_index.getStats();
// init_stats.is_trained == false until train() is called;
// init_stats.is_gpu reflects whether GPU was actually enabled.
```

### 1.4 Lifecycle

```cpp
// 1. Train on representative sample (required for IVF-based types)
std::vector<float> training_data(150'000 * 768);
// ... fill training_data from your corpus ...
bool trained = adv_index.train(training_data.data(), 150'000);

// 2. Add vectors with IDs
std::vector<float>   vectors(10'000 * 768);
std::vector<int64_t> ids(10'000);
// ... fill vectors/ids ...
adv_index.addWithIds(vectors.data(), ids.data(), 10'000);

// 3. Search
AdvancedVectorIndex::SearchResult result = adv_index.search(query.data(), /*k=*/10);
for (size_t i = 0; i < result.ids.size(); ++i) {
    std::cout << "ID " << result.ids[i]
              << "  dist " << result.distances[i] << "\n";
}

// 4. Persist / restore
adv_index.save("/data/indexes/adv_768.faiss");
adv_index.load("/data/indexes/adv_768.faiss");

// 5. Statistics
auto stats = adv_index.getStats();
std::cout << "Vectors: "      << stats.total_vectors       << "\n"
          << "Memory (MB): "  << stats.memory_usage_bytes / 1'048'576 << "\n"
          << "Compression: "  << stats.compression_ratio   << "×\n"
          << "Trained: "      << stats.is_trained           << "\n";
```

### 1.5 StubCallbacks (non-FAISS environments)

When FAISS is unavailable at compile time, the implementation uses
**injectable `StubCallbacks`** so tests and CI can verify integration logic
without a real FAISS library:

```cpp
// STUB/SIMULATION NOTE:
// Purpose: allow unit tests and CI to exercise AdvancedVectorIndex APIs
//          without a FAISS build.
// Activation: StubCallbacks registered; no THEMIS_GPU_ENABLED define.
// Production Delta: no real vector math — returns configured dummy results.
// Removal Plan: replaced by real FAISS dispatch when build flag is present.

AdvancedVectorIndex::StubCallbacks stubs;
stubs.train        = [](const float*, size_t) { return true; };
stubs.add_with_ids = [](const float*, const int64_t*, size_t) { return true; };
stubs.search       = [](const float*, size_t k) {
    AdvancedVectorIndex::SearchResult r;
    r.ids.resize(k, -1);
    r.distances.resize(k, 0.0f);
    return r;
};
stubs.stats = [] {
    AdvancedVectorIndex::Stats s;
    s.is_trained = true;
    s.total_vectors = 42;
    return s;
};
AdvancedVectorIndex::setStubCallbacks(std::move(stubs));
```

> **Important:** Register stubs before construction. Pass a
> default-constructed `StubCallbacks` to clear all hooks and restore
> fail-closed behavior.

### 1.6 Performance Targets

| Workload | Dataset | Dimension | Target Throughput | Target Recall@10 |
|---|---|---|---|---|
| RAG | 10 M docs | 768 | ≥ 2 000 QPS (CPU) | ≥ 0.90 |
| Analytics batch | 50 M docs | 1 536 | ≥ 500 QPS (GPU) | ≥ 0.95 |
| OLTP | 1 M docs | 384 | ≥ 10 000 QPS (CPU) | ≥ 0.92 |

---

## 2. GPU Vector Index

### 2.1 Single-GPU (`GPUVectorIndex`)

**Header:** `include/index/gpu_vector_index.h`
**Source:** `src/index/gpu_vector_index.cpp`
**Status:** ✅ Production-Ready
**Build flags:** `THEMIS_ENABLE_CUDA` (CUDA backend) or `THEMIS_ENABLE_HIP` (ROCm/HIP backend) or `THEMIS_ENABLE_VULKAN` (Vulkan compute backend)

```cpp
#include "index/gpu_vector_index.h"

GPUVectorIndex::Config gpu_cfg;
gpu_cfg.device_id         = 0;
gpu_cfg.backend           = GPUVectorIndex::Backend::CUDA;  // or HIP / VULKAN
gpu_cfg.allow_cpu_fallback = true;  // auto-fallback when GPU is unavailable

GPUVectorIndex gpu_index(768, gpu_cfg);
gpu_index.add(vectors.data(), count);

// Batch search — maximises GPU throughput
auto results = gpu_index.searchBatch(queries.data(), num_queries, /*k=*/10);
```

**CPU fallback behavior:** When the GPU runtime is unavailable or the
requested backend is not compiled in, `GPUVectorIndex` automatically falls
back to CPU HNSW and emits a `spdlog::warn` message.  No exception is thrown.

### 2.2 Multi-GPU (`MultiGPUVectorIndex`)

**Header:** `include/index/multi_gpu_vector_index.h`
**Source:** `src/index/multi_gpu_vector_index.cpp`
**Status:** ✅ Production-Ready

```cpp
#include "index/multi_gpu_vector_index.h"

MultiGPUVectorIndex::Config cfg;
cfg.device_ids       = {0, 1, 2, 3};  // Use all 4 GPUs
cfg.replication_mode = MultiGPUVectorIndex::ReplicationMode::FULL;

MultiGPUVectorIndex multi_gpu(768, cfg);
multi_gpu.add(vectors.data(), count);

// Parallel batch search across all GPUs
auto results = multi_gpu.searchBatch(queries.data(), num_queries, /*k=*/10);

// Statistics per GPU
auto stats = multi_gpu.getStats();
for (const auto& s : stats.per_device) {
    std::cout << "GPU " << s.device_id
              << "  utilisation " << s.utilisation_pct << " %\n";
}
```

### 2.3 GPU Memory Oversubscription

**Header:** `include/index/gpu_memory_oversubscription.h`
**Source:** `src/index/gpu_memory_oversubscription.cpp`
**Status:** ✅ Production-Ready (v1.7.0)
**Tests:** `tests/index/test_gpu_memory_oversubscription.cpp` (26 tests)

Enables vector indexes larger than available VRAM via LRU-eviction paging and
prefetch strategies:

```cpp
#include "index/gpu_memory_oversubscription.h"

GPUVectorIndex::Config cfg;
cfg.enable_oversubscription = true;
cfg.vram_budget_mb          = 8'192;  // 8 GB VRAM budget
cfg.prefetch_strategy       = GPUMemoryOversubscriptionManager::PrefetchStrategy::LRU;

GPUVectorIndex gpu_index(1536, cfg);

// Monitor paging activity
auto os_stats = gpu_index.getOversubscriptionStats();
std::cout << "Page hits:   " << os_stats.page_hits   << "\n"
          << "Page misses: " << os_stats.page_misses  << "\n"
          << "Evictions:   " << os_stats.evictions    << "\n";
```

**Prefetch strategies:** `NONE`, `LRU`, `MRU`, `SEQUENTIAL`

**Performance note:** Oversubscription adds per-query VRAM-page-fault overhead.
Set `vram_budget_mb` to at least 80 % of the working-set size for best throughput.

---

## 3. Quantization

All quantizers compress stored vectors to reduce memory footprint at the cost
of a small recall drop.  They integrate with `VectorIndexManager` via
`AdvancedIndexConfig`.

### 3.1 Product Quantization with ADC (`ProductQuantizer`)

**Header:** `include/index/product_quantizer.h`
**Status:** ✅ Production-Ready

```cpp
#include "index/product_quantizer.h"

ProductQuantizer::Config pq_cfg;
pq_cfg.m      = 8;   // Sub-quantizers (dimension % m == 0)
pq_cfg.nbits  = 8;   // Bits per sub-quantizer
pq_cfg.use_adc_tables = true;  // Asymmetric Distance Computation (v1.5.x)

ProductQuantizer pq(768, pq_cfg);
pq.train(training_data.data(), 100'000);

// Encode
std::vector<uint8_t> codes(count * pq.getCodeSize());
pq.encode(vectors.data(), codes.data(), count);

// Search with ADC (no full decode — ~3× faster than decode path)
auto results = pq.searchADC(query.data(), codes.data(), count, /*k=*/10);
```

**ADC optimization:** Asymmetric Distance Computation (ADC) computes
distances directly from sub-quantizer lookup tables without decoding full
vectors.  This delivers 3–5× speedup for PQ-compressed search on CPU.

### 3.2 Binary Quantization (`BinaryQuantizer`)

**Header:** `include/index/binary_quantizer.h`
**Status:** ✅ Production-Ready

256× compression ratio; XOR + popcount distance.  Best for recall@1 screening
before full re-ranking with the original float vectors.

```cpp
#include "index/binary_quantizer.h"

BinaryQuantizer bq(768);
bq.train(training_data.data(), 100'000);

std::vector<uint8_t> binary_codes(count * bq.getCodeSize());
bq.encode(vectors.data(), binary_codes.data(), count);

auto candidates = bq.search(query.data(), binary_codes.data(), count, /*k=*/100);
// Re-rank candidates with original float vectors ...
```

### 3.3 Residual Quantization (`ResidualQuantizer`)

**Header:** `include/index/residual_quantizer.h`
**Status:** ✅ Production-Ready

Multi-stage quantization that encodes the residual error of each stage,
achieving higher accuracy than PQ at the same code length.

### 3.4 Learned Quantizer (`LearnedQuantizer`)

**Header:** `include/index/learned_quantizer.h`
**Status:** ✅ Production-Ready

Neural-network-based quantization trained end-to-end on the target
distribution.

> **Open optimization (FUTURE_ENHANCEMENTS.md):** `learned_quantizer.cpp`
> line 353 contains a TODO for ADC-style distance computation directly from
> codes/centroids, bypassing full decode.  When implemented this will deliver
> 3–5× speedup matching the PQ ADC path.

---

## 4. Approximate Radius Search (`ApproximateRadiusSearch`)

**Header:** `include/index/approximate_radius_search.h`
**Source:** `src/index/approximate_radius_search.cpp`
**Status:** ✅ Production-Ready
**Tests:** `tests/test_approximate_radius_search_integration.cpp`

Efficient search for **all vectors within a distance threshold**, as opposed
to k-NN which returns a fixed count.

### 4.1 Basic Usage

```cpp
#include "index/vector_index.h"
#include "index/approximate_radius_search.h"

using namespace themis::vector;

// Initialize VectorIndexManager (basic setup — see README.md)
VectorIndexManager vector_manager(db);
vector_manager.init("documents", 768, VectorIndexManager::Metric::COSINE);

// Create radius search module
ApproximateRadiusSearch radius_search(vector_manager);

ApproximateRadiusSearch::SearchConfig cfg;
cfg.radius      = 0.3f;   // Cosine distance threshold
cfg.metric      = Metric::COSINE;
cfg.max_results = 1000;
cfg.min_recall  = 0.95f;  // 95 % recall guarantee

auto result = radius_search.search(query_vector, cfg);
if (result) {
    for (const auto& item : result.value().results) {
        std::cout << item.id << ": " << item.distance << "\n";
    }
}
```

### 4.2 Batch Search

```cpp
std::vector<std::vector<float>> batch_queries = { q1, q2, q3 };
auto batch_results = radius_search.searchBatch(batch_queries, cfg);
```

### 4.3 k-NN vs. Radius Search

| Aspect | k-NN | Radius Search |
|---|---|---|
| Result count | Fixed (k) | Variable (all within radius) |
| Query parameter | k neighbors | Distance threshold |
| Use case | "Top-k similar" | "All within threshold" |
| Performance | O(log n) HNSW | O(log n + r); r = result count |

### 4.4 Use Cases

- Finding all similar documents within a similarity threshold
- Duplicate detection with distance threshold
- Clustering pre-processing (neighborhood discovery)
- Anomaly detection (items with very few neighbors)
- Local density estimation

### 4.5 Algorithm Complexity

| Operation | Time | Space | Notes |
|---|---|---|---|
| Search | O(log n + r) | O(r) | r = results in radius |
| Batch Search | O(m × (log n + r̄)) | O(m × r̄) | m = queries, r̄ = avg result count |
| Estimate Count | O(log n × s) | O(1) | s = sample size |

### 4.6 Performance Tips

1. Tune `min_recall` — lower values accelerate search at cost of missed results
2. Set `max_results` to bound memory when radius is broad
3. Use batch mode to amortize index traversal overhead across queries
4. For very large radii, consider `AdvancedVectorIndex` IVF+PQ instead

---

## 5. Multi-Vector / Hybrid Search (`MultiVectorSearch`)

**Header:** `include/index/multi_vector_search.h`
**Source:** `src/index/multi_vector_search.cpp`
**Status:** ✅ Production-Ready (all 7 fusion strategies implemented)
**Tests:** `tests/test_multi_vector_search.cpp`

### 5.1 Fusion Strategies

| Strategy | Formula | Tuning | Best For |
|---|---|---|---|
| `LINEAR_COMBINATION` | `score = Σ wᵢ·sᵢ` | Weights required | Known per-source importance |
| `RANK_FUSION` (Borda) | `score = Σ (n − rank_i)` | None | Unknown importance |
| `RRF` | `score = Σ 1/(k + rank_i)` | `k` param (default 60) | General purpose |
| `MAX` | `score = max(sᵢ)` | None | Any-match queries |
| `MIN` | `score = min(sᵢ)` | None | All-match queries |
| `AVG` | `score = mean(sᵢ)` | None | Equal-importance sources |
| `LEARNED_FUSION` | pre-optimized weights | Requires `optimizeWeights()` | Best recall with labelled data |

### 5.2 Usage Examples

```cpp
#include "index/multi_vector_search.h"

using namespace themis::vector;

MultiVectorSearch multi_search(vector_manager);

// Example 1: Multi-query ensemble (3 query formulations)
MultiVectorSearch::MultiQuery query;
query.vectors = {query_vec1, query_vec2, query_vec3};
query.weights = {0.5f, 0.3f, 0.2f};  // used by LINEAR_COMBINATION

MultiVectorSearch::SearchConfig cfg;
cfg.fusion = MultiVectorSearch::FusionStrategy::RRF;  // robust default
cfg.top_k  = 20;

auto result = multi_search.search(query, cfg);
if (result) {
    for (const auto& res : result.value().results) {
        std::cout << "ID: " << res.id << "  score: " << res.fused_score << "\n";
    }
}

// Example 2: Multi-field (title + content embeddings)
auto result2 = multi_search.searchMultiField(
    query_vector,
    {"title_embedding", "content_embedding"},
    cfg
);

// Example 3: Hybrid search (semantic + keyword BM25)
std::unordered_map<std::string, float> keyword_scores = {
    {"doc1", 0.8f}, {"doc2", 0.6f}
};
auto result3 = multi_search.hybridSearch(query_vector, keyword_scores, cfg);

// Example 4: Learned Fusion (requires labelled training data)
std::vector<MultiVectorSearch::MultiQuery> training_queries = { /* ... */ };
std::vector<std::vector<std::string>>      relevance_labels  = { /* ... */ };

auto learned_weights = multi_search.optimizeWeights(training_queries, relevance_labels);
if (learned_weights) {
    cfg.fusion  = MultiVectorSearch::FusionStrategy::LEARNED_FUSION;
    cfg.weights = learned_weights.value();
    auto result4 = multi_search.search(query, cfg);
}
```

> **LEARNED_FUSION note:** This strategy requires pre-computed weights from
> `optimizeWeights()`.  Calling `search()` without valid weights returns
> `INVALID_ARGUMENT` / "LEARNED_FUSION requires pre-computed weights from
> optimizeWeights()".

### 5.3 Use Cases

- Multi-modal search (text + image + audio)
- Ensemble retrieval (multiple query formulations / languages)
- Hybrid semantic + BM25 search
- Multi-aspect similarity (title, abstract, metadata)
- Recommendation systems (multiple user preference vectors)

### 5.4 Algorithm Complexity

| Operation | Time | Space | Notes |
|---|---|---|---|
| Linear / Max / Min / Avg | O(k × m × log n) | O(k × m) | k = top-k, m = vectors |
| Rank Fusion (Borda) | O(k × m × log n + k log k) | O(k × m) | Extra sort step |
| RRF | O(k × m × log n) | O(k × m) | Constant-time fusion |
| Hybrid Search | O(k × log n + h) | O(k + h) | h = keyword candidates |

### 5.5 Performance Tips

1. `RRF` is the safest default for unknown source weights
2. Tune `LINEAR_COMBINATION` weights with `optimizeWeights()` on labelled queries
3. Enable parallel search (`cfg.parallel = true`) when latency matters
4. Cache individual vector scores when re-running fusion experiments

---

## 6. Matryoshka Embedding Truncation (v1.8.0)

**Header:** `include/index/matryoshka_truncation.h`
**Source:** `src/index/matryoshka_truncation.cpp`
**Status:** ✅ Production-Ready (v1.8.0)
**Tests:** `tests/index/test_matryoshka_truncation.cpp` (25 tests, AC-1–AC-25)
**CI:** `.github/workflows/matryoshka-truncation-ci.yml`

### 6.1 Overview

Matryoshka Representation Learning (MRL) trains embeddings so that every
prefix sub-vector of length d < D is itself a useful d-dimensional
representation (Kusupati et al., NeurIPS 2022).  A single full-dimensional
embedding can be truncated to any standard granularity for multi-stage
retrieval without retraining.

```
Full: [·····················768·····················]
       ↳ 64 ↳ 128 ↳ 256 ↳ 512 ↳ 768  (any prefix is valid)
```

**Supported granularities (`kMRL_*` constants):**

| Constant | Dimensions | Compatible Models |
|---|---|---|
| `kMRL_64` | 64 | text-embedding-3-small, Nomic Embed v1.5, BGE-M3 |
| `kMRL_128` | 128 | same |
| `kMRL_256` | 256 | text-embedding-3-small, text-embedding-3-large, Nomic Embed v1.5, BGE-M3 |
| `kMRL_512` | 512 | same |
| `kMRL_768` | 768 | text-embedding-3-small (full), Nomic Embed v1.5 (full), BGE-M3 (full) |
| `kMRL_1024` | 1 024 | text-embedding-3-large |
| `kMRL_1536` | 1 536 | text-embedding-3-large, text-embedding-ada-002 (full) |

### 6.2 Usage

```cpp
#include "index/matryoshka_truncation.h"
#include "index/ann_index.h"

// Wrap any IAnnIndex backend with MRL truncation
auto hnsw_backend = std::make_shared<HnswAnnIndex>(768, hnsw_cfg);

MatryoshkaTruncatedIndex mrl_index(hnsw_backend, kMRL_256);
// ^ Stores and searches with 256-dim truncated + normalised embeddings

// Multi-stage retrieval
MatryoshkaTruncatedIndex coarse_index(hnsw_backend, kMRL_64);
MatryoshkaTruncatedIndex fine_index  (hnsw_backend, kMRL_768);

// Stage 1: cheap coarse candidate selection
auto candidates = coarse_index.search(query.data(), /*k=*/100);

// Stage 2: re-rank top-100 with full-dim embeddings
auto final_results = fine_index.rerank(candidates, query.data(), /*k=*/10);
```

### 6.3 Performance Characteristics

- Truncation + L2 normalisation: O(trunc_dim) per vector — negligible overhead
- No index-build overhead beyond the wrapped backend
- 64-dim index vs. 768-dim: ~12× smaller, ~8× faster ANN search
- Recommended pipeline: `kMRL_64` coarse filter → `kMRL_768` fine re-rank

---

## 7. HNSW Parameter Auto-Tuning

### 7.1 `HnswParameterTuner`

**Header:** `include/index/hnsw_parameter_tuner.h`
**Status:** ✅ Production-Ready

Selects optimal HNSW `M`, `efConstruction`, and `efSearch` parameters for a
given workload class at runtime:

```cpp
#include "index/hnsw_parameter_tuner.h"

HnswParameterTuner tuner;
auto params = tuner.tune(
    /*dataset_size=*/  2'000'000,
    /*dimension=*/     768,
    /*workload=*/      WorkloadClass::MIXED
);

VectorIndexManager vim(db);
vim.init("docs", 768, VectorIndexManager::Metric::COSINE,
         params.M, params.efConstruction, params.efSearch);
```

### 7.2 `HnswProductionDefaults`

**Header:** `include/index/hnsw_production_defaults.h`
**Status:** ✅ Production-Ready

Pre-validated parameter sets for common dataset sizes:

| Preset | Dataset Size | M | efConstruction | efSearch | Recall@10 |
|---|---|---|---|---|---|
| `SMALL` | < 100 K | 16 | 200 | 64 | ≥ 0.98 |
| `MEDIUM` | 100 K – 5 M | 32 | 400 | 128 | ≥ 0.95 |
| `LARGE` | > 5 M | 48 | 600 | 256 | ≥ 0.92 |

### 7.3 `HnswLayerOptimizer`

**Header:** `include/index/hnsw_layer_optimizer.h`
**Status:** ✅ Production-Ready

Optimises the HNSW layer structure post-build to improve query throughput
without changing recall.

---

## 8. Distributed Vector Index

**Header:** `include/index/distributed_vector_index.h`
**Source:** `src/index/distributed_vector_index.cpp`
**Status:** ✅ Production-Ready

Partitions vector indexes across multiple shards for horizontal scale-out.

```cpp
#include "index/distributed_vector_index.h"

DistributedVectorIndex::Config dist_cfg;
dist_cfg.shard_count = 8;
dist_cfg.replication_factor = 2;

DistributedVectorIndex dist_index(768, dist_cfg);
dist_index.addWithIds(vectors.data(), ids.data(), count);

// Scatter-gather search across all shards
auto results = dist_index.search(query.data(), /*k=*/10);
```

**Use cases:** datasets > 100 M vectors; multi-region deployments; tenant
isolation via shard-level partitioning.

---

## 9. Error Handling

All advanced-search methods use `Result<T>` / `themis::expected<T, Error>`:

```cpp
auto result = multi_search.search(query, cfg);
if (!result) {
    std::cerr << "Error: " << result.error().message << "\n";
    return;
}
const auto& sr = result.value();
std::cout << "Found " << sr.results.size() << " results\n";
```

**Common error codes:**

| Code | Cause | Resolution |
|---|---|---|
| `INVALID_ARGUMENT` | LEARNED_FUSION without weights | Call `optimizeWeights()` first |
| `INVALID_ARGUMENT` | Dimension mismatch | Check `init()` dimension vs. query |
| `NOT_TRAINED` | IVF search before training | Call `train()` before `add()` |
| `GPU_UNAVAILABLE` | GPU backend missing | Set `allow_cpu_fallback = true` |
| `OUT_OF_MEMORY` | VRAM exhausted | Enable oversubscription or reduce budget |

---

## 10. Testing

### Test Matrix

| Module | Test File | Tests | Type |
|---|---|---|---|
| AdvancedVectorIndex | `tests/index/test_advanced_vector_index.cpp` | — | Unit + Integration |
| GPUVectorIndex | `tests/index/test_gpu_vector_index.cpp` | — | Unit |
| GPU Memory Oversubscription | `tests/index/test_gpu_memory_oversubscription.cpp` | 26 | Unit |
| ApproximateRadiusSearch | `tests/test_approximate_radius_search_integration.cpp` | — | Integration |
| MultiVectorSearch | `tests/test_multi_vector_search.cpp` | — | Unit + Integration |
| MatryoshkaTruncation | `tests/index/test_matryoshka_truncation.cpp` | 25 | Unit |

### Running Tests

```bash
# Configure and build (Linux)
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release --parallel 4

# Run all index tests
ctest --preset linux-ninja-release -R "index" --output-on-failure

# Run specific test suites
./build/tests/test_approximate_radius_search_integration
./build/tests/test_multi_vector_search
./build/tests/index/test_matryoshka_truncation
./build/tests/index/test_gpu_memory_oversubscription
```

### Benchmarks

```bash
# Radius search throughput / scalability
./build/benchmarks/bench_approximate_radius_search

# FAISS IVF+PQ throughput vs. flat HNSW
./build/benchmarks/bench_advanced_vector_index
```

---

## 11. References

### Academic Papers

1. **HNSW**: Malkov, Y. A., & Yashunin, D. A. (2018). "Efficient and robust approximate nearest neighbor search using hierarchical navigable small world graphs." *IEEE TPAMI*.
2. **FAISS**: Johnson, J., Douze, M., & Jégou, H. (2019). "Billion-scale similarity search with GPUs." *IEEE Transactions on Big Data*.
3. **RRF**: Cormack, G. V., Clarke, C. L. A., & Buettcher, S. (2009). "Reciprocal rank fusion outperforms condorcet and individual rank learning methods." *SIGIR*.
4. **CombSUM / CombMNZ**: Fox, E. A., & Shaw, J. A. (1994). "Combination of multiple searches." *TREC*.
5. **MRL**: Kusupati, A. et al. (2022). "Matryoshka Representation Learning." *NeurIPS*.
6. **ADC**: Jégou, H., Douze, M., & Schmid, C. (2011). "Product quantization for nearest neighbor search." *IEEE TPAMI*.

### Comparable Systems

| System | Relevant Feature |
|---|---|
| FAISS | IVF+PQ, GPU search, ADC |
| Milvus | Multi-vector, distributed, GPU |
| Weaviate | Hybrid BM25 + vector |
| Qdrant | Payload filtering, scoring |
| ScaNN | Anisotropic PQ, ADC |

---

## 12. Related Documentation

| Document | Link |
|---|---|
| Index module — implementation guide | [src/index/README.md](README.md) |
| Index module — public-API reference | [include/index/README.md](../../include/index/README.md) |
| Index module roadmap | [src/index/ROADMAP.md](ROADMAP.md) |
| Index module future enhancements | [src/index/FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) |
| `VectorIndexManager` API | [include/index/vector_index.h](../../include/index/vector_index.h) |
| `AdvancedVectorIndex` API | [include/index/advanced_vector_index.h](../../include/index/advanced_vector_index.h) |
| `MatryoshkaTruncatedIndex` API | [include/index/matryoshka_truncation.h](../../include/index/matryoshka_truncation.h) |
| `Result<T>` / `expected<T,E>` | [include/utils/expected.h](../../include/utils/expected.h) |
| PathConstraints & Advanced Vectors (usage guide) | [docs/en/features/PATH_CONSTRAINTS_AND_ADVANCED_VECTORS.md](../../docs/en/features/PATH_CONSTRAINTS_AND_ADVANCED_VECTORS.md) |
| Documentation review guidelines | [docs/DOCUMENTATION_REVIEW_GUIDELINES.md](../../docs/DOCUMENTATION_REVIEW_GUIDELINES.md) |
| Systematic review plan | [docs/SYSTEMATISCHER_REVIEWPLAN.md](../../docs/SYSTEMATISCHER_REVIEWPLAN.md) |

---

## 13. Review / Audit Trail

<!-- Acceptance criteria from issue [Docs][Module] index - VECTOR_ADVANCED_FEATURES_README.md aktualisieren -->

| Criterion | Status |
|---|---|
| Content consistent with index module source headers | ✅ Verified against `advanced_vector_index.h`, `gpu_vector_index.h`, `matryoshka_truncation.h`, `approximate_radius_search.h`, `multi_vector_search.h` (2026-05-13) |
| Advanced/basic documentation clearly separated and cross-linked | ✅ Feature boundary table in §0; cross-links in §12 |
| Examples and performance hints updated | ✅ Updated in §§1–8 |
| Fachreview durchgeführt | ✅ Self-review against source-code audit (2026-05-13) |
| Sourcecode-/Dokumentationsaudit durchgeführt | ✅ All API signatures verified against include/index/ headers |
| Ergebnis verlinkt | ✅ Links in §12 point to canonical include/index/ paths |
| Betroffene Dateien im Review festgehalten | `src/index/VECTOR_ADVANCED_FEATURES_README.md` (primary); cross-references in `README.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md` |
