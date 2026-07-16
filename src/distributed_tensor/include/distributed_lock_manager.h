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

/// @brief Lock status codes.
enum class LockStatus : uint8_t {
  /// Lock operation succeeded.
  OK = 0,

  /// Lock is already held by another holder.
  LOCKED = 1,

  /// Lock acquisition timed out.
  TIMEOUT = 2,

  /// Lock holder mismatch (trying to unlock with wrong holder ID).
  HOLDER_MISMATCH = 3,

  /// Lock does not exist or was never acquired.
  NOT_HELD = 4,

  /// Lock has expired due to TTL.
  EXPIRED = 5,

  /// I/O error accessing lock backend.
  STORAGE_ERROR = 6,

  /// Unknown error.
  UNKNOWN_ERROR = 7,
};

/// @brief Lock state for a single artifact.
struct DistributedLock {
  /// Artifact being locked.
  std::string artifact_id;

  /// Unique ID of the lock holder.
  std::string holder_id;

  /// Timestamp when lock was acquired (seconds since epoch).
  int64_t acquired_at_unix_sec = 0;

  /// Time-to-live for the lock in seconds (0 = no expiry).
  int64_t ttl_seconds = 0;

  /// Timestamp when lock will expire (seconds since epoch).
  int64_t expires_at_unix_sec = 0;

  /// Number of times lock has been renewed.
  uint32_t renewal_count = 0;

  /// Optional context/reason for lock (e.g., "partial_refit", "rebuild").
  std::string lock_reason;
};

/// @brief Distributed lock manager.
///
/// Manages distributed locks for tensor artifacts, supporting:
/// - Exclusive access control during updates
/// - TTL-based expiry for crash recovery
/// - Lock renewal for long-running operations
/// - Forceful acquisition when locks expire
///
class DistributedLockManager {
 public:
  /// Creates a lock manager.
  DistributedLockManager();

  virtual ~DistributedLockManager() = default;

  // Prevent copy/move
  DistributedLockManager(const DistributedLockManager&) = delete;
  DistributedLockManager& operator=(const DistributedLockManager&) = delete;
  DistributedLockManager(DistributedLockManager&&) = delete;
  DistributedLockManager& operator=(DistributedLockManager&&) = delete;

  /// Attempts to acquire a lock for an artifact.
  /// @param artifact_id Artifact to lock
  /// @param holder_id Unique ID of the lock holder (e.g., worker ID)
  /// @param ttl_seconds Time-to-live for the lock (0 = no expiry)
  /// @param lock_reason Optional reason for the lock
  /// @return OK on success, LOCKED if already held, TIMEOUT if acquisition times out
  virtual LockStatus acquireLock(const std::string& artifact_id,
                                  const std::string& holder_id,
                                  int64_t ttl_seconds = 3600,
                                  const std::string& lock_reason = "");

  /// Releases a held lock.
  /// @param artifact_id Artifact to unlock
  /// @param holder_id Holder ID to verify ownership
  /// @return OK on success, NOT_HELD if lock is not held, HOLDER_MISMATCH if holder differs
  virtual LockStatus releaseLock(const std::string& artifact_id, const std::string& holder_id);

  /// Renews an acquired lock (extends TTL).
  /// @param artifact_id Artifact whose lock to renew
  /// @param holder_id Holder ID to verify ownership
  /// @param ttl_seconds New time-to-live
  /// @return OK on success, NOT_HELD if not held, HOLDER_MISMATCH if holder differs
  virtual LockStatus renewLock(const std::string& artifact_id,
                               const std::string& holder_id,
                               int64_t ttl_seconds = 3600);

  /// Checks if a lock is currently held.
  /// @param artifact_id Artifact to check
  /// @return true if locked and not expired, false otherwise
  virtual bool isLocked(const std::string& artifact_id);

  /// Gets the current lock holder information.
  /// @param artifact_id Artifact to check
  /// @return Lock info if held, empty optional if not held
  virtual std::optional<DistributedLock> getLockInfo(const std::string& artifact_id);

  /// Forcefully acquires a lock even if expired.
  /// @param artifact_id Artifact to lock
  /// @param holder_id New holder ID
  /// @param ttl_seconds Time-to-live for the lock
  /// @return OK on success, LOCKED if held by non-expired lock, error otherwise
  virtual LockStatus forcefullyAcquireLock(const std::string& artifact_id,
                                           const std::string& holder_id,
                                           int64_t ttl_seconds = 3600);

  /// Checks for and removes expired locks.
  /// @return Number of locks that were expired and removed
  virtual uint64_t cleanupExpiredLocks();

  /// Gets lock statistics.
  struct LockStats {
    /// Number of currently held locks.
    uint64_t active_locks = 0;

    /// Number of locks that have been acquired (cumulative).
    uint64_t total_acquisitions = 0;

    /// Number of lock contentions (acquisition failed due to existing lock).
    uint64_t contentions = 0;

    /// Number of locks that expired.
    uint64_t expirations = 0;

    /// Average lock hold time in seconds.
    double average_hold_time_sec = 0.0;
  };

  /// Gets statistics about lock operations.
  virtual LockStats getStats() const;

  /// Clears all locks (for testing/admin purposes).
  /// @param force If true, forcefully clear all locks. If false, only clear expired ones.
  virtual void clearAllLocks(bool force = false);

 protected:
  // Thread-safe storage of locks
  mutable std::mutex locks_mutex_;
  std::map<std::string, DistributedLock> locks_;

  // Statistics
  LockStats stats_;

  /// Internal helper to check if a lock has expired.
  bool isExpired(const DistributedLock& lock);

  /// Internal helper to get current time in seconds since epoch.
  int64_t getCurrentTimeUnixSec();
};

}  // namespace distributed_tensor
}  // namespace themis
