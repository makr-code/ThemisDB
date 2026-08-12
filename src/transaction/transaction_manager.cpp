/**
 * @file transaction_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=20, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/transaction_manager.h"
#include "transaction/crash_recovery_manager.h"
#include "transaction/snapshot_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/history_manager.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/saga.h"
#include "utils/logger.h"
#include <algorithm>
#include <functional>
#include <thread>

namespace themis {

TransactionManager::TransactionManager(RocksDBWrapper& db,
                                       SecondaryIndexManager& secIdx,
                                       GraphIndexManager& graphIdx,
                                       VectorIndexManager& vecIdx)
    : db_(db), secIdx_(secIdx), graphIdx_(graphIdx), vecIdx_(vecIdx) {
    // Start deadlock detector thread
    deadlock_detector_running_ = true;
    deadlock_detector_thread_ = std::make_unique<std::thread>(&TransactionManager::deadlockDetectorLoop, this);
}

TransactionManager::~TransactionManager() {
    // Stop deadlock detector thread with proper synchronization
    deadlock_detector_running_ = false;
    
    // Notify with proper mutex locking
    {
        std::lock_guard<std::mutex> lock(deadlock_detector_mutex_);
        deadlock_detector_cv_.notify_all();
    }
    
    if (deadlock_detector_thread_ && deadlock_detector_thread_->joinable()) {
        deadlock_detector_thread_->join();
    }
}

// Deadlock detection methods
void TransactionManager::setDeadlockDetection(bool enabled) {
    deadlock_detection_enabled_.store(enabled, std::memory_order_relaxed);
    if (enabled) {
        THEMIS_INFO("Deadlock detection enabled");
    } else {
        THEMIS_INFO("Deadlock detection disabled");
    }
}

void TransactionManager::setDeadlockTimeout(std::chrono::milliseconds timeout_ms) {
    deadlock_timeout_ms_.store(timeout_ms.count(), std::memory_order_relaxed);
    THEMIS_INFO("Deadlock timeout set to {} ms", timeout_ms.count());
    
    // Wake up detector thread to use new timeout immediately
    {
        std::lock_guard<std::mutex> lock(deadlock_detector_mutex_);
        deadlock_detector_cv_.notify_one();
    }
}

void TransactionManager::setDeadlockVictimPolicy(DeadlockVictimPolicy policy) {
    victim_policy_.store(static_cast<int>(policy), std::memory_order_relaxed);
    const char* name =
        policy == DeadlockVictimPolicy::YOUNGEST        ? "YOUNGEST"        :
        policy == DeadlockVictimPolicy::OLDEST          ? "OLDEST"          :
        policy == DeadlockVictimPolicy::LEAST_EXPENSIVE ? "LEAST_EXPENSIVE" : "UNKNOWN";
    THEMIS_INFO("Deadlock victim policy set to {}", name);
}

TransactionManager::DeadlockVictimPolicy TransactionManager::getDeadlockVictimPolicy() const {
    return static_cast<DeadlockVictimPolicy>(victim_policy_.load(std::memory_order_relaxed));
}

TransactionManager::DeadlockMetrics TransactionManager::getDeadlockMetrics() const {
    DeadlockMetrics m;
    m.total_detected  = total_deadlocks_.load(std::memory_order_relaxed);
    m.total_resolved  = m.total_detected; // every detected deadlock is resolved
    m.max_cycle_length = deadlock_max_cycle_len_.load(std::memory_order_relaxed);
    if (m.total_detected > 0) {
        m.avg_cycle_length = static_cast<double>(
            deadlock_total_cycle_len_.load(std::memory_order_relaxed)) /
            static_cast<double>(m.total_detected);
    }
    m.active_policy = getDeadlockVictimPolicy();
    return m;
}

std::vector<TransactionManager::DeadlockInfo> TransactionManager::getDeadlocks(std::chrono::seconds max_age) const {
    std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
    
    auto cutoff = std::chrono::system_clock::now() - max_age;
    std::vector<DeadlockInfo> result;
    
    for (const auto& deadlock : recent_deadlocks_) {
        if (deadlock.detected_at >= cutoff) {
            result.push_back(deadlock);
        }
    }
    
    return result;
}

void TransactionManager::trackLockAcquired(TransactionId txn_id, const std::string& key) {
    if (!deadlock_detection_enabled_.load(std::memory_order_relaxed)) return;
    
    std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
    held_locks_[key] = LockInfo{txn_id, std::chrono::system_clock::now()};
    
    // Clear waiting status for this transaction
    auto it = waiting_for_.find(txn_id);
    if (it != waiting_for_.end()) {
        it->second.erase(key);
        if (it->second.empty()) {
            waiting_for_.erase(it);
        }
    }
}

void TransactionManager::trackLockReleased(TransactionId txn_id, const std::string& key) {
    if (!deadlock_detection_enabled_.load(std::memory_order_relaxed)) return;
    
    std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
    auto it = held_locks_.find(key);
    if (it != held_locks_.end() && it->second.holder == txn_id) {
        held_locks_.erase(it);
    }
}

void TransactionManager::trackLockWaiting(TransactionId txn_id, const std::string& key) {
    if (!deadlock_detection_enabled_.load(std::memory_order_relaxed)) return;
    
    std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
    waiting_for_[txn_id].insert(key);
}

void TransactionManager::clearWaiting(TransactionId txn_id) {
    if (!deadlock_detection_enabled_.load(std::memory_order_relaxed)) return;
    
    std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
    waiting_for_.erase(txn_id);
}

void TransactionManager::deadlockDetectorLoop() {
    while (deadlock_detector_running_.load(std::memory_order_relaxed)) {
        // Wait for the configured timeout period using separate mutex
        {
            std::unique_lock<std::mutex> lock(deadlock_detector_mutex_);
            auto timeout = std::chrono::milliseconds(deadlock_timeout_ms_.load(std::memory_order_relaxed));
            deadlock_detector_cv_.wait_for(lock, timeout, [this]() {
                return !deadlock_detector_running_.load(std::memory_order_relaxed);
            });
        }
        
        if (!deadlock_detector_running_.load(std::memory_order_relaxed)) break;

        // Check for timed-out transactions (independent of deadlock detection flag)
        abortTimedOutTransactions();

        if (!deadlock_detection_enabled_.load(std::memory_order_relaxed)) continue;
        
        // Check for deadlocks
        std::vector<TransactionId> cycle;
        if (detectDeadlockCycle(cycle)) {
            THEMIS_WARN("Deadlock detected involving {} transactions", cycle.size());
            resolveDeadlock(cycle);
            total_deadlocks_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

bool TransactionManager::detectDeadlockCycle(std::vector<TransactionId>& cycle) {
    std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
    
    if (waiting_for_.empty()) return false;
    
    // Build wait-for graph: txn -> transactions it's waiting for
    std::unordered_map<TransactionId, std::unordered_set<TransactionId>> wait_graph;
    
    for (const auto& [waiting_txn, keys] : waiting_for_) {
        for (const auto& key : keys) {
            auto lock_it = held_locks_.find(key);
            if (lock_it != held_locks_.end()) {
                TransactionId holding_txn = lock_it->second.holder;
                if (holding_txn != waiting_txn) {
                    wait_graph[waiting_txn].insert(holding_txn);
                }
            }
        }
    }
    
    // Detect cycle using DFS
    std::unordered_set<TransactionId> visited;
    std::unordered_set<TransactionId> rec_stack;
    std::vector<TransactionId> path;
    
    std::function<bool(TransactionId)> dfs = [&](TransactionId node) -> bool {
        visited.insert(node);
        rec_stack.insert(node);
        path.push_back(node);
        
        auto it = wait_graph.find(node);
        if (it != wait_graph.end()) {
            for (TransactionId neighbor : it->second) {
                if (rec_stack.count(neighbor)) {
                    // Found a cycle
                    auto cycle_start = std::find(path.begin(), path.end(), neighbor);
                    cycle.assign(cycle_start, path.end());
                    return true;
                }
                if (!visited.count(neighbor)) {
                    if (dfs(neighbor)) return true;
                }
            }
        }
        
        path.pop_back();
        rec_stack.erase(node);
        return false;
    };
    
    for (const auto& [txn_id, _] : wait_graph) {
        if (!visited.count(txn_id)) {
            if (dfs(txn_id)) {
                return true;
            }
        }
    }
    
    return false;
}

void TransactionManager::resolveDeadlock(const std::vector<TransactionId>& cycle) {
    if (cycle.empty()) return;

    auto policy = static_cast<DeadlockVictimPolicy>(
        victim_policy_.load(std::memory_order_relaxed));

    TransactionId victim_id;

    switch (policy) {
        case DeadlockVictimPolicy::OLDEST:
            // Abort the transaction with the smallest (oldest) ID
            victim_id = *std::min_element(cycle.begin(), cycle.end());
            THEMIS_WARN("Deadlock resolution: aborting transaction {} (oldest in cycle)", victim_id);
            break;

        case DeadlockVictimPolicy::LEAST_EXPENSIVE: {
            // Abort the transaction that holds the fewest locks (cheapest to restart)
            victim_id = cycle[0];
            size_t min_locks = std::numeric_limits<size_t>::max();
            {
                std::lock_guard<std::mutex> lk(lock_tracking_mutex_);
                for (TransactionId txn : cycle) {
                    size_t cnt = 0;
                    for (const auto& [key, info] : held_locks_) {
                        if (info.holder == txn) ++cnt;
                    }
                    if (cnt < min_locks) {
                        min_locks = cnt;
                        victim_id = txn;
                    }
                }
            }
            THEMIS_WARN("Deadlock resolution: aborting transaction {} (fewest locks={})", victim_id, min_locks);
            break;
        }

        case DeadlockVictimPolicy::YOUNGEST:
        default:
            // Abort the transaction with the highest (newest) ID
            victim_id = *std::max_element(cycle.begin(), cycle.end());
            THEMIS_WARN("Deadlock resolution: aborting transaction {} (youngest in cycle)", victim_id);
            break;
    }

    // Update cumulative metrics
    uint64_t cycle_len = static_cast<uint64_t>(cycle.size());
    deadlock_total_cycle_len_.fetch_add(cycle_len, std::memory_order_relaxed);
    // Update max cycle length atomically using compare_exchange_strong to avoid
    // spurious failures on architectures where weak CAS is unreliable.
    uint64_t prev = deadlock_max_cycle_len_.load(std::memory_order_relaxed);
    while (cycle_len > prev &&
           !deadlock_max_cycle_len_.compare_exchange_strong(prev, cycle_len,
               std::memory_order_relaxed, std::memory_order_relaxed)) {}
    
    // Record deadlock info
    DeadlockInfo info;
    info.cycle = cycle;
    info.detected_at = std::chrono::system_clock::now();
    info.victim_id = victim_id;
    info.policy_used = policy;

    // Adaptive Deadlock Prevention: collect cycle keys BEFORE erasing the
    // victim's entries from held_locks_ / waiting_for_ so that all participants'
    // keys (including the victim's) are captured in the training data.
    std::vector<std::string> cycle_keys;
    {
        std::lock_guard<std::mutex> lock(lock_tracking_mutex_);

        // Collect for predictor (deduplicate via temporary set).
        if (deadlock_predictor_.load(std::memory_order_acquire)) {
            std::unordered_set<std::string> seen;
            for (const auto& txn_id : cycle) {
                for (const auto& [key, linfo] : held_locks_) {
                    if (linfo.holder == txn_id && seen.insert(key).second) {
                        cycle_keys.push_back(key);
                    }
                }
                auto wit = waiting_for_.find(txn_id);
                if (wit != waiting_for_.end()) {
                    for (const auto& wkey : wit->second) {
                        if (seen.insert(wkey).second) {
                            cycle_keys.push_back(wkey);
                        }
                    }
                }
            }
        }

        recent_deadlocks_.push_back(info);
        
        // Keep only last 100 deadlocks (use pop_front for efficiency with deque)
        if (recent_deadlocks_.size() > 100) {
            recent_deadlocks_.pop_front();
        }
        
        // Clear waiting and locks for victim
        waiting_for_.erase(victim_id);
        for (auto it = held_locks_.begin(); it != held_locks_.end();) {
            if (it->second.holder == victim_id) {
                it = held_locks_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Abort the victim transaction (outside lock to avoid potential deadlock)
    // Note: rollbackTransaction has its own internal locking
    rollbackTransaction(victim_id);

    // Adaptive Deadlock Prevention: notify the predictor about this deadlock so
    // it can increase the conflict weight for the keys involved in the cycle.
    if (!cycle_keys.empty()) {
        if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
            dp->recordDeadlock(cycle_keys);
        }
    }
}

// Session-based transaction management
TransactionManager::TransactionId TransactionManager::generateTransactionId() {
    return next_transaction_id_.fetch_add(1, std::memory_order_relaxed);
}

TransactionManager::TransactionId TransactionManager::beginTransaction(IsolationLevel isolation) {
    auto txn_id = generateTransactionId();
    auto txn = std::make_shared<Transaction>(txn_id, db_, secIdx_, graphIdx_, vecIdx_, isolation,
                                              &lock_manager_);
    // Inject history/conflict managers so the transaction can record history entries
    // and build conflict artifacts.
    txn->history_mgr_  = history_mgr_;
    txn->conflict_mgr_ = conflict_mgr_;
    applyDefaultTimeout(*txn);
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        active_transactions_[txn_id] = txn;
    }
    
    // SOLUTION 2B: Update statistics with sequence lock
    updateStatsWithSeqLock([this]() {
        total_begun_.fetch_add(1, std::memory_order_relaxed);
    });
    THEMIS_INFO("Transaction {} begun (isolation: {})", txn_id,
               isolation == IsolationLevel::READ_UNCOMMITTED  ? "READ_UNCOMMITTED"  :
               isolation == IsolationLevel::READ_COMMITTED    ? "READ_COMMITTED"    :
               isolation == IsolationLevel::REPEATABLE_READ   ? "REPEATABLE_READ"   :
               isolation == IsolationLevel::SERIALIZABLE      ? "SERIALIZABLE"      :
                                                                "UNKNOWN");

    // Phase 8: WAL – log transaction begin for crash recovery
    if (crash_recovery_mgr_) {
        crash_recovery_mgr_->logBegin(txn_id, isolation);
    }

    return txn_id;
}

TransactionManager::TransactionId TransactionManager::beginTransaction(
    std::string_view tenant_id, IsolationLevel isolation)
{
    // Delegate to the no-tenant overload when tenant_id is empty.
    if (tenant_id.empty()) return beginTransaction(isolation);

    auto txn_id = generateTransactionId();
    auto txn = std::make_shared<Transaction>(txn_id, db_, secIdx_, graphIdx_, vecIdx_,
                                              isolation, &lock_manager_,
                                              std::string(tenant_id));
    txn->history_mgr_  = history_mgr_;
    txn->conflict_mgr_ = conflict_mgr_;
    applyDefaultTimeout(*txn);

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        active_transactions_[txn_id] = txn;
        tenant_stats_[std::string(tenant_id)].total_begun++;
    }

    updateStatsWithSeqLock([this]() {
        total_begun_.fetch_add(1, std::memory_order_relaxed);
    });
    THEMIS_INFO("Transaction {} begun for tenant '{}' (isolation: {})", txn_id, tenant_id,
               isolation == IsolationLevel::READ_UNCOMMITTED  ? "READ_UNCOMMITTED"  :
               isolation == IsolationLevel::READ_COMMITTED    ? "READ_COMMITTED"    :
               isolation == IsolationLevel::REPEATABLE_READ   ? "REPEATABLE_READ"   :
               isolation == IsolationLevel::SERIALIZABLE      ? "SERIALIZABLE"      :
                                                                "UNKNOWN");

    if (crash_recovery_mgr_) {
        crash_recovery_mgr_->logBegin(txn_id, isolation);
    }

    return txn_id;
}

std::shared_ptr<TransactionManager::Transaction> TransactionManager::getTransaction(TransactionId id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = active_transactions_.find(id);
    if (it != active_transactions_.end()) {
        return it->second;
    }
    return nullptr;
}

TransactionManager::Status TransactionManager::commitTransaction(TransactionId id) {
    std::shared_ptr<Transaction> txn;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = active_transactions_.find(id);
        if (it == active_transactions_.end()) {
            return Status::Error("Transaction not found or already completed");
        }
        txn = it->second;
    }
    
    auto status = txn->commit();
    if (status.ok) {
        // SOLUTION 2B: Update statistics with sequence lock
        updateStatsWithSeqLock([this]() {
            total_committed_.fetch_add(1, std::memory_order_relaxed);
        });
        // Update per-tenant stats when the transaction belongs to a tenant.
        if (!txn->tenant_id_.empty()) {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            tenant_stats_[txn->tenant_id_].total_committed++;
        }
        THEMIS_INFO("Transaction {} committed (duration: {} ms)", id, txn->getDurationMs());
        // Phase 8: WAL – log commit
        if (crash_recovery_mgr_) crash_recovery_mgr_->logCommit(id);
    } else {
        // SOLUTION 2B: Update statistics with sequence lock
        updateStatsWithSeqLock([this]() {
            total_aborted_.fetch_add(1, std::memory_order_relaxed);
        });
        // Update per-tenant stats when the transaction belongs to a tenant.
        if (!txn->tenant_id_.empty()) {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            tenant_stats_[txn->tenant_id_].total_aborted++;
        }
        THEMIS_WARN("Transaction {} commit failed: {}", id, status.message);
        // Phase 8: WAL – log abort (commit failed → transaction is rolled back)
        if (crash_recovery_mgr_) crash_recovery_mgr_->logAbort(id);
    }
    
    moveToCompleted(id);

    // Adaptive Deadlock Prevention: feed the predictor with this transaction's
    // lock history so future probability estimates improve over time.
    if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
        std::vector<std::string> keys;
        {
            std::lock_guard<std::mutex> lk(lock_tracking_mutex_);
            for (const auto& [key, info] : held_locks_) {
                if (info.holder == id) keys.push_back(key);
            }
        }
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::milliseconds(txn->getDurationMs()));
        dp->recordTransaction(id, keys, duration_us);
    }

    return status;
}

bool TransactionManager::rollbackTransaction(TransactionId id) {
    std::shared_ptr<Transaction> txn;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = active_transactions_.find(id);
        if (it == active_transactions_.end()) {
            return false;  // Already completed or doesn't exist
        }
        txn = it->second;
    }
    
    txn->rollback();
    // SOLUTION 2B: Update statistics with sequence lock
    updateStatsWithSeqLock([this]() {
        total_aborted_.fetch_add(1, std::memory_order_relaxed);
    });
    // Update per-tenant stats when the transaction belongs to a tenant.
    if (!txn->tenant_id_.empty()) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        tenant_stats_[txn->tenant_id_].total_aborted++;
    }
    THEMIS_INFO("Transaction {} rolled back (duration: {} ms)", id, txn->getDurationMs());
    // Phase 8: WAL – log abort
    if (crash_recovery_mgr_) crash_recovery_mgr_->logAbort(id);
    
    moveToCompleted(id);

    // Adaptive Deadlock Prevention: record rolled-back transaction's pattern.
    if (DeadlockPredictor* dp = deadlock_predictor_.load(std::memory_order_acquire)) {
        std::vector<std::string> keys;
        {
            std::lock_guard<std::mutex> lk(lock_tracking_mutex_);
            for (const auto& [key, info] : held_locks_) {
                if (info.holder == id) keys.push_back(key);
            }
        }
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::milliseconds(txn->getDurationMs()));
        dp->recordTransaction(id, keys, duration_us);
    }

    return true;
}

void TransactionManager::moveToCompleted(TransactionId id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = active_transactions_.find(id);
    if (it != active_transactions_.end()) {
        // RACE CONDITION FIX: Check if transaction already exists in completed map
        auto completed_it = completed_transactions_.find(id);
        if (completed_it != completed_transactions_.end()) {
            THEMIS_WARN("Transaction {} already in completed map, skipping duplicate move", id);
            active_transactions_.erase(it);
            return;
        }
        
        completed_transactions_[id] = std::move(it->second);
        active_transactions_.erase(it);
    }
}

TransactionManager::Stats TransactionManager::getStats() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    Stats stats;
    stats.total_begun = total_begun_.load(std::memory_order_relaxed);
    stats.total_committed = total_committed_.load(std::memory_order_relaxed);
    stats.total_aborted = total_aborted_.load(std::memory_order_relaxed);
    stats.total_timed_out = total_timed_out_.load(std::memory_order_relaxed);
    stats.active_count = active_transactions_.size();
    
    // Calculate average and max duration
    uint64_t total_duration = 0;
    stats.max_duration_ms = 0;
    size_t count = 0;
    
    for (const auto& [id, txn] : completed_transactions_) {
        auto duration = txn->getDurationMs();
        total_duration += duration;
        stats.max_duration_ms = std::max(stats.max_duration_ms, duration);
        ++count;
    }
    
    stats.avg_duration_ms = count > 0 ? total_duration / count : 0;
    
    return stats;
}

// SOLUTION 2B: Lock-free statistics with sequence lock pattern
TransactionManager::Stats TransactionManager::getStatsLockFree() const {
    Stats stats;
    uint64_t seq1, seq2;
    
    // Optimistic read with retry on concurrent modification
    do {
        // Read sequence number (acquire semantics)
        seq1 = stats_sequence_.load(std::memory_order_acquire);
        
        // If sequence is odd, a writer is active - retry
        if ((seq1 & 1) != 0) {
            std::this_thread::yield();  // Give writer a chance to finish
            continue;
        }
        
        // Read all statistics atomically
        stats.total_begun = total_begun_.load(std::memory_order_relaxed);
        stats.total_committed = total_committed_.load(std::memory_order_relaxed);
        stats.total_aborted = total_aborted_.load(std::memory_order_relaxed);
        stats.total_timed_out = total_timed_out_.load(std::memory_order_relaxed);
        
        // For map sizes, we need a quick lock (cannot be done lock-free)
        // This is acceptable as the lock is held very briefly
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            stats.active_count = active_transactions_.size();
            
            // Calculate duration stats
            uint64_t total_duration = 0;
            stats.max_duration_ms = 0;
            size_t count = 0;
            
            for (const auto& [id, txn] : completed_transactions_) {
                auto duration = txn->getDurationMs();
                total_duration += duration;
                stats.max_duration_ms = std::max(stats.max_duration_ms, duration);
                ++count;
            }
            
            stats.avg_duration_ms = count > 0 ? total_duration / count : 0;
        }
        
        // Read sequence number again (acquire semantics)
        seq2 = stats_sequence_.load(std::memory_order_acquire);
        
    } while (seq1 != seq2 || (seq1 & 1) != 0);  // Retry if modified during read
    
    return stats;
}

// Helper to update statistics with sequence lock protocol
void TransactionManager::updateStatsWithSeqLock(std::function<void()> update) {
    // Increment sequence (odd = writer active)
    stats_sequence_.fetch_add(1, std::memory_order_release);
    
    // Perform the update
    update();
    
    // Increment sequence again (even = no active writer)
    stats_sequence_.fetch_add(1, std::memory_order_release);
}

void TransactionManager::cleanupOldTransactions(std::chrono::seconds max_age) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - max_age;
    
    for (auto it = completed_transactions_.begin(); it != completed_transactions_.end(); ) {
        if (it->second->getStartTime() < cutoff) {
            it = completed_transactions_.erase(it);
        } else {
            ++it;
        }
    }
}

// ── Per-tenant transaction namespace ─────────────────────────────────────────

size_t TransactionManager::countActiveTenantTransactionsLocked(std::string_view tenant_id) const {
    size_t count = 0;
    for (const auto& [txn_id, txn] : active_transactions_) {
        if (txn->tenant_id_ == tenant_id) {
            count++;
        }
    }
    return count;
}

TransactionManager::TenantTransactionStats
TransactionManager::getTenantTransactionStats(std::string_view tenant_id) const {
    TenantTransactionStats result;
    result.tenant_id = std::string(tenant_id);

    std::lock_guard<std::mutex> lock(sessions_mutex_);

    // Cumulative counters from the stats map
    auto it = tenant_stats_.find(std::string(tenant_id));
    if (it != tenant_stats_.end()) {
        result.total_begun     = it->second.total_begun;
        result.total_committed = it->second.total_committed;
        result.total_aborted   = it->second.total_aborted;
    }

    result.active_count = countActiveTenantTransactionsLocked(tenant_id);
    return result;
}

std::vector<TransactionManager::TenantTransactionStats>
TransactionManager::getAllTenantTransactionStats() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);

    // Count active transactions per tenant
    std::unordered_map<std::string, uint64_t> active_counts;
    for (const auto& [txn_id, txn] : active_transactions_) {
        if (!txn->tenant_id_.empty()) {
            active_counts[txn->tenant_id_]++;
        }
    }

    std::vector<TenantTransactionStats> result;
    result.reserve(tenant_stats_.size());
    for (const auto& [tid, entry] : tenant_stats_) {
        TenantTransactionStats s;
        s.tenant_id       = tid;
        s.total_begun     = entry.total_begun;
        s.total_committed = entry.total_committed;
        s.total_aborted   = entry.total_aborted;
        auto ac = active_counts.find(tid);
        s.active_count = (ac != active_counts.end()) ? ac->second : 0;
        result.push_back(std::move(s));
    }
    return result;
}

size_t TransactionManager::getActiveTenantTransactionCount(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return countActiveTenantTransactionsLocked(tenant_id);
}

std::vector<TransactionManager::TransactionId>
TransactionManager::listTenantTransactionIds(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    std::vector<TransactionId> ids;
    for (const auto& [txn_id, txn] : active_transactions_) {
        if (txn->tenant_id_ == tenant_id) {
            ids.push_back(txn_id);
        }
    }
    return ids;
}

size_t TransactionManager::abortTenantTransactions(std::string_view tenant_id) {
    // Collect transaction IDs first to avoid rolling back while holding the lock.
    std::vector<TransactionId> to_abort;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& [txn_id, txn] : active_transactions_) {
            if (txn->tenant_id_ == tenant_id) {
                to_abort.push_back(txn_id);
            }
        }
    }

    for (auto txn_id : to_abort) {
        rollbackTransaction(txn_id);
    }

    THEMIS_INFO("Aborted {} transaction(s) for tenant '{}'", to_abort.size(), tenant_id);
    return to_abort.size();
}

// ── Transaction Timeout / Auto-Rollback ──────────────────────────────────────

void TransactionManager::setTransactionTimeout(std::chrono::milliseconds timeout_ms) {
    const uint64_t prev = transaction_timeout_ms_.exchange(
        static_cast<uint64_t>(timeout_ms.count()),
        std::memory_order_relaxed);

    // Only log on transitions between disabled (0) and enabled, or value changes
    if (prev != static_cast<uint64_t>(timeout_ms.count())) {
        if (timeout_ms.count() == 0) {
            THEMIS_DEBUG("TransactionManager: transaction timeout disabled");
        } else {
            THEMIS_DEBUG("TransactionManager: transaction timeout set to {} ms",
                        timeout_ms.count());
        }
    }
}

std::chrono::milliseconds TransactionManager::getTransactionTimeout() const {
    return std::chrono::milliseconds(
        transaction_timeout_ms_.load(std::memory_order_relaxed));
}

uint64_t TransactionManager::getTimedOutCount() const {
    return total_timed_out_.load(std::memory_order_relaxed);
}

size_t TransactionManager::abortTimedOutTransactions() {
    const uint64_t timeout_ms = transaction_timeout_ms_.load(std::memory_order_relaxed);
    if (timeout_ms == 0) return 0;   // feature disabled

    const auto now = std::chrono::system_clock::now();
    const auto limit = std::chrono::milliseconds(timeout_ms);

    // Collect IDs of transactions that have exceeded the timeout.
    // We hold sessions_mutex_ briefly to snapshot candidate IDs, then
    // release it before calling rollbackTransaction (which re-acquires it
    // via moveToCompleted) to avoid a nested-lock deadlock.
    std::vector<TransactionId> expired;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& [id, txn] : active_transactions_) {
            if (txn->isFinished()) continue;  // already decided
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - txn->getStartTime());
            if (elapsed >= limit) {
                expired.push_back(id);
            }
        }
    }

    for (TransactionId id : expired) {
        THEMIS_WARN("TransactionManager: aborting timed-out transaction {} ({}ms limit)",
                    id, timeout_ms);
        rollbackTransaction(id);
        total_timed_out_.fetch_add(1, std::memory_order_relaxed);
    }

    return expired.size();
}

// Direct transaction (legacy API)
TransactionManager::Transaction TransactionManager::begin(IsolationLevel isolation) {
    auto txn_id = generateTransactionId();
    // SOLUTION 2B: Update statistics with sequence lock
    updateStatsWithSeqLock([this]() {
        total_begun_.fetch_add(1, std::memory_order_relaxed);
    });
    Transaction txn(txn_id, db_, secIdx_, graphIdx_, vecIdx_, isolation, &lock_manager_);
    txn.history_mgr_  = history_mgr_;
    txn.conflict_mgr_ = conflict_mgr_;
    applyDefaultTimeout(txn);
    return txn;
}

// ==== Transaction ==== 

TransactionManager::Transaction::Transaction(TransactionId id,
                                             RocksDBWrapper& db,
                                             SecondaryIndexManager& secIdx,
                                             GraphIndexManager& graphIdx,
                                             VectorIndexManager& vecIdx,
                                             IsolationLevel isolation,
                                             LockManager* lock_manager,
                                             std::string_view tenant_id)
    : id_(id), db_(db), secIdx_(secIdx), graphIdx_(graphIdx), vecIdx_(vecIdx), isolation_(isolation),
      start_time_(std::chrono::system_clock::now()),
      lock_manager_(lock_manager),
      tenant_id_(tenant_id) {
    // Map ThemisDB IsolationLevel to the appropriate RocksDB isolation level.
    // SERIALIZABLE and REPEATABLE_READ use snapshot isolation at the storage layer;
    // additional write-conflict checks are performed at commit time.
    // READ_UNCOMMITTED uses ReadCommitted at the RocksDB level (dirty reads are
    // prevented by RocksDB itself; the isolation hint is used by higher layers).
    auto rocksdb_isolation =
        (isolation_ == IsolationLevel::REPEATABLE_READ ||
         isolation_ == IsolationLevel::Snapshot        ||
         isolation_ == IsolationLevel::SERIALIZABLE)
        ? RocksDBWrapper::TransactionIsolationLevel::Snapshot
        : RocksDBWrapper::TransactionIsolationLevel::ReadCommitted;

    mvcc_txn_ = db_.beginTransaction(rocksdb_isolation);
    if (!mvcc_txn_) {
        throw std::runtime_error("Failed to create MVCC transaction");
    }
    saga_ = std::make_unique<Saga>();
    THEMIS_INFO("Transaction {} initialized with MVCC and SAGA support (isolation: {})",
               id_,
               isolation_ == IsolationLevel::READ_UNCOMMITTED ? "READ_UNCOMMITTED" :
               isolation_ == IsolationLevel::READ_COMMITTED   ? "READ_COMMITTED"   :
               isolation_ == IsolationLevel::REPEATABLE_READ  ? "REPEATABLE_READ"  :
               isolation_ == IsolationLevel::SERIALIZABLE     ? "SERIALIZABLE"     :
                                                                "UNKNOWN");
}

TransactionManager::Transaction::~Transaction() {
    // Use atomic load to check if finished
    if (!finished_.load(std::memory_order_acquire) && mvcc_txn_ && mvcc_txn_->isActive()) {
        THEMIS_WARN("Transaction {} destructed without commit/rollback; rolling back implicitly", id_);
        // Capture duration before the implicit rollback so that getDurationMs() returns
        // the actual run time rather than "time since start" after the object is destroyed.
        captureDuration();
        mvcc_txn_->rollback();
        saga_->compensate();
        // Release any predicate locks held by this transaction
        if (lock_manager_) lock_manager_->releasePredicateLocks(id_);
    }
}

void TransactionManager::Transaction::captureDuration() noexcept {
    finished_duration_ms_.store(
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time_).count()),
        std::memory_order_relaxed);
}

uint64_t TransactionManager::Transaction::getDurationMs() const {
    // Once the transaction is finished, return the frozen duration captured at
    // commit/rollback time.  Without this, getStats() would report ever-growing
    // durations for transactions sitting in completed_transactions_.
    if (finished_.load(std::memory_order_acquire)) {
        return finished_duration_ms_.load(std::memory_order_relaxed);
    }
    auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count());
}

TransactionManager::Transaction::Transaction(Transaction&& other) noexcept
    : id_(other.id_), db_(other.db_), secIdx_(other.secIdx_), graphIdx_(other.graphIdx_), 
      vecIdx_(other.vecIdx_), isolation_(other.isolation_), start_time_(other.start_time_),
      mvcc_txn_(std::move(other.mvcc_txn_)), saga_(std::move(other.saga_)), 
      finished_(other.finished_.load(std::memory_order_acquire)),
      timeout_ms_(other.timeout_ms_.load(std::memory_order_acquire)),
      finished_duration_ms_(other.finished_duration_ms_.load(std::memory_order_acquire)),
      savepoints_(std::move(other.savepoints_)),
      lock_manager_(other.lock_manager_),
      history_mgr_(other.history_mgr_),
      conflict_mgr_(other.conflict_mgr_),
      base_values_(std::move(other.base_values_)),
      our_values_(std::move(other.our_values_)),
      tenant_id_(std::move(other.tenant_id_)) {
    other.finished_.store(true, std::memory_order_release);
    other.lock_manager_ = nullptr;
    other.history_mgr_  = nullptr;
    other.conflict_mgr_ = nullptr;
}

TransactionManager::Transaction& TransactionManager::Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (!finished_.load(std::memory_order_acquire) && mvcc_txn_ && mvcc_txn_->isActive()) {
            mvcc_txn_->rollback();
            saga_->compensate();
            if (lock_manager_) lock_manager_->releasePredicateLocks(id_);
        }
        // Anmerkung: db_, secIdx_, graphIdx_ sind Referenzen und können nicht erneut gebunden werden.
        // Diese werden in Konstruktor initialisiert und bleiben über Lebensdauer konstant.
        mvcc_txn_ = std::move(other.mvcc_txn_);
        saga_ = std::move(other.saga_);
        savepoints_ = std::move(other.savepoints_);
        lock_manager_ = other.lock_manager_;
        other.lock_manager_ = nullptr;
        history_mgr_  = other.history_mgr_;
        conflict_mgr_ = other.conflict_mgr_;
        other.history_mgr_  = nullptr;
        other.conflict_mgr_ = nullptr;
        base_values_ = std::move(other.base_values_);
        our_values_  = std::move(other.our_values_);
        tenant_id_   = std::move(other.tenant_id_);
        timeout_ms_.store(other.timeout_ms_.load(std::memory_order_acquire), std::memory_order_release);
        finished_duration_ms_.store(other.finished_duration_ms_.load(std::memory_order_acquire), std::memory_order_release);
        finished_.store(other.finished_.load(std::memory_order_acquire), std::memory_order_release);
        other.finished_.store(true, std::memory_order_release);
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Read-Only Transaction Optimization
// ---------------------------------------------------------------------------

TransactionManager::Status TransactionManager::Transaction::setReadOnly(bool read_only)
{
    if (!mvcc_txn_ || !mvcc_txn_->isActive())
        return Status::Error("setReadOnly: keine aktive Transaktion");
    if (read_only && !write_set_.empty())
        return Status::Error("setReadOnly: cannot set read-only flag after writes have been made");
    read_only_ = read_only;
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::putEntity(std::string_view table, const BaseEntity& entity) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("putEntity: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("putEntity: transaction timed out");
    if (read_only_) return Status::Error("putEntity: transaction is read-only");
    
    // Serialize entity
    auto serialized = entity.serialize();
    std::string key = makeNamespacedKey(std::string("entity:") + std::string(table) + ":" + entity.getPrimaryKey());

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);

    // Capture pre-write (base) value for potential conflict record.
    // Capture only on first write to this key so base reflects the snapshot value.
    if (history_mgr_ || conflict_mgr_) {
        if (base_values_.count(key) == 0) {
            auto base = mvcc_txn_->get(key);
            base_values_[key] = base ? std::move(*base) : std::vector<uint8_t>{};
        }
        our_values_[key]  = serialized;
    }

    // Write to MVCC transaction
    if (!mvcc_txn_->put(key, serialized)) {
        return Status::Error("putEntity: MVCC conflict detected");
    }

    // Write atomic history entry in the same transaction.
    if (history_mgr_) {
        if (!history_mgr_->recordPut(*mvcc_txn_, key, serialized, id_)) {
            return Status::Error("putEntity: history write failed");
        }
    }
    
    // Update secondary indexes using MVCC transaction for atomicity
    auto st = secIdx_.put(makeNamespacedTable(table), entity, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    trackWrite(key, "put");
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::eraseEntity(std::string_view table, std::string_view pk) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("eraseEntity: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("eraseEntity: transaction timed out");
    if (read_only_) return Status::Error("eraseEntity: transaction is read-only");
    
    std::string key = makeNamespacedKey(std::string("entity:") + std::string(table) + ":" + std::string(pk));

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);

    // Capture pre-delete (base) value for potential conflict record.
    // Capture only on first write to this key so base reflects the snapshot value.
    if (history_mgr_ || conflict_mgr_) {
        if (base_values_.count(key) == 0) {
            auto base = mvcc_txn_->get(key);
            base_values_[key] = base ? std::move(*base) : std::vector<uint8_t>{};
        }
        our_values_[key]  = {};  // deletion → empty "ours"
    }

    // Delete from MVCC transaction
    if (!mvcc_txn_->del(key)) {
        return Status::Error("eraseEntity: MVCC conflict detected");
    }

    // Write atomic tombstone history entry in the same transaction.
    if (history_mgr_) {
        if (!history_mgr_->recordDel(*mvcc_txn_, key, id_)) {
            return Status::Error("eraseEntity: history write failed");
        }
    }
    
    // Update secondary indexes using MVCC transaction
    auto st = secIdx_.erase(makeNamespacedTable(table), pk, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    trackWrite(key, "delete");
    return Status::OK();
}

std::optional<std::string> TransactionManager::Transaction::readEntityJson(
    std::string_view table,
    std::string_view pk
) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return std::nullopt;
    }

    const std::string key = makeNamespacedKey(
        std::string("entity:") + std::string(table) + ":" + std::string(pk));

    auto value = mvcc_txn_->get(key);
    if (!value) {
        return std::nullopt;
    }

    const auto entity = BaseEntity::deserialize(pk, *value);
    return entity.toJson();
}

TransactionManager::Status TransactionManager::Transaction::addEdge(const BaseEntity& edgeEntity) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("addEdge: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("addEdge: transaction timed out");
    if (read_only_) return Status::Error("addEdge: transaction is read-only");
    
    // Graph edges stored with MVCC
    std::string edge_key = makeNamespacedKey("graph:edge:" + edgeEntity.getPrimaryKey());

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(edge_key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);

    auto serialized = edgeEntity.serialize();
    
    if (!mvcc_txn_->put(edge_key, serialized)) {
        return Status::Error("addEdge: MVCC conflict detected");
    }
    
    // Update graph index using MVCC transaction
    auto st = graphIdx_.addEdge(edgeEntity, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    trackWrite(edge_key, "put");
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::deleteEdge(std::string_view edgeId) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("deleteEdge: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("deleteEdge: transaction timed out");
    if (read_only_) return Status::Error("deleteEdge: transaction is read-only");
    
    std::string edge_key = makeNamespacedKey("graph:edge:" + std::string(edgeId));

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(edge_key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);
    
    if (!mvcc_txn_->del(edge_key)) {
        return Status::Error("deleteEdge: MVCC conflict detected");
    }
    
    // Update graph index using MVCC transaction
    auto st = graphIdx_.deleteEdge(edgeId, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    trackWrite(edge_key, "delete");
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::addVector(const BaseEntity& entity, std::string_view vectorField) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("addVector: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("addVector: transaction timed out");
    if (read_only_) return Status::Error("addVector: transaction is read-only");
    
    std::string pk = entity.getPrimaryKey();

    // Store vector entity in MVCC transaction
    std::string vector_key = makeNamespacedKey("vector:" + pk);

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(vector_key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);

    auto serialized = entity.serialize();
    if (!mvcc_txn_->put(vector_key, serialized)) {
        return Status::Error("addVector: MVCC conflict detected");
    }
    
    // Update vector index using MVCC transaction
    auto st = vecIdx_.addEntity(entity, *mvcc_txn_, vectorField);
    if (!st.ok) {
        return Status::Error(st.message);
    }

    // Register SAGA compensating action AFTER both writes succeeded so that
    // a failed addVector (e.g. MVCC conflict) does not leave a spurious step
    // in the SAGA queue.
    saga_->addStep("vectorAdd:" + pk, [this, pk]() {
        auto status = vecIdx_.removeByPk(pk);
        if (!status.ok) {
            THEMIS_WARN("SAGA: Vector remove compensation failed for '{}': {}", pk, status.message);
        }
    });

    trackWrite(vector_key, "put");
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::updateVector(const BaseEntity& entity, std::string_view vectorField) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("updateVector: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("updateVector: transaction timed out");
    if (read_only_) return Status::Error("updateVector: transaction is read-only");
    
    // Capture old vector BEFORE the write.  If we read after the put(), the MVCC
    // read-your-own-writes buffer would return the new value instead of the original.
    std::string pk = entity.getPrimaryKey();
    std::string vector_key = makeNamespacedKey("vector:" + pk);

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(vector_key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);

    auto old_data = mvcc_txn_->get(vector_key);

    // Update vector entity in MVCC transaction
    auto serialized = entity.serialize();
    if (!mvcc_txn_->put(vector_key, serialized)) {
        return Status::Error("updateVector: MVCC conflict detected");
    }
    
    // Update vector index using MVCC transaction
    auto st = vecIdx_.updateEntity(entity, *mvcc_txn_, vectorField);
    if (!st.ok) {
        return Status::Error(st.message);
    }

    // Register SAGA compensating action AFTER both writes succeeded so that
    // a failed updateVector (e.g. MVCC conflict) does not leave a spurious step
    // in the SAGA queue.
    if (old_data) {
        // Old vector exists: capture for restoration
        auto old_entity = BaseEntity::deserialize(pk, *old_data);
        saga_->addStep("vectorUpdate:" + pk, [this, old_entity = std::move(old_entity), vectorField = std::string(vectorField)]() {
            THEMIS_DEBUG("SAGA: Restoring old vector for '{}'", old_entity.getPrimaryKey());
            auto status = vecIdx_.updateEntity(old_entity, vectorField);
            if (!status.ok) {
                THEMIS_WARN("SAGA: Vector restore failed for '{}': {}", old_entity.getPrimaryKey(), status.message);
            }
        });
    } else {
        // No old vector: this is effectively an insert, compensate with remove
        saga_->addStep("vectorUpdate:" + pk, [this, pk]() {
            THEMIS_DEBUG("SAGA: Removing newly added vector for '{}'", pk);
            vecIdx_.removeByPk(pk);
        });
    }

    trackWrite(vector_key, "put");
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::removeVector(std::string_view pk) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("removeVector: keine aktive Transaktion");
    if (isTimedOut()) return Status::Error("removeVector: transaction timed out");
    if (read_only_) return Status::Error("removeVector: transaction is read-only");
    
    // Capture old vector BEFORE the delete.  If we read after del(), the MVCC
    // read-your-own-writes buffer would return nothing instead of the original value.
    std::string pk_str(pk);
    std::string vector_key = makeNamespacedKey("vector:" + pk_str);

    // SSI: check for predicate conflict before writing
    auto conflict_msg = checkSerializableWriteConflict(vector_key);
    if (!conflict_msg.empty()) return Status::Error(conflict_msg);

    auto old_data = mvcc_txn_->get(vector_key);

    // Delete vector entity from MVCC transaction
    if (!mvcc_txn_->del(vector_key)) {
        return Status::Error("removeVector: MVCC conflict detected");
    }
    
    // Update vector index using MVCC transaction
    auto st = vecIdx_.removeByPk(pk, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }

    // Register SAGA compensating action AFTER both writes succeeded so that
    // a failed removeVector (e.g. MVCC conflict) does not leave a spurious step
    // in the SAGA queue.  Without this guard, a failed del() would queue a
    // restore step that calls addEntity() on a vector that was never deleted,
    // potentially inserting a duplicate index entry.
    if (old_data) {
        // Old vector exists: capture for restoration
        auto old_entity = BaseEntity::deserialize(pk_str, *old_data);
        saga_->addStep("vectorRemove:" + pk_str, [this, old_entity = std::move(old_entity)]() {
            THEMIS_DEBUG("SAGA: Restoring removed vector for '{}'", old_entity.getPrimaryKey());
            auto status = vecIdx_.addEntity(old_entity, "embedding");
            if (!status.ok) {
                THEMIS_WARN("SAGA: Vector restoration failed for '{}': {}", old_entity.getPrimaryKey(), status.message);
            }
        });
    } else {
        // No old vector: nothing to compensate
        saga_->addStep("vectorRemove:" + pk_str, [pk_str]() {
            THEMIS_DEBUG("SAGA: Vector remove compensation skipped (no old data) for '{}'", pk_str);
        });
    }

    trackWrite(vector_key, "delete");
    return Status::OK();
}

// ---------------------------------------------------------------------------
// OCC helpers
// ---------------------------------------------------------------------------

/// Encode a uint64_t as an 8-byte little-endian blob.
static std::vector<uint8_t> encodeVersion(uint64_t v) {
    std::vector<uint8_t> buf(8);
    for (int i = 0; i < 8; ++i) {
        buf[i] = static_cast<uint8_t>(v >> (8 * i));
    }
    return buf;
}

/// Decode an 8-byte little-endian blob to uint64_t; returns 0 on wrong size.
static uint64_t decodeVersion(const std::vector<uint8_t>& buf) {
    if (buf.size() != 8) return 0;
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(buf[i]) << (8 * i);
    }
    return v;
}

/// Build the version key for an entity.
static std::string versionKey(std::string_view table, std::string_view pk) {
    std::string k;
    k.reserve(9 + table.size() + 1 + pk.size()); // "occ:ver:" + table + ":" + pk
    k += "occ:ver:";
    k += table;
    k += ':';
    k += pk;
    return k;
}

// ---------------------------------------------------------------------------
// OCC public API
// ---------------------------------------------------------------------------

std::optional<uint64_t> TransactionManager::Transaction::getEntityVersion(
    std::string_view table, std::string_view pk)
{
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return std::nullopt;
    auto raw = mvcc_txn_->get(makeNamespacedKey(versionKey(table, pk)));
    if (!raw) return 0; // entity does not exist → version 0
    return decodeVersion(*raw);
}

TransactionManager::Status TransactionManager::Transaction::optimisticPut(
    std::string_view table, const BaseEntity& entity, uint64_t expected_version)
{
    if (!mvcc_txn_ || !mvcc_txn_->isActive())
        return Status::Error("optimisticPut: keine aktive Transaktion");
    if (isTimedOut())
        return Status::Error("optimisticPut: transaction timed out");
    if (read_only_)
        return Status::Error("optimisticPut: transaction is read-only");

    const std::string pk    = entity.getPrimaryKey();
    const std::string verKey = makeNamespacedKey(versionKey(table, pk));

    // Read current version
    auto raw = mvcc_txn_->get(verKey);
    uint64_t current_version = raw ? decodeVersion(*raw) : 0;

    // Version check
    if (expected_version == 0 && current_version != 0) {
        return Status::Error(
            "OCC entity already exists: table=" + std::string(table) +
            " pk=" + pk + " stored_version=" + std::to_string(current_version));
    }
    if (expected_version != current_version) {
        return Status::Error(
            "OCC version conflict: table=" + std::string(table) +
            " pk=" + pk +
            " expected=" + std::to_string(expected_version) +
            " actual=" + std::to_string(current_version));
    }

    // SSI: check for predicate conflict
    const std::string entKey = makeNamespacedKey(std::string("entity:") + std::string(table) + ":" + pk);
    auto conflict = checkSerializableWriteConflict(entKey);
    if (!conflict.empty()) return Status::Error(conflict);

    // Write new version
    uint64_t new_version = expected_version + 1;
    if (!mvcc_txn_->put(verKey, encodeVersion(new_version))) {
        return Status::Error("optimisticPut: MVCC conflict on version key");
    }

    // Write entity
    auto serialized = entity.serialize();
    if (!mvcc_txn_->put(entKey, serialized)) {
        return Status::Error("optimisticPut: MVCC conflict on entity key");
    }

    // Update secondary indexes
    auto st = secIdx_.put(makeNamespacedTable(table), entity, *mvcc_txn_);
    if (!st.ok) return Status::Error(st.message);

    THEMIS_DEBUG("OCC optimisticPut: table={} pk={} version {} → {}",
                 table, pk, expected_version, new_version);
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::optimisticErase(
    std::string_view table, std::string_view pk, uint64_t expected_version)
{
    if (!mvcc_txn_ || !mvcc_txn_->isActive())
        return Status::Error("optimisticErase: keine aktive Transaktion");
    if (isTimedOut())
        return Status::Error("optimisticErase: transaction timed out");
    if (read_only_)
        return Status::Error("optimisticErase: transaction is read-only");

    const std::string verKey = makeNamespacedKey(versionKey(table, pk));

    // Read current version
    auto raw = mvcc_txn_->get(verKey);
    uint64_t current_version = raw ? decodeVersion(*raw) : 0;

    if (current_version == 0) {
        return Status::Error(
            "OCC entity not found: table=" + std::string(table) +
            " pk=" + std::string(pk));
    }
    if (expected_version != current_version) {
        return Status::Error(
            "OCC version conflict: table=" + std::string(table) +
            " pk=" + std::string(pk) +
            " expected=" + std::to_string(expected_version) +
            " actual=" + std::to_string(current_version));
    }

    // SSI: check for predicate conflict
    const std::string entKey = makeNamespacedKey(
        std::string("entity:") + std::string(table) + ":" + std::string(pk));
    auto conflict = checkSerializableWriteConflict(entKey);
    if (!conflict.empty()) return Status::Error(conflict);

    // Delete version key
    if (!mvcc_txn_->del(verKey)) {
        return Status::Error("optimisticErase: MVCC conflict on version key");
    }

    // Delete entity
    if (!mvcc_txn_->del(entKey)) {
        return Status::Error("optimisticErase: MVCC conflict on entity key");
    }

    // Update secondary indexes
    auto st = secIdx_.erase(makeNamespacedTable(table), pk, *mvcc_txn_);
    if (!st.ok) return Status::Error(st.message);

    THEMIS_DEBUG("OCC optimisticErase: table={} pk={} version={}", table, pk, expected_version);
    return Status::OK();
}

// ---------------------------------------------------------------------------
// Bulk API
// ---------------------------------------------------------------------------

TransactionManager::Status TransactionManager::Transaction::bulkPutEntities(
    std::string_view table, const std::vector<BaseEntity>& entities)
{
    if (!mvcc_txn_ || !mvcc_txn_->isActive())
        return Status::Error("bulkPutEntities: keine aktive Transaktion");
    if (isTimedOut())
        return Status::Error("bulkPutEntities: transaction timed out");
    if (read_only_)
        return Status::Error("bulkPutEntities: transaction is read-only");
    if (entities.empty())
        return Status::OK();

    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& entity = entities[i];
        const std::string pk = entity.getPrimaryKey();
        if (pk.empty()) {
            return Status::Error("bulkPutEntities: entity[" + std::to_string(i) +
                                 "] has empty primary key");
        }
        const std::string key = makeNamespacedKey(std::string("entity:") + std::string(table) + ":" + pk);

        // SSI: check for predicate conflict before writing
        auto conflict = checkSerializableWriteConflict(key);
        if (!conflict.empty()) {
            return Status::Error("bulkPutEntities[" + std::to_string(i) + "]: " + conflict);
        }

        // Write entity data
        if (!mvcc_txn_->put(key, entity.serialize())) {
            return Status::Error("bulkPutEntities[" + std::to_string(i) +
                                 "]: MVCC conflict detected for pk=" + pk);
        }

        // Update secondary indexes within the same MVCC transaction
        auto st = secIdx_.put(makeNamespacedTable(table), entity, *mvcc_txn_);
        if (!st.ok) {
            return Status::Error("bulkPutEntities[" + std::to_string(i) + "]: " + st.message);
        }
    }

    THEMIS_DEBUG("bulkPutEntities: table={} count={}", table, entities.size());
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::bulkEraseEntities(
    std::string_view table, const std::vector<std::string>& pks)
{
    if (!mvcc_txn_ || !mvcc_txn_->isActive())
        return Status::Error("bulkEraseEntities: keine aktive Transaktion");
    if (isTimedOut())
        return Status::Error("bulkEraseEntities: transaction timed out");
    if (read_only_)
        return Status::Error("bulkEraseEntities: transaction is read-only");
    if (pks.empty())
        return Status::OK();

    for (size_t i = 0; i < pks.size(); ++i) {
        const auto& pk = pks[i];
        if (pk.empty()) {
            return Status::Error("bulkEraseEntities: pk[" + std::to_string(i) + "] is empty");
        }
        const std::string key = makeNamespacedKey(std::string("entity:") + std::string(table) + ":" + pk);

        // SSI: check for predicate conflict before writing
        auto conflict = checkSerializableWriteConflict(key);
        if (!conflict.empty()) {
            return Status::Error("bulkEraseEntities[" + std::to_string(i) + "]: " + conflict);
        }

        // Delete entity
        if (!mvcc_txn_->del(key)) {
            return Status::Error("bulkEraseEntities[" + std::to_string(i) +
                                 "]: MVCC conflict detected for pk=" + pk);
        }

        // Update secondary indexes within the same MVCC transaction
        auto st = secIdx_.erase(makeNamespacedTable(table), pk, *mvcc_txn_);
        if (!st.ok) {
            return Status::Error("bulkEraseEntities[" + std::to_string(i) + "]: " + st.message);
        }
    }

    THEMIS_DEBUG("bulkEraseEntities: table={} count={}", table, pks.size());
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::trackPredicateRead(
    const std::string& start_key, const std::string& end_key)
{
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("trackPredicateRead: transaction already finished");
    }
    if (isolation_ != IsolationLevel::SERIALIZABLE) {
        return Status::OK(); // no-op for non-serializable isolation levels
    }
    if (!lock_manager_) {
        return Status::Error("trackPredicateRead: no LockManager available");
    }
    lock_manager_->acquirePredicateLock(id_, start_key, end_key);
    THEMIS_DEBUG("Transaction {} acquired predicate lock on [{}, {}]", id_, start_key, end_key);
    return Status::OK();
}

std::string TransactionManager::Transaction::checkSerializableWriteConflict(
    const std::string& key) const
{
    if (isolation_ != IsolationLevel::SERIALIZABLE) return {};
    if (!lock_manager_) return {};
    auto conflicting_txn = lock_manager_->checkPredicateConflict(id_, key);
    if (conflicting_txn != 0) {
        THEMIS_WARN("Transaction {} write to '{}' conflicts with predicate lock held by txn {}",
                    id_, key, conflicting_txn);
        return "serialization failure: write conflicts with predicate lock held by txn " +
               std::to_string(conflicting_txn) + ", transaction must be retried";
    }
    return {};
}

TransactionManager::Status TransactionManager::Transaction::commit() {
    // RACE CONDITION FIX: Use atomic compare-exchange to prevent double commit
    bool expected = false;
    if (!finished_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return Status::Error("commit: Transaktion bereits abgeschlossen");
    }

    // Capture the actual run duration now that we are the exclusive owner.
    // getDurationMs() returns this frozen value once finished_ is true, so that
    // Stats::avg_duration_ms / max_duration_ms reflect real transaction runtimes
    // rather than "time since start" measured at arbitrary query time.
    captureDuration();

    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("commit: keine aktive Transaktion");
    }

    // Timeout check: refuse commit on an expired transaction
    if (isTimedOut()) {
        THEMIS_WARN("Transaction {} timed out ({} ms), aborting commit", id_, getDurationMs());
        mvcc_txn_->rollback();
        saga_->compensate();
        if (lock_manager_) lock_manager_->releasePredicateLocks(id_);
        return Status::Error("commit: transaction timed out");
    }

    // Read-only fast path: release the snapshot without writing to the WAL.
    if (read_only_) {
        THEMIS_DEBUG("Transaction {} is read-only — skipping WAL write on commit (duration: {} ms)",
                     id_, getDurationMs());
        mvcc_txn_->rollback(); // releases the RocksDB snapshot; no data is written
        if (lock_manager_) lock_manager_->releasePredicateLocks(id_);
        return Status::OK();
    }
    
    THEMIS_DEBUG("Committing MVCC transaction {} with {} SAGA steps (duration: {} ms)", 
                id_, saga_->stepCount(), getDurationMs());
    
    if (!mvcc_txn_->commit()) {
        // Commit failed - MVCC conflict detected
        THEMIS_ERROR("Transaction {} commit failed - MVCC conflict, executing SAGA compensation", id_);

        // Determine the conflict type from the underlying RocksDB failure reason.
        auto failure_type = mvcc_txn_->getLastCommitFailureType();
        std::string conflict_type;
        switch (failure_type) {
            case RocksDBWrapper::TransactionWrapper::CommitFailureType::Busy:
                conflict_type = "busy"; break;
            case RocksDBWrapper::TransactionWrapper::CommitFailureType::TimedOut:
                conflict_type = "timeout"; break;
            case RocksDBWrapper::TransactionWrapper::CommitFailureType::TryAgain:
                conflict_type = "try_again"; break;
            default:
                conflict_type = "commit_error"; break;
        }

        // Build and persist ConflictRecord(s) if a ConflictManager is available.
        std::vector<std::string> conflict_record_ids;
        std::vector<std::string> conflict_keys;
        std::string conflict_set_id;
        if (conflict_mgr_ && !our_values_.empty()) {
            for (const auto& [key, ours] : our_values_) {
                conflict_keys.push_back(key);
                ConflictRecord crec;
                crec.base_key    = key;
                crec.txn_id      = id_;
                crec.type        = conflict_type;
                // base = snapshot value at txn start (captured once per key)
                auto base_it = base_values_.find(key);
                crec.base_value  = (base_it != base_values_.end()) ? base_it->second : std::vector<uint8_t>{};
                crec.ours_value  = ours;
                // theirs = current committed value after conflict
                auto theirs_raw = db_.get(key);
                crec.theirs_value = theirs_raw ? std::move(*theirs_raw) : std::vector<uint8_t>{};
                std::string cid = conflict_mgr_->storeConflict(crec);
                conflict_record_ids.push_back(cid);
            }

            // Persist a ConflictSet grouping all per-key ConflictRecords.
            ConflictSet cset;
            cset.txn_id              = id_;
            cset.conflict_record_ids = conflict_record_ids;
            cset.affected_keys       = conflict_keys;
            conflict_set_id = conflict_mgr_->storeConflictSet(cset);
        }

        saga_->compensate();
        if (lock_manager_) lock_manager_->releasePredicateLocks(id_);

        if (!conflict_set_id.empty()) {
            return Status::Conflict(
                "commit: MVCC conflict detected, transaction must be retried",
                conflict_set_id,
                std::move(conflict_keys)
            );
        }
        return Status::Error("commit: MVCC conflict detected, transaction must be retried");
    }
    
    // Success - clear SAGA (no compensation needed)
    saga_->clear();
    if (lock_manager_) lock_manager_->releasePredicateLocks(id_);
    THEMIS_INFO("Transaction {} committed successfully (MVCC)", id_);
    return Status::OK();
}

void TransactionManager::Transaction::rollback() {
    // RACE CONDITION FIX: Use atomic compare-exchange to prevent double rollback
    bool expected = false;
    if (!finished_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        THEMIS_WARN("Transaction {} already finished, rollback skipped", id_);
        return;
    }

    // Capture the actual run duration now that we are the exclusive owner.
    captureDuration();
    
    THEMIS_DEBUG("Rolling back MVCC transaction {} with {} SAGA steps", id_, saga_->stepCount());
    
    if (mvcc_txn_ && mvcc_txn_->isActive()) {
        mvcc_txn_->rollback();
    }
    
    // Execute SAGA compensation
    saga_->compensate();
    if (lock_manager_) lock_manager_->releasePredicateLocks(id_);
    
    THEMIS_INFO("Transaction {} rolled back, {} steps compensated", id_, saga_->compensatedCount());
}

TransactionManager::Status TransactionManager::Transaction::setSavePoint() {
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("setSavePoint: transaction already finished");
    }
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("setSavePoint: no active transaction");
    }
    mvcc_txn_->setSavePoint();
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::rollbackToSavePoint() {
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("rollbackToSavePoint: transaction already finished");
    }
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("rollbackToSavePoint: no active transaction");
    }
    if (!mvcc_txn_->rollbackToSavePoint()) {
        return Status::Error("rollbackToSavePoint: no savepoint to roll back to");
    }
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::popSavePoint() {
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("popSavePoint: transaction already finished");
    }
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("popSavePoint: no active transaction");
    }
    if (!mvcc_txn_->popSavePoint()) {
        return Status::Error("popSavePoint: no savepoint to pop");
    }
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::createSavepoint(std::string_view name) {
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("createSavepoint: transaction already finished");
    }
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("createSavepoint: no active transaction");
    }
    if (name.empty()) {
        return Status::Error("createSavepoint: savepoint name must not be empty");
    }
    std::string sname(name);
    for (const auto& e : savepoints_) {
        if (e.name == sname) {
            return Status::Error("createSavepoint: savepoint '" + sname + "' already exists");
        }
    }
    // Reserve capacity BEFORE calling setSavePoint() so that push_back below cannot
    // throw std::bad_alloc.  If setSavePoint were called first and push_back threw,
    // the RocksDB savepoint stack would have an extra entry not tracked by savepoints_,
    // corrupting all subsequent savepoint operations.
    savepoints_.reserve(savepoints_.size() + 1);
    mvcc_txn_->setSavePoint();
    savepoints_.push_back({std::move(sname), saga_->stepCount()});
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::rollbackToSavepoint(std::string_view name) {
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("rollbackToSavepoint: transaction already finished");
    }
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("rollbackToSavepoint: no active transaction");
    }
    std::string sname(name);
    auto it = std::find_if(savepoints_.begin(), savepoints_.end(),
                           [&sname](const SavepointEntry& e) { return e.name == sname; });
    if (it == savepoints_.end()) {
        return Status::Error("rollbackToSavepoint: no savepoint named '" + sname + "'");
    }
    // Pop all anonymous savepoints above the target (collapse their write deltas
    // into the target's delta so that the subsequent rollback undoes them all).
    size_t num_above = static_cast<size_t>(savepoints_.end() - it) - 1;
    for (size_t i = 0; i < num_above; ++i) {
        mvcc_txn_->popSavePoint();
    }
    // Rollback to (and pop) the target savepoint itself.
    if (!mvcc_txn_->rollbackToSavePoint()) {
        // The num_above pops above have already been executed — the corresponding
        // entries in savepoints_ no longer have a backing RocksDB savepoint.
        // Remove them so that savepoints_ stays in sync with the RocksDB stack,
        // preventing future rollbackToSavepoint/releaseSavepoint from popping
        // the wrong number of savepoints.
        savepoints_.erase(it + 1, savepoints_.end());
        return Status::Error("rollbackToSavepoint: rollback failed for '" + sname + "'");
    }
    // Discard SAGA steps added after the savepoint was created.
    saga_->trimToSize(it->saga_step_count);
    savepoints_.erase(it, savepoints_.end());
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::releaseSavepoint(std::string_view name) {
    if (finished_.load(std::memory_order_acquire)) {
        return Status::Error("releaseSavepoint: transaction already finished");
    }
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("releaseSavepoint: no active transaction");
    }
    std::string sname(name);
    auto it = std::find_if(savepoints_.begin(), savepoints_.end(),
                           [&sname](const SavepointEntry& e) { return e.name == sname; });
    if (it == savepoints_.end()) {
        return Status::Error("releaseSavepoint: no savepoint named '" + sname + "'");
    }
    // Pop the target savepoint and every savepoint above it (writes are preserved).
    size_t num_to_pop = static_cast<size_t>(savepoints_.end() - it);
    for (size_t i = 0; i < num_to_pop; ++i) {
        mvcc_txn_->popSavePoint();
    }
    savepoints_.erase(it, savepoints_.end());
    return Status::OK();
}

std::vector<std::string> TransactionManager::Transaction::getSavepoints() const {
    std::vector<std::string> names;
    names.reserve(savepoints_.size());
    for (const auto& e : savepoints_) {
        names.push_back(e.name);
    }
    return names;
}

bool TransactionManager::Transaction::hasSavepoint(std::string_view name) const {
    std::string sname(name);
    return std::any_of(savepoints_.begin(), savepoints_.end(),
                       [&sname](const SavepointEntry& e) { return e.name == sname; });
}

// ── Transaction timeout ───────────────────────────────────────────────────────

void TransactionManager::applyDefaultTimeout(Transaction& txn) const {
    uint64_t tms = default_transaction_timeout_ms_.load(std::memory_order_relaxed);
    if (tms > 0) {
        txn.setTimeout(std::chrono::milliseconds(tms));
    }
}

void TransactionManager::setDefaultTransactionTimeout(std::chrono::milliseconds timeout) {
    // Clamp to 0: a negative duration is treated the same as "no timeout".
    auto ms = timeout.count();
    default_transaction_timeout_ms_.store(ms > 0 ? static_cast<uint64_t>(ms) : 0u,
                                          std::memory_order_relaxed);
    THEMIS_INFO("Default transaction timeout set to {} ms", ms > 0 ? ms : 0);
}

std::chrono::milliseconds TransactionManager::getDefaultTransactionTimeout() const {
    return std::chrono::milliseconds(
        default_transaction_timeout_ms_.load(std::memory_order_relaxed));
}

void TransactionManager::timeoutExpiredTransactions() {
    // Collect IDs of timed-out transactions while holding the lock (read-only scan),
    // then release the lock before calling rollbackTransaction() to avoid re-entrancy.
    std::vector<TransactionId> expired;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& [id, txn] : active_transactions_) {
            if (txn && !txn->isFinished() && txn->isTimedOut()) {
                expired.push_back(id);
            }
        }
    }
    for (TransactionId id : expired) {
        THEMIS_WARN("Transaction {} exceeded its timeout — auto-rolling back", id);
        if (rollbackTransaction(id)) {
            // Only count transactions that were actually rolled back by the monitor.
            // If the transaction was already completed by user code between the scan
            // and this call, rollbackTransaction returns false and we must not inflate
            // the counter.
            total_timed_out_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

// ── Phase 8: Durability & Crash-Recovery ─────────────────────────────────────

void TransactionManager::enableCrashRecovery(const std::string& wal_path,
                                              bool sync_on_write) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    crash_recovery_mgr_ = std::make_unique<transaction::CrashRecoveryManager>(
        wal_path, sync_on_write);
    THEMIS_INFO("CrashRecoveryManager enabled (wal_path='{}', sync={})",
                wal_path, sync_on_write);
}

bool TransactionManager::needsCrashRecovery() const {
    if (!crash_recovery_mgr_) return false;
    return crash_recovery_mgr_->needsRecovery();
}

transaction::CrashRecoveryManager::RecoveryResult
TransactionManager::crashRecover() {
    if (!crash_recovery_mgr_) {
        transaction::CrashRecoveryManager::RecoveryResult r;
        r.success = true;
        r.message = "Crash recovery not enabled (call enableCrashRecovery first).";
        return r;
    }
    return crash_recovery_mgr_->recover(db_);
}

// ── Transaction Explain ───────────────────────────────────────────────────────

void TransactionManager::Transaction::trackWrite(std::string key, std::string operation) {
    write_set_.push_back({std::move(key), std::move(operation)});
}

static std::string isolationLevelName(IsolationLevel level) {
    switch (level) {
    case IsolationLevel::READ_UNCOMMITTED: return "READ_UNCOMMITTED";
    case IsolationLevel::READ_COMMITTED:   return "READ_COMMITTED";
    case IsolationLevel::REPEATABLE_READ:  return "REPEATABLE_READ";
    case IsolationLevel::SERIALIZABLE:     return "SERIALIZABLE";
    default:                               return "UNKNOWN";
    }
}

static std::string lockTypeName(LockType t) {
    switch (t) {
    case LockType::SHARED:           return "SHARED";
    case LockType::EXCLUSIVE:        return "EXCLUSIVE";
    case LockType::INTENT_SHARED:    return "INTENT_SHARED";
    case LockType::INTENT_EXCLUSIVE: return "INTENT_EXCLUSIVE";
    default:                         return "UNKNOWN";
    }
}

TransactionManager::Transaction::ExplainResult
TransactionManager::Transaction::explain() const {
    ExplainResult result;
    result.txn_id         = id_;
    result.isolation_level = isolationLevelName(isolation_);
    result.duration_ms    = getDurationMs();
    result.is_finished    = finished_.load(std::memory_order_acquire);

    // Collect locks held via LockManager
    if (lock_manager_) {
        for (const auto& [key, lock_type] : lock_manager_->getLocksHeld(id_)) {
            result.locks_held.push_back({key, lockTypeName(lock_type)});
        }
    }

    // Copy the write set (MVCC version chain entries)
    result.write_set = write_set_;

    return result;
}

std::optional<TransactionManager::Transaction::ExplainResult>
TransactionManager::explainTransaction(TransactionId id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = active_transactions_.find(id);
    if (it != active_transactions_.end() && it->second) {
        return it->second->explain();
    }
    auto cit = completed_transactions_.find(id);
    if (cit != completed_transactions_.end() && cit->second) {
        return cit->second->explain();
    }
    return std::nullopt;
}

// ── Time-travel queries ────────────────────────────────────────────────────────

namespace {
/// Convert a HistoryRecord into a TransactionManager::TimeTravelRecord.
TransactionManager::TimeTravelRecord toTimeTravelRecord(const HistoryRecord& rec) {
    TransactionManager::TimeTravelRecord r;
    r.base_key  = rec.base_key;
    r.timestamp = rec.timestamp;
    r.op        = rec.op;
    r.value     = rec.value;
    r.txn_id    = rec.txn_id;
    return r;
}
} // anonymous namespace

std::optional<TransactionManager::TimeTravelRecord>
TransactionManager::readEntityAtTimestamp(
    std::string_view table,
    std::string_view pk,
    HLCTimestamp ts) const
{
    if (!history_mgr_) return std::nullopt;
    const std::string live_key =
        std::string("entity:") + std::string(table) + ":" + std::string(pk);
    auto rec = history_mgr_->getAtTimestamp(live_key, ts);
    if (!rec) return std::nullopt;
    return toTimeTravelRecord(*rec);
}

std::optional<TransactionManager::TimeTravelRecord>
TransactionManager::readEntityAtUnixMs(
    std::string_view table,
    std::string_view pk,
    int64_t unix_ms) const
{
    if (unix_ms < 0) return std::nullopt;
    // Build an HLC upper-bound: physical = unix_ms, logical = MAX_LOGICAL.
    // Using MAX_LOGICAL ensures this timestamp is strictly greater than any
    // real HLC timestamp recorded at the same wall-clock millisecond (real
    // logical counters are always < MAX_LOGICAL), so getAtTimestamp() will
    // return the latest entry at or before unix_ms as required.
    HLCTimestamp ts = HLCTimestamp::from(
        static_cast<uint64_t>(unix_ms), HLCTimestamp::MAX_LOGICAL);
    return readEntityAtTimestamp(table, pk, ts);
}

std::optional<TransactionManager::TimeTravelRecord>
TransactionManager::readEntityAtSnapshot(
    std::string_view table,
    std::string_view pk,
    const std::string& tag_name) const
{
    if (!snapshot_mgr_) return std::nullopt;
    auto ts_opt = snapshot_mgr_->getTimestampForTag(tag_name);
    if (!ts_opt) return std::nullopt;
    return readEntityAtUnixMs(table, pk, *ts_opt);
}

std::vector<TransactionManager::TimeTravelRecord>
TransactionManager::listEntityVersions(
    std::string_view table,
    std::string_view pk) const
{
    if (!history_mgr_) return {};
    const std::string live_key =
        std::string("entity:") + std::string(table) + ":" + std::string(pk);
    auto records = history_mgr_->listVersions(live_key);
    std::vector<TimeTravelRecord> result;
    result.reserve(records.size());
    for (const auto& rec : records) {
        result.push_back(toTimeTravelRecord(rec));
    }
    return result;
}

// ── Adaptive Deadlock Prevention (v1.9.0) ────────────────────────────────────

void TransactionManager::setDeadlockPredictor(DeadlockPredictor* predictor) {
    // Release store so that any writes made to *predictor before this call are
    // visible to threads that subsequently load the pointer with acquire order.
    deadlock_predictor_.store(predictor, std::memory_order_release);
}

DeadlockPredictor* TransactionManager::getDeadlockPredictor() const {
    return deadlock_predictor_.load(std::memory_order_acquire);
}

double TransactionManager::predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks) const
{
    DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
    if (!p || proposed_locks.empty()) {
        return 0.0;
    }

    // Build the set of currently active transaction IDs.
    std::set<DeadlockPredictor::TransactionId> active_ids;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& [id, _] : active_transactions_) {
            active_ids.insert(id);
        }
    }
    return p->predictDeadlockProbability(proposed_locks, active_ids);
}

std::vector<std::string> TransactionManager::recommendLockOrder(
        const std::vector<std::string>& keys) const
{
    DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
    if (!p) {
        // Fall back to lexicographic order for determinism.
        std::vector<std::string> sorted = keys;
        std::sort(sorted.begin(), sorted.end());
        return sorted;
    }
    return p->recommendLockOrder(keys);
}

std::chrono::milliseconds TransactionManager::recommendTimeout(
        const std::vector<std::string>& keys) const
{
    DeadlockPredictor* p = deadlock_predictor_.load(std::memory_order_acquire);
    if (!p) {
        return std::chrono::milliseconds(
            deadlock_timeout_ms_.load(std::memory_order_relaxed));
    }
    return p->recommendTimeout(keys);
}

// ── Serializable Snapshot Isolation (SSI) ─────────────────────────────────────

void TransactionManager::setSSIConfig(const SSIConfig& config) {
    std::lock_guard<std::mutex> lock(ssi_config_mutex_);
    ssi_config_ = config;
    // Propagate settings to the shared LockManager.
    lock_manager_.setPredicateLockingEnabled(config.enable_predicate_locking);
    lock_manager_.setMaxPredicateLocks(config.enable_predicate_locking
                                       ? config.max_predicate_locks
                                       : 0);
    THEMIS_INFO("SSIConfig updated: enable_predicate_locking={}, max_predicate_locks={}, "
                "conflict_detection_interval={}ms",
                config.enable_predicate_locking,
                config.max_predicate_locks,
                config.conflict_detection_interval.count());
}

TransactionManager::SSIConfig TransactionManager::getSSIConfig() const {
    std::lock_guard<std::mutex> lock(ssi_config_mutex_);
    return ssi_config_;
}

std::vector<TransactionManager::SerializationConflict>
TransactionManager::detectConflicts(TransactionId txn_id) const
{
    // Read current config so we can honour enable_predicate_locking.
    SSIConfig cfg;
    {
        std::lock_guard<std::mutex> cfgLock(ssi_config_mutex_);
        cfg = ssi_config_;
    }

    std::vector<SerializationConflict> result;

    if (!cfg.enable_predicate_locking) {
        return result;
    }

    // Fetch the target transaction.
    std::shared_ptr<Transaction> target_txn;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = active_transactions_.find(txn_id);
        if (it == active_transactions_.end()) {
            return result; // transaction not found or already completed
        }
        target_txn = it->second;
    }

    // Only SERIALIZABLE transactions maintain predicate locks.
    if (target_txn->isolation_ != IsolationLevel::SERIALIZABLE) {
        return result;
    }

    // Retrieve all predicate-lock ranges owned by txn_id.
    auto my_ranges = lock_manager_.getPredicateLockRanges(txn_id);
    if (my_ranges.empty()) {
        return result;
    }

    // Collect other active SERIALIZABLE transactions and their predicate ranges.
    std::vector<std::pair<TransactionId,
                          std::vector<std::pair<std::string, std::string>>>>
        others;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& [id, txn] : active_transactions_) {
            if (id != txn_id && txn &&
                txn->isolation_ == IsolationLevel::SERIALIZABLE)
            {
                others.emplace_back(id, lock_manager_.getPredicateLockRanges(id));
            }
        }
    }

    // For each of our ranges, check for overlap with every range of every other
    // active SERIALIZABLE transaction.  Overlapping predicate ranges signal a
    // potential read-write conflict: both transactions have read overlapping key
    // sets, so a write by either could violate serializability.
    //
    // Two ranges [s1, e1] and [s2, e2] overlap iff s1 <= e2 && s2 <= e1.
    for (const auto& [s1, e1] : my_ranges) {
        for (const auto& [other_id, other_ranges] : others) {
            for (const auto& [s2, e2] : other_ranges) {
                if (s1 <= e2 && s2 <= e1) {
                    SerializationConflict sc;
                    sc.other_txn_id  = other_id;
                    sc.key           = s1;  // representative key (start of our range)
                    sc.conflict_type = "read-write";
                    sc.message       = "predicate lock [" + s1 + ", " + e1 +
                                       "] held by txn " + std::to_string(txn_id) +
                                       " overlaps with predicate lock [" + s2 + ", " +
                                       e2 + "] held by txn " +
                                       std::to_string(other_id) +
                                       "; serialization failure – transaction must be retried";
                    result.push_back(std::move(sc));
                    // Report one conflict per (our_range, other_txn) pair to
                    // avoid flooding the caller with duplicates.
                    break;
                }
            }
        }
    }

    return result;
}

} // namespace themis


