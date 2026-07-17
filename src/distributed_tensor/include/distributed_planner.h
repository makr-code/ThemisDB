/**
 * @file distributed_planner.h
 * @brief Distributed tensor retrieval planner with summary-first, exact-on-demand, graph-verified finalization.
 *
 * Implements the core workflow:
 * 1. Summary-first routing (use advisory summaries to select shards)
 * 2. Exact-on-demand fragment loading (fetch actual tensor data from selected shards)
 * 3. Graph-verified finalization (validate results against graph truth before output)
 *
 * Enforces the "no-summary-only-truth" rule: distributed tensor summaries can never
 * be the sole source of truth for query results. All outputs must be validated against
 * exact graph data.
 */

#pragma once

#include "tensor/tensor_summary_types.h"
#include "rag/graph_truth_validator.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// Forward declarations
// ============================================================================

class IFragmentFetcher;
class IGraphValidator;

// ============================================================================
// RoutingSummary — advisory summary for planner routing decisions
// ============================================================================

/**
 * @brief Routing decision made from a shard summary (advisory only).
 *
 * Used in summary-first routing phase to decide which shards to query.
 * These are routing hints, NOT final truth.
 */
struct RoutingSummary {
    /// Shard identifier.
    std::string shard_id;

    /// Freshness state of the summary.
    tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::FRESH;

    /// Shard relevance score (0.0-1.0).
    float shard_relevance = 0.0f;

    /// Confidence in routing decision (0.0-1.0).
    float routing_confidence = 0.8f;

    /// Human-readable routing reason.
    std::string routing_reason;

    /// If true, this shard was rejected due to staleness.
    bool rejected_as_stale = false;

    /// If true, this shard was rejected due to unhealthy status.
    bool rejected_as_unhealthy = false;
};

// ============================================================================
// FragmentLoadRequest — request for exact tensor fragment loading
// ============================================================================

/**
 * @brief Request to load exact tensor fragments on-demand.
 *
 * After summary-first routing, the planner issues fragment load requests
 * to fetch the actual tensor data needed for graph validation.
 */
struct FragmentLoadRequest {
    /// Shard identifier to fetch from.
    std::string shard_id;

    /// Artifact/fragment identifier within the shard.
    std::string artifact_id;

    /// Whether to prioritize latency over completeness.
    bool expedited = false;

    /// Timeout for fragment load in milliseconds.
    uint32_t timeout_ms = 5000;

    /// Optional compression codec hint.
    std::string codec_hint;
};

// ============================================================================
// FragmentLoadResult — result of exact fragment loading
// ============================================================================

/**
 * @brief Result of loading an exact tensor fragment.
 *
 * Contains the actual tensor data plus metadata about success/failure.
 */
struct FragmentLoadResult {
    /// Shard identifier of the loaded fragment.
    std::string shard_id;

    /// Artifact identifier.
    std::string artifact_id;

    /// Whether load succeeded.
    bool success = false;

    /// If not successful, error reason.
    std::string error_reason;

    /// Actual tensor fragment data (opaque).
    std::vector<uint8_t> fragment_data;

    /// Metadata checksum for integrity verification.
    std::string content_hash;

    /// Latency of fragment load in milliseconds.
    float load_latency_ms = 0.0f;

    /// Timestamp when fragment was loaded.
    std::string loaded_at;
};

// ============================================================================
// TensorRetrievalPlan — comprehensive retrieval plan
// ============================================================================

/**
 * @brief Complete retrieval plan for a query over distributed tensor shards.
 *
 * Specifies the full workflow:
 * 1. Which shards to query (summary-first routing)
 * 2. Which fragments to load (exact-on-demand)
 * 3. How to validate against graph truth (mandatory finalization)
 * 4. Fallback strategy if validation fails
 */
struct TensorRetrievalPlan {
    /// Query correlation ID for tracing.
    std::string correlation_id;

    /// Routing summaries for summary-first phase (advisory only).
    std::vector<RoutingSummary> routing_summaries;

    /// Fragment load requests for exact-on-demand phase.
    std::vector<FragmentLoadRequest> fragment_requests;

    /// Results of fragment loading (populated after execution).
    std::vector<FragmentLoadResult> fragment_results;

    /// Whether all routing summaries passed freshness validation.
    bool all_summaries_fresh = true;

