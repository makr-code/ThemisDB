/**
 * @file cross_cluster_federation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Configuration for a remote ThemisDB cluster endpoint.
 *
 * A cluster endpoint represents a separate ThemisDB instance that can
 * participate in cross-cluster federated AQL query execution.
 */
struct ClusterEndpoint {
    std::string cluster_id;       ///< Unique identifier for this cluster
    std::string base_url;         ///< HTTP base URL, e.g. "http://cluster-b:8080"
    std::string auth_token;       ///< Bearer token (empty = no auth)
    uint32_t    timeout_ms = 30000; ///< Per-request timeout in milliseconds

    /// Hint for cost estimation: approximate number of documents in this cluster.
    /// 0 means unknown (federation will estimate from a metadata probe).
    uint64_t estimated_rows_hint = 0;

    /// Approximate one-way network latency in milliseconds.
    /// 0 means unknown (federation will measure on first execution).
    double network_latency_hint_ms = 0.0;
};

/**
 * @brief Cost estimate for executing an AQL query on a single remote cluster.
 */
struct ClusterCostEstimate {
    std::string cluster_id;
    uint64_t    estimated_rows    = 0;
    double      network_latency_ms = 0.0;
    double      total_cost         = 0.0;   ///< Composite cost score (lower = cheaper)
    bool        should_include     = true;  ///< Whether to include this cluster in the query plan
};

/**
 * @brief Cross-cluster federated AQL execution engine with cost estimation.
 *
 * Executes AQL queries across multiple independent ThemisDB clusters by:
 * 1. Estimating per-cluster execution costs (row count + network latency).
 * 2. Selecting which clusters to query based on the cost model.
 * 3. Dispatching the AQL query in parallel to all selected clusters via HTTP POST.
 * 4. Merging and de-duplicating the results locally.
 *
 * Each remote cluster must expose the standard ThemisDB HTTP query endpoint:
 *   POST /query/aql   body: {"query": "<aql>"}
 *
 * Thread-safety: `execute()` and `estimateCosts()` are thread-safe.
 * `registerCluster()` / `unregisterCluster()` are safe to call concurrently.
 *
 * HTTP injection: for unit testing, supply a mock via `setHttpPostForTesting()`.
 */
class CrossClusterFederator {
public:
    /**
     * @brief Signature of the injectable HTTP POST function.
     *
     * @param url          Target URL
     * @param body         JSON request body (UTF-8)
     * @param auth_header  Value for the "Authorization" header (may be empty)
     * @param timeout_ms   Request timeout
     * @param response     Output: response body
     * @return             HTTP status code (0 on transport error)
     */
    using HttpPostFn = std::function<int(
        const std::string& url,
        const std::string& body,
        const std::string& auth_header,
        uint32_t           timeout_ms,
        std::string&       response
    )>;

    /**
     * @brief Federation-wide tuning parameters.
     */
    struct Config {
        bool     enable_parallel_execution = true;  ///< Query clusters in parallel
        uint32_t max_parallel_clusters     = 16;    ///< Thread-pool cap
        uint32_t default_timeout_ms        = 30000; ///< Default per-cluster timeout
        bool     skip_unreachable_clusters = true;  ///< Continue if some clusters fail
        /// Prune a cluster from the plan when its estimated cost exceeds this
        /// factor multiplied by the cheapest cluster's cost (0 = no pruning).
        double   cost_pruning_factor       = 0.0;
    };

    /**
     * @brief Execution plan produced by the cost model.
     */
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
    explicit CrossClusterFederator(const Config& config);

    // ----------------------------------------------------------------
    // Cluster registry
    // ----------------------------------------------------------------

    /**
     * @brief Register a remote cluster endpoint.
     *
     * Replaces any existing registration with the same `cluster_id`.
     */
    void registerCluster(const ClusterEndpoint& endpoint);

    /**
     * @brief Remove a previously registered cluster.
     *
     * No-op if the cluster_id is unknown.
     */
    void unregisterCluster(const std::string& cluster_id);

    /**
     * @brief Return all currently registered endpoints.
     */
    std::vector<ClusterEndpoint> listClusters() const;

    // ----------------------------------------------------------------
    // Cost estimation
    // ----------------------------------------------------------------

    /**
     * @brief Estimate per-cluster costs for executing `query`.
     *
     * Uses stored row-count hints and measured (or hinted) network latency.
     * Does NOT actually execute the query.
     *
     * Cost formula:
     *   total_cost = (estimated_rows * ROW_COST_WEIGHT)
     *              + (network_latency_ms * LATENCY_COST_WEIGHT)
     *
     * @param query  AQL query string (used for cardinality hints)
     * @return       One estimate per registered cluster, ordered cheapest first
     */
    std::vector<ClusterCostEstimate> estimateCosts(const std::string& query) const;

    /**
     * @brief Build an execution plan for `query` using the cost model.
     *
     * Applies cost-based pruning when `Config::cost_pruning_factor > 0`.
     */
    ExecutionPlan createExecutionPlan(const std::string& query) const;

    // ----------------------------------------------------------------
    // Query execution
    // ----------------------------------------------------------------

    /**
     * @brief Execute `query` across all (or selected) remote clusters.
     *
     * Dispatches the query via HTTP POST to each selected cluster's
     * `/query/aql` endpoint, then merges the result arrays.
     *
     * @param query  AQL query string
     * @return       JSON array containing the union of all cluster results.
     *               On partial failure the successfully retrieved results
     *               are still returned (when `Config::skip_unreachable_clusters`
     *               is true).
     * @throws std::runtime_error  When all clusters fail and skip is disabled.
     */
    nlohmann::json execute(const std::string& query);

    // ----------------------------------------------------------------
    // Statistics & diagnostics
    // ----------------------------------------------------------------

    /// Return execution statistics as a JSON object.
    nlohmann::json getStatistics() const;

    // ----------------------------------------------------------------
    // Test seam
    // ----------------------------------------------------------------

    /**
     * @brief Replace the real libcurl transport with a test double.
     *
     * Pass an empty `HttpPostFn{}` to restore the production implementation.
     */
    void setHttpPostForTesting(HttpPostFn fn);

private:
    // ----------------------------------------------------------------
    // Internal helpers
    // ----------------------------------------------------------------

    /**
     * @brief Dispatch query to a single cluster; return parsed JSON results.
     *
     * @param endpoint  Target cluster
     * @param query     AQL query string
     * @param[out] ok   Set to false on transport / HTTP error
     * @return          Parsed result array (empty on error)
     */
    nlohmann::json queryCluster(const ClusterEndpoint& endpoint,
                                const std::string&     query,
                                bool&                  ok);

    /**
     * @brief Merge result arrays from multiple clusters into a single array.
     */
    static nlohmann::json mergeResults(
        const std::vector<nlohmann::json>& shard_results);

    /**
     * @brief Execute the real HTTP POST via libcurl.
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
