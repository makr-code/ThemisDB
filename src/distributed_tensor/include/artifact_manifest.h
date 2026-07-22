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

/**
 * @brief Classification of a tensor artifact by data type.
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
// ArtifactClass — source-of-truth vs. derived artifact policy
// ---------------------------------------------------------------------------

/**
 * @brief Classifies whether an artifact is a source of truth or derived.
 *
 * SOURCE_OF_TRUTH is reserved for the Graph Truth Layer.  All tensor artifacts
 * produced by the ANN or tensor mid-layers MUST use DERIVED or EPHEMERAL.
 *
 * @invariant Tensor artifacts are NEVER SOURCE_OF_TRUTH.
 */
enum class ArtifactClass : uint8_t {
    /// Ground-truth artifact; exclusively owned by the Graph Truth Layer.
    SOURCE_OF_TRUTH = 0,
    /// Derived advisory artifact (tensor summaries, shard summaries, ANN candidates).
    DERIVED         = 1,
    /// Ephemeral cached computation; volatile, no provenance requirements.
    EPHEMERAL       = 2,
};

// ---------------------------------------------------------------------------
// TruthSemantic — advisory-only vs. authoritative
// ---------------------------------------------------------------------------

/**
 * @brief Whether an artifact carries authoritative or advisory-only semantics.
 *
 * @invariant Tensor artifacts are always ADVISORY_ONLY.
 * @invariant GROUND_TRUTH is reserved for SOURCE_OF_TRUTH artifacts only.
 */
enum class TruthSemantic : uint8_t {
    /// Advisory only; result must be verified against the Graph Truth Layer.
    ADVISORY_ONLY = 0,
    /// Authoritative ground truth; reserved for the Graph Truth Layer.
    GROUND_TRUTH  = 1,
};

// ---------------------------------------------------------------------------
// LifecycleState — derived-artifact FSM (issue #5442)
// ---------------------------------------------------------------------------

/**
 * @brief State machine for a derived artifact's lifecycle.
 *
 * Transition diagram:
 * @code
 *   READY ─────────────────► STALE ──────────────────► INVALIDATED
 *     ▲    (lag/age/residual)          (policy breach)       │
 *     │                                                      │
 *     │ (rebuild success)                          REBUILDING◄┘
 *     └──────────────────────────────────────────────────────┘
 *                                      │ (rebuild failure)
 *                                      ▼
 *                                   FAILED
 * @endcode
 *
 * Planner contract:
 *   - READY and STALE: artifact is usable; planner may use with advisory caveat.
 *   - INVALIDATED, REBUILDING, FAILED: artifact must NOT be used; planner must
 *     fall back to exact graph retrieval.
 */
enum class LifecycleState : uint8_t {
    /// Artifact is current and usable.  Identical to ACTIVE (legacy alias).
    READY        = 0,
    /// Legacy alias for READY; kept for backward compatibility.
    ACTIVE       = 0,
    /// Artifact has exceeded a freshness/lag/residual threshold; still usable
    /// but planner should prefer exact graph.
    STALE        = 1,
    /// Artifact is invalidated and MUST NOT be used until rebuilt.
    INVALIDATED  = 2,
    /// Rebuild or re-materialization is actively in progress.
    REBUILDING   = 3,
    /// Rebuild failed; artifact cannot be used; exact graph fallback required.
    FAILED       = 4,
};

// ---------------------------------------------------------------------------
// RebuildState — update path tracking
// ---------------------------------------------------------------------------

/**
 * @brief Records which update path last modified this artifact.
 *
 * Used by the planner to understand the artifact's provenance and decide
 * whether a full rebuild is preferable over incremental updates.
 */
enum class RebuildState : uint8_t {
    /// Artifact has never been rebuilt; original from initial materialization.
    PRISTINE         = 0,
    /// Artifact was patched (small delta window applied in-place).
    PATCHED          = 1,
    /// Artifact was partially refitted (medium delta window, selective retraining).
    PARTIAL_REFITTED = 2,
    /// Artifact was fully rebuilt from the source-of-truth.
    REBUILT          = 3,
};

// ---------------------------------------------------------------------------
// UpdateMode — which path is requested / was used
// ---------------------------------------------------------------------------

/**
 * @brief Requested or executed update strategy for a rebuild operation.
 */