    /// Number of shards rejected due to staleness.
    std::size_t stale_shards_rejected = 0;

    /// Whether the plan passed graph truth validation.
    bool passed_graph_validation = false;

    /// Graph validation result (if performed).
    rag::GraphTruthValidationResult validation_result;

    /// Human-readable plan description.
    std::string plan_reason;

    /// Fallback routing if primary plan fails.
    std::string fallback_routing;

    /// Whether this is a fallback execution.
    bool is_fallback = false;
};

// ============================================================================
// DistributedTensorPlanner — main planner interface
// ============================================================================

/**
 * @brief Distributed tensor planner implementing summary-first, exact-on-demand,
 * graph-verified finalization workflow.
 *
 * Core responsibilities:
 * 1. Accept advisory shard summaries
 * 2. Plan summary-first routing (identify fresh shards to query)
 * 3. Plan exact-on-demand fragment loading
 * 4. Ensure graph truth validation before final output
 * 5. Enforce "no-summary-only-truth" rule
 * 6. Reject stale or invalid summaries
 */
class DistributedTensorPlanner {
public:
    DistributedTensorPlanner() = default;
    ~DistributedTensorPlanner() = default;

    // Prevent copies and moves (may contain internal state)
    DistributedTensorPlanner(const DistributedTensorPlanner&) = delete;
    DistributedTensorPlanner& operator=(const DistributedTensorPlanner&) = delete;
    DistributedTensorPlanner(DistributedTensorPlanner&&) = delete;
    DistributedTensorPlanner& operator=(DistributedTensorPlanner&&) = delete;

    // ─── Dependency Injection ──────────────────────────────────────────────

    /**
     * @brief Set the fragment fetcher for exact-on-demand loading.
     * 
     * @param fetcher Fragment fetcher implementation.
     */
    void setFragmentFetcher(std::shared_ptr<IFragmentFetcher> fetcher) noexcept;

    /**
     * @brief Set the graph validator for mandatory finalization validation.
     * 
     * @param validator Graph truth validator.
     */
    void setGraphValidator(std::shared_ptr<rag::GraphTruthValidator> validator) noexcept;

    // ─── Core Planning Methods ─────────────────────────────────────────────

    /**
     * @brief Plan summary-first routing from shard summaries.
     *
     * Phase 1: Accept advisory shard summaries, validate freshness, and produce
     * routing hints. Rejects summaries that are stale or invalid.
     *
     * @param summaries        Vector of shard summaries (may be from distributed sources).
     * @param correlation_id   Trace ID for this query.
     * @param freshness_ttl_s  Override TTL for summary freshness validation.
     * @return Routing summaries (validated for freshness).
     */
    [[nodiscard]] std::vector<RoutingSummary> planSummaryFirstRouting(
        const std::vector<tensor::ShardSummary>& summaries,
        const std::string& correlation_id = {},
        uint32_t freshness_ttl_s = 3600) noexcept;

    /**
     * @brief Plan exact-on-demand fragment loading from selected shards.
     *
     * Phase 2: Given routing decisions, plan which tensor fragments to load
     * from which shards to satisfy the query.
     *
     * @param routing_summaries   Routing decisions from summary-first phase.
     * @param query_context       Human-readable query context.
     * @param correlation_id      Trace ID.
     * @return Fragment load requests for execution.
     */
    [[nodiscard]] std::vector<FragmentLoadRequest> planExactOnDemandLoading(
        const std::vector<RoutingSummary>& routing_summaries,
        const std::string& query_context = {},
        const std::string& correlation_id = {}) noexcept;

    /**
     * @brief Build comprehensive retrieval plan with fallback handling.
     *
     * Combines phases 1 and 2 with fallback strategy if primary plan would
     * violate the "no-summary-only-truth" rule.
     *
     * @param summaries       Vector of shard summaries.
     * @param query_context   Query context for planning.
     * @param correlation_id  Trace ID.
     * @return Complete retrieval plan including fallback routing.
     */
    [[nodiscard]] TensorRetrievalPlan buildRetrievalPlan(
        const std::vector<tensor::ShardSummary>& summaries,
        const std::string& query_context = {},
        const std::string& correlation_id = {}) noexcept;

    // ─── Fragment Loading & Validation ────────────────────────────────────

