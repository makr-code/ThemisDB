/**
 * @file process_telemetry_contract.h
 * @brief Process telemetry contract for audit trail, temporal reconstruction, and diagnostics.
 *
 * Defines interfaces for immutable audit trails, delta encoding, temporal snapshots,
 * and point-in-time model recovery in federated Process Module deployments.
 *
 * @version 2.1.0
 * @date 2026-08-06
 * @status PHASE_1_DESIGN (Q1 2027)
 *
 * ## Overview
 *
 * Incremental evolution tracking provides:
 * - **Immutable Audit Trail:** Append-only log of all model changes
 * - **Delta Encoding:** RFC 7386 JSON Merge Patch for efficient storage
 * - **Temporal Snapshots:** O(log N) reconstruction of any historical model version
 * - **Storage Backends:** Pluggable backends (RocksDB, S3, file-based)
 *
 * ## Immutability Guarantee
 *
 * Once an entry is committed to the audit trail:
 * 1. Entry checksum validated (CRC32 chain per entry)
 * 2. No retroactive modifications possible
 * 3. Bulk verification on recovery ensures integrity
 * 4. Corrupted entries trigger incident (AUDIT_CORRUPTION)
 *
 * ## Delta Encoding
 *
 * Uses RFC 7386 JSON Merge Patch format:
 * - Minimal encoding overhead (typical 5-10% of full model size)
 * - Invertible: can compute forward and reverse deltas
 * - Semantic equivalence: replayed deltas produce identical state
 *
 * ## Temporal Query Support
 *
 * - **Point-in-Time Queries:** Retrieve model as of timestamp T
 * - **Delta Queries:** Get deltas between two timestamps
 * - **History Search:** Find model versions by creation time range
 *
 * @see process_federation_contract.h – Federated concurrency model
 * @see ROADMAP_FEDERATION.md – Phase 1-6 implementation plan
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_TELEMETRY_CONTRACT_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_TELEMETRY_CONTRACT_H

#include <string>
#include <cstdint>
#include <chrono>
#include <vector>
#include <memory>
#include <optional>

namespace themisdb::process {

// ============================================================================
// AUDIT TRAIL ENTRY
// ============================================================================

/**
 * @brief Immutable audit trail entry.
 *
 * Each entry represents one model mutation. Cannot be modified after creation.
 */
struct AuditTrailEntry {
  /// Unique entry ID (monotonically increasing)
  std::uint64_t entry_id = 0;

  /// Model ID that was mutated
  std::string model_id;

  /// Operation type (insert, update, delete)
  std::string operation_type;

  /// Delta encoding (RFC 7386 JSON Merge Patch format)
  std::string delta_patch;

  /// Before-mutation model hash (SHA-256)
  std::string before_hash;

  /// After-mutation model hash (SHA-256)
  std::string after_hash;

  /// CRC32 checksum for integrity verification
  std::uint32_t crc32_checksum = 0;

  /// Timestamp of mutation
  std::chrono::system_clock::time_point mutation_time;

  /// Source node ID (for auditing replicated mutations)
  std::string source_node_id;

  /// Model size before mutation (bytes)
  std::uint64_t before_size_bytes = 0;

  /// Model size after mutation (bytes)
  std::uint64_t after_size_bytes = 0;

  /// Additional metadata as JSON
  std::string metadata_json;

  /// Is entry persisted to durable storage?
  bool is_persisted = false;
};

// ============================================================================
// DELTA ENCODING & SNAPSHOTS
// ============================================================================

/**
 * @brief Delta encoding format.
 */
enum class DeltaEncodingFormat : std::uint8_t {
  /// RFC 7386 JSON Merge Patch (recommended)
  kJsonMergePatch = 0,

  /// Binary diff format (reserved for future)
  kBinaryDiff = 1,
};

/**
 * @brief Snapshot index for O(log N) temporal reconstruction.
 *
 * Maps timestamp ranges to snapshot locations for efficient retrieval.
 */
struct SnapshotIndex {
  /// Base model ID
  std::string model_id;

  /// Snapshot creation timestamp
  std::chrono::system_clock::time_point snapshot_time;

  /// Snapshot size (bytes)
  std::uint64_t snapshot_size_bytes = 0;

  /// Starting entry ID in audit trail this snapshot covers
  std::uint64_t start_entry_id = 0;

  /// Ending entry ID in audit trail this snapshot covers
  std::uint64_t end_entry_id = 0;

  /// Storage location (backend-specific path/key)
  std::string storage_location;

  /// Snapshot integrity check (SHA-256)
  std::string integrity_hash;
};

// ============================================================================
// TEMPORAL QUERY SUPPORT
// ============================================================================

/**
 * @brief Temporal query result for point-in-time model reconstruction.
 */
struct TemporalQueryResult {
  /// Reconstructed model (JSON string)
  std::string model_json;

  /// Reconstruction timestamp (requested time)
  std::chrono::system_clock::time_point query_time;

  /// Actual entry timestamp (may differ slightly from query_time)
  std::chrono::system_clock::time_point actual_time;

  /// Number of deltas applied during reconstruction
  std::uint32_t deltas_applied = 0;

  /// Reconstruction time (milliseconds)
  std::uint64_t reconstruction_ms = 0;

  /// Is reconstruction result accurate (or approximated)?
  bool is_accurate = true;
};

// ============================================================================
// STORAGE BACKENDS
// ============================================================================

/**
 * @brief Storage backend enumeration.
 */
