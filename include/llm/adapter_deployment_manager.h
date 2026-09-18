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
     * @brief TBD: Describe ~AdapterPlacement.
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
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    std::string toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
     * @param[in] json Input parameter.
     * @return Return value.
     */
    static DeploymentConfig fromJSON(const std::string& json);
};

// Deployment result
struct DeploymentResult {
    /**
     * @brief TBD: Describe ~DeploymentResult.
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
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    std::string toJSON() const;
};

// Shard affinity metrics
struct ShardAffinityMetrics {
    /**
     * @brief TBD: Describe ~ShardAffinityMetrics.
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
     * @brief TBD: Describe ~DeploymentPlan.
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
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    std::string toJSON() const;
    /**
     * @brief TBD: Describe fromJSON.
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

/**
 * AdapterDeploymentManager
 * 
 * Manages intelligent deployment of LoRA adapters across ThemisDB shards with:
 * - Co-located placement: Deploy adapters on shards with relevant data
 * - Load balancing: Distribute based on query patterns and resource availability
 * - Replication: Ensure high availability for critical adapters
 * - Compatibility validation: Verify adapter works with shard's base model
 * - Signature verification: Ensure adapter authenticity before deployment
 * - Rollback support: Automatic recovery on deployment failures
 * - Monitoring: Track deployment status and health
 * 
 * Example usage:
 * 
 *   auto deployment_mgr = AdapterDeploymentManagerFactory::create(
 *       shard_router, shard_topology, adapter_registry, validator
 *   );
 * 
 *   DeploymentConfig config;
 *   config.strategy = DeploymentStrategy::CO_LOCATED;
 *   config.validate_compatibility = true;
 *   config.verify_signature = true;
 * 
 *   auto plan = deployment_mgr->planDeployment("legal_qa_v1", config);
 *   auto result = deployment_mgr->executeDeployment(plan);
 * 
 *   if (!result.success) {
 *       deployment_mgr->rollback(plan.adapter_id);
 *   }
 */
class AdapterDeploymentManager {
public:
    AdapterDeploymentManager(
        std::shared_ptr<ShardRouter> shard_router,
        std::shared_ptr<ShardTopology> shard_topology,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterCompatibilityValidator> validator
    );
    
    /**
     * @brief TBD: Describe ~AdapterDeploymentManager.
     * @return Return value.
     */
    virtual ~AdapterDeploymentManager() = default;
    
    /**
     * @brief Planning
     * @param[in] adapter_id Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    DeploymentPlan planDeployment(
        const std::string& adapter_id,
        const DeploymentConfig& config
    );
    
    /**
     * @brief TBD: Describe computePlacements.
     * @param[in] adapter_id Input parameter.
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
    
    /**
     * @brief Execution
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    DeploymentResult executeDeployment(const DeploymentPlan& plan);
    
    /**
     * @brief TBD: Describe deployToShard.
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    DeploymentResult deployToShard(
        const std::string& adapter_id,
        const std::string& shard_id,
        const DeploymentConfig& config
    );
    
    /**
     * @brief TBD: Describe undeployFromShard.
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
     * @return True on success.
     */
    bool undeployFromShard(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    /**
     * @brief Rollback & Recovery
     * @param[in] adapter_id Input parameter.
     * @return True on success.
     */
    bool rollback(const std::string& adapter_id);
    
    bool saveRollbackState(
        const std::string& adapter_id,
        const std::map<std::string, std::string>& previous_versions
    );
    
    /**
     * @brief TBD: Describe getRollbackState.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    std::optional<RollbackState> getRollbackState(const std::string& adapter_id);
    
    // Monitoring
    std::map<std::string, std::vector<std::string>> getAdapterDeployments() const;
    
    /**
     * @brief TBD: Describe getShardsForAdapter.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getShardsForAdapter(const std::string& adapter_id) const;
    
    /**
     * @brief TBD: Describe isDeployedOnShard.
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
     * @return True on success.
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
         * @brief TBD: Describe toJSON.
         * @return Return value.
         */
        std::string toJSON() const;
    };
    
    /**
     * @brief TBD: Describe getDeploymentStatuses.
     * @param[in] adapter_id Input parameter.
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
     * @brief TBD: Describe performHealthCheck.
     * @param[in] adapter_id Input parameter.
     * @return Return value.
     */
    std::vector<HealthCheckResult> performHealthCheck(const std::string& adapter_id);
    
    /**
     * @brief TBD: Describe verifyDeploymentIntegrity.
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
     * @return True on success.
     */
    bool verifyDeploymentIntegrity(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    /**
     * @brief Progress tracking
     * @param[in] callback Input parameter.
     */
    void setProgressCallback(DeploymentProgressCallback callback);
    
    /**
     * @brief TBD: Describe getDeploymentProgress.
     * @param[in] deployment_id Input parameter.
     * @return Return value.
     */
    float getDeploymentProgress(const std::string& deployment_id) const;
    
    /**
     * @brief Utility
     * @return Return value.
     */
    std::string generateDeploymentId() const;
    
    /**
     * @brief TBD: Describe validateDeploymentPlan.
     * @param[in] plan Input parameter.
     * @return True on success.
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
    
    /**
     * @brief Internal helpers
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
     * @return Return value.
     */
    float computeAffinityScore(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    /**
     * @brief TBD: Describe transferAdapterToShard.
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
     * @param[in] config Input parameter.
     * @return True on success.
     */
    bool transferAdapterToShard(
        const std::string& adapter_id,
        const std::string& shard_id,
        const DeploymentConfig& config
    );
    
    /**
     * @brief TBD: Describe notifyProgress.
     * @param[in] adapter_id Input parameter.
     * @param[in] shard_id Input parameter.
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
/** @brief Factory for creating deployment managers. */
class AdapterDeploymentManagerFactory {
public:
    /**
     * @brief TBD: Describe create.
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
