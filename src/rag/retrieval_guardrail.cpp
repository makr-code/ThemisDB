/**
 * @file retrieval_guardrail.cpp
 * @brief Per-query federated retrieval cost guardrail implementation.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "rag/retrieval_guardrail.h"
#include "utils/logger.h"

#include <sstream>

namespace themis {
namespace rag {

RetrievalGuardrail::RetrievalGuardrail(const TensorRagCostModel&      cost_model,
                                       const RetrievalGuardrailConfig& config) noexcept
    : cost_model_(cost_model)
    , config_(config)
{}

GuardrailDecision RetrievalGuardrail::checkFederatedCost(
    const std::string&       query,
    const FederatedQueryPlan& plan) const
{
    GuardrailDecision decision;

    // ── Master switch ────────────────────────────────────────────────────────
    if (!config_.enabled) {
        decision.allow = true;
        return decision;
    }

    // ── Derive effective cost ────────────────────────────────────────────────
    float effective_cost = plan.estimated_cost_ms;
    if (effective_cost == 0.0f) {
        // Fall back to model estimation with a config adapted from the plan.
        TensorRagConfig rag_cfg;
        rag_cfg.num_chunks = plan.num_chunks;
        CostEstimate est = cost_model_.estimate(query, rag_cfg);
        effective_cost = est.total_ms;
    }
    decision.estimated_cost_ms = effective_cost;

    // ── Select threshold ─────────────────────────────────────────────────────
    const float threshold = plan.cross_datacenter
                          ? config_.max_cross_dc_cost_ms
                          : config_.max_cost_ms;

    // ── Evaluate ─────────────────────────────────────────────────────────────
    if (effective_cost > threshold) {
        std::ostringstream oss = {};
        oss << "Federated cost denied: query_len=" << query.size()
            << ", cost=" << effective_cost << "ms"
            << ", threshold=" << threshold << "ms"
            << ", cross_dc=" << (plan.cross_datacenter ? "true" : "false");
        decision.deny_reason = oss.str();
        decision.allow = false;

        THEMIS_WARN("[GUARDRAIL] Federated cost deny: query_len={}, cost={}ms, threshold={}ms",
                    query.size(), effective_cost, threshold);
    }

    return decision;
}

} // namespace rag
} // namespace themis
