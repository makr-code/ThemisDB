/**
 * @file retrieval_guardrail.h
 * @brief Per-query federated retrieval cost guardrail.
 *
 * RetrievalGuardrail::checkFederatedCost() evaluates whether a
 * FederatedQueryPlan's estimated cost stays within configured SLO thresholds
 * before the query is dispatched to remote shards.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "rag/tensor_rag_cost_model.h"

#include <string>
#include <vector>

namespace themis {
namespace rag {

// ─────────────────────────────────────────────────────────────────────────────
// GuardrailDecision
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result returned by RetrievalGuardrail::checkFederatedCost().
 *
 * When @c allow is @c false, @c deny_reason carries a structured message
 * suitable for surfacing in @c SearchStats or audit logs.
 */
struct GuardrailDecision {
    /// True if the query may proceed; false if it was denied.
    bool allow{true};

    /// Human-readable denial reason (empty when @c allow is true).
    std::string deny_reason;

    /// Estimated end-to-end cost in milliseconds used for the decision.
    float estimated_cost_ms{0.0f};
};

// ─────────────────────────────────────────────────────────────────────────────
// FederatedQueryPlan
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Lightweight descriptor of a federated query plan passed to the guardrail.
 */
struct FederatedQueryPlan {
    /// Shard identifiers targeted by this query.
    std::vector<std::string> shard_ids;

    /// Total number of candidate chunks to retrieve across all shards.
    std::size_t num_chunks{0};

    /**
     * @brief Pre-computed cost estimate from an upstream planner (ms).
     *
     * When 0.0, the guardrail derives the estimate itself via
     * TensorRagCostModel::estimate().
     */
    float estimated_cost_ms{0.0f};

    /// True when the plan spans shards in different data-centres.
    bool cross_datacenter{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RetrievalGuardrailConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Threshold configuration for RetrievalGuardrail.
 */
struct RetrievalGuardrailConfig {
    /// Maximum allowed cost for same-DC queries (ms).
    float max_cost_ms{500.0f};

    /// Stricter maximum allowed cost for cross-DC queries (ms).
    float max_cross_dc_cost_ms{200.0f};

    /// Master switch — when false, every query is unconditionally allowed.
    bool enabled{true};
};

// ─────────────────────────────────────────────────────────────────────────────
// RetrievalGuardrail
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe federated retrieval cost guardrail.
 *
 * Uses a TensorRagCostModel to estimate the end-to-end latency of a
 * FederatedQueryPlan and denies execution when the estimate exceeds the
 * configured SLO threshold.  All public methods are @c const and therefore
 * safe to call concurrently from multiple threads without external locking.
 *
 * @code{.cpp}
 * themis::rag::TensorRagCostModel model;
 * themis::rag::RetrievalGuardrailConfig cfg;
 * cfg.max_cost_ms = 300.0f;
 * themis::rag::RetrievalGuardrail guard(model, cfg);
 *
 * themis::rag::FederatedQueryPlan plan;
 * plan.num_chunks = 50;
 * auto decision = guard.checkFederatedCost("SELECT ...", plan);
 * if (!decision.allow) { log(decision.deny_reason); }
 * @endcode
 */
class RetrievalGuardrail {
public:
    /**
     * @brief Constructs a guardrail backed by @p cost_model with @p config thresholds.
     *
     * @param cost_model Reference to a TensorRagCostModel; must outlive this object.
     * @param config     Threshold and enable/disable configuration.
     */
    RetrievalGuardrail(const TensorRagCostModel&      cost_model,
                       const RetrievalGuardrailConfig& config = {}) noexcept;

    /**
     * @brief Evaluates whether @p plan should be executed given cost thresholds.
     *
     * Steps:
     *  1. If the guardrail is disabled, return allow unconditionally.
     *  2. Derive effective cost: use @c plan.estimated_cost_ms when non-zero,
     *     otherwise call TensorRagCostModel::estimate() with default config
     *     adapted to @c plan.num_chunks.
     *  3. Select threshold: @c max_cross_dc_cost_ms for cross-DC plans,
     *     @c max_cost_ms otherwise.
     *  4. Deny with structured reason and THEMIS_WARN when cost > threshold.
     *
     * @param query  The raw query string (UTF-8).
     * @param plan   Federated plan to evaluate.
     * @return GuardrailDecision describing the allow/deny outcome.
     */
    GuardrailDecision checkFederatedCost(const std::string&      query,
                                         const FederatedQueryPlan& plan) const;

private:
    const TensorRagCostModel& cost_model_;
    RetrievalGuardrailConfig  config_;
};

} // namespace rag
} // namespace themis
