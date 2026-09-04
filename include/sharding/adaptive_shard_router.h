/**
 * @file adaptive_shard_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/shard_router.h"
#include "sharding/capability_matcher.h"
#include "sharding/shard_topology.h"
#include "distributed_knowledge/adapter_capability_announcement.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <optional>
#include <chrono>
#include <functional>
#include <optional>
#include <string_view>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * Adaptive Shard Router - Capability-based routing with iterative execution
 * 
 * Extends ShardRouter with intelligent shard selection based on:
 * 1. Capability matching (domain, organization, region, data type)
 * 2. Iterative query execution (rounds with decreasing thresholds)
 * 3. Adaptive stop criteria (sufficient results, timeout, diminishing returns)
 * 
 * Execution Model:
 * - Iteration 1: Top-N shards (score > initial_threshold)
 * - Iteration 2: Next-N shards (score > intermediate_threshold) - only if needed
 * - Iteration 3+: Fallback shards (score > fallback_threshold) - only if very needed
 * - Falls back to scatter-gather if feature disabled or no capability matches
 */
class AdaptiveShardRouter : public ShardRouter {
public:
    using ShardRouter::executeOnShards;

    /**
     * Configuration for adaptive routing
     */
    struct AdaptiveConfig {
        // Feature toggle
        bool enable_adaptive_routing = false;  // Enable/disable feature
        
        // Iteration parameters
        uint32_t max_iterations = 3;           // Maximum number of iterations
        uint32_t results_per_iteration = 3;    // Shards to query per iteration
        
        // Threshold parameters (relevance score thresholds)
        double initial_threshold = 0.8;        // Round 1 threshold
        double intermediate_threshold = 0.6;   // Round 2 threshold
        double fallback_threshold = 0.4;       // Round 3+ threshold
        
        // Stop criteria
        uint32_t target_result_count = 100;    // Target number of results
        double diminishing_returns_ratio = 0.1; // Stop if new results < 10% of previous
        uint32_t per_iteration_timeout_ms = 2000;  // Timeout per iteration
        uint32_t total_query_timeout_ms = 10000;   // Total query timeout
        uint32_t llm_load_freshness_ms = 30000;    // Max age for shard load snapshots
        
        // Fallback behavior
        bool fallback_to_scatter_gather = true;  // Fallback if no capability matches
        
        // Capability matcher configuration
        CapabilityMatcher::Config matcher_config;
        
        /**
         * Validate configuration
         */
        bool isValid() const {
            return max_iterations > 0 &&
                   results_per_iteration > 0 &&
                   initial_threshold >= intermediate_threshold &&
                   intermediate_threshold >= fallback_threshold &&
                   fallback_threshold > 0.0 &&
                   target_result_count > 0 &&
                   diminishing_returns_ratio > 0.0 &&
                   diminishing_returns_ratio < 1.0 &&
                   per_iteration_timeout_ms > 0 &&
                   llm_load_freshness_ms > 0 &&
                   total_query_timeout_ms >= per_iteration_timeout_ms &&
                   matcher_config.isValid();
        }
    };
    
    /**
     * Statistics for adaptive routing
     */
    struct IterationStats {
        uint32_t iteration_number = 0;
        uint32_t shards_queried;
        uint32_t results_received;
        uint64_t iteration_time_ms;
        double min_score;
        double max_score;
        double avg_score;
        std::vector<std::string> shard_ids;
    };
    
    struct AdaptiveStats {
        std::string query_id;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        uint64_t total_time_ms;
        uint32_t iterations_executed;
        uint32_t total_shards_queried;
        uint32_t total_results;
        bool used_adaptive_routing;
        bool stopped_early;
        std::string stop_reason;
        std::vector<IterationStats> iteration_details;
    };
    
    /**
     * Construct adaptive shard router
     * 
     * @param resolver URN resolver
     * @param executor Remote executor
     * @param topology Shard topology (for capability access)
     * @param config Base router configuration
     * @param adaptive_config Adaptive routing configuration
     * @param metrics Optional Prometheus metrics
     * @param truetime Optional TrueTime
     */
    AdaptiveShardRouter(
        std::shared_ptr<URNResolver> resolver,
        std::shared_ptr<RemoteExecutor> executor,
        std::shared_ptr<ShardTopology> topology,
        const Config& config,
        const AdaptiveConfig& adaptive_config,
        std::shared_ptr<PrometheusMetrics> metrics = nullptr,
        std::shared_ptr<TrueTime> truetime = nullptr
    );

