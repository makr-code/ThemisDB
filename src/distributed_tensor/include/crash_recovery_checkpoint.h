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
#include "tensor_delta_log.h"
#include <string>
#include <memory>
#include <cstdint>
#include <optional>

namespace themis {
namespace distributed_tensor {

enum class CheckpointStatus : uint8_t {
  OK = 0,

  NOT_FOUND = 1,

  CORRUPTED = 2,

  IO_ERROR = 3,

  VERSION_MISMATCH = 4,

  NO_SPACE = 5,

  UNKNOWN_ERROR = 6,
};

struct Checkpoint {
  uint32_t version = 1;

  int64_t created_at_unix_sec = 0;

  std::string artifact_id;

  DeltaWindow delta_window;

  ArtifactManifest current_manifest;

  uint64_t artifact_size_bytes = 0;

  uint32_t last_decision = 0;  // UpdateDecision as uint32_t

  uint32_t progress_percent = 0;

  std::string last_error_message;

  uint32_t retry_count = 0;

  uint32_t max_retries = 3;
};

class CrashRecoveryCheckpoint {
 public:
  /**
   * @brief Crash Recovery Checkpoint.
   * @param[in] checkpoint_dir Input parameter.
   * @return Return value.
   */
  explicit CrashRecoveryCheckpoint(const std::string& checkpoint_dir);

  /**
   * @brief Crash Recovery Checkpoint.
   * @return Return value.
   */
  virtual ~CrashRecoveryCheckpoint() = default;

  // Prevent copy/move
  CrashRecoveryCheckpoint(const CrashRecoveryCheckpoint&) = delete;
  CrashRecoveryCheckpoint& operator=(const CrashRecoveryCheckpoint&) = delete;
  CrashRecoveryCheckpoint(CrashRecoveryCheckpoint&&) = delete;
  CrashRecoveryCheckpoint& operator=(CrashRecoveryCheckpoint&&) = delete;

  /**
   * @brief Save.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] checkpoint Input parameter.
   * @return Return value.
   */
  virtual CheckpointStatus save(const std::string& artifact_id, const Checkpoint& checkpoint);

  /**
   * @brief Load.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in,out] checkpoint Input/output parameter.
   * @return Return value.
   */
  virtual CheckpointStatus load(const std::string& artifact_id, Checkpoint& checkpoint);

  /**
   * @brief Delete Checkpoint.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  virtual CheckpointStatus deleteCheckpoint(const std::string& artifact_id);

  /**
   * @brief Exists.
   * @param[in] artifact_id Identifier of the artifact.
   * @return True when the operation succeeds.
   */
  virtual bool exists(const std::string& artifact_id);

  struct CheckpointStats {
    uint64_t checkpoint_count = 0;

    uint64_t total_size_bytes = 0;

    int64_t oldest_checkpoint_unix_sec = 0;

    int64_t newest_checkpoint_unix_sec = 0;
  };

  /**
   * @brief Get Stats.
   * @return Return value.
   */
  virtual CheckpointStats getStats();

  /**
   * @brief Cleanup Old Checkpoints.
   * @param[in] retention_days Input parameter.
   * @return Return value.
   */
  virtual uint64_t cleanupOldCheckpoints(uint32_t retention_days);

  /**
   * @brief Set Checkpoint Dir.
   * @param[in] checkpoint_dir Input parameter.
   */
  void setCheckpointDir(const std::string& checkpoint_dir);

  /**
   * @brief Get Checkpoint Dir.
   * @return Return value.
   */
  std::string getCheckpointDir() const;

 protected:
  std::string checkpoint_dir_;

  /**
   * @brief Serialize Checkpoint.
   * @param[in] checkpoint Input parameter.
   * @return Return value.
   */
  std::string serializeCheckpoint(const Checkpoint& checkpoint);

  /**
   * @brief Deserialize Checkpoint.
   * @param[in] data Input parameter.
   * @return Return value.
   */
  Checkpoint deserializeCheckpoint(const std::string& data);

  /**
   * @brief Get Checkpoint Path.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  std::string getCheckpointPath(const std::string& artifact_id);

  /**
   * @brief Validate Checkpoint.
   * @param[in] checkpoint Input parameter.
   * @return Return value.
   */
  CheckpointStatus validateCheckpoint(const Checkpoint& checkpoint);
};

}  // namespace distributed_tensor
}  // namespace themis
