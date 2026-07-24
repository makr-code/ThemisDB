/// @file tensor_delta_log.h
/// @brief Tensor delta log for recording mutations in exact graph state
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines the tensor delta log system for recording mutations that occur
/// in the exact graph state. These deltas are used by the snapshot-based update worker
/// to decide whether to patch, partially refit, or fully rebuild tensor artifacts.
///
/// ## Design Philosophy
///
/// The delta log is:
/// - **Non-blocking**: Recording deltas does not impede graph commit path
/// - **Immutable**: Deltas are write-once; history is immutable
/// - **Versioned**: Each delta references the commit transaction that created it
/// - **Windowed**: Deltas are consumed in fixed windows by the update worker
/// - **Deterministic**: Delta ordering and content are reproducible
///
/// ## Thread Safety
///
/// TensorDeltaLog supports concurrent reads and serialized writes:
/// - Writers must externally synchronize (e.g., single writer per artifact)
/// - Readers can safely iterate through deltas concurrently
/// - Extracting delta windows is atomic relative to append
///
/// ## Lifecycle
///
/// 1. DeltaLogger appends mutations (INSERT/UPDATE/DELETE/SHARD_CHANGE)
/// 2. Deltas are buffered in-memory with optional persistence to RocksDB
/// 3. SnapshotUpdateWorker extracts delta windows and processes them
/// 4. Processed deltas can be garbage-collected per retention policy
///

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <optional>

namespace themis {
namespace distributed_tensor {

// Forward declarations
struct ArtifactManifest;

/// @brief Enumeration of delta mutation types.
///
/// Describes the type of operation that triggered the delta entry.
enum class DeltaMutationType : uint8_t {
  /// INSERT: A new entity/relation was added to the graph.
  INSERT = 0,

  /// UPDATE: An existing entity/relation was modified.
  UPDATE = 1,

  /// DELETE: An entity/relation was removed from the graph.
  DELETE = 2,

  /// SHARD_CHANGE: Affected entity was moved to a different shard.
  /// Implication: Tensor artifact may need to recompute shard-local summaries.
  SHARD_CHANGE = 3,

  /// METADATA_UPDATE: Metadata-only change (no payload modification).
  /// Implication: Tensor artifact metadata may need refresh but payload unchanged.
  METADATA_UPDATE = 4,
};

/// @brief A single entry in the tensor delta log.
///
/// Immutable record of a mutation that occurred in the exact graph state.
struct DeltaLogEntry {
  /// Sequence number assigned at recording time (monotonically increasing).
  /// Used to detect missing or out-of-order deltas.
  uint64_t sequence_number = 0;

  /// The type of mutation (INSERT, UPDATE, DELETE, SHARD_CHANGE, etc.).
  DeltaMutationType mutation_type = DeltaMutationType::INSERT;

  /// ID of the entity/relation affected by this delta.
  /// Format is domain-specific (e.g., graph node ID, relation ID, shard ID).
  std::string affected_entity_id;

  /// Timestamp when this delta was recorded (milliseconds since epoch).
  int64_t recorded_at_ms = 0;

  /// Source transaction/commit ID that produced this delta.
  /// Allows tracing deltas back to originating transaction.
  std::string source_transaction_id;

  /// Optional partition hint for routing/shard-awareness.
  /// Populated when shard information is known at recording time.
  std::string shard_hint;

  /// Payload size in bytes (for bandwidth estimation).
  /// Helps update worker decide between patch vs full rebuild.
  uint32_t payload_size_bytes = 0;

  /// Optional payload checksum (for integrity verification).
  std::string payload_checksum;

  /// Checks if this entry is valid (has required fields).
  bool isValid() const;

  /// Serializes this entry to a compact string format.
  std::string serialize() const;

  /// Deserializes an entry from string format.
  /// @param serialized Serialized entry string
  /// @return Deserialized entry on success, std::nullopt on parse error
  static std::optional<DeltaLogEntry> deserialize(const std::string& serialized);
};

/// @brief A contiguous window of deltas extracted from the log.
///
/// Immutable snapshot of deltas within a specific sequence range.
/// Used by the update worker to analyze changes and decide update strategy.
struct DeltaWindow {
  /// Artifact ID this delta window applies to.
  std::string artifact_id;

  /// Sequence range: start (inclusive)
  uint64_t sequence_start = 0;

  /// Sequence range: end (inclusive)
  uint64_t sequence_end = 0;

  /// Deltas in this window (ordered by sequence_number).
  std::vector<DeltaLogEntry> entries;

  /// Total size of all delta payloads in this window (bytes).
  uint64_t total_payload_size_bytes = 0;

  /// Timestamp when this window was extracted (milliseconds since epoch).
  int64_t extracted_at_ms = 0;

  /// Returns number of INSERT mutations in window.
  size_t countInserts() const;

  /// Returns number of UPDATE mutations in window.
  size_t countUpdates() const;

  /// Returns number of DELETE mutations in window.
  size_t countDeletes() const;

  /// Returns number of SHARD_CHANGE mutations in window.
  size_t countShardChanges() const;

  /// Estimated change fraction: (total_payload_size / artifact_size)
  /// Used to decide patch vs partial_refit vs rebuild.
  /// @param artifact_size_bytes Total size of the artifact being updated
  /// @return Fraction in range [0.0, 1.0]
  double estimateChangeFraction(uint64_t artifact_size_bytes) const;

  /// Checks if window is valid (has entries and consistent sequence ordering).
  bool isValid() const;

  /// Serializes window to a compact newline-delimited string for persistence/transmission.
  /// @return Serialized string representation of this window
  std::string serialize() const;

