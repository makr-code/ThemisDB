/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            reflection_tuner.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 07:08:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     580                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7b0078c531  2026-03-23  fix(prompt_engineering): correct line counts in reflectio... ║
    • edb4aad675  2026-03-23  feat(prompt_engineering): implement Reflection Tuning wit... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file reflection_tuner.h
 * @brief Reflection Tuning for dynamic, self-aware LLM prompt improvement.
 *
 * Implements iterative self-critique and revision cycles for LLM responses,
 * grounded in the following peer-reviewed research:
 *
 * - Madaan et al. (NeurIPS 2023) "Self-Refine: Iterative Refinement with
 *   Self-Feedback" — generate → critique → refine loop.
 * - Shinn et al. (NeurIPS 2023) "Reflexion: Language Agents with Verbal
 *   Reinforcement Learning" — episodic verbal reflection memory.
 * - Bai et al. (Anthropic, 2022) "Constitutional AI: Harmlessness from AI
 *   Feedback" — principle-guided critique and revision.
 * - Li et al. (2023) "Reflection-Tuning: Recycling Data for Better
 *   Instruction Tuning" — reflection as data augmentation for fine-tuning.
 *
 * @note Risk mitigation: As reported in "Reflection Tuning bei KI:
 *   Selbstkritik bis hin zur Halluzination" (Golem.de, 2026-03), reflection
 *   cycles can amplify hallucinations when the model confidently critiques
 *   correct information.  The `ReflectionHallucinationGuard` detects quality
 *   divergence and halts the cycle before errors are compounded.
 *
 * ## Architecture
 *
 *  ┌──────────────────────────────────────────────────────┐
 *  │                   ReflectionTuner                    │
 *  │                                                      │
 *  │  tune(prompt, initial_response)                      │
 *  │  ┌────────────────────────────────────────────────┐  │
 *  │  │ for iter in [0, max_iterations):               │  │
 *  │  │   ctx  = SelfAwareContext::fromResponse(resp)  │  │
 *  │  │   crit = provider.critique(prompt, resp, ctx)  │  │
 *  │  │   resp = provider.revise(prompt, resp, crit)   │  │
 *  │  │   score= provider.score(prompt, resp)          │  │
 *  │  │   if guard.shouldHalt(steps) → break (guard)   │  │
 *  │  │   if shouldConverge(result)  → break (ok)      │  │
 *  │  └────────────────────────────────────────────────┘  │
 *  └──────────────────────────────────────────────────────┘
 *
 * All classes are pure computation helpers; no LLM inference or network I/O
 * is performed by the module itself.
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// ReflectionStrategy
// ============================================================================

/**
 * @brief Selects the prompt template family used for the critique step.
 *
 * | Strategy       | Paper                        | Critique style              |
 * |----------------|------------------------------|-----------------------------|
 * | SELF_REFINE    | Madaan et al. NeurIPS 2023   | General quality critique    |
 * | REFLEXION      | Shinn et al. NeurIPS 2023    | Verbal reinforcement memory |
 * | CONSTITUTIONAL | Bai et al. Anthropic 2022    | Principle-guided critique   |
 * | SOCRATIC       | Socratic method              | Probing assumption questions|
 */
enum class ReflectionStrategy {
    SELF_REFINE,     ///< Generate → critique → refine (Madaan et al., NeurIPS 2023)
    REFLEXION,       ///< Act → reflect → episodic memory → act (Shinn et al., NeurIPS 2023)
    CONSTITUTIONAL,  ///< Critique against explicit principles (Bai et al., Anthropic 2022)
    SOCRATIC         ///< Expose hidden assumptions via questioning
};

// ============================================================================
// ReflectionConfig
// ============================================================================

/**
 * @brief Configuration for one `ReflectionTuner` instance.
 */
struct ReflectionConfig {
    /** @brief Strategy that determines the critique-prompt template family. */
    ReflectionStrategy strategy = ReflectionStrategy::SELF_REFINE;

