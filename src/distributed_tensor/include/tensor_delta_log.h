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

enum class DeltaMutationType : uint8_t {
  INSERT = 0,

  UPDATE = 1,

  DELETE = 2,

  SHARD_CHANGE = 3,

  METADATA_UPDATE = 4,
};

struct DeltaLogEntry {
  uint64_t sequence_number = 0;

  DeltaMutationType mutation_type = DeltaMutationType::INSERT;

  std::string affected_entity_id;

  int64_t recorded_at_ms = 0;

  std::string source_transaction_id;

  std::string shard_hint;

  uint32_t payload_size_bytes = 0;

  std::string payload_checksum;

  /**
   * @brief Is Valid.
   * @return True when the operation succeeds.
   */
  bool isValid() const;

  /**
   * @brief Serialize.
   * @return Return value.
   */
  std::string serialize() const;

  /**
   * @brief Deserialize.
   * @param[in] serialized Input parameter.
   * @return Return value.
   */
  static std::optional<DeltaLogEntry> deserialize(const std::string& serialized);
};

struct DeltaWindow {
  std::string artifact_id;

  uint64_t sequence_start = 0;

  uint64_t sequence_end = 0;

  std::vector<DeltaLogEntry> entries;

  uint64_t total_payload_size_bytes = 0;

  int64_t extracted_at_ms = 0;

  /**
   * @brief Count Inserts.
   * @return Return value.
   */
  size_t countInserts() const;

  /**
   * @brief Count Updates.
   * @return Return value.
   */
  size_t countUpdates() const;

  /**
   * @brief Count Deletes.
   * @return Return value.
   */
  size_t countDeletes() const;

  /**
   * @brief Count Shard Changes.
   * @return Return value.
   */
  size_t countShardChanges() const;

  /**
   * @brief Estimate Change Fraction.
   * @param[in] artifact_size_bytes Input parameter.
   * @return Return value.
   */
  double estimateChangeFraction(uint64_t artifact_size_bytes) const;

  /**
   * @brief Is Valid.
   * @return True when the operation succeeds.
   */
  bool isValid() const;

  /**
   * @brief Serialize.
   * @return Return value.
   */
  std::string serialize() const;

  /**
   * @brief Deserialize.
   * @param[in] data Input parameter.
   * @return Return value.
   */
  static std::optional<DeltaWindow> deserialize(const std::string& data);

  /**
   * @brief To JSON.
   * @return Return value.
   */
  std::string toJSON() const;

  /**
   * @brief From JSON.
   * @param[in] json_str Input parameter.
   * @return Return value.
   */
  static std::optional<DeltaWindow> fromJSON(const std::string& json_str);
};

class TensorDeltaLog {
 public:
  /**
   * @brief Tensor Delta Log.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  explicit TensorDeltaLog(const std::string& artifact_id);

  /**
   * @brief Tensor Delta Log.
   * @return Return value.
   */
  virtual ~TensorDeltaLog() = default;

  // Prevent copy/move operations to maintain invariants
  TensorDeltaLog(const TensorDeltaLog&) = delete;
  TensorDeltaLog& operator=(const TensorDeltaLog&) = delete;
  TensorDeltaLog(TensorDeltaLog&&) = delete;
  TensorDeltaLog& operator=(TensorDeltaLog&&) = delete;

  uint64_t appendDelta(DeltaMutationType mutation_type,
                       const std::string& affected_entity_id,
                       const std::string& source_transaction_id,
                       const std::string& shard_hint = "",
                       uint32_t payload_size_bytes = 0);

  /**
   * @brief Extract Window.
   * @param[in] sequence_start Input parameter.
   * @param[in] sequence_end Input parameter.
   * @return Return value.
   */
  std::optional<DeltaWindow> extractWindow(uint64_t sequence_start, uint64_t sequence_end);

  /**
   * @brief Get Current Sequence.
   * @return Return value.
   */
  uint64_t getCurrentSequence() const;

  const std::string& getArtifactId() const { return artifact_id_; }

  /**
   * @brief Size.
   * @return Return value.
   */
  size_t size() const;

  /**
   * @brief Empty.
   * @return True when the operation succeeds.
   */
  bool empty() const;

  /**
   * @brief Clear.
   */
  void clear();

  /**
   * @brief Get Memory Usage.
   * @return Return value.
   */
  size_t getMemoryUsage() const;

  /**
   * @brief Persist To Storage.
   * @return True when the operation succeeds.
   */
  virtual bool persistToStorage() const;

  /**
   * @brief Load From Storage.
   * @return Return value.
   */
  virtual int64_t loadFromStorage();

  /**
   * @brief Garbage collect.
   * @param[in] cutoff_sequence Input parameter.
   * @return Return value.
   */
  size_t garbage_collect(uint64_t cutoff_sequence);

  /**
   * @brief Set Retention Policy.
   * @param[in] max_entries Input parameter.
   * @param[in] max_age_ms Input parameter.
   */
  void setRetentionPolicy(size_t max_entries, int64_t max_age_ms);

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

  /**
   * @brief Get Stats.
   * @return Return value.
   */
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