    AdaptiveShardRouter(
        std::shared_ptr<URNResolver> resolver,
        std::shared_ptr<RemoteExecutor> executor,
        std::shared_ptr<ShardTopology> topology,
        const Config& config,
        std::shared_ptr<PrometheusMetrics> metrics = nullptr,
        std::shared_ptr<TrueTime> truetime = nullptr
    );
    
    /**
     * Execute query with adaptive routing
     * 
     * Overrides base ShardRouter::executeQuery() to use capability-based
     * routing when enabled, falling back to scatter-gather otherwise.
     * 
     * @param query Query string
     * @return Combined results
     */
    nlohmann::json executeQuery(const std::string& query) override;
    
    /**
     * Execute query with adaptive iterative shard selection
     * 
     * @param query Query string
     * @param stats Output: execution statistics
     * @return Combined results
     */
    nlohmann::json executeAdaptiveQuery(
        const std::string& query,
        AdaptiveStats& stats
    );
    
    /**
     * Update the domain-score map for a shard based on a gossip announcement.
     *
     * Called by the `GossipProtocol` custom handler registered for
     * `message_type = "adapter_capability"`.  Thread-safe.
     *
     * @param shard_id      Originating shard identifier (from GossipMessage::sender_id)
     * @param announcement  Deserialized capability announcement
     */
    void updateAdapterCapability(
        const std::string& shard_id,
        const themis::distributed_knowledge::AdapterCapabilityAnnouncement& announcement
    );

    /**
     * Update per-shard LLM queue load snapshot used for LEAST_LOADED tie-breaking
     * in routeByDomain().
     *
     * Called after each `ContinuousBatchScheduler::getLLMStats()` poll or
     * whenever a ShardStats gossip message is received.  Thread-safe.
     *
     * @param shard_id         Shard identifier
     * @param pending_requests Current waiting-request count on that shard
     * @param avg_queue_ms     Current average queue wait time on that shard
     */
    void updateShardLLMLoad(
        const std::string& shard_id,
        uint64_t pending_requests,
        double avg_queue_ms
    );

    /**
     * Route a query to the shard with the highest `accuracy_delta` for the
     * given adapter domain.
     *
     * When two shards share the same best accuracy_delta the one with the
     * lower `pending_llm_requests` (LEAST_LOADED) wins.  Fallback: if no
     * shard has registered a score for `domain`, the method returns an empty
    * string and callers should use the default `route()` behaviour.
    *
    * Load tie-break semantics:
    * - Fresh queue snapshots win over stale/missing snapshots.
    * - For equally fresh snapshots with equal score, lower pending queue wins.
    * - Remaining ties are resolved deterministically by lexical shard_id order.
     *
     * @param domain  Domain type to look up
     * @return shard_id of the best-scoring shard, or "" if no score exists
     */
    std::string routeByDomain(
        themis::distributed_knowledge::AdapterDomainType domain
    ) const;

    /**
     * Return the `accuracy_delta` registered for a specific shard + domain pair.
     *
     * Returns 0.0 when the shard is unknown or has no score for the domain
     * (used by `FederatedRAGMerger` in DK-4).
     */
    double getAdapterAccuracyDelta(
        const std::string& shard_id,
        themis::distributed_knowledge::AdapterDomainType domain
    ) const;

    /**
     * Get adaptive routing statistics
     * @return Statistics JSON
     */
    nlohmann::json getAdaptiveStatistics() const;
    
    /**
     * Update adaptive configuration at runtime
     * @param config New configuration
     */
    void updateAdaptiveConfig(const AdaptiveConfig& config);

    /**
     * @brief Inject NLP/ML query-context enrichment callback.
     *
     * The callback receives the raw query text and the current query context
     * (already populated with extracted keywords), and can enrich domains,
     * regions, organizations, data types, or embeddings in-place.
     *
     * @param fn NLP/ML enrichment function.
     */
    using NlpContextFn = std::function<void(
        const std::string& query,
        CapabilityMatcher::QueryContext& context
    )>;

    /**
     * @brief Backward-compatible NLP callback variant.
     *
     * The callback may return a fully prepared QueryContext. Returning
     * std::nullopt keeps the existing heuristic/extracted context.
     */
    using LegacyNlpContextFn = std::function<std::optional<CapabilityMatcher::QueryContext>(
        std::string_view query
    )>;

