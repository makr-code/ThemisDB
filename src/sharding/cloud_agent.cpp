#include "sharding/cloud_agent.h"
#include "sharding/shard_topology.h"
#include "sharding/remote_executor.h"
#include "sharding/prometheus_metrics.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace themis {
namespace sharding {

CloudAgent::CloudAgent(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<RemoteExecutor> executor,
    std::shared_ptr<PrometheusMetrics> metrics,
    const Config& config
) : topology_(std::move(topology)),
    executor_(std::move(executor)),
    metrics_(std::move(metrics)),
    config_(config) {
    
    statistics_.start_time = std::chrono::system_clock::now();
    statistics_.last_activity = statistics_.start_time;
    
    // Generate agent ID if not provided
    if (config_.agent_id.empty()) {
        config_.agent_id = generateAgentId();
    }
}

CloudAgent::~CloudAgent() {
    stop();
}

void CloudAgent::start() {
    if (running_.load()) {
        return;  // Already running
    }
    
    running_.store(true);
    
    // Start worker thread for processing operations
    worker_thread_ = std::thread(&CloudAgent::workerLoop, this);
    
    // Start health monitoring thread if enabled
    if (config_.enable_health_monitoring) {
        health_thread_ = std::thread(&CloudAgent::healthLoop, this);
    }
}

void CloudAgent::stop() {
    if (!running_.load()) {
        return;  // Already stopped
    }
    
    running_.store(false);
    cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    if (health_thread_.joinable()) {
        health_thread_.join();
    }
}

CloudAgentResult CloudAgent::delegate(const CloudAgentOperation& operation) {
    if (!running_.load()) {
        CloudAgentResult result;
        result.operation_id = operation.operation_id.empty() ? 
            generateOperationId() : operation.operation_id;
        result.success = false;
        result.status = "failed";
        result.error_message = "Cloud agent is not running";
        return result;
    }
    
    // Execute the operation synchronously
    auto start_time = std::chrono::steady_clock::now();
    auto result = executeOperation(operation);
    auto end_time = std::chrono::steady_clock::now();
    
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    // Update statistics and metrics
    updateStatistics(result);
    recordMetrics(operation, result);
    
    // Call completion callback if provided
    if (operation.on_complete) {
        operation.on_complete(result.result);
    }
    
    return result;
}

std::string CloudAgent::delegateAsync(const CloudAgentOperation& operation) {
    std::string op_id = operation.operation_id.empty() ? 
        generateOperationId() : operation.operation_id;
    
    CloudAgentOperation op = operation;
    op.operation_id = op_id;
    op.created_at = std::chrono::system_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_operations_[op_id] = op;
        statistics_.pending_operations++;
    }
    
    cv_.notify_one();
    return op_id;
}

CloudAgentResult CloudAgent::getOperationStatus(const std::string& operation_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check completed operations
    auto completed_it = completed_operations_.find(operation_id);
    if (completed_it != completed_operations_.end()) {
        return completed_it->second;
    }
    
    // Check pending operations
    auto pending_it = pending_operations_.find(operation_id);
    if (pending_it != pending_operations_.end()) {
        CloudAgentResult result;
        result.operation_id = operation_id;
        result.success = false;
        result.status = "pending";
        return result;
    }
    
    // Operation not found
    CloudAgentResult result;
    result.operation_id = operation_id;
    result.success = false;
    result.status = "not_found";
    result.error_message = "Operation not found";
    return result;
}

bool CloudAgent::cancelOperation(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pending_operations_.find(operation_id);
    if (it != pending_operations_.end()) {
        pending_operations_.erase(it);
        statistics_.pending_operations--;
        
        // Add to completed with cancelled status
        CloudAgentResult result;
        result.operation_id = operation_id;
        result.success = false;
        result.status = "cancelled";
        completed_operations_[operation_id] = result;
        
        return true;
    }
    
    return false;
}

std::vector<std::string> CloudAgent::getPendingOperations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> pending;
    pending.reserve(pending_operations_.size());
    
    for (const auto& [op_id, op] : pending_operations_) {
        pending.push_back(op_id);
    }
    
    return pending;
}

CloudAgent::Statistics CloudAgent::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return statistics_;
}

