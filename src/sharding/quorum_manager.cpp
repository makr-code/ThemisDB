/**
 * @file quorum_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/quorum_manager.h"
#include "utils/logger.h"
#include <stdexcept>
#include <algorithm>
#include <thread>

namespace themisdb {
namespace sharding {

/**
 * @brief Successful.
 * @param[in] acks Input parameter.
 * @param[in] required Input parameter.
 * @param[in] nodes Input parameter.
 * @param[in] lat Input parameter.
 * @return Return value.
 * @details Implements successful without additional internal calls.
 */
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

/**
 * @brief Failed.
 * @param[in] error Input parameter.
 * @return Return value.
 * @details Calls: std::chrono::milliseconds().
 */
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

/**
 * @brief Execute Write.
 * @param[in] operation Input parameter.
 * @param[in] target_nodes Input parameter.
 * @return Return value.
 * @details Calls: operation(), QuorumResult::successful(), size(), std::chrono::milliseconds(), fetch_add(), std::chrono::steady_clock::now(), getWriteQuorumSize(), isQuorumAchievable().
 */
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
    
    stats_.total_writes.fetch_add(1, std::memory_order_release);
    
    auto start = std::chrono::steady_clock::now();
    
    size_t required_acks = getWriteQuorumSize(target_nodes.size());
    
    if (!isQuorumAchievable(target_nodes.size(), true)) {
        stats_.failed_writes.fetch_add(1, std::memory_order_release);
        THEMIS_WARN("Write quorum not achievable: required={}, available={}", 
                    required_acks,target_nodes.size());
        return QuorumResult::failed("Quorum not achievable with available nodes");
    }
    
    if (required_acks == 1) {
        std::vector<std::string> successful_nodes;
        std::vector<std::string> failed_nodes;

        for (const auto& node : target_nodes) {
            bool success = false;
            try {
                success = operation(node);
            } catch (const std::exception& e) {
                THEMIS_DEBUG("Operation on node {} failed with exception: {}", node, e.what());
            }

            if (success) {
                successful_nodes.push_back(node);
                auto end = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                stats_.successful_writes.fetch_add(1, std::memory_order_release);
                return QuorumResult::successful(1, required_acks, successful_nodes, latency);
            }

            failed_nodes.push_back(node);
        }

        auto end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        stats_.failed_writes.fetch_add(1, std::memory_order_release);
        QuorumResult result = QuorumResult::failed("Failed to achieve write quorum");
        result.acks_received = 0;
        result.acks_required = required_acks;
        result.successful_nodes = successful_nodes;
        result.failed_nodes = failed_nodes;
        result.latency = latency;
        return result;
    }

    // Execute operations in parallel.
    // NOTE: For production with large clusters, consider using a thread pool
    // to avoid excessive thread creation overhead.
    std::vector<std::pair<std::string, std::future<bool>>> futures;
    futures.reserve(target_nodes.size());
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
        stats_.successful_writes.fetch_add(1, std::memory_order_release);
        THEMIS_DEBUG("Write quorum achieved: {}/{} nodes acknowledged", 
                     successful_nodes.size(), required_acks);
        return QuorumResult::successful(successful_nodes.size(), required_acks,
                                       successful_nodes, latency);
    } else {
        stats_.failed_writes.fetch_add(1, std::memory_order_release);
        THEMIS_WARN("Write quorum FAILED: {}/{} nodes acknowledged; {} timed out", 
                    successful_nodes.size(), required_acks,failed_nodes.size());
        QuorumResult result = QuorumResult::failed("Failed to achieve write quorum");
        result.acks_received = successful_nodes.size();
        result.acks_required = required_acks;
        result.successful_nodes = successful_nodes;
        result.failed_nodes = failed_nodes;
        result.latency = latency;
        return result;
    }
}

