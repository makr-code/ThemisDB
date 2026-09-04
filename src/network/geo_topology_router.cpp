/**
 * @file geo_topology_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Network topology-aware routing for geo-distributed clusters.
// See include/network/geo_topology_router.h for design documentation.

#include "network/geo_topology_router.h"
#include "utils/logger.h"

#include <algorithm>
#include <limits>

namespace themis::network {

namespace {
/// Latency (ms) assigned to regions without an explicit hint in
/// LOWEST_LATENCY mode.  High enough to deprioritise unspecified regions
/// while keeping them reachable if every hinted region is unavailable.
constexpr uint32_t kUnhintedRegionLatencyMs = 999999u;
}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

GeoTopologyRouter::GeoTopologyRouter(
    const Config&                            config,
    std::shared_ptr<sharding::ShardTopology> topology)
    : config_(config)
    , topology_(std::move(topology))
{
    THEMIS_INFO("[GeoTopologyRouter] initialized: region='{}' zone='{}' "
                "datacenter='{}' strategy={}",
                config_.local_region, config_.local_zone,
                config_.local_datacenter,
                static_cast<int>(config_.strategy));
}

// ─────────────────────────────────────────────────────────────────────────────
// Locality scoring
// ─────────────────────────────────────────────────────────────────────────────

int GeoTopologyRouter::localityScore(const sharding::ShardInfo& shard) const {
    int score = 0;
    if (!config_.local_region.empty() && shard.region == config_.local_region) {
        score += 4;
    }
    if (!config_.local_zone.empty() && shard.zone == config_.local_zone) {
        score += 2;
    }
    if (!config_.local_datacenter.empty() &&
        shard.datacenter == config_.local_datacenter)
    {
        score += 1;
    }
    return score;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal selectors
// ─────────────────────────────────────────────────────────────────────────────

const sharding::ShardInfo* GeoTopologyRouter::selectPreferLocal(
    const std::vector<sharding::ShardInfo>& healthy) const
{
    if (healthy.empty()) {
      return nullptr;
    }

    const sharding::ShardInfo* best       = nullptr;
    int                        best_score = -1;

    for (const auto& shard : healthy) {
        const int score = localityScore(shard);
        if (score > best_score) {
            best_score = score;
            best       = &shard;
        }
    }
    return best;
}

const sharding::ShardInfo* GeoTopologyRouter::selectLowestLatency(
    const std::vector<sharding::ShardInfo>& healthy) const
{
    if (healthy.empty()) {
      return nullptr;
    }

    // Fall back to PREFER_LOCAL when no latency hints are available.
    if (config_.region_latency_hints.empty()) {
        return selectPreferLocal(healthy);
    }

    const sharding::ShardInfo* best         = nullptr;
    uint32_t                   best_latency = std::numeric_limits<uint32_t>::max();

    for (const auto& shard : healthy) {
        const auto it = config_.region_latency_hints.find(shard.region);
        // Shards in regions without a hint are reachable but deprioritised.
        const uint32_t latency =
            (it != config_.region_latency_hints.end()) ? it->second : kUnhintedRegionLatencyMs;
        if (latency < best_latency) {
            best_latency = latency;
            best         = &shard;
        }
    }
    return best;
}

const sharding::ShardInfo* GeoTopologyRouter::selectRoundRobin(
    const std::vector<sharding::ShardInfo>& healthy) const
{
    if (healthy.empty()) {
      return nullptr;
    }

    const uint64_t idx =
        round_robin_counter_.fetch_add(1, std::memory_order_relaxed);
    return &healthy[static_cast<std::size_t>(idx % healthy.size())];
}

// ─────────────────────────────────────────────────────────────────────────────
// Public routing interface
// ─────────────────────────────────────────────────────────────────────────────

std::string GeoTopologyRouter::selectEndpoint() const {
    if (!topology_) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.routing_failures;
        return {};
    }

    const auto healthy = topology_->getHealthyShards();
    if (healthy.empty()) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.routing_failures;
        THEMIS_WARN("[GeoTopologyRouter] no healthy shards available");
        return {};
    }

    const sharding::ShardInfo* selected = nullptr;

    switch (config_.strategy) {
        case Strategy::PREFER_LOCAL:
            selected = selectPreferLocal(healthy);
            break;
        case Strategy::LOWEST_LATENCY:
            selected = selectLowestLatency(healthy);
            break;
        case Strategy::ROUND_ROBIN:
            selected = selectRoundRobin(healthy);
            break;
    }

    if (!selected) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.routing_failures;
        return {};
    }

    const bool is_local =
        (!config_.local_region.empty() &&
         selected->region == config_.local_region);

    // Honour the cross-region fallback guard: when the flag is false and the
    // best available shard is outside the configured local region, treat the
    // request as unroutable rather than silently forwarding cross-region.
    if (!is_local && !config_.fallback_cross_region &&
        !config_.local_region.empty())
    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.routing_failures;
        THEMIS_WARN("[GeoTopologyRouter] rejecting request: no local shards "
                    "available and cross-region fallback is disabled");
        return {};
    }

    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.requests_routed;
        if (is_local) {
            ++stats_.local_region_hits;
        } else {
            ++stats_.cross_region_fallbacks;
        }
    }

    THEMIS_DEBUG("[GeoTopologyRouter] routed to shard='{}' region='{}' "
                 "endpoint='{}'",
                 selected->shard_id, selected->region,
                 selected->primary_endpoint);

    return selected->primary_endpoint;
}

std::string GeoTopologyRouter::selectEndpointInRegion(
    const std::string& region) const
{
    if (!topology_) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.routing_failures;
        return {};
    }

    const auto shards = topology_->getHealthyShardsInRegion(region);
    if (shards.empty()) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.routing_failures;
        THEMIS_WARN("[GeoTopologyRouter] no healthy shards in region '{}'",
                    region);
        return {};
    }

    // Apply sub-region affinity (zone/datacenter) within the target region.
    // `shards` is non-empty here (guarded above), so `best` will always be set
    // after the loop – no post-loop null check is required.
    const sharding::ShardInfo* best       = &shards[0];
    int                        best_score = localityScore(shards[0]);

    for (std::size_t i = 1; i < shards.size(); ++i) {
        const int score = localityScore(shards[i]);
        if (score > best_score) {
            best_score = score;
            best       = &shards[i];
        }
    }

    const bool is_local = (region == config_.local_region);

    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.requests_routed;
        if (is_local) {
            ++stats_.local_region_hits;
        } else {
            ++stats_.cross_region_fallbacks;
        }
    }

    return best->primary_endpoint;
}

std::vector<sharding::ShardInfo> GeoTopologyRouter::getRankedShards() const {
    if (!topology_) return {};

    auto healthy = topology_->getHealthyShards();
    if (healthy.empty()) return {};

    if (config_.strategy == Strategy::LOWEST_LATENCY &&
        !config_.region_latency_hints.empty())
    {
        std::stable_sort(
            healthy.begin(), healthy.end(),
            [this](const sharding::ShardInfo& a,
                   const sharding::ShardInfo& b) {
                auto la = config_.region_latency_hints.find(a.region);
                auto lb = config_.region_latency_hints.find(b.region);
                const uint32_t lat_a =
                    (la != config_.region_latency_hints.end())
                        ? la->second
                        : kUnhintedRegionLatencyMs;
                const uint32_t lat_b =
                    (lb != config_.region_latency_hints.end())
                        ? lb->second
                        : kUnhintedRegionLatencyMs;
                return lat_a < lat_b;
            });
    } else if (config_.strategy != Strategy::ROUND_ROBIN) {
        std::stable_sort(
            healthy.begin(), healthy.end(),
            [this](const sharding::ShardInfo& a,
                   const sharding::ShardInfo& b) {
                return localityScore(a) > localityScore(b);
            });
    }

    return healthy;
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

GeoTopologyRouter::Stats GeoTopologyRouter::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

}  // namespace themis::network

