/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quorum_manager.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     298                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/quorum_manager.h"
#include <algorithm>
#include <thread>

namespace themisdb {
namespace sharding {

QuorumResult QuorumResult::successful(size_t acks, size_t required,
                                     const std::vector<std::string>& nodes,
                                     std::chrono::milliseconds lat) {
    QuorumResult result;
    result.success = true;
    result.acks_received = acks;
    result.acks_required = required;
    result.successful_nodes = nodes;
    result.latency = lat;
    return result;
}

QuorumResult QuorumResult::failed(const std::string& error) {
    QuorumResult result;
    result.success = false;
    result.acks_received = 0;
    result.acks_required = 0;
    result.error_message = error;
    result.latency = std::chrono::milliseconds(0);
    return result;
}

QuorumManager::QuorumManager(const QuorumConfig& config)
    : config_(config) {}

QuorumResult QuorumManager::executeWrite(WriteOperation operation,
                                        const std::vector<std::string>& target_nodes) {
    if (!config_.enable_quorum_enforcement) {
        // Quorum enforcement disabled - execute on all nodes without waiting
        for (const auto& node : target_nodes) {
            operation(node);  // Fire and forget
        }
        return QuorumResult::successful(target_nodes.size(), 1, target_nodes, 
                                       std::chrono::milliseconds(0));
    }
    
    stats_.total_writes++;
    
    auto start = std::chrono::steady_clock::now();
    
    size_t required_acks = getWriteQuorumSize(target_nodes.size());
    
    if (!isQuorumAchievable(target_nodes.size(), true)) {
        stats_.failed_writes++;
        return QuorumResult::failed("Quorum not achievable with available nodes");
    }
    
    // Execute operations in parallel
    // NOTE: For production with large clusters, consider using a thread pool
    // to avoid excessive thread creation overhead
    std::vector<std::pair<std::string, std::future<bool>>> futures;
    for (const auto& node : target_nodes) {
        auto future = std::async(std::launch::async, operation, node);
        futures.push_back({node, std::move(future)});
    }
    
    // Wait for quorum
    auto results = waitForOperations(futures, required_acks, config_.operation_timeout);
    
    // Count successful operations
    std::vector<std::string> successful_nodes;
    std::vector<std::string> failed_nodes;
    
    for (const auto& [node, success] : results) {
        if (success) {
            successful_nodes.push_back(node);
        } else {
            failed_nodes.push_back(node);
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    bool success = successful_nodes.size() >= required_acks;
    
    if (success) {
        stats_.successful_writes++;
        return QuorumResult::successful(successful_nodes.size(), required_acks,
                                       successful_nodes, latency);
    } else {
        stats_.failed_writes++;
        QuorumResult result = QuorumResult::failed("Failed to achieve write quorum");
        result.acks_received = successful_nodes.size();
        result.acks_required = required_acks;
        result.successful_nodes = successful_nodes;
        result.failed_nodes = failed_nodes;
        result.latency = latency;
        return result;
    }
}

QuorumResult QuorumManager::executeRead(ReadOperation operation,
                                       const std::vector<std::string>& target_nodes) {
    if (!config_.enable_quorum_enforcement) {
        // Try first available node
        for (const auto& node : target_nodes) {
            auto result = operation(node);
            if (result.has_value()) {
                return QuorumResult::successful(1, 1, {node}, 
                                               std::chrono::milliseconds(0));
            }
        }
        return QuorumResult::failed("No nodes available for read");
    }
    
    stats_.total_reads++;
    
    auto start = std::chrono::steady_clock::now();
    
    size_t required_acks = getReadQuorumSize(target_nodes.size());
    
    // Execute operations in parallel
    std::vector<std::pair<std::string, std::future<std::optional<std::string>>>> futures;
    for (const auto& node : target_nodes) {
        auto future = std::async(std::launch::async, operation, node);
        futures.push_back({node, std::move(future)});
    }
    
    // Wait for quorum
    auto results = waitForOperations(futures, required_acks, config_.operation_timeout);
    
    std::vector<std::string> successful_nodes;
    std::vector<std::string> failed_nodes;
    
    for (const auto& [node, data] : results) {
        if (data.has_value()) {
            successful_nodes.push_back(node);
        } else {
            failed_nodes.push_back(node);
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    bool success = successful_nodes.size() >= required_acks;
    
    if (success) {
        stats_.successful_reads++;
        return QuorumResult::successful(successful_nodes.size(), required_acks,
                                       successful_nodes, latency);
    } else {
        stats_.failed_reads++;
        QuorumResult result = QuorumResult::failed("Failed to achieve read quorum");
        result.acks_received = successful_nodes.size();
        result.acks_required = required_acks;
        result.successful_nodes = successful_nodes;
        result.failed_nodes = failed_nodes;
        result.latency = latency;
        return result;
    }
}

size_t QuorumManager::getWriteQuorumSize(size_t total_nodes) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return calculateQuorumSize(config_.write_quorum, config_.custom_write_quorum, total_nodes);
}

size_t QuorumManager::getReadQuorumSize(size_t total_nodes) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return calculateQuorumSize(config_.read_quorum, config_.custom_read_quorum, total_nodes);
}

bool QuorumManager::isQuorumAchievable(size_t available_nodes, bool is_write) const {
    size_t required = is_write ? getWriteQuorumSize(available_nodes) 
                               : getReadQuorumSize(available_nodes);
    return available_nodes >= required;
}

void QuorumManager::updateConfig(const QuorumConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

void QuorumManager::resetStatistics() {
    stats_.total_writes = 0;
    stats_.successful_writes = 0;
    stats_.failed_writes = 0;
    stats_.total_reads = 0;
    stats_.successful_reads = 0;
    stats_.failed_reads = 0;
    stats_.quorum_timeouts = 0;
}

size_t QuorumManager::calculateQuorumSize(QuorumType type, size_t custom_size,
                                         size_t total_nodes) const {
    switch (type) {
        case QuorumType::ONE:
            return 1;
        case QuorumType::MAJORITY:
            return (total_nodes / 2) + 1;
        case QuorumType::ALL:
            return total_nodes;
        case QuorumType::CUSTOM:
            return std::min(custom_size, total_nodes);
        default:
            return 1;
    }
}

template<typename T>
std::vector<std::pair<std::string, T>> QuorumManager::waitForOperations(
    std::vector<std::pair<std::string, std::future<T>>>& futures,
    size_t required_acks,
    std::chrono::milliseconds timeout) {
    
    std::vector<std::pair<std::string, T>> results;
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    size_t acks = 0;
    for (auto& [node, future] : futures) {
        auto remaining = deadline - std::chrono::steady_clock::now();
        
        if (remaining <= std::chrono::milliseconds(0)) {
            stats_.quorum_timeouts++;
            break;  // Timeout
        }
        
        auto status = future.wait_for(remaining);
        if (status == std::future_status::ready) {
            try {
                T result = future.get();
                results.push_back({node, result});
                
                // Check if this is a successful operation
                if constexpr (std::is_same_v<T, bool>) {
                    if (result) {
                        acks++;
                        if (config_.fail_fast && acks >= required_acks) {
                            break;  // Early exit
                        }
                    }
                } else if constexpr (std::is_same_v<T, std::optional<std::string>>) {
                    if (result.has_value()) {
                        acks++;
                        if (config_.fail_fast && acks >= required_acks) {
                            break;  // Early exit
                        }
                    }
                }
            } catch (...) {
                // Operation failed
            }
        } else {
            stats_.quorum_timeouts++;
        }
    }
    
    return results;
}

// Explicit template instantiations
template std::vector<std::pair<std::string, bool>> QuorumManager::waitForOperations(
    std::vector<std::pair<std::string, std::future<bool>>>& futures,
    size_t required_acks,
    std::chrono::milliseconds timeout);

template std::vector<std::pair<std::string, std::optional<std::string>>> 
QuorumManager::waitForOperations(
    std::vector<std::pair<std::string, std::future<std::optional<std::string>>>>& futures,
    size_t required_acks,
    std::chrono::milliseconds timeout);

}  // namespace sharding
}  // namespace themisdb
