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

    /// True when crc32 != 0 and matches the current payload.
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
};

} // namespace distributed_tensor
} // namespace themis
