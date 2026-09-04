/**
 * @file shard_load_balancer.cpp
 * @brief Phase 3 P3-04-B: Load-aware shard selection — implementation.
 * @version 1.0.0
 * @note Status: Block B P3-04-B delivery
 */

#include "sharding/shard_load_balancer.h"

#include <algorithm>
#include <stdexcept>

namespace themis::sharding {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ShardLoadBalancer::ShardLoadBalancer(std::vector<std::string> shard_ids)
    : ShardLoadBalancer(std::move(shard_ids), Config{}) {}

ShardLoadBalancer::ShardLoadBalancer(std::vector<std::string> shard_ids,
                                     Config cfg)
    : cfg_(std::move(cfg)) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& id : shard_ids) {
        if (shards_.find(id) == shards_.end()) {
            shard_order_.push_back(id);
            shards_.emplace(id, ShardState{});
            shards_[id].metrics.available = true;
        }
    }
}

// ---------------------------------------------------------------------------
// addShard / removeShard
// ---------------------------------------------------------------------------

void ShardLoadBalancer::addShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (shards_.find(shard_id) == shards_.end()) {
        shard_order_.push_back(shard_id);
        shards_.emplace(shard_id, ShardState{});
        shards_[shard_id].metrics.available = true;
    }
}

bool ShardLoadBalancer::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = shards_.find(shard_id);
    if (it == shards_.end()) {
        return false;
    }
    it->second.metrics.available = false;
    shard_order_.erase(
        std::remove(shard_order_.begin(), shard_order_.end(), shard_id),
        shard_order_.end());
    shards_.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// updateMetrics / setAvailable
// ---------------------------------------------------------------------------

void ShardLoadBalancer::updateMetrics(const std::string& shard_id,
                                      const ShardMetrics& metrics) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = shards_.find(shard_id);
    if (it == shards_.end()) {
        return;
    }
    it->second.metrics = metrics;
    it->second.metrics.last_updated = std::chrono::steady_clock::now();
}

void ShardLoadBalancer::setAvailable(const std::string& shard_id, bool available) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = shards_.find(shard_id);
    if (it != shards_.end()) {
        it->second.metrics.available = available;
    }
}

// ---------------------------------------------------------------------------
// computeScore
// ---------------------------------------------------------------------------

double ShardLoadBalancer::computeScore(const ShardMetrics& m) const noexcept {
    const double cpu_score =
        std::min(std::max(m.cpu_percent, 0.0), 100.0);

    const double queue_score = cfg_.max_pending > 0.0
        ? std::min(static_cast<double>(m.pending_queries) / cfg_.max_pending * 100.0, 100.0)
        : 0.0;

    const double lat_score = cfg_.max_p99_ms > 0.0
        ? std::min(std::max(m.p99_latency_ms, 0.0) / cfg_.max_p99_ms * 100.0, 100.0)
        : 0.0;

    return cfg_.weights.cpu     * cpu_score
         + cfg_.weights.queue   * queue_score
         + cfg_.weights.latency * lat_score;
}

// ---------------------------------------------------------------------------
// selectShard
// ---------------------------------------------------------------------------

std::string ShardLoadBalancer::selectShard(
    std::optional<std::size_t> client_hash) const {
    std::lock_guard<std::mutex> lk(mutex_);

    if (shards_.empty()) {
        throw std::runtime_error("ShardLoadBalancer: no shards registered");
    }

    // Sticky-session: try the preferred shard first if load is acceptable.
    if (cfg_.sticky_sessions && client_hash.has_value()) {
        const std::size_t idx = *client_hash % shard_order_.size();
        const std::string& sticky_id = shard_order_[idx];
        auto sit = shards_.find(sticky_id);
        if (sit != shards_.end() && sit->second.metrics.available) {
            const double score = computeScore(sit->second.metrics);
            if (score <= cfg_.sticky_threshold) {
                ++const_cast<ShardState&>(sit->second).total_selected;
                return sticky_id;
            }
        }
    }

    // Select shard with minimum score among available shards.
    double      best_score = std::numeric_limits<double>::max();
    std::string best_id = {};

    for (const auto& id : shard_order_) {
        auto it = shards_.find(id);
        if (it == shards_.end() || !it->second.metrics.available) {
            continue;
        }
        const double score = computeScore(it->second.metrics);
        // Tie-breaking: lower score wins; ties broken by earlier insertion order.
        if (score < best_score) {
            best_score = score;
            best_id    = id;
        }
    }

    if (best_id.empty()) {
        throw std::runtime_error("ShardLoadBalancer: no available shards");
    }

    ++const_cast<ShardState&>(shards_.at(best_id)).total_selected;
    return best_id;
}

// ---------------------------------------------------------------------------
// reportCompletion
// ---------------------------------------------------------------------------

void ShardLoadBalancer::reportCompletion(const std::string& shard_id,
                                         double latency_ms) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = shards_.find(shard_id);
    if (it == shards_.end()) {
        return;
    }
    // Exponential moving average for p99 latency (alpha=0.1).
    auto& m = it->second.metrics;
    m.p99_latency_ms = 0.9 * m.p99_latency_ms + 0.1 * latency_ms;
    if (m.pending_queries > 0) {
        --m.pending_queries;
    }
}

// ---------------------------------------------------------------------------
// statistics
// ---------------------------------------------------------------------------

std::vector<ShardLoadBalancer::ShardStatistics>
ShardLoadBalancer::statistics() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<ShardStatistics> out = {};

    out.reserve(shard_order_.size());
    for (const auto& id : shard_order_) {
        auto it = shards_.find(id);
        if (it == shards_.end()) {
          continue;
        }
        ShardStatistics ss;
        ss.shard_id      = id;
        ss.metrics       = it->second.metrics;
        ss.load_score    = computeScore(it->second.metrics);
        ss.total_selected= it->second.total_selected;
        out.push_back(std::move(ss));
    }
    return out;
}

std::size_t ShardLoadBalancer::availableShardCount() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    std::size_t cnt = 0;
    for (const auto& [id, st] : shards_) {
        if (st.metrics.available) {
          ++cnt;
        }
    }
    return cnt;
}

}  // namespace themis::sharding
