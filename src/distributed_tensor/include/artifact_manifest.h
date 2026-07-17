/**
 * @file artifact_manifest.h
 * @brief Tensor artifact manifest schema for ThemisDB distributed tensor layer.
 *
 * ## Advisory-Only Artifact Policy (Phase A invariant)
 *
 * **Tensor artifacts described by this manifest are advisory only.**
 * They capture summaries, candidates, and freshness metadata produced by the
 * ANN and tensor mid-layers.  They MUST NOT be used as the authoritative
 * source of truth for any query result.  The Graph Truth Layer (CPU-first,
 * exact traversal) is the sole authoritative source.
 *
 * @invariant ArtifactKind::ADVISORY_SUMMARY artifacts are inputs to the
 *            query planner, never outputs returned to callers.
 * @invariant No manifest entry overrides a graph-verified result.
 * @invariant Stale manifests (freshness_age_s > threshold) must cause the
 *            planner to fall back to exact graph retrieval.
 *
 * @see ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 Phase A
 * @see ManifestStore for the storage and freshness API
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// ArtifactKind
// ---------------------------------------------------------------------------

/**
 * @brief Classification of a tensor artifact.
 *
 * Only ADVISORY_SUMMARY is allowed in Phase A.  DELTA_LOG and SHARD_SUMMARY
 * entries are reserved for Phase B and C respectively.
 */
enum class ArtifactKind : uint8_t {
    /// Summarized representation of a tensor; advisory, never authoritative.
    ADVISORY_SUMMARY = 0,
    /// Incremental delta record; Phase B entry gate.
    DELTA_LOG        = 1,
    /// Cross-shard summary for routing; Phase C entry gate.
    SHARD_SUMMARY    = 2,
};

/**
 * @brief Lifecycle state for an advisory tensor artifact.
 *
 * ACTIVE and STALE remain readable by the planner subject to freshness and
 * residual gates. INVALIDATED artifacts must force exact-graph fallback.
 */
enum class LifecycleState : uint8_t {
    ACTIVE      = 0,
    STALE       = 1,
    INVALIDATED = 2,
};

/**
 * @brief Last maintenance action applied to an artifact.
 */
enum class RebuildState : uint8_t {
    NONE               = 0,
    PATCHED            = 1,
    PARTIAL_REFITTED   = 2,
    REBUILT            = 3,
};

/**
 * @brief Update strategy used to publish the current manifest version.
 */
enum class UpdateMode : uint8_t {
    NONE          = 0,
    PATCH         = 1,
    PARTIAL_REFIT = 2,
    REBUILD       = 3,
};

/**
 * @brief Reason an artifact left the ACTIVE/STALE lifecycle.
 */
enum class InvalidationReason : uint8_t {
    NONE                = 0,
    STALE_DATA          = 1,
    INTEGRITY_FAILURE   = 2,
    RANK_CAP_BREACH     = 3,
    RESIDUAL_THRESHOLD  = 4,
    MANUAL_INVALIDATION = 5,
};

// ---------------------------------------------------------------------------
// ArtifactIntegrity
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight integrity token attached to each manifest entry.
 *
 * The checksum is a CRC-32 over the serialized payload bytes.  It is
 * verified on read to detect in-flight or at-rest corruption before an
 * artifact is handed to the planner.
 */
struct ArtifactIntegrity {
    /// CRC-32 over the artifact payload bytes; 0 means "not computed".
    uint32_t crc32 = 0;

    /// Number of payload bytes covered by @p crc32.
    uint64_t payload_bytes = 0;

    /// True when a non-zero CRC token is present (payload match is not verified here).
    [[nodiscard]] bool isValid() const noexcept { return crc32 != 0; }
};

// ---------------------------------------------------------------------------
// ArtifactManifest
// ---------------------------------------------------------------------------

/**
 * @brief A single entry in the distributed tensor artifact manifest.
 *
 * Each entry describes one advisory artifact associated with a named tensor.
 * The freshness age is derived from @p created_at relative to the caller's
 * wall-clock time; the planner must reject entries whose age exceeds the
 * configured threshold before using them for routing decisions.
 *
 * ### Example (Phase A, advisory summary):
 * @code
 * ArtifactManifest m;
 * m.artifact_id  = "ann-summary-users-v1";
 * m.tensor_name  = "users/embedding";
 * m.kind         = ArtifactKind::ADVISORY_SUMMARY;
 * m.shard_id     = 0;
 * m.version      = 1;
 * m.created_at   = std::chrono::system_clock::now();
 * m.integrity    = computeIntegrity(payload_bytes, payload_size);
 * @endcode
 */
struct ArtifactManifest {
    /// Unique identifier for this artifact (UUID or content-addressed key).
    std::string artifact_id;

    /// Logical tensor name in the format "<module>/<tensor>" (e.g. "users/embedding").
    std::string tensor_name;

    /// Classification of this artifact.
    ArtifactKind kind = ArtifactKind::ADVISORY_SUMMARY;

    /// Shard index this entry belongs to (0 = single-shard Phase A).
    uint32_t shard_id = 0;

    /// Monotonically increasing version; larger = newer.
    uint64_t version = 0;

    /// Wall-clock time when the artifact was created or last refreshed.
    std::chrono::system_clock::time_point created_at;

