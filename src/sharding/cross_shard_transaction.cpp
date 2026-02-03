// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/cross_shard_transaction.h"
#include "sharding/shard_rpc_client.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>
#include <fstream>
#include <thread>
#include <chrono>

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
    , transaction_log_path_("/tmp/themisdb_transaction_log.jsonl")
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
    
    // Attempt to recover from any previous coordinator failure
    if (!recoverFromFailure()) {
        spdlog::error("Failed to recover from previous coordinator failure");
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
    
    // Persist transaction state
    persistTransactionState(transaction_id, TransactionState::ACTIVE);
    
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
    persistTransactionState(transaction_id, TransactionState::PREPARING);
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
        persistTransactionState(transaction_id, TransactionState::PREPARED);
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
        persistTransactionState(transaction_id, TransactionState::COMMITTED);
        spdlog::info("Transaction {} committed successfully", transaction_id);
    } else {
        txn.state = TransactionState::ABORTED;
        txn.end_time = std::chrono::system_clock::now();
        aborted_transactions_++;
        persistTransactionState(transaction_id, TransactionState::ABORTED);
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
    persistTransactionState(transaction_id, TransactionState::ABORTING);
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
    persistTransactionState(transaction_id, TransactionState::ABORTED);
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
    
    if (steps.size() != compensations.size()) {
        spdlog::error("SAGA transaction {} has mismatched steps ({}) and compensations ({})", 
                     transaction_id, steps.size(), compensations.size());
        return false;
    }
    
    lock.unlock();
    
    spdlog::info("Executing SAGA transaction {} with {} steps", 
                transaction_id, steps.size());
    
    // Execute steps sequentially
    size_t completed_steps = 0;
    std::vector<nlohmann::json> executed_steps;
    
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        
        // Extract shard_id and operation from step
        if (!step.contains("shard_id") || !step.contains("operation")) {
            spdlog::error("SAGA step {} missing shard_id or operation", i);
            
            // Execute compensations for completed steps
            executeCompensations(transaction_id, executed_steps, compensations);
            
            return false;
        }
        
        std::string shard_id = step["shard_id"];
        nlohmann::json operation = step["operation"];
        
        spdlog::info("Executing SAGA step {} on shard {} for transaction {}", 
                    i, shard_id, transaction_id);
        
        // Execute step - send operation to shard via RPC
        try {
            lock.lock();
            auto participant_it = txn.participants.find(shard_id);
            if (participant_it == txn.participants.end()) {
                lock.unlock();
                spdlog::error("Shard {} not found in transaction {} participants", 
                            shard_id, transaction_id);
                
                // Execute compensations
                executeCompensations(transaction_id, executed_steps, compensations);
                
                return false;
            }
            
            auto& participant = participant_it->second;
            std::string endpoint = participant.endpoint;
            lock.unlock();
            
            // Create RPC client for this shard
            themis::sharding::ShardRPCClient::Config rpc_config;
            rpc_config.endpoint = endpoint;
            rpc_config.timeout_ms = static_cast<int>(config_.saga_step_timeout.count());
            rpc_config.max_retries = 2;  // SAGA steps should be idempotent
            rpc_config.retry_delay_ms = 100;
            
            themis::sharding::ShardRPCClient rpc_client(rpc_config);
            
            // Execute the step with timeout
            auto step_start = std::chrono::steady_clock::now();
            bool success = false;
            
            // For SAGA, we use a simplified execution model
            // In production, this would be a specific SAGA operation RPC
            nlohmann::json operations = nlohmann::json::array();
            operations.push_back(operation);
            
            // Try to execute the step
            int retries = 0;
            while (retries <= rpc_config.max_retries) {
                try {
                    // Check timeout
                    auto elapsed = std::chrono::steady_clock::now() - step_start;
                    if (elapsed > config_.saga_step_timeout) {
                        spdlog::error("SAGA step {} timed out after {}ms", 
                                    i, 
                                    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                        break;
                    }
                    
                    // Execute step (using prepare as proxy for step execution)
                    success = rpc_client.prepare(transaction_id + "_step_" + std::to_string(i), 
                                                operations);
                    
                    if (success) {
                        break;
                    }
                    
                } catch (const std::exception& e) {
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("SAGA step {} execution failed (attempt {}/{}): {}. Retrying", 
                                   i, retries + 1, rpc_config.max_retries + 1, e.what());
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                        retries++;
                    } else {
                        spdlog::error("SAGA step {} execution failed after {} retries: {}", 
                                    i, rpc_config.max_retries, e.what());
                        break;
                    }
                }
            }
            
            if (!success) {
                spdlog::error("SAGA step {} failed, executing compensations", i);
                
                // Execute compensations for completed steps in reverse order
                executeCompensations(transaction_id, executed_steps, compensations);
                
                abort(transaction_id);
                return false;
            }
            
            executed_steps.push_back(step);
            completed_steps++;
            
            spdlog::info("SAGA step {} completed successfully", i);
            
        } catch (const std::exception& e) {
            spdlog::error("SAGA step {} failed with exception: {}", i, e.what());
            
            // Execute compensations
            executeCompensations(transaction_id, executed_steps, compensations);
            
            abort(transaction_id);
            return false;
        }
    }
    
    // All steps completed successfully
    lock.lock();
    txn.state = TransactionState::COMMITTED;
    txn.end_time = std::chrono::system_clock::now();
    committed_transactions_++;
    lock.unlock();
    
    spdlog::info("SAGA transaction {} completed successfully with {} steps", 
                transaction_id, completed_steps);
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
    // Build wait-for graph
    auto graph = buildWaitForGraph();
    
    // Check if transaction_id is part of a cycle
    std::set<std::string> visited;
    std::set<std::string> rec_stack;
    
    // Start DFS from the given transaction
    if (graph.find(transaction_id) != graph.end()) {
        return detectCycle(graph, transaction_id, visited, rec_stack);
    }
    
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
    // Three-Phase Commit Protocol
    // Phase 1: Prepare (CanCommit)
    // Phase 2: PreCommit
    // Phase 3: DoCommit
    
    // Phase 1: Prepare - check if all participants can commit
    if (txn.state != TransactionState::PREPARED) {
        if (!prepare(txn.transaction_id)) {
            spdlog::error("3PC Phase 1 (Prepare) failed for transaction {}", 
                         txn.transaction_id);
            return false;
        }
    }
    
    // Phase 2: PreCommit - participants write to stable storage but don't commit
    txn.state = TransactionState::COMMITTING;  // Using COMMITTING state for PreCommit phase
    
    spdlog::info("3PC Phase 2 (PreCommit) starting for transaction {}", 
                txn.transaction_id);
    
    bool all_precommitted = true;
    for (auto& [shard_id, participant] : txn.participants) {
        // Send PreCommit message to each participant
        // In a full implementation, this would be a separate RPC call
        // For now, we'll log the intent and mark as precommitted
        spdlog::debug("Sending PreCommit to shard {} for transaction {}", 
                     shard_id, txn.transaction_id);
        
        // In production, you would:
        // bool precommitted = sendPreCommit(shard_id, txn.transaction_id);
        // For now, we assume precommit succeeds if prepare succeeded
        bool precommitted = participant.prepared;
        
        if (!precommitted) {
            all_precommitted = false;
            spdlog::error("PreCommit failed for shard {} in transaction {}", 
                         shard_id, txn.transaction_id);
        }
    }
    
    if (!all_precommitted) {
        spdlog::error("3PC Phase 2 (PreCommit) failed for transaction {}", 
                     txn.transaction_id);
        
        // In 3PC, if PreCommit fails, we can still abort
        // Send abort to all participants
        for (auto& [shard_id, participant] : txn.participants) {
            sendAbort(shard_id, txn.transaction_id);
            participant.aborted = true;
        }
        
        return false;
    }
    
    spdlog::info("3PC Phase 2 (PreCommit) succeeded for transaction {}", 
                txn.transaction_id);
    
    // Phase 3: DoCommit - final commit
    spdlog::info("3PC Phase 3 (DoCommit) starting for transaction {}", 
                txn.transaction_id);
    
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
    
    if (all_committed) {
        spdlog::info("3PC completed successfully for transaction {}", 
                    txn.transaction_id);
    } else {
        spdlog::error("3PC Phase 3 (DoCommit) had failures for transaction {}", 
                     txn.transaction_id);
    }
    
    return all_committed;
}