enum class UpdateMode : uint8_t {
    /// No update requested / default-initialized
        NONE          = 0,
        /// Apply patch updates for small delta windows (delta_size < 10% artifact).
        PATCH         = 1,
        /// Apply partial refit for medium delta windows (10%–50% artifact size).
        PARTIAL_REFIT = 2,
        /// Full rebuild; required when delta_size > 50% or quality metrics breach thresholds.
        REBUILD       = 3,
};

// ---------------------------------------------------------------------------
// InvalidationReason — why an artifact was invalidated
// ---------------------------------------------------------------------------

/**
 * @brief Reason code recorded when an artifact transitions to INVALIDATED.
 *
 * Stored in the manifest for observability, triage, and policy audit.
 */
enum class InvalidationReason : uint8_t {
    /// Reason not recorded or unknown.
    UNKNOWN                  = 0,
    /// CRC-32 or hash validation failed — artifact payload is corrupted.
    INTEGRITY_CHECK_FAILED   = 1,
    /// Age or delta lag exceeded the configured staleness policy threshold.
    STALENESS_EXCEEDED       = 2,
    /// Parent or source-of-truth artifact was invalidated (cascade invalidation).
    SOURCE_INVALIDATED       = 3,
    /// Provenance chain is broken or evidence of tampering detected.
    SOURCE_LINEAGE_CORRUPTED = 4,
    /// Rank cap or residual threshold was breached.
    POLICY_VIOLATION         = 5,
    /// Explicit operator or admin invalidation.
    ADMIN_REQUESTED          = 6,
    /// One or more shards are unavailable and the artifact cannot be read.
    SHARD_UNAVAILABLE        = 7,
};

// ---------------------------------------------------------------------------
// Utility helper structs — string conversion for enums
// ---------------------------------------------------------------------------

/// @brief String conversion helpers for RebuildState.
struct RebuildStateUtils {
    /// @brief Convert a RebuildState to its canonical string representation.
    /// @param state  Rebuild state to convert.
    /// @return       Canonical uppercase string (e.g. "PRISTINE").
    static std::string stateToString(RebuildState state);

    /// @brief Parse a canonical string to RebuildState.
    /// @param state_str  String from stateToString().
    /// @return           Parsed state, or nullopt if unrecognized.
    static std::optional<RebuildState> stringToState(const std::string& state_str);
};

/// @brief String conversion helpers for UpdateMode.
struct UpdateModeUtils {
    /// @brief Convert an UpdateMode to its canonical string representation.
    /// @param mode  Update mode to convert.
    /// @return      Canonical lowercase string (e.g. "patch").
    static std::string modeToString(UpdateMode mode);

    /// @brief Parse a canonical string to UpdateMode.
    /// @param mode_str  String from modeToString().
    /// @return          Parsed mode, or nullopt if unrecognized.
    static std::optional<UpdateMode> stringToMode(const std::string& mode_str);
};

/// @brief String conversion helpers for InvalidationReason.
struct InvalidationReasonUtils {
    /// @brief Convert an InvalidationReason to its canonical string representation.
    /// @param reason  Reason to convert.
    /// @return        Canonical uppercase string (e.g. "STALENESS_EXCEEDED").
    static std::string reasonToString(InvalidationReason reason);

    /// @brief Parse a canonical string to InvalidationReason.
    /// @param reason_str  String from reasonToString().
    /// @return            Parsed reason, or nullopt if unrecognized.
    static std::optional<InvalidationReason> stringToReason(const std::string& reason_str);
};

// ---------------------------------------------------------------------------
// ArtifactLifecyclePolicy — state serialization / deserialization
// ---------------------------------------------------------------------------

/**
 * @brief Lifecycle state policy helpers for serialization and planner checks.
 *
 * Provides string conversion and usability predicates for LifecycleState.
 */
struct ArtifactLifecyclePolicy {
    /// @brief Convert a LifecycleState to its canonical string representation.
    /// @param state  Lifecycle state to convert.
    /// @return       Canonical uppercase string (e.g. "READY").
    static std::string stateToString(LifecycleState state);

    /// @brief Parse a canonical string to LifecycleState.
    /// @param state_str  String from stateToString().
    /// @return           Parsed state, or nullopt if unrecognized.
    static std::optional<LifecycleState> stringToState(const std::string& state_str);