    /// Integrity token computed over the artifact payload.
    ArtifactIntegrity integrity;

    /// Current lifecycle state.
    LifecycleState lifecycle_state = LifecycleState::ACTIVE;

    /// Advisory-only invariant flag. This must stay true for all rollout phases here.
    bool advisory_only = true;

    /// Sequence range from the exact graph lineage captured by this artifact.
    uint64_t source_seq_start = 0;
    uint64_t source_seq_end   = 0;

    /// Delta backlog relative to the exact graph source.
    uint64_t delta_lag = 0;

    /// Current artifact age in milliseconds.
    uint64_t artifact_age_ms = 0;

    /// Last full rebuild time in seconds since Unix epoch.
    int64_t last_rebuild_at_unix_sec = 0;

    /// Freshness budget for staleness checks.
    int64_t staleness_threshold_sec = 0;

    /// Approximation residual for advisory quality checks.
    double residual = 0.0;

    /// Maximum permitted rank growth for partial refits (0 disables the cap).
    uint32_t rank_cap = 0;

    /// Current observed rank after the latest update.
    uint32_t rank_status = 0;

    /// Last maintenance action applied.
    RebuildState rebuild_state = RebuildState::NONE;

    /// Strategy used to produce the current version.
    UpdateMode update_mode = UpdateMode::NONE;

    /// Why the artifact was invalidated, if applicable.
    InvalidationReason invalidation_reason = InvalidationReason::NONE;

    /**
     * @brief Freshness age in seconds relative to @p now.
     *
     * @param now  Reference time point (default: wall clock).
     * @return     Age in seconds; negative values indicate clock skew and
     *             should be treated as stale.
     */
    [[nodiscard]] double freshnessAgeSeconds(
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const noexcept {
        using namespace std::chrono;
        const auto delta = now - created_at;
        return duration_cast<duration<double>>(delta).count();
    }

    /**
     * @brief True when this entry is within the given staleness budget.
     *
     * @param max_age_s  Maximum allowed age in seconds.
     * @param now        Reference time point (default: wall clock).
     * @return           true if the entry is fresh enough for planner use.
     */
    [[nodiscard]] bool isFresh(
        double max_age_s,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const noexcept {
        const double age = freshnessAgeSeconds(now);
        return age >= 0.0 && age <= max_age_s;
    }

    /**
     * @brief Validate baseline manifest invariants for Phase A/B processing.
     *
     * @return true when the manifest carries the minimum fields required for
     *         advisory publication and update-worker maintenance.
     */
    [[nodiscard]] bool validate() const noexcept {
        if (artifact_id.empty() || tensor_name.empty() || version == 0) {
            return false;
        }
        if (residual < 0.0) {
            return false;
        }
        if (rank_cap > 0 && rank_status > rank_cap) {
            return false;
        }
        if (source_seq_end > 0 && source_seq_start > source_seq_end) {
            return false;
        }
        return true;
    }

    /**
     * @brief Returns true when the artifact may still be consulted by the planner.
     *
     * INVALIDATED entries are always unusable; ACTIVE and STALE remain advisory.
     */
    [[nodiscard]] bool isUsable([[maybe_unused]] int64_t now_unix_sec = 0) const noexcept {
        return lifecycle_state == LifecycleState::ACTIVE
            || lifecycle_state == LifecycleState::STALE;
    }

    /**
     * @brief Returns true when the artifact exceeds its explicit staleness budget.
     */
    [[nodiscard]] bool isStale(int64_t now_unix_sec) const noexcept {
        if (staleness_threshold_sec <= 0) {
            return false;
        }

        const auto age_ms = computeArtifactAgeMs(now_unix_sec);
        return age_ms > static_cast<uint64_t>(staleness_threshold_sec) * 1000ULL;
    }

    /**
     * @brief Returns true when the artifact integrity token is absent or invalid.
     */
    [[nodiscard]] bool isCorrupted() const noexcept {
        return !integrity.isValid();
    }

    /**
     * @brief Refresh derived temporal fields after publication.
     */
    void markPublished(
        UpdateMode mode,
        RebuildState rebuild,
        uint64_t latest_source_seq,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) noexcept {
        created_at = now;
        update_mode = mode;
        rebuild_state = rebuild;
        source_seq_end = latest_source_seq;
        delta_lag = 0;
        artifact_age_ms = 0;
        lifecycle_state = LifecycleState::ACTIVE;
        if (rebuild == RebuildState::REBUILT) {
            last_rebuild_at_unix_sec = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();
        }
    }

    /**
     * @brief Update the cached artifact age for observability.
     */
    void refreshArtifactAge(
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) noexcept {
        const auto age = now - created_at;
        artifact_age_ms = static_cast<uint64_t>(std::max<int64_t>(
            0, std::chrono::duration_cast<std::chrono::milliseconds>(age).count()));
    }

private:
    [[nodiscard]] uint64_t computeArtifactAgeMs(int64_t now_unix_sec) const noexcept {
        const auto created_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            created_at.time_since_epoch()).count();
        const int64_t now_ms = now_unix_sec > 0 ? now_unix_sec * 1000 : created_ms;
        return static_cast<uint64_t>(std::max<int64_t>(0, now_ms - created_ms));
    }
};

} // namespace distributed_tensor
} // namespace themis
