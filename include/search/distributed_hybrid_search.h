/**
 * @file distributed_hybrid_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.2.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Status: Production Ready - v2.2.0 Phase 2 (Core Implementation Hardening)
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "search/hybrid_search.h"
#include "search/search_error_codes.h"
#include "sharding/remote_executor.h"
#include "sharding/urn_resolver.h"
#include "sharding/shard_topology.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <atomic>
#include <functional>
#include "query/request_context.h"

namespace themis {

/**
 * @brief Distributed hybrid search across multiple database shards.
 *
 * DistributedHybridSearch extends the local HybridSearch engine to operate
 * across a cluster of ThemisDB shards.  On each `search()` call it:
 *
 * 1. Runs a local `HybridSearch` on this node.
 * 2. Dispatches the same query in parallel to every healthy remote shard via
 *    `RemoteExecutor::post()` (REST, mTLS-secured).
 * 3. Merges all per-shard result lists into a single global ranking using
 *    cross-shard Reciprocal Rank Fusion (RRF).
 * 4. Returns the top-k globally merged results.
 *
 * ### Fault tolerance
 *
 * When `Config::skip_failed_shards` is true (the default), shards that time
 * out, return HTTP errors, or are unreachable are silently skipped; the merged
 * result is still returned from the surviving shards.  Callers can inspect
 * `SearchStats::shards_failed` to detect degraded results.
 *
 * ### mTLS
 *
 * All inter-node traffic is routed through the injected `RemoteExecutor`,
 * which was constructed with mTLS credentials.  No additional TLS
 * configuration is required here.
 *
 * ### Thread safety
 *
 * A single `DistributedHybridSearch` instance is NOT thread-safe.  Concurrent
 * `search()` calls must be serialised externally, or separate instances should
 * be used per thread.
 *
 * ### API path
 *
 * Remote shards are queried via HTTP POST to `Config::search_endpoint`
 * (default: `/api/v1/search/hybrid`).  The request body is a JSON object:
 * ```json
 * { "query": "<text>", "k": <int>, "vector_query": [<float>, ...] }
 * ```
 * The response must be a JSON array of result objects, each with fields
 * `document_id`, `bm25_score`, `vector_score`, `hybrid_score`, `bm25_rank`,
 * `vector_rank`, and optionally `content`.
 *
 * @since v2.3.0
 */
class DistributedHybridSearch {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Maximum number of globally merged results to return.
        size_t k = 10;

        /// RRF smoothing constant for cross-shard rank fusion (default 60).
        double rrf_k = 60.0;

        /// Per-shard request timeout in milliseconds.
        uint32_t shard_timeout_ms = 5000;

        /// Maximum number of shards queried concurrently.
        size_t max_concurrent_shards = 10;

        /// When true, failed shards are skipped; results come from
        /// surviving shards only.  When false, any shard failure causes
        /// `search()` to return an empty result vector.
        bool skip_failed_shards = true;

        /// Shard ID of this node.  Results from this shard come from the
        /// local `HybridSearch` instance, not via the network.
        std::string local_shard_id;

