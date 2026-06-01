/**
 * @file artifact_lifecycle.h
 * @brief Derived artifact lifecycle and staleness management.
 *
 * Governs when derived artifacts (index snapshots, summary caches, compiled
 * adapters) become stale, how they are invalidated, and how rebuild is
 * triggered.
 *
 * Planned in: docs/EPIC2_ARTIFACT_LIFECYCLE.md
 * Sub-issue:   #5442
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::evaluation {

/// Classification of an artifact relative to its source of truth.
enum class ArtifactFreshness {
    Fresh,    ///< Up-to-date
    Stale,    ///< Source has changed; rebuild recommended
    Expired,  ///< TTL exceeded; rebuild mandatory
    Missing,  ///< No derived artifact found
};

/// Staleness trigger that caused a freshness downgrade.
enum class StalenessTrigger {
    SourceUpdated,     ///< Source data changed
    TtlExpired,        ///< Time-to-live elapsed
    SchemaChanged,     ///< Schema or model incompatibility
    ManualInvalidation,///< Explicitly requested rebuild
};

/// Metadata record for one derived artifact.
struct ArtifactRecord {
    std::string  id;
    std::string  source_id;       ///< ID of the source-of-truth artifact
    std::string  artifact_type;   ///< e.g. "hnsw_index", "tensor_summary"
    ArtifactFreshness freshness;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::optional<StalenessTrigger> staleness_trigger;
    std::string  storage_path;
};

/// Policy that controls freshness windows for a given artifact type.
struct ArtifactPolicy {
    std::string  artifact_type;
    std::chrono::seconds ttl{3600};        ///< Max age before Expired
    bool         auto_rebuild = true;      ///< Trigger rebuild on staleness
    std::uint32_t max_rebuild_retries = 3;
};

/**
 * @brief Artifact lifecycle manager.
 */
class IArtifactLifecycle {
public:
    virtual ~IArtifactLifecycle() = default;

    /// Register or update an artifact record.
    virtual void upsert(ArtifactRecord record) = 0;

    /// Look up an artifact record by ID.
    virtual std::optional<ArtifactRecord> lookup(const std::string& id) const = 0;

    /// Evaluate freshness for a given artifact.
    virtual ArtifactFreshness evaluate(const std::string& id) const = 0;

    /// Invalidate an artifact (mark as Stale or Expired).
    virtual void invalidate(const std::string& id,
                             StalenessTrigger trigger) = 0;

    /// Trigger a rebuild for a stale/expired artifact.
    virtual bool triggerRebuild(const std::string& id) = 0;

    /// Register a policy for a given artifact type.
    virtual void registerPolicy(ArtifactPolicy policy) = 0;

    /// Register a callback invoked when a rebuild is triggered.
    using RebuildCallback = std::function<void(const ArtifactRecord&)>;
    virtual void onRebuild(RebuildCallback cb) = 0;
};

/// Factory: create an in-memory artifact lifecycle manager.
std::unique_ptr<IArtifactLifecycle> makeArtifactLifecycle();

} // namespace themis::evaluation
