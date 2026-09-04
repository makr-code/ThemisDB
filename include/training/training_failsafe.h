/**
 * @file training_failsafe.h
 * @brief Fail-safe behavior standardization for training fault classes (Phase 3).
 *
 * Provides three focused failsafe handlers that enforce fail-closed or
 * graceful-degradation policies for the three primary training fault classes:
 *
 *  - @ref CheckpointFaultHandler  – fail-closed for checkpoint corruption, I/O
 *    faults, and disk exhaustion; supports rollback to last verified checkpoint.
 *
 *  - @ref AdapterMergeFailsafe    – fail-safe for merge failures; detects
 *    partial-merge states and rolls back to the pre-merge adapter state.
 *
 *  - @ref EnrichmentGapHandler    – graceful degradation for enrichment gaps;
 *    returns partial results with a coverage ratio and emits diagnostics rather
 *    than failing the entire enrichment pass.
 *
 * All handlers are stateless value types — the caller owns lifecycle and
 * threading. Incident emission uses @ref TrainingIncidentEmitter when a
 * non-null pointer is supplied; pass nullptr to suppress incident output.
 *
 * Thread safety: the handlers themselves contain no mutable state. The
 * TrainingIncidentEmitter passed in must be externally thread-safe (it is,
 * by design — see training_incident_emitter.h).
 *
 * Usage example (checkpoint):
 * @code
 * CheckpointFaultHandler handler;
 * CheckpointFaultContext ctx;
 * ctx.fault_code   = TrainingErrorCode::CHECKPOINT_MANIFEST_INVALID;
 * ctx.checkpoint_path = "/data/checkpoints/step_100";
 * ctx.has_fallback    = true;
 * ctx.fallback_path   = "/data/checkpoints/step_050";
 *
 * auto result = handler.handle(ctx, emitter.get());
 * if (result.strategy == CheckpointFaultHandler::RecoveryStrategy::ROLLBACK) {
 *     // resume from result.recovery_path
 * }
 * @endcode
 *
 * @version 1.0.0
 * @date 2026-08-10
 * @since v2.4.0 (Phase 3: Error Handling & Edge Cases)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/training_error_codes.h"
#include "training/training_incident_emitter.h"

#include <cstddef>
#include <string>
#include <vector>

namespace themis {
namespace training {

// ============================================================================
// CheckpointFaultHandler
// ============================================================================

/**
 * @brief Input context for a checkpoint fault handling decision.
 */
struct CheckpointFaultContext {
    /// Error code that triggered the fault.
    TrainingErrorCode fault_code = TrainingErrorCode::SUCCESS;
    /// Path of the checkpoint that encountered the fault.
    std::string checkpoint_path;
    /// Whether a verified fallback checkpoint is available.
    bool has_fallback = false;
    /// Path of the fallback checkpoint (ignored when has_fallback is false).
    std::string fallback_path;
    /// Whether the fault occurred during a save (vs. load/verify) operation.
    bool during_save = false;
    /// Additional diagnostic context (key=value pairs, space-separated).
    std::string context_kv;
};

/**
 * @brief Result of a checkpoint fault handling decision.
 */
struct CheckpointFaultResult {
    /**
     * @brief Resolved recovery strategy.
     *
     * - ROLLBACK:   Use recovery_path to resume from a verified prior state.
     * - ABORT:      No recovery is possible; training must stop.
     * - SKIP_SAVE:  Fault was during save and is non-fatal; training continues
     *               without this checkpoint being persisted.
     */
    enum class RecoveryStrategy : uint8_t { ROLLBACK, ABORT, SKIP_SAVE };

    RecoveryStrategy strategy       = RecoveryStrategy::ABORT;
    std::string      recovery_path; ///< Valid only when strategy == ROLLBACK.
    std::string      reason;        ///< Human-readable reason for the decision.
    bool             incident_emitted = false;
};