bool CrossShardTransactionCoordinator::executePercolator(CrossShardTransaction& txn) {
    // Percolator-style distributed transaction protocol
    // Based on Google's Percolator paper
    // Uses optimistic concurrency control with locks
    
    spdlog::info("Starting Percolator transaction {}", txn.transaction_id);
    
    if (txn.participants.empty()) {
        spdlog::error("No participants in transaction {}", txn.transaction_id);
        return false;
    }
    
    // Step 1: Choose a primary shard (first participant)
    auto primary_it = txn.participants.begin();
    const std::string& primary_shard_id = primary_it->first;
    auto& primary_participant = primary_it->second;
    
    spdlog::info("Primary shard for transaction {}: {}", 
                txn.transaction_id, primary_shard_id);
    
    // Step 2: Acquire locks on all shards (starting with secondaries)
    // In a full implementation, this would use a lock column in the database
    std::vector<std::string> locked_shards;
    bool all_locked = true;
    
    // Lock secondaries first
    for (auto& [shard_id, participant] : txn.participants) {
        if (shard_id == primary_shard_id) {
            continue;  // Lock primary last
        }
        
        // Attempt to acquire lock with timeout
        spdlog::debug("Acquiring lock on secondary shard {} for transaction {}", 
                     shard_id, txn.transaction_id);
        
        // In production, this would be an RPC call to acquire a lock
        // For now, we'll use the prepare mechanism as a proxy
        bool locked = sendPrepare(shard_id, txn.transaction_id);
        
        if (locked) {
            locked_shards.push_back(shard_id);
            participant.prepared = true;
        } else {
            all_locked = false;
            spdlog::error("Failed to acquire lock on shard {} for transaction {}", 
                         shard_id, txn.transaction_id);
            break;
        }
    }
    
    // If secondary locks failed, abort
    if (!all_locked) {
        spdlog::error("Failed to acquire all secondary locks for transaction {}", 
                     txn.transaction_id);
        
        // Release acquired locks
        for (const auto& shard_id : locked_shards) {
            sendAbort(shard_id, txn.transaction_id);
        }
        
        return false;
    }
    
    // Lock primary
    spdlog::debug("Acquiring lock on primary shard {} for transaction {}", 
                 primary_shard_id, txn.transaction_id);
    
    bool primary_locked = sendPrepare(primary_shard_id, txn.transaction_id);
    
    if (!primary_locked) {
        spdlog::error("Failed to acquire lock on primary shard {} for transaction {}", 
                     primary_shard_id, txn.transaction_id);
        
        // Release all locks
        for (const auto& shard_id : locked_shards) {
            sendAbort(shard_id, txn.transaction_id);
        }
        
        return false;
    }
    
    locked_shards.push_back(primary_shard_id);
    primary_participant.prepared = true;
    
    txn.state = TransactionState::PREPARED;
    
    spdlog::info("All locks acquired for Percolator transaction {}", 
                txn.transaction_id);
    
    // Step 3: Write data to all shards (with locks held)
    // In Percolator, this is the "write" column
    // For our implementation, the prepare phase has already written the data
    
    // Step 4: Commit primary first
    txn.state = TransactionState::COMMITTING;
    
    // Generate commit timestamp
    int64_t commit_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    spdlog::info("Committing primary shard {} for Percolator transaction {} at timestamp {}", 
                primary_shard_id, txn.transaction_id, commit_timestamp);
    
    bool primary_committed = sendCommit(primary_shard_id, txn.transaction_id);
    
    if (!primary_committed) {
        spdlog::error("Primary shard commit failed for transaction {}", 
                     txn.transaction_id);
        
        // Primary commit failed - this is a critical error in Percolator
        // We must abort all participants
        for (const auto& shard_id : locked_shards) {
            sendAbort(shard_id, txn.transaction_id);
        }
        
        return false;
    }
    
    primary_participant.committed = true;
    
    spdlog::info("Primary shard committed for Percolator transaction {}", 
                txn.transaction_id);
    
    // Step 5: Commit secondaries (can be done asynchronously in production)
    // Once primary is committed, the transaction is durable
    // Secondary commits can be retried if they fail
    bool all_committed = true;
    
    for (auto& [shard_id, participant] : txn.participants) {
        if (shard_id == primary_shard_id) {
            continue;  // Already committed
        }
        
        spdlog::debug("Committing secondary shard {} for Percolator transaction {}", 
                     shard_id, txn.transaction_id);
        
        bool committed = sendCommit(shard_id, txn.transaction_id);
        participant.committed = committed;
        
        if (!committed) {
            all_committed = false;
            spdlog::error("Secondary shard {} commit failed for transaction {}", 
                         shard_id, txn.transaction_id);
            // In Percolator, this can be retried later since primary is committed
        }
    }
    
    if (all_committed) {
        spdlog::info("Percolator transaction {} completed successfully", 
                    txn.transaction_id);
    } else {
        spdlog::warn("Percolator transaction {} committed but some secondaries failed (can be retried)", 
                    txn.transaction_id);
    }
    
    // Consider transaction successful if primary committed
    // Secondary commits can be repaired asynchronously
    return true;
}

