/**
 * @file llm_cascade_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief LLM tier for cascade routing.
 */
enum class CascadeModelTier { SMALL, MEDIUM, LARGE };

/**
 * @brief Token budget for a model tier.
 */
struct ModelTokenBudget {
    size_t max_tokens{4096};
    size_t max_output_tokens{512};
    size_t context_k{4};  ///< Context window in K tokens
};

/**
 * @brief Configuration for the LLM cascade router.
 *
 * Mirrors the discourse_config.yaml llm_cascade section (§12.2.1).
 */
struct CascadeRoutingConfig {
    std::map<std::string, CascadeModelTier> round_to_tier;
    std::map<CascadeModelTier, std::string> tier_to_model;
    std::map<CascadeModelTier, size_t>      tier_to_context_k;
    std::string                             fallback_policy{"escalate"};
    bool                                    enabled{true};

    /// Returns a default config matching the discourse_config.yaml example.
    static CascadeRoutingConfig defaultConfig() {
        CascadeRoutingConfig cfg;
        cfg.round_to_tier = {
            {"PRO",          CascadeModelTier::SMALL},
            {"REBUTTAL",     CascadeModelTier::MEDIUM},
            {"SURREBUTTAL",  CascadeModelTier::MEDIUM},
            {"SYNTHESIS",    CascadeModelTier::LARGE},
            {"META_VERDICT", CascadeModelTier::SMALL},
        };
        cfg.tier_to_model = {
            {CascadeModelTier::SMALL,  "llama-3-8b-instruct"},
            {CascadeModelTier::MEDIUM, "mistral-7b-instruct-32k"},
            {CascadeModelTier::LARGE,  "gpt-4o"},
        };
        cfg.tier_to_context_k = {
            {CascadeModelTier::SMALL,  4},
            {CascadeModelTier::MEDIUM, 32},
            {CascadeModelTier::LARGE,  128},
        };
        return cfg;
    }
};

/**
 * @brief Routing result from ILlmCascadeRouter::routeForRound().
 */
struct CascadeRoutingDecision {
    std::string       model_id;       ///< Model alias (e.g. "llama-3-8b-instruct")
    CascadeModelTier  tier;
    ModelTokenBudget  budget;
    bool              was_escalated{false}; ///< true if fallback escalation occurred
};

/**
 * @brief Interface for LLM cascade routing per discourse round.
 *
 * Implements §12.2.1 (Language Model Cascades, Dohan et al. 2022).
 * Routes each discourse round to the minimum-necessary model tier,
 * reducing overall cost by ~60 % while preserving R4 SYNTHESIS quality.
 */
class ILlmCascadeRouter {
public:
    virtual ~ILlmCascadeRouter() = default;

    /**
     * @brief Determine the model to use for a given round role.
     *
     * @param round_role             "PRO"|"REBUTTAL"|"SURREBUTTAL"|"SYNTHESIS"|"META_VERDICT"
     * @param estimated_prompt_tokens Estimated input token count from ContextWindowBudgetManager.
     * @return CascadeRoutingDecision with model and budget.
     */
    virtual CascadeRoutingDecision routeForRound(
        const std::string& round_role,
        size_t             estimated_prompt_tokens) const = 0;

    /**
     * @brief Get the token budget for a given round role.
     */
    virtual ModelTokenBudget budgetForRound(
        const std::string& round_role) const = 0;

    /**
     * @brief Get the assigned tier for a round role (for logging/observability).
     */
    virtual CascadeModelTier tierForRound(
        const std::string& round_role) const noexcept = 0;
};

/**
 * @brief Concrete LLM cascade router.
 *
 * All routing is deterministic (round_role → tier → model). No LLM calls
 * are made here; this is a pure routing/configuration component.
 *
 * Thread-safe: all state is read-only after construction.
 */
class LlmCascadeRouter : public ILlmCascadeRouter {
public:
    explicit LlmCascadeRouter(CascadeRoutingConfig config = CascadeRoutingConfig::defaultConfig());

    CascadeRoutingDecision routeForRound(
        const std::string& round_role,
        size_t             estimated_prompt_tokens) const override;

    ModelTokenBudget budgetForRound(
        const std::string& round_role) const override;

    CascadeModelTier tierForRound(
        const std::string& round_role) const noexcept override;

    /// Access the routing config (for testing/observability).
    const CascadeRoutingConfig& config() const noexcept { return config_; }

    /**
     * @brief LLM backend invoke function type.
     *
     * When set via setLlmInvokeFn(), the invoke() method will call the real
     * LLM backend with the resolved model_id instead of only returning the
     * routing decision.
     *
     * @param model_id   Model alias from the routing decision (e.g. "llama-3-8b-instruct").
     * @param prompt     Full user + system prompt for this round.
     * @param max_tokens Maximum tokens to generate.
     * @return           Generated text from the model.
     */
    using LlmInvokeFn = std::function<std::string(
        const std::string& model_id,
        const std::string& prompt,
        size_t             max_tokens)>;

    /**
     * @brief Inject the LLM backend provider.
     *
     * After injection, the invoke() method resolves the route for a given
     * round and immediately calls the model via the provided function.
     * Without an injected provider, invoke() returns an empty string and
     * callers must resolve the model_id themselves from the routing decision.
     *
     * Thread safety: call before any concurrent invoke(); the function object
     * is stored under a plain copy (not atomic) and must be set once at
     * startup before concurrent use.
     */
    void setLlmInvokeFn(LlmInvokeFn fn);

    /**
     * @brief Route and optionally invoke the LLM for a discourse round.
     *
     * If a provider function was set via setLlmInvokeFn(), this method routes
     * the round, then calls the model and returns its generated text.
     * If no provider is set, an empty string is returned (callers should use
     * routeForRound() directly and dispatch to the model themselves).
     *
     * @param round_role  Discourse round role ("PRO", "REBUTTAL", etc.).
     * @param prompt      Full prompt to send to the selected model.
     * @return            Generated model output, or empty string when no provider set.
     */
    std::string invoke(const std::string& round_role, const std::string& prompt) const;

private:
    CascadeRoutingConfig config_;
    LlmInvokeFn          llm_invoke_fn_;

    CascadeModelTier resolveTier(const std::string& round_role) const noexcept;
    ModelTokenBudget budgetForTier(CascadeModelTier tier) const noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
