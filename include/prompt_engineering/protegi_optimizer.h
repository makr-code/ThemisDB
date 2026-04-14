/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            protegi_optimizer.h                                ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 11:27:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     292                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 696d2d349b  2026-03-24  fix: address 7 Copilot review comments (docs, beam_width ... ║
    • b87706b26d  2026-03-24  feat(prompt_engineering): implement ToT reasoner, ProTeGi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file protegi_optimizer.h
 * @brief ProTeGi – textual gradient descent prompt optimizer.
 *
 * Implements the "Automatic Prompt Optimization with Gradient Descent and
 * Beam Search" algorithm (Pryzant et al., EMNLP 2023).  Instead of numeric
 * gradients the optimizer uses an LLM to:
 *   1. Run a mini-batch of inputs through the current prompt and collect errors.
 *   2. Generate a natural-language "gradient" (critique) explaining what went wrong.
 *   3. Produce a set of improved prompt candidates guided by the critique.
 *   4. Evaluate candidates on a held-out set and retain the top-k (beam).
 *   5. Repeat until convergence or the maximum number of steps is reached.
 *
 * The optimizer integrates with the existing @c PromptOptimizer infrastructure
 * through the shared @c EvaluationFunction / @c TestCase types defined in
 * @c prompt_optimizer.h.
 *
 * Reference:
 *   R. Pryzant et al., "Automatic Prompt Optimization with 'Gradient Descent'
 *   and Beam Search," in Proc. EMNLP 2023, pp. 7957–7968, 2023.
 *   Available: https://arxiv.org/abs/2305.03495
 */

#pragma once

#include "prompt_engineering/prompt_optimizer.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration knobs for the ProTeGi optimizer.
 */
struct ProTeGiConfig {
    size_t max_steps         = 5;    ///< Maximum optimization steps (outer loop).
    size_t beam_width        = 4;    ///< Number of candidate prompts retained between steps.
    size_t num_candidates    = 4;    ///< New prompt candidates generated per gradient step.
    size_t mini_batch_size   = 8;    ///< Inputs sampled per gradient computation.
    double target_score      = 0.9;  ///< Early-stop when best beam score ≥ this value.
    double min_improvement   = 0.01; ///< Stop when best-beam improvement drops below this.
};

// ---------------------------------------------------------------------------
// Gradient representation
// ---------------------------------------------------------------------------

/**
 * @brief Natural-language gradient produced by the critique step.
 *
 * Mirrors the concept of a numeric gradient: it describes in natural language
 * why the current prompt failed on the mini-batch and what to change.
 */
struct ProTeGiGradient {
    std::string critique;             ///< High-level explanation of current prompt failures.
    std::vector<std::string> errors;  ///< Per-example failure descriptions from the mini-batch.
    double error_rate = 0.0;          ///< Fraction of mini-batch examples that failed.
};

// ---------------------------------------------------------------------------
// Result
// ---------------------------------------------------------------------------

/**
 * @brief Complete result of a ProTeGi optimization run.
 */
struct ProTeGiResult {
    std::string              best_prompt;          ///< Prompt with the highest score at end of run.
    double                   best_score  = 0.0;    ///< Score of @c best_prompt on the full test set.
    size_t                   steps       = 0;      ///< Number of gradient steps taken.
    bool                     converged   = false;  ///< True when target_score reached or no improvement.
    std::vector<double>      score_history;        ///< Best beam score after each step.
    std::vector<std::string> beam;                 ///< Final beam of candidate prompts (best first).
};

// ---------------------------------------------------------------------------
// LLM provider interface
// ---------------------------------------------------------------------------

/**
 * @brief Interface that the ProTeGi optimizer calls for LLM-dependent operations.
 *
 * Callers inject an implementation backed by any LLM.  A built-in
 * @c HeuristicProTeGiProvider is supplied for unit testing without a live model.
 */
class IProTeGiLLMProvider {
public:
    virtual ~IProTeGiLLMProvider() = default;

    /**
     * @brief Compute a textual gradient from mini-batch errors.
     *
     * @param prompt     The current prompt being evaluated.
     * @param errors     Per-example failure strings collected from the mini-batch.
     * @return ProTeGiGradient describing what to change and why.
     */
    virtual ProTeGiGradient computeGradient(
        const std::string& prompt,
        const std::vector<std::string>& errors) = 0;

    /**
     * @brief Generate prompt candidates from the gradient.
     *
     * @param prompt    The current prompt.
     * @param gradient  Textual gradient computed by @c computeGradient.
     * @param k         Number of candidate prompts to generate.
     * @return Vector of up to @p k candidate prompt strings.
     */
    virtual std::vector<std::string> generateCandidates(
        const std::string& prompt,
        const ProTeGiGradient& gradient,
        size_t k) = 0;
};

// ---------------------------------------------------------------------------
// Built-in heuristic provider (no LLM required for unit tests)
// ---------------------------------------------------------------------------

/**
 * @brief Heuristic ProTeGi provider that operates without an LLM.
 *
 * The critique is derived from keyword analysis of the error strings.
 * Candidate prompts are generated by appending structured improvement
 * guidance based on the critique.  Useful for integration tests.
 */
class HeuristicProTeGiProvider : public IProTeGiLLMProvider {
public:
    ProTeGiGradient computeGradient(
        const std::string& prompt,
        const std::vector<std::string>& errors) override;

    std::vector<std::string> generateCandidates(
        const std::string& prompt,
        const ProTeGiGradient& gradient,
        size_t k) override;
};

// ---------------------------------------------------------------------------
// ProTeGi optimizer
// ---------------------------------------------------------------------------

/**
 * @brief ProTeGi – textual gradient descent optimizer.
 *
 * Usage:
 * @code
 * ProTeGiConfig cfg;
 * cfg.beam_width     = 4;
 * cfg.num_candidates = 4;
 * cfg.max_steps      = 10;
 *
 * ProTeGiOptimizer opt(cfg);
 * opt.setLLMProvider(std::make_shared<MyLLMProvider>());
 *
 * ProTeGiResult result = opt.optimize(
 *     initial_prompt,
 *     test_cases,
 *     my_eval_fn,
 *     mini_batch_error_fn  // optional: extract error strings from mini-batch
 * );
 * std::cout << result.best_prompt << "\n";
 * @endcode
 */
class ProTeGiOptimizer {
public:
    /**
     * @brief Function type for extracting per-example error strings from a mini-batch.
     *
     * Takes the prompt and a subset of test cases; returns one error string per
     * failed case (empty string for a passing case).
     */
    using MiniBatchErrorFn = std::function<std::vector<std::string>(
        const std::string& prompt,
        const std::vector<TestCase>& mini_batch)>;

    /**
     * @brief Construct with given configuration.
     */
    explicit ProTeGiOptimizer(const ProTeGiConfig& config = ProTeGiConfig{});

    /**
     * @brief Inject the LLM provider used for gradient and candidate generation.
     *
     * If not called, a @c HeuristicProTeGiProvider is used automatically.
     */
    ProTeGiOptimizer& setLLMProvider(std::shared_ptr<IProTeGiLLMProvider> provider);

    /**
     * @brief Run ProTeGi optimization.
     *
     * @param initial_prompt   Starting prompt text.
     * @param test_cases       Full evaluation corpus.
     * @param eval_fn          Scoring function (0–1 range).
     * @param error_fn         Optional: per-example error extractor for mini-batch.
     *                         Defaults to a simple threshold-based fallback.
     * @return ProTeGiResult   Best prompt and convergence diagnostics.
     */
    ProTeGiResult optimize(
        const std::string& initial_prompt,
        const std::vector<TestCase>& test_cases,
        EvaluationFunction eval_fn,
        MiniBatchErrorFn error_fn = nullptr);

    /**
     * @brief Return the current configuration.
     */
    const ProTeGiConfig& getConfig() const;

    /**
     * @brief Update the configuration.
     */
    void setConfig(const ProTeGiConfig& config);

    // -------------------------------------------------------------------------
    // Static prompt builders (exposed for testing and introspection)
    // -------------------------------------------------------------------------

    /**
     * @brief Build the meta-prompt used to compute a textual gradient.
     *
     * @param prompt   Current prompt.
     * @param errors   Per-example failure descriptions.
     * @return Formatted gradient-computation prompt.
     */
    static std::string buildGradientPrompt(
        const std::string& prompt,
        const std::vector<std::string>& errors);

    /**
     * @brief Build the meta-prompt used to generate improved candidates.
     *
     * @param prompt    Current prompt.
     * @param gradient  Textual gradient.
     * @param k         Number of candidates to request.
     * @return Formatted candidate-generation prompt.
     */
    static std::string buildCandidatePrompt(
        const std::string& prompt,
        const ProTeGiGradient& gradient,
        size_t k);

private:
    ProTeGiConfig                         config_;
    std::shared_ptr<IProTeGiLLMProvider>  llm_provider_;

    // Sample a mini-batch of test cases uniformly without replacement.
    std::vector<TestCase> sampleMiniBatch(
        const std::vector<TestCase>& test_cases,
        size_t n) const;

    // Default error extractor: uses a simple heuristic based on the relation
    // between the prompt and each test case's expected output (e.g., treating
    // cases where the expected output is longer than the prompt as failures)
    // to produce generic error messages when no custom error_fn is provided.
    static std::vector<std::string> defaultErrorFn(
        const std::string& prompt,
        const std::vector<TestCase>& mini_batch);
};

} // namespace prompt_engineering
} // namespace themis
