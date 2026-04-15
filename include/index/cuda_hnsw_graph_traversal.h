/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_hnsw_graph_traversal.h                        ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:07:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     235                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • da22cf1ef2  2026-04-13  feat(acceleration): CUDA HNSW visited array memory scalin... ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file cuda_hnsw_graph_traversal.h
 * @brief CUDA-accelerated HNSW graph traversal wiring.
 *
 * Exposes a host-side C++ API that dispatches HNSW layer traversal to
 * a CUDA device kernel.  When CUDA is unavailable (THEMIS_NO_CUDA defined
 * or no compatible GPU is detected at runtime) the implementation falls
 * back automatically to the CPU HnswLayerOptimizer path.
 *
 * ## Design
 * - CudaHnswTraversalEngine wraps the CUDA graph-traversal kernel and
 *   manages device memory (adjacency list, vector store, query buffer).
 * - Neighbour lists are stored in CSR (Compressed Sparse Row) format on
 *   the device to maximise coalesced memory access.
 * - Distances are computed with fused CUDA kernels (L2 / Cosine / Dot).
 * - Results are returned as sorted (id, score) pairs on the host.
 *
 * ## Performance Targets
 * - Batch ANN query throughput: ≥200 000 queries/s on RTX-class GPU.
 * - Host-device latency overhead: ≤0.5 ms per batch of 512 queries.
 * - Memory: ≤1.6× raw vector data for adjacency lists (M=16, ef=200).
 *
 * @note Thread Safety: A single CudaHnswTraversalEngine instance must not
 *   be used from multiple host threads simultaneously.  Create one engine
 *   per thread (or protect with a mutex) when concurrent searches are needed.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief A single nearest-neighbour result from a graph traversal.
 */
struct HnswTraversalResult {
    int64_t id;     ///< Vector ID
    float   score;  ///< Distance / similarity score (lower = closer for L2)
};

/**
 * @brief Distance metric used for HNSW traversal on the GPU.
 */
enum class HnswDistanceMetric : uint8_t {
    L2      = 0,  ///< Squared Euclidean distance
    Cosine  = 1,  ///< Cosine distance (1 − cosine similarity)
    Dot     = 2,  ///< Negative dot product (maximise dot product)
};

/**
 * @brief CSR adjacency list for a single HNSW layer, resident on the GPU.
 *
 * Offsets and neighbours are pinned host arrays mirrored to device memory
 * by CudaHnswTraversalEngine::buildIndex().
 */
struct HnswLayerGraph {
    std::vector<int32_t>  offsets;    ///< Row offsets (size = num_nodes + 1)
    std::vector<int32_t>  neighbours; ///< Neighbour IDs (concatenated adjacency lists)
    uint32_t              num_nodes = 0;
    uint32_t              max_neighbours = 0;  ///< Maximum out-degree (M)
};

/**
 * @brief Configuration for the CUDA HNSW traversal engine.
 */
struct CudaHnswConfig {
    int      device_id          = 0;       ///< CUDA device index
    uint32_t dim                = 128;     ///< Vector dimensionality
    uint32_t ef_search          = 64;      ///< Search-time candidate list size
    uint32_t max_layers         = 16;      ///< Maximum number of HNSW layers
    HnswDistanceMetric metric   = HnswDistanceMetric::L2;
    size_t   device_memory_cap  = 0;       ///< Hard cap in bytes (0 = no limit)
    bool     use_fp16           = false;   ///< Use half-precision for distance kernels
    bool     enable_graph_cache = true;    ///< Cache CUDA graph capture for ef_search
};

/**
 * @brief CUDA-accelerated HNSW graph traversal engine.
 *
 * Manages device memory for the multi-layer graph, the vector store, and
 * the query pipeline.  On hosts without CUDA support the public API falls
 * back to CPU execution transparently.
 */
class CudaHnswTraversalEngine {
public:
    /**
     * @brief Construct the engine.
     *
     * Does NOT allocate device memory; call buildIndex() to transfer data.
     * Throws std::runtime_error if @p config.device_id is invalid.
     */
    explicit CudaHnswTraversalEngine(CudaHnswConfig config = {});
    ~CudaHnswTraversalEngine();

