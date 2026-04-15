/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_router.h                                      ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     454                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/adapter_load_balancer.h"
#include "llm/multi_lora_manager.h"
#include "llm/adapter_registry.h"
#include "llm/lora_framework/embedding_provider.h"
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

/**
 * @brief Routing policy for adapter selection
 */
enum class RoutingPolicy {
    SEMANTIC,           // Pure semantic similarity
    LOAD_AWARE,         // Semantic + GPU load balancing
    AB_TEST,            // A/B testing between adapters
    ROLLOUT,            // Incremental rollout
    FALLBACK            // Default fallback adapter
};

/**
 * @brief A/B testing configuration
 */
struct ABTestConfig {
    std::vector<std::string> adapter_ids;  // Adapters to test
    std::vector<float> traffic_splits;     // Traffic split percentages (must sum to 1.0)
    std::string experiment_id;             // Unique experiment identifier
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    bool enabled = false;
};

/**
 * @brief Incremental rollout configuration
 */
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

/**
 * @brief Fallback configuration
 */
struct FallbackConfig {
    std::string default_adapter_id;        // Default adapter to use
    float similarity_threshold = 0.5f;     // Minimum similarity for non-fallback
    bool enable_fallback = true;
};

/**
 * @brief Routing decision result
 */
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

/**
 * @brief Routing metrics for monitoring
 */
struct RoutingMetrics {
    size_t total_requests = 0;
    size_t successful_routes = 0;
    size_t fallback_routes = 0;
    std::unordered_map<std::string, size_t> adapter_usage_count;
    std::unordered_map<std::string, double> adapter_avg_similarity;
    double avg_routing_latency_ms = 0.0;
    double avg_similarity_score = 0.0;
    
    json toJson() const;
};

/**
 * @brief LoRA Router - Automatic LoRA-to-LLM Routing Automation
 * 
 * Provides intelligent routing of queries to optimal LoRA adapters based on:
 * - Semantic similarity (embedding-based matching)
 * - Load-aware balancing (GPU health and utilization)
 * - A/B testing policies (traffic splitting for experiments)
 * - Incremental rollout (gradual deployment of new adapters)
 * - Fallback policies (default adapter when no good match)
 * 
 * Features:
 * - Automatic adapter selection via semantic query embedding
 * - Multi-LLM routing support
 * - Load-aware routing integrated with AdapterLoadBalancer
 * - Policy engine for A/B testing and rollouts
 * - Comprehensive metrics (adapter usage, fallback rate, latency)
 * - Audit logging for routing decisions
 * 
 * Architecture:
 *   Query → Embed → Find Similar Adapters → Apply Policy → 
 *   → Check Load → Route to GPU → Log Decision → Return
 */
class LoRARouter {
public:
    /**
     * @brief Configuration for LoRA router
     */
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
     * @brief Construct LoRA router
     * 
     * @param embedding_provider Provider for query embeddings
     * @param adapter_registry Registry for adapter metadata
     * @param load_balancer Load balancer for GPU-aware placement
     * @param lora_manager Multi-LoRA manager for adapter operations
     * @param config Router configuration
     */
    explicit LoRARouter(
        std::shared_ptr<lora::EmbeddingProvider> embedding_provider,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterLoadBalancer> load_balancer,
        std::shared_ptr<MultiLoRAManager> lora_manager
    );
    explicit LoRARouter(
        std::shared_ptr<lora::EmbeddingProvider> embedding_provider,
        std::shared_ptr<AdapterRegistry> adapter_registry,
        std::shared_ptr<AdapterLoadBalancer> load_balancer,
        std::shared_ptr<MultiLoRAManager> lora_manager,
        const Config& config
    );
    
    ~LoRARouter();
    
    /**
     * @brief Route a query to the optimal adapter
     * 
     * Main routing function that:
     * 1. Embeds the query using EmbeddingProvider
     * 2. Finds semantically similar adapters
     * 3. Applies routing policy (load-aware, A/B test, etc.)
     * 4. Selects GPU for placement
     * 5. Returns routing decision with metrics
     * 
     * @param query Input query/prompt
     * @param base_model_id Base model to use (optional, auto-detect if empty)
     * @param policy Routing policy override (optional)
     * @return Routing decision with selected adapter and GPU
     */
    RoutingDecision routeQuery(
        const std::string& query,
        const std::string& base_model_id = "",
        std::optional<RoutingPolicy> policy = std::nullopt
    );
    