nlohmann::json CloudAgent::getStatisticsJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        now - statistics_.start_time
    );
    
    return {
        {"agent_id", config_.agent_id},
        {"datacenter", config_.datacenter},
        {"region", config_.region},
        {"cloud_provider", config_.cloud_provider},
        {"running", running_.load()},
        {"uptime_seconds", uptime.count()},
        {"total_operations", statistics_.total_operations},
        {"completed_operations", statistics_.completed_operations},
        {"failed_operations", statistics_.failed_operations},
        {"timeout_operations", statistics_.timeout_operations},
        {"pending_operations", statistics_.pending_operations},
        {"avg_execution_time_ms", statistics_.avg_execution_time_ms}
    };
}

nlohmann::json CloudAgent::getHealthStatus() const {
    nlohmann::json health;
    health["agent_id"] = config_.agent_id;
    health["status"] = running_.load() ? "healthy" : "stopped";
    health["running"] = running_.load();
    
    // Add shard health information if available
    if (topology_) {
        auto healthy_shards = topology_->getHealthyShards();
        auto all_shards = topology_->getAllShards();
        
        health["shards"] = {
            {"total", all_shards.size()},
            {"healthy", healthy_shards.size()},
            {"unhealthy", all_shards.size() - healthy_shards.size()}
        };
    }
    
    return health;
}

void CloudAgent::updateConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

nlohmann::json CloudAgent::executeHealthCheck() {
    nlohmann::json results;
    results["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (!topology_) {
        results["error"] = "Topology not available";
        return results;
    }
    
    auto all_shards = topology_->getAllShards();
    results["shard_count"] = all_shards.size();
    
    nlohmann::json shard_results = nlohmann::json::array();
    
    for (const auto& shard : all_shards) {
        nlohmann::json shard_health;
        shard_health["shard_id"] = shard.shard_id;
        shard_health["endpoint"] = shard.primary_endpoint;
        shard_health["is_healthy"] = shard.is_healthy;
        
        // Try to execute health check on remote shard
        if (executor_ && shard.is_healthy) {
            try {
                auto response = executor_->get(shard, "/health");
                shard_health["reachable"] = response.success;
                shard_health["latency_ms"] = response.execution_time_ms;
                if (!response.success) {
                    shard_health["error"] = response.error;
                }
            } catch (const std::exception& e) {
                shard_health["reachable"] = false;
                shard_health["error"] = e.what();
            }
        } else {
            shard_health["reachable"] = false;
        }
        
        shard_results.push_back(shard_health);
    }
    
    results["shards"] = shard_results;
    return results;
}

// Private methods

void CloudAgent::workerLoop() {
    while (running_.load()) {
        CloudAgentOperation operation;
        bool has_operation = false;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait for operations or shutdown
            cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
                return !running_.load() || !pending_operations_.empty();
            });
            
            if (!running_.load()) {
                break;
            }
            
            if (!pending_operations_.empty()) {
                auto it = pending_operations_.begin();
                operation = it->second;
                pending_operations_.erase(it);
                statistics_.pending_operations--;
                has_operation = true;
            }
        }
        
        if (has_operation) {
            auto start_time = std::chrono::steady_clock::now();
            auto result = executeOperation(operation);
            auto end_time = std::chrono::steady_clock::now();
            
            result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time
            );
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                completed_operations_[operation.operation_id] = result;
            }
            
            updateStatistics(result);
            recordMetrics(operation, result);
            
            // Call completion callback if provided
            if (operation.on_complete) {
                operation.on_complete(result.result);
            }
        }
        
        // Cleanup old completed operations periodically
        cleanupOldOperations();
    }
}

void CloudAgent::healthLoop() {
    while (running_.load()) {
        // Wait for health check interval
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, config_.health_check_interval, [this]() {
                return !running_.load();
            });
        }
        
        if (!running_.load()) {
            break;
        }
        
        // Execute health check
        auto health = executeHealthCheck();
        
        // Record health metrics
        if (metrics_) {
            // Health check metrics would be recorded here
        }
    }
}

std::string CloudAgent::generateOperationId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss;
    ss << "op_" << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return ss.str();
}

