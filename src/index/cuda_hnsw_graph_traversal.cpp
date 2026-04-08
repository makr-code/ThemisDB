/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_hnsw_graph_traversal.cpp                      ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:16:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     617                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 539fd56b36  2026-03-18  fix(acceleration): address code review - replace __trap()... ║
    • 04753d4acd  2026-03-18  feat(acceleration): remove silent k>kMaxK clamping, incre... ║
    • e2fff830f0  2026-03-11  feat(acceleration): wire HNSW graph traversal into CUDAVe... ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file cuda_hnsw_graph_traversal.cpp
 * @brief CUDA HNSW graph traversal — host-side wiring and CPU fallback.
 *
 * When built with CUDA (THEMIS_ENABLE_CUDA defined by the build system)
 * the implementation allocates device memory and issues kernel launches via
 * the cuda_hnsw_kernels.cu device-side definitions in
 * src/acceleration/cuda/cuda_hnsw_kernels.cu.
 *
 * When built without CUDA the engine performs an equivalent greedy best-first
 * traversal entirely on the CPU so that all unit-tests and non-GPU deployments
 * remain functional without any conditional compilation at the call-site.
 */

#include "index/cuda_hnsw_graph_traversal.h"
#include "utils/logger.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
// Conditional CUDA headers
// ─────────────────────────────────────────────────────────────────────────────
#ifdef THEMIS_ENABLE_CUDA
#  include <cuda_runtime.h>
// Forward-declare device kernels (defined in src/acceleration/cuda/cuda_hnsw_kernels.cu)
namespace themis::cuda {
void launchHnswSearchKernel(const float* d_vectors, uint32_t dim,
                             const int32_t* d_offsets, const int32_t* d_neighbours,
                             uint32_t num_nodes,
                             const float* d_queries, uint32_t num_queries,
                             uint32_t k, uint32_t ef, uint8_t metric,
                             uint32_t entry_node,
                             int64_t* d_result_ids, float* d_result_scores,
                             cudaStream_t stream, bool* h_overflow,
                             uint8_t* d_visited_pool   = nullptr,
                             size_t   visited_pool_bytes = 0);
// Maximum k for a single GPU kernel pass (mirrors kMaxK in cuda_hnsw_kernels.cu)
static constexpr uint32_t kHnswKernelMaxK = 1024u;
} // namespace themis::cuda
#endif

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Distance helpers (CPU fallback)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

inline float l2Distance(const float* a, const float* b, uint32_t dim) {
    float acc = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        acc += d * d;
    }
    return acc;
}

inline float cosineDistance(const float* a, const float* b, uint32_t dim) {
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    if (denom < 1e-9f) return 1.0f;
    return 1.0f - dot / denom;
}

inline float dotDistance(const float* a, const float* b, uint32_t dim) {
    float dot = 0.0f;
    for (uint32_t i = 0; i < dim; ++i) dot += a[i] * b[i];
    return -dot;  // Negate so that "smaller = more similar" invariant holds
}

float computeDistance(const float* a, const float* b, uint32_t dim,
                      HnswDistanceMetric metric) {
    switch (metric) {
        case HnswDistanceMetric::Cosine: return cosineDistance(a, b, dim);
        case HnswDistanceMetric::Dot:    return dotDistance(a, b, dim);
        default:                         return l2Distance(a, b, dim);
    }
}

/**
 * @brief CPU greedy best-first HNSW search (ef-limited candidate heap).
 *
 * Traverses from the top layer down to layer 0, keeping a candidate list of
 * ef entries at the bottom layer, then extracts the top-k results.
 */
