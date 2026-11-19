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

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
class RocksDBWrapper;
}

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// ManifestStore
// ---------------------------------------------------------------------------

class ManifestStore {
public:
    /**
     * @brief Open the durable manifest registry.
     * @param db_path Directory used by the RocksDB-backed store; must be non-empty.
     * @param metrics Optional borrowed metrics collector, which must outlive this store.
     * @throws std::invalid_argument if db_path is empty.
     * @throws std::runtime_error if RocksDB cannot open the database or persisted
     *         manifest records are corrupt.
     * @note Writes use the RocksDB WAL and synchronous commits. The store is
     *       thread-safe within this process and remains advisory-only.
     */
    explicit ManifestStore(std::string db_path,
                           observability::MetricsCollector* metrics = nullptr);

    ~ManifestStore();

    // Non-copyable; move-only to avoid shared mutable state.
    ManifestStore(const ManifestStore&)            = delete;
    ManifestStore& operator=(const ManifestStore&) = delete;
    ManifestStore(ManifestStore&&)                 = delete;
    ManifestStore& operator=(ManifestStore&&)      = delete;

    // ── Write operations ────────────────────────────────────────────────────

    /**
     * @brief Persist a valid manifest if it advances the version for its identity.
     * @param manifest Manifest to store; its identity and schema must validate.
     * @return True only after the durable RocksDB batch has committed.
     */
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
    mutable std::mutex mutex_;
    std::unique_ptr<::themis::RocksDBWrapper> storage_;
    observability::MetricsCollector* metrics_ = nullptr;
};

} // namespace distributed_tensor
} // namespace themis
