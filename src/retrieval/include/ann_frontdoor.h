/**
 * @file ann_frontdoor.h
 * @brief ANN Frontdoor — first layer of the hybrid knowledge retrieval stack.
 *
 * Abstracts over HNSW, DiskANN, and future ANN backends. Owns candidate
 * generation, routing policy, and hot/cold-path switching.
 *
 * Planned in: docs/EPIC1_ANN_FRONTDOOR.md
 * Sub-issue:   #5424
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::retrieval {

/// Backend selector for the ANN index.
enum class AnnBackend {
    HNSW,    ///< In-memory hierarchical navigable small world graph
    DiskANN, ///< Disk-resident approximate nearest neighbour index
    Auto,    ///< Runtime selection based on index size and hardware profile
};

/// Routing classification returned by the frontdoor for every query.
enum class AnnRoute {
    Hot,  ///< Index fits in DRAM; low-latency path
    Cold, ///< Index lives on NVMe; moderate-latency path
    Fallback, ///< Neither hot nor cold index available; exact scan or error
};

/// A single retrieval candidate produced by the ANN frontdoor.
struct AnnCandidate {
    std::uint64_t id;          ///< Document / chunk identifier
    float         distance;    ///< L2 or inner-product distance
    float         score;       ///< Normalised relevance score [0, 1]
    std::string   shard_key;   ///< Originating shard (empty for local index)
};

/// Query descriptor forwarded to the ANN layer.
struct AnnQuery {
    std::vector<float> embedding;   ///< Query vector
    std::uint32_t      top_k = 10;  ///< Candidate set size
    float              ef_search = 128.0f; ///< HNSW ef_search parameter
    AnnBackend         backend = AnnBackend::Auto;
    std::optional<std::string> namespace_filter; ///< Optional collection scope
};

/// Result produced by the frontdoor for a single query.
struct AnnResult {
    std::vector<AnnCandidate> candidates;
    AnnRoute                  route;
    double                    latency_ms = 0.0;
    bool                      cache_hit  = false;
};

/// Configuration for the ANN frontdoor.
struct AnnFrontdoorConfig {
    AnnBackend preferred_backend = AnnBackend::Auto;
    std::size_t hot_index_size_limit_bytes = 4ULL * 1024 * 1024 * 1024; // 4 GiB
    float       min_score_threshold = 0.0f;
    bool        enable_cache = true;
    std::string index_directory; ///< Root path for DiskANN segment files
};

/**
 * @brief ANN Frontdoor interface.
 *
 * Implementations select the appropriate ANN backend, perform candidate
 * generation, apply routing policy, and expose observability hooks.
 */
class IAnnFrontdoor {
public:
    virtual ~IAnnFrontdoor() = default;

    /// Search the index and return the top-k candidates.
    virtual AnnResult search(const AnnQuery& query) = 0;

    /// Return the current routing classification for this node.
    virtual AnnRoute getRoute() const = 0;

    /// Switch the active backend at runtime (hot-reload).
    virtual void setBackend(AnnBackend backend) = 0;

    /// Register a callback invoked after every search for telemetry.
    using SearchCallback = std::function<void(const AnnResult&)>;
    virtual void onSearch(SearchCallback cb) = 0;
};

/// Factory: create an AnnFrontdoor from configuration.
std::unique_ptr<IAnnFrontdoor> makeAnnFrontdoor(const AnnFrontdoorConfig& cfg);

} // namespace themis::retrieval
