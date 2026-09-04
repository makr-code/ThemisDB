/**
 * @file llm_generate_operator.h
 * @brief LLMGenerateOperator — first-class probabilistic query plan operator
 *        for LLM inference within the ThemisDB query executor.
 *
 * ## Motivation (P1.2)
 *
 * LLM inference is a valid query-plan operator: it takes retrieved data as
 * input, applies a probabilistic transformation (language model), and produces
 * structured or unstructured output.  Representing it as a first-class
 * `QueryPlanNode` makes it visible in EXPLAIN output, enables cost-based
 * decisions (skip LLM when budget exceeded, use cached result, fan-out to
 * multiple models), and allows the optimizer to reason about combined plans
 * (e.g. VectorSearch → GraphTraversal → LLMGenerate).
 *
 * ### Key design properties
 * - `is_deterministic = false` — the planner treats LLM outputs as
 *   probabilistic; results may differ between identical inputs.
 * - Cost model parameters: `estimated_input_tokens`, `estimated_output_tokens`,
 *   `latency_budget_ms`, `token_budget`.  When budget is exceeded, the operator
 *   exposes a `FallbackPolicy` that the executor enforces.
 * - `require_snapshot_context = true` by default — the operator requires that
 *   its input sub-operators (retrieval) have been evaluated in a consistent
 *   snapshot (see `LLMQueryContext`).
 * - `audit_mode` — whether to persist an `AIDecisionAudit` record for this
 *   invocation (default: on for production builds).
 *
 * ### Integration points
 * - `QueryOptimizer::Plan` — extend `Plan` with an optional `llm_operator`
 *   when the query contains an LLM INFER / LLM RAG step.
 * - `QueryPlanVisualizer` — rendered as node type `LLMGenerate`.
 * - `LLMAQLHandler` — instantiates and executes `LLMGenerateOperator` for
 *   LLM_INFER and LLM_RAG AQL commands.
 *
 * @see include/query/query_plan_visualizer.h  — QueryPlanNode / PlanNodeType
 * @see include/aql/llm_query_context.h        — MVCC snapshot context
 * @see src/aql/llm_aql_handler.cpp            — execution entry point
 */

#pragma once

#include "aql/llm_query_context.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// FallbackPolicy — what the executor does when the LLM budget is exceeded
// ---------------------------------------------------------------------------

/**
 * @brief Policy that governs executor behaviour when the LLM operator's
 *        budget (token or latency) is exceeded or the backend is unavailable.
 */
enum class LLMFallbackPolicy {
    /// Return an empty result set with a diagnostic status field.
    ReturnEmpty,
    /// Return the retrieval results without LLM augmentation.
    ReturnRetrievalOnly,
    /// Propagate an error to the caller (fail-closed).
    PropagateError,
};

// ---------------------------------------------------------------------------
// LLMOperatorCost — cost model for the LLM generate operator
// ---------------------------------------------------------------------------

/**
 * @brief Cost model parameters for a single `LLMGenerateOperator` invocation.
 *
 * The optimizer uses these estimates to decide whether to include the LLM
 * operator in the plan, how to budget its resources, and when to activate the
 * fallback policy.
 */
struct LLMOperatorCost {
    /// Estimated number of input tokens (prompt + context).
    uint32_t estimated_input_tokens = 512;

    /// Estimated number of output tokens (generated response).
    uint32_t estimated_output_tokens = 256;

    /// Target latency budget for the entire LLM step in milliseconds.
    /// Zero = no limit (unlimited budget; not recommended for production).
    uint32_t latency_budget_ms = 10'000;

    /**
     * @brief Estimated cost in abstract "cost units" (consistent with
     *        OptimizerCostModel units used for other operators).
     *
     * Formula: `estimated_input_tokens * latency_per_token_ms
     *           + estimated_output_tokens * latency_per_token_ms
     *           + kvcache_miss_penalty_ms`
     *
     * Default value is a conservative upper-bound estimate.  The actual cost
     * model should be calibrated against measured throughput data.
     */
    double estimated_cost_units = 5000.0;

