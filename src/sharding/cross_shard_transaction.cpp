// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/cross_shard_transaction.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>

namespace themisdb {
namespace sharding {

CrossShardTransactionCoordinator::CrossShardTransactionCoordinator(
    const CrossShardTransactionConfig& config,
    std::shared_ptr<ConsensusModule> consensus
)
    : config_(config)
    , consensus_(consensus)
    , running_(false)
    , total_transactions_(0)
    , committed_transactions_(0)
    , aborted_transactions_(0)
    , deadlocked_transactions_(0)
{
}

CrossShardTransactionCoordinator::~CrossShardTransactionCoordinator() {
    stop();
}

bool CrossShardTransactionCoordinator::initialize() {
    if (!consensus_) {
        spdlog::error("Consensus module required for cross-shard transactions");
        return false;
    }
    
    spdlog::info("Cross-shard transaction coordinator initialized");
    return true;
}

bool CrossShardTransactionCoordinator::start() {
    if (running_.load()) {
        spdlog::warn("Cross-shard transaction coordinator already running");
        return false;
    }
    
    running_.store(true);
    
    // Start deadlock detection thread if enabled
    if (config_.enable_deadlock_detection) {
        deadlock_detection_thread_ = std::thread(
            &CrossShardTransactionCoordinator::deadlockDetectionThread, this
        );
    }
    
    spdlog::info("Cross-shard transaction coordinator started");
    return true;
}

void CrossShardTransactionCoordinator::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (deadlock_detection_thread_.joinable()) {
        deadlock_detection_thread_.join();
    }
    
    spdlog::info("Cross-shard transaction coordinator stopped");
}

bool CrossShardTransactionCoordinator::beginTransaction(
    const std::string& transaction_id,
    TransactionProtocol protocol,
    IsolationLevel isolation_level
) {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    // Check if transaction already exists
    if (transactions_.find(transaction_id) != transactions_.end()) {
        spdlog::warn("Transaction {} already exists", transaction_id);
        return false;
    }
    
    // Create new transaction
    CrossShardTransaction txn;
    txn.transaction_id = transaction_id;
    txn.protocol = protocol;
    txn.isolation_level = isolation_level;
    txn.state = TransactionState::ACTIVE;
    txn.start_time = std::chrono::system_clock::now();
    
    transactions_[transaction_id] = txn;
    total_transactions_++;
    
    // Replicate transaction metadata via consensus
    if (consensus_) {
        nlohmann::json data = {
            {"transaction_id", transaction_id},
            {"protocol", static_cast<int>(protocol)},
            {"isolation_level", static_cast<int>(isolation_level)},
            {"state", static_cast<int>(TransactionState::ACTIVE)}
        };
        
        consensus_->propose("BEGIN_TRANSACTION", data);
    }
    
    spdlog::info("Transaction {} started with protocol {}", 
                 transaction_id, static_cast<int>(protocol));
    return true;
}

bool CrossShardTransactionCoordinator::addParticipant(
    const std::string& transaction_id,
    const std::string& shard_id,
    const std::string& endpoint,
    const std::vector<std::string>& operations
) {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        spdlog::error("Transaction {} is not active", transaction_id);
        return false;
    }
    
    // Add participant
    ShardParticipant participant;
    participant.shard_id = shard_id;
    participant.endpoint = endpoint;
    participant.operations = operations;
    
    txn.participants[shard_id] = participant;
    
    spdlog::debug("Added participant {} to transaction {}", shard_id, transaction_id);
    return true;
}

bool CrossShardTransactionCoordinator::prepare(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        spdlog::error("Transaction {} is not active", transaction_id);
        return false;
    }
    
    txn.state = TransactionState::PREPARING;
    lock.unlock();
    
    // Send prepare requests to all participants
    bool all_prepared = true;
    for (auto& [shard_id, participant] : txn.participants) {
        bool prepared = sendPrepare(shard_id, transaction_id);
        
        lock.lock();
        participant.prepared = prepared;
        lock.unlock();
        
        if (!prepared) {
            all_prepared = false;
            participant.error_message = "Prepare failed";
            spdlog::error("Prepare failed for shard {} in transaction {}", 
                         shard_id, transaction_id);
        }
    }
    
    lock.lock();
    if (all_prepared) {
        txn.state = TransactionState::PREPARED;
        spdlog::info("Transaction {} prepared successfully", transaction_id);
    } else {
        txn.state = TransactionState::ACTIVE;  // Roll back to active
    }
    lock.unlock();
    
    // Replicate prepare state via consensus
    if (consensus_ && all_prepared) {
        consensus_->propose("PREPARE_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(TransactionState::PREPARED)}
        });
    }
    
    return all_prepared;
}

