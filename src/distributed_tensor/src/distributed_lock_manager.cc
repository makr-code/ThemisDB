/// @file distributed_lock_manager.cc
/// @brief Implementation of distributed lock manager
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "../include/distributed_lock_manager.h"
#include <chrono>
#include <algorithm>

namespace themis {
namespace distributed_tensor {

DistributedLockManager::DistributedLockManager() {}

/**
 * @brief Get Current Time Unix Sec.
 * @return Return value.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count().
 */
int64_t DistributedLockManager::getCurrentTimeUnixSec() {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

/**
 * @brief Is Expired.
 * @param[in] lock Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: getCurrentTimeUnixSec().
 */
bool DistributedLockManager::isExpired(const DistributedLock& lock) {
  if (lock.ttl_seconds == 0) {
    return false;  // No expiry
  }

  int64_t now = getCurrentTimeUnixSec();
  return now >= lock.expires_at_unix_sec;
}

/**
 * @brief Acquire Lock.
 * @param[in] artifact_id Identifier of the artifact.
 * @param[in] holder_id Identifier of the holder.
 * @param[in] ttl_seconds Input parameter.
 * @param[in] lock_reason Input parameter.
 * @return Return value.
 * @details Calls: empty(), lock(), find(), end(), isExpired(), erase(), getCurrentTimeUnixSec().
 */
LockStatus DistributedLockManager::acquireLock(const std::string& artifact_id,
                                               const std::string& holder_id,
                                               int64_t ttl_seconds,
                                               const std::string& lock_reason) {
  if (artifact_id.empty() || holder_id.empty()) {
    return LockStatus::UNKNOWN_ERROR;
  }

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.find(artifact_id);
  if (it != locks_.end()) {
    // Check if existing lock is expired
    if (isExpired(it->second)) {
      // Lock has expired, we can take it over
      locks_.erase(it);
      stats_.expirations++;
    } else {
      // Lock is still held
      stats_.contentions++;
      return LockStatus::LOCKED;
    }
  }

  // Create new lock
  DistributedLock new_lock;
  new_lock.artifact_id = artifact_id;
  new_lock.holder_id = holder_id;
  new_lock.acquired_at_unix_sec = getCurrentTimeUnixSec();
  new_lock.ttl_seconds = ttl_seconds;
  new_lock.expires_at_unix_sec = new_lock.acquired_at_unix_sec + ttl_seconds;
  new_lock.lock_reason = lock_reason;
  new_lock.renewal_count = 0;

  locks_[artifact_id] = new_lock;
  stats_.active_locks++;
  stats_.total_acquisitions++;

  return LockStatus::OK;
}

/**
 * @brief Release Lock.
 * @param[in] artifact_id Identifier of the artifact.
 * @param[in] holder_id Identifier of the holder.
 * @return Return value.
 * @details Calls: empty(), lock(), find(), end(), getCurrentTimeUnixSec(), erase().
 */
LockStatus DistributedLockManager::releaseLock(const std::string& artifact_id,
                                               const std::string& holder_id) {
  if (artifact_id.empty() || holder_id.empty()) {
    return LockStatus::UNKNOWN_ERROR;
  }

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.find(artifact_id);
  if (it == locks_.end()) {
    return LockStatus::NOT_HELD;
  }

  if (it->second.holder_id != holder_id) {
    return LockStatus::HOLDER_MISMATCH;
  }

  // Calculate hold time for statistics
  int64_t hold_time_sec = getCurrentTimeUnixSec() - it->second.acquired_at_unix_sec;
  stats_.average_hold_time_sec =
      (stats_.average_hold_time_sec * (stats_.active_locks - 1) + hold_time_sec) / stats_.active_locks;

  locks_.erase(it);
  if (stats_.active_locks > 0) {
    stats_.active_locks--;
  }

  return LockStatus::OK;
}

/**
 * @brief Renew Lock.
 * @param[in] artifact_id Identifier of the artifact.
 * @param[in] holder_id Identifier of the holder.
 * @param[in] ttl_seconds Input parameter.
 * @return Return value.
 * @details Calls: empty(), lock(), find(), end(), getCurrentTimeUnixSec().
 */
LockStatus DistributedLockManager::renewLock(const std::string& artifact_id,
                                             const std::string& holder_id,
                                             int64_t ttl_seconds) {
  if (artifact_id.empty() || holder_id.empty()) {
    return LockStatus::UNKNOWN_ERROR;
  }

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.find(artifact_id);
  if (it == locks_.end()) {
    return LockStatus::NOT_HELD;
  }

  if (it->second.holder_id != holder_id) {
    return LockStatus::HOLDER_MISMATCH;
  }

  // Renew the lock
  int64_t now = getCurrentTimeUnixSec();
  it->second.ttl_seconds = ttl_seconds;
  it->second.expires_at_unix_sec = now + ttl_seconds;
  it->second.renewal_count++;

  return LockStatus::OK;
}

/**
 * @brief Is Locked.
 * @param[in] artifact_id Identifier of the artifact.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lock(), find(), end(), isExpired().
 */
bool DistributedLockManager::isLocked(const std::string& artifact_id) {
  if (artifact_id.empty()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.find(artifact_id);
  if (it == locks_.end()) {
    return false;
  }

  return !isExpired(it->second);
}

/**
 * @brief Get Lock Info.
 * @param[in] artifact_id Identifier of the artifact.
 * @return Return value.
 * @details Calls: empty(), lock(), find(), end(), isExpired().
 */
std::optional<DistributedLock> DistributedLockManager::getLockInfo(const std::string& artifact_id) {
  if (artifact_id.empty()) {
    return std::nullopt;
  }

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.find(artifact_id);
  if (it == locks_.end()) {
    return std::nullopt;
  }

  if (isExpired(it->second)) {
    return std::nullopt;
  }

  return it->second;
}

/**
 * @brief Forcefully Acquire Lock.
 * @param[in] artifact_id Identifier of the artifact.
 * @param[in] holder_id Identifier of the holder.
 * @param[in] ttl_seconds Input parameter.
 * @return Return value.
 * @details Calls: empty(), lock(), find(), end(), isExpired(), getCurrentTimeUnixSec().
 */
LockStatus DistributedLockManager::forcefullyAcquireLock(const std::string& artifact_id,
                                                        const std::string& holder_id,
                                                        int64_t ttl_seconds) {
  if (artifact_id.empty() || holder_id.empty()) {
    return LockStatus::UNKNOWN_ERROR;
  }

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.find(artifact_id);
  if (it != locks_.end() && !isExpired(it->second)) {
    // Lock is still held by someone else
    return LockStatus::LOCKED;
  }

  // Create new lock (forcefully overwriting if expired)
  DistributedLock new_lock;
  new_lock.artifact_id = artifact_id;
  new_lock.holder_id = holder_id;
  new_lock.acquired_at_unix_sec = getCurrentTimeUnixSec();
  new_lock.ttl_seconds = ttl_seconds;
  new_lock.expires_at_unix_sec = new_lock.acquired_at_unix_sec + ttl_seconds;
  new_lock.lock_reason = "forceful_acquisition";
  new_lock.renewal_count = 0;

  bool was_existing = (it != locks_.end());
  locks_[artifact_id] = new_lock;

  if (!was_existing) {
    stats_.active_locks++;
  }
  stats_.total_acquisitions++;

  return LockStatus::OK;
}

/**
 * @brief Cleanup Expired Locks.
 * @return Return value.
 * @details Calls: lock(), begin(), end(), isExpired(), erase().
 */
uint64_t DistributedLockManager::cleanupExpiredLocks() {
  uint64_t cleanup_count = 0;

  std::lock_guard<std::mutex> lock(locks_mutex_);

  auto it = locks_.begin();
  while (it != locks_.end()) {
    if (isExpired(it->second)) {
      it = locks_.erase(it);
      cleanup_count++;
      stats_.expirations++;
      if (stats_.active_locks > 0) {
        stats_.active_locks--;
      }
    } else {
      ++it;
    }
  }

  return cleanup_count;
}

DistributedLockManager::LockStats DistributedLockManager::getStats() const {
  /**
   * @brief Lock.
   * @param[in] locks_mutex_ Input parameter.
   * @return Return value.
   */
  std::lock_guard<std::mutex> lock(locks_mutex_);
  return stats_;
}

/**
 * @brief Clear All Locks.
 * @param[in] force Input parameter.
 * @details Calls: lock(), clear(), begin(), end(), isExpired(), erase().
 */
void DistributedLockManager::clearAllLocks(bool force) {
  std::lock_guard<std::mutex> lock(locks_mutex_);

  if (force) {
    locks_.clear();
    stats_.active_locks = 0;
  } else {
    // Only clear expired locks
    auto it = locks_.begin();
    while (it != locks_.end()) {
      if (isExpired(it->second)) {
        it = locks_.erase(it);
        if (stats_.active_locks > 0) {
          stats_.active_locks--;
        }
      } else {
        ++it;
      }
    }
  }
}

}  // namespace distributed_tensor
}  // namespace themis
