/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quorum_manager.h                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <future>
#include <chrono>
#include <memory>
#include <atomic>
#include <mutex>

namespace themisdb {
namespace sharding {

/**
 * @brief Quorum type for operations
 */
enum class QuorumType {
    ONE,        // Acknowledge after one node
    MAJORITY,   // Wait for majority (N/2 + 1)
    ALL,        // Wait for all nodes
    CUSTOM      // Custom quorum size
};

/**
 * @brief Result of a quorum operation
 */
struct QuorumResult {
    bool success;
    size_t acks_received;
    size_t acks_required;
    std::vector<std::string> successful_nodes;
    std::vector<std::string> failed_nodes;
    std::chrono::milliseconds latency;
    std::string error_message;
    
    static QuorumResult successful(size_t acks, size_t required,
                                   const std::vector<std::string>& nodes,
                                   std::chrono::milliseconds lat);
    static QuorumResult failed(const std::string& error);
};

/**
 * @brief Configuration for quorum operations
 */
struct QuorumConfig {
    QuorumType write_quorum{QuorumType::MAJORITY};
    QuorumType read_quorum{QuorumType::ONE};
    size_t custom_write_quorum{2};
    size_t custom_read_quorum{1};
    std::chrono::milliseconds operation_timeout{5000};
    bool fail_fast{false};  // Fail immediately if quorum impossible
    bool enable_quorum_enforcement{true};  // Enable/disable for backward compat
};

/**
 * @brief Manages quorum-based operations
 * 
 * Coordinates write and read operations across multiple replicas,
 * ensuring quorum requirements are met before acknowledging success.
 */
class QuorumManager {
public:
    using WriteOperation = std::function<bool(const std::string& node_id)>;
    using ReadOperation = std::function<std::optional<std::string>(const std::string& node_id)>;
    
    explicit QuorumManager(const QuorumConfig& config);
    ~QuorumManager() = default;
    
    /**
     * @brief Execute a write operation with quorum enforcement
     * @param operation Write operation to execute on each node
     * @param target_nodes List of nodes to write to
     * @return Result indicating success/failure and acknowledgments
     */
    QuorumResult executeWrite(WriteOperation operation,
                             const std::vector<std::string>& target_nodes);
    
    /**
     * @brief Execute a read operation with quorum enforcement
     * @param operation Read operation to execute
     * @param target_nodes List of nodes to read from
     * @return Result with data from quorum
     */
    QuorumResult executeRead(ReadOperation operation,
                            const std::vector<std::string>& target_nodes);
    
    /**
     * @brief Calculate required acknowledgments for write quorum
     * @param total_nodes Total number of nodes
     * @return Number of acks required
     */
    size_t getWriteQuorumSize(size_t total_nodes) const;
    
    /**
     * @brief Calculate required acknowledgments for read quorum
     * @param total_nodes Total number of nodes
     * @return Number of acks required
     */
    size_t getReadQuorumSize(size_t total_nodes) const;
    
    /**
     * @brief Check if quorum is achievable with current node count
     * @param available_nodes Number of available nodes
     * @param is_write True for write quorum, false for read
     * @return True if quorum is possible
     */
    bool isQuorumAchievable(size_t available_nodes, bool is_write) const;
    
    /**
     * @brief Update configuration
     */
    void updateConfig(const QuorumConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const QuorumConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        std::atomic<uint64_t> total_writes{0};
        std::atomic<uint64_t> successful_writes{0};
        std::atomic<uint64_t> failed_writes{0};
        std::atomic<uint64_t> total_reads{0};
        std::atomic<uint64_t> successful_reads{0};
        std::atomic<uint64_t> failed_reads{0};
        std::atomic<uint64_t> quorum_timeouts{0};
    };
    
    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();

private:
    QuorumConfig config_;
    mutable std::mutex config_mutex_;
    Statistics stats_;
    
    /**
     * @brief Calculate quorum size based on type
     */
    size_t calculateQuorumSize(QuorumType type, size_t custom_size, 
                              size_t total_nodes) const;
    
    /**
     * @brief Wait for operation futures with timeout
     */
    template<typename T>
    std::vector<std::pair<std::string, T>> waitForOperations(
        std::vector<std::pair<std::string, std::future<T>>>& futures,
        size_t required_acks,
        std::chrono::milliseconds timeout);
};

}  // namespace sharding
}  // namespace themisdb