    /// @brief Return true if the artifact in @p state is safe to use for planning.
    ///
    /// Only READY and STALE are usable; all other states require exact-graph fallback.
    ///
    /// @param state  Lifecycle state to test.
    /// @return       true if the planner may use an artifact in this state.
    static bool isUsableForPlanning(LifecycleState state) noexcept;
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
 * @brief Manifest entry for a distributed tensor artifact, including full
 *        lifecycle and staleness management metadata (issue #5442).
 *
 * ## Phase A fields (advisory store)
 * The fields @p artifact_id, @p tensor_name, @p kind, @p shard_id, @p version,
 * @p created_at, and @p integrity are used by the Phase A ManifestStore and
 * query planner.
 *
 * ## Lifecycle fields (issue #5442)
 * @p source_seq_start / @p source_seq_end — the exact-graph sequence window
 *   that this artifact was built from.
 * @p delta_lag — number of exact-graph commits not yet incorporated.
 * @p artifact_age_ms — wall-clock age at the time of last materialization (ms).
 * @p residual — approximation quality metric; higher value = more error.
 * @p rank_cap — maximum permissible rank for partial-refit update paths.
 * @p lifecycle_state — FSM state: READY → STALE → INVALIDATED → REBUILDING →
 *   READY (success) / FAILED (failure).
 *
 * ### State transition invariants
 * - Only a rebuild worker may transition from REBUILDING back to READY.
 * - The planner must NOT use artifacts in INVALIDATED, REBUILDING, or FAILED states.
 * - Transitioning to INVALIDATED requires recording an @p invalidation_reason.
 *
 * ### Example
 * @code
 * ArtifactManifest m;
 * m.artifact_id       = "ann-summary-users-v1";
 * m.tensor_name       = "users/embedding";
 * m.kind              = ArtifactKind::ADVISORY_SUMMARY;
 * m.artifact_class    = ArtifactClass::DERIVED;
 * m.truth_semantic    = TruthSemantic::ADVISORY_ONLY;
 * m.shard_id          = 0;
 * m.version           = 1;
 * m.source_seq_start  = 100;
 * m.source_seq_end    = 500;
 * m.delta_lag         = 12;
 * m.residual          = 0.02;
 * m.rank_cap          = 128;
 * m.lifecycle_state   = LifecycleState::READY;
 * m.created_at        = std::chrono::system_clock::now();
 * m.integrity.crc32   = computeCRC32(payload, payload_size);
 * @endcode
 */
struct ArtifactManifest {

    // ── Phase A advisory-store fields ────────────────────────────────────────

    /// Unique identifier for this artifact (UUID or content-addressed key).
    std::string artifact_id;

    /// Logical tensor name in the format "<module>/<tensor>" (e.g. "users/embedding").
    std::string tensor_name;

    /// Classification of this artifact by data type (Phase A: ADVISORY_SUMMARY only).
    ArtifactKind kind = ArtifactKind::ADVISORY_SUMMARY;

    /// Shard index this entry belongs to (0 = single-shard Phase A).
    uint32_t shard_id = 0;

    /// Monotonically increasing version number; larger = newer.
    uint64_t version = 0;

    /// Wall-clock time when the artifact was created or last refreshed.
    std::chrono::system_clock::time_point created_at;

    /// CRC-32 integrity token over the artifact payload.
    ArtifactIntegrity integrity;

    // ── Classification & truth semantics ────────────────────────────────────

    /// Source-of-truth vs. derived artifact classification.
    ArtifactClass artifact_class = ArtifactClass::DERIVED;

    /// Advisory-only vs. ground-truth semantic.  Tensor artifacts are always ADVISORY_ONLY.
    TruthSemantic truth_semantic = TruthSemantic::ADVISORY_ONLY;

    // ── Lifecycle state (issue #5442 FSM) ────────────────────────────────────

    /// Current lifecycle state in the READY/STALE/INVALIDATED/REBUILDING/FAILED FSM.
    LifecycleState lifecycle_state = LifecycleState::READY;

    /// Records which update path last modified this artifact.
    RebuildState rebuild_state = RebuildState::PRISTINE;

    /// Requested or executed update strategy for the most recent rebuild operation.
    UpdateMode update_mode = UpdateMode::REBUILD;

    /// Reason code set when the artifact transitions to INVALIDATED.
    InvalidationReason invalidation_reason = InvalidationReason::UNKNOWN;

    // ── Versioning & content integrity ──────────────────────────────────────

    /// Textual version identifier (e.g. semver or content hash tag).
    std::string content_hash;

    /// Hash of the manifest metadata itself (for tamper detection).
    std::string manifest_hash;

    // ── Temporal metadata ────────────────────────────────────────────────────

    /// Unix timestamp (seconds) when the artifact was first created.
    int64_t created_at_unix_sec = 0;

    /// Unix timestamp (seconds) of the most recent update to this manifest.
    int64_t updated_at_unix_sec = 0;

    /// Unix timestamp (seconds) of the most recent integrity verification.
    int64_t last_verified_unix_sec = 0;

    /// Unix timestamp (seconds) of the most recent full or partial rebuild.
    int64_t last_rebuild_at_unix_sec = 0;

    /// Configured staleness threshold in seconds; 0 = no threshold (always fresh).
    int64_t staleness_threshold_sec = 0;

    /// Age of the artifact in milliseconds at the time it was last materialized.
    uint64_t artifact_age_ms = 0;

    // ── Sequence & delta-log tracking (issue #5442) ──────────────────────────

    /// Lowest exact-graph sequence number incorporated into this artifact.
    uint64_t source_seq_start = 0;

    /// Highest exact-graph sequence number incorporated into this artifact.
    uint64_t source_seq_end = 0;

    /// Number of exact-graph commits after @p source_seq_end not yet incorporated.
    /// @invariant delta_lag = current_graph_head - source_seq_end.
    uint64_t delta_lag = 0;

    // ── Approximation & quality metrics (issue #5442) ────────────────────────

    /// Approximation quality metric: higher value = more error.
    /// Range [0.0, 1.0] for normalized error; may exceed 1.0 for unnormalized.
    double residual = 0.0;

    /// Maximum permissible rank for partial-refit update paths.
    /// 0 = no cap (unbounded rank).
    uint32_t rank_cap = 0;

    /// Current effective rank of the artifact.
    /// @invariant rank_status <= rank_cap (when rank_cap > 0).
    uint32_t rank_status = 0;

    // ── Provenance & reconstruction ──────────────────────────────────────────

    /// Identifier of the source artifact this was derived from (if any).
    std::string source_artifact_id;

    /// Ordered chain of artifact IDs forming the provenance lineage.
    std::vector<std::string> provenance_chain;

    /// Serialized instructions for re-materializing this artifact from its source.
    std::string reconstruction_instructions;

    // ── Placement & distribution ─────────────────────────────────────────────

    /// Shard placement identifiers for this artifact's shards.
    std::vector<std::string> shard_placements;

    /// Whether all shards must be replicated before the artifact is considered ready.
    bool requires_full_replication = false;

    /// Whether this artifact can be rebuilt from available sources.
    bool is_rebuildable = true;

    // ── Redundancy strategy ──────────────────────────────────────────────────

    /// Replication factor for this artifact's shards.
    uint32_t replication_factor = 1;

    /// Erasure coding scheme identifier (e.g. "8+4").
    std::string erasure_coding_scheme;

    /// Backup shard placement identifiers.
    std::vector<std::string> backup_shard_placements;

    // ── Compatibility & planner constraints ──────────────────────────────────

    /// Planner compatibility metadata (key-value pairs).
    std::map<std::string, std::string> compatibility_metadata;

    /// Minimum planner version required to consume this artifact.
    std::string min_planner_version;

    /// Whether this artifact is advisory-only (must always be true for tensor artifacts).
    bool advisory_only = true;

    // ── Custom metadata & description ────────────────────────────────────────

    /// Arbitrary user-defined key-value attributes.
    std::map<std::string, std::string> custom_attributes;

    /// Optional human-readable description of this artifact.
    std::string description;

    // ── Phase A advisory-store methods ───────────────────────────────────────

    /**
     * @brief Freshness age in seconds relative to @p now.
     *
     * Uses the @p created_at (chrono) field, which is set by the Phase A ManifestStore.
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
     * @brief True when this entry is within the given Phase A staleness budget.
     *
     * Uses the @p created_at (chrono) field and @p max_age_s.  For lifecycle-state
     * based staleness, use @p isStale(now_unix_sec) instead.
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

    // ── Lifecycle management methods (issue #5442) ───────────────────────────

    /**
     * @brief Validate this manifest for consistency and policy compliance.
     *
     * Checks:
     *   - @p artifact_id is non-empty.
     *   - @p source_seq_end >= @p source_seq_start (when both are non-zero).
     *   - @p residual is in a reasonable range [0.0, 1000.0].
     *   - @p rank_status <= @p rank_cap (when rank_cap > 0).
     *   - @p replication_factor >= 1.
     *   - timestamp consistency (updated_at >= created_at).
     *   - @p artifact_class / @p truth_semantic combination is valid.
     *
     * @return true if all invariants hold; false on any violation.
     */
    [[nodiscard]] bool validate() const;

    /**
     * @brief Return true if this artifact may be used for query planning.
     *
     * Only READY (ACTIVE) and STALE artifacts are usable.  All other states
     * require the caller to fall back to exact-graph retrieval.
     *
     * @param now_unix_sec  Current time in Unix seconds (unused in this check;
     *                      reserved for future time-gated usability logic).
     * @return              true if the planner may use this artifact.
     */
    [[nodiscard]] bool isUsable(int64_t now_unix_sec = 0) const;

    /**
     * @brief Return true if this artifact has exceeded its staleness threshold.
     *
     * Checks @p last_verified_unix_sec against @p staleness_threshold_sec.
     * If @p staleness_threshold_sec is 0, the artifact is never considered stale
     * by this method.
     *
     * @param now_unix_sec  Current time in Unix seconds.
     * @return              true if the artifact is stale.
     */
    [[nodiscard]] bool isStale(int64_t now_unix_sec) const;

    /**
     * @brief Freshness score in [0.0, 1.0]; 1.0 = freshest, 0.0 = fully stale.
     *
     * @param now_unix_sec  Current time in Unix seconds.
     * @return              Freshness score; 1.0 when age is 0, 0.0 when age >=
     *                      staleness_threshold_sec.
     */
    [[nodiscard]] double getFreshnessScore(int64_t now_unix_sec) const;

    /**
     * @brief Return true if the manifest_hash indicates content corruption.
     *
     * Recomputes a hash of the core manifest fields and compares it to
     * @p manifest_hash.  Returns false if @p manifest_hash is empty.
     *
     * @return true if corruption is detected.
     */
    [[nodiscard]] bool isCorrupted() const;

    /**
     * @brief Serialize this manifest to a JSON string.
     *
     * @return JSON string representing all manifest fields.
     */
    [[nodiscard]] std::string toJSON() const;

    /**
     * @brief Deserialize a manifest from a JSON string.
     *
     * @param json_str  JSON string produced by toJSON().
     * @return          Populated manifest on success, nullopt on parse error.
     */
    [[nodiscard]] static std::optional<ArtifactManifest> fromJSON(
        const std::string& json_str);

    /**
     * @brief Serialize this manifest to a YAML-style string.
     *
     * @return YAML-style string representing core manifest fields.
     */
    [[nodiscard]] std::string toYAML() const;

    /**
     * @brief Deserialize a manifest from a YAML-style (or JSON) string.
     *
     * @param yaml_str  String produced by toYAML() or toJSON().
     * @return          Populated manifest on success, nullopt on parse error.
     */
    [[nodiscard]] static std::optional<ArtifactManifest> fromYAML(
        const std::string& yaml_str);

    /**
     * @brief Mark this artifact as published after a successful update.
     *
     * Updates the manifest metadata to reflect a completed update operation:
     *   - Sets @p rebuild_state to indicate the type of update performed
     *   - Sets @p source_seq_end to reflect the latest included sequence
     *   - Updates @p last_verified_unix_sec to mark the time of publish
     *   - Resets @p delta_lag based on new source_seq_end
     *
     * This is called by the snapshot-based update worker after patch/refit/rebuild.
     *
     * @param mode              Update mode (PATCH, PARTIAL_REFIT, REBUILD)
     * @param rebuild_state     State to record (PATCHED, REFITTED, REBUILT)
     * @param new_source_seq    Latest exact-graph sequence now incorporated
     */
    void markPublished(UpdateMode mode, RebuildState rebuild_state,
                      uint64_t new_source_seq);
};

} // namespace distributed_tensor
} // namespace themis
