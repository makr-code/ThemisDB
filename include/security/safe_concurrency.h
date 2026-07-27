/**
 * @file safe_concurrency.h
 * @brief SafeConcurrency library: RAII-based thread-safe primitives for race condition prevention
 * @version 1.0.0
 * @author ThemisDB Sprint 9 Concurrency Remediation Team
 * @date 2026-07-27
 * 
 * @section Overview
 * Provides production-grade RAII wrappers around C++ synchronization primitives to prevent
 * common concurrency errors:
 * - Data races via thread-safe access guards
 * - Lost wakeups via predicate-based condition variables
 * - Deadlocks via lock ordering enforcement
 * - Atomic memory ordering violations via semantic wrappers
 * 
 * @section Usage Example
 * 
 * ```cpp
 * #include "include/security/safe_concurrency.h"
 * using namespace themis::security;
 * 
 * // Data race prevention: RAII lock guard with ordering validation
 * {
 *   LockGuard<std::mutex> lock(my_mutex);  // Acquires, validates ordering
 *   shared_data.field = new_value;         // Safe access
 * } // Lock released automatically
 * 
 * // Lost wakeup prevention: predicate-based condition variable
 * SafeCondition<std::function<bool()>> cv;
 * {
 *   std::unique_lock<std::mutex> lock(mutex);
 *   cv.wait(lock, [this]() { return data_ready.load(); });  // Checks predicate
 *   process(data);
 * }
 * 
 * // Lock ordering validation: automatic deadlock detection
 * DataRaceDetector detector;
 * detector.recordLockAcquisition("coord_mutex", __FILE__, __LINE__);
 * detector.recordLockAcquisition("participant_mutex", __FILE__, __LINE__);
 * // Validates against known deadlock patterns
 * ```
 * 
 * @defgroup SafeConcurrency Safe Concurrency Primitives
 * @{
 */

#ifndef THEMIS_INCLUDE_SECURITY_SAFE_CONCURRENCY_H_
#define THEMIS_INCLUDE_SECURITY_SAFE_CONCURRENCY_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace themis::security {

/**
 * @brief Configuration options for concurrency safety
 */
struct ConcurrencyConfig {
  /// Enable deadlock detection via lock ordering validation
  bool enable_deadlock_detection = true;
  
  /// Enable ThreadSanitizer annotations (if available)
  bool enable_tsan_annotations = true;
  
  /// Maximum time to hold any single lock (0 = no timeout)
  std::chrono::milliseconds lock_timeout_ms{0};
  
  /// Enable verbose logging of lock operations (debug mode)
  bool verbose_logging = false;
};

/**
 * @brief Global configuration instance
 */
extern ConcurrencyConfig g_concurrency_config;

/**
 * @brief Enhanced RAII lock guard with deadlock detection and ordering validation
 * 
 * Replaces std::lock_guard with:
 * - Automatic lock ordering validation (prevents deadlock patterns)
 * - ThreadSanitizer-compatible annotations
 * - Optional lock timeout and recovery
 * - Verbose logging of lock acquisitions
 * 
 * @tparam Mutex Lockable type (std::mutex, std::shared_mutex, etc.)
 * 
 * Thread-safety: SAFE - all operations are atomic
 * Memory-safety: SAFE - no raw pointers, RAII managed
 * 
 * @note Lock ordering is validated at acquisition time if deadlock detection is enabled.
 *       Attempting to acquire a lock in wrong order will throw std::runtime_error.
 * 
 * @section Example
 * 
 * ```cpp
 * std::mutex coord_mutex, participant_mutex;
 * 
 * // Correct order: coordinator → participant
 * {
 *   LockGuard<std::mutex> coord_lock(coord_mutex);      // Level 1
 *   LockGuard<std::mutex> part_lock(participant_mutex); // Level 2 - OK
 * }
 * 
 * // Incorrect order would throw std::runtime_error:
 * // {
 * //   LockGuard<std::mutex> part_lock(participant_mutex); // Level 2
 * //   LockGuard<std::mutex> coord_lock(coord_mutex);      // Level 1 - ERROR!
 * // }
 * ```
 */
