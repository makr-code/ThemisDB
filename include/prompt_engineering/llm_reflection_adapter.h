/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_reflection_adapter.h                           ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 18:04:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7d8f5cfa2b  2026-03-23  feat(prompt_engineering): Reflection Tuning integration —... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_reflection_adapter.h
 * @brief Adapter bridging ILLMProvider → IReflectionProvider.
 *
 * Allows any existing `ILLMProvider` implementation (from
 * `meta_prompt_generator.h`) to be used as a `ReflectionTuner` backend
 * without writing additional glue code:
 *
 * @code
 * auto adapter = std::make_shared<ILLMProviderReflectionAdapter>(my_llm_provider);
 * ReflectionTuner tuner;
 * tuner.setReflectionProvider(adapter);
 * auto result = tuner.tune("task", initial_response);
 * @endcode
 *
 * The adapter uses `DynamicReflectionPromptBuilder` to construct the
 * strategy-specific critique and revision prompts, then forwards them
 * to `ILLMProvider::complete()`.  Quality scoring uses a heuristic
 * fallback (response length + structural signals) unless a custom
 * `IReflectionScorer` is provided.
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
    virtual ~IReflectionScorer() = default;

    /**
     * @brief Evaluate the quality of @p response for @p prompt.
     * @return Quality score in [0.0, 1.0].
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

    /** @brief Replace the reflection strategy used for prompt construction. */
    void setStrategy(ReflectionStrategy strategy);

    /** @brief Return the current reflection strategy. */
    ReflectionStrategy getStrategy() const noexcept;

    /**
     * @brief Inject a custom quality scorer, replacing the built-in heuristic.
     * @param scorer  Non-null scorer implementation.
     */
    void setScorer(std::shared_ptr<IReflectionScorer> scorer);

    /** @brief Remove the custom scorer (fall back to built-in heuristic). */
    void clearScorer();

    /** @brief Return `true` when a custom scorer is attached. */
    bool hasScorer() const noexcept;

private:
    std::shared_ptr<ILLMProvider>       llm_;
    DynamicReflectionPromptBuilder      builder_;
    std::shared_ptr<IReflectionScorer>  scorer_;

    /** @brief Built-in heuristic scorer (length, structure, hallucination markers). */
    double heuristicScore(const std::string& response) const;
};

} // namespace prompt_engineering
} // namespace themis
