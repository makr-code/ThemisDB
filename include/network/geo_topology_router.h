/**
 * @file geo_topology_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Network topology-aware routing for geo-distributed clusters.
//
// Routes wire protocol requests to the optimal shard endpoint based on the
// cluster's network topology (region, availability zone, datacenter, rack),
// shard health status, and a configurable routing strategy.
//
// Key properties:
//   - Integrates with sharding::ShardTopology to retrieve live shard locations
//   - Three routing strategies: PREFER_LOCAL, LOWEST_LATENCY, ROUND_ROBIN
//   - Zone/datacenter affinity within a region (sub-region locality scoring)
//   - Optional cross-region fallback when all local shards are unavailable
//   - Per-region latency hints for the LOWEST_LATENCY strategy
//   - Prometheus-compatible Stats struct for metrics integration
//   - Thread-safe; round-robin counter uses std::atomic
//
// Design constraints (from ROADMAP.md Phase 3):
//   This class is a routing component only.  It does not manage connections,
//   perform health checks, or own any sockets.  Connection establishment is
//   handled by WireProtocolConnectionPool; health data is maintained by
//   ShardTopology.

#pragma once

#include "sharding/shard_topology.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace network {

// ─────────────────────────────────────────────────────────────────────────────
// GeoTopologyRouter
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Topology-aware request router for geo-distributed ThemisDB clusters.
 *
 * Selects the most suitable shard endpoint for each request by scoring
 * candidates according to their network proximity to this node (region →
 * zone → datacenter → rack) and applying the configured routing strategy.
 *
 * Usage:
 * @code
 *   GeoTopologyRouter::Config cfg;
 *   cfg.local_region     = "us-east";
 *   cfg.local_zone       = "us-east-1a";
 *   cfg.local_datacenter = "dc1";
 *   cfg.strategy         = GeoTopologyRouter::Strategy::PREFER_LOCAL;
 *
 *   GeoTopologyRouter router(cfg, shard_topology);
 *
 *   std::string endpoint = router.selectEndpoint();
 *   if (!endpoint.empty()) {
 *       // acquire connection from WireProtocolConnectionPool
 *       pool.acquireConnection(endpoint);
 *   }
 * @endcode
 *
 * Thread safety: all public methods are safe to call concurrently.
 */
class GeoTopologyRouter {
public:
    // ── Routing strategy ─────────────────────────────────────────────────────

    /**
     * @brief Controls how the router selects among healthy shard candidates.
     */
    enum class Strategy {
        /// Prefer shards co-located with this node (same region > zone >
        /// datacenter).  Falls back to the nearest remote region when no
        /// local shard is available and @c fallback_cross_region is true.
        PREFER_LOCAL,

        /// Route to the region with the lowest configured latency hint
        /// (see @c region_latency_hints).  Falls back to PREFER_LOCAL when
        /// no hints are configured.
        LOWEST_LATENCY,

        /// Distribute requests evenly across all healthy shards in a
        /// round-robin fashion, ignoring topology locality.
        ROUND_ROBIN,
    };

    // ── Configuration ─────────────────────────────────────────────────────────

    struct Config {
        /// Geo-region of this node (e.g., "us-east", "eu-west").
        /// Used to score local vs remote shards.  May be empty if the
        /// node has no region assignment (treats all regions as equal).
        std::string local_region;

        /// Availability zone of this node (e.g., "us-east-1a").  Optional.
        std::string local_zone;

        /// Datacenter identifier of this node (e.g., "dc1").  Optional.
        std::string local_datacenter;

        /// Routing strategy applied when selecting endpoints.
        Strategy strategy = Strategy::PREFER_LOCAL;

        /// When true, requests that cannot be served by the local region
        /// are forwarded to healthy shards in other regions.  When false,
        /// @c selectEndpoint() returns an empty string for such requests.
        bool fallback_cross_region = true;

        /// One-way latency hints per region in milliseconds.
        /// Used exclusively by the LOWEST_LATENCY strategy.
        /// Key: region name  Value: estimated one-way latency in ms.
        std::unordered_map<std::string, uint32_t> region_latency_hints;