    /**
     * @brief Execute fragment loads planned by planExactOnDemandLoading().
     *
     * Fetches actual tensor fragments from selected shards.
     * All failures recorded in results; no exception thrown.
     *
     * @param requests        Fragment load requests to execute.
     * @param correlation_id  Trace ID.
     * @return Load results for each request (success/failure).
     */
    [[nodiscard]] std::vector<FragmentLoadResult> executeFragmentLoads(
        const std::vector<FragmentLoadRequest>& requests,
        const std::string& correlation_id = {}) noexcept;

    /**
     * @brief Validate plan against graph truth (mandatory finalization).
     *
     * Phase 3: Enforces "no-summary-only-truth" rule. No query result can be
     * returned without passing graph truth validation.
     *
     * @param plan        Retrieval plan with fragment results.
     * @param query       Original query string.
     * @param config      Graph validation config.
     * @return Validation result (success = passed graph validation).
     */
    [[nodiscard]] rag::GraphTruthValidationResult validateAgainstGraphTruth(
        const TensorRetrievalPlan& plan,
        const std::string& query,
        const rag::GraphTruthValidatorConfig& config = {}) const noexcept;

    /**
     * @brief Determine if a plan passes the "no-summary-only-truth" rule.
     *
     * A plan is valid only if:
     * 1. All advisory summaries have valid freshness state, AND
     * 2. Exact fragments have been successfully loaded, AND
     * 3. Graph truth validation passed
     *
     * @param plan Retrieved retrieval plan.
     * @return true if plan satisfies all constraints for final output.
     */
    [[nodiscard]] bool isValidFinalPlan(const TensorRetrievalPlan& plan) const noexcept;

    // ─── Configuration ────────────────────────────────────────────────────

    /**
     * @brief Configuration for distributed planner behavior.
     */
    struct Config {
        /// Maximum summary staleness TTL override (seconds).
        uint32_t max_summary_ttl_seconds = 3600;

        /// Minimum fragment load success rate to proceed (0.0-1.0).
        float min_fragment_success_rate = 0.8f;

        /// Whether to enable automatic fallback routing on failure.
        bool enable_fallback_routing = true;

        /// Maximum time to wait for fragment loads (milliseconds).
        uint32_t fragment_load_timeout_ms = 10000;

        /// Whether to enforce strict graph validation (no lenient mode).
        bool strict_graph_validation = true;
    };

    /**
     * @brief Set planner configuration.
     * 
     * @param config Configuration options.
     */
    void setConfig(const Config& config) noexcept;

    /**
     * @brief Get current configuration.
     * 
     * @return Current config.
     */
    [[nodiscard]] const Config& config() const noexcept;

private:
    std::shared_ptr<IFragmentFetcher> fragment_fetcher_;
    std::shared_ptr<rag::GraphTruthValidator> graph_validator_;
    Config config_;

    // Helper methods
    [[nodiscard]] bool validateSummaryFreshness(
        const tensor::ShardSummary& summary) const noexcept;

    [[nodiscard]] float computeRoutingRelevance(
        const tensor::ShardSummary& summary) const noexcept;

    [[nodiscard]] std::string buildFallbackRouting(
        const std::vector<tensor::ShardSummary>& summaries) const noexcept;
};

// ============================================================================
// IFragmentFetcher — abstract interface for fragment loading
// ============================================================================

/**
 * @brief Interface for loading exact tensor fragments from distributed shards.
 *
 * Implementations handle shard communication, error handling, and data transfer.
 */
class IFragmentFetcher {
public:
    virtual ~IFragmentFetcher() = default;

    /**
     * @brief Fetch a tensor fragment from a shard.
     *
     * @param request       Fragment load request.
     * @param correlation_id Trace ID.
     * @return Load result (may be failure if shard unreachable).
     */
    [[nodiscard]] virtual FragmentLoadResult fetchFragment(
        const FragmentLoadRequest& request,
        const std::string& correlation_id = {}) const noexcept = 0;

    /**
     * @brief Fetch multiple fragments (may batch for efficiency).
     *
     * @param requests      Vector of fragment requests.
     * @param correlation_id Trace ID.
     * @return Vector of load results (1:1 correspondence with requests).
     */
    [[nodiscard]] virtual std::vector<FragmentLoadResult> fetchFragments(
        const std::vector<FragmentLoadRequest>& requests,
        const std::string& correlation_id = {}) const noexcept = 0;
};

} // namespace distributed_tensor
} // namespace themis