    /**
     * @brief Route multiple queries in batch
     * 
     * Efficient batch routing for multiple queries.
     * 
     * @param queries Vector of input queries
     * @param base_model_id Base model to use (optional)
     * @return Vector of routing decisions
     */
    std::vector<RoutingDecision> routeQueryBatch(
        const std::vector<std::string>& queries,
        const std::string& base_model_id = ""
    );
    
    // Policy Management
    
    /**
     * @brief Configure A/B testing
     * 
     * Sets up A/B test between multiple adapters with traffic splits.
     * 
     * @param config A/B test configuration
     * @return true if configuration successful
     */
    bool configureABTest(const ABTestConfig& config);
    
    /**
     * @brief Get current A/B test configuration
     */
    std::optional<ABTestConfig> getABTestConfig() const;
    
    /**
     * @brief End A/B test
     */
    void endABTest();
    
    /**
     * @brief Configure incremental rollout
     * 
     * Sets up gradual rollout of new adapter.
     * 
     * @param config Rollout configuration
     * @return true if configuration successful
     */
    bool configureRollout(const RolloutConfig& config);
    
    /**
     * @brief Get current rollout configuration
     */
    std::optional<RolloutConfig> getRolloutConfig() const;
    
    /**
     * @brief Increment rollout percentage
     * 
     * Manually advance rollout to next percentage.
     * 
     * @return New rollout percentage
     */
    float incrementRollout();
    
    /**
     * @brief End rollout (promote to 100% or rollback)
     * 
     * @param promote If true, promote to 100%. If false, rollback.
     */
    void endRollout(bool promote = true);
    
    /**
     * @brief Configure fallback adapter
     * 
     * @param config Fallback configuration
     */
    void configureFallback(const FallbackConfig& config);
    
    /**
     * @brief Get fallback configuration
     */
    FallbackConfig getFallbackConfig() const;
    
    // Metrics & Monitoring
    
    /**
     * @brief Get routing metrics
     * 
     * Returns comprehensive metrics about routing decisions.
     * 
     * @return Routing metrics
     */
    RoutingMetrics getMetrics() const;
    
    /**
     * @brief Reset metrics
     */
    void resetMetrics();
    
    /**
     * @brief Export metrics as JSON
     */
    json exportMetrics() const;
    
    // Cache Management
    
    /**
     * @brief Clear decision cache
     */
    void clearCache();
    
    /**
     * @brief Get cache statistics
     */
    json getCacheStats() const;
    
private:
    Config config_;
    std::shared_ptr<lora::EmbeddingProvider> embedding_provider_;
    std::shared_ptr<AdapterRegistry> adapter_registry_;
    std::shared_ptr<AdapterLoadBalancer> load_balancer_;
    std::shared_ptr<MultiLoRAManager> lora_manager_;
    
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
    
    /**
     * @brief Find candidate adapters using semantic similarity
     */
    std::vector<std::pair<std::string, float>> findSemanticCandidates(
        const std::string& query,
        const std::string& base_model_id
    );
    
    /**
     * @brief Apply routing policy to select adapter
     */
    RoutingDecision applyRoutingPolicy(
        const std::string& query,
        const std::vector<std::pair<std::string, float>>& candidates,
        RoutingPolicy policy,
        const std::string& base_model_id
    );
    
    /**
     * @brief Select adapter using semantic similarity
     */
    RoutingDecision selectBySemantic(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    /**
     * @brief Select adapter using load-aware policy
     */
    RoutingDecision selectByLoadAware(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    /**
     * @brief Select adapter using A/B test policy
     */
    RoutingDecision selectByABTest(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    /**
     * @brief Select adapter using rollout policy
     */
    RoutingDecision selectByRollout(
        const std::vector<std::pair<std::string, float>>& candidates
    );
    
    /**
     * @brief Select fallback adapter
     */
    RoutingDecision selectFallback(const std::string& reason);
    
    /**
     * @brief Calculate cosine similarity between embeddings
     */
    float cosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const;
    
    /**
     * @brief Update routing metrics
     */
    void updateMetrics(const RoutingDecision& decision);
    
    /**
     * @brief Get cached decision if available
     */
    std::optional<RoutingDecision> getCachedDecision(const std::string& query);
    
    /**
     * @brief Add decision to cache
     */
    void cacheDecision(const std::string& query, const RoutingDecision& decision);
    
    /**
     * @brief Compute hash for query (for caching)
     */
    std::string hashQuery(const std::string& query) const;
    
    /**
     * @brief Check if AB test is active
     */
    bool isABTestActive() const;
    
    /**
     * @brief Check if rollout is active
     */
    bool isRolloutActive() const;
    
    /**
     * @brief Evict expired cache entries
     */
    void evictExpiredCache();
};

} // namespace llm
} // namespace themis
