/// @file distributed_lock_manager.h
/// @brief Distributed lock management for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines distributed locking for exclusive artifact access
/// during long-running update operations.
///
/// ## Design
///
/// Locks support:
/// - Exclusive (single writer) semantics
/// - Time-to-live (TTL) expiry for crash recovery
/// - Automatic lock renewal during long operations
/// - Release on operation completion
/// - Timeout detection and escalation
///
/// Lock states:
/// - UNLOCKED: Available for acquisition
/// - LOCKED: Held by a worker/operator
/// - EXPIRED: TTL exceeded, available for forceful acquisition
/// - HELD_WITH_RENEWAL: Being renewed by holder

#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <optional>
#include <map>
#include <mutex>

namespace themis {
namespace distributed_tensor {

enum class LockStatus : uint8_t {
  OK = 0,

  LOCKED = 1,

  TIMEOUT = 2,

  HOLDER_MISMATCH = 3,

  NOT_HELD = 4,

  EXPIRED = 5,

  STORAGE_ERROR = 6,

  UNKNOWN_ERROR = 7,
};

struct DistributedLock {
  std::string artifact_id;

  std::string holder_id;

  int64_t acquired_at_unix_sec = 0;

  int64_t ttl_seconds = 0;

  int64_t expires_at_unix_sec = 0;

  uint32_t renewal_count = 0;

  std::string lock_reason;
};

class DistributedLockManager {
 public:
  DistributedLockManager();

  /**
   * @brief Distributed Lock Manager.
   * @return Return value.
   */
  virtual ~DistributedLockManager() = default;

  // Prevent copy/move
  DistributedLockManager(const DistributedLockManager&) = delete;
  DistributedLockManager& operator=(const DistributedLockManager&) = delete;
  DistributedLockManager(DistributedLockManager&&) = delete;
  DistributedLockManager& operator=(DistributedLockManager&&) = delete;

  virtual LockStatus acquireLock(const std::string& artifact_id,
                                  const std::string& holder_id,
                                  int64_t ttl_seconds = 3600,
                                  const std::string& lock_reason = "");

  /**
   * @brief Release Lock.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] holder_id Identifier of the holder.
   * @return Return value.
   */
  virtual LockStatus releaseLock(const std::string& artifact_id, const std::string& holder_id);

  virtual LockStatus renewLock(const std::string& artifact_id,
                               const std::string& holder_id,
                               int64_t ttl_seconds = 3600);

  /**
   * @brief Is Locked.
   * @param[in] artifact_id Identifier of the artifact.
   * @return True when the operation succeeds.
   */
  virtual bool isLocked(const std::string& artifact_id);

  /**
   * @brief Get Lock Info.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  virtual std::optional<DistributedLock> getLockInfo(const std::string& artifact_id);

  virtual LockStatus forcefullyAcquireLock(const std::string& artifact_id,
                                           const std::string& holder_id,
                                           int64_t ttl_seconds = 3600);

  /**
   * @brief Cleanup Expired Locks.
   * @return Return value.
   */
  virtual uint64_t cleanupExpiredLocks();

  struct LockStats {
    uint64_t active_locks = 0;

    uint64_t total_acquisitions = 0;

    uint64_t contentions = 0;

    uint64_t expirations = 0;

    double average_hold_time_sec = 0.0;
  };

  /**
   * @brief Get Stats.
   * @return Return value.
   */
  virtual LockStats getStats() const;

  virtual void clearAllLocks(bool force = false);

 protected:
  // Thread-safe storage of locks
  mutable std::mutex locks_mutex_;
  std::map<std::string, DistributedLock> locks_;

  // Statistics
  LockStats stats_;

  /**
   * @brief Is Expired.
   * @param[in] lock Input parameter.
   * @return True when the operation succeeds.
   */
  bool isExpired(const DistributedLock& lock);

  /**
   * @brief Get Current Time Unix Sec.
   * @return Return value.
   */
  int64_t getCurrentTimeUnixSec();
};

}  // namespace distributed_tensor
}  // namespace themis
