/**
 * @file llm_cascade_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/llm_cascade_router.h"

namespace themis {
namespace plugins {
namespace ethics {

LlmCascadeRouter::LlmCascadeRouter(CascadeRoutingConfig config)
    : config_(std::move(config))
{}

CascadeModelTier LlmCascadeRouter::resolveTier(const std::string& round_role) const noexcept {
    const auto it = config_.round_to_tier.find(round_role);
    if (it != config_.round_to_tier.end()) {
        return it->second;
    }
    return CascadeModelTier::MEDIUM;
}

ModelTokenBudget LlmCascadeRouter::budgetForTier(CascadeModelTier tier) const noexcept {
    ModelTokenBudget budget;
    const auto ctx_it = config_.tier_to_context_k.find(tier);
    if (ctx_it != config_.tier_to_context_k.end()) {
        budget.context_k = ctx_it->second;
    } else {
        budget.context_k = 4;
    }

    // Scale max_tokens to context window size (context_k * 1024)
    budget.max_tokens = budget.context_k * 1024;

    // Output token budget: 1/8 of context, capped at 2048
    budget.max_output_tokens = std::min(budget.max_tokens / 8, static_cast<size_t>(2048));

    return budget;
}

CascadeRoutingDecision LlmCascadeRouter::routeForRound(
    const std::string& round_role,
    size_t             estimated_prompt_tokens) const
{
    CascadeModelTier tier = resolveTier(round_role);
    ModelTokenBudget budget = budgetForTier(tier);
    bool was_escalated = false;

    // Check if prompt exceeds current tier's budget and escalate if needed
    if (estimated_prompt_tokens > budget.max_tokens
            && config_.fallback_policy == "escalate")
    {
        // Escalate through tiers: SMALL → MEDIUM → LARGE
        if (tier == CascadeModelTier::SMALL) {
            tier    = CascadeModelTier::MEDIUM;
            budget  = budgetForTier(tier);
            was_escalated = true;
        }
        if (estimated_prompt_tokens > budget.max_tokens && tier == CascadeModelTier::MEDIUM) {
            tier    = CascadeModelTier::LARGE;
            budget  = budgetForTier(tier);
            was_escalated = true;
        }
    }

    // Resolve model id
    std::string model_id;
    const auto model_it = config_.tier_to_model.find(tier);
    if (model_it != config_.tier_to_model.end()) {
        model_id = model_it->second;
    }

    CascadeRoutingDecision decision;
    decision.model_id      = model_id;
    decision.tier          = tier;
    decision.budget        = budget;
    decision.was_escalated = was_escalated;

    return decision;
}

ModelTokenBudget LlmCascadeRouter::budgetForRound(const std::string& round_role) const {
    return budgetForTier(resolveTier(round_role));
}

CascadeModelTier LlmCascadeRouter::tierForRound(const std::string& round_role) const noexcept {
    return resolveTier(round_role);
}

void LlmCascadeRouter::setLlmInvokeFn(LlmInvokeFn fn) {
    llm_invoke_fn_ = std::move(fn);
}

std::string LlmCascadeRouter::invoke(const std::string& round_role,
                                     const std::string& prompt) const {
    if (!llm_invoke_fn_) {
        return {};
    }
    const size_t estimated_tokens = prompt.size() / 4;  // rough char→token estimate
    const CascadeRoutingDecision decision = routeForRound(round_role, estimated_tokens);
    return llm_invoke_fn_(decision.model_id, prompt, decision.budget.max_output_tokens);
}

} // namespace ethics
} // namespace plugins
} // namespace themis
