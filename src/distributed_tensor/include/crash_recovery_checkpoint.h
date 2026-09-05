/// @file crash_recovery_checkpoint.h
/// @brief Crash recovery checkpoint mechanism for update worker
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines checkpoint persistence for the update worker,
/// enabling recovery from crashes during long-running artifact updates.
///
/// ## Design
///
/// Checkpoints capture:
/// - Current artifact being processed
/// - Delta window being analyzed
/// - Current manifest state
/// - Update decision taken
/// - Partial execution progress
///
/// Checkpoints are:
/// - Written to disk before starting long-running operations
/// - Atomic (all-or-nothing writes)
/// - Idempotent (recovery replay is safe)
/// - Versioned (for forward compatibility)

#pragma once

#include "artifact_manifest.h"
#include "src/distributed_tensor/include/tensor_delta_log.h"
#include <string>
#include <memory>
#include <cstdint>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @brief Checkpoint status codes.
enum class CheckpointStatus : uint8_t {
  /// Checkpoint operation succeeded.
  OK = 0,

  /// Checkpoint file not found.
  NOT_FOUND = 1,

  /// Checkpoint file is corrupted or invalid.
  CORRUPTED = 2,

  /// I/O error reading/writing checkpoint.
  IO_ERROR = 3,

  /// Checkpoint format version mismatch.
  VERSION_MISMATCH = 4,

  /// Insufficient disk space.
  NO_SPACE = 5,

  /// Unknown error.
  UNKNOWN_ERROR = 6,
};

/// @brief Checkpoint data structure.
struct Checkpoint {
  /// Checkpoint version for forward compatibility.
  uint32_t version = 1;

  /// Timestamp when checkpoint was created (seconds since epoch).
  int64_t created_at_unix_sec = 0;

  /// Artifact being processed.
  std::string artifact_id;

  /// Delta window being processed.
  DeltaWindow delta_window;

  /// Current manifest state at checkpoint time.
  ArtifactManifest current_manifest;

  /// Size of the artifact in bytes.
  uint64_t artifact_size_bytes = 0;

  /// Last update decision made.
  uint32_t last_decision = 0;  // UpdateDecision as uint32_t

  /// Progress indicator (0-100%).
  uint32_t progress_percent = 0;

  /// Optional error state from previous attempt.
  std::string last_error_message;

  /// Number of retry attempts so far.
  uint32_t retry_count = 0;

  /// Maximum retries allowed for this checkpoint.
  uint32_t max_retries = 3;
};

/// @brief Crash recovery checkpoint manager.
///
/// Manages checkpoint persistence and recovery for the update worker.
/// Enables safe recovery from crashes during long-running updates.
///
class CrashRecoveryCheckpoint {
 public:
  /// Creates a checkpoint manager with the given directory.
  /// @param checkpoint_dir Directory to store checkpoint files
  explicit CrashRecoveryCheckpoint(const std::string& checkpoint_dir);

  virtual ~CrashRecoveryCheckpoint() = default;

  // Prevent copy/move
  CrashRecoveryCheckpoint(const CrashRecoveryCheckpoint&) = delete;
  CrashRecoveryCheckpoint& operator=(const CrashRecoveryCheckpoint&) = delete;
  CrashRecoveryCheckpoint(CrashRecoveryCheckpoint&&) = delete;
  CrashRecoveryCheckpoint& operator=(CrashRecoveryCheckpoint&&) = delete;

  /// Saves a checkpoint to disk.
  /// @param artifact_id Artifact being processed
  /// @param checkpoint Checkpoint data to save
  /// @return OK on success, error code otherwise
  virtual CheckpointStatus save(const std::string& artifact_id, const Checkpoint& checkpoint);

  /// Loads a checkpoint from disk.
  /// @param artifact_id Artifact identifier
  /// @param[out] checkpoint Loaded checkpoint data
  /// @return OK on success, NOT_FOUND if no checkpoint exists, error code otherwise
  virtual CheckpointStatus load(const std::string& artifact_id, Checkpoint& checkpoint);

  /// Deletes a checkpoint file.
  /// @param artifact_id Artifact identifier
  /// @return OK on success, NOT_FOUND if checkpoint doesn't exist, error code otherwise
  virtual CheckpointStatus deleteCheckpoint(const std::string& artifact_id);

  /// Checks if a checkpoint exists for the given artifact.
  /// @param artifact_id Artifact identifier
  /// @return true if checkpoint exists, false otherwise
  virtual bool exists(const std::string& artifact_id);

  /// Gets checkpoint statistics.
  struct CheckpointStats {
    /// Number of checkpoints currently stored.
    uint64_t checkpoint_count = 0;

    /// Total size of all checkpoint files in bytes.
    uint64_t total_size_bytes = 0;

    /// Timestamp of the oldest checkpoint (seconds since epoch).
    int64_t oldest_checkpoint_unix_sec = 0;

    /// Timestamp of the newest checkpoint (seconds since epoch).
    int64_t newest_checkpoint_unix_sec = 0;
  };

  /// Gets statistics about stored checkpoints.
  virtual CheckpointStats getStats();

  /// Cleans up old checkpoints beyond retention policy.
  /// @param retention_days Retain checkpoints modified within this many days
  /// @return Number of checkpoints deleted
  virtual uint64_t cleanupOldCheckpoints(uint32_t retention_days);

  /// Sets the directory for checkpoint storage.
  /// @param checkpoint_dir Directory path
  void setCheckpointDir(const std::string& checkpoint_dir);

  /// Gets the current checkpoint directory.
  std::string getCheckpointDir() const;

 protected:
  std::string checkpoint_dir_;

  /// Internal helper to serialize checkpoint to string.
  std::string serializeCheckpoint(const Checkpoint& checkpoint);

  /// Internal helper to deserialize checkpoint from string.
  Checkpoint deserializeCheckpoint(const std::string& data);

  /// Internal helper to get checkpoint file path for artifact.
  std::string getCheckpointPath(const std::string& artifact_id);

  /// Internal helper to validate checkpoint format and version.
  CheckpointStatus validateCheckpoint(const Checkpoint& checkpoint);
};

}  // namespace distributed_tensor
}  // namespace themis