  /// Deserializes window from the compact string format produced by serialize().
  /// @param data Serialized string produced by serialize()
  /// @return Deserialized window on success, std::nullopt on parse error
  static std::optional<DeltaWindow> deserialize(const std::string& data);

  /// Serializes window to a JSON string representation.
  /// @return JSON string containing all window fields and entries
  std::string toJSON() const;

  /// Deserializes window from a JSON string.
  /// @param json_str JSON string produced by toJSON()
  /// @return Deserialized window on success, std::nullopt on parse error
  static std::optional<DeltaWindow> fromJSON(const std::string& json_str);
};

/// @brief TensorDeltaLog: Records mutations in exact graph state for tensor artifact updates.
///
/// The delta log is the primary data structure for bridging the exact graph state
/// with derived tensor artifacts. It records all mutations and provides windowed
/// access to deltas for the update worker.
///
/// ## Invariants
///
/// - Sequence numbers are monotonically increasing
/// - Deltas within a sequence range are always ordered
/// - A delta is write-once; no modification or deletion after recording
/// - Gaps in sequence numbers indicate lost/dropped deltas (should not occur)
/// - All artifacts share a single global sequence namespace
///
/// ## Implementation Notes
///
/// - In-memory buffering uses a circular buffer with overflow handling
/// - Optional RocksDB persistence for durability
/// - Checkpoint system for crash recovery
/// - Garbage collection per retention policy
///
class TensorDeltaLog {
 public:
  /// Creates a new delta log for the specified artifact.
  /// @param artifact_id Unique artifact identifier
  explicit TensorDeltaLog(const std::string& artifact_id);

  virtual ~TensorDeltaLog() = default;

  // Prevent copy/move operations to maintain invariants
  TensorDeltaLog(const TensorDeltaLog&) = delete;
  TensorDeltaLog& operator=(const TensorDeltaLog&) = delete;
  TensorDeltaLog(TensorDeltaLog&&) = delete;
  TensorDeltaLog& operator=(TensorDeltaLog&&) = delete;

  /// Appends a delta entry to the log.
  ///
  /// Thread-safe for single writer (external synchronization required).
  /// Sequence number is assigned automatically.
  ///
  /// @param mutation_type Type of mutation (INSERT, UPDATE, DELETE, etc.)
  /// @param affected_entity_id ID of entity affected by the mutation
  /// @param source_transaction_id ID of the transaction that produced this delta
  /// @param shard_hint Optional shard hint for routing
  /// @param payload_size_bytes Size of the changed data in bytes
  /// @return The assigned sequence number on success, 0 on error
  uint64_t appendDelta(DeltaMutationType mutation_type,
                       const std::string& affected_entity_id,
                       const std::string& source_transaction_id,
                       const std::string& shard_hint = "",
                       uint32_t payload_size_bytes = 0);

  /// Extracts a delta window for the specified sequence range.
  ///
  /// Returns all deltas with sequence numbers in [sequence_start, sequence_end].
  /// The window is immutable and can be safely passed to other threads.
  ///
  /// @param sequence_start Start of sequence range (inclusive)
  /// @param sequence_end End of sequence range (inclusive)
  /// @return DeltaWindow containing all deltas in range, or std::nullopt if range is invalid
  std::optional<DeltaWindow> extractWindow(uint64_t sequence_start, uint64_t sequence_end);

  /// Returns the current sequence number (highest sequence assigned so far).
  uint64_t getCurrentSequence() const;

  /// Returns the artifact ID this log is tracking.
  const std::string& getArtifactId() const { return artifact_id_; }

  /// Returns total number of deltas in the log.
  size_t size() const;

  /// Checks if the log is empty.
  bool empty() const;

  /// Clears all deltas (for testing or reset scenarios).
  /// WARNING: This permanently discards all delta history.
  void clear();

  /// Returns approximate memory usage of the log (bytes).
  size_t getMemoryUsage() const;

  /// Persists all deltas to durable storage (RocksDB).
  /// @return true on success, false on error
  virtual bool persistToStorage() const;

  /// Loads deltas from durable storage for recovery after crash.
  /// @return Number of deltas loaded on success, -1 on error
  virtual int64_t loadFromStorage();

  /// Garbage-collects deltas older than the specified sequence.
  ///
  /// Removes all entries with sequence_number < cutoff_sequence.
  /// Returns the number of deltas removed.
  ///
  /// @param cutoff_sequence Sequence number threshold (entries < this are removed)
  /// @return Number of entries removed
  size_t garbage_collect(uint64_t cutoff_sequence);

  /// Sets retention policy for delta garbage collection.
  /// @param max_entries Keep at most this many recent entries
  /// @param max_age_ms Keep deltas no older than this (milliseconds)
  void setRetentionPolicy(size_t max_entries, int64_t max_age_ms);

  /// Returns delta statistics for metrics/observability.
  struct Stats {
    uint64_t total_deltas = 0;
    uint64_t total_insert_mutations = 0;
    uint64_t total_update_mutations = 0;
    uint64_t total_delete_mutations = 0;
    uint64_t total_shard_change_mutations = 0;
    uint64_t total_payload_bytes = 0;
    int64_t oldest_delta_ms = 0;
    int64_t newest_delta_ms = 0;
  };

  /// Returns current statistics about the delta log.
  Stats getStats() const;

 protected:
  std::string artifact_id_;
  mutable std::mutex entries_mutex_;
  std::vector<DeltaLogEntry> entries_;
  uint64_t current_sequence_ = 0;
  int64_t last_recorded_ms_ = 0;
  size_t max_entries_retention_ = 100000;
  int64_t max_age_ms_retention_ = 86400000;  // 24 hours default
};

}  // namespace distributed_tensor
}  // namespace themis