    /**
     * @brief Whether a KV-cache miss is expected for this request's prompt prefix.
     *
     * When true, `kvcache_miss_penalty_ms` is added to `estimated_cost_units`.
     */
    bool kvcache_miss_expected = true;

    /// Penalty (ms) added to cost estimate when KV-cache miss is expected.
    uint32_t kvcache_miss_penalty_ms = 200;
};

// ---------------------------------------------------------------------------
// LLMGenerateOperator — the operator itself
// ---------------------------------------------------------------------------

/**
 * @brief First-class query plan operator representing an LLM generation step.
 *
 * `LLMGenerateOperator` wraps one LLM inference call as a typed operator node
 * that participates in the query plan.  It carries:
 *   - The MVCC snapshot context that binds upstream retrieval reads.
 *   - Cost model parameters for budget enforcement and EXPLAIN output.
 *   - Fallback policy for budget/availability failures.
 *   - Determinism annotation (always false for LLM).
 *   - Audit mode flag for governance compliance.
 *
 * Instances are created by `LLMAQLHandler` for LLM_INFER / LLM_RAG commands
 * and are attached to the `QueryOptimizer::Plan` before execution begins.
 *
 * ### Thread safety
 * `LLMGenerateOperator` is a value type; it is not shared between threads.
 * Callers must synchronise access if the same instance is used concurrently.
 */
class LLMGenerateOperator {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    LLMGenerateOperator() = default;

    /**
     * @brief Construct with explicit context and cost model.
     *
     * @param ctx   MVCC snapshot context (see `LLMQueryContext`).
     * @param cost  Cost model parameters.
     * @param model_id  Logical model identifier (e.g. "llama-3-8b-q4").
     */
    LLMGenerateOperator(
        aql::LLMQueryContext ctx,
        LLMOperatorCost cost,
        std::string model_id)
        : context_(std::move(ctx))
        , cost_(cost)
        , model_id_(std::move(model_id))
    {}

    // -----------------------------------------------------------------------
    // Core properties
    // -----------------------------------------------------------------------

    /**
     * @brief LLM operators are always non-deterministic.
     *
     * Planner and executor must not assume that re-running the operator with
     * identical inputs produces identical outputs.  Caching and result reuse
     * require explicit opt-in via `allow_cached_result`.
     */
    [[nodiscard]] constexpr bool isDeterministic() const noexcept { return false; }

    /**
     * @brief Whether a consistent MVCC snapshot is required for the upstream
     *        retrieval operators that feed this LLM step.
     *
     * Defaults to `true` (P1.1 requirement).  May be relaxed to `false` for
     * fire-and-forget analytics queries where consistency is not critical.
     */
    [[nodiscard]] bool requiresSnapshotContext() const noexcept {
        return require_snapshot_context_;
    }

    /**
     * @brief Set snapshot requirement (override for non-critical queries).
     * @param required true = enforce snapshot; false = allow no-snapshot path.
     */
    void setRequireSnapshotContext(bool required) noexcept {
        require_snapshot_context_ = required;
    }

    // -----------------------------------------------------------------------
    // Cost model
    // -----------------------------------------------------------------------

    /// @return Immutable cost model parameters.
    [[nodiscard]] const LLMOperatorCost& cost() const noexcept { return cost_; }

    /// @return Mutable reference for in-place calibration by the optimizer.
    LLMOperatorCost& cost() noexcept { return cost_; }

    // -----------------------------------------------------------------------
    // Context & routing
    // -----------------------------------------------------------------------

    /// @return The MVCC snapshot context for this request.
    [[nodiscard]] const aql::LLMQueryContext& context() const noexcept { return context_; }

    /// @return The logical model identifier.
    [[nodiscard]] const std::string& modelId() const noexcept { return model_id_; }