        /// HTTP endpoint on each shard that accepts hybrid search POST requests.
        std::string search_endpoint = "/search/hybrid";
    };

    // -----------------------------------------------------------------------
    // Result types
    // -----------------------------------------------------------------------

    /**
     * @brief Per-shard search outcome including individual shard results.
     */
    struct ShardSearchResult {
        std::string shard_id;                    ///< Shard identifier
        std::vector<HybridSearch::Result> results; ///< Results from this shard
        bool success = false;                    ///< true if shard responded OK
        std::string error_msg;                   ///< Error description if failed
        uint64_t execution_time_ms = 0;          ///< Round-trip time
        /// Why the shard stopped early (None for success or an ordinary failure).
        /// Lets callers tell a timeout/cancellation from "no results".
        query::StopReason stop_reason = query::StopReason::None;
    };

    /**
     * @brief Diagnostics returned alongside the merged result set.
     *
     * Provides explicit visibility into degradation paths and partial result
     * conditions, enabling operators to make informed decisions about result
     * quality and fallback behavior.
     */
    struct SearchStats {
        size_t shards_queried = 0;        ///< Total shards attempted
        size_t shards_succeeded = 0;      ///< Shards that returned results
        size_t shards_failed = 0;         ///< Shards that failed or timed out
        bool partial_result = false;      ///< True when results come from only some shards (failed or not started, with >=1 success)

        // Request-context (deadline / cancellation) visibility
        size_t shards_skipped = 0;        ///< Shards never started because the request had stopped
        size_t shards_timed_out = 0;      ///< Shards that hit the shard/request deadline (subset of shards_failed)
        size_t shards_cancelled = 0;      ///< Shards aborted by client cancellation (subset of shards_failed)
        bool cancelled = false;           ///< True when the request was cancelled by the client
        bool deadline_exceeded = false;   ///< True when the request deadline expired before all shards finished
        
        // Phase 2: Degradation visibility flags
        bool merge_underflow = false;     ///< True when merge produced < k results due to candidate deficit
        bool high_overlap_variance = false; ///< True when high-cardinality overlap detected
        bool ranking_conflict = false;    ///< True when shard ranking conflicts detected
        
        /// Per-shard diagnostics (Phase 2 enhancement)
        std::vector<std::string> failed_shard_reasons;  ///< Reason for each failed shard (e.g. "timeout", "HTTP 500")
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a DistributedHybridSearch engine.
     *
     * @param local_search  Local HybridSearch instance for this node.
     *                      May be null; the local shard will then return no
     *                      results.
     * @param resolver      URN/shard resolver used to enumerate healthy shards.
     *                      May be null; only the local shard will be queried.
     * @param executor      RemoteExecutor configured with mTLS credentials for
     *                      inter-node communication.  May be null; in that case
     *                      only local search is executed.
     * @throws std::invalid_argument on invalid configuration values.
     */
    explicit DistributedHybridSearch(
        HybridSearch* local_search,
        themis::sharding::URNResolver* resolver,
        themis::sharding::RemoteExecutor* executor
    );

    /**
     * @brief Construct a DistributedHybridSearch engine.
     *
     * @param local_search  Local HybridSearch instance for this node.
     *                      May be null; the local shard will then return no
     *                      results.
     * @param resolver      URN/shard resolver used to enumerate healthy shards.
     *                      May be null; only the local shard will be queried.
     * @param executor      RemoteExecutor configured with mTLS credentials for
     *                      inter-node communication.  May be null; in that case
     *                      only local search is executed.
     * @param config        Engine configuration.
     * @throws std::invalid_argument on invalid configuration values.
     */
    DistributedHybridSearch(
        HybridSearch* local_search,
        themis::sharding::URNResolver* resolver,
        themis::sharding::RemoteExecutor* executor,
        const Config& config
    );

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Execute distributed hybrid search across all healthy shards.
     *
     * Runs the local `HybridSearch` and dispatches the same query to every
     * healthy remote shard in parallel.  Results are merged via cross-shard
     * RRF and the top-k globally ranked results are returned.
     *
     * Never throws; all errors from individual shards are caught internally.
     *
     * @param text_query    Full-text query string (empty to skip BM25).
     * @param vector_query  Optional semantic query vector (empty to skip ANN).
     * @param stats         Optional output: per-shard diagnostics.
     * @return Globally merged top-k results, sorted by hybrid score.
     */
    std::vector<HybridSearch::Result> search(
        const std::string& text_query,
        const std::vector<float>& vector_query = {},
        SearchStats* stats = nullptr
    );

    /**
     * @brief Distributed search bound to a request deadline and cancellation token.
     *
     * Behavior versus the legacy overload:
     * - The context is checked before the local shard and before every fan-out
     *   batch. Once it has stopped, no further batch is launched; the unstarted
     *   shards are counted in SearchStats::shards_skipped.
     * - Each remote shard call gets `ctx.child(shard_timeout_ms)`, so its time
     *   budget is min(remaining request time, shard timeout). RemoteExecutor
     *   propagates it into MTLSClient's asynchronous transport, which aborts the
     *   in-flight connect/handshake/write/read on cancellation or deadline.
     * - Every launched shard task is joined before this function returns;
     *   nothing is detached or abandoned. The local HybridSearch call is not
     *   interruptible: it is only skipped if the context had already stopped.
     * - Results of shards that completed before the stop are merged and
     *   reported as partial (SearchStats::partial_result, cancelled,
     *   deadline_exceeded). With skip_failed_shards=false any failed or skipped
     *   shard yields an empty result vector.
     *
     * A custom shard function (setShardSearchFunction) that ignores its context
     * cannot be interrupted and delays the join accordingly.
     *
     * Never throws.
     *
     * @param[in] text_query Full-text query (empty to skip BM25).
     * @param[in] vector_query Optional semantic vector (empty to skip ANN).
     * @param[in] ctx Request deadline + cancellation token.
     * @param[out] stats Optional diagnostics, including stop information.
     * @return Globally merged top-k results from the shards that completed.
     */
    std::vector<HybridSearch::Result> search(
        const std::string& text_query,
        const std::vector<float>& vector_query,
        const query::RequestContext& ctx,
        SearchStats* stats = nullptr
    );

    /// Signature of a per-shard search transport.
    using ShardSearchFn = std::function<ShardSearchResult(
        const themis::sharding::ShardInfo& shard,
        const std::string& text_query,
        const std::vector<float>& vector_query,
        size_t k,
        const query::RequestContext& ctx)>;

    /**
     * @brief Replace the remote shard transport (default: RemoteExecutor over mTLS).
     *
     * The function runs on a fan-out worker thread and must honour @p ctx
     * to stay cancellable. Intended for alternative transports and for tests
     * that need deterministic shard behavior. Must be set before search() is
     * called concurrently.
     *
     * @param[in] fn Transport; an empty function restores the RemoteExecutor path.
     */
    void setShardSearchFunction(ShardSearchFn fn) { shard_search_fn_ = std::move(fn); }

    // -----------------------------------------------------------------------
    // Merging (public for unit testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Merge per-shard result lists via Reciprocal Rank Fusion.
     *
     * Phase 2: Enhanced merge with degradation flag tracking (merge_underflow,
     * high_overlap_variance). Applies a two-level RRF: within each shard results
     * are already ranked; across shards each result contributes `1 / (rrf_k + rank_in_shard)`
     * to the global score. Results that appear in multiple shards accumulate
     * contributions from each shard.
     *
     * @param shard_results  Per-shard result collections (failed shards are
     *                       skipped when `skip_failed_shards` is true).
     * @param stats          Optional output: merge-specific degradation flags
     *                       (merge_underflow, high_overlap_variance).
     * @return Merged results sorted by hybrid score descending, capped at k.
     */
    std::vector<HybridSearch::Result> mergeShardResults(
        const std::vector<ShardSearchResult>& shard_results,
        SearchStats* stats = nullptr
    ) const;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

    /**
     * @brief Parse a JSON result array from a shard HTTP response.
     *
     * Exposed as public static so it can be unit-tested directly.
     * Tolerates missing fields by using zero/empty defaults.
     *
     * Accepted formats:
     * - Direct JSON array: `[{"document_id": "...", ...}, ...]`
     * - Wrapped: `{"results": [{...}, ...]}`
     *
     * Entries with empty `document_id` are silently dropped.
     *
     * @note This method is `public` primarily to enable direct unit testing
     *       of the JSON deserialization logic without requiring network
     *       infrastructure.  It is a stateless utility with no side effects.
     */
    static std::vector<HybridSearch::Result> parseShardResponse(
        const nlohmann::json& data
    );

private:
    HybridSearch* local_search_;
    themis::sharding::URNResolver* resolver_;
    themis::sharding::RemoteExecutor* executor_;
    Config config_;

    // Statistics counters
    mutable std::atomic<uint64_t> total_searches_{0};
    mutable std::atomic<uint64_t> total_shards_queried_{0};
    mutable std::atomic<uint64_t> total_shard_failures_{0};

    /**
     * @brief Query a single remote shard via HTTP POST (mTLS).
     * @param[in] shard Target shard.
     * @param[in] text_query Text query.
     * @param[in] vector_query Query vector.
     * @param[in] k Per-shard result count.
     * @param[in] ctx Per-shard context (already clamped to the shard timeout).
     * @return Shard outcome with stop_reason set on timeout/cancellation.
     */
    ShardSearchResult searchRemoteShard(
        const themis::sharding::ShardInfo& shard,
        const std::string& text_query,
        const std::vector<float>& vector_query,
        size_t k,
        const query::RequestContext& ctx
    );

    ShardSearchFn shard_search_fn_;
};

} // namespace themis