    void setNlpContextFn(NlpContextFn fn);
    void setNlpContextFn(LegacyNlpContextFn fn);
    
    /**
     * Get current adaptive configuration
     * @return Current configuration
     */
    AdaptiveConfig getAdaptiveConfig() const {
        return adaptive_config_;
    }

private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<CapabilityMatcher> matcher_;
    AdaptiveConfig adaptive_config_;

    // Domain-score map: shard_id → { domain_type → accuracy_delta }
    // Updated via updateAdapterCapability(); consulted by routeByDomain().
    std::map<std::string,
             std::map<themis::distributed_knowledge::AdapterDomainType, double>>
        shard_domain_scores_;

    // LLM load map: shard_id → { pending_requests, avg_queue_ms }
    // Updated via updateShardLLMLoad(); used as LEAST_LOADED tie-breaker in routeByDomain().
    struct ShardLLMLoad {
        uint64_t pending_requests = 0;
        double   avg_queue_ms    = 0.0;
        std::chrono::steady_clock::time_point updated_at{};
    };
    std::map<std::string, ShardLLMLoad> shard_llm_load_;

    mutable std::mutex domain_scores_mutex_;

    std::optional<NlpContextFn> nlp_context_fn_;
    mutable std::mutex nlp_context_fn_mutex_;
    
    // Statistics
    mutable std::atomic<uint64_t> total_adaptive_queries_{0};
    mutable std::atomic<uint64_t> iterations_saved_{0};
    mutable std::atomic<uint64_t> early_stops_{0};
    mutable std::atomic<uint64_t> fallback_to_scatter_gather_{0};
    /**
     * Prepare query context for capability matching
     * 
     * Extracts keywords, detects domains/orgs/regions, etc.
     * 
     * @param query Query string
     * @return Query context
     */
    CapabilityMatcher::QueryContext prepareQueryContext(const std::string& query);
    
    /**
     * Select shards for an iteration based on capability scores
     * 
     * @param match_results All capability match results
     * @param threshold Score threshold for this iteration
     * @param max_shards Maximum shards to select
     * @param already_queried Set of already queried shard IDs
     * @return List of shard IDs to query in this iteration
     */
    std::vector<std::string> selectShardsForIteration(
        const std::vector<CapabilityMatchResult>& match_results,
        double threshold,
        size_t max_shards,
        const std::set<std::string>& already_queried
    );
    
    /**
     * Execute query on specific shards
     * 
     * @param query Query string
     * @param shard_ids Shard IDs to query
     * @param timeout_ms Timeout for this iteration
     * @return Shard results
     */
    std::vector<ShardResult> executeOnShards(
        const std::string& query,
        const std::vector<std::string>& shard_ids,
        uint32_t timeout_ms
    );

    std::vector<ShardResult> executeOnShards(
        const std::string& query,
        const std::vector<std::string>& shard_ids
    ) override;
    
    /**
     * Check if stop criteria are met
     * 
     * @param current_results Current result count
     * @param previous_results Previous iteration result count
     * @param elapsed_ms Total elapsed time
     * @param iteration Current iteration number
     * @param reason Output: stop reason if stopping
     * @return true if should stop
     */
    bool shouldStop(
        uint32_t current_results,
        uint32_t previous_results,
        uint64_t elapsed_ms,
        uint32_t iteration,
        std::string& reason
    );
    
    /**
     * Merge and deduplicate results from multiple iterations
     * 
     * @param all_results Results from all iterations
     * @return Merged results
     */
    nlohmann::json mergeIterationResults(
        const std::vector<std::vector<ShardResult>>& all_results
    );
    
    /**
     * Calculate iteration statistics
     * 
     * @param iteration Iteration number
     * @param shard_ids Queried shard IDs
     * @param results Shard results
     * @param match_results Capability match results
     * @param iteration_time_ms Iteration execution time
     * @return Iteration statistics
     */
    IterationStats calculateIterationStats(
        uint32_t iteration,
        const std::vector<std::string>& shard_ids,
        const std::vector<ShardResult>& results,
        const std::vector<CapabilityMatchResult>& match_results,
        uint64_t iteration_time_ms
    );
};

} // namespace themis::sharding