    // -----------------------------------------------------------------------
    // Fallback
    // -----------------------------------------------------------------------

    /// @return Current fallback policy.
    [[nodiscard]] LLMFallbackPolicy fallbackPolicy() const noexcept { return fallback_policy_; }

    /**
     * @brief Override the fallback policy.
     * @param p New fallback policy.
     */
    void setFallbackPolicy(LLMFallbackPolicy p) noexcept { fallback_policy_ = p; }

    /**
     * @brief Check whether the operator is within its latency budget.
     *
     * @param elapsed  Wall-clock time elapsed since inference started.
     * @return true when elapsed < latency_budget_ms (or budget is zero/unlimited).
     */
    [[nodiscard]] bool withinBudget(
        std::chrono::milliseconds elapsed) const noexcept
    {
        if (cost_.latency_budget_ms == 0) return true;  // unlimited
        return static_cast<uint32_t>(elapsed.count()) < cost_.latency_budget_ms;
    }

    // -----------------------------------------------------------------------
    // Governance / audit
    // -----------------------------------------------------------------------

    /**
     * @brief Whether an `AIDecisionAudit` record should be persisted for
     *        this invocation.  Defaults to true for production builds.
     */
    [[nodiscard]] bool auditEnabled() const noexcept { return audit_enabled_; }

    /// @param enabled  Set to false to disable audit for this operator.
    void setAuditEnabled(bool enabled) noexcept { audit_enabled_ = enabled; }

    // -----------------------------------------------------------------------
    // Result caching
    // -----------------------------------------------------------------------

    /**
     * @brief Whether a cached result may be returned instead of re-running
     *        inference.  Off by default because LLM outputs are probabilistic.
     */
    [[nodiscard]] bool allowCachedResult() const noexcept { return allow_cached_result_; }

    /// @param allowed Set to true to allow result reuse from the response cache.
    void setAllowCachedResult(bool allowed) noexcept { allow_cached_result_ = allowed; }

    // -----------------------------------------------------------------------
    // EXPLAIN output helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Return a human-readable summary for EXPLAIN plan output.
     *
     * Example:
     * ```
     * LLMGenerate [model=llama-3-8b-q4, est_cost=5000, budget=10000ms,
     *              deterministic=false, snapshot=snapshot-isolated,
     *              fallback=ReturnRetrievalOnly, audit=on]
     * ```
     */
    [[nodiscard]] std::string toExplainString() const;

private:
    aql::LLMQueryContext context_;
    LLMOperatorCost      cost_;
    std::string          model_id_;
    LLMFallbackPolicy    fallback_policy_   = LLMFallbackPolicy::ReturnRetrievalOnly;
    bool                 require_snapshot_context_ = true;
    bool                 audit_enabled_    = true;
    bool                 allow_cached_result_ = false;
};

// ---------------------------------------------------------------------------
// Inline implementation of toExplainString
// ---------------------------------------------------------------------------

inline std::string LLMGenerateOperator::toExplainString() const {
    std::string fb = {};
    switch (fallback_policy_) {
        case LLMFallbackPolicy::ReturnEmpty:          fb = "ReturnEmpty";          break;
        case LLMFallbackPolicy::ReturnRetrievalOnly:  fb = "ReturnRetrievalOnly";  break;
        case LLMFallbackPolicy::PropagateError:       fb = "PropagateError";       break;
    }
    return "LLMGenerate [model=" + model_id_ +
           ", est_cost=" + std::to_string(static_cast<int>(cost_.estimated_cost_units)) +
           ", budget=" + std::to_string(cost_.latency_budget_ms) + "ms" +
           ", deterministic=false" +
           ", snapshot=" + context_.isolation_mode +
           ", fallback=" + fb +
           ", audit=" + (audit_enabled_ ? "on" : "off") + "]";
}

} // namespace query
} // namespace themis
