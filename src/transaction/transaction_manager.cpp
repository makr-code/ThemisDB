#include "transaction/transaction_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/saga.h"
#include "utils/logger.h"
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
    
    // Choose victim: youngest transaction (highest ID)
    TransactionId victim_id = *std::max_element(cycle.begin(), cycle.end());
    
    THEMIS_WARN("Deadlock resolution: aborting transaction {} (youngest in cycle)", victim_id);
    
    // Record deadlock info
    DeadlockInfo info;
    info.cycle = cycle;
    info.detected_at = std::chrono::system_clock::now();
    info.victim_id = victim_id;
    
    {
        std::lock_guard<std::mutex> lock(lock_tracking_mutex_);
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
}

// Session-based transaction management
TransactionManager::TransactionId TransactionManager::generateTransactionId() {
    return next_transaction_id_.fetch_add(1, std::memory_order_relaxed);
}

TransactionManager::TransactionId TransactionManager::beginTransaction(IsolationLevel isolation) {
    auto txn_id = generateTransactionId();
    auto txn = std::make_shared<Transaction>(txn_id, db_, secIdx_, graphIdx_, vecIdx_, isolation);
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        active_transactions_[txn_id] = txn;
    }
    
    // SOLUTION 2B: Update statistics with sequence lock
    updateStatsWithSeqLock([this]() {
        total_begun_.fetch_add(1, std::memory_order_relaxed);
    });
    THEMIS_INFO("Transaction {} begun (isolation: {})", txn_id, 
               isolation == IsolationLevel::ReadCommitted ? "ReadCommitted" : "Snapshot");
    
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
        THEMIS_INFO("Transaction {} committed (duration: {} ms)", id, txn->getDurationMs());
    } else {
        // SOLUTION 2B: Update statistics with sequence lock
        updateStatsWithSeqLock([this]() {
            total_aborted_.fetch_add(1, std::memory_order_relaxed);
        });
        THEMIS_WARN("Transaction {} commit failed: {}", id, status.message);
    }
    
    moveToCompleted(id);
    return status;
}

void TransactionManager::rollbackTransaction(TransactionId id) {
    std::shared_ptr<Transaction> txn;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = active_transactions_.find(id);
        if (it == active_transactions_.end()) {
            return;  // Already completed or doesn't exist
        }
        txn = it->second;
    }
    
    txn->rollback();
    // SOLUTION 2B: Update statistics with sequence lock
    updateStatsWithSeqLock([this]() {
        total_aborted_.fetch_add(1, std::memory_order_relaxed);
    });
    THEMIS_INFO("Transaction {} rolled back (duration: {} ms)", id, txn->getDurationMs());
    
    moveToCompleted(id);
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

// Direct transaction (legacy API)
TransactionManager::Transaction TransactionManager::begin(IsolationLevel isolation) {
    auto txn_id = generateTransactionId();
    // SOLUTION 2B: Update statistics with sequence lock
    updateStatsWithSeqLock([this]() {
        total_begun_.fetch_add(1, std::memory_order_relaxed);
    });
    return Transaction(txn_id, db_, secIdx_, graphIdx_, vecIdx_, isolation);
}

// ==== Transaction ==== 

TransactionManager::Transaction::Transaction(TransactionId id,
                                             RocksDBWrapper& db,
                                             SecondaryIndexManager& secIdx,
                                             GraphIndexManager& graphIdx,
                                             VectorIndexManager& vecIdx,
                                             IsolationLevel isolation)
    : id_(id), db_(db), secIdx_(secIdx), graphIdx_(graphIdx), vecIdx_(vecIdx), isolation_(isolation),
      start_time_(std::chrono::system_clock::now()) {
    mvcc_txn_ = db_.beginTransaction();
    if (!mvcc_txn_) {
        throw std::runtime_error("Failed to create MVCC transaction");
    }
    saga_ = std::make_unique<Saga>();
    THEMIS_INFO("Transaction {} initialized with MVCC and SAGA support (isolation: {})", 
               id_, isolation_ == IsolationLevel::Snapshot ? "Snapshot" : "ReadCommitted");
}

TransactionManager::Transaction::~Transaction() {
    // Use atomic load to check if finished
    if (!finished_.load(std::memory_order_acquire) && mvcc_txn_ && mvcc_txn_->isActive()) {
        THEMIS_WARN("Transaction {} destructed without commit/rollback; rolling back implicitly", id_);
        mvcc_txn_->rollback();
        saga_->compensate();
    }
}

uint64_t TransactionManager::Transaction::getDurationMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return duration.count();
}

TransactionManager::Transaction::Transaction(Transaction&& other) noexcept
    : id_(other.id_), db_(other.db_), secIdx_(other.secIdx_), graphIdx_(other.graphIdx_), 
      vecIdx_(other.vecIdx_), isolation_(other.isolation_), start_time_(other.start_time_),
      mvcc_txn_(std::move(other.mvcc_txn_)), saga_(std::move(other.saga_)), 
      finished_(other.finished_.load(std::memory_order_acquire)) {
    other.finished_.store(true, std::memory_order_release);
}

TransactionManager::Transaction& TransactionManager::Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (!finished_.load(std::memory_order_acquire) && mvcc_txn_ && mvcc_txn_->isActive()) {
            mvcc_txn_->rollback();
            saga_->compensate();
        }
        // Anmerkung: db_, secIdx_, graphIdx_ sind Referenzen und können nicht erneut gebunden werden.
        // Diese werden in Konstruktor initialisiert und bleiben über Lebensdauer konstant.
        mvcc_txn_ = std::move(other.mvcc_txn_);
        saga_ = std::move(other.saga_);
        finished_.store(other.finished_.load(std::memory_order_acquire), std::memory_order_release);
        other.finished_.store(true, std::memory_order_release);
    }
    return *this;
}