std::vector<HnswTraversalResult>
cpuHnswSearch(const std::vector<HnswLayerGraph>& layers,
              const std::vector<float>&           flat_vectors,
              uint32_t                            dim,
              const float*                        query,
              uint32_t                            k,
              uint32_t                            ef,
              HnswDistanceMetric                  metric)
{
    if (layers.empty() || flat_vectors.empty()) return {};

    // Entry point: node 0 at the top layer
    int32_t entry = 0;
    int num_layers = static_cast<int>(layers.size());

    // Traverse upper layers with ef=1
    for (int layer = num_layers - 1; layer > 0; --layer) {
        const HnswLayerGraph& g = layers[static_cast<size_t>(layer)];
        if (g.num_nodes == 0) continue;

        float best_dist = computeDistance(query,
                                          flat_vectors.data() + static_cast<size_t>(entry) * dim,
                                          dim, metric);
        bool changed = true;
        while (changed) {
            changed = false;
            int32_t start = g.offsets[static_cast<size_t>(entry)];
            int32_t end   = g.offsets[static_cast<size_t>(entry) + 1];
            for (int32_t ni = start; ni < end; ++ni) {
                int32_t nb = g.neighbours[static_cast<size_t>(ni)];
                if (nb < 0 || static_cast<uint32_t>(nb) >= g.num_nodes) continue;
                float d = computeDistance(query,
                                          flat_vectors.data() + static_cast<size_t>(nb) * dim,
                                          dim, metric);
                if (d < best_dist) {
                    best_dist = d;
                    entry = nb;
                    changed = true;
                }
            }
        }
    }

    // Bottom layer: ef-limited search
    const HnswLayerGraph& bottom = layers[0];
    if (bottom.num_nodes == 0) return {};

    using QEl = std::pair<float, int32_t>; // (dist, id)
    // candidates: min-heap (smallest distance at top)
    std::priority_queue<QEl, std::vector<QEl>, std::greater<QEl>> candidates;
    // results:    max-heap of size ef (largest distance at top, to evict)
    std::priority_queue<QEl> results;
    std::vector<bool> visited(bottom.num_nodes, false);

    auto enqueue = [&](int32_t node_id) {
        float d = computeDistance(query,
                                  flat_vectors.data() + static_cast<size_t>(node_id) * dim,
                                  dim, metric);
        candidates.push({d, node_id});
        results.push({d, node_id});
        if (results.size() > ef) results.pop();
    };

    visited[static_cast<size_t>(entry)] = true;
    enqueue(entry);

    while (!candidates.empty()) {
        auto [cd, cn] = candidates.top(); candidates.pop();
        if (!results.empty() && cd > results.top().first) break; // pruning

        int32_t start = bottom.offsets[static_cast<size_t>(cn)];
        int32_t end   = bottom.offsets[static_cast<size_t>(cn) + 1];
        for (int32_t ni = start; ni < end; ++ni) {
            int32_t nb = bottom.neighbours[static_cast<size_t>(ni)];
            if (nb < 0 || static_cast<uint32_t>(nb) >= bottom.num_nodes) continue;
            if (visited[static_cast<size_t>(nb)]) continue;
            visited[static_cast<size_t>(nb)] = true;
            enqueue(nb);
        }
    }

    // Collect results (sorted ascending by distance)
    std::vector<HnswTraversalResult> out;
    out.reserve(results.size());
    while (!results.empty()) {
        auto [d, id] = results.top(); results.pop();
        out.push_back({static_cast<int64_t>(id), d});
    }
    std::sort(out.begin(), out.end(),
              [](const auto& a, const auto& b){ return a.score < b.score; });
    if (out.size() > k) out.resize(k);
    return out;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// CudaHnswTraversalEngine::Impl
// ─────────────────────────────────────────────────────────────────────────────

struct CudaHnswTraversalEngine::Impl {
    bool cuda_available = false;
    bool index_built    = false;

    // Host-side copies (always kept for CPU fallback)
    std::vector<HnswLayerGraph> layers;
    std::vector<float>          flat_vectors;
    size_t                      num_vectors = 0;

#ifdef THEMIS_ENABLE_CUDA
    // Device pointers
    float*   d_vectors    = nullptr;
    int32_t* d_offsets    = nullptr;  // Bottom layer only (layer 0)
    int32_t* d_neighbours = nullptr;
    int64_t* d_result_ids     = nullptr;
    float*   d_result_scores  = nullptr;
    size_t   result_buf_size  = 0;    // Allocated query capacity

    // Persistent visited-bitset pool.
    // Sized at construction time to maxBatchSize × ceil(numNodes / 8) bytes.
    // Eliminates per-kernel cudaMalloc/cudaFree (avoids ~2-5 µs stream stall).
    uint8_t* d_visited_pool       = nullptr;
    size_t   visited_pool_bytes   = 0;   // Current allocated capacity
    size_t   maxBatchSize_        = 512; // Tunable via setMaxBatchSize()

    cudaStream_t stream = nullptr;

    void freeDevice() {
        if (d_vectors)       { cudaFree(d_vectors);       d_vectors       = nullptr; }
        if (d_offsets)       { cudaFree(d_offsets);       d_offsets       = nullptr; }
        if (d_neighbours)    { cudaFree(d_neighbours);    d_neighbours    = nullptr; }
        if (d_result_ids)    { cudaFree(d_result_ids);    d_result_ids    = nullptr; }
        if (d_result_scores) { cudaFree(d_result_scores); d_result_scores = nullptr; }
        if (d_visited_pool)  { cudaFree(d_visited_pool);  d_visited_pool  = nullptr; visited_pool_bytes = 0; }
        if (stream)          { cudaStreamDestroy(stream); stream          = nullptr; }
    }

    /// Allocate / re-allocate the visited-bitset pool to cover
    /// maxBatchSize_ queries over numNodes nodes.  No-op when the current
    /// pool is already large enough.  Returns false if cudaMalloc fails.
    bool ensureVisitedPool(size_t numNodes) {
        const size_t bytes_per_query = (numNodes + 7u) / 8u;
        const size_t required = maxBatchSize_ * bytes_per_query;
        if (visited_pool_bytes >= required) return true;  // already sufficient
        if (d_visited_pool) { cudaFree(d_visited_pool); d_visited_pool = nullptr; visited_pool_bytes = 0; }
        const cudaError_t err = cudaMalloc(&d_visited_pool, required);
        if (err != cudaSuccess || d_visited_pool == nullptr) return false;
        visited_pool_bytes = required;
        return true;
    }
#endif

    ~Impl() {
#ifdef THEMIS_ENABLE_CUDA
        freeDevice();
#endif
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor / Move
// ─────────────────────────────────────────────────────────────────────────────

CudaHnswTraversalEngine::CudaHnswTraversalEngine(CudaHnswConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config))
{
#ifdef THEMIS_ENABLE_CUDA
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err == cudaSuccess && device_count > 0 &&
        config_.device_id < device_count) {
        cudaSetDevice(config_.device_id);
        cudaStreamCreate(&impl_->stream);
        impl_->cuda_available = true;
        THEMIS_INFO("CudaHnswTraversalEngine: CUDA device {} selected",
                    config_.device_id);
    } else {
        THEMIS_WARN("CudaHnswTraversalEngine: no CUDA device available; "
                    "falling back to CPU traversal");
    }
#else
    THEMIS_INFO("CudaHnswTraversalEngine: built without CUDA; "
                "using CPU traversal");
#endif
}

CudaHnswTraversalEngine::~CudaHnswTraversalEngine() = default;

CudaHnswTraversalEngine::CudaHnswTraversalEngine(CudaHnswTraversalEngine&&) noexcept = default;
CudaHnswTraversalEngine& CudaHnswTraversalEngine::operator=(CudaHnswTraversalEngine&&) noexcept = default;

void CudaHnswTraversalEngine::setMaxBatchSize(size_t n) noexcept {
    if (!impl_) return;
#ifdef THEMIS_ENABLE_CUDA
    if (n > impl_->maxBatchSize_ || impl_->visited_pool_bytes == 0) {
        impl_->maxBatchSize_ = n;
        // Re-allocate if the index is already built and we know the node count.
        if (impl_->index_built && impl_->d_offsets) {
            // Number of nodes = last offset value in the CSR offsets array.
            // We don't store it separately, so derive it from the impl
            // (bottom layer node count is num_vectors for the flat layout).
            const size_t num_nodes = impl_->num_vectors;
            if (!impl_->ensureVisitedPool(num_nodes)) {
                THEMIS_WARN("CudaHnswTraversalEngine::setMaxBatchSize: "
                            "cudaMalloc({} bytes) failed — pool unavailable; "
                            "falling back to per-invocation allocation",
                            n * ((num_nodes + 7u) / 8u));
            }
        }
    }
#else
    (void)n;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// buildIndex
// ─────────────────────────────────────────────────────────────────────────────

bool CudaHnswTraversalEngine::buildIndex(const std::vector<HnswLayerGraph>& layers,
                                          const float*                        vectors,
                                          size_t                              num_vectors) {
    if (layers.empty() || vectors == nullptr || num_vectors == 0) return false;

    // Always keep a host copy for CPU fallback
    impl_->layers.clear();
    impl_->layers = layers;
    impl_->num_vectors = num_vectors;
    impl_->flat_vectors.assign(vectors, vectors + num_vectors * config_.dim);

#ifdef THEMIS_ENABLE_CUDA
    if (!impl_->cuda_available) {
        impl_->index_built = true;
        return true;
    }

    impl_->freeDevice();

    // Upload vectors
    size_t vec_bytes = num_vectors * config_.dim * sizeof(float);
    if (cudaMalloc(&impl_->d_vectors, vec_bytes) != cudaSuccess) {
        THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(vectors) failed");
        impl_->cuda_available = false;
        impl_->index_built = true;
        return true;  // CPU fallback still works
    }
    cudaMemcpy(impl_->d_vectors, vectors, vec_bytes, cudaMemcpyHostToDevice);

    // Upload bottom-layer CSR graph (layer 0 is used for the search kernel)
    const HnswLayerGraph& bottom = layers[0];
    size_t off_bytes = (bottom.num_nodes + 1) * sizeof(int32_t);
    size_t nb_bytes  = bottom.neighbours.size() * sizeof(int32_t);

    if (cudaMalloc(&impl_->d_offsets, off_bytes) != cudaSuccess ||
        cudaMalloc(&impl_->d_neighbours, nb_bytes) != cudaSuccess) {
        THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(graph) failed");
        impl_->freeDevice();
        impl_->cuda_available = false;
        impl_->index_built = true;
        return true;
    }
    cudaMemcpy(impl_->d_offsets,    bottom.offsets.data(),    off_bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(impl_->d_neighbours, bottom.neighbours.data(), nb_bytes,  cudaMemcpyHostToDevice);

    // Pre-allocate the persistent visited-bitset pool to cover the default
    // (or previously configured) maxBatchSize_.  Failure here is non-fatal:
    // launchHnswSearchKernel falls back to per-invocation allocation silently.
    if (!impl_->ensureVisitedPool(num_vectors)) {
        THEMIS_WARN("CudaHnswTraversalEngine::buildIndex: "
                    "visited pool pre-allocation failed — "
                    "per-invocation fallback will be used");
    }
#endif

    impl_->index_built = true;
    THEMIS_INFO("CudaHnswTraversalEngine::buildIndex: {} vectors × dim {} indexed",
                num_vectors, config_.dim);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// addNode
// ─────────────────────────────────────────────────────────────────────────────

bool CudaHnswTraversalEngine::addNode(int64_t new_id,
                                       const float* vector,
                                       const std::vector<HnswLayerGraph>& updated_layers) {
    if (!impl_->index_built) return false;
    // Update host copy
    impl_->layers = updated_layers;
    impl_->flat_vectors.insert(impl_->flat_vectors.end(),
                                vector, vector + config_.dim);
    ++impl_->num_vectors;

    // Re-upload is cheap for single-node incremental adds; a production path
    // would use device-side append; for now rebuild device buffers.
    (void)new_id;
    return buildIndex(impl_->layers,
                      impl_->flat_vectors.data(),
                      impl_->num_vectors);
}

// ─────────────────────────────────────────────────────────────────────────────
// search
// ─────────────────────────────────────────────────────────────────────────────

std::vector<HnswTraversalResult>
CudaHnswTraversalEngine::search(const float* query, uint32_t k, uint32_t ef) const {
    if (!impl_->index_built || impl_->flat_vectors.empty()) return {};
    if (ef == 0) ef = config_.ef_search;
    if (k == 0)  k  = 1;

#ifdef THEMIS_ENABLE_CUDA
    if (impl_->cuda_available && impl_->d_vectors) {
        auto batch = batchSearch(query, 1, k, ef);
        if (!batch.empty()) return batch[0];
    }
#endif

    return cpuHnswSearch(impl_->layers, impl_->flat_vectors, config_.dim,
                          query, k, ef, config_.metric);
}

// ─────────────────────────────────────────────────────────────────────────────
// batchSearch
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::vector<HnswTraversalResult>>
CudaHnswTraversalEngine::batchSearch(const float* queries, size_t num_queries,
                                      uint32_t k, uint32_t ef) const {
    if (!impl_->index_built || queries == nullptr || num_queries == 0) return {};
    if (ef == 0) ef = config_.ef_search;
    if (k  == 0) k  = 1;

    std::vector<std::vector<HnswTraversalResult>> results(num_queries);

#ifdef THEMIS_ENABLE_CUDA
    if (impl_->cuda_available && impl_->d_vectors) {
        const uint32_t num_nodes = impl_->layers[0].num_nodes;

        // ── Single-pass GPU path (k ≤ kHnswKernelMaxK) ───────────────────────
        if (k <= themis::cuda::kHnswKernelMaxK) {
            if (impl_->result_buf_size < num_queries * k) {
                if (impl_->d_result_ids)    cudaFree(impl_->d_result_ids);
                if (impl_->d_result_scores) cudaFree(impl_->d_result_scores);
                cudaMalloc(&impl_->d_result_ids,    num_queries * k * sizeof(int64_t));
                cudaMalloc(&impl_->d_result_scores, num_queries * k * sizeof(float));
                impl_->result_buf_size = num_queries * k;
            }

            // Grow visited pool if this batch is larger than the pre-allocated size
            impl_->ensureVisitedPool(num_nodes);

            float* d_queries = nullptr;
            cudaMalloc(&d_queries, num_queries * config_.dim * sizeof(float));
            cudaMemcpy(d_queries, queries,
                       num_queries * config_.dim * sizeof(float),
                       cudaMemcpyHostToDevice);

            bool overflow = false;
            themis::cuda::launchHnswSearchKernel(
                impl_->d_vectors, config_.dim,
                impl_->d_offsets, impl_->d_neighbours,
                num_nodes,
                d_queries, static_cast<uint32_t>(num_queries),
                k, ef, static_cast<uint8_t>(config_.metric),
                /*entry_node=*/0u,
                impl_->d_result_ids, impl_->d_result_scores,
                impl_->stream,
                &overflow,
                impl_->d_visited_pool,
                impl_->visited_pool_bytes);

            cudaStreamSynchronize(impl_->stream);
            cudaFree(d_queries);

            if (overflow) {
                // Pool allocation failed and per-invocation fallback also failed
                // (degraded mode) — fall through to CPU.
            } else {
                std::vector<int64_t> h_ids(num_queries * k);
                std::vector<float>   h_scores(num_queries * k);
                cudaMemcpy(h_ids.data(), impl_->d_result_ids,
                           h_ids.size() * sizeof(int64_t), cudaMemcpyDeviceToHost);
                cudaMemcpy(h_scores.data(), impl_->d_result_scores,
                           h_scores.size() * sizeof(float), cudaMemcpyDeviceToHost);

                for (size_t qi = 0; qi < num_queries; ++qi) {
                    for (uint32_t ri = 0; ri < k; ++ri) {
                        results[qi].push_back({h_ids[qi * k + ri],
                                               h_scores[qi * k + ri]});
                    }
                }
                return results;
            }
        }

        // ── Multi-pass GPU path (k > kHnswKernelMaxK) ────────────────────────
        // Run ceil(k / kHnswKernelMaxK) passes, each retrieving up to kMaxK
        // results from a different entry node.  Results from all passes are
        // merged on the host and the top-k winners are selected with
        // std::partial_sort.
        bool multi_pass_ok = false;
        {
            const uint32_t pass_k     = themis::cuda::kHnswKernelMaxK;
            const uint32_t num_passes = (k + pass_k - 1u) / pass_k;

            // Per-pass device buffers (reused across passes)
            int64_t* d_pass_ids    = nullptr;
            float*   d_pass_scores = nullptr;
            const cudaError_t e1 = cudaMalloc(&d_pass_ids,
                                              num_queries * pass_k * sizeof(int64_t));
            const cudaError_t e2 = (e1 == cudaSuccess)
                                   ? cudaMalloc(&d_pass_scores,
                                               num_queries * pass_k * sizeof(float))
                                   : cudaErrorMemoryAllocation;

            if (e1 == cudaSuccess && e2 == cudaSuccess) {
                float* d_queries = nullptr;
                cudaMalloc(&d_queries, num_queries * config_.dim * sizeof(float));
                cudaMemcpy(d_queries, queries,
                           num_queries * config_.dim * sizeof(float),
                           cudaMemcpyHostToDevice);

                // Accumulate all candidates across passes (per query)
                using Candidate = std::pair<float, int64_t>;  // (score, id)
                std::vector<std::vector<Candidate>> all_cands(num_queries);

                for (uint32_t pass = 0; pass < num_passes; ++pass) {
                    // Select entry node: pass 0 uses node 0; subsequent passes use
                    // the node at index (pass * num_nodes / num_passes) to spread
                    // exploration across the graph.
                    const uint32_t entry_node = (pass == 0u)
                        ? 0u
                        : static_cast<uint32_t>(
                              (static_cast<uint64_t>(pass) * num_nodes) / num_passes);

                    bool overflow = false;
                    themis::cuda::launchHnswSearchKernel(
                        impl_->d_vectors, config_.dim,
                        impl_->d_offsets, impl_->d_neighbours,
                        num_nodes,
                        d_queries, static_cast<uint32_t>(num_queries),
                        pass_k, ef, static_cast<uint8_t>(config_.metric),
                        entry_node,
                        d_pass_ids, d_pass_scores,
                        impl_->stream,
                        &overflow,
                        impl_->d_visited_pool,
                        impl_->visited_pool_bytes);

                    cudaStreamSynchronize(impl_->stream);
                    if (overflow) continue;  // Should not happen since pass_k ≤ kMaxK

                    std::vector<int64_t> h_ids(num_queries * pass_k);
                    std::vector<float>   h_sc(num_queries * pass_k);
                    cudaMemcpy(h_ids.data(), d_pass_ids,
                               h_ids.size() * sizeof(int64_t), cudaMemcpyDeviceToHost);
                    cudaMemcpy(h_sc.data(),  d_pass_scores,
                               h_sc.size()  * sizeof(float),   cudaMemcpyDeviceToHost);

                    for (size_t qi = 0; qi < num_queries; ++qi) {
                        for (uint32_t ri = 0; ri < pass_k; ++ri) {
                            const int64_t id    = h_ids[qi * pass_k + ri];
                            const float   score = h_sc [qi * pass_k + ri];
                            if (id >= 0) {
                                all_cands[qi].emplace_back(score, id);
                            }
                        }
                    }
                }

                cudaFree(d_queries);

                // Merge: deduplicate by id, then partial_sort to get top-k
                for (size_t qi = 0; qi < num_queries; ++qi) {
                    auto& cands = all_cands[qi];
                    // Sort by id for deduplication
                    std::sort(cands.begin(), cands.end(),
                              [](const Candidate& a, const Candidate& b) {
                                  return a.second < b.second;
                              });
                    cands.erase(std::unique(cands.begin(), cands.end(),
                                            [](const Candidate& a, const Candidate& b) {
                                                return a.second == b.second;
                                            }),
                                cands.end());

                    // Partial-sort by score (ascending) to select top-k
                    const size_t take = std::min(static_cast<size_t>(k), cands.size());
                    if (take > 0 && take < cands.size()) {
                        std::partial_sort(cands.begin(),
                                          cands.begin() + static_cast<ptrdiff_t>(take),
                                          cands.end(),
                                          [](const Candidate& a, const Candidate& b) {
                                              return a.first < b.first;
                                          });
                    } else {
                        std::sort(cands.begin(), cands.end(),
                                  [](const Candidate& a, const Candidate& b) {
                                      return a.first < b.first;
                                  });
                    }
                    cands.resize(take);

                    results[qi].reserve(take);
                    for (const auto& c : cands) {
                        results[qi].push_back({c.second, c.first});
                    }
                }
                multi_pass_ok = true;
            }

            cudaFree(d_pass_ids);
            cudaFree(d_pass_scores);
        }

        if (multi_pass_ok) return results;
        // Fall through to CPU if GPU allocation failed
    }
#endif

    // CPU fallback — supports any k without restriction
    for (size_t qi = 0; qi < num_queries; ++qi) {
        results[qi] = cpuHnswSearch(impl_->layers, impl_->flat_vectors,
                                     config_.dim,
                                     queries + qi * config_.dim,
                                     k, ef, config_.metric);
    }
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

bool CudaHnswTraversalEngine::isBuilt() const noexcept {
    return impl_ && impl_->index_built;
}

bool CudaHnswTraversalEngine::isCudaAvailable() const noexcept {
    return impl_ && impl_->cuda_available;
}

std::string CudaHnswTraversalEngine::deviceInfo() const {
    std::ostringstream oss;
    oss << "CudaHnswTraversalEngine{"
        << "device=" << config_.device_id
        << ", dim=" << config_.dim
        << ", ef=" << config_.ef_search
        << ", cuda=" << (isCudaAvailable() ? "yes" : "no (CPU fallback)")
        << ", built=" << (isBuilt() ? "yes" : "no")
        << ", vectors=" << (impl_ ? impl_->num_vectors : 0)
        << "}";
    return oss.str();
}

} // namespace themis
