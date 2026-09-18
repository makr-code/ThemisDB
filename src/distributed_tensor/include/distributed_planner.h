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

struct RoutingSummary {
    std::string shard_id;

    tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::FRESH;

    float shard_relevance = 0.0f;

    float routing_confidence = 0.8f;

    std::string routing_reason;

    bool rejected_as_stale = false;

    bool rejected_as_unhealthy = false;
};

// ============================================================================
// FragmentLoadRequest — request for exact tensor fragment loading
// ============================================================================

struct FragmentLoadRequest {
    std::string shard_id;

    std::string artifact_id;

    bool expedited = false;

    uint32_t timeout_ms = 5000;

    std::string codec_hint;
};

// ============================================================================
// FragmentLoadResult — result of exact fragment loading
// ============================================================================

struct FragmentLoadResult {
    std::string shard_id;

    std::string artifact_id;

    bool success = false;

    std::string error_reason;

    std::vector<uint8_t> fragment_data;

    std::string content_hash;

    float load_latency_ms = 0.0f;

    std::string loaded_at;
};

// ============================================================================
// TensorRetrievalPlan — comprehensive retrieval plan
// ============================================================================

struct TensorRetrievalPlan {
    std::string correlation_id;

    std::vector<RoutingSummary> routing_summaries;

    std::vector<FragmentLoadRequest> fragment_requests;

    std::vector<FragmentLoadResult> fragment_results;

    bool all_summaries_fresh = true;

    std::size_t stale_shards_rejected = 0;

    bool passed_graph_validation = false;

    rag::GraphTruthValidationResult validation_result;

    std::string plan_reason;

    std::string fallback_routing;

    bool is_fallback = false;
};

// ============================================================================
// DistributedTensorPlanner — main planner interface
// ============================================================================

class DistributedTensorPlanner {
public:
    DistributedTensorPlanner() = default;
    ~DistributedTensorPlanner() = default;

    // Prevent copies and moves (may contain internal state)
    DistributedTensorPlanner(const DistributedTensorPlanner&) = delete;
    DistributedTensorPlanner& operator=(const DistributedTensorPlanner&) = delete;
    DistributedTensorPlanner(DistributedTensorPlanner&&) = delete;
    DistributedTensorPlanner& operator=(DistributedTensorPlanner&&) = delete;

    /**
     * @brief ─── Dependency Injection ──────────────────────────────────────────────
     * @param[in] fetcher Input parameter.
     * @note Exception safety: noexcept.
     */

    void setFragmentFetcher(std::shared_ptr<IFragmentFetcher> fetcher) noexcept;

    /**
     * @brief Set Graph Validator.
     * @param[in] validator Input parameter.
     * @note Exception safety: noexcept.
     */
    void setGraphValidator(std::shared_ptr<rag::GraphTruthValidator> validator) noexcept;

    // ─── Core Planning Methods ─────────────────────────────────────────────

    [[nodiscard]] std::vector<RoutingSummary> planSummaryFirstRouting(
        const std::vector<tensor::ShardSummary>& summaries,
        const std::string& correlation_id = {},
        uint32_t freshness_ttl_s = 3600) noexcept;

    [[nodiscard]] std::vector<FragmentLoadRequest> planExactOnDemandLoading(
        const std::vector<RoutingSummary>& routing_summaries,
        const std::string& query_context = {},
        const std::string& correlation_id = {}) noexcept;

    [[nodiscard]] TensorRetrievalPlan buildRetrievalPlan(
        const std::vector<tensor::ShardSummary>& summaries,
        const std::string& query_context = {},
        const std::string& correlation_id = {}) noexcept;

    // ─── Fragment Loading & Validation ────────────────────────────────────

    [[nodiscard]] std::vector<FragmentLoadResult> executeFragmentLoads(
        const std::vector<FragmentLoadRequest>& requests,
        const std::string& correlation_id = {}) noexcept;

    [[nodiscard]] rag::GraphTruthValidationResult validateAgainstGraphTruth(
        const TensorRetrievalPlan& plan,
        const std::string& query,
        const rag::GraphTruthValidatorConfig& config = {}) const noexcept;

    [[nodiscard]] bool isValidFinalPlan(const TensorRetrievalPlan& plan) const noexcept;

    // ─── Configuration ────────────────────────────────────────────────────

    struct Config {
        uint32_t max_summary_ttl_seconds = 3600;

        float min_fragment_success_rate = 0.8f;

        bool enable_fallback_routing = true;

        uint32_t fragment_load_timeout_ms = 10000;

        bool strict_graph_validation = true;
    };

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @note Exception safety: noexcept.
     */
    void setConfig(const Config& config) noexcept;

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

class IFragmentFetcher {
public:
    /**
     * @brief IFragment Fetcher.
     * @return Return value.
     */
    virtual ~IFragmentFetcher() = default;

    [[nodiscard]] virtual FragmentLoadResult fetchFragment(
        const FragmentLoadRequest& request,
        const std::string& correlation_id = {}) const noexcept = 0;

    [[nodiscard]] virtual std::vector<FragmentLoadResult> fetchFragments(
        const std::vector<FragmentLoadRequest>& requests,
        const std::string& correlation_id = {}) const noexcept = 0;
};

} // namespace distributed_tensor
} // namespace themis
