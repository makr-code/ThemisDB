/**
 * @file adapter_deployment_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <functional>

// Forward declarations
class ShardRouter;
class ShardTopology;
class AdapterRegistry;
class AdapterCompatibilityValidator;

namespace llm {

// Deployment strategy types
enum class DeploymentStrategy {
    CO_LOCATED,          // Deploy adapter on shard with relevant data
    REPLICATED,          // Replicate adapter across all shards
    LOAD_BALANCED,       // Distribute based on load
    AFFINITY_BASED,      // Based on data affinity patterns
    CUSTOM               // User-defined placement
};

// Adapter placement decision
struct AdapterPlacement {
    /**
     * @brief Adapter Placement.
     * @return Return value.
     */
    virtual ~AdapterPlacement() = default;
    std::string adapter_id;
    std::string shard_id;
    DeploymentStrategy strategy;
    float affinity_score = 0.0f;    // 0.0-1.0, higher = better fit
    size_t estimated_requests_per_sec = 0;
    size_t adapter_size_bytes = 0;
    std::string reason;             // Human-readable explanation
};

// Deployment configuration
struct DeploymentConfig {
    DeploymentStrategy strategy = DeploymentStrategy::CO_LOCATED;
    bool validate_compatibility = true;
    bool verify_signature = true;
    bool auto_rollback_on_failure = true;
    int max_concurrent_deployments = 4;
    int deployment_timeout_seconds = 300;
    float min_affinity_threshold = 0.5f;
    std::vector<std::string> preferred_shards;
    std::map<std::string, std::string> custom_metadata;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    std::string toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] json Input parameter.
     * @return Return value.
     */
    static DeploymentConfig fromJSON(const std::string& json);
};

// Deployment result
struct DeploymentResult {
    /**
     * @brief Deployment Result.
     * @return Return value.
     */
    virtual ~DeploymentResult() = default;
    bool success = false;
    std::string adapter_id;
    std::vector<std::string> deployed_shards;
    std::vector<std::string> failed_shards;
    std::map<std::string, std::string> shard_errors;
    int64_t deployment_time_ms = 0;
    size_t total_data_transferred_bytes = 0;
    std::string error_message;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    std::string toJSON() const;
};

// Shard affinity metrics
struct ShardAffinityMetrics {
    /**
     * @brief Shard Affinity Metrics.
     * @return Return value.
     */
    virtual ~ShardAffinityMetrics() = default;
    std::string shard_id;
    float data_coverage_ratio = 0.0f;      // % of training data on this shard
    float query_frequency = 0.0f;          // Expected query rate
    float network_latency_ms = 0.0f;       // Latency to shard
    float available_memory_gb = 0.0f;      // Free memory
    float cpu_utilization = 0.0f;          // Current CPU usage (0.0-1.0)
    size_t active_adapters_count = 0;   // Already deployed adapters
    
    float computeAffinityScore() const {
        // Weighted score combining multiple factors
        return (data_coverage_ratio * 0.4f) +
               (query_frequency * 0.3f) +
               ((1.0f - cpu_utilization) * 0.2f) +
               ((available_memory_gb > 2.0f ? 1.0f : 0.5f) * 0.1f);
    }
};

// Deployment plan
struct DeploymentPlan {
    /**
     * @brief Deployment Plan.
     * @return Return value.
     */
    virtual ~DeploymentPlan() = default;
    std::string adapter_id;
    DeploymentStrategy strategy;
    std::vector<AdapterPlacement> placements;
    int estimated_total_time_seconds = 0;
    size_t total_bandwidth_required_mbps = 0;
    std::vector<std::string> prerequisites;  // e.g., "Adapter must be validated"
    std::string created_at;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    std::string toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] json Input parameter.
     * @return Return value.
     */
    static DeploymentPlan fromJSON(const std::string& json);
};

// Rollback state for recovery
struct RollbackState {
    std::string deployment_id;
    std::string adapter_id;
    std::map<std::string, std::string> previous_adapter_versions;  // shard_id -> version
    std::string snapshot_timestamp;
    std::string reason;
};

// Progress callback
using DeploymentProgressCallback = std::function<void(
    const std::string& adapter_id,
    const std::string& shard_id,
    float progress,  // 0.0-1.0
    const std::string& status_message
)>;

class AdapterDeploymentManager {
public:
    AdapterDeploymentManager(
        std::shared_ptr<ShardRouter> shard_router,
        std::shared_ptr<ShardTopology> shard_topology,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterCompatibilityValidator> validator
    );
    
    /**
     * @brief Adapter Deployment Manager.
     * @return Return value.
     */
    virtual ~AdapterDeploymentManager() = default;
    
