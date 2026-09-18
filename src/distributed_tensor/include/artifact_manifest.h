/**
 * @file artifact_manifest.h
 * @brief Tensor artifact manifest schema and derived-artifact lifecycle management.
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
 * ## Derived-Artifact Lifecycle (issue #5442)
 *
 * Each artifact transitions through states: READY → STALE → INVALIDATED →
 * REBUILDING → READY (on success) or FAILED (on rebuild failure).  Only
 * READY and STALE artifacts are usable by the planner.  INVALIDATED, REBUILDING,
 * and FAILED artifacts require exact-graph fallback.
 *
 * Lifecycle fields:
 *   - @p source_seq_start / @p source_seq_end — sequence window this artifact covers.
 *   - @p delta_lag — gap between source_seq_end and the current exact-graph head.
 *   - @p artifact_age_ms — age of this artifact in milliseconds at the time of last update.
 *   - @p residual — approximation quality metric; higher = more error.
 *   - @p rank_cap — maximum permissible rank for partial-refit paths.
 *   - @p lifecycle_state — current state in the READY/STALE/INVALIDATED/REBUILDING/FAILED FSM.
 *
 * @see ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 Phase A
 * @see ManifestStore for the storage and freshness API
 * @see StaleArtifactDetector for staleness detection logic
 * @see ArtifactInvalidationManager for invalidation triggers
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// ArtifactKind — Phase A / B / C entry gate
// ---------------------------------------------------------------------------

enum class ArtifactKind : uint8_t {
    ADVISORY_SUMMARY = 0,
    DELTA_LOG        = 1,
    SHARD_SUMMARY    = 2,
};

// ---------------------------------------------------------------------------
// ArtifactClass — source-of-truth vs. derived artifact policy
// ---------------------------------------------------------------------------

enum class ArtifactClass : uint8_t {
    SOURCE_OF_TRUTH = 0,
    DERIVED         = 1,
    EPHEMERAL       = 2,
};

// ---------------------------------------------------------------------------
// TruthSemantic — advisory-only vs. authoritative
// ---------------------------------------------------------------------------

enum class TruthSemantic : uint8_t {
    ADVISORY_ONLY = 0,
    GROUND_TRUTH  = 1,
};

// ---------------------------------------------------------------------------
// LifecycleState — derived-artifact FSM (issue #5442)
// ---------------------------------------------------------------------------

enum class LifecycleState : uint8_t {
    READY        = 0,
    ACTIVE       = 0,
    STALE        = 1,
    INVALIDATED  = 2,
    REBUILDING   = 3,
    FAILED       = 4,
};

// ---------------------------------------------------------------------------
// RebuildState — update path tracking
// ---------------------------------------------------------------------------

enum class RebuildState : uint8_t {
    PRISTINE         = 0,
    PATCHED          = 1,
    PARTIAL_REFITTED = 2,
    REBUILT          = 3,
};

// ---------------------------------------------------------------------------
// UpdateMode — which path is requested / was used
// ---------------------------------------------------------------------------

enum class UpdateMode : uint8_t {
        NONE          = 0,
        PATCH         = 1,
        PARTIAL_REFIT = 2,
        REBUILD       = 3,
};

// ---------------------------------------------------------------------------
// InvalidationReason — why an artifact was invalidated
// ---------------------------------------------------------------------------

enum class InvalidationReason : uint8_t {
    UNKNOWN                  = 0,
    INTEGRITY_CHECK_FAILED   = 1,
    STALENESS_EXCEEDED       = 2,
    SOURCE_INVALIDATED       = 3,
    SOURCE_LINEAGE_CORRUPTED = 4,
    POLICY_VIOLATION         = 5,
    ADMIN_REQUESTED          = 6,
    SHARD_UNAVAILABLE        = 7,
};

// ---------------------------------------------------------------------------
// Utility helper structs — string conversion for enums
// ---------------------------------------------------------------------------

struct RebuildStateUtils {
    /**
     * @brief State To String.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    static std::string stateToString(RebuildState state);

    /**
     * @brief String To State.
     * @param[in] state_str Input parameter.
     * @return Return value.
     */
    static std::optional<RebuildState> stringToState(const std::string& state_str);
};

struct UpdateModeUtils {
    /**
     * @brief Mode To String.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    static std::string modeToString(UpdateMode mode);

    /**
     * @brief String To Mode.
     * @param[in] mode_str Input parameter.
     * @return Return value.
     */
    static std::optional<UpdateMode> stringToMode(const std::string& mode_str);
};

struct InvalidationReasonUtils {
    /**
     * @brief Reason To String.
     * @param[in] reason Input parameter.
     * @return Return value.
     */
    static std::string reasonToString(InvalidationReason reason);

    /**
     * @brief String To Reason.
     * @param[in] reason_str Input parameter.
     * @return Return value.
     */
    static std::optional<InvalidationReason> stringToReason(const std::string& reason_str);
};

// ---------------------------------------------------------------------------
// ArtifactLifecyclePolicy — state serialization / deserialization
// ---------------------------------------------------------------------------