/**
 * @brief Execute Read.
 * @param[in] operation Input parameter.
 * @param[in] target_nodes Input parameter.
 * @return Return value.
 * @details Calls: operation(), has_value(), QuorumResult::successful(), std::chrono::milliseconds(), QuorumResult::failed(), fetch_add(), std::chrono::steady_clock::now(), getReadQuorumSize().
 */
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
    
    stats_.total_reads.fetch_add(1, std::memory_order_release);
    
    auto start = std::chrono::steady_clock::now();
    
    size_t required_acks = getReadQuorumSize(target_nodes.size());
    
    // Execute operations in parallel
    if (required_acks == 1) {
        std::vector<std::string> successful_nodes;
        std::vector<std::string> failed_nodes;

        for (const auto& node : target_nodes) {
            std::optional<std::string> data;
            try {
                data = operation(node);
            } catch (const std::exception& e) {
                THEMIS_DEBUG("Operation on node {} failed with exception: {}", node, e.what());
            }

            if (data.has_value()) {
                successful_nodes.push_back(node);
                auto end = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                stats_.successful_reads.fetch_add(1, std::memory_order_release);
                return QuorumResult::successful(1, required_acks, successful_nodes, latency);
            }

            failed_nodes.push_back(node);
        }

        auto end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        stats_.failed_reads.fetch_add(1, std::memory_order_release);
        QuorumResult result = QuorumResult::failed("No nodes available for read");
        result.acks_received = 0;
        result.acks_required = required_acks;
        result.successful_nodes = successful_nodes;
        result.failed_nodes = failed_nodes;
        result.latency = latency;
        return result;
    }

    std::vector<std::pair<std::string, std::future<std::optional<std::string>>>> futures;
    futures.reserve(target_nodes.size());
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
        stats_.successful_reads.fetch_add(1, std::memory_order_release);
        THEMIS_DEBUG("Read quorum achieved: {}/{} nodes responded", 
                     successful_nodes.size(), required_acks);
        return QuorumResult::successful(successful_nodes.size(), required_acks,
                                       successful_nodes, latency);
    } else {
        stats_.failed_reads.fetch_add(1, std::memory_order_release);
        THEMIS_WARN("Read quorum FAILED: {}/{} nodes responded; {} timed out", 
                    successful_nodes.size(), required_acks,failed_nodes.size());
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
    /**
     * @brief Lock.
     * @param[in] config_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(config_mutex_);
    return calculateQuorumSize(config_.write_quorum, config_.custom_write_quorum, total_nodes);
}

size_t QuorumManager::getReadQuorumSize(size_t total_nodes) const {
    /**
     * @brief Lock.
     * @param[in] config_mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(config_mutex_);
    return calculateQuorumSize(config_.read_quorum, config_.custom_read_quorum, total_nodes);
}

bool QuorumManager::isQuorumAchievable(size_t available_nodes, bool is_write) const {
    size_t required = is_write ? getWriteQuorumSize(available_nodes) 
                               : getReadQuorumSize(available_nodes);
    return available_nodes >= required;
}

/**
 * @brief Update the access control configuration.
 * @param[in] config New access control configuration.
 * @details Calls: lock().
 */
void QuorumManager::updateConfig(const QuorumConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

/**
 * @brief Reset Statistics.
 * @details Implements resetStatistics without additional internal calls.
 */
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
    std::vector<std::string> timed_out_nodes;
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    size_t acks = 0;
    for (auto& [node, future] : futures) {
        auto remaining = deadline - std::chrono::steady_clock::now();
        
        if (remaining <= std::chrono::milliseconds(0)) {
            stats_.quorum_timeouts.fetch_add(1, std::memory_order_release);
            timed_out_nodes.push_back(node);
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
                        if (acks >= required_acks) {
                            break;  // Early exit
                        }
                    }
                } else if constexpr (std::is_same_v<T, std::optional<std::string>>) {
                    if (result.has_value()) {
                        acks++;
                        if (acks >= required_acks) {
                            break;  // Early exit
                        }
                    }
                }
            } catch (const std::exception& e) {
                // Operation failed; log for diagnostics
                THEMIS_DEBUG("Operation on node {} failed with exception: {}", node, e.what());
            }
        } else {
            stats_.quorum_timeouts.fetch_add(1, std::memory_order_release);
            timed_out_nodes.push_back(node);
            THEMIS_DEBUG("Operation on node {} timed out (remaining: {}ms)", 
                        node, remaining.count());
        }
    }
    
    if (!timed_out_nodes.empty()) {
        THEMIS_WARN("Quorum wait: {} nodes timed out",timed_out_nodes.size());
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