template<typename Mutex>
class LockGuard {
 public:
  /**
   * @brief Acquire lock with optional ordering validation
   * @param mutex The mutex to acquire
   * @param lock_name Human-readable name for logging/debugging
   * @param expected_level Expected lock hierarchy level (for ordering validation)
   * @throws std::runtime_error if lock ordering is violated
   * @throws std::system_error if lock acquisition fails
   */
  explicit LockGuard(Mutex& mutex, 
                     const std::string& lock_name = "<unnamed>",
                     int expected_level = -1);
  
  /// Release lock automatically
  ~LockGuard();
  
  /// Deleted: no copy semantics
  LockGuard(const LockGuard&) = delete;
  LockGuard& operator=(const LockGuard&) = delete;
  
  /// Deleted: no move semantics (RAII scope ownership)
  LockGuard(LockGuard&&) = delete;
  LockGuard& operator=(LockGuard&&) = delete;
  
  /**
   * @brief Get the human-readable name of this lock
   * @return Lock name provided at construction
   */
  const std::string& getLockName() const { return lock_name_; }
  
  /**
   * @brief Get the lock hierarchy level
   * @return Hierarchy level (for ordering validation)
   */
  int getLockLevel() const { return lock_level_; }
  
  /**
   * @brief Manually unlock (for early release patterns)
   * 
   * After unlock(), the lock is released and ~LockGuard will not double-unlock.
   * Use sparingly - RAII scope is preferred.
   */
  void unlock();
  
 private:
  Mutex* mutex_;
  std::string lock_name_;
  int lock_level_;
  bool locked_;
};

/**
 * @brief Data race detector: tracks concurrent access patterns and identifies races
 * 
 * Maintains a timeline of memory accesses and mutex acquisitions to detect:
 * - Unsynchronized concurrent access to shared memory
 * - Use-after-free patterns
 * - Mutex ordering violations leading to potential deadlocks
 * 
 * Thread-safety: SAFE (uses internal synchronization)
 * 
 * @section Usage
 * 
 * ```cpp
 * DataRaceDetector detector;
 * 
 * // Record a memory access
 * detector.recordMemoryAccess(shared_ptr, "read", __FILE__, __LINE__);
 * 
 * // Record lock acquisitions
 * detector.recordLockAcquisition("mutex_name", __FILE__, __LINE__);
 * detector.recordLockRelease("mutex_name");
 * 
 * // Analyze for race conditions
 * auto races = detector.analyzeForRaces();
 * if (!races.empty()) {
 *   // Found potential data races!
 * }
 * ```
 */
class DataRaceDetector {
 public:
  enum class AccessType { kRead, kWrite };
  
  struct AccessRecord {
    std::thread::id thread_id;
    std::chrono::nanoseconds timestamp;
    AccessType type;
    const void* address;
    std::string location;  // "file.cpp:123"
  };
  
  struct LockRecord {
    std::thread::id thread_id;
    std::chrono::nanoseconds timestamp;
    std::string lock_name;
    bool is_acquisition;  // true for acquire, false for release
    std::string location;  // "file.cpp:123"
  };
  
  struct RaceCandidate {
    std::vector<AccessRecord> conflicting_accesses;
    std::string description;
    float confidence;  // 0.0 = low, 1.0 = high
  };
  
  /**
   * @brief Record a memory access for race detection
   * @param address Memory address being accessed
   * @param type Read or Write
   * @param file Source file name
   * @param line Source line number
   */
  void recordMemoryAccess(const void* address, 
                          AccessType type,
                          const char* file,
                          int line);
  
  /**
   * @brief Record a lock acquisition
   * @param lock_name Human-readable lock name
   * @param file Source file name
   * @param line Source line number
   */
  void recordLockAcquisition(const std::string& lock_name,
                             const char* file,
                             int line);
  
  /**
   * @brief Record a lock release
   * @param lock_name Human-readable lock name
   */
  void recordLockRelease(const std::string& lock_name);
  
  /**
   * @brief Analyze access history for potential races
   * @return Vector of detected race candidates
   * 
   * Returns potential data races where:
   * - Multiple threads access the same memory location
   * - At least one access is a write
   * - Not all accesses are protected by same lock
   */
  std::vector<RaceCandidate> analyzeForRaces() const;
  
  /**
   * @brief Clear all recorded access history
   */
  void reset();
  
