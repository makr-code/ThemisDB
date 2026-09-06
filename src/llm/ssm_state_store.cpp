/**
 * @file ssm_state_store.cpp
 * @brief In-memory SSM state store implementation (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Status: Phase 1 PoC only
 */

#include "llm/ssm_state_store.h"

#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#if 0
namespace themis::llm {

InMemorySSMStateStore::InMemorySSMStateStore(size_t max_snapshots_per_session)
    : max_snapshots_per_session_(max_snapshots_per_session) {}

bool InMemorySSMStateStore::checkpoint(const std::string& session_id,
                                        const SSMStateSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mu_);

    // Check for duplicate checkpoint at same timestamp
    auto it = state_by_session_.find(session_id);
    if (it != state_by_session_.end()) {
        for (const auto& existing : it->second) {
            if (existing.snapshot_ts == snapshot.snapshot_ts) {
                // Duplicate timestamp: return false
                return false;
            }
        }
    }

    // Add snapshot and enforce size limit
    auto& snapshots = state_by_session_[session_id];
    snapshots.push_back(snapshot);

    // Keep only most recent N snapshots
    if (static_cast<int>(snapshots.size()) > max_snapshots_per_session_) {
        // Sort by HLC timestamp (ascending) and remove oldest
        std::sort(snapshots.begin(), snapshots.end(),
                  [](const SSMStateSnapshot& a, const SSMStateSnapshot& b) {
                      return a.snapshot_ts < b.snapshot_ts;
                  });
        snapshots.erase(snapshots.begin());
    }

    return true;
}

std::optional<SSMStateSnapshot> InMemorySSMStateStore::resume(
    const std::string& session_id,
    const std::optional<core::HLCTimestamp>& snapshot_ts) {
    std::lock_guard<std::mutex> lock(mu_);

    auto it = state_by_session_.find(session_id);
    if (it == state_by_session_.end() || it->second.empty()) {
        return std::nullopt;
    }

    if (!snapshot_ts.has_value()) {
        // Return most recent (highest HLC timestamp)
        auto max_it =
            std::max_element(it->second.begin(), it->second.end(),
                             [](const SSMStateSnapshot& a, const SSMStateSnapshot& b) {
                                 return a.snapshot_ts < b.snapshot_ts;
                             });
        return *max_it;
    }

    // Find snapshot matching requested timestamp
    for (const auto& snap : it->second) {
        if (snap.snapshot_ts == snapshot_ts.value()) {
            return snap;
        }
    }

    return std::nullopt;
}

bool InMemorySSMStateStore::invalidate(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mu_);

    auto it = state_by_session_.find(session_id);
    if (it == state_by_session_.end()) {
        return false;
    }

    state_by_session_.erase(it);
    return true;
}

uint64_t InMemorySSMStateStore::compact(uint64_t retention_window_ms) {
    std::lock_guard<std::mutex> lock(mu_);

    uint64_t deleted_count = 0;

    // In Phase 1, in-memory store doesn't implement time-based retention
    // (no real timestamps). Phase 2 RocksDB will implement proper TTL.
    // For now, return 0 (no cleanup needed for PoC).

    return deleted_count;
}

std::string InMemorySSMStateStore::getStats() const {
    std::lock_guard<std::mutex> lock(mu_);

    json stats;
    stats["store_type"] = "in_memory";
    stats["session_count"] = state_by_session_.size();

    uint64_t total_snapshots = 0;
    uint64_t total_size_bytes = 0;

    for (const auto& [session_id, snapshots] : state_by_session_) {
        total_snapshots += snapshots.size();
        for (const auto& snap : snapshots) {
            total_size_bytes += snap.state_data.size();
        }
    }

    stats["total_snapshots"] = total_snapshots;
    stats["total_size_mb"] = total_size_bytes / (1024.0 * 1024.0);
    stats["max_snapshots_per_session"] = max_snapshots_per_session_;

    return stats.dump();
}

}  // namespace themis::llm
#endif