    /** @brief Maximum number of generate→critique→revise iterations. */
    size_t max_iterations = 3;

    /**
     * @brief Stop iterating when quality score reaches this value.
     * Value in [0.0, 1.0].
     */
    double convergence_threshold = 0.95;

    /**
     * @brief Stop iterating when the per-step quality gain falls below
     * this value (plateau detection).  Value in [0.0, 1.0].
     */
    double min_delta_improvement = 0.02;

    /** @brief Enable the divergence-based hallucination guard. */
    bool hallucination_guard_enabled = true;

    /**
     * @brief Halt when the rolling quality average drops by at least this
     * fraction relative to the peak within the divergence window.
     */
    double divergence_threshold = 0.15;

    /**
     * @brief Number of trailing steps inspected by the divergence detector.
     */
    size_t divergence_window = 2;

    /**
     * @brief Principles used when strategy == CONSTITUTIONAL.
     * Each string should be one imperative sentence, e.g.
     * "The response must not contain personally identifiable information."
     */
    std::vector<std::string> constitutional_principles;

    /**
     * @brief When true, the self-aware context header (confidence, uncertainty
     * markers) is injected into the critique and revision prompts.
     */
    bool include_self_aware_context = true;
};

// ============================================================================
// ReflectionStep
// ============================================================================

/**
 * @brief Record of a single generate→critique→revise iteration.
 */
struct ReflectionStep {
    size_t      iteration              = 0;    ///< Zero-based iteration index
    std::string response;                      ///< Model response after revision
    std::string critique;                      ///< Self-generated critique text
    double      quality_score          = 0.0;  ///< Evaluated quality in [0.0, 1.0]
    double      quality_delta          = 0.0;  ///< Quality change vs previous step
    bool        hallucination_suspected = false; ///< Guard detected a hallucination signal
    nlohmann::json metadata = nlohmann::json::object(); ///< Provider-specific extras
};

// ============================================================================
// SelfAwareContext
// ============================================================================

/**
 * @brief Captures the model's self-reported confidence and uncertainty.
 *
 * Extracted from the response text by scanning for linguistic confidence
 * and uncertainty markers.  The resulting confidence score and uncertainty
 * marker list are injected into critique prompts to produce **dynamic**,
 * context-sensitive prompts — the "self-aware AI" component.
 *
 * High uncertainty → critique emphasises factual verification.
 * High confidence  → critique checks for overconfidence masking errors.
 *
 * Usage:
 * @code
 * auto ctx = SelfAwareContext::fromResponse(response_text);
 * if (ctx.has_uncertain_claims) {
 *     // use a more sceptical critique template
 * }
 * @endcode
 */
struct SelfAwareContext {
    /**
     * @brief Self-reported confidence level in [0.0, 1.0].
     *
     * Derived from the ratio of confident-language markers to uncertain-
     * language markers found in the response.  Defaults to 0.7 (neutral).
     */
    double confidence = 0.7;

    /** @brief Domain claimed by the response (empty if none detected). */
    std::string domain;

    /** @brief Uncertainty markers found in the response text. */
    std::vector<std::string> uncertainty_markers;

    /** @brief True when at least one uncertainty marker was detected. */
    bool has_uncertain_claims = false;

    /** @brief Serialise to JSON for logging and metadata storage. */
    nlohmann::json toJson() const;

    /**
     * @brief Scan @p response for confidence/uncertainty markers and return
     *        the resulting `SelfAwareContext`.
     * @param response UTF-8 response text to analyse.
     */
    static SelfAwareContext fromResponse(const std::string& response);
};

// ============================================================================
// ReflectionResult
// ============================================================================

/**
 * @brief Complete result of a `ReflectionTuner::tune()` call.
 */