bool CrossShardTransactionCoordinator::commit(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    
    // Execute protocol-specific commit
    bool success = false;
    lock.unlock();
    
    switch (txn.protocol) {
        case TransactionProtocol::TWO_PHASE_COMMIT:
            success = execute2PC(txn);
            break;
        case TransactionProtocol::THREE_PHASE_COMMIT:
            success = execute3PC(txn);
            break;
        case TransactionProtocol::PERCOLATOR:
            success = executePercolator(txn);
            break;
        case TransactionProtocol::SAGA:
            // SAGA commit handled by executeSaga
            spdlog::error("SAGA transactions should use executeSaga method");
            return false;
        default:
            spdlog::error("Unknown transaction protocol");
            return false;
    }
    
    lock.lock();
    if (success) {
        txn.state = TransactionState::COMMITTED;
        txn.end_time = std::chrono::system_clock::now();
        committed_transactions_++;
        spdlog::info("Transaction {} committed successfully", transaction_id);
    } else {
        txn.state = TransactionState::ABORTED;
        txn.end_time = std::chrono::system_clock::now();
        aborted_transactions_++;
        spdlog::error("Transaction {} commit failed, aborted", transaction_id);
    }
    lock.unlock();
    
    // Replicate final state via consensus
    if (consensus_) {
        consensus_->propose("FINALIZE_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(txn.state)}
        });
    }
    
    return success;
}

bool CrossShardTransactionCoordinator::abort(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    txn.state = TransactionState::ABORTING;
    lock.unlock();
    
    // Send abort requests to all participants
    for (auto& [shard_id, participant] : txn.participants) {
        sendAbort(shard_id, transaction_id);
        participant.aborted = true;
    }
    
    lock.lock();
    txn.state = TransactionState::ABORTED;
    txn.end_time = std::chrono::system_clock::now();
    aborted_transactions_++;
    lock.unlock();
    
    // Replicate abort state via consensus
    if (consensus_) {
        consensus_->propose("ABORT_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(TransactionState::ABORTED)}
        });
    }
    
    spdlog::info("Transaction {} aborted", transaction_id);
    return true;
}

bool CrossShardTransactionCoordinator::executeSaga(
    const std::string& transaction_id,
    const std::vector<nlohmann::json>& steps,
    const std::vector<nlohmann::json>& compensations
) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.protocol != TransactionProtocol::SAGA) {
        spdlog::error("Transaction {} is not a SAGA transaction", transaction_id);
        return false;
    }
    
    lock.unlock();
    
    // Execute steps sequentially
    size_t completed_steps = 0;
    for (size_t i = 0; i < steps.size(); ++i) {
        (void)steps[i];  // Step would be used in full implementation
        
        // TODO: Complete implementation
        // Execute step - should send operation to appropriate shard via RPC
        // and wait for result, respecting saga_step_timeout
        bool success = true;  // Placeholder - assumes step succeeds
        
        if (!success) {
            spdlog::error("SAGA step {} failed, executing compensation", i);
            
            // Execute compensations for completed steps in reverse order
            for (int j = static_cast<int>(completed_steps) - 1; j >= 0; --j) {
                (void)compensations[j];  // Compensation would be used in full implementation
                // Execute compensation (simplified)
                spdlog::info("Executing compensation for step {}", j);
            }
            
            abort(transaction_id);
            return false;
        }
        
        completed_steps++;
    }
    
    // All steps completed successfully
    lock.lock();
    txn.state = TransactionState::COMMITTED;
    txn.end_time = std::chrono::system_clock::now();
    committed_transactions_++;
    lock.unlock();
    
    spdlog::info("SAGA transaction {} completed successfully", transaction_id);
    return true;
}

std::optional<TransactionState> CrossShardTransactionCoordinator::getTransactionState(
    const std::string& transaction_id
) const {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second.state;
}

