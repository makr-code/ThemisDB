/**
 * @file manifest_store.h
 * @brief ManifestStore — advisory-only tensor artifact registry for Phase A.
 *
 * Stores, retrieves, and invalidates ArtifactManifest entries.  The store
 * emits Prometheus metrics that the query planner uses as Phase A freshness
 * gates:
 *
 *   - `tensor_delta_log_entries_total` — counter incremented on each store()
 *   - `tensor_freshness_age_seconds`   — gauge set to the age of the oldest
 *                                        live entry for each tensor_name
 *
 * ## Advisory-Only Contract
 *
 * ManifestStore only tracks advisory summary artifacts.  The store MUST NOT
 * be used as the authoritative query result cache.  Every planner path that
 * consults the store must fall back to Graph Truth Layer retrieval when:
 *   - no entry is found;
 *   - the entry fails isFresh(); or
 *   - the entry integrity check fails.
 *
 * @see ArtifactManifest for the per-entry schema
 * @see ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §9 (Phase A observability)
 */

#pragma once

#include "artifact_manifest.h"
#include "observability/metrics_collector.h"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// ManifestStore
// ---------------------------------------------------------------------------

class ManifestStore {
public:
    explicit ManifestStore(
        observability::MetricsCollector* metrics = nullptr) noexcept;

    ~ManifestStore() = default;

    // Non-copyable; move-only to avoid shared mutable state.
    ManifestStore(const ManifestStore&)            = delete;
    ManifestStore& operator=(const ManifestStore&) = delete;
    ManifestStore(ManifestStore&&)                 = delete;
    ManifestStore& operator=(ManifestStore&&)      = delete;

    // ── Write operations ────────────────────────────────────────────────────

    [[nodiscard]] bool store(const ArtifactManifest& manifest);

    /**
     * @brief Evict.
     * @param[in] artifact_id Identifier of the artifact.
     * @return Return value.
     */
    std::size_t evict(const std::string& artifact_id);

    /**
     * @brief Evict Stale.
     * @param[in] max_age_s Input parameter.
     * @return Return value.
     */
    std::size_t evictStale(double max_age_s);

    // ── Read operations ─────────────────────────────────────────────────────

    [[nodiscard]] std::optional<ArtifactManifest>
    get(const std::string& tensor_name, uint32_t shard_id) const;

    [[nodiscard]] std::vector<ArtifactManifest>
    list(const std::string& tensor_name) const;

    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief ── Observability ───────────────────────────────────────────────────────
     */

    void refreshFreshnessMetrics() const;

private:
    // Composite key: (tensor_name, shard_id, artifact_id)
    struct Key {
        std::string tensor_name = {};
        uint32_t    shard_id    = 0;
        std::string artifact_id = {};

        bool operator==(const Key& o) const noexcept {
            return tensor_name == o.tensor_name
                && shard_id    == o.shard_id
                && artifact_id == o.artifact_id;
        }
    };

    struct KeyHash {
        std::size_t operator()(const Key& k) const noexcept {
            std::size_t h = std::hash<std::string>{}(k.tensor_name);
            h ^= std::hash<uint32_t>{}(k.shard_id)     + 0x9e3779b9u + (h << 6) + (h >> 2);
            h ^= std::hash<std::string>{}(k.artifact_id) + 0x9e3779b9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    mutable std::mutex                                              mutex_;
    std::unordered_map<Key, ArtifactManifest, KeyHash>             entries_;
    observability::MetricsCollector*                                metrics_ = nullptr;
};

} // namespace distributed_tensor
} // namespace themis