    // Non-copyable, movable
    CudaHnswTraversalEngine(const CudaHnswTraversalEngine&)            = delete;
    CudaHnswTraversalEngine& operator=(const CudaHnswTraversalEngine&) = delete;
    CudaHnswTraversalEngine(CudaHnswTraversalEngine&&)                 noexcept;
    CudaHnswTraversalEngine& operator=(CudaHnswTraversalEngine&&)      noexcept;

    // ── Index management ──────────────────────────────────────────────────────

    /**
     * @brief Transfer the multi-layer HNSW graph and vector store to device memory.
     *
     * @param layers     CSR adjacency lists for each HNSW layer (index 0 = bottom).
     * @param vectors    Flat row-major float array [num_vectors × dim].
     * @param num_vectors Number of vectors.
     * @return true on success; false on CUDA error (engine stays usable in CPU mode).
     */
    bool buildIndex(const std::vector<HnswLayerGraph>& layers,
                    const float*                        vectors,
                    size_t                              num_vectors);

    /**
     * @brief Incrementally add a single node to the on-device graph.
     *
     * The host-side layer graphs must already have been updated before this
     * call.  Only the new node's adjacency rows are transferred.
     */
    bool addNode(int64_t                             new_id,
                 const float*                        vector,
                 const std::vector<HnswLayerGraph>&  updated_layers);

    // ── Search ────────────────────────────────────────────────────────────────

    /**
     * @brief Run a single ANN query on the device.
     *
     * @param query  Pointer to @p dim floats (host memory).
     * @param k      Number of nearest neighbours to return.
     * @param ef     Search-time ef override (0 = use config_.ef_search).
     * @return       Sorted list of up to @p k (id, score) pairs.
     */
    std::vector<HnswTraversalResult> search(const float* query,
                                             uint32_t     k,
                                             uint32_t     ef = 0) const;

    /**
     * @brief Batch ANN query — processes @p num_queries in parallel on the GPU.
     *
     * @param queries    Row-major float array [num_queries × dim] (host memory).
     * @param num_queries Number of query vectors.
     * @param k          Neighbours per query.
     * @param ef         Search-time ef override (0 = use config_.ef_search).
     * @return           Outer vector indexed by query; inner vector is sorted results.
     */
    std::vector<std::vector<HnswTraversalResult>>
    batchSearch(const float* queries,
                size_t       num_queries,
                uint32_t     k,
                uint32_t     ef = 0) const;

    // ── Visited bitset pool tuning ────────────────────────────────────────────

    /**
     * @brief Set the maximum query-batch size used to size the persistent
     *        visited bitset pool.
     *
     * The pool is allocated once in buildIndex() as
     *   `n × ceil(numNodes / 8)` bytes.
     * Calling setMaxBatchSize() after buildIndex() takes effect on the next
     * buildIndex() call (the pool is not reallocated lazily).
     *
     * Default: 512 queries.
     *
     * @param n  Maximum number of queries per single kernel launch.
     *           Must be ≥ 1; values of 0 are silently clamped to 1.
     */
    void setMaxBatchSize(size_t n);

    /**
     * @brief Return the current max-batch-size setting.
     */
    size_t maxBatchSize() const noexcept;

    /**
     * @brief Return true when the persistent visited bitset pool has been
     *        successfully allocated (i.e. buildIndex() allocated it without
     *        error).  When false, batchSearch() falls back to per-invocation
     *        allocation.
     */
    bool hasVisitedPool() const noexcept;

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /** @brief True when device memory has been allocated and the index is built. */
    bool isBuilt() const noexcept;

    /** @brief True when a CUDA-capable device is available and selected. */
    bool isCudaAvailable() const noexcept;

    /** @brief Return a human-readable summary of device utilisation. */
    std::string deviceInfo() const;

    const CudaHnswConfig& config() const noexcept { return config_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    CudaHnswConfig        config_;
};

} // namespace themis