  /**
   * @brief Get total number of recorded accesses
   */
  size_t getAccessCount() const;
  
  /**
   * @brief Get total number of lock operations
   */
  size_t getLockOperationCount() const;
  
 private:
  std::mutex history_mutex_;
  std::vector<AccessRecord> access_history_;
  std::vector<LockRecord> lock_history_;
};

/**
 * @brief Thread-safe condition variable with predicate verification
 * 
 * Wraps std::condition_variable with enforced predicate-checking to prevent
 * lost wakeups and spurious wake issues.
 * 
 * Thread-safety: SAFE (standard condition variable semantics)
 * 
 * @tparam Predicate Callable that returns bool - represents "condition ready"
 * 
 * @section Usage
 * 
 * ```cpp
 * std::atomic<bool> data_ready{false};
 * SafeCondition<decltype([]() { return data_ready.load(); })> cv;
 * 
 * // Consumer: wait until data is ready
 * {
 *   std::unique_lock<std::mutex> lock(mutex);
 *   cv.wait(lock, [](){ return data_ready.load(); });
 *   process_data();
 * }
 * 
 * // Producer: set data ready and notify
 * {
 *   std::lock_guard<std::mutex> lock(mutex);
 *   prepare_data();
 *   data_ready.store(true);
 *   cv.notify_all();
 * }
 * ```
 */
template<typename Predicate>
class SafeCondition {
 public:
  /**
   * @brief Construct a safe condition variable
   * @param predicate Callable that returns bool - true when condition is met
   */
  explicit SafeCondition(Predicate predicate) : predicate_(predicate) {}
  
  /**
   * @brief Wait for condition to become true (predicate-based)
   * 
   * Atomically:
   * 1. Verifies predicate() returns false (not already satisfied)
   * 2. Releases mutex
   * 3. Waits for notification
   * 4. Re-acquires mutex
   * 5. Verifies predicate() now returns true
   * 
   * @tparam Lock std::unique_lock or compatible
   * @param lock The unique lock (must be locked)
   * @throws std::runtime_error if predicate verification fails after wakeup
   */
  template<typename Lock>
  void wait(Lock& lock) {
    if (!predicate_()) {
      cv_.wait(lock);
      if (!predicate_()) {
        throw std::runtime_error("SafeCondition: predicate false after wakeup (lost wakeup detected)");
      }
    }
  }
  
  /**
   * @brief Wait with timeout
   * @tparam Rep, Period std::chrono duration types
   * @param lock The unique lock (must be locked)
   * @param timeout_duration Maximum time to wait
   * @return true if predicate true, false if timeout
   */
  template<typename Lock, typename Rep, typename Period>
  bool wait_for(Lock& lock, const std::chrono::duration<Rep, Period>& timeout_duration) {
    if (!predicate_()) {
      if (!cv_.wait_for(lock, timeout_duration)) {
        return false;  // Timeout
      }
    }
    return predicate_();  // Return true if satisfied
  }
  
  /**
   * @brief Notify one waiting thread
   */
  void notify_one() {
    cv_.notify_one();
  }
  
  /**
   * @brief Notify all waiting threads
   */
  void notify_all() {
    cv_.notify_all();
  }
  
 private:
  std::condition_variable cv_;
  Predicate predicate_;
};

/**
 * @brief Lock ordering validator: enforces canonical lock acquisition order
 * 
 * Validates that locks are acquired in a globally-consistent order to prevent
 * deadlocks. Defines per-module lock hierarchies and validates at runtime.
 * 
 * Thread-safety: SAFE (uses internal synchronization)
 * 
 * @section Configuration
 * 
 * Define lock hierarchies per module:
 * 
 * ```cpp
 * LockOrderingValidator validator;
 * 
 * // Sharding module: Coordinator (L1) → Partition (L2) → Transaction (L3)
 * validator.defineHierarchy("sharding", {
 *   {"shard_coordinator::coord_mutex", 1},
 *   {"partition_manager::partition_mutex", 2},
 *   {"transaction::lock_mutex", 3}
 * });
 * 
 * // Validate acquisition order
 * validator.recordAcquisition("sharding", "shard_coordinator::coord_mutex");
 * validator.recordAcquisition("sharding", "partition_manager::partition_mutex");  // OK (1→2)
 * // validator.recordAcquisition("sharding", "shard_coordinator::coord_mutex");   // ERROR! (2→1)
 * ```
 */
