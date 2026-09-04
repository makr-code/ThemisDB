// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file model_history_contract.h
 * @brief Incremental model evolution with audit trails, temporal queries, and point-in-time recovery.
 * @version 2.1.0-beta
 *
 * @section purpose Purpose
 * Provides time-travel semantics for process models: query historical states, compute deltas,
 * replay operations, and recover models to any point in their history. Enables compliance
 * auditing (who changed what model, when, why) and debugging of model state issues.
 *
 * @section architecture Architecture
 *
 * Model history is built from immutable append-only operation log:
 * 1. **Operation Log:** Sequence of {timestamp, actor, operation_type (import/link/delete), model_id, payload}
 * 2. **Snapshot Index:** Periodic full snapshots for fast point-in-time queries (every N operations)
 * 3. **Delta Encoding:** Efficient storage between snapshots (replaying deltas is faster than storing all snapshots)
 * 4. **Temporal Query:** Given timestamp, find nearest snapshot + replay deltas to reconstruct model state
 *
 * Storage efficiency: Delta encoding reduces storage by ≥30% vs. full snapshots.
 * Query latency: < 100 ms P95 for queries within 30-day window (with index).
 *
 * @section audit_trail Audit Trail Structure
 *
 * Each operation on a process model is recorded in the audit trail:
 * ```
 * AuditRecord {
 *     timestamp_ns: int64,          // UTC, nanoseconds since epoch
 *     actor: string,                // Principal ID (user/service)
 *     operation: string,            // "import" | "link" | "delete" | "merge"
 *     model_id: string,             // Which model was affected
 *     shard_id: string,             // Which shard the operation ran on
 *     model_state_hash: string,     // Hash of model after operation
 *     delta: string,                // Operation delta (serialized diff)
 *     conflict_resolved: bool,      // Whether conflict resolution was triggered
 *     trace_id: string,             // Correlation ID for distributed tracing
 * }
 * ```
 *
 * Immutable: Records appended to audit log; no retroactive deletion. Optional external sink:
 * SIEM, archive, or compliance system.
 *
 * @section delta_encoding Delta Encoding Strategy
 *
 * ### Operation-Log Deltas (Efficient)
 * - Store sequence of operations: {timestamp, operation_type, parameters}
 * - Example: "import(model_id='proc1', version_hash='abc123')"
 * - Replay: Iterate deltas from snapshot, apply each in order
 * - Size: ~100-500 bytes per operation (small)
 *
 * ### Snapshot-Based Encoding (Fast recovery)
 * - Store full model snapshot every N operations (default N=100)
 * - Between snapshots: store operation-log deltas
 * - Recovery time: O(1) snapshot load + O(N') delta replay (N' = operations since snapshot)
 * - Size: Hybrid (snapshots + deltas)
 *
 * ### Compression
 * - Identify common substructures across consecutive versions (e.g., unchanged BPMN nodes)
 * - Store differences only: "added nodes=[X, Y]", "removed edges=[a->b]"
 * - Compression ratio: ≥30% reduction vs. full snapshots
 *
 * @section temporal_queries Temporal Query Types
 *
 * | Query | Semantics | Latency | Use Case |
 * |-------|-----------|---------|----------|
 * | getHistoricalModel(id, ts) | Model state at exact timestamp | < 100ms P95 | "What did this model look like on 2026-08-01?" |
 * | getDelta(id, ts1, ts2) | Diff between two snapshots | < 100ms P95 | "What changed between these two times?" |
 * | getOperationLog(id, range) | Sequence of operations in time range | < 1s | "Show me all modifications to this model last week" |
 * | findRevisionAt(id, ts) | Nearest revision at/before timestamp | < 100ms P95 | "Which snapshot covers this timestamp?" |
 * | replay(id, base_ts, operations) | Apply operations to model at base_ts | Varies | "What would model look like if I replayed ops X, Y, Z?" |
 *
 * @section point_in_time_recovery Point-in-Time Recovery
 *
 * **Use Case:** Model corrupted or incorrect; recover to known-good state at timestamp T.
 *
 * **Procedure:**
 * 1. Call `recoverModel(model_id, target_timestamp)` → Returns model state at T
 * 2. Validate recovered model (schema check, cycle detection, etc.)
 * 3. If valid: overwrite current model with recovered version; emit audit record
 * 4. If invalid: query audit trail to understand what happened; manual remediation
 *
 * **Guarantees:**
 * - Deterministic: identical timestamp always produces identical model
 * - Atomic: recovery is all-or-nothing (no partial updates)
 * - Preserves immutability: original audit trail unchanged; recovery recorded as new operation
 *
 * @section retention_policy Retention Policy
 *
 * Audit trail can grow unbounded; configure retention:
 * - **Time-based:** Keep operations from last N days (e.g., 90 days)
 * - **Operation-based:** Keep last M operations per model (e.g., last 10,000)
 * - **Compliance holds:** Marks preventing deletion (for legal hold, GDPR right to explain)
 *
 * Old audit records can be archived to external storage; queries span live + archive.
 *
 * @section conflict_audit Conflict Resolution Audit
 *
 * When plugin resolves conflict or fallback LWW tiebreaker applies:
 * - Audit record marks: conflict_resolved=true, merge_metadata={ strategy, local_version, remote_version }
 * - Enables forensics: "Which conflicts occurred on this model? How were they resolved?"
 * - Determinism verification: replaying same conflict path must produce same result
 *
 * @section contract_freeze Contract Freeze
 * This contract is frozen for ThemisDB v2.1; breaking changes require v3.0.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis::process {

// ============================================================================
// Audit Trail Records
// ============================================================================

/**
 * @brief Single record in the immutable audit trail.
 *
 * Append-only log entry; no retroactive deletion.
 */
struct AuditRecord {
    /// UTC timestamp of operation (nanoseconds since epoch)
    int64_t timestamp_ns = 0;

    /// Principal (user/service) ID that performed the operation
    std::string actor;

    /// Operation type: "import" | "link" | "delete" | "merge" | "recover"
    std::string operation_type;

    /// Model ID affected by this operation
    std::string model_id;

    /// Shard ID where operation executed
    std::string shard_id;

    /// Cryptographic hash of model state after operation (for integrity)
    std::string model_state_hash;

    /// Serialized operation delta (diff representation)
    std::string delta;

    /// true if conflict resolution was involved
    bool conflict_resolved = false;

    /// Metadata about conflict resolution (if conflict_resolved=true)
    std::string merge_metadata;

    /// Correlation ID for distributed tracing
    std::string trace_id;

    /// Optional: legal hold flag (prevents deletion during retention window)
    bool legal_hold = false;

    /// Optional: GDPR/compliance marker (right to explain, data subject access)
    std::string compliance_marker;

    /**
     * @brief Serialize audit record to JSON.
     * @return JSON representation
     */
    nlohmann::json toJson() const {
        return {
            {"timestamp_ns", timestamp_ns},
            {"actor", actor},
            {"operation_type", operation_type},
            {"model_id", model_id},
            {"shard_id", shard_id},
            {"model_state_hash", model_state_hash},
            {"delta", delta},
            {"conflict_resolved", conflict_resolved},
            {"merge_metadata", merge_metadata},
            {"trace_id", trace_id},
            {"legal_hold", legal_hold},
            {"compliance_marker", compliance_marker},
        };
    }
};

// ============================================================================
// Delta Encoding
// ============================================================================

/**
 * @brief Representation of model change (delta) between two versions.
 *
 * Delta can be operation-log based or snapshot-based; both support compression.
 */
struct ModelDelta {
    /// Type of delta representation: "operation_log" | "snapshot_diff" | "compressed"
    std::string delta_type = {};

    /// Timestamp of base snapshot (starting point for replay)
    int64_t base_timestamp_ns = 0;

    /// Timestamp of target (ending point after replay)
    int64_t target_timestamp_ns = 0;

    /// Sequence of operations to apply (if delta_type == "operation_log")
    std::vector<AuditRecord> operations;

    /// Compressed delta representation (serialized diff; opaque to consensus layer)
    std::string compressed_delta;

    /// List of model node IDs that were added
    std::vector<std::string> added_nodes;

    /// List of model node IDs that were removed
    std::vector<std::string> removed_nodes;

    /// List of model edge descriptions that were added
    std::vector<std::string> added_edges;

    /// List of model edge descriptions that were removed
    std::vector<std::string> removed_edges;

    /// Compression ratio achieved (size_delta / size_full_snapshot)
    double compression_ratio = 0.0;

    /**
     * @brief Estimate size of delta in bytes.
     * @return Approximate byte size
     */
    size_t estimatedSize() const noexcept {
        size_t size = compressed_delta.size();
        for (const auto& op : operations) {
            size += op.delta.size();
        }
        return size;
    }
};

/**
 * @brief Snapshot of model state at a point in time (for efficient recovery).
 *
 * Periodic snapshots allow efficient point-in-time queries without replaying all history.
 */
struct HistorySnapshot {
    /// Timestamp when this snapshot was taken
    int64_t snapshot_timestamp_ns = 0;

    /// Revision/version number at this snapshot
    uint64_t revision = 0;

    /// Full serialized model content (BPMN/CMMN XML or JSON)
    std::string model_content = {};

    /// Cryptographic hash of model_content
    std::string model_state_hash;

    /// Index of this snapshot (e.g., "snapshot #10" if snapshots every 100 ops)
    uint32_t snapshot_index = 0;

    /// Sequence of delta records between this snapshot and the next
    std::vector<ModelDelta> deltas_to_next;

    /// true if this snapshot has been validated (schema, cycle checks)
    bool validated = false;
};

// ============================================================================
// Temporal Query Interfaces
// ============================================================================

/**
 * @brief Query for model state at a specific point in time.
 *
 * Used by: `getHistoricalModel(model_id, target_timestamp)`
 */
struct TemporalQuery {
    /// Model ID to query
    std::string model_id;

    /// Target timestamp (UTC, nanoseconds since epoch)
    int64_t target_timestamp_ns = 0;

    /// If true: find nearest snapshot at or before timestamp
    /// If false: fail if exact timestamp not found
    bool nearest_before = true;

    /// Maximum age of snapshot to accept (ms); 0 = no limit
    uint32_t max_age_ms = 0;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Result of temporal query.
 *
 * Contains model state at requested timestamp, plus metadata about recovery.
 */
struct TemporalQueryResult {
    /// Whether query succeeded
    bool success = false;

    /// Model state at target timestamp (if success=true)
    std::string model_content;

    /// Timestamp of the model returned (may differ from target if nearest_before=true)
    int64_t actual_timestamp_ns = 0;

    /// Revision/version number at this timestamp
    uint64_t revision = 0;

    /// Number of deltas replayed from base snapshot
    uint32_t deltas_replayed = 0;

    /// Time to reconstruct model (ms)
    uint32_t reconstruction_time_ms = 0;

    /// Reason for failure (if success=false)
    std::string error_message;

    /// Confidence in result (1.0 = exact match, <1.0 = reconstructed via deltas)
    double confidence = 1.0;
};

/**
 * @brief Query for operations performed on a model within a time range.
 *
 * Used by: `getOperationLog(model_id, start_timestamp, end_timestamp)`
 */
struct OperationLogQuery {
    /// Model ID to query
    std::string model_id;

    /// Start timestamp (inclusive; UTC, nanoseconds since epoch)
    int64_t start_timestamp_ns = 0;

    /// End timestamp (inclusive; UTC, nanoseconds since epoch)
    int64_t end_timestamp_ns = 0;

    /// Filter by actor (principal ID); empty = no filter
    std::string actor_filter;

    /// Filter by operation type (import|link|delete|merge); empty = no filter
    std::string operation_type_filter;

    /// Maximum number of records to return (0 = no limit)
    uint32_t limit = 0;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Result of operation log query.
 */
struct OperationLogQueryResult {
    /// Whether query succeeded
    bool success = false;

    /// Audit records matching query (in chronological order)
    std::vector<AuditRecord> records;

    /// Total number of matching records (may exceed len(records) if limited)
    uint32_t total_count = 0;

    /// Time to execute query (ms)
    uint32_t query_time_ms = 0;

    /// Reason for failure (if success=false)
    std::string error_message;
};

// ============================================================================
// Recovery Operations
// ============================================================================

/**
 * @brief Request to recover a model to a specific point in time.
 */
struct RecoverModelRequest {
    /// Model ID to recover
    std::string model_id;

    /// Target timestamp (UTC, nanoseconds since epoch)
    int64_t target_timestamp_ns = 0;

    /// Principal ID performing recovery (for audit trail)
    std::string actor;

    /// Reason for recovery (for audit trail)
    std::string reason;

    /// If true: validate recovered model before applying
    bool validate_before_apply = true;

    /// If true: create backup of current model before overwriting
    bool create_backup = true;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Result of recovery operation.
 */
struct RecoverModelResult {
    /// Whether recovery succeeded
    bool success = false;

    /// Model state at target timestamp (if success=true)
    std::string recovered_model_content;

    /// Timestamp of recovered model
    int64_t actual_timestamp_ns = 0;

    /// Backup model ID (if create_backup=true); used to restore if recovery was wrong
    std::optional<std::string> backup_model_id;

    /// Audit record ID created for this recovery operation
    std::string audit_record_id;

    /// Reason for failure (if success=false)
    std::string error_message;

    /// Time to recover (ms)
    uint32_t recovery_time_ms = 0;
};

// ============================================================================
// Retention and Compliance Policy
// ============================================================================

/**
 * @brief Retention policy for audit trail records.
 *
 * Governs how long records are kept before archival/deletion.
 */
struct AuditRetentionPolicy {
    /// Time-based retention: keep records from last N days (0 = no limit)
    uint32_t retention_days = 90;

    /// Operation-based retention: keep last M operations per model (0 = no limit)
    uint32_t operations_per_model = 10000;

    /// Archival target: external service for old records (e.g., SIEM, S3)
    std::string archive_endpoint;

    /// true to compress records before archival
    bool archive_with_compression = true;

    /// Minimum compliance hold period (days); records subject to hold cannot be deleted
    uint32_t min_legal_hold_days = 7;

    /// Grace period before actual deletion (days); soft-delete → hard-delete
    uint32_t soft_delete_grace_period_days = 30;
};

// ============================================================================
// Model History Configuration
// ============================================================================

/**
 * @brief Configuration for model history and evolution tracking.
 */
struct ModelHistoryConfig {
    /// true to enable model history tracking
    bool enable_history = true;

    /// Snapshot frequency: create snapshot every N operations
    uint32_t snapshot_frequency = 100;

    /// Delta encoding strategy: "operation_log" | "snapshot_diff" | "auto"
    std::string delta_encoding = "auto";

    /// Compression algorithm: "none" | "zstd" | "lz4"
    std::string compression_algorithm = "zstd";

    /// Audit trail retention policy
    AuditRetentionPolicy retention_policy;

    /// Maximum size of in-memory history cache (MB)
    uint32_t cache_size_mb = 256;

    /// true to record all queries in audit trail (verbose)
    bool audit_queries = false;

    /// Correlation ID for tracing history operations
    std::string trace_id;
};

} // namespace themis::process