struct ReflectionResult {
    std::string             final_response;                    ///< Best response after all iterations
    std::vector<ReflectionStep> steps;                        ///< Per-iteration trace
    bool                    converged                  = false; ///< True when quality threshold met or plateau detected
    bool                    halted_by_hallucination_guard = false; ///< True when guard fired
    size_t                  total_iterations           = 0;    ///< Number of completed iterations
    double                  initial_quality            = 0.0;  ///< Score before any reflection
    double                  final_quality              = 0.0;  ///< Score of the final response
    double                  quality_improvement        = 0.0;  ///< final_quality - initial_quality
    std::vector<double>     quality_trajectory;               ///< Score at step[0] = initial, step[i+1] = after iter i
    SelfAwareContext        self_aware_context;               ///< Context extracted from last response
    nlohmann::json          metadata = nlohmann::json::object();

    /** @brief Serialise to JSON for storage and observability. */
    nlohmann::json toJson() const;
};

// ============================================================================
// IReflectionProvider
// ============================================================================

/**
 * @brief Abstract interface for a reflection-capable LLM backend.
 *
 * Implement this to integrate any LLM that can generate, critique, and revise
 * text.  A single LLM can implement all four methods by forwarding to its
 * `complete()` endpoint with appropriately constructed prompts — use
 * `DynamicReflectionPromptBuilder` to construct those prompts.
 *
 * @code
 * class MyLLMReflectionAdapter : public IReflectionProvider {
 *     DynamicReflectionPromptBuilder builder_;
 *     MyLLM llm_;
 * public:
 *     std::string generate(const std::string& prompt) const override {
 *         return llm_.complete(prompt);
 *     }
 *     std::string critique(const std::string& p, const std::string& r) const override {
 *         return llm_.complete(builder_.buildCritiquePrompt(p, r));
 *     }
 *     std::string revise(const std::string& p, const std::string& r, const std::string& c) const override {
 *         return llm_.complete(builder_.buildRevisionPrompt(p, r, c));
 *     }
 *     double score(const std::string&, const std::string& r) const override {
 *         // implement quality heuristic or call a judge model
 *         return r.empty() ? 0.0 : 0.7;
 *     }
 *     std::string name() const override { return "my-llm"; }
 * };
 * @endcode
 */
class IReflectionProvider {
public:
    virtual ~IReflectionProvider() = default;

    /** @brief Generate an initial response to @p prompt. */
    virtual std::string generate(const std::string& prompt) const = 0;

    /**
     * @brief Critique @p response in the context of @p original_prompt.
     * @return Critique text (actionable feedback).
     */
    virtual std::string critique(const std::string& original_prompt,
                                 const std::string& response) const = 0;

    /**
     * @brief Revise @p response by applying @p critique.
     * @return Improved response text.
     */
    virtual std::string revise(const std::string& original_prompt,
                                const std::string& response,
                                const std::string& critique) const = 0;

    /**
     * @brief Evaluate the quality of @p response for @p prompt.
     * @return Quality score in [0.0, 1.0].
     */
    virtual double score(const std::string& prompt,
                         const std::string& response) const = 0;

    /** @brief Human-readable provider name (for logs and metadata). */
    virtual std::string name() const = 0;
};

// ============================================================================
// DynamicReflectionPromptBuilder
// ============================================================================

/**
 * @brief Builds strategy-specific, self-aware critique and revision prompts.
 *
 * The "dynamic" nature comes from injecting the `SelfAwareContext` extracted
 * from the model's own response back into the next critique prompt, creating
 * a feedback loop:
 *
 *   response → SelfAwareContext::fromResponse() → context-adaptive critique
 *
 * When @p ctx indicates low confidence, the critique template emphasises
 * factual verification.  When @p ctx indicates high confidence, the template
 * checks for overconfidence that may mask inaccuracies.
 */
class DynamicReflectionPromptBuilder {
public:
    /** @brief Construct with the given reflection strategy. */
    explicit DynamicReflectionPromptBuilder(
        ReflectionStrategy strategy = ReflectionStrategy::SELF_REFINE);

    /**
     * @brief Build a critique prompt using the current strategy.
     *
     * @param original_prompt  The original task prompt sent to the LLM.
     * @param response         The response to be critiqued.
     * @param ctx              Self-aware context from the response (optional).
     */
    std::string buildCritiquePrompt(
        const std::string& original_prompt,
        const std::string& response,
        const SelfAwareContext& ctx = SelfAwareContext{}) const;

