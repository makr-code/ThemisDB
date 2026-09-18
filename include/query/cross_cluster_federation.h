/**
 * @file cross_cluster_federation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis::query {

struct ClusterEndpoint {
    std::string cluster_id;       ///< Unique identifier for this cluster
    std::string base_url;         ///< HTTP base URL, e.g. "http://cluster-b:8080"
    std::string auth_token;       ///< Bearer token (empty = no auth)
    uint32_t    timeout_ms = 30000; ///< Per-request timeout in milliseconds

    uint64_t estimated_rows_hint = 0;

    double network_latency_hint_ms = 0.0;
};

struct ClusterCostEstimate {
    std::string cluster_id;
    uint64_t    estimated_rows    = 0;
    double      network_latency_ms = 0.0;
    double      total_cost         = 0.0;   ///< Composite cost score (lower = cheaper)
    bool        should_include     = true;  ///< Whether to include this cluster in the query plan
};

class CrossClusterFederator {
public:
    using HttpPostFn = std::function<int(
        const std::string& url,
        const std::string& body,
        const std::string& auth_header,
        uint32_t           timeout_ms,
        std::string&       response
    )>;

    struct Config {
        bool     enable_parallel_execution = true;  ///< Query clusters in parallel
        uint32_t max_parallel_clusters     = 16;    ///< Thread-pool cap
        uint32_t default_timeout_ms        = 30000; ///< Default per-cluster timeout
        bool     skip_unreachable_clusters = true;  ///< Continue if some clusters fail
        double   cost_pruning_factor       = 0.0;
    };

    struct ExecutionPlan {
        std::vector<std::string>       selected_clusters; ///< Cluster IDs to query
        std::vector<ClusterCostEstimate> cost_estimates;
        double                          total_estimated_cost = 0.0;
        std::string                     merge_strategy;   ///< "union" (default)
    };

    // ----------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------

    CrossClusterFederator();
    /**
     * @brief Cross Cluster Federator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CrossClusterFederator(const Config& config);

    // ----------------------------------------------------------------
    // Cluster registry
    // ----------------------------------------------------------------

    /**
     * @brief Register Cluster.
     * @param[in] endpoint Input parameter.
     */
    void registerCluster(const ClusterEndpoint& endpoint);

    /**
     * @brief Unregister Cluster.
     * @param[in] cluster_id Identifier of the cluster.
     */
    void unregisterCluster(const std::string& cluster_id);

    /**
     * @brief List Clusters.
     * @return Return value.
     */
    std::vector<ClusterEndpoint> listClusters() const;

    // ----------------------------------------------------------------
    // Cost estimation
    // ----------------------------------------------------------------

    /**
     * @brief Estimate Costs.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::vector<ClusterCostEstimate> estimateCosts(const std::string& query) const;

    /**
     * @brief Create Execution Plan.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    ExecutionPlan createExecutionPlan(const std::string& query) const;

    // ----------------------------------------------------------------
    // Query execution
    // ----------------------------------------------------------------

    /**
     * @brief Execute.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    nlohmann::json execute(const std::string& query);

    // ----------------------------------------------------------------
    // Statistics & diagnostics
    // ----------------------------------------------------------------

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    nlohmann::json getStatistics() const;

    // ----------------------------------------------------------------
    // Test seam
    // ----------------------------------------------------------------

    /**
     * @brief Set Http Post For Testing.
     * @param[in] fn Input parameter.
     */
    void setHttpPostForTesting(HttpPostFn fn);

private:
    // ----------------------------------------------------------------
    // Internal helpers
    // ----------------------------------------------------------------

    /**
     * @brief Query Cluster.
     * @param[in] endpoint Input parameter.
     * @param[in] query Input parameter.
     * @param[in,out] ok Input/output parameter.
     * @return Return value.
     */
    nlohmann::json queryCluster(const ClusterEndpoint& endpoint,
                                const std::string&     query,
                                bool&                  ok);

    /**
     * @brief Merge Results.
     * @param[in] shard_results Input parameter.
     * @return Return value.
     */
    static nlohmann::json mergeResults(
        const std::vector<nlohmann::json>& shard_results);

    /**
     * @brief Curl Http Post.
     * @param[in] url Input parameter.
     * @param[in] body Input parameter.
     * @param[in] auth_header Input parameter.
     * @param[in] timeout_ms Input parameter.
     * @param[in,out] response Input/output parameter.
     * @return Return value.
     */
    static int curlHttpPost(const std::string& url,
                            const std::string& body,
                            const std::string& auth_header,
                            uint32_t           timeout_ms,
                            std::string&       response);

    // ----------------------------------------------------------------
    // Cost model constants
    // ----------------------------------------------------------------

    static constexpr double ROW_COST_WEIGHT     = 0.001; ///< ms per row
    static constexpr double LATENCY_COST_WEIGHT = 1.0;   ///< ms per ms latency

    // Default assumed latency when no hint is available (local LAN round-trip)
    static constexpr double DEFAULT_LATENCY_MS  = 5.0;
    // Default assumed row count when no hint is available
    static constexpr uint64_t DEFAULT_ROWS      = 100'000;

    // ----------------------------------------------------------------
    // Members
    // ----------------------------------------------------------------

    Config config_;

    mutable std::mutex registry_mutex_;
    std::unordered_map<std::string, ClusterEndpoint> clusters_;

    // Measured latency cache (cluster_id → ms)
    mutable std::unordered_map<std::string, double> latency_cache_;

    // Statistics
    std::atomic<uint64_t> total_queries_{0};
    std::atomic<uint64_t> successful_queries_{0};
    std::atomic<uint64_t> failed_cluster_requests_{0};

    // Injectable HTTP transport (empty = use real libcurl)
    HttpPostFn http_post_fn_;
};

} // namespace themis::query
