/**
 * @file manifest_store.cpp
 * @brief ManifestStore implementation — Phase A advisory tensor manifest registry.
 *
 * This translation unit implements the ManifestStore declared in
 * `include/manifest_store.h`.  The store uses an in-memory flat map guarded
 * by a single mutex.  Advisory-only invariants are enforced by the public API:
 *   - No entry can be read without the caller also receiving the ArtifactManifest
 *     so it can call isFresh() and check integrity.
 *   - Prometheus counters/gauges are updated on every write path.
 *
 * @see manifest_store.h for the full API contract and advisory-only policy.
 */

#include "manifest_store.h"

#include <algorithm>
#include <chrono>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ManifestStore::ManifestStore(observability::MetricsCollector* metrics) noexcept
    : metrics_(metrics) {}

// ---------------------------------------------------------------------------
// store
// ---------------------------------------------------------------------------

bool ManifestStore::store(const ArtifactManifest& manifest) {
    // SG-DT-01: Fail-closed validation.
    // Reject any manifest that fails invariant checks.
    if (!manifest.validate()) {
        return false;
    }

    Key key{manifest.tensor_name, manifest.shard_id, manifest.artifact_id};

    std::unique_lock<std::mutex> lock(mutex_);

    auto it = entries_.find(key);
    if (it != entries_.end()) {
        // Monotonic version enforcement: only replace if newer.
        if (manifest.version <= it->second.version) {
            return false;
        }
        it->second = manifest;
    } else {
        entries_.emplace(std::move(key), manifest);
    }

    lock.unlock();

    // Prometheus: increment delta log counter (Phase A gate metric).
    if (metrics_) {
        metrics_->addCounter("tensor_delta_log_entries_total", 1,
            {{"tensor_name", manifest.tensor_name}});
    }

    return true;
}

// ---------------------------------------------------------------------------
// evict
// ---------------------------------------------------------------------------

std::size_t ManifestStore::evict(const std::string& artifact_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t removed = 0;
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        if (it->first.artifact_id == artifact_id) {
            it = entries_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

// ---------------------------------------------------------------------------
// evictStale
// ---------------------------------------------------------------------------

std::size_t ManifestStore::evictStale(double max_age_s) {
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t removed = 0;
    for (auto it = entries_.begin(); it != entries_.end(); ) {
        if (!it->second.isFresh(max_age_s, now)) {
            it = entries_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

// ---------------------------------------------------------------------------
// get
// ---------------------------------------------------------------------------

std::optional<ArtifactManifest>
ManifestStore::get(const std::string& tensor_name, uint32_t shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const ArtifactManifest* best = nullptr;
    for (const auto& [k, v] : entries_) {
        if (k.tensor_name == tensor_name && k.shard_id == shard_id) {
            if (!best || v.version > best->version) {
                best = &v;
            }
        }
    }

    if (!best) return std::nullopt;
    return *best;
}

// ---------------------------------------------------------------------------
// list
// ---------------------------------------------------------------------------

std::vector<ArtifactManifest>
ManifestStore::list(const std::string& tensor_name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ArtifactManifest> result;
    for (const auto& [k, v] : entries_) {
        if (k.tensor_name == tensor_name) {
            result.push_back(v);
        }
    }

    // Sort by shard_id asc, then version desc.
    std::sort(result.begin(), result.end(),
        [](const ArtifactManifest& a, const ArtifactManifest& b) {
            if (a.shard_id != b.shard_id) return a.shard_id < b.shard_id;
            return a.version > b.version;
        });

    return result;
}

// ---------------------------------------------------------------------------
// size
// ---------------------------------------------------------------------------

std::size_t ManifestStore::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

// ---------------------------------------------------------------------------
// refreshFreshnessMetrics
// ---------------------------------------------------------------------------

void ManifestStore::refreshFreshnessMetrics() const {
    if (!metrics_) return;

    const auto now = std::chrono::system_clock::now();

    // Collect the oldest (largest) age per tensor_name across all shards.
    std::unordered_map<std::string, double> oldest_age;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [k, v] : entries_) {
            const double age = v.freshnessAgeSeconds(now);
            auto [it, inserted] = oldest_age.emplace(k.tensor_name, age);
            if (!inserted && age > it->second) {
                it->second = age;
            }
        }
    }

    for (const auto& [name, age] : oldest_age) {
        metrics_->setGauge("tensor_freshness_age_seconds", age,
            {{"tensor_name", name}});
    }
}

} // namespace distributed_tensor
} // namespace themis