/**
 * @brief Fail-closed handler for checkpoint faults.
 *
 * Decision table:
 * | Fault class                          | has_fallback | during_save | Strategy  |
 * |--------------------------------------|-------------|-------------|-----------|
 * | CORRUPTION / INTEGRITY / INVALID     | yes         | any         | ROLLBACK  |
 * | CORRUPTION / INTEGRITY / INVALID     | no          | any         | ABORT     |
 * | DISK_SPACE_EXHAUSTED                 | any         | yes         | SKIP_SAVE |
 * | DISK_SPACE_EXHAUSTED                 | yes         | no          | ROLLBACK  |
 * | DISK_SPACE_EXHAUSTED                 | no          | no          | ABORT     |
 * | IO_WRITE / IO_READ / TIMEOUT         | yes         | any         | ROLLBACK  |
 * | IO_WRITE / IO_READ / TIMEOUT         | no          | yes         | SKIP_SAVE |
 * | IO_WRITE / IO_READ / TIMEOUT         | no          | no          | ABORT     |
 * | Other                                | yes         | any         | ROLLBACK  |
 * | Other                                | no          | any         | ABORT     |
 *
 * All handled faults emit an incident to @p emitter when non-null.
 */
class CheckpointFaultHandler {
public:
    using RecoveryStrategy = CheckpointFaultResult::RecoveryStrategy;

    /**
     * @brief Evaluate a checkpoint fault and return the recovery decision.
     *
     * @param ctx     Fault context (path, code, fallback availability).
     * @param emitter Optional incident emitter; pass nullptr to suppress output.
     * @return Resolved @ref CheckpointFaultResult.
     */
    [[nodiscard]] CheckpointFaultResult handle(
        const CheckpointFaultContext& ctx,
        TrainingIncidentEmitter*      emitter) const noexcept;

private:
    /**
     * @brief Classify the fault code into broad fault families.
     *
     * @param code Error code to classify.
     * @return true if the fault indicates data corruption or integrity failure.
     */
    [[nodiscard]] static bool isCorruptionFault(TrainingErrorCode code) noexcept;

    /**
     * @brief Return true if the fault is a disk/resource exhaustion fault.
     *
     * @param code Error code to classify.
     * @return true for disk space or I/O resource exhaustion.
     */
    [[nodiscard]] static bool isDiskExhaustionFault(TrainingErrorCode code) noexcept;

    /**
     * @brief Return true if the fault is a transient I/O or timeout fault.
     *
     * @param code Error code to classify.
     * @return true for I/O read/write errors and timeouts.
     */
    [[nodiscard]] static bool isTransientIoFault(TrainingErrorCode code) noexcept;
};

// ============================================================================
// AdapterMergeFailsafe
// ============================================================================

/**
 * @brief Snapshot of adapter state captured before a merge operation begins.
 *
 * The snapshot is lightweight — it stores metadata only, not weight tensors.
 * Recovery from a merge fault restores the adapter identity so the caller
 * can reload from a verified checkpoint rather than using a partial result.
 */
struct AdapterMergeSnapshot {
    std::string adapter_id;      ///< Stable identifier for the adapter.
    std::string checkpoint_path; ///< Path to the last verified checkpoint.
    std::string adapter_version; ///< Version tag at time of snapshot.
    size_t      layer_count = 0; ///< Number of layers in the adapter.
};

/**
 * @brief Context for an adapter merge fault decision.
 */
struct AdapterMergeFaultContext {
    /// Error code that triggered the fault.
    TrainingErrorCode fault_code = TrainingErrorCode::SUCCESS;
    /// Snapshot taken before the merge operation started.
    AdapterMergeSnapshot pre_merge_snapshot;
    /// Number of layers successfully merged before the fault occurred.
    size_t layers_merged_before_fault = 0;
    /// Total layers in the merge plan.
    size_t total_layers = 0;
    /// Additional diagnostic context.
    std::string context_kv;
};

/**
 * @brief Result of an adapter merge fault handling decision.
 */