bool CrossShardTransactionCoordinator::sendPrepare(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending prepare to shard {} for transaction {}", 
                  shard_id, transaction_id);
    
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    auto participant_it = txn.participants.find(shard_id);
    if (participant_it == txn.participants.end()) {
        spdlog::error("Shard {} is not a participant in transaction {}", 
                     shard_id, transaction_id);
        return false;
    }
    
    auto& participant = participant_it->second;
    
    // Create RPC client for this shard
    try {
        themis::sharding::ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = static_cast<int>(config_.prepare_timeout.count());
        rpc_config.max_retries = 3;
        rpc_config.retry_delay_ms = 100;
        
        themis::sharding::ShardRPCClient rpc_client(rpc_config);
        
        // Prepare operations for this shard
        nlohmann::json operations = nlohmann::json::array();
        for (const auto& op : participant.operations) {
            operations.push_back(op);
        }
        
        // Send prepare request with retry logic
        int retries = 0;
        int delay_ms = rpc_config.retry_delay_ms;
        
        while (retries <= rpc_config.max_retries) {
            try {
                bool vote = rpc_client.prepare(transaction_id, operations);
                
                if (vote) {
                    spdlog::info("Shard {} voted COMMIT for transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::warn("Shard {} voted ABORT for transaction {}", 
                               shard_id, transaction_id);
                    return false;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Prepare RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;  // Exponential backoff
                    retries++;
                } else {
                    spdlog::error("Prepare RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    return false;
                }
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        return false;
    }
}

bool CrossShardTransactionCoordinator::sendCommit(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending commit to shard {} for transaction {}", 
                  shard_id, transaction_id);
    
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    auto participant_it = txn.participants.find(shard_id);
    if (participant_it == txn.participants.end()) {
        spdlog::error("Shard {} is not a participant in transaction {}", 
                     shard_id, transaction_id);
        return false;
    }
    
    auto& participant = participant_it->second;
    
    // Create RPC client for this shard
    try {
        themis::sharding::ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = static_cast<int>(config_.commit_timeout.count());
        rpc_config.max_retries = 3;
        rpc_config.retry_delay_ms = 100;
        
        themis::sharding::ShardRPCClient rpc_client(rpc_config);
        
        // Generate commit timestamp using TrueTime if available
        int64_t commit_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Send commit request with retry logic
        int retries = 0;
        int delay_ms = rpc_config.retry_delay_ms;
        
        while (retries <= rpc_config.max_retries) {
            try {
                bool success = rpc_client.commit(transaction_id, commit_timestamp);
                
                if (success) {
                    spdlog::info("Shard {} committed transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::error("Shard {} failed to commit transaction {}", 
                                shard_id, transaction_id);
                    return false;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Commit RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;  // Exponential backoff
                    retries++;
                } else {
                    spdlog::error("Commit RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    return false;
                }
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        return false;
    }
}

bool CrossShardTransactionCoordinator::sendAbort(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending abort to shard {} for transaction {}", 
                  shard_id, transaction_id);
    
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    auto participant_it = txn.participants.find(shard_id);
    if (participant_it == txn.participants.end()) {
        spdlog::error("Shard {} is not a participant in transaction {}", 
                     shard_id, transaction_id);
        return false;
    }
    
    auto& participant = participant_it->second;
    
    // Create RPC client for this shard
    try {
        themis::sharding::ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = static_cast<int>(config_.abort_timeout.count());
        rpc_config.max_retries = 3;
        rpc_config.retry_delay_ms = 100;
        
        themis::sharding::ShardRPCClient rpc_client(rpc_config);
        
        // Send abort request with retry logic
        int retries = 0;
        int delay_ms = rpc_config.retry_delay_ms;
        
        while (retries <= rpc_config.max_retries) {
            try {
                bool success = rpc_client.abort(transaction_id);
                
                if (success) {
                    spdlog::info("Shard {} aborted transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::warn("Shard {} reported abort failure for transaction {}", 
                               shard_id, transaction_id);
                    // Abort is best-effort, so we consider it successful even if shard reports failure
                    return true;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Abort RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;  // Exponential backoff
                    retries++;
                } else {
                    spdlog::error("Abort RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    // Abort is best-effort, so we consider it successful even after retries fail
                    return true;
                }
            }
        }
        
        return true;  // Abort is best-effort
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        // Abort is best-effort, so we consider it successful even on client creation failure
        return true;
    }
}

void CrossShardTransactionCoordinator::deadlockDetectionThread() {
    spdlog::debug("Deadlock detection thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(config_.deadlock_detection_interval);
        
        // Build wait-for graph
        auto graph = buildWaitForGraph();
        
        if (graph.empty()) {
            continue;  // No active transactions with potential conflicts
        }
        
        // Detect cycles using DFS
        std::set<std::string> visited;
        std::set<std::string> rec_stack;
        std::vector<std::string> deadlocked_txns;
        
        for (const auto& [node, _] : graph) {
            if (visited.find(node) == visited.end()) {
                if (detectCycle(graph, node, visited, rec_stack)) {
                    // Found a cycle - all nodes in rec_stack are part of the deadlock
                    for (const auto& txn_id : rec_stack) {
                        deadlocked_txns.push_back(txn_id);
                    }
                    break;  // Handle one deadlock at a time
                }
            }
        }
        
        if (!deadlocked_txns.empty()) {
            spdlog::warn("Deadlock detected involving {} transactions", 
                        deadlocked_txns.size());
            
            deadlocked_transactions_++;
            
            // Select victim: choose the youngest transaction (most recent start time)
            std::string victim_id;
            std::chrono::system_clock::time_point latest_start;
            
            {
                std::lock_guard<std::mutex> lock(transactions_mutex_);
                
                for (const auto& txn_id : deadlocked_txns) {
                    auto it = transactions_.find(txn_id);
                    if (it != transactions_.end()) {
                        if (victim_id.empty() || it->second.start_time > latest_start) {
                            victim_id = txn_id;
                            latest_start = it->second.start_time;
                        }
                    }
                }
            }
            
            if (!victim_id.empty()) {
                spdlog::warn("Aborting transaction {} to resolve deadlock", victim_id);
                abort(victim_id);
            }
        }
    }
    
    spdlog::debug("Deadlock detection thread stopped");
}

std::map<std::string, std::vector<std::string>> 
CrossShardTransactionCoordinator::buildWaitForGraph() const {
    std::map<std::string, std::vector<std::string>> graph;
    
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    // Build wait-for graph from active transactions
    // Transaction A waits for B if:
    // 1. Both are active or preparing
    // 2. They have overlapping participants
    // 3. A started after B
    
    std::vector<std::string> active_txn_ids;
    for (const auto& [txn_id, txn] : transactions_) {
        if (txn.state == TransactionState::ACTIVE ||
            txn.state == TransactionState::PREPARING) {
            active_txn_ids.push_back(txn_id);
        }
    }
    
    // For each pair of active transactions, check for potential conflicts
    for (size_t i = 0; i < active_txn_ids.size(); ++i) {
        const auto& txn_a_id = active_txn_ids[i];
        const auto& txn_a = transactions_.at(txn_a_id);
        
        for (size_t j = i + 1; j < active_txn_ids.size(); ++j) {
            const auto& txn_b_id = active_txn_ids[j];
            const auto& txn_b = transactions_.at(txn_b_id);
            
            // Check if transactions have overlapping participants
            bool has_overlap = false;
            for (const auto& [shard_id_a, _] : txn_a.participants) {
                if (txn_b.participants.find(shard_id_a) != txn_b.participants.end()) {
                    has_overlap = true;
                    break;
                }
            }
            
            if (has_overlap) {
                // Determine wait-for relationship based on start time
                // Younger transaction waits for older transaction
                if (txn_a.start_time < txn_b.start_time) {
                    // B waits for A
                    graph[txn_b_id].push_back(txn_a_id);
                    spdlog::trace("Wait-for edge: {} -> {}", txn_b_id, txn_a_id);
                } else {
                    // A waits for B
                    graph[txn_a_id].push_back(txn_b_id);
                    spdlog::trace("Wait-for edge: {} -> {}", txn_a_id, txn_b_id);
                }
            }
        }
    }
    
    // Add additional wait-for edges based on prepare status
    // If a transaction is in PREPARING state and another has overlapping
    // participants in ACTIVE state, the ACTIVE waits for PREPARING
    for (const auto& [txn_id, txn] : transactions_) {
        if (txn.state == TransactionState::PREPARING) {
            for (const auto& [other_id, other_txn] : transactions_) {
                if (other_id == txn_id) continue;
                if (other_txn.state != TransactionState::ACTIVE) continue;
                
                // Check for overlapping participants
                for (const auto& [shard_id, _] : txn.participants) {
                    if (other_txn.participants.find(shard_id) != 
                        other_txn.participants.end()) {
                        graph[other_id].push_back(txn_id);
                        spdlog::trace("Wait-for edge (prepare): {} -> {}", other_id, txn_id);
                        break;
                    }
                }
            }
        }
    }
    
    spdlog::debug("Built wait-for graph with {} nodes", graph.size());
    
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

void CrossShardTransactionCoordinator::executeCompensations(
    const std::string& transaction_id,
    const std::vector<nlohmann::json>& executed_steps,
    const std::vector<nlohmann::json>& compensations
) {
    spdlog::info("Executing compensations for SAGA transaction {} ({} steps to compensate)", 
                transaction_id, executed_steps.size());
    
    // Execute compensations in reverse order
    for (int j = static_cast<int>(executed_steps.size()) - 1; j >= 0; --j) {
        const auto& step = executed_steps[j];
        const auto& compensation = compensations[j];
        
        if (!compensation.contains("shard_id") || !compensation.contains("operation")) {
            spdlog::error("Compensation {} missing shard_id or operation", j);
            continue;
        }
        
        std::string shard_id = compensation["shard_id"];
        nlohmann::json operation = compensation["operation"];
        
        spdlog::info("Executing compensation {} on shard {} for transaction {}", 
                    j, shard_id, transaction_id);
        
        try {
            std::lock_guard<std::mutex> lock(transactions_mutex_);
            auto it = transactions_.find(transaction_id);
            if (it == transactions_.end()) {
                spdlog::error("Transaction {} not found during compensation", transaction_id);
                continue;
            }
            
            auto& txn = it->second;
            auto participant_it = txn.participants.find(shard_id);
            if (participant_it == txn.participants.end()) {
                spdlog::error("Shard {} not found in transaction {} participants", 
                            shard_id, transaction_id);
                continue;
            }
            
            auto& participant = participant_it->second;
            std::string endpoint = participant.endpoint;
            
            // Create RPC client for compensation
            themis::sharding::ShardRPCClient::Config rpc_config;
            rpc_config.endpoint = endpoint;
            rpc_config.timeout_ms = static_cast<int>(config_.saga_step_timeout.count());
            rpc_config.max_retries = 3;  // Retry compensations aggressively
            rpc_config.retry_delay_ms = 100;
            
            themis::sharding::ShardRPCClient rpc_client(rpc_config);
            
            // Execute compensation operation
            nlohmann::json operations = nlohmann::json::array();
            operations.push_back(operation);
            
            int retries = 0;
            bool success = false;
            
            while (retries <= rpc_config.max_retries) {
                try {
                    // Execute compensation (using abort as proxy for compensation execution)
                    success = rpc_client.abort(transaction_id + "_compensation_" + std::to_string(j));
                    
                    if (success) {
                        spdlog::info("Compensation {} completed successfully", j);
                        break;
                    }
                    
                } catch (const std::exception& e) {
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("Compensation {} execution failed (attempt {}/{}): {}. Retrying", 
                                   j, retries + 1, rpc_config.max_retries + 1, e.what());
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                        retries++;
                    } else {
                        spdlog::error("Compensation {} execution failed after {} retries: {}", 
                                    j, rpc_config.max_retries, e.what());
                        break;
                    }
                }
            }
            
            if (!success) {
                spdlog::error("Compensation {} failed - manual intervention may be required", j);
                // In production, this would be logged to a persistent compensation log
                // for manual intervention or retry
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to execute compensation {}: {}", j, e.what());
        }
    }
    
    spdlog::info("Compensation execution completed for SAGA transaction {}", 
                transaction_id);
}

bool CrossShardTransactionCoordinator::persistTransactionState(
    const std::string& transaction_id,
    TransactionState state
) {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Cannot persist state for non-existent transaction {}", transaction_id);
        return false;
    }
    
    const auto& txn = it->second;
    
    try {
        // Open transaction log file in append mode
        std::ofstream log_file(transaction_log_path_, std::ios::app);
        if (!log_file.is_open()) {
            spdlog::error("Failed to open transaction log file: {}", transaction_log_path_);
            return false;
        }
        
        // Create log entry
        nlohmann::json log_entry = {
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(state)},
            {"protocol", static_cast<int>(txn.protocol)},
            {"isolation_level", static_cast<int>(txn.isolation_level)}
        };
        
        // Add participant information
        nlohmann::json participants_json = nlohmann::json::array();
        for (const auto& [shard_id, participant] : txn.participants) {
            participants_json.push_back({
                {"shard_id", shard_id},
                {"endpoint", participant.endpoint},
                {"prepared", participant.prepared},
                {"committed", participant.committed},
                {"aborted", participant.aborted}
            });
        }
        log_entry["participants"] = participants_json;
        
        // Write log entry as a single line (JSONL format)
        log_file << log_entry.dump() << std::endl;
        log_file.close();
        
        spdlog::debug("Persisted transaction {} state: {}", 
                     transaction_id, static_cast<int>(state));
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to persist transaction state: {}", e.what());
        return false;
    }
}

std::vector<CrossShardTransaction> CrossShardTransactionCoordinator::loadPendingTransactions() {
    std::vector<CrossShardTransaction> pending_transactions;
    
    try {
        std::ifstream log_file(transaction_log_path_);
        if (!log_file.is_open()) {
            spdlog::info("No transaction log file found at {}", transaction_log_path_);
            return pending_transactions;
        }
        
        std::string line;
        std::map<std::string, CrossShardTransaction> txn_map;
        
        // Read all log entries
        while (std::getline(log_file, line)) {
            if (line.empty()) continue;
            
            try {
                auto log_entry = nlohmann::json::parse(line);
                
                std::string txn_id = log_entry["transaction_id"];
                int state_int = log_entry["state"];
                TransactionState state = static_cast<TransactionState>(state_int);
                
                // Check if we've seen this transaction before
                auto it = txn_map.find(txn_id);
                if (it == txn_map.end()) {
                    // New transaction
                    CrossShardTransaction txn;
                    txn.transaction_id = txn_id;
                    txn.protocol = static_cast<TransactionProtocol>(log_entry["protocol"].get<int>());
                    txn.isolation_level = static_cast<IsolationLevel>(log_entry["isolation_level"].get<int>());
                    txn.state = state;
                    
                    // Restore participants
                    if (log_entry.contains("participants")) {
                        for (const auto& p : log_entry["participants"]) {
                            ShardParticipant participant;
                            participant.shard_id = p["shard_id"];
                            participant.endpoint = p["endpoint"];
                            participant.prepared = p.value("prepared", false);
                            participant.committed = p.value("committed", false);
                            participant.aborted = p.value("aborted", false);
                            
                            txn.participants[participant.shard_id] = participant;
                        }
                    }
                    
                    txn_map[txn_id] = txn;
                } else {
                    // Update existing transaction state
                    it->second.state = state;
                    
                    // Update participant states
                    if (log_entry.contains("participants")) {
                        for (const auto& p : log_entry["participants"]) {
                            std::string shard_id = p["shard_id"];
                            auto& participant = it->second.participants[shard_id];
                            participant.prepared = p.value("prepared", false);
                            participant.committed = p.value("committed", false);
                            participant.aborted = p.value("aborted", false);
                        }
                    }
                }
                
            } catch (const std::exception& e) {
                spdlog::error("Failed to parse log entry: {}", e.what());
                continue;
            }
        }
        
        log_file.close();
        
        // Filter for pending transactions (not in final state)
        for (const auto& [txn_id, txn] : txn_map) {
            if (txn.state != TransactionState::COMMITTED && 
                txn.state != TransactionState::ABORTED) {
                pending_transactions.push_back(txn);
                spdlog::info("Found pending transaction: {} in state {}", 
                           txn_id, static_cast<int>(txn.state));
            }
        }
        
        spdlog::info("Loaded {} pending transactions from log", pending_transactions.size());
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load pending transactions: {}", e.what());
    }
    
    return pending_transactions;
}

bool CrossShardTransactionCoordinator::recoverFromFailure() {
    spdlog::info("Starting coordinator recovery from failure");
    
    // Load pending transactions from log
    auto pending = loadPendingTransactions();
    
    if (pending.empty()) {
        spdlog::info("No pending transactions to recover");
        return true;
    }
    
    spdlog::info("Recovering {} pending transactions", pending.size());
    
    int recovered = 0;
    int aborted = 0;
    
    for (auto& txn : pending) {
        spdlog::info("Recovering transaction {} in state {}", 
                    txn.transaction_id, static_cast<int>(txn.state));
        
        // Restore transaction to in-memory map
        {
            std::lock_guard<std::mutex> lock(transactions_mutex_);
            transactions_[txn.transaction_id] = txn;
        }
        
        // Apply recovery logic based on state
        switch (txn.state) {
            case TransactionState::ACTIVE:
            case TransactionState::PREPARING:
                // Transaction was in progress but not prepared
                // Safe to abort
                spdlog::info("Aborting unprepared transaction {}", txn.transaction_id);
                abort(txn.transaction_id);
                aborted++;
                break;
                
            case TransactionState::PREPARED:
                // All participants prepared - we can commit or abort
                // For safety, try to commit if all participants are prepared
                spdlog::info("Attempting to commit prepared transaction {}", txn.transaction_id);
                if (commit(txn.transaction_id)) {
                    recovered++;
                } else {
                    spdlog::warn("Failed to commit prepared transaction {}, aborting", 
                               txn.transaction_id);
                    abort(txn.transaction_id);
                    aborted++;
                }
                break;
                
            case TransactionState::COMMITTING:
                // Commit was in progress - try to complete it
                spdlog::info("Completing commit for transaction {}", txn.transaction_id);
                
                // Send commit to any participants that haven't committed yet
                {
                    bool all_committed = true;
                    for (auto& [shard_id, participant] : txn.participants) {
                        if (!participant.committed) {
                            bool success = sendCommit(shard_id, txn.transaction_id);
                            if (success) {
                                std::lock_guard<std::mutex> lock(transactions_mutex_);
                                transactions_[txn.transaction_id].participants[shard_id].committed = true;
                            } else {
                                all_committed = false;
                            }
                        }
                    }
                    
                    if (all_committed) {
                        std::lock_guard<std::mutex> lock(transactions_mutex_);
                        transactions_[txn.transaction_id].state = TransactionState::COMMITTED;
                        transactions_[txn.transaction_id].end_time = std::chrono::system_clock::now();
                        committed_transactions_++;
                        persistTransactionState(txn.transaction_id, TransactionState::COMMITTED);
                        recovered++;
                    } else {
                        spdlog::warn("Could not complete all commits for transaction {}", 
                                   txn.transaction_id);
                        aborted++;
                    }
                }
                break;
                
            case TransactionState::ABORTING:
                // Abort was in progress - complete it
                spdlog::info("Completing abort for transaction {}", txn.transaction_id);
                abort(txn.transaction_id);
                aborted++;
                break;
                
            case TransactionState::COMMITTED:
            case TransactionState::ABORTED:
                // Already in final state
                recovered++;
                break;
                
            case TransactionState::UNKNOWN:
            default:
                // Unknown state - abort for safety
                spdlog::error("Transaction {} in unknown state, aborting", txn.transaction_id);
                abort(txn.transaction_id);
                aborted++;
                break;
        }
    }
    
    spdlog::info("Recovery complete: {} recovered, {} aborted", recovered, aborted);
    return true;
}

} // namespace sharding
} // namespace themisdb
