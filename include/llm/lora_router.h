/**
 * @file lora_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/adapter_load_balancer.h"
#include "llm/multi_lora_manager.h"
#include "llm/adapter_registry.h"
#include "llm/lora_framework/embedding_provider.h"
#include "llm/decision_record_yaml_processor.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

enum class RoutingPolicy {
    SEMANTIC,           // Pure semantic similarity
    LOAD_AWARE,         // Semantic + GPU load balancing
    AB_TEST,            // A/B testing between adapters
    ROLLOUT,            // Incremental rollout
    FALLBACK            // Default fallback adapter
};

struct ABTestConfig {
    std::vector<std::string> adapter_ids;  // Adapters to test
    std::vector<float> traffic_splits;     // Traffic split percentages (must sum to 1.0)
    std::string experiment_id;             // Unique experiment identifier
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    bool enabled = false;
};

struct RolloutConfig {
    std::string new_adapter_id;            // New adapter being rolled out
    std::string baseline_adapter_id;       // Baseline adapter
    float rollout_percentage = 0.0f;       // Current rollout percentage (0.0 - 1.0)
    float increment_step = 0.1f;           // Increment per step (e.g., 10%)
    std::chrono::seconds increment_interval{3600};  // Time between increments
    std::chrono::system_clock::time_point last_increment;
    std::chrono::system_clock::time_point start_time;
    bool enabled = false;
};

struct FallbackConfig {
    std::string default_adapter_id;        // Default adapter to use
    float similarity_threshold = 0.5f;     // Minimum similarity for non-fallback
    bool enable_fallback = true;
};

struct RoutingDecision {
    std::string adapter_id;                // Selected adapter
    std::string base_model_id;             // Base model to use
    int gpu_device_id = -1;                // Selected GPU (-1 = unassigned)
    float similarity_score = 0.0f;         // Semantic similarity score
    float confidence = 0.0f;               // Decision confidence
    RoutingPolicy policy_used;             // Policy that made the decision
    bool is_fallback = false;              // Whether fallback was used
    std::string reason;                    // Decision reason/explanation
    std::chrono::milliseconds routing_latency_ms{0};  // Routing decision latency
};

struct RoutingMetrics {
    /**
     * @brief Routing Metrics.
     * @return Return value.
     */
    virtual ~RoutingMetrics() = default;
    size_t total_requests = 0;
    size_t successful_routes = 0;
    size_t fallback_routes = 0;
    std::unordered_map<std::string, size_t> adapter_usage_count;
    std::unordered_map<std::string, double> adapter_avg_similarity;
    double avg_routing_latency_ms = 0.0;
    double avg_similarity_score = 0.0;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

class LoRARouter {
public:
    struct Config {
        // Semantic routing
        bool enable_semantic_routing = true;
        size_t top_k_candidates = 5;           // Top K similar adapters to consider
        float min_similarity_threshold = 0.3f;  // Minimum similarity to consider
        
        // Load-aware routing
        bool enable_load_aware = true;
        float load_weight = 0.3f;              // Weight for load in scoring (0.0-1.0)
        
        // Policies
        RoutingPolicy default_policy = RoutingPolicy::LOAD_AWARE;
        
        // Fallback
        FallbackConfig fallback;
        
        // Metrics
        bool enable_metrics = true;
        size_t metrics_window_size = 1000;     // Rolling window for metrics
        
        // Caching
        bool enable_decision_cache = true;
        size_t decision_cache_size = 1000;
        std::chrono::seconds decision_cache_ttl{300};  // 5 minutes
    };
    
    /**
     * @brief Lo RARouter.
     * @param[in] embedding_provider Input parameter.
     * @param[in] adapter_registry Input parameter.
     * @param[in] load_balancer Input parameter.
     * @param[in] lora_manager Input parameter.
     * @return Return value.
     */
    explicit LoRARouter(
        std::shared_ptr<lora::EmbeddingProvider> embedding_provider,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterLoadBalancer> load_balancer,
        std::shared_ptr<MultiLoRAManager> lora_manager
    );
    /**
     * @brief Lo RARouter.
     * @param[in] embedding_provider Input parameter.
     * @param[in] adapter_registry Input parameter.
     * @param[in] load_balancer Input parameter.
     * @param[in] lora_manager Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRARouter(
        std::shared_ptr<lora::EmbeddingProvider> embedding_provider,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterLoadBalancer> load_balancer,
        std::shared_ptr<MultiLoRAManager> lora_manager,
        const Config& config
    );
    
    ~LoRARouter();
    
    RoutingDecision routeQuery(
        const std::string& query,
        const std::string& base_model_id = "",
        std::optional<RoutingPolicy> policy = std::nullopt
    );
    
    std::vector<RoutingDecision> routeQueryBatch(
        const std::vector<std::string>& queries,
        const std::string& base_model_id = ""
    );
    
    // Policy Management
    
    /**
     * @brief Configure ABTest.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool configureABTest(const ABTestConfig& config);
    
    /**
     * @brief Get ABTest Config.
     * @return Return value.
     */
    std::optional<ABTestConfig> getABTestConfig() const;
    
