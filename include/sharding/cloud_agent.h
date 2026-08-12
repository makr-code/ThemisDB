/**
 * @file cloud_agent.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <chrono>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declarations
class ShardTopology;
class RemoteExecutor;
class PrometheusMetrics;
class AutoRebalancer;

/**
 * Cloud Agent Operation
 * 
 * Represents an operation that can be delegated to the cloud agent
 * for execution across multiple shards.
 */
struct CloudAgentOperation {
    std::string operation_id;
    std::string operation_type;  // "query", "rebalance", "health_check", "maintenance"
    nlohmann::json parameters;
    std::vector<std::string> target_shards;  // Empty = all shards
    std::chrono::system_clock::time_point created_at;
    std::chrono::milliseconds timeout{std::chrono::minutes(5)};
    
    // Callback for operation completion
    using CompletionCallback = std::function<void(const nlohmann::json& result)>;
    CompletionCallback on_complete;
};

/**
 * Cloud Agent Operation Result
 * 
 * Result from delegated operation execution.
 */
struct CloudAgentResult {
    std::string operation_id;
    bool success = false;
    std::string status;  // "pending", "running", "completed", "failed", "timeout"
    nlohmann::json result;
    std::string error_message;
    std::chrono::milliseconds execution_time{0};
    std::map<std::string, nlohmann::json> shard_results;  // Per-shard results
};

/**
 * Cloud Agent
 * 
 * A cloud-based coordination agent for managing distributed operations
 * across ThemisDB shards. The Cloud Agent provides:
 * 
 * - Operation delegation to remote shards
 * - Parallel scatter-gather execution
 * - Health monitoring and reporting
 * - Cloud service integration (AWS, Azure, GCP)
 * - Automatic failover and retry logic
 * 
 * Features:
 * - Async operation execution
 * - Progress tracking
 * - Prometheus metrics integration
 * - OpenTelemetry tracing support
 * 
 * Example:
 *   auto agent = std::make_unique<CloudAgent>(
 *       topology, executor, metrics
 *   );
 *   agent->start();
 *   
 *   CloudAgentOperation op;
 *   op.operation_type = "query";
 *   op.parameters = {{"aql", "FOR doc IN users RETURN doc"}};
 *   op.target_shards = {};  // All shards
 *   
 *   auto result = agent->delegate(op);
 *   
 *   agent->stop();
 */
class CloudAgent {
public:
    struct Config {
        // Agent identity
        std::string agent_id;
        std::string datacenter;
        std::string region;
        
        // Execution settings
        size_t max_concurrent_operations = 10;
        std::chrono::milliseconds operation_timeout{std::chrono::minutes(5)};
        size_t max_retries = 3;
        
        // Health check settings
        bool enable_health_monitoring = true;
        std::chrono::milliseconds health_check_interval{std::chrono::seconds(30)};
        
        // Cloud provider settings (optional)
        std::string cloud_provider;  // "aws", "azure", "gcp", "local"
        std::string cloud_region;
        nlohmann::json cloud_credentials;
        
        // Metrics settings
        bool enable_metrics = true;
        std::string metrics_prefix = "themis_cloud_agent_";
        
        // Statistics settings
        double avg_execution_time_smoothing_factor = 0.1;  // EMA alpha
        size_t max_completed_operations_history = 1000;    // Cleanup threshold
        std::chrono::minutes cleanup_interval{5};           // Cleanup frequency
    };
    
    struct Statistics {
        uint64_t total_operations = 0;
        uint64_t completed_operations = 0;
        uint64_t failed_operations = 0;
        uint64_t timeout_operations = 0;
        uint64_t pending_operations = 0;
        double avg_execution_time_ms = 0.0;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point last_activity;
    };
    
    explicit CloudAgent(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<RemoteExecutor> executor,
        std::shared_ptr<PrometheusMetrics> metrics
    );
    
    CloudAgent(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<RemoteExecutor> executor,
        std::shared_ptr<PrometheusMetrics> metrics,
        const Config& config
    );
    
    ~CloudAgent();
    
    /**
     * Start the cloud agent
     */
    void start();
    
    /**
     * Stop the cloud agent
     */
    void stop();
    
    /**
     * Check if agent is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * Delegate an operation to the cloud agent
     * @param operation Operation to delegate
     * @return Result of the operation (may be async if callback is set)
     */
    CloudAgentResult delegate(const CloudAgentOperation& operation);
    
    /**
     * Delegate an operation asynchronously
     * @param operation Operation to delegate
     * @return Operation ID for tracking
     */
    std::string delegateAsync(const CloudAgentOperation& operation);
    
    /**
     * Get status of an operation
     * @param operation_id Operation identifier
     * @return Current status of the operation
     */
    CloudAgentResult getOperationStatus(const std::string& operation_id) const;
    
    /**
     * Cancel a pending or running operation
     * @param operation_id Operation identifier
     * @return true if cancelled successfully
     */
    bool cancelOperation(const std::string& operation_id);
    
    /**
     * Get all pending operations
     * @return List of pending operation IDs
     */
    std::vector<std::string> getPendingOperations() const;
    
    /**
     * Get agent statistics
     * @return Statistics object
     */
    Statistics getStatistics() const;
    
    /**
     * Get agent statistics as JSON
     * @return JSON statistics
     */
    nlohmann::json getStatisticsJson() const;
    
    /**
     * Get agent health status
     * @return JSON with health information
     */
    nlohmann::json getHealthStatus() const;
    
    /**
     * Get agent configuration
     * @return Current configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * Update agent configuration
     * @param config New configuration
     */
    void updateConfig(const Config& config);
    
    /**
     * Execute health check on all shards
     * @return Health check results per shard
     */
    nlohmann::json executeHealthCheck();
    
private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<RemoteExecutor> executor_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    Config config_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
    std::thread health_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // Operation tracking
    std::map<std::string, CloudAgentOperation> pending_operations_;
    std::map<std::string, CloudAgentResult> completed_operations_;
    
    // Statistics
    Statistics statistics_;
    
    // Cleanup tracking (protected by mutex_)
    std::chrono::steady_clock::time_point last_cleanup_{std::chrono::steady_clock::now()};
    
    // Internal methods
    void workerLoop();
    void healthLoop();
    std::string generateOperationId() const;
    std::string generateAgentId() const;
    CloudAgentResult executeOperation(const CloudAgentOperation& operation);
    CloudAgentResult executeScatterGather(
        const CloudAgentOperation& operation,
        const std::vector<std::string>& shards
    );
    void updateStatistics(const CloudAgentResult& result);
    void recordMetrics(const CloudAgentOperation& operation, const CloudAgentResult& result);
    void cleanupOldOperations();
};

} // namespace sharding
} // namespace themis