    /**
     * @brief Build a revision prompt instructing the LLM to revise @p response
     *        based on @p critique.
     */
    std::string buildRevisionPrompt(
        const std::string& original_prompt,
        const std::string& response,
        const std::string& critique,
        const SelfAwareContext& ctx = SelfAwareContext{}) const;

    /**
     * @brief Build a constitutional critique prompt (CONSTITUTIONAL strategy).
     *
     * Evaluates @p response against each principle in @p principles.
     */
    std::string buildConstitutionalCritiquePrompt(
        const std::string& response,
        const std::vector<std::string>& principles) const;

    /**
     * @brief Build a Socratic questioning prompt (SOCRATIC strategy).
     *
     * Cycles through a set of probing questions; @p iteration selects the
     * question modulo the question count.
     */
    std::string buildSocraticPrompt(
        const std::string& claim,
        size_t iteration) const;

    /**
     * @brief Build the self-aware context preamble injected at the start of
     *        critique/revision prompts when the context is meaningful.
     *
     * Returns an empty string when the context is neutral (no uncertainty
     * markers and confidence ≥ 0.7).
     */
    std::string buildSelfAwareContextHeader(const SelfAwareContext& ctx) const;

    /** @brief Replace the current strategy. */
    void setStrategy(ReflectionStrategy strategy);

    /** @brief Return the current strategy. */
    ReflectionStrategy getStrategy() const noexcept;

private:
    ReflectionStrategy strategy_;
};

// ============================================================================
// ReflectionHallucinationGuard
// ============================================================================

/**
 * @brief Detects quality divergence and hallucination signals in the
 *        reflection trace, halting the cycle before errors are amplified.
 *
 * Two detection mechanisms:
 *
 * 1. **Marker detection** – Scans response and critique text for phrases that
 *    indicate the model is self-correcting fabricated content (e.g. "I was
 *    wrong", "my previous response was incorrect").
 *
 * 2. **Divergence detection** – Monitors the quality trajectory; halts if the
 *    rolling average of the last `window_` steps drops more than
 *    `divergence_threshold_` below the preceding step's score.
 *
 * @see Golem.de (2026-03): "Reflection Tuning bei KI: Selbstkritik bis hin
 *      zur Halluzination" — empirical motivation for this guard.
 */
class ReflectionHallucinationGuard {
public:
    /**
     * @param divergence_threshold  Quality drop that triggers a halt (default: 0.15).
     * @param window                Trailing steps used for rolling average (default: 2).
     */
    explicit ReflectionHallucinationGuard(double divergence_threshold = 0.15,
                                          size_t window = 2);

    /**
     * @brief Return true if the reflection cycle should be halted based on
     *        the current step history.
     *
     * Checks both the last step's `hallucination_suspected` flag and the
     * quality trajectory via `isDiverging()`.
     */
    bool shouldHalt(const std::vector<ReflectionStep>& steps) const;

    /**
     * @brief Scan @p response and @p critique for hallucination-indicating
     *        patterns.
     * @return true if at least one pattern is found.
     */
    bool detectHallucinationSignals(const std::string& response,
                                    const std::string& critique) const;

    /**
     * @brief Return true when the rolling quality average of the last
     *        `window_` steps has dropped by more than `divergence_threshold_`
     *        relative to the step at `trajectory[n - window_ - 1]`.
     *
     * Returns false when `trajectory.size() < window_ + 1`.
     */
    bool isDiverging(const std::vector<double>& trajectory) const;

    double getDivergenceThreshold() const noexcept { return divergence_threshold_; }
    size_t getWindow() const noexcept { return window_; }

    /** @brief Hallucination marker phrases used by `detectHallucinationSignals()`. */
    static const std::vector<std::string> kHallucinationMarkers;

private:
    double divergence_threshold_;
    size_t window_;
};

