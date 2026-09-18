/**
 * @file lock_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <functional>

namespace themis {

enum class LockType {
    SHARED,           ///< Read lock – many transactions can hold simultaneously
    EXCLUSIVE,        ///< Write lock – only one holder allowed
    INTENT_SHARED,    ///< Signals intent to acquire SHARED lock on a sub-resource
    INTENT_EXCLUSIVE  ///< Signals intent to acquire EXCLUSIVE lock on a sub-resource
};

enum class TwoPLPhase {
    GROWING,  ///< Transaction is in growing phase – may acquire, must not release
    SHRINKING ///< Transaction is in shrinking phase – may release, must not acquire
};

enum class LockStatus {
    GRANTED,  ///< Lock granted immediately
    TIMEOUT,  ///< Lock wait exceeded timeout
    DENIED    ///< Lock denied (2PL violation, or incompatible held by same txn)
};

class LockManager {
public:
    using TransactionId = uint64_t;

    static constexpr std::chrono::milliseconds DEFAULT_LOCK_TIMEOUT{5000};
    static constexpr size_t DEFAULT_ESCALATION_THRESHOLD = 100;

    struct LockResult {
        LockStatus status{LockStatus::GRANTED};
        std::string message;

        /**
         * @brief Granted.
         * @return Return value.
         * @details Implements Granted without additional internal calls.
         */
        static LockResult Granted() { return {LockStatus::GRANTED, ""}; }
        /**
         * @brief Timeout.
         * @return Return value.
         * @details Implements Timeout without additional internal calls.
         */
        static LockResult Timeout() { return {LockStatus::TIMEOUT, "lock wait timeout"}; }
        /**
         * @brief Denied.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static LockResult Denied(std::string msg) { return {LockStatus::DENIED, std::move(msg)}; }
    };

    struct LockStats {
        uint64_t total_acquired{0};
        uint64_t total_released{0};
        uint64_t total_timeouts{0};
        uint64_t total_escalations{0};
        uint64_t current_held{0};
        uint64_t current_waiting{0};
    };

    LockManager();
    ~LockManager() = default;

    LockManager(const LockManager&) = delete;
    LockManager& operator=(const LockManager&) = delete;

    LockResult acquireLock(
        TransactionId txn_id,
        const std::string& key,
        LockType type,
        std::chrono::milliseconds timeout = DEFAULT_LOCK_TIMEOUT);

    /**
     * @brief Release Lock.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    bool releaseLock(TransactionId txn_id, const std::string& key);

    /**
     * @brief Release All Locks.
     * @param[in] txn_id Identifier of the txn.
     */
    void releaseAllLocks(TransactionId txn_id);

    LockResult upgradeLock(
        TransactionId txn_id,
        const std::string& key,
        std::chrono::milliseconds timeout = DEFAULT_LOCK_TIMEOUT);

    /**
     * @brief Holds Lock.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] key Input parameter.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool holdsLock(TransactionId txn_id, const std::string& key, LockType type) const;

    std::vector<std::pair<std::string, LockType>> getLocksHeld(TransactionId txn_id) const;

    /**
     * @brief Begin Shrinking Phase.
     * @param[in] txn_id Identifier of the txn.
     */
    void beginShrinkingPhase(TransactionId txn_id);

    /**
     * @brief Is In Shrinking Phase.
     * @param[in] txn_id Identifier of the txn.
     * @return True when the operation succeeds.
     */
    bool isInShrinkingPhase(TransactionId txn_id) const;

    /**
     * @brief Set Escalation Threshold.
     * @param[in] threshold Input parameter.
     */
    void setEscalationThreshold(size_t threshold);

    /**
     * @brief Set Default Timeout.
     * @param[in] timeout Input parameter.
     */
    void setDefaultTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    LockStats getStats() const;

    /**
     * @brief Get Waiters.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::vector<TransactionId> getWaiters(const std::string& key) const;

    /**
     * @brief Get Waiting For.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    std::vector<std::string> getWaitingFor(TransactionId txn_id) const;


    /**
     * @brief Acquire Predicate Lock.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] start_key Input parameter.
     * @param[in] end_key Input parameter.
     * @return True when the operation succeeds.
     */
    bool acquirePredicateLock(TransactionId txn_id,
                              const std::string& start_key,
                              const std::string& end_key);

    /**
     * @brief Set Max Predicate Locks.
     * @param[in] max_locks Input parameter.
     */
    void setMaxPredicateLocks(size_t max_locks);

    /**
     * @brief Get Max Predicate Locks.
     * @return Return value.
     */
    size_t getMaxPredicateLocks() const;

    /**
     * @brief Set Predicate Locking Enabled.
     * @param[in] enabled Input parameter.
     */
    void setPredicateLockingEnabled(bool enabled);

    /**
     * @brief Is Predicate Locking Enabled.
     * @return True when the operation succeeds.
     */
    bool isPredicateLockingEnabled() const;

    /**
     * @brief Release Predicate Locks.
     * @param[in] txn_id Identifier of the txn.
     */
    void releasePredicateLocks(TransactionId txn_id);

    /**
     * @brief Check Predicate Conflict.
     * @param[in] writing_txn_id Identifier of the writing txn.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    TransactionId checkPredicateConflict(TransactionId writing_txn_id,
                                         const std::string& key) const;

    /**
     * @brief Get Predicate Lock Count.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    size_t getPredicateLockCount(TransactionId txn_id) const;

    std::vector<std::pair<std::string, std::string>> getPredicateLockRanges(
        TransactionId txn_id) const;

private:
    struct LockEntry {
        TransactionId holder;
        LockType      type;
        std::chrono::system_clock::time_point acquired_at;
    };

    struct LockRequest {
        TransactionId txn_id;
        LockType      type;
        bool          granted{false};
        // Each waiter has its own CV so it can be woken individually.
        std::condition_variable cv;

        LockRequest(TransactionId t, LockType lt) : txn_id(t), type(lt) {}
    };

    struct LockTableEntry {
        std::vector<LockEntry>                          holders;
        std::list<std::shared_ptr<LockRequest>>         waiters;
    };

    /**
     * @brief Compatible.
     * @param[in] held Input parameter.
     * @param[in] requested Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool compatible(LockType held, LockType requested) noexcept;

    /**
     * @brief Try Grant Lock.
     * @param[in] key Input parameter.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool tryGrantLock(const std::string& key, TransactionId txn_id, LockType type);

    /**
     * @brief Process Waiters.
     * @param[in] key Input parameter.
     */
    void processWaiters(const std::string& key);

    /**
     * @brief Check Escalation.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] key Input parameter.
     */
    void checkEscalation(TransactionId txn_id, const std::string& key);

    mutable std::mutex mutex_;

    std::unordered_map<std::string, LockTableEntry> lock_table_;

    std::unordered_map<TransactionId,
                       std::unordered_map<std::string, LockType>> held_by_txn_;

    std::unordered_set<TransactionId> shrinking_txns_;

    std::unordered_map<TransactionId, std::string> waiting_for_;

    std::atomic<size_t>   escalation_threshold_{DEFAULT_ESCALATION_THRESHOLD};
    std::atomic<uint64_t> default_timeout_ms_{5000};

    // Statistics
    std::atomic<uint64_t> stats_acquired_{0};
    std::atomic<uint64_t> stats_released_{0};
    std::atomic<uint64_t> stats_timeouts_{0};
    std::atomic<uint64_t> stats_escalations_{0};
    std::atomic<uint64_t> stats_waiting_{0};
    std::atomic<uint64_t> stats_deadlocks_{0};

    // ── Predicate locks for SSI ───────────────────────────────────────────────

    struct PredicateLock {
        TransactionId txn_id;
        std::string   start_key; ///< lower bound (inclusive)
        std::string   end_key;   ///< upper bound (inclusive)
    };

    std::vector<PredicateLock> predicate_locks_;

    std::atomic<size_t> max_predicate_locks_{0};

    std::atomic<bool> predicate_locking_enabled_{true};

    std::atomic<uint64_t> predicate_lock_drops_{0};

public:
    uint64_t predicateLockDropCount() const noexcept {
        return predicate_lock_drops_.load(std::memory_order_relaxed);
    }

private:

}; // class LockManager

} // namespace themis