struct AdapterMergeFaultResult {
    /**
     * @brief Resolved recovery strategy.
     *
     * - ROLLBACK:     Discard partial merge; restore from pre_merge_snapshot.
     * - ABORT_MERGE:  Merge cannot be completed; caller should stop and
     *                 report the fault upstream.
     */
    enum class RecoveryStrategy : uint8_t { ROLLBACK, ABORT_MERGE };

    RecoveryStrategy     strategy          = RecoveryStrategy::ABORT_MERGE;
    AdapterMergeSnapshot restore_snapshot; ///< Valid only for ROLLBACK.
    std::string          reason;
    bool                 incident_emitted  = false;
    /// True when layers_merged_before_fault > 0 (partial merge was detected).
    bool                 partial_merge_detected = false;
};

/**
 * @brief Fail-safe handler for adapter merge faults.
 *
 * Policy:
 * - Any fault during a merge that has a valid pre-merge snapshot triggers
 *   ROLLBACK to the snapshot, regardless of how many layers were merged.
 * - If the snapshot is empty (no adapter_id), strategy is ABORT_MERGE.
 * - Partial merges (layers_merged_before_fault > 0) always emit a
 *   PARTIAL_MERGE diagnostic incident before the rollback decision.
 */
class AdapterMergeFailsafe {
public:
    using RecoveryStrategy = AdapterMergeFaultResult::RecoveryStrategy;

    /**
     * @brief Evaluate a merge fault and return the recovery decision.
     *
     * @param ctx     Merge fault context.
     * @param emitter Optional incident emitter.
     * @return Resolved @ref AdapterMergeFaultResult.
     */
    [[nodiscard]] AdapterMergeFaultResult handle(
        const AdapterMergeFaultContext& ctx,
        TrainingIncidentEmitter*        emitter) const noexcept;
};

// ============================================================================
// EnrichmentGapHandler
// ============================================================================

/**
 * @brief Summary of enrichment gap coverage returned by EnrichmentGapHandler.
 */
struct EnrichmentGapSummary {
    size_t total_items      = 0; ///< Total items submitted for enrichment.
    size_t enriched_items   = 0; ///< Items successfully enriched.
    size_t gap_items        = 0; ///< Items that could not be enriched.
    double coverage_ratio   = 0.0; ///< enriched_items / total_items (0.0–1.0).

    /// True when coverage_ratio >= minimum_coverage_threshold.
    bool   meets_threshold  = false;
    /// Human-readable description of the gap source.
    std::string gap_reason;
    bool        incident_emitted = false;
};

/**
 * @brief Context for an enrichment gap evaluation.
 */
struct EnrichmentGapContext {
    size_t total_items     = 0; ///< Total items in the enrichment batch.
    size_t enriched_items  = 0; ///< Items successfully enriched.
    /// Minimum acceptable coverage ratio (0.0–1.0). Default: 0.5.
    double minimum_coverage_threshold = 0.5;
    /// Source component that produced the gap (e.g., "knowledge_graph_enricher").
    std::string source_component;
    /// Operation name (e.g., "enrich_batch").
    std::string operation;
    /// Additional diagnostic context.
    std::string context_kv;
};

/**
 * @brief Graceful-degradation handler for enrichment gaps.
 *
 * Policy:
 * - Always returns a @ref EnrichmentGapSummary with the computed coverage ratio.
 * - Emits a DATASET incident when coverage_ratio < minimum_coverage_threshold.
 * - Does NOT raise exceptions — callers decide whether to proceed with partial
 *   enrichment based on meets_threshold.
 * - A total_items == 0 input is treated as coverage_ratio = 1.0 (no gap).
 */
class EnrichmentGapHandler {
public:
    /**
     * @brief Evaluate enrichment gap coverage and emit diagnostics.
     *
     * @param ctx     Enrichment gap context (counts + threshold).
     * @param emitter Optional incident emitter.
     * @return @ref EnrichmentGapSummary with coverage ratio and threshold flag.
     */
    [[nodiscard]] EnrichmentGapSummary evaluate(
        const EnrichmentGapContext& ctx,
        TrainingIncidentEmitter*    emitter) const noexcept;
};

