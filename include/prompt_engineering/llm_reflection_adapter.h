/**
 * @file llm_reflection_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <memory>
#include <string>

#include "meta_prompt_generator.h"   // ILLMProvider
#include "reflection_tuner.h"        // IReflectionProvider, DynamicReflectionPromptBuilder

namespace themis {
namespace prompt_engineering {

// ============================================================================
// IReflectionScorer
// ============================================================================

/**
 * @brief Optional pluggable quality scorer for `ILLMProviderReflectionAdapter`.
 *
 * Implement this to supply a task-specific scoring function (e.g. ROUGE,
 * BLEU, or an LLM-as-judge call) instead of the built-in heuristic.
 */
class IReflectionScorer {
public:
    /**
     * @brief TBD: Describe ~IReflectionScorer.
     * @return Return value.
     */
    virtual ~IReflectionScorer() = default;

    /**
     * @brief Evaluate the quality of @p response for @p prompt.
     * @return Quality score in [0.0, 1.0].
     * @param[in] prompt Input parameter.
     * @param[in] response Input parameter.
     */
    virtual double score(const std::string& prompt,
                         const std::string& response) const = 0;
};

// ============================================================================
// ILLMProviderReflectionAdapter
// ============================================================================

/**
 * @brief Adapts any `ILLMProvider` for use as an `IReflectionProvider`.
 *
 * **Adapter pattern**: translates `IReflectionProvider`'s four-method
 * interface into `ILLMProvider::complete()` calls, with
 * `DynamicReflectionPromptBuilder` producing the strategy-specific prompts.
 *
 * Strategy and self-aware context behaviour are inherited from the
 * `DynamicReflectionPromptBuilder` that is embedded in the adapter and
 * configurable via `setStrategy()`.
 *
 * Quality scoring falls back to a built-in heuristic when no
 * `IReflectionScorer` is injected.  The heuristic rewards length and
 * structured content, and penalises hallucination marker phrases.
 */
class ILLMProviderReflectionAdapter final : public IReflectionProvider {
public:
    /**
     * @brief Construct the adapter wrapping @p llm_provider.
     *
     * @param llm_provider  Non-null `ILLMProvider` to delegate LLM calls to.
     * @param strategy      Reflection strategy used for prompt construction
     *                      (defaults to `SELF_REFINE`).
     */
    explicit ILLMProviderReflectionAdapter(
        std::shared_ptr<ILLMProvider>       llm_provider,
        ReflectionStrategy                  strategy = ReflectionStrategy::SELF_REFINE);

    // -------------------------------------------------------------------------
    // IReflectionProvider implementation
    // -------------------------------------------------------------------------

    /**
     * @brief Generate an initial response by forwarding @p prompt directly
     *        to the underlying `ILLMProvider::complete()`.
     */
    std::string generate(const std::string& prompt) const override;

    /**
     * @brief Build a strategy-specific critique prompt via
     *        `DynamicReflectionPromptBuilder`, then call `complete()`.
     */
    std::string critique(const std::string& original_prompt,
                         const std::string& response) const override;

    /**
     * @brief Build a strategy-specific revision prompt, then call `complete()`.
     */
    std::string revise(const std::string& original_prompt,
                       const std::string& response,
                       const std::string& critique) const override;

    /**
     * @brief Return the quality score from an injected `IReflectionScorer`,
     *        or from the built-in heuristic when none is set.
     */
    double score(const std::string& prompt,
                 const std::string& response) const override;

    /**
     * @brief Return `"llm-reflection-adapter(<provider-name>)"`.
     */
    std::string name() const override;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

     * @param[in] strategy Input parameter.
    /** @brief Replace the reflection strategy used for prompt construction. */
    void setStrategy(ReflectionStrategy strategy);

     * @return Return value.
     * @note Exception safety: noexcept.
    /** @brief Return the current reflection strategy. */
    ReflectionStrategy getStrategy() const noexcept;

    /**
     * @brief Inject a custom quality scorer, replacing the built-in heuristic.
     * @param scorer  Non-null scorer implementation.
     */
    void setScorer(std::shared_ptr<IReflectionScorer> scorer);

    /** @brief Remove the custom scorer (fall back to built-in heuristic). */
    void clearScorer();

     * @return True on success.
     * @note Exception safety: noexcept.
    /** @brief Return `true` when a custom scorer is attached. */
    bool hasScorer() const noexcept;

private:
    std::shared_ptr<ILLMProvider>       llm_;
    DynamicReflectionPromptBuilder      builder_;
    std::shared_ptr<IReflectionScorer>  scorer_;

     * @param[in] response Input parameter.
     * @return Return value.
    /** @brief Built-in heuristic scorer (length, structure, hallucination markers). */
    double heuristicScore(const std::string& response) const;
};

} // namespace prompt_engineering
} // namespace themis
