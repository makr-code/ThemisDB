/**
 * @file admin_operations.h
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

#include "sharding/admin_api.h"
#include "sharding/operational_metrics.h"
#include "sharding/shard_topology.h"
#include "sharding/health_check.h"
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

/**
 * @brief High-level admin operations wrapper
 * 
 * Provides a unified interface for administrative operations,
 * combining admin API, metrics, topology management, and health checks.
 */
class AdminOperations {
public:
    struct Config {
        AdminAPI::Config admin_api_config;
        bool enable_metrics{true};
        bool enable_health_checks{true};
        std::chrono::seconds health_check_interval{30};
    };
    
    /**
     * @brief Constructor
     * @param config Admin operations configuration
     */
    explicit AdminOperations(const Config& config);
    
    ~AdminOperations();
    
    /**
     * @brief Initialize admin operations
     * @return true on success
     */
    bool initialize();
    
    /**
     * @brief Shutdown admin operations
     */
    void shutdown();
    
    /**
     * @brief Get topology information
     * @return JSON topology data
     */
    nlohmann::json getTopology() const;
    
    /**
     * @brief Add a new shard to the cluster
     * @param shard_id Shard identifier
     * @param endpoint Shard endpoint (host:port)
     * @return true on success
     */
    bool addShard(const std::string& shard_id, const std::string& endpoint);
    
    /**
     * @brief Remove a shard from the cluster
     * @param shard_id Shard identifier
     * @return true on success
     */
    bool removeShard(const std::string& shard_id);
    
    /**
     * @brief Trigger rebalancing operation
     * @return Rebalance operation ID
     */
    std::string triggerRebalance();
    
    /**
     * @brief Get rebalance status
     * @param operation_id Rebalance operation ID
     * @return JSON status data
     */
    nlohmann::json getRebalanceStatus(const std::string& operation_id) const;
    
    /**
     * @brief Get cluster health status
     * @return JSON health data
     */
    nlohmann::json getHealthStatus() const;
    
    /**
     * @brief Get cluster statistics
     * @return JSON statistics data
     */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Export Prometheus metrics
     * @return Prometheus-formatted metrics string
     */
    std::string exportPrometheusMetrics() const;
    
    /**
     * @brief Get admin API instance
     */
    AdminAPI& getAdminAPI() { return admin_api_; }
    
    /**
     * @brief Get operational metrics instance
     */
    themisdb::sharding::OperationalMetrics& getMetrics() { return *metrics_; }

private:
    Config config_;
    AdminAPI admin_api_;
    std::unique_ptr<themisdb::sharding::OperationalMetrics> metrics_;
    std::unique_ptr<ShardTopology> topology_;
    std::unique_ptr<HealthCheckSystem> health_check_;

    /// In-memory registry of rebalance operations.
    /// Estimated total duration used for time-based progress approximation.
    static constexpr int64_t kRebalanceEstimatedDurationSeconds = 300;
    struct RebalanceOp {
        std::string operation_id;
        std::chrono::system_clock::time_point started_at;
        std::optional<std::chrono::system_clock::time_point> completed_at;
        std::string error_message; ///< Non-empty only on failure
    };
    mutable std::mutex rebalance_ops_mutex_;
    std::map<std::string, RebalanceOp> rebalance_ops_;
    
    /**
     * @brief Handle topology requests
     */
    nlohmann::json handleTopologyRequest(const nlohmann::json& body);
    
    /**
     * @brief Handle rebalance requests
     */
    nlohmann::json handleRebalanceRequest(const nlohmann::json& body);
    
    /**
     * @brief Handle health check requests
     */
    nlohmann::json handleHealthRequest(const nlohmann::json& body);
    
    /**
     * @brief Handle statistics requests
     */
    nlohmann::json handleStatsRequest(const nlohmann::json& body);
};

}  // namespace sharding
}  // namespace themis