// ============================================================================
// Inline implementations
// ============================================================================

inline bool CheckpointFaultHandler::isCorruptionFault(TrainingErrorCode code) noexcept {
    switch (code) {
        case TrainingErrorCode::CHECKPOINT_MANIFEST_INVALID:
        case TrainingErrorCode::CHECKPOINT_SHA256_MISMATCH:
        case TrainingErrorCode::CHECKPOINT_TRUNCATED:
        case TrainingErrorCode::CHECKPOINT_INVALID_FORMAT:
            return true;
        default:
            return false;
    }
}

inline bool CheckpointFaultHandler::isDiskExhaustionFault(TrainingErrorCode code) noexcept {
    return code == TrainingErrorCode::CHECKPOINT_DISK_SPACE_EXHAUSTED;
}

inline bool CheckpointFaultHandler::isTransientIoFault(TrainingErrorCode code) noexcept {
    switch (code) {
        case TrainingErrorCode::CHECKPOINT_WRITE_FAILED:
        case TrainingErrorCode::CHECKPOINT_READ_FAILED:
        case TrainingErrorCode::CHECKPOINT_IO_TIMEOUT:
        case TrainingErrorCode::CHECKPOINT_VALIDATION_TIMEOUT:
            return true;
        default:
            return false;
    }
}

inline CheckpointFaultResult CheckpointFaultHandler::handle(
    const CheckpointFaultContext& ctx,
    TrainingIncidentEmitter*      emitter) const noexcept
{
    CheckpointFaultResult result = {};

    if (isCorruptionFault(ctx.fault_code) || isTransientIoFault(ctx.fault_code)) {
        if (ctx.has_fallback && !ctx.fallback_path.empty()) {
            result.strategy      = RecoveryStrategy::ROLLBACK;
            result.recovery_path = ctx.fallback_path;
            result.reason        = "checkpoint fault — rolling back to verified fallback";
        } else if (ctx.during_save && isTransientIoFault(ctx.fault_code)) {
            result.strategy = RecoveryStrategy::SKIP_SAVE;
            result.reason   = "transient I/O fault during save — skipping checkpoint; training continues";
        } else {
            result.strategy = RecoveryStrategy::ABORT;
            result.reason   = "checkpoint fault — no fallback available; training aborted";
        }
    } else if (isDiskExhaustionFault(ctx.fault_code)) {
        if (ctx.during_save) {
            result.strategy = RecoveryStrategy::SKIP_SAVE;
            result.reason   = "disk exhaustion during save — skipping checkpoint; training continues";
        } else if (ctx.has_fallback && !ctx.fallback_path.empty()) {
            result.strategy      = RecoveryStrategy::ROLLBACK;
            result.recovery_path = ctx.fallback_path;
            result.reason        = "disk exhaustion during load — rolling back to verified fallback";
        } else {
            result.strategy = RecoveryStrategy::ABORT;
            result.reason   = "disk exhaustion — no fallback; training aborted";
        }
    } else {
        // Generic / unknown fault: rollback if fallback is available.
        if (ctx.has_fallback && !ctx.fallback_path.empty()) {
            result.strategy      = RecoveryStrategy::ROLLBACK;
            result.recovery_path = ctx.fallback_path;
            result.reason        = "checkpoint fault — rolling back to verified fallback";
        } else {
            result.strategy = RecoveryStrategy::ABORT;
            result.reason   = "checkpoint fault — no fallback; training aborted";
        }
    }

    if (emitter != nullptr) {
        const bool recoverable = (result.strategy != RecoveryStrategy::ABORT);
        std::string kv = "checkpoint_path=" + ctx.checkpoint_path
                       + " strategy=" + (result.strategy == RecoveryStrategy::ROLLBACK ? "rollback"
                                       : result.strategy == RecoveryStrategy::SKIP_SAVE ? "skip_save"
                                       : "abort");
        if (!ctx.context_kv.empty()) {
            kv += " " + ctx.context_kv;
        }
        emitter->emitTrainingIncident(
            ctx.fault_code,
            "CheckpointFaultHandler",
            "handle",
            result.reason,
            recoverable,
            kv);
        result.incident_emitted = true;
    }

    return result;
}

