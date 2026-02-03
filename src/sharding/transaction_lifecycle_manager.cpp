// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/transaction_lifecycle_manager.h"

namespace themisdb {
namespace sharding {

TransactionLifecycleManager::TransactionLifecycleManager(const Config& config)
    : config_(config) {}

bool TransactionLifecycleManager::registerTransaction(const std::string& txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already exists
    if (transactions_.find(txn_id) != transactions_.end()) {
        return false;
    }
    
    // Check pending limit
    size_t pending_count = 0;
    for (const auto& [id, meta] : transactions_) {
        if (!isTerminalState(meta.state)) {
            ++pending_count;
        }
    }
    
    if (pending_count >= config_.max_pending) {
        return false;
    }
    
    // Register transaction
    auto now = std::chrono::system_clock::now();
    TransactionMetadata meta;
    meta.transaction_id = txn_id;
    meta.state = TransactionState::ACTIVE;
    meta.created_at = now;
    meta.last_updated = now;
    meta.timeout_at = now + config_.ttl;
    
    transactions_[txn_id] = meta;
    
    // Add to timeout queue
    timeouts_.push(TransactionTimeout{txn_id, meta.timeout_at});
    
    return true;
}

bool TransactionLifecycleManager::transitionState(
    const std::string& txn_id,
    TransactionState new_state
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return false;
    }
    
    it->second.state = new_state;
    it->second.last_updated = std::chrono::system_clock::now();
    
    if (isTerminalState(new_state)) {
        ++completed_count_;
    }
    
    return true;
}

size_t TransactionLifecycleManager::cleanupCompletedTransactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t cleaned = 0;
    std::vector<std::string> to_remove;
    
    for (const auto& [txn_id, meta] : transactions_) {
        if (isTerminalState(meta.state)) {
            to_remove.push_back(txn_id);
        }
    }
    
    for (const std::string& txn_id : to_remove) {
        transactions_.erase(txn_id);
        ++cleaned;
    }
    
    return cleaned;
}

size_t TransactionLifecycleManager::reapTimedOutTransactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t reaped = 0;
    
    // Process timeouts from priority queue
    while (!timeouts_.empty()) {
        const auto& timeout = timeouts_.top();
        
        if (timeout.timeout_at > now) {
            // No more timed-out transactions
            break;
        }
        
        auto it = transactions_.find(timeout.transaction_id);
        if (it != transactions_.end() && !isTerminalState(it->second.state)) {
            // Transaction timed out
            if (config_.on_timeout) {
                config_.on_timeout(timeout.transaction_id);
            }
            
            it->second.state = TransactionState::ABORTED;
            it->second.last_updated = now;
            ++timed_out_count_;
            ++reaped;
        }
        
        timeouts_.pop();
    }
    
    return reaped;
}

std::optional<TransactionMetadata> TransactionLifecycleManager::getMetadata(
    const std::string& txn_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

TransactionLifecycleManager::Stats TransactionLifecycleManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.timed_out = timed_out_count_;
    stats.completed = completed_count_;
    
    for (const auto& [txn_id, meta] : transactions_) {
        if (!isTerminalState(meta.state)) {
            ++stats.total_pending;
        }
        ++stats.by_state[meta.state];
    }
    
    return stats;
}

bool TransactionLifecycleManager::isTerminalState(TransactionState state) const {
    return state == TransactionState::COMMITTED || 
           state == TransactionState::ABORTED;
}

void TransactionLifecycleManager::removeTransaction(const std::string& txn_id) {
    transactions_.erase(txn_id);
}

} // namespace sharding
} // namespace themisdb