CloudAgentResult CloudAgent::executeOperation(const CloudAgentOperation& operation) {
    CloudAgentResult result;
    result.operation_id = operation.operation_id;
    
    try {
        // Get target shards
        std::vector<std::string> target_shards;
        
        if (operation.target_shards.empty() && topology_) {
            // All healthy shards
            auto healthy = topology_->getHealthyShards();
            for (const auto& shard : healthy) {
                target_shards.push_back(shard.shard_id);
            }
        } else {
            target_shards = operation.target_shards;
        }
        
        if (target_shards.empty()) {
            result.success = false;
            result.status = "failed";
            result.error_message = "No target shards available";
            return result;
        }
        
        // Execute based on operation type
        if (operation.operation_type == "health_check") {
            result.result = executeHealthCheck();
            result.success = true;
            result.status = "completed";
        } else if (operation.operation_type == "query" || 
                   operation.operation_type == "rebalance" ||
                   operation.operation_type == "maintenance") {
            // Execute scatter-gather
            result = executeScatterGather(operation, target_shards);
        } else {
            result.success = false;
            result.status = "failed";
            result.error_message = "Unknown operation type: " + operation.operation_type;
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.status = "failed";
        result.error_message = e.what();
    }
    
    return result;
}

CloudAgentResult CloudAgent::executeScatterGather(
    const CloudAgentOperation& operation,
    const std::vector<std::string>& shards
) {
    CloudAgentResult result;
    result.operation_id = operation.operation_id;
    result.success = true;
    result.status = "completed";
    
    nlohmann::json aggregated_result = nlohmann::json::array();
    size_t success_count = 0;
    size_t failure_count = 0;
    
    for (const auto& shard_id : shards) {
        nlohmann::json shard_result;
        shard_result["shard_id"] = shard_id;
        
        if (!topology_) {
            shard_result["success"] = false;
            shard_result["error"] = "Topology not available";
            failure_count++;
            result.shard_results[shard_id] = shard_result;
            continue;
        }
        
        auto shard_info = topology_->getShard(shard_id);
        if (!shard_info) {
            shard_result["success"] = false;
            shard_result["error"] = "Shard not found: " + shard_id;
            failure_count++;
            result.shard_results[shard_id] = shard_result;
            continue;
        }
        
        if (!executor_) {
            shard_result["success"] = false;
            shard_result["error"] = "Remote executor not available";
            failure_count++;
            result.shard_results[shard_id] = shard_result;
            continue;
        }
        
        try {
            // Execute the operation on the shard
            std::string path = "/api/v1/" + operation.operation_type;
            auto response = executor_->post(*shard_info, path, operation.parameters);
            
            shard_result["success"] = response.success;
            shard_result["data"] = response.data;
            shard_result["latency_ms"] = response.execution_time_ms;
            
            if (response.success) {
                success_count++;
                aggregated_result.push_back(response.data);
            } else {
                failure_count++;
                shard_result["error"] = response.error;
            }
        } catch (const std::exception& e) {
            shard_result["success"] = false;
            shard_result["error"] = e.what();
            failure_count++;
        }
        
        result.shard_results[shard_id] = shard_result;
    }
    
    result.result = {
        {"total_shards", shards.size()},
        {"success_count", success_count},
        {"failure_count", failure_count},
        {"aggregated_results", aggregated_result}
    };
    
    // Set overall success based on partial success threshold
    result.success = (success_count > 0);
    if (failure_count > 0 && success_count > 0) {
        result.status = "partial_success";
    } else if (failure_count == shards.size()) {
        result.success = false;
        result.status = "failed";
        result.error_message = "All shard operations failed";
    }
    
    return result;
}

void CloudAgent::updateStatistics(const CloudAgentResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    statistics_.total_operations++;
    statistics_.last_activity = std::chrono::system_clock::now();
    
    if (result.success) {
        statistics_.completed_operations++;
    } else if (result.status == "timeout") {
        statistics_.timeout_operations++;
    } else {
        statistics_.failed_operations++;
    }
    
    // Update average execution time using exponential moving average
    const double alpha = config_.avg_execution_time_smoothing_factor;
    statistics_.avg_execution_time_ms = 
        alpha * result.execution_time.count() + 
        (1.0 - alpha) * statistics_.avg_execution_time_ms;
}

void CloudAgent::recordMetrics(
    const CloudAgentOperation& operation,
    const CloudAgentResult& result
) {
    if (!metrics_ || !config_.enable_metrics) {
        return;
    }
    
    // Metrics recording would be implemented here
    // Using the PrometheusMetrics interface
}

void CloudAgent::cleanupOldOperations() {
    static auto last_cleanup = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    // Use configurable cleanup interval
    if (now - last_cleanup < config_.cleanup_interval) {
        return;
    }
    
    last_cleanup = now;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove completed operations when exceeding the configured threshold
    const size_t max_history = config_.max_completed_operations_history;
    while (completed_operations_.size() > max_history) {
        completed_operations_.erase(completed_operations_.begin());
    }
}

std::string CloudAgent::generateAgentId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    
    std::stringstream ss;
    ss << "cloud_agent_" << std::hex << std::setfill('0') << std::setw(8) << dis(gen);
    return ss.str();
}

} // namespace sharding
} // namespace themis