struct ArtifactLifecyclePolicy {
    /**
     * @brief State To String.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    static std::string stateToString(LifecycleState state);

    /**
     * @brief String To State.
     * @param[in] state_str Input parameter.
     * @return Return value.
     */
    static std::optional<LifecycleState> stringToState(const std::string& state_str);

    /**
     * @brief Is Usable For Planning.
     * @param[in] state Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isUsableForPlanning(LifecycleState state) noexcept;
};

// ---------------------------------------------------------------------------
// ArtifactIntegrity
// ---------------------------------------------------------------------------

struct ArtifactIntegrity {
    uint32_t crc32 = 0;

    uint64_t payload_bytes = 0;

    [[nodiscard]] bool isValid() const noexcept { return crc32 != 0; }
};

// ---------------------------------------------------------------------------
// ArtifactManifest
// ---------------------------------------------------------------------------

struct ArtifactManifest {

    // ── Phase A advisory-store fields ────────────────────────────────────────

    std::string artifact_id;

    std::string tensor_name;

    ArtifactKind kind = ArtifactKind::ADVISORY_SUMMARY;

    uint32_t shard_id = 0;

    uint64_t version = 0;

    std::chrono::system_clock::time_point created_at;

    ArtifactIntegrity integrity;

    // ── Classification & truth semantics ────────────────────────────────────

    ArtifactClass artifact_class = ArtifactClass::DERIVED;

    TruthSemantic truth_semantic = TruthSemantic::ADVISORY_ONLY;

    // ── Lifecycle state (issue #5442 FSM) ────────────────────────────────────

    LifecycleState lifecycle_state = LifecycleState::READY;

    RebuildState rebuild_state = RebuildState::PRISTINE;

    UpdateMode update_mode = UpdateMode::REBUILD;

    InvalidationReason invalidation_reason = InvalidationReason::UNKNOWN;

    // ── Versioning & content integrity ──────────────────────────────────────

    std::string content_hash;

    std::string manifest_hash;

    // ── Temporal metadata ────────────────────────────────────────────────────

    int64_t created_at_unix_sec = 0;

    int64_t updated_at_unix_sec = 0;

    int64_t last_verified_unix_sec = 0;

    int64_t last_rebuild_at_unix_sec = 0;

    int64_t staleness_threshold_sec = 0;

    uint64_t artifact_age_ms = 0;

    // ── Sequence & delta-log tracking (issue #5442) ──────────────────────────

    uint64_t source_seq_start = 0;

    uint64_t source_seq_end = 0;

    uint64_t delta_lag = 0;

    // ── Approximation & quality metrics (issue #5442) ────────────────────────

    double residual = 0.0;

    uint32_t rank_cap = 0;

    uint32_t rank_status = 0;

    // ── Provenance & reconstruction ──────────────────────────────────────────

    std::string source_artifact_id;

    std::vector<std::string> provenance_chain;

    std::string reconstruction_instructions;

    // ── Placement & distribution ─────────────────────────────────────────────

    std::vector<std::string> shard_placements;

    bool requires_full_replication = false;

    bool is_rebuildable = true;

    // ── Redundancy strategy ──────────────────────────────────────────────────

    uint32_t replication_factor = 1;

    std::string erasure_coding_scheme;

    std::vector<std::string> backup_shard_placements;

    // ── Compatibility & planner constraints ──────────────────────────────────

    std::map<std::string, std::string> compatibility_metadata;

    std::string min_planner_version;

    bool advisory_only = true;

    // ── Custom metadata & description ────────────────────────────────────────

    std::map<std::string, std::string> custom_attributes;

    std::string description;

    // ── Phase A advisory-store methods ───────────────────────────────────────

    [[nodiscard]] double freshnessAgeSeconds(
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const noexcept {
        using namespace std::chrono;
        const auto delta = now - created_at;
        return duration_cast<duration<double>>(delta).count();
    }

    [[nodiscard]] bool isFresh(
        double max_age_s,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const noexcept {
        const double age = freshnessAgeSeconds(now);
        return age >= 0.0 && age <= max_age_s;
    }

    // ── Lifecycle management methods (issue #5442) ───────────────────────────

    [[nodiscard]] bool validate() const;

    [[nodiscard]] bool isUsable(int64_t now_unix_sec = 0) const;

    [[nodiscard]] bool isStale(int64_t now_unix_sec) const;

    [[nodiscard]] double getFreshnessScore(int64_t now_unix_sec) const;

    [[nodiscard]] bool isCorrupted() const;

    [[nodiscard]] std::string toJSON() const;

    [[nodiscard]] static std::optional<ArtifactManifest> fromJSON(
        const std::string& json_str);

    [[nodiscard]] std::string toYAML() const;

    [[nodiscard]] static std::optional<ArtifactManifest> fromYAML(
        const std::string& yaml_str);

    /**
     * @brief Mark Published.
     * @param[in] mode Input parameter.
     * @param[in] rebuild_state Input parameter.
     * @param[in] new_source_seq Input parameter.
     */
    void markPublished(UpdateMode mode, RebuildState rebuild_state,
                      uint64_t new_source_seq);
};

} // namespace distributed_tensor
} // namespace themis
