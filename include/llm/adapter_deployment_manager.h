/**
 * @file adapter_deployment_manager.h
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
    
    std::string toJSON() const;
    static DeploymentConfig fromJSON(const std::string& json);
};

// Deployment result
struct DeploymentResult {
    virtual ~DeploymentResult() = default;
    bool success = false;
    std::string adapter_id;
    std::vector<std::string> deployed_shards;
    std::vector<std::string> failed_shards;
    std::map<std::string, std::string> shard_errors;
    int64_t deployment_time_ms = 0;
    size_t total_data_transferred_bytes = 0;
    std::string error_message;
    
    std::string toJSON() const;
};

// Shard affinity metrics
struct ShardAffinityMetrics {
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
    virtual ~DeploymentPlan() = default;
    std::string adapter_id;
    DeploymentStrategy strategy;
    std::vector<AdapterPlacement> placements;
    int estimated_total_time_seconds = 0;
    size_t total_bandwidth_required_mbps = 0;
    std::vector<std::string> prerequisites;  // e.g., "Adapter must be validated"
    std::string created_at;
    
    std::string toJSON() const;
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
    
    virtual ~AdapterDeploymentManager() = default;
    
    // Planning
    DeploymentPlan planDeployment(
        const std::string& adapter_id,
        const DeploymentConfig& config
    );
    
    std::vector<AdapterPlacement> computePlacements(
        const std::string& adapter_id,
        DeploymentStrategy strategy,
        const DeploymentConfig& config
    );
    
    std::map<std::string, ShardAffinityMetrics> analyzeShardAffinity(
        const std::string& adapter_id
    );
    
    // Execution
    DeploymentResult executeDeployment(const DeploymentPlan& plan);
    
    DeploymentResult deployToShard(
        const std::string& adapter_id,
        const std::string& shard_id,
        const DeploymentConfig& config
    );
    
    bool undeployFromShard(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    // Rollback & Recovery
    bool rollback(const std::string& adapter_id);
    
    bool saveRollbackState(
        const std::string& adapter_id,
        const std::map<std::string, std::string>& previous_versions
    );
    
    std::optional<RollbackState> getRollbackState(const std::string& adapter_id);
    
    // Monitoring
    std::map<std::string, std::vector<std::string>> getAdapterDeployments() const;
    
    std::vector<std::string> getShardsForAdapter(const std::string& adapter_id) const;
    
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
        
        std::string toJSON() const;
    };
    
    std::vector<DeploymentStatus> getDeploymentStatuses(
        const std::string& adapter_id
    ) const;
    
    // Health checks
    struct HealthCheckResult {
        std::string shard_id;
        bool is_healthy = false;
        std::string adapter_id;
        std::string issue_description;
        int64_t last_check_timestamp = 0;
    };
    
    std::vector<HealthCheckResult> performHealthCheck(const std::string& adapter_id);
    
    bool verifyDeploymentIntegrity(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    // Progress tracking
    void setProgressCallback(DeploymentProgressCallback callback);
    
    float getDeploymentProgress(const std::string& deployment_id) const;
    
    // Utility
    std::string generateDeploymentId() const;
    
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
    float computeAffinityScore(
        const std::string& adapter_id,
        const std::string& shard_id
    );
    
    bool transferAdapterToShard(
        const std::string& adapter_id,
        const std::string& shard_id,
        const DeploymentConfig& config
    );
    
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
    static std::unique_ptr<AdapterDeploymentManager> create(
        std::shared_ptr<ShardRouter> shard_router,
        std::shared_ptr<ShardTopology> shard_topology,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterCompatibilityValidator> validator
    );
};

} // namespace llm