std::optional<CrossShardTransaction> CrossShardTransactionCoordinator::getTransaction(
    const std::string& transaction_id
) const {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool CrossShardTransactionCoordinator::isDeadlocked(
    const std::string& transaction_id
) const {
    // Simplified deadlock check
    // In production, this would check the wait-for graph
    return false;
}

std::vector<CrossShardTransaction> CrossShardTransactionCoordinator::getActiveTransactions() const {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    std::vector<CrossShardTransaction> active;
    for (const auto& [id, txn] : transactions_) {
        if (txn.state == TransactionState::ACTIVE ||
            txn.state == TransactionState::PREPARING ||
            txn.state == TransactionState::PREPARED) {
            active.push_back(txn);
        }
    }
    
    return active;
}

nlohmann::json CrossShardTransactionCoordinator::getStatistics() const {
    return {
        {"total_transactions", total_transactions_.load()},
        {"committed_transactions", committed_transactions_.load()},
        {"aborted_transactions", aborted_transactions_.load()},
        {"deadlocked_transactions", deadlocked_transactions_.load()},
        {"active_transactions", getActiveTransactions().size()}
    };
}

void CrossShardTransactionCoordinator::onTransactionStateChange(
    std::function<void(const std::string&, TransactionState, TransactionState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

// Private methods

bool CrossShardTransactionCoordinator::execute2PC(CrossShardTransaction& txn) {
    // Phase 1: Prepare (already done)
    if (txn.state != TransactionState::PREPARED) {
        if (!prepare(txn.transaction_id)) {
            return false;
        }
    }
    
    // Phase 2: Commit
    txn.state = TransactionState::COMMITTING;
    
    bool all_committed = true;
    for (auto& [shard_id, participant] : txn.participants) {
        bool committed = sendCommit(shard_id, txn.transaction_id);
        participant.committed = committed;
        
        if (!committed) {
            all_committed = false;
            spdlog::error("Commit failed for shard {} in transaction {}", 
                         shard_id, txn.transaction_id);
        }
    }
    
    return all_committed;
}

bool CrossShardTransactionCoordinator::execute3PC(CrossShardTransaction& txn) {
    // 3PC adds a pre-commit phase to avoid blocking
    // Simplified implementation
    return execute2PC(txn);
}

bool CrossShardTransactionCoordinator::executePercolator(CrossShardTransaction& txn) {
    // Percolator uses optimistic concurrency with locks
    // Simplified implementation
    return execute2PC(txn);
}

bool CrossShardTransactionCoordinator::sendPrepare(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    // TODO: Complete implementation
    // Placeholder - should use ShardRPCClient to send prepare request
    // and wait for response, respecting prepare_timeout
    spdlog::debug("Sending prepare to shard {} for transaction {}", 
                  shard_id, transaction_id);
    return true;  // Placeholder - assumes success
}

bool CrossShardTransactionCoordinator::sendCommit(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    // TODO: Complete implementation
    // Placeholder - should use ShardRPCClient to send commit request
    // and wait for response, respecting commit_timeout
    spdlog::debug("Sending commit to shard {} for transaction {}", 
                  shard_id, transaction_id);
    return true;  // Placeholder - assumes success
}

bool CrossShardTransactionCoordinator::sendAbort(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    // TODO: Complete implementation
    // Placeholder - should use ShardRPCClient to send abort request
    // and wait for response, respecting abort_timeout
    spdlog::debug("Sending abort to shard {} for transaction {}", 
                  shard_id, transaction_id);
    return true;  // Placeholder - assumes success
}

void CrossShardTransactionCoordinator::deadlockDetectionThread() {
    spdlog::debug("Deadlock detection thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(config_.deadlock_detection_interval);
        
        // Build wait-for graph
        auto graph = buildWaitForGraph();
        
        // Detect cycles
        std::set<std::string> visited;
        std::set<std::string> rec_stack;
        
        for (const auto& [node, _] : graph) {
            if (detectCycle(graph, node, visited, rec_stack)) {
                spdlog::warn("Deadlock detected involving transaction {}", node);
                deadlocked_transactions_++;
                
                // Abort youngest transaction in cycle
                abort(node);
            }
        }
    }
    
    spdlog::debug("Deadlock detection thread stopped");
}

std::map<std::string, std::vector<std::string>> 
CrossShardTransactionCoordinator::buildWaitForGraph() const {
    std::map<std::string, std::vector<std::string>> graph;
    
    // Placeholder - would build actual wait-for graph from lock information
    
    return graph;
}

bool CrossShardTransactionCoordinator::detectCycle(
    const std::map<std::string, std::vector<std::string>>& graph,
    const std::string& start_node,
    std::set<std::string>& visited,
    std::set<std::string>& rec_stack
) {
    if (rec_stack.find(start_node) != rec_stack.end()) {
        return true;  // Cycle detected
    }
    
    if (visited.find(start_node) != visited.end()) {
        return false;  // Already visited, no cycle from here
    }
    
    visited.insert(start_node);
    rec_stack.insert(start_node);
    
    auto it = graph.find(start_node);
    if (it != graph.end()) {
        for (const auto& neighbor : it->second) {
            if (detectCycle(graph, neighbor, visited, rec_stack)) {
                return true;
            }
        }
    }
    
    rec_stack.erase(start_node);
    return false;
}

} // namespace sharding
} // namespace themisdb
