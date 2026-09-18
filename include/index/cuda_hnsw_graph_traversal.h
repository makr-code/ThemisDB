/**
 * @file cuda_hnsw_graph_traversal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {

struct HnswTraversalResult {
    int64_t id;     ///< Vector ID
    float   score;  ///< Distance / similarity score (lower = closer for L2)
};

enum class HnswDistanceMetric : uint8_t {
    L2      = 0,  ///< Squared Euclidean distance
    Cosine  = 1,  ///< Cosine distance (1 − cosine similarity)
    Dot     = 2,  ///< Negative dot product (maximise dot product)
};

struct HnswLayerGraph {
    std::vector<int32_t>  offsets;    ///< Row offsets (size = num_nodes + 1)
    std::vector<int32_t>  neighbours; ///< Neighbour IDs (concatenated adjacency lists)
    uint32_t              num_nodes = 0;
    uint32_t              max_neighbours = 0;  ///< Maximum out-degree (M)
};

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

class CudaHnswTraversalEngine {
public:
    explicit CudaHnswTraversalEngine(CudaHnswConfig config = {});
    ~CudaHnswTraversalEngine();

    // Non-copyable, movable
    CudaHnswTraversalEngine(const CudaHnswTraversalEngine&)            = delete;
    CudaHnswTraversalEngine& operator=(const CudaHnswTraversalEngine&) = delete;
    CudaHnswTraversalEngine(CudaHnswTraversalEngine&&)                 noexcept;
    CudaHnswTraversalEngine& operator=(CudaHnswTraversalEngine&&)      noexcept;

    /**
     * @brief ── Index management ──────────────────────────────────────────────────────
     * @param[in] layers Input parameter.
     * @param[in] vectors Input parameter.
     * @param[in] num_vectors Input parameter.
     * @return True when the operation succeeds.
     */

    bool buildIndex(const std::vector<HnswLayerGraph>& layers,
                    const float*                        vectors,
                    size_t                              num_vectors);

    /**
     * @brief Add Node.
     * @param[in] new_id Identifier of the new.
     * @param[in] vector Input parameter.
     * @param[in] updated_layers Input parameter.
     * @return True when the operation succeeds.
     */
    bool addNode(int64_t                             new_id,
                 const float*                        vector,
                 const std::vector<HnswLayerGraph>&  updated_layers);

    // ── Search ────────────────────────────────────────────────────────────────

    std::vector<HnswTraversalResult> search(const float* query,
                                             uint32_t     k,
                                             uint32_t     ef = 0) const;

    std::vector<std::vector<HnswTraversalResult>>
    batchSearch(const float* queries,
                size_t       num_queries,
                uint32_t     k,
                uint32_t     ef = 0) const;

    /**
     * @brief ── Visited bitset pool tuning ────────────────────────────────────────────
     * @param[in] n Input parameter.
     */

    void setMaxBatchSize(size_t n);

    /**
     * @brief Max Batch Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t maxBatchSize() const noexcept;

    /**
     * @brief Has Visited Pool.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool hasVisitedPool() const noexcept;

    /**
     * @brief ── Diagnostics ───────────────────────────────────────────────────────────
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    bool isBuilt() const noexcept;

    /**
     * @brief Is Cuda Available.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isCudaAvailable() const noexcept;

    /**
     * @brief Device Info.
     * @return Return value.
     */
    std::string deviceInfo() const;

    const CudaHnswConfig& config() const noexcept { return config_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    CudaHnswConfig        config_;
};

} // namespace themis