TransactionManager::Status TransactionManager::Transaction::putEntity(std::string_view table, const BaseEntity& entity) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("putEntity: keine aktive Transaktion");
    
    // Serialize entity
    auto serialized = entity.serialize();
    std::string key = std::string("entity:") + std::string(table) + ":" + entity.getPrimaryKey();
    
    // Write to MVCC transaction
    if (!mvcc_txn_->put(key, serialized)) {
        return Status::Error("putEntity: MVCC conflict detected");
    }
    
    // Update secondary indexes using MVCC transaction for atomicity
    auto st = secIdx_.put(table, entity, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::eraseEntity(std::string_view table, std::string_view pk) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("eraseEntity: keine aktive Transaktion");
    
    std::string key = std::string("entity:") + std::string(table) + ":" + std::string(pk);
    
    // Delete from MVCC transaction
    if (!mvcc_txn_->del(key)) {
        return Status::Error("eraseEntity: MVCC conflict detected");
    }
    
    // Update secondary indexes using MVCC transaction
    auto st = secIdx_.erase(table, pk, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::addEdge(const BaseEntity& edgeEntity) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("addEdge: keine aktive Transaktion");
    
    // Graph edges stored with MVCC
    std::string edge_key = "graph:edge:" + edgeEntity.getPrimaryKey();
    auto serialized = edgeEntity.serialize();
    
    if (!mvcc_txn_->put(edge_key, serialized)) {
        return Status::Error("addEdge: MVCC conflict detected");
    }
    
    // Update graph index using MVCC transaction
    auto st = graphIdx_.addEdge(edgeEntity, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::deleteEdge(std::string_view edgeId) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("deleteEdge: keine aktive Transaktion");
    
    std::string edge_key = "graph:edge:" + std::string(edgeId);
    
    if (!mvcc_txn_->del(edge_key)) {
        return Status::Error("deleteEdge: MVCC conflict detected");
    }
    
    // Update graph index using MVCC transaction
    auto st = graphIdx_.deleteEdge(edgeId, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::addVector(const BaseEntity& entity, std::string_view vectorField) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("addVector: keine aktive Transaktion");
    
    // Add SAGA compensating action for vector cache
    std::string pk = entity.getPrimaryKey();
    saga_->addStep("vectorAdd:" + pk, [this, pk]() {
        auto status = vecIdx_.removeByPk(pk);
        if (!status.ok) {
            THEMIS_WARN("SAGA: Vector remove compensation failed for '{}': {}", pk, status.message);
        }
    });
    
    // Store vector entity in MVCC transaction
    std::string vector_key = "vector:" + pk;
    auto serialized = entity.serialize();
    if (!mvcc_txn_->put(vector_key, serialized)) {
        return Status::Error("addVector: MVCC conflict detected");
    }
    
    // Update vector index using MVCC transaction
    auto st = vecIdx_.addEntity(entity, *mvcc_txn_, vectorField);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::updateVector(const BaseEntity& entity, std::string_view vectorField) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("updateVector: keine aktive Transaktion");
    
    // Capture old vector for compensation (MVCC: read before write)
    std::string pk = entity.getPrimaryKey();
    std::string vector_key = "vector:" + pk;
    
    auto old_data = mvcc_txn_->get(vector_key);
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
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::removeVector(std::string_view pk) {
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) return Status::Error("removeVector: keine aktive Transaktion");
    
    // Capture old vector before removal for compensation
    std::string pk_str(pk);
    std::string vector_key = "vector:" + pk_str;
    
    auto old_data = mvcc_txn_->get(vector_key);
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
    
    // Delete vector entity from MVCC transaction
    if (!mvcc_txn_->del(vector_key)) {
        return Status::Error("removeVector: MVCC conflict detected");
    }
    
    // Update vector index using MVCC transaction
    auto st = vecIdx_.removeByPk(pk, *mvcc_txn_);
    if (!st.ok) {
        return Status::Error(st.message);
    }
    
    return Status::OK();
}

TransactionManager::Status TransactionManager::Transaction::commit() {
    // RACE CONDITION FIX: Use atomic compare-exchange to prevent double commit
    bool expected = false;
    if (!finished_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return Status::Error("commit: Transaktion bereits abgeschlossen");
    }
    
    if (!mvcc_txn_ || !mvcc_txn_->isActive()) {
        return Status::Error("commit: keine aktive Transaktion");
    }
    
    THEMIS_DEBUG("Committing MVCC transaction {} with {} SAGA steps (duration: {} ms)", 
                id_, saga_->stepCount(), getDurationMs());
    
    if (!mvcc_txn_->commit()) {
        // Commit failed - MVCC conflict detected
        THEMIS_ERROR("Transaction {} commit failed - MVCC conflict, executing SAGA compensation", id_);
        saga_->compensate();
        return Status::Error("commit: MVCC conflict detected, transaction must be retried");
    }
    
    // Success - clear SAGA (no compensation needed)
    saga_->clear();
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
    
    THEMIS_DEBUG("Rolling back MVCC transaction {} with {} SAGA steps", id_, saga_->stepCount());
    
    if (mvcc_txn_ && mvcc_txn_->isActive()) {
        mvcc_txn_->rollback();
    }
    
    // Execute SAGA compensation
    saga_->compensate();
    
    THEMIS_INFO("Transaction {} rolled back, {} steps compensated", id_, saga_->compensatedCount());
}

} // namespace themis