    /**
     * @brief End ABTest.
     */
    void endABTest();
    
    /**
     * @brief Configure Rollout.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool configureRollout(const RolloutConfig& config);
    
    /**
     * @brief Get Rollout Config.
     * @return Return value.
     */
    std::optional<RolloutConfig> getRolloutConfig() const;
    
    /**
     * @brief Increment Rollout.
     * @return Return value.
     */
    float incrementRollout();
    
    void endRollout(bool promote = true);
    
    /**
     * @brief Configure Fallback.
     * @param[in] config Input parameter.
     */
    void configureFallback(const FallbackConfig& config);
    
    /**
     * @brief Get Fallback Config.
     * @return Return value.
     */
    FallbackConfig getFallbackConfig() const;
    
    // Metrics & Monitoring
    
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    RoutingMetrics getMetrics() const;
    
    /**
     * @brief Reset Metrics.
     */
    void resetMetrics();
    
    /**
     * @brief Export Metrics.
     * @return Return value.
     */
    json exportMetrics() const;
    
    // Cache Management
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    json getCacheStats() const;

    /**
     * @brief Set Decision Record Processor.
     * @param[in] processor Input parameter.
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<DecisionRecordYamlProcessor> processor);
    
private:
    Config config_;
    std::shared_ptr<lora::EmbeddingProvider> embedding_provider_;
    std::shared_ptr<AdapterRegistry> adapter_registry_;
    std::shared_ptr<AdapterLoadBalancer> load_balancer_;
    std::shared_ptr<MultiLoRAManager> lora_manager_;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<DecisionRecordYamlProcessor> dr_processor_;
    
    mutable std::mutex mutex_;
    
    // Policies
    std::optional<ABTestConfig> ab_test_config_;
    std::optional<RolloutConfig> rollout_config_;
    FallbackConfig fallback_config_;
    
    // Metrics
    RoutingMetrics metrics_;
    std::vector<double> recent_latencies_;
    std::vector<double> recent_similarities_;
    
    // Decision cache: query hash -> decision
    struct CachedDecision {
        RoutingDecision decision;
        std::chrono::system_clock::time_point cached_at;
    };
    std::unordered_map<std::string, CachedDecision> decision_cache_;
    
    // Internal routing methods
    
    std::vector<std::pair<std::string, float>> findSemanticCandidates(
        const std::string& query,
        const std::string& base_model_id
    );
    
    RoutingDecision applyRoutingPolicy(
        const std::string& query,
        const std::vector<std::pair<std::string, float>>& candidates,
        RoutingPolicy policy,
        const std::string& base_model_id
    );
    
    RoutingDecision selectBySemantic(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    RoutingDecision selectByLoadAware(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    RoutingDecision selectByABTest(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    RoutingDecision selectByRollout(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    /**
     * @brief Select Fallback.
     * @param[in] reason Input parameter.
     * @return Return value.
     */
    RoutingDecision selectFallback(const std::string& reason);
    
    /**
     * @brief Cosine Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    float cosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const;
    
    /**
     * @brief Update Metrics.
     * @param[in] decision Input parameter.
     */
    void updateMetrics(const RoutingDecision& decision);
    
    /**
     * @brief Get Cached Decision.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::optional<RoutingDecision> getCachedDecision(const std::string& query);
    
    /**
     * @brief Cache Decision.
     * @param[in] query Input parameter.
     * @param[in] decision Input parameter.
     */
    void cacheDecision(const std::string& query, const RoutingDecision& decision);
    
    /**
     * @brief Hash Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string hashQuery(const std::string& query) const;
    
    /**
     * @brief Is ABTest Active.
     * @return True when the operation succeeds.
     */
    bool isABTestActive() const;
    
    /**
     * @brief Is Rollout Active.
     * @return True when the operation succeeds.
     */
    bool isRolloutActive() const;
    
    /**
     * @brief Evict Expired Cache.
     */
    void evictExpiredCache();

    /**
     * @brief Emit Adapter Selection Record.
     * @param[in] decision Input parameter.
     */
    void emitAdapterSelectionRecord(const RoutingDecision& decision) const;
};

} // namespace llm
} // namespace themis