// ============================================================================
// ReflectionTuner
// ============================================================================

/**
 * @brief Orchestrates the reflection tuning cycle for a single LLM response.
 *
 * The `tune()` method runs a configurable number of generate→critique→revise
 * iterations, stopping early when the quality converges, plateaus, or the
 * hallucination guard fires.
 *
 * When no `IReflectionProvider` is attached, the tuner operates in
 * **fallback mode**: critique prompts are generated by the
 * `DynamicReflectionPromptBuilder` (returned in the step metadata for callers
 * to forward to an LLM), and the response is scored heuristically.
 *
 * Usage with a provider:
 * @code
 * ReflectionTuner tuner;
 * tuner.setConfig({.strategy = ReflectionStrategy::REFLEXION, .max_iterations = 4});
 * tuner.setReflectionProvider(std::make_shared<MyReflectionAdapter>());
 *
 * auto result = tuner.tune("Summarise the GDPR.", initial_draft);
 * if (result.converged) {
 *     use(result.final_response);
 * } else if (result.halted_by_hallucination_guard) {
 *     // use the best step before divergence
 *     use(result.steps.front().response);
 * }
 * @endcode
 */
class ReflectionTuner {
public:
    /** @brief Construct with an optional configuration. */
    explicit ReflectionTuner(const ReflectionConfig& config = ReflectionConfig{});

    // -------------------------------------------------------------------------
    // Main entry points
    // -------------------------------------------------------------------------

    /**
     * @brief Run the full reflection cycle starting from @p initial_response.
     *
     * @param prompt            Original task prompt (used for context in critiques).
     * @param initial_response  Pre-generated response to start the cycle from.
     * @return `ReflectionResult` with the refined response and full trace.
     */
    ReflectionResult tune(const std::string& prompt,
                          const std::string& initial_response);

    /**
     * @brief Generate an initial response and run the full reflection cycle.
     *
     * Requires an `IReflectionProvider` to be attached for the generation step;
     * in fallback mode the original @p prompt is used as the initial response.
     */
    ReflectionResult tuneFromPrompt(const std::string& prompt);

    // -------------------------------------------------------------------------
    // Provider management
    // -------------------------------------------------------------------------

    /** @brief Attach a reflection-capable LLM provider. */
    void setReflectionProvider(std::shared_ptr<IReflectionProvider> provider);

    /** @brief Remove the attached provider (fall back to template/heuristic mode). */
    void clearReflectionProvider();

    /** @brief Returns true if a provider is currently attached. */
    bool hasReflectionProvider() const noexcept;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** @brief Return the current configuration. */
    const ReflectionConfig& getConfig() const noexcept;

    /**
     * @brief Replace the configuration.
     *
     * Also updates the embedded `DynamicReflectionPromptBuilder` strategy and
     * `ReflectionHallucinationGuard` thresholds.
     */
    void setConfig(const ReflectionConfig& config);

    // -------------------------------------------------------------------------
    // Sub-component access (for testing and observability)
    // -------------------------------------------------------------------------

    /** @brief Read-only access to the embedded prompt builder. */
    const DynamicReflectionPromptBuilder& getPromptBuilder() const noexcept;

    /** @brief Read-only access to the embedded hallucination guard. */
    const ReflectionHallucinationGuard& getHallucinationGuard() const noexcept;

private:
    ReflectionConfig                  config_;
    std::shared_ptr<IReflectionProvider> provider_;
    DynamicReflectionPromptBuilder    prompt_builder_;
    ReflectionHallucinationGuard      hallucination_guard_;

    ReflectionStep runIteration(const std::string& prompt,
                                const std::string& current_response,
                                size_t iteration,
                                const SelfAwareContext& ctx);

    bool shouldConverge(const ReflectionResult& result,
                        const ReflectionStep& step) const;

    double computeHeuristicScore(const std::string& prompt,
                                 const std::string& response) const;

    SelfAwareContext extractContext(const std::vector<ReflectionStep>& steps) const;
};

} // namespace prompt_engineering
} // namespace themis
