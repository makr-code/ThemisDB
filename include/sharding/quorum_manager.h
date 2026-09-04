/**
 * @file quorum_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    bool success = 0;                                 ///< True when required quorum was reached.
    size_t acks_received;                        ///< Number of successful acknowledgments.
    size_t acks_required;                        ///< Required acknowledgments for success.
    std::vector<std::string> successful_nodes;   ///< Nodes that completed the operation successfully.
    std::vector<std::string> failed_nodes;       ///< Nodes that timed out or failed.
    std::chrono::milliseconds latency;           ///< End-to-end latency observed by the coordinator.
    std::string error_message;                   ///< Failure reason when success is false.

    /**
     * @brief Create successful quorum result.
     * @param acks Number of acknowledged nodes.
     * @param required Number of acknowledgments required by policy.
     * @param nodes Node IDs that acknowledged successfully.
     * @param lat Observed operation latency.
     * @return Populated success result.
     */
    static QuorumResult successful(size_t acks, size_t required,
                                   const std::vector<std::string>& nodes,
                                   std::chrono::milliseconds lat);

    /**
     * @brief Create failed quorum result with error text.
     * @param error Human-readable failure reason.
     * @return Populated failed result with zero acknowledgments.
     */
    static QuorumResult failed(const std::string& error);
};

/**
 * @brief Configuration for quorum operations
 */
struct QuorumConfig {
    QuorumType write_quorum{QuorumType::MAJORITY};            ///< Policy used for write operations.
    QuorumType read_quorum{QuorumType::ONE};                  ///< Policy used for read operations.
    size_t custom_write_quorum{2};                            ///< Required write acknowledgments when using CUSTOM.
    size_t custom_read_quorum{1};                             ///< Required read acknowledgments when using CUSTOM.
    std::chrono::milliseconds operation_timeout{5000};        ///< Maximum wait time for node responses.
    bool fail_fast{false};                                    ///< Stop early once success quorum is reached.
    bool enable_quorum_enforcement{true};                     ///< Disable to bypass quorum checks for compatibility.
};

/**
 * @brief Manages quorum-based operations
 * 
 * Coordinates write and read operations across multiple replicas,
 * ensuring quorum requirements are met before acknowledging success.
 */
class QuorumManager {
public:
    /** @brief Callable signature for node-local write execution. */
    using WriteOperation = std::function<bool(const std::string& node_id)>;
    /** @brief Callable signature for node-local read execution. */
    using ReadOperation = std::function<std::optional<std::string>(const std::string& node_id)>;

    /**
     * @brief Construct quorum manager with initial configuration.
     * @param config Quorum policy and timeout settings.
     */
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
        * @param config New runtime configuration to apply atomically.
     */
    void updateConfig(const QuorumConfig& config);
    
    /**
     * @brief Get current configuration
        * @return Immutable reference to current configuration snapshot.
     */
    const QuorumConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        std::atomic<uint64_t> total_writes{0};        ///< Number of attempted write operations.
        std::atomic<uint64_t> successful_writes{0};   ///< Writes that reached configured quorum.
        std::atomic<uint64_t> failed_writes{0};       ///< Writes that did not reach quorum.
        std::atomic<uint64_t> total_reads{0};         ///< Number of attempted read operations.
        std::atomic<uint64_t> successful_reads{0};    ///< Reads that reached configured quorum.
        std::atomic<uint64_t> failed_reads{0};        ///< Reads that did not reach quorum.
        std::atomic<uint64_t> quorum_timeouts{0};     ///< Number of operations terminated due to timeout.
    };

    /** @brief Return live statistics counters. */
    const Statistics& getStatistics() const { return stats_; }
    /** @brief Reset all statistics counters to zero. */
    void resetStatistics();

private:
    QuorumConfig config_;
    /// @brief LOCK ORDERING (CANONICAL):
    /// Single-tier lock hierarchy for simplicity:
    /// Tier 1 (terminal): config_mutex_ — protects config_ and all operations
    ///
    /// RATIONALE: Quorum manager has minimal state (just config_), so a single mutex
    /// is sufficient. Statistics are atomic (no mutex required for lock-free updates).
    /// All acquisitions must release before returning to prevent deadlock.
    ///
    mutable std::mutex config_mutex_;
    Statistics stats_;
    
    ///< Statistics are maintained using relaxed atomic operations since they're
    /// diagnostic counters with no synchronization requirements with other state.
    
    /**
     * @brief Calculate quorum size based on type
        * @param type Quorum policy variant.
        * @param custom_size Custom threshold when type is CUSTOM.
        * @param total_nodes Total number of candidate nodes.
        * @return Required acknowledgments for success.
     */
    size_t calculateQuorumSize(QuorumType type, size_t custom_size, 
                              size_t total_nodes) const;
    
    /**
     * @brief Wait for operation futures with timeout
        * @tparam T Future result type (`bool` for writes, optional payload for reads).
        * @param futures Pending futures associated with node IDs.
        * @param required_acks Required successful acknowledgments for early success.
        * @param timeout Maximum time window for collecting results.
        * @return Node/result tuples for operations that completed within timeout.
     */
    template<typename T>
    std::vector<std::pair<std::string, T>> waitForOperations(
        std::vector<std::pair<std::string, std::future<T>>>& futures,
        size_t required_acks,
        std::chrono::milliseconds timeout);
};

}  // namespace sharding
}  // namespace themisdb
