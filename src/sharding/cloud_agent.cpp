/**
 * @file cloud_agent.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=6, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/cloud_agent.h"
#include "sharding/shard_topology.h"
#include "sharding/remote_executor.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <future>
#include <thread>
#include <vector>

namespace themis {
namespace sharding {

CloudAgent::CloudAgent(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<RemoteExecutor> executor,
    std::shared_ptr<PrometheusMetrics> metrics
) : CloudAgent(topology, executor, metrics, Config{}) {}

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
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(worker_thread_)) {
        THEMIS_WARN("[CloudAgent] worker thread did not finish within shutdown deadline; detaching.");
    }
    
    if (!themis::utils::joinThreadWithin(health_thread_)) {
        THEMIS_WARN("[CloudAgent] health thread did not finish within shutdown deadline; detaching.");
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
    
    std::vector<std::string> pending = {};

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
            {"total",static_cast<int>(all_shards.size())},
            {"healthy",static_cast<int>(healthy_shards.size())},
            {"unhealthy", static_cast<int>(all_shards.size()) - static_cast<int>(healthy_shards.size()) }
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
    
    std::stringstream ss = {};
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
    
    if (shards.empty()) {
        result.success = false;
        result.status = "failed";
        result.error_message = "No shards to execute on";
        return result;
    }
    
    // Prepare shared data structures for parallel execution
    std::mutex results_mutex;
    nlohmann::json aggregated_result = nlohmann::json::array();
    std::atomic<size_t> success_count{0};
    std::atomic<size_t> failure_count{0};
    std::atomic<size_t> timeout_counter{0};  // Unique counter for timeout keys
    
    // Limit concurrency based on configuration
    const size_t max_concurrent = config_.max_concurrent_operations;
    
    // Launch parallel requests with datacenter-aware ordering
    // Sort shards by datacenter proximity (local DC first)
    std::vector<std::string> sorted_shards = shards;
    if (topology_ && !config_.datacenter.empty()) {
        std::sort(sorted_shards.begin(), sorted_shards.end(),
            [this](const std::string& a, const std::string& b) {
                auto shard_a = topology_->getShard(a);
                auto shard_b = topology_->getShard(b);
                if (shard_a && shard_b) {
                    // Prioritize local datacenter
                    bool a_local = (shard_a->datacenter == config_.datacenter);
                    bool b_local = (shard_b->datacenter == config_.datacenter);
                    if (a_local != b_local) {
                      return a_local;
                    }
                    // Then prioritize same region (if both are non-local)
                    if (!config_.region.empty()) {
                        // Check if datacenter contains region identifier
                        bool a_same_region = (shard_a->datacenter.find(config_.region) != std::string::npos);
                        bool b_same_region = (shard_b->datacenter.find(config_.region) != std::string::npos);
                        if (a_same_region != b_same_region) {
                          return a_same_region;
                        }
                    }
                }
                return a < b;  // Fallback: alphabetical order
            });
    }
    
    // Process shards in batches to limit concurrency
    for (size_t batch_start = 0; batch_start <static_cast<int>(sorted_shards.size()); batch_start += max_concurrent) {
        size_t batch_end = std::min(batch_start + max_concurrent,static_cast<int>(sorted_shards.size()));
        
        // Create futures for this batch
        std::vector<std::future<std::pair<std::string, nlohmann::json>>> futures;
        futures.reserve(batch_end - batch_start);
        
        // Store shard IDs for this batch (for timeout error reporting)
        std::vector<std::string> batch_shard_ids;
        batch_shard_ids.reserve(batch_end - batch_start);
        
        // Execute operations in parallel for this batch
        for (size_t i = batch_start; i < batch_end; ++i) {
            const std::string& shard_id = sorted_shards[i];
            batch_shard_ids.push_back(shard_id);
            
            futures.push_back(std::async(std::launch::async,
                [this, &operation, shard_id]() -> std::pair<std::string, nlohmann::json> {
                nlohmann::json shard_result;
                shard_result["shard_id"] = shard_id;
                
                auto start_time = std::chrono::steady_clock::now();
                
                if (!topology_) {
                    shard_result["success"] = false;
                    shard_result["error"] = "Topology not available";
                    return {shard_id, shard_result};
                }
                
                auto shard_info = topology_->getShard(shard_id);
                if (!shard_info) {
                    shard_result["success"] = false;
                    shard_result["error"] = "Shard not found: " + shard_id;
                    return {shard_id, shard_result};
                }
                
                // Add datacenter info to result
                shard_result["datacenter"] = shard_info->datacenter;
                
                if (!executor_) {
                    shard_result["success"] = false;
                    shard_result["error"] = "Remote executor not available";
                    return {shard_id, shard_result};
                }
                
                try {
                    // Execute the operation on the shard
                    std::string path = "/api/v1/" + operation.operation_type;
                    auto response = executor_->post(*shard_info, path, operation.parameters);
                    
                    auto end_time = std::chrono::steady_clock::now();
                    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time - start_time
                    ).count();
                    
                    shard_result["success"] = response.success;
                    shard_result["data"] = response.data;
                    shard_result["latency_ms"] = (response.execution_time_ms > 0) 
                        ? response.execution_time_ms : latency;
                    
                    if (!response.success) {
                        shard_result["error"] = response.error;
                    }
                } catch (const std::exception& e) {
                    shard_result["success"] = false;
                    shard_result["error"] = e.what();
                }
                
                return {shard_id, shard_result};
            }));
        }
        
        // Collect results from all futures with timeout
        const auto timeout = operation.timeout;
        
        for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
            try {
                auto status = futures[i].wait_for(timeout);
                
                if (status == std::future_status::ready) {
                    auto [shard_id, shard_result] = futures[i].get();
                    
                    // Thread-safe update of shared state
                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        result.shard_results[shard_id] = shard_result;
                        
                        if (shard_result["success"].get<bool>()) {
                            success_count++;
                            if (shard_result.contains("data")) {
                                aggregated_result.push_back(shard_result["data"]);
                            }
                        } else {
                            failure_count++;
                        }
                    }
                } else {
                    // Timeout - add error result with correct shard_id
                    std::lock_guard<std::mutex> lock(results_mutex);
                    nlohmann::json timeout_result;
                    timeout_result["shard_id"] = batch_shard_ids[i];
                    timeout_result["success"] = false;
                    timeout_result["error"] = "Operation timed out";
                    timeout_result["timeout_ms"] = timeout.count();
                    result.shard_results[batch_shard_ids[i]] = timeout_result;
                    failure_count++;
                }
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(results_mutex);
                nlohmann::json error_result;
                error_result["shard_id"] = batch_shard_ids[i];
                error_result["success"] = false;
                error_result["error"] = std::string("Exception: ") + e.what();
                result.shard_results[batch_shard_ids[i]] = error_result;
                failure_count++;
            }
        }
    }
    
    // Build final result
    result.result = {
        {"total_shards",static_cast<int>(shards.size())},
        {"success_count", success_count.load()},
        {"failure_count", failure_count.load()},
        {"aggregated_results", aggregated_result},
        {"agent_datacenter", config_.datacenter},
        {"agent_region", config_.region}
    };
    
    // Set overall success based on partial success threshold
    result.success = (success_count.load() > 0);
    if (failure_count.load() > 0 && success_count.load() > 0) {
        result.status = "partial_success";
    } else if (failure_count.load() == shards.size()) {
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
    [[maybe_unused]] const CloudAgentOperation& operation,
    [[maybe_unused]] const CloudAgentResult& result
) {
    if (!metrics_ || !config_.enable_metrics) {
        return;
    }
    
    // Metrics recording would be implemented here
    // Using the PrometheusMetrics interface
}

void CloudAgent::cleanupOldOperations() {
    auto now = std::chrono::steady_clock::now();
    
    // Lock mutex to access shared state (last_cleanup_ is protected by mutex_)
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if enough time has passed since last cleanup
    if (now - last_cleanup_ < config_.cleanup_interval) {
        return;
    }
    
    last_cleanup_ = now;
    
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
    
    std::stringstream ss = {};
    ss << "cloud_agent_" << std::hex << std::setfill('0') << std::setw(8) << dis(gen);
    return ss.str();
}

} // namespace sharding
} // namespace themis
