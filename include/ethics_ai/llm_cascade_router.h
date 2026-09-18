/**
 * @file llm_cascade_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class CascadeModelTier { SMALL, MEDIUM, LARGE };

struct ModelTokenBudget {
    size_t max_tokens{4096};
    size_t max_output_tokens{512};
    size_t context_k{4};  ///< Context window in K tokens
};

struct CascadeRoutingConfig {
    std::map<std::string, CascadeModelTier> round_to_tier;
    std::map<CascadeModelTier, std::string> tier_to_model;
    std::map<CascadeModelTier, size_t>      tier_to_context_k;
    std::string                             fallback_policy{"escalate"};
    bool                                    enabled{true};

    /**
     * @brief Default Config.
     * @return Return value.
     * @details Implements defaultConfig without additional internal calls.
     */
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

struct CascadeRoutingDecision {
    std::string       model_id;       ///< Model alias (e.g. "llama-3-8b-instruct")
    CascadeModelTier  tier;
    ModelTokenBudget  budget;
    bool              was_escalated{false}; ///< true if fallback escalation occurred
};

class ILlmCascadeRouter {
public:
    /**
     * @brief ILlm Cascade Router.
     * @return Return value.
     */
    virtual ~ILlmCascadeRouter() = default;

    /**
     * @brief Route For Round.
     * @param[in] round_role Input parameter.
     * @param[in] estimated_prompt_tokens Input parameter.
     * @return Return value.
     */
    virtual CascadeRoutingDecision routeForRound(
        const std::string& round_role,
        size_t             estimated_prompt_tokens) const = 0;

    /**
     * @brief Budget For Round.
     * @param[in] round_role Input parameter.
     * @return Return value.
     */
    virtual ModelTokenBudget budgetForRound(
        const std::string& round_role) const = 0;

    /**
     * @brief Tier For Round.
     * @param[in] round_role Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    virtual CascadeModelTier tierForRound(
        const std::string& round_role) const noexcept = 0;
};

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

    const CascadeRoutingConfig& config() const noexcept { return config_; }

    using LlmInvokeFn = std::function<std::string(
        const std::string& model_id,
        const std::string& prompt,
        size_t             max_tokens)>;

    /**
     * @brief Set Llm Invoke Fn.
     * @param[in] fn Input parameter.
     */
    void setLlmInvokeFn(LlmInvokeFn fn);

    /**
     * @brief Invoke.
     * @param[in] round_role Input parameter.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    std::string invoke(const std::string& round_role, const std::string& prompt) const;

private:
    CascadeRoutingConfig config_;
    LlmInvokeFn          llm_invoke_fn_;

    /**
     * @brief Resolve Tier.
     * @param[in] round_role Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    CascadeModelTier resolveTier(const std::string& round_role) const noexcept;
    /**
     * @brief Budget For Tier.
     * @param[in] tier Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    ModelTokenBudget budgetForTier(CascadeModelTier tier) const noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