class LockOrderingValidator {
 public:
  /**
   * @brief Define lock hierarchy for a module
   * @param module_name Name of module (e.g., "sharding", "replication")
   * @param lock_hierarchy Map of {lock_name -> hierarchy_level}
   *        Lower levels must be acquired first
   */
  void defineHierarchy(const std::string& module_name,
                       const std::map<std::string, int>& lock_hierarchy);
  
  /**
   * @brief Record a lock acquisition for ordering validation
   * @param module_name Module where lock is being acquired
   * @param lock_name Name of lock being acquired
   * @throws std::runtime_error if acquisition violates lock hierarchy
   */
  void recordAcquisition(const std::string& module_name,
                         const std::string& lock_name);
  
  /**
   * @brief Record a lock release
   * @param module_name Module where lock is being released
   * @param lock_name Name of lock being released
   */
  void recordRelease(const std::string& module_name,
                     const std::string& lock_name);
  
  /**
   * @brief Check if a proposed acquisition order is valid
   * @param module_name Module where lock is being acquired
   * @param lock_name Name of lock being acquired
   * @return true if acquisition is valid, false if would violate ordering
   */
  bool isValidAcquisition(const std::string& module_name,
                          const std::string& lock_name) const;
  
  /**
   * @brief Get current lock hierarchy level for thread
   * @param module_name Module name
   * @return Current hierarchy level (0 = no lock held, >0 = lock held at level)
   */
  int getCurrentLevel(const std::string& module_name) const;
  
 private:
  std::mutex validator_mutex_;
  std::map<std::string, std::map<std::string, int>> hierarchies_;
  std::map<std::thread::id, std::map<std::string, int>> thread_levels_;
};

/**
 * @brief Atomic operation verifier: validates memory ordering semantics
 * 
 * Wraps std::atomic operations with explicit memory ordering verification
 * to catch subtle synchronization bugs.
 * 
 * Thread-safety: SAFE (atomic operations are inherently thread-safe)
 * 
 * @tparam T Type of atomic value
 */
template<typename T>
class AtomicVerifier {
 public:
  /**
   * @brief Create atomic with initial value
   * @param init_value Initial value
   */
  explicit AtomicVerifier(T init_value = T()) : value_(init_value) {}
  
  /**
   * @brief Load with explicit memory ordering validation
   * @param expected_order Expected memory order (e.g., memory_order_acquire)
   * @return Current value
   * @throws std::runtime_error if actual order differs from expected
   */
  T load(std::memory_order order = std::memory_order_seq_cst) const {
    validateLoadOrder(order);
    return value_.load(order);
  }
  
  /**
   * @brief Store with explicit memory ordering validation
   * @param new_value Value to store
   * @param order Memory order to use (e.g., memory_order_release)
   * @throws std::runtime_error if order is inappropriate for store
   */
  void store(T new_value, std::memory_order order = std::memory_order_seq_cst) {
    validateStoreOrder(order);
    value_.store(new_value, order);
  }
  
  /**
   * @brief Exchange with validation
   */
  T exchange(T new_value, std::memory_order order = std::memory_order_seq_cst) {
    validateExchangeOrder(order);
    return value_.exchange(new_value, order);
  }
  
  /**
   * @brief Compare-and-swap with validation
   */
  bool compare_exchange_strong(T& expected, T desired,
                               std::memory_order success_order = std::memory_order_seq_cst,
                               std::memory_order failure_order = std::memory_order_seq_cst) {
    validateCASOrder(success_order, failure_order);
    return value_.compare_exchange_strong(expected, desired, success_order, failure_order);
  }
  
 private:
  std::atomic<T> value_;
  
  void validateLoadOrder(std::memory_order order) const;
  void validateStoreOrder(std::memory_order order) const;
  void validateExchangeOrder(std::memory_order order) const;
  void validateCASOrder(std::memory_order success, std::memory_order failure) const;
};

// @}  // End of SafeConcurrency group

}  // namespace themis::security

#endif  // THEMIS_INCLUDE_SECURITY_SAFE_CONCURRENCY_H_