enum class AuditStorageBackend : std::uint8_t {
  /// RocksDB backend (primary; local SSD)
  kRocksDB = 0,

  /// File-based backend (local filesystem)
  kFile = 1,

  /// S3 backend (cloud storage)
  kS3 = 2,

  /// Dual-write backend (RocksDB + S3)
  kDualWrite = 3,
};

// ============================================================================
// AUDIT LOGGER INTERFACE (Phase 2 Implementation)
// ============================================================================

/**
 * @brief Audit trail logger interface.
 *
 * @note Actual implementation in src/process/process_audit_logger.cpp
 *
 * Single writer (consensus leader), multiple readers (followers, clients).
 * Append-only; no retroactive modification.
 */
class ProcessAuditLogger {
 public:
  /**
   * @brief Append entry to immutable audit trail.
   *
   * @param entry Audit trail entry (will be assigned entry_id)
   * @return Assigned entry ID (monotonically increasing)
   *
   * @note Single writer (leader); followers call through leader
   * @note Blocks until CRC32 checksum computed and entry persisted
   * @note Cannot fail once committed (fail-closed on backend failure)
   */
  virtual std::uint64_t AppendEntry(AuditTrailEntry& entry) = 0;

  /**
   * @brief Retrieve entry from audit trail.
   *
   * @param entry_id Entry ID to retrieve
   * @return Optional entry (empty if entry_id >= next_entry_id)
   *
   * @note Multiple concurrent readers allowed
   * @note Returned entry is immutable; modifications create new entries
   */
  virtual std::optional<AuditTrailEntry> GetEntry(std::uint64_t entry_id) const = 0;

  /**
   * @brief Retrieve entries in range.
   *
   * @param start_id Starting entry ID (inclusive)
   * @param end_id Ending entry ID (inclusive)
   * @return Vector of entries in range
   *
   * @note Multiple concurrent readers allowed
   */
  virtual std::vector<AuditTrailEntry> GetEntriesInRange(
      std::uint64_t start_id, std::uint64_t end_id) const = 0;

  /**
   * @brief Reconstruct model as of given timestamp.
   *
   * @param model_id Model ID to reconstruct
   * @param timestamp Target timestamp for reconstruction
   * @return Temporal query result (reconstructed model, deltas applied)
   *
   * @note O(log N) snapshot lookup; O(D) delta application (D = deltas since snapshot)
   * @note Multiple concurrent queries allowed
   */
  virtual TemporalQueryResult ReconstructAsOfTime(
      const std::string& model_id,
      std::chrono::system_clock::time_point timestamp) const = 0;

  /**
   * @brief Verify audit trail integrity (all CRC32 checksums).
   *
   * @return Integrity check result (all_valid, corrupted_entries_count)
   *
   * @note Bulk verification on recovery; detects offline corruption
   */
  virtual bool VerifyIntegrity() const = 0;

  /**
   * @brief Get current next entry ID.
   *
   * @return Entry ID to be assigned to next append
   */
  virtual std::uint64_t GetNextEntryId() const = 0;

  virtual ~ProcessAuditLogger() = default;
};

// ============================================================================
// DOCUMENTATION SECTIONS (Design-Phase Specifications)
// ============================================================================

/**
 * @section immutability_guarantee Immutability Guarantee
 *
 * Once an entry is committed:
 * 1. **Checksum Chain:** Entry I contains hash(Entry I-1, data I)
 * 2. **Retroactive Detection:** Changing any entry invalidates all downstream checksums
 * 3. **Bulk Verification:** On recovery, scan all entries; any checksum mismatch → AUDIT_CORRUPTION incident
 * 4. **No Direct Modification:** No API to modify existing entries; only append allowed
 *
 * @section delta_composition Delta Composition Property
 *
 * Composed deltas maintain semantic equivalence:
 * ```
 * model_at(T1) ⊕ delta(T1→T2) ⊕ delta(T2→T3) = model_at(T3)
 * ```
 *
 * Where ⊕ = RFC 7386 JSON Merge Patch application.
 *
 * @section temporal_queries Temporal Query Examples
 *
 * - **Point-in-time:** "Show me model M as it was at 2027-02-15 14:00:00"
 * - **Temporal range:** "Get all versions of model M created between 2027-02-01 and 2027-02-28"
 * - **Delta sequence:** "Show me all changes to model M since 2027-02-14"
 *
 * @section storage_backend_options Storage Backend Options
 *
 * | Backend | Latency | Capacity | Cost | Use Case |
 * |---------|---------|----------|------|----------|
 * | RocksDB | <1ms | Local SSD | Low | Primary (leader) |
 * | File | <5ms | Local filesystem | Low | Development |
 * | S3 | 50-200ms | Unlimited | Medium | Archival |
 * | Dual-Write | <1ms leader, 50-200ms S3 | Unlimited | High | HA with archival |
 *
 * @section performance_expectations Performance Expectations
 *
 * | Operation | P95 | P99 | Budget |
 * |-----------|-----|-----|--------|
 * | Append entry (batched) | <1ms | <5ms | ≤1ms per op |
 * | Get entry by ID | <1ms | <5ms | ≤5ms |
 * | Reconstruct (1K deltas) | 100ms | 500ms | ≤200ms |
 * | Integrity verification (10K entries) | 50ms | 200ms | ≤100ms |
 * | Snapshot transfer (100MB) | 500ms | 1s | ≤1s |
 */

}  // namespace themisdb::process

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_TELEMETRY_CONTRACT_H