    // Planning
    /**
     * @brief Plan Deployment.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    DeploymentPlan planDeployment(
        const std::string& adapter_id,
        const DeploymentConfig& config
    );
    
    /**
     * @brief Compute Placements.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] strategy Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    std::vector<AdapterPlacement> computePlacements(
        const std::string& adapter_id,
        DeploymentStrategy strategy,
        const DeploymentConfig& config
    );
    
    std::map<std::string, ShardAffinityMetrics> analyzeShardAffinity(
        const std::string& adapter_id
    );
    
    // Execution
    /**
     * @brief Execute Deployment.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    DeploymentResult executeDeployment(const DeploymentPlan& plan);
    
    /**
     * @brief Deploy To Shard.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    DeploymentResult deployToShard(
        const std::string& adapter_id,
        const std::string& shard_id,
        const DeploymentConfig& config
    );
    
    /**
     * @brief Undeploy From Shard.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @return True when the operation succeeds.
     */
    bool undeployFromShard(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    // Rollback & Recovery
    /**
     * @brief Rollback.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool rollback(const std::string& adapter_id);
    
    bool saveRollbackState(
        const std::string& adapter_id,
        const std::map<std::string, std::string>& previous_versions
    );
    
    /**
     * @brief Get Rollback State.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<RollbackState> getRollbackState(const std::string& adapter_id);
    
    // Monitoring
    std::map<std::string, std::vector<std::string>> getAdapterDeployments() const;
    
    /**
     * @brief Get Shards For Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<std::string> getShardsForAdapter(const std::string& adapter_id) const;
    
    /**
     * @brief Is Deployed On Shard.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @return True when the operation succeeds.
     */
    bool isDeployedOnShard(
        const std::string& adapter_id,
        const std::string& shard_id
    ) const;
    
    struct DeploymentStatus {
        std::string adapter_id;
        std::string shard_id;
        std::string status;  // "DEPLOYED", "DEPLOYING", "FAILED", "UNDEPLOYED"
        std::string version;
        int64_t deployed_at_timestamp = 0;
        size_t requests_served = 0;
        float avg_latency_ms = 0.0f;
        
        /**
         * @brief To JSON.
         * @return Return value.
         */
        std::string toJSON() const;
    };
    
    /**
     * @brief Get Deployment Statuses.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<DeploymentStatus> getDeploymentStatuses(
        const std::string& adapter_id
    ) const;
    
    // Health checks
    struct HealthCheckResult {
        std::string shard_id;
        bool is_healthy = false;
        std::string adapter_id = {};
        std::string issue_description;
        int64_t last_check_timestamp = 0;
    };
    
    /**
     * @brief Perform Health Check.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<HealthCheckResult> performHealthCheck(const std::string& adapter_id);
    
    /**
     * @brief Verify Deployment Integrity.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @return True when the operation succeeds.
     */
    bool verifyDeploymentIntegrity(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    // Progress tracking
    /**
     * @brief Set Progress Callback.
     * @param[in] callback Input parameter.
     */
    void setProgressCallback(DeploymentProgressCallback callback);
    
    /**
     * @brief Get Deployment Progress.
     * @param[in] deployment_id Identifier of the deployment.
     * @return Return value.
     */
    float getDeploymentProgress(const std::string& deployment_id) const;
    
    // Utility
    /**
     * @brief Generate Deployment Id.
     * @return Return value.
     */
    std::string generateDeploymentId() const;
    
    /**
     * @brief Validate Deployment Plan.
     * @param[in] plan Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDeploymentPlan(const DeploymentPlan& plan) const;
    
private:
    std::shared_ptr<ShardRouter> shard_router_;
    std::shared_ptr<ShardTopology> shard_topology_;
    std::shared_ptr<AdapterRegistry> adapter_registry_;
    std::shared_ptr<AdapterCompatibilityValidator> validator_;
    
    // State tracking
    std::map<std::string, std::vector<std::string>> adapter_to_shards_;
    std::map<std::string, RollbackState> rollback_states_;
    std::map<std::string, float> deployment_progress_;
    
    DeploymentProgressCallback progress_callback_;
    
    // Internal helpers
    /**
     * @brief Compute Affinity Score.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @return Return value.
     */
    float computeAffinityScore(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    /**
     * @brief Transfer Adapter To Shard.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool transferAdapterToShard(
        const std::string& adapter_id,
        const std::string& shard_id,
        const DeploymentConfig& config
    );
    
    /**
     * @brief Notify Progress.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] progress Input parameter.
     * @param[in] message Input parameter.
     */
    void notifyProgress(
        const std::string& adapter_id,
        const std::string& shard_id,
        float progress,
        const std::string& message
    );
};

// Factory for creating deployment managers
class AdapterDeploymentManagerFactory {
public:
    /**
     * @brief Create.
     * @param[in] shard_router Input parameter.
     * @param[in] shard_topology Input parameter.
     * @param[in] adapter_registry Input parameter.
     * @param[in] validator Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<AdapterDeploymentManager> create(
        std::shared_ptr<ShardRouter> shard_router,
        std::shared_ptr<ShardTopology> shard_topology,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterCompatibilityValidator> validator
    );
};

} // namespace llm
