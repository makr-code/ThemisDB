/**
 * @file lock_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/lock_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <stdexcept>

namespace themis {

// ---------------------------------------------------------------------------
// Compatibility matrix for lock types
// ---------------------------------------------------------------------------
// Compatibility table (held × requested):
//   IS  IX  S   X
// IS  Y   Y   Y   N
// IX  Y   Y   N   N
// S   Y   N   Y   N
// X   N   N   N   N
bool LockManager::compatible(LockType held, LockType requested) noexcept {
    if (held == LockType::EXCLUSIVE) return false;

    if (held == LockType::SHARED) {
        return requested == LockType::SHARED ||
               requested == LockType::INTENT_SHARED;
    }

    if (held == LockType::INTENT_SHARED) {
        return requested != LockType::EXCLUSIVE;
    }

    // INTENT_EXCLUSIVE held
    return requested == LockType::INTENT_SHARED ||
           requested == LockType::INTENT_EXCLUSIVE;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
LockManager::LockManager() = default;

// ---------------------------------------------------------------------------
// acquireLock
// ---------------------------------------------------------------------------
LockManager::LockResult LockManager::acquireLock(
    TransactionId txn_id,
    const std::string& key,
    LockType type,
    std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lk(mutex_);

    // 2PL enforcement: no new acquisitions in shrinking phase
    if (shrinking_txns_.count(txn_id)) {
        return LockResult::Denied(
            "2PL violation: transaction is in shrinking phase and cannot acquire new locks");
    }

    // If the transaction already holds this key at the same or stronger mode,
    // grant immediately (re-entrant).
    auto txn_it = held_by_txn_.find(txn_id);
    if (txn_it != held_by_txn_.end()) {
        auto key_it = txn_it->second.find(key);
        if (key_it != txn_it->second.end()) {
            // Already holds a lock on this key
            LockType held = key_it->second;
            if (held == type || held == LockType::EXCLUSIVE) {
                // Re-entrant grant
                return LockResult::Granted();
            }
            // Upgrade path (SHARED → EXCLUSIVE) handled via upgradeLock
        }
    }

    // Try immediate grant
    if (tryGrantLock(key, txn_id, type)) {
        checkEscalation(txn_id, key);
        stats_acquired_.fetch_add(1, std::memory_order_relaxed);
        return LockResult::Granted();
    }

    // Must wait – enqueue request
    auto req = std::make_shared<LockRequest>(txn_id, type);
    lock_table_[key].waiters.push_back(req);
    waiting_for_[txn_id] = key;
    stats_waiting_.fetch_add(1, std::memory_order_relaxed);

    THEMIS_DEBUG("LockManager: txn {} waiting for {} lock on '{}'",
                 txn_id,
                 type == LockType::EXCLUSIVE ? "X" :
                 type == LockType::SHARED    ? "S" :
                 type == LockType::INTENT_SHARED ? "IS" : "IX",
                 key);

    bool granted = req->cv.wait_for(lk, timeout, [&req] { return req->granted; });

    waiting_for_.erase(txn_id);
    stats_waiting_.fetch_sub(1, std::memory_order_relaxed);

    // Remove from waiters list - use find() to avoid re-hashing lock_table_
    auto lt_it_waiter = lock_table_.find(key);
    if (lt_it_waiter != lock_table_.end()) {
        lt_it_waiter->second.waiters.remove(req);
    }

    if (!granted) {
        stats_timeouts_.fetch_add(1, std::memory_order_relaxed);
        THEMIS_WARN("LockManager: txn {} timed out waiting for lock on '{}'", txn_id, key);
        return LockResult::Timeout();
    }

    checkEscalation(txn_id, key);
    stats_acquired_.fetch_add(1, std::memory_order_relaxed);
    THEMIS_DEBUG("LockManager: txn {} acquired lock on '{}'", txn_id, key);
    return LockResult::Granted();
}

// ---------------------------------------------------------------------------
// releaseLock
// ---------------------------------------------------------------------------
bool LockManager::releaseLock(TransactionId txn_id, const std::string& key) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto txn_it = held_by_txn_.find(txn_id);
    if (txn_it == held_by_txn_.end()) return false;

    auto key_it = txn_it->second.find(key);
    if (key_it == txn_it->second.end()) return false;

    // Remove from per-transaction map
    txn_it->second.erase(key_it);
    if (txn_it->second.empty()) {
        held_by_txn_.erase(txn_it);
    }

    // Remove from lock table
    auto lt_it = lock_table_.find(key);
    if (lt_it != lock_table_.end()) {
        auto& holders = lt_it->second.holders;
        holders.erase(
            std::remove_if(holders.begin(), holders.end(),
                [txn_id](const LockEntry& e) { return e.holder == txn_id; }),
            holders.end());

        processWaiters(key);

        if (lt_it->second.holders.empty() && lt_it->second.waiters.empty()) {
            lock_table_.erase(lt_it);
        }
    }

    stats_released_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

// ---------------------------------------------------------------------------
// releaseAllLocks
// ---------------------------------------------------------------------------
void LockManager::releaseAllLocks(TransactionId txn_id) {
    std::lock_guard<std::mutex> lk(mutex_);

    auto txn_it = held_by_txn_.find(txn_id);
    if (txn_it == held_by_txn_.end()) {
        // Also clean up waiting state if present
        shrinking_txns_.erase(txn_id);
        waiting_for_.erase(txn_id);
        return;
    }

    // Collect keys before modification
    std::vector<std::string> keys;
    keys.reserve(txn_it->second.size());
    for (const auto& [k, _] : txn_it->second) {
        keys.push_back(k);
    }

    for (const auto& key : keys) {
        auto lt_it = lock_table_.find(key);
        if (lt_it != lock_table_.end()) {
            auto& holders = lt_it->second.holders;
            holders.erase(
                std::remove_if(holders.begin(), holders.end(),
                    [txn_id](const LockEntry& e) { return e.holder == txn_id; }),
                holders.end());

            processWaiters(key);

            if (lt_it->second.holders.empty() && lt_it->second.waiters.empty()) {
                lock_table_.erase(lt_it);
            }
        }
        stats_released_.fetch_add(1, std::memory_order_relaxed);
    }

    held_by_txn_.erase(txn_id);
    shrinking_txns_.erase(txn_id);
    waiting_for_.erase(txn_id);

    THEMIS_DEBUG("LockManager: released all {} locks for txn {}", keys.size(), txn_id);
}

// ---------------------------------------------------------------------------
// upgradeLock  (SHARED → EXCLUSIVE)
// ---------------------------------------------------------------------------
LockManager::LockResult LockManager::upgradeLock(
    TransactionId txn_id,
    const std::string& key,
    std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lk(mutex_);

    auto txn_it = held_by_txn_.find(txn_id);
    if (txn_it == held_by_txn_.end()) {
        return LockResult::Denied("upgradeLock: transaction holds no locks");
    }

    auto key_it = txn_it->second.find(key);
    if (key_it == txn_it->second.end()) {
        return LockResult::Denied("upgradeLock: no lock held on key");
    }

    if (key_it->second == LockType::EXCLUSIVE) {
        return LockResult::Granted(); // Already exclusive
    }

    if (key_it->second != LockType::SHARED) {
        return LockResult::Denied("upgradeLock: can only upgrade from SHARED");
    }

    // Check if upgrade is immediately possible (we are the only holder)
    auto& entry = lock_table_[key];
    bool only_holder = (entry.holders.size() == 1 &&
                        entry.holders[0].holder == txn_id);

    if (!only_holder) {
        // Must wait for other holders to release
        auto req = std::make_shared<LockRequest>(txn_id, LockType::EXCLUSIVE);
        entry.waiters.push_front(req); // Priority: upgrade at front
        waiting_for_[txn_id] = key;
        stats_waiting_.fetch_add(1, std::memory_order_relaxed);

        bool granted = req->cv.wait_for(lk, timeout, [&req] { return req->granted; });

        waiting_for_.erase(txn_id);
        stats_waiting_.fetch_sub(1, std::memory_order_relaxed);
        entry.waiters.remove(req);

        if (!granted) {
            stats_timeouts_.fetch_add(1, std::memory_order_relaxed);
            return LockResult::Timeout();
        }
    } else {
        // Upgrade immediately
        entry.holders[0].type = LockType::EXCLUSIVE;
    }

    key_it->second = LockType::EXCLUSIVE;
    THEMIS_DEBUG("LockManager: txn {} upgraded lock on '{}' to EXCLUSIVE", txn_id, key);
    return LockResult::Granted();
}

// ---------------------------------------------------------------------------
// holdsLock
// ---------------------------------------------------------------------------
bool LockManager::holdsLock(
    TransactionId txn_id,
    const std::string& key,
    LockType type) const
{
    std::lock_guard<std::mutex> lk(mutex_);

    auto txn_it = held_by_txn_.find(txn_id);
    if (txn_it == held_by_txn_.end()) return false;

    auto key_it = txn_it->second.find(key);
    if (key_it == txn_it->second.end()) return false;

    LockType held = key_it->second;
    // EXCLUSIVE implies SHARED/IS/IX
    if (held == LockType::EXCLUSIVE) return true;
    return held == type;
}

// ---------------------------------------------------------------------------
// getLocksHeld
// ---------------------------------------------------------------------------
std::vector<std::pair<std::string, LockType>>
LockManager::getLocksHeld(TransactionId txn_id) const {
    std::lock_guard<std::mutex> lk(mutex_);

    std::vector<std::pair<std::string, LockType>> result;
    auto it = held_by_txn_.find(txn_id);
    if (it == held_by_txn_.end()) return result;

    result.reserve(it->second.size());
    for (const auto& [k, lt] : it->second) {
        result.emplace_back(k, lt);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Two-Phase Locking phase management
// ---------------------------------------------------------------------------
void LockManager::beginShrinkingPhase(TransactionId txn_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    shrinking_txns_.insert(txn_id);
    THEMIS_DEBUG("LockManager: txn {} entered shrinking phase", txn_id);
}

bool LockManager::isInShrinkingPhase(TransactionId txn_id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return shrinking_txns_.count(txn_id) > 0;
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
void LockManager::setEscalationThreshold(size_t threshold) {
    escalation_threshold_.store(threshold, std::memory_order_relaxed);
}

void LockManager::setDefaultTimeout(std::chrono::milliseconds timeout) {
    default_timeout_ms_.store(
        static_cast<uint64_t>(timeout.count()),
        std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------
LockManager::LockStats LockManager::getStats() const {
    LockStats s;
    s.total_acquired   = stats_acquired_.load(std::memory_order_relaxed);
    s.total_released   = stats_released_.load(std::memory_order_relaxed);
    s.total_timeouts   = stats_timeouts_.load(std::memory_order_relaxed);
    s.total_escalations = stats_escalations_.load(std::memory_order_relaxed);
    s.current_waiting  = stats_waiting_.load(std::memory_order_relaxed);

    std::lock_guard<std::mutex> lk(mutex_);
    uint64_t held = 0;
    for (const auto& [txn, keys] : held_by_txn_) {
        held += keys.size();
    }
    s.current_held = held;
    return s;
}

// ---------------------------------------------------------------------------
// Deadlock-detector helpers
// ---------------------------------------------------------------------------
std::vector<LockManager::TransactionId>
LockManager::getWaiters(const std::string& key) const {
    std::lock_guard<std::mutex> lk(mutex_);

    std::vector<TransactionId> result;
    auto it = lock_table_.find(key);
    if (it == lock_table_.end()) return result;

    for (const auto& req : it->second.waiters) {
        result.push_back(req->txn_id);
    }
    return result;
}

std::vector<std::string>
LockManager::getWaitingFor(TransactionId txn_id) const {
    std::lock_guard<std::mutex> lk(mutex_);

    std::vector<std::string> result;
    auto it = waiting_for_.find(txn_id);
    if (it != waiting_for_.end()) {
        result.push_back(it->second);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
bool LockManager::tryGrantLock(
    const std::string& key,
    TransactionId txn_id,
    LockType type)
{
    // mutex_ must be held
    auto& entry = lock_table_[key];

    for (const auto& holder : entry.holders) {
        if (holder.holder == txn_id) continue; // Same transaction
        if (!compatible(holder.type, type)) {
            return false;
        }
    }

    // Grant
    entry.holders.push_back({txn_id, type, std::chrono::system_clock::now()});
    held_by_txn_[txn_id][key] = type;
    return true;
}

void LockManager::processWaiters(const std::string& key) {
    // mutex_ must be held
    auto lt_it = lock_table_.find(key);
    if (lt_it == lock_table_.end()) return;

    auto& entry = lt_it->second;

    for (auto& req : entry.waiters) {
        if (req->granted) continue;

        // Check compatibility with all current holders
        bool ok = true;
        for (const auto& holder : entry.holders) {
            if (holder.holder == req->txn_id) continue;
            if (!compatible(holder.type, req->type)) {
                ok = false;
                break;
            }
        }

        if (ok) {
            entry.holders.push_back(
                {req->txn_id, req->type, std::chrono::system_clock::now()});
            held_by_txn_[req->txn_id][key] = req->type;
            req->granted = true;
            req->cv.notify_one();
        }
    }
}

void LockManager::checkEscalation(TransactionId txn_id, const std::string& key) {
    // mutex_ must be held.
    // Escalation: when a transaction holds more than `escalation_threshold_` row-level
    // locks on the same table, release them and acquire a single INTENT_EXCLUSIVE on the
    // table instead.  This reduces lock-table memory pressure for bulk operations.
    //
    // Key naming convention: keys are expected to follow "table_name:row_identifier"
    // format (colon-separated). The table prefix is "table_name:". Keys that do not
    // contain a colon are not eligible for escalation.
    auto txn_it = held_by_txn_.find(txn_id);
    if (txn_it == held_by_txn_.end()) return;

    // Cache threshold to avoid repeated atomic loads
    const size_t threshold = escalation_threshold_.load(std::memory_order_relaxed);
    if (txn_it->second.size() < threshold) {
        return;
    }

    // Extract table prefix (e.g. "table:pk" → "table:")
    auto colon_pos = key.find(':');
    if (colon_pos == std::string::npos) return;

    std::string table_prefix = key.substr(0, colon_pos + 1);

    // Count row-level locks on this table
    std::vector<std::string> row_keys;
    for (const auto& [k, _] : txn_it->second) {
        if (k.find(table_prefix) == 0 && k.size() > table_prefix.size()) {
            row_keys.push_back(k);
        }
    }

    if (row_keys.size() < threshold) {
        return;
    }

    // Acquire INTENT_EXCLUSIVE on the table key first (before releasing row locks)
    std::string table_key = table_prefix + "*";
    bool table_lock_ok = tryGrantLock(table_key, txn_id, LockType::INTENT_EXCLUSIVE);

    if (!table_lock_ok) {
        // Another transaction holds an incompatible lock on the table – skip escalation
        return;
    }

    // Release individual row locks and remove them from the lock table
    for (const auto& rk : row_keys) {
        auto lt_it = lock_table_.find(rk);
        if (lt_it != lock_table_.end()) {
            auto& holders = lt_it->second.holders;
            holders.erase(
                std::remove_if(holders.begin(), holders.end(),
                    [txn_id](const LockEntry& e) { return e.holder == txn_id; }),
                holders.end());
            processWaiters(rk);
            if (lt_it->second.holders.empty() && lt_it->second.waiters.empty()) {
                lock_table_.erase(lt_it);
            }
        }
        txn_it->second.erase(rk);
    }

    stats_escalations_.fetch_add(1, std::memory_order_relaxed);
    THEMIS_INFO("LockManager: escalated {} row locks to table lock '{}' for txn {}",
                row_keys.size(), table_key, txn_id);
}

// ---------------------------------------------------------------------------
// Predicate locking for SSI
// ---------------------------------------------------------------------------

bool LockManager::acquirePredicateLock(TransactionId txn_id,
                                        const std::string& start_key,
                                        const std::string& end_key)
{
    if (!predicate_locking_enabled_.load(std::memory_order_relaxed)) {
        return false;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    size_t max_locks = max_predicate_locks_.load(std::memory_order_relaxed);
    if (max_locks > 0 && predicate_locks_.size() >= max_locks) {
        // Limit reached: drop the lock silently.  This may raise the
        // false-positive abort rate but does not compromise correctness.
        return false;
    }
    predicate_locks_.push_back({txn_id, start_key, end_key});
    return true;
}

void LockManager::setMaxPredicateLocks(size_t max_locks) {
    max_predicate_locks_.store(max_locks, std::memory_order_relaxed);
}

size_t LockManager::getMaxPredicateLocks() const {
    return max_predicate_locks_.load(std::memory_order_relaxed);
}

void LockManager::setPredicateLockingEnabled(bool enabled) {
    predicate_locking_enabled_.store(enabled, std::memory_order_relaxed);
}

bool LockManager::isPredicateLockingEnabled() const {
    return predicate_locking_enabled_.load(std::memory_order_relaxed);
}

void LockManager::releasePredicateLocks(TransactionId txn_id)
{
    std::lock_guard<std::mutex> lk(mutex_);
    predicate_locks_.erase(
        std::remove_if(predicate_locks_.begin(), predicate_locks_.end(),
                       [txn_id](const PredicateLock& pl) {
                           return pl.txn_id == txn_id;
                       }),
        predicate_locks_.end());
}

LockManager::TransactionId LockManager::checkPredicateConflict(
    TransactionId writing_txn_id, const std::string& key) const
{
    if (!predicate_locking_enabled_.load(std::memory_order_relaxed)) {
        return 0;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& pl : predicate_locks_) {
        if (pl.txn_id == writing_txn_id) continue;
        if (key >= pl.start_key && key <= pl.end_key) {
            return pl.txn_id;
        }
    }
    return 0;
}

size_t LockManager::getPredicateLockCount(TransactionId txn_id) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<size_t>(
        std::count_if(predicate_locks_.begin(), predicate_locks_.end(),
                      [txn_id](const PredicateLock& pl) {
                          return pl.txn_id == txn_id;
                      }));
}

std::vector<std::pair<std::string, std::string>>
LockManager::getPredicateLockRanges(TransactionId txn_id) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& pl : predicate_locks_) {
        if (pl.txn_id == txn_id) {
            result.emplace_back(pl.start_key, pl.end_key);
        }
    }
    return result;
}

} // namespace themis
