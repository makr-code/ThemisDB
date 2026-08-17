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

/**
 * @brief Thread-safe in-memory registry for advisory tensor artifact manifests.
 *
 * Phase A implementation uses a flat in-memory map; the store is intentionally
 * not persistent to underline the advisory-only semantics (freshness decay
 * forces planners to re-derive from the exact graph layer after restart).
 *
 * ### Usage
 * @code
 * auto store = std::make_shared<ManifestStore>(metrics);
 *
 * // Producer: store an advisory summary after ANN candidate generation.
 * ArtifactManifest m = buildSummary(...);
 * store->store(m);
 *
 * // Consumer (query planner): get the freshest advisory entry.
 * auto entry = store->get("users/embedding", 0 // shard_id
 * );
 * if (!entry || !entry->isFresh(max_age_s)) {
 *     // Fall back to exact graph retrieval.
 * }
 * @endcode
 */
class ManifestStore {
public:
    /**
     * @brief Construct with an optional MetricsCollector for Prometheus export.
     *
     * @param metrics  Collector used for `tensor_delta_log_entries_total` and
     *                 `tensor_freshness_age_seconds`.  May be nullptr (no-op).
     */
    explicit ManifestStore(
        observability::MetricsCollector* metrics = nullptr) noexcept;

    ~ManifestStore() = default;

    // Non-copyable; move-only to avoid shared mutable state.
    ManifestStore(const ManifestStore&)            = delete;
    ManifestStore& operator=(const ManifestStore&) = delete;
    ManifestStore(ManifestStore&&)                 = delete;
    ManifestStore& operator=(ManifestStore&&)      = delete;

    // ── Write operations ────────────────────────────────────────────────────

    /**
     * @brief Insert or update an artifact manifest entry.
     *
     * If an entry with the same (tensor_name, shard_id, artifact_id) already
     * exists, it is replaced when @p manifest.version is greater; otherwise
     * the existing entry is kept (monotonic version enforcement).
     *
     * Increments `tensor_delta_log_entries_total` counter on each call.
     *
     * @param manifest  Entry to store.
     * @return          true  if the entry was inserted or updated.
     *                  false if a newer version already exists.
     */
    [[nodiscard]] bool store(const ArtifactManifest& manifest);

    /**
     * @brief Remove all entries whose artifact_id matches @p artifact_id.
     *
     * @param artifact_id  The ID to remove.
     * @return             Number of entries removed.
     */
    std::size_t evict(const std::string& artifact_id);

    /**
     * @brief Remove all entries older than @p max_age_s seconds.
     *
     * Called periodically by the planner to keep the store bounded.
     *
     * @param max_age_s  Maximum allowed age in seconds.
     * @return           Number of entries evicted.
     */
    std::size_t evictStale(double max_age_s);

    // ── Read operations ─────────────────────────────────────────────────────

    /**
     * @brief Retrieve the freshest entry for a given tensor and shard.
     *
     * Returns the entry with the highest version that matches the
     * (tensor_name, shard_id) key.
     *
     * @param tensor_name  Logical tensor name (e.g. "users/embedding").
     * @param shard_id     Shard index (0 for single-shard Phase A).
     * @return             Matching entry, or nullopt if none exists.
     */
    [[nodiscard]] std::optional<ArtifactManifest>
    get(const std::string& tensor_name, uint32_t shard_id) const;

    /**
     * @brief List all entries for a given tensor across all shards.
     *
     * @param tensor_name  Logical tensor name.
     * @return             All entries sorted by (shard_id, version desc).
     */
    [[nodiscard]] std::vector<ArtifactManifest>
    list(const std::string& tensor_name) const;

    /**
     * @brief Total number of live entries in the store.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    // ── Observability ───────────────────────────────────────────────────────

    /**
     * @brief Push freshness gauge for all registered tensor names.
     *
     * Updates `tensor_freshness_age_seconds{tensor_name=...}` for each
     * tensor that has at least one live entry.  Should be called periodically
     * (e.g. from a background tick or before each planner invocation).
     */
    void refreshFreshnessMetrics() const;

private:
    // Composite key: (tensor_name, shard_id, artifact_id)
    struct Key {
        std::string tensor_name;
        uint32_t    shard_id    = 0;
        std::string artifact_id;

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