inline AdapterMergeFaultResult AdapterMergeFailsafe::handle(
    const AdapterMergeFaultContext& ctx,
    TrainingIncidentEmitter*        emitter) const noexcept
{
    AdapterMergeFaultResult result;

    result.partial_merge_detected = (ctx.layers_merged_before_fault > 0);

    if (ctx.pre_merge_snapshot.adapter_id.empty()) {
        result.strategy = RecoveryStrategy::ABORT_MERGE;
        result.reason   = "merge fault — no pre-merge snapshot; cannot roll back";
    } else {
        result.strategy        = RecoveryStrategy::ROLLBACK;
        result.restore_snapshot = ctx.pre_merge_snapshot;
        result.reason           = "merge fault — rolling back to pre-merge adapter state";
    }

    if (emitter != nullptr) {
        const bool recoverable = (result.strategy == RecoveryStrategy::ROLLBACK);
        std::string kv = "adapter_id=" + ctx.pre_merge_snapshot.adapter_id
                       + " layers_merged=" + std::to_string(ctx.layers_merged_before_fault)
                       + "/" + std::to_string(ctx.total_layers)
                       + " partial=" + (result.partial_merge_detected ? "true" : "false");
        if (!ctx.context_kv.empty()) {
            kv += " " + ctx.context_kv;
        }
        emitter->emitAdapterIncident(
            ctx.fault_code,
            "AdapterMergeFailsafe",
            "handle",
            result.reason,
            recoverable,
            kv);
        result.incident_emitted = true;
    }

    return result;
}

inline EnrichmentGapSummary EnrichmentGapHandler::evaluate(
    const EnrichmentGapContext& ctx,
    TrainingIncidentEmitter*    emitter) const noexcept
{
    EnrichmentGapSummary summary;
    summary.total_items    = ctx.total_items;
    summary.enriched_items = ctx.enriched_items;

    if (ctx.total_items == 0) {
        summary.gap_items       = 0;
        summary.coverage_ratio  = 1.0;
        summary.meets_threshold = true;
        summary.gap_reason      = "no items to enrich";
        return summary;
    }

    summary.gap_items      = ctx.total_items - ctx.enriched_items;
    summary.coverage_ratio = static_cast<double>(ctx.enriched_items)
                           / static_cast<double>(ctx.total_items);
    summary.meets_threshold = (summary.coverage_ratio >= ctx.minimum_coverage_threshold);

    if (!summary.meets_threshold) {
        summary.gap_reason = "enrichment coverage below threshold ("
            + std::to_string(static_cast<int>(summary.coverage_ratio * 100))
            + "% < "
            + std::to_string(static_cast<int>(ctx.minimum_coverage_threshold * 100))
            + "%)";

        if (emitter != nullptr) {
            const std::string kv =
                "total=" + std::to_string(ctx.total_items)
                + " enriched=" + std::to_string(ctx.enriched_items)
                + " gap=" + std::to_string(summary.gap_items)
                + " coverage=" + std::to_string(summary.coverage_ratio)
                + " threshold=" + std::to_string(ctx.minimum_coverage_threshold)
                + (ctx.context_kv.empty() ? "" : " " + ctx.context_kv);

            emitter->emitDatasetIncident(
                TrainingErrorCode::ENRICHMENT_NO_ENRICHABLE_CONTENT,
                ctx.source_component.empty() ? "EnrichmentGapHandler" : ctx.source_component,
                ctx.operation.empty()        ? "evaluate"             : ctx.operation,
                summary.gap_reason,
                /*recoverable=*/true,
                kv);
            summary.incident_emitted = true;
        }
    }

    return summary;
}

} // namespace training
} // namespace themis