        Config() = default;
    };

    // ── Statistics ────────────────────────────────────────────────────────────

    struct Stats {
        uint64_t requests_routed        = 0;  ///< Total successful routing decisions
        uint64_t local_region_hits      = 0;  ///< Requests served by local region
        uint64_t cross_region_fallbacks = 0;  ///< Requests forwarded to remote region
        uint64_t routing_failures       = 0;  ///< Requests with no suitable shard found
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @param config    Routing configuration (topology position + strategy).
     * @param topology  Live cluster topology provider.  Must remain valid for
     *                  the lifetime of this router.
     */
    GeoTopologyRouter(const Config&                            config,
                      std::shared_ptr<sharding::ShardTopology> topology);

    ~GeoTopologyRouter() = default;

    // Non-copyable, non-movable
    GeoTopologyRouter(const GeoTopologyRouter&)            = delete;
    GeoTopologyRouter& operator=(const GeoTopologyRouter&) = delete;
    GeoTopologyRouter(GeoTopologyRouter&&)                 = delete;
    GeoTopologyRouter& operator=(GeoTopologyRouter&&)      = delete;

    // ── Routing ───────────────────────────────────────────────────────────────

    /**
     * @brief Select the primary endpoint of the best available shard.
     *
     * Applies the configured strategy to the current set of healthy shards
     * and returns the @c primary_endpoint of the selected shard.
     *
     * @return Endpoint address string (e.g., "host:port"), or empty string
     *         when no suitable healthy shard is available.
     */
    std::string selectEndpoint() const;

    /**
     * @brief Select the primary endpoint of the best shard in a named region.
     *
     * Within the target region, sub-region affinity (zone/datacenter) is
     * applied using the same locality scoring as PREFER_LOCAL.
     *
     * @param region  Target region name (e.g., "us-east").
     * @return Endpoint address string, or empty string when the region
     *         has no healthy shards.
     */
    std::string selectEndpointInRegion(const std::string& region) const;

    /**
     * @brief Return healthy shards ranked by routing preference.
     *
     * The returned vector is ordered from most-preferred (index 0) to
     * least-preferred under the configured strategy.
     *
     * @return Ordered list of healthy @c ShardInfo objects.
     */
    std::vector<sharding::ShardInfo> getRankedShards() const;

    // ── Inspection ────────────────────────────────────────────────────────────

    /// Return a snapshot of accumulated routing statistics.
    Stats getStats() const;

    /// Return the routing configuration used at construction time.
    const Config& getConfig() const { return config_; }

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Compute a non-negative locality score for @p shard.
    /// Higher values mean the shard is topologically closer to this node.
    ///   +4 : same region
    ///   +2 : same zone
    ///   +1 : same datacenter
    int localityScore(const sharding::ShardInfo& shard) const;

    /// Select the best shard by locality score (PREFER_LOCAL strategy).
    /// Returns nullptr when @p healthy is empty.
    const sharding::ShardInfo* selectPreferLocal(
        const std::vector<sharding::ShardInfo>& healthy) const;

    /// Select the shard whose region has the lowest configured latency hint
    /// (LOWEST_LATENCY strategy).  Falls back to selectPreferLocal() when
    /// @c region_latency_hints is empty.
    const sharding::ShardInfo* selectLowestLatency(
        const std::vector<sharding::ShardInfo>& healthy) const;

    /// Select the next shard in round-robin order (ROUND_ROBIN strategy).
    /// Returns nullptr when @p healthy is empty.
    const sharding::ShardInfo* selectRoundRobin(
        const std::vector<sharding::ShardInfo>& healthy) const;

    // ── Members ───────────────────────────────────────────────────────────────

    Config                                   config_;
    std::shared_ptr<sharding::ShardTopology> topology_;

    mutable std::atomic<uint64_t> round_robin_counter_{0};

    mutable std::mutex stats_mutex_;
    mutable Stats      stats_;
};

}  // namespace network
}  // namespace themis
