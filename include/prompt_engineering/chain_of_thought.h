/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chain_of_thought.h                                 ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:12:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     238                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0ce4ab1155  2026-03-24  fix: address PR review — data race, reflection_max_iterat... ║
    • 93aebd9731  2026-03-23  feat(prompt_engineering): CoT Step Tracer — IChainOfThoug... ║
    • d135ff3ad9  2026-03-09  feat(prompt_engineering): implement ChainOfThoughtBuilder... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file chain_of_thought.h
 * @brief Chain-of-Thought (CoT) prompt construction with step delimiters.
 *
 * Provides utilities to build structured reasoning prompts that guide LLMs
 * through explicit, step-by-step reasoning chains.  Three construction modes
 * are supported:
 *
 *  - **Builder mode** – add named reasoning steps incrementally then call
 *    `build()` to produce the final prompt string.
 *  - **Zero-shot** – append "Let's think step by step." to a question
 *    (`buildZeroShot()`).
 *  - **Few-shot** – prepend solved (question, reasoning+answer) examples
 *    before the target question (`buildFewShot()`).
 *
 * All output is pure text; no LLM inference is performed inside this class.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "cot_tracer.h"

namespace themis {
namespace prompt_engineering {

/**
 * @brief A single step in a chain-of-thought reasoning sequence.
 */
struct CoTStep {
    std::string label;    ///< Human-readable label, e.g. "Step 1" or "Observation"
    std::string content;  ///< Instruction or reasoning text for this step
};

/**
 * @brief Configuration for the ChainOfThoughtBuilder.
 */
struct CoTConfig {
    std::string step_prefix        = "Step ";   ///< Prefix for auto-numbered steps
    std::string step_delimiter     = "\n\n";    ///< Separator inserted between steps
    bool        number_steps       = true;      ///< Auto-number steps when label is empty
    std::string final_answer_label = "Answer:"; ///< Label for the final-answer section
};

/**
 * @brief Builds chain-of-thought prompts with structured reasoning steps.
 *
 * Usage – builder mode:
 * @code
 * ChainOfThoughtBuilder builder;
 * builder.addStep("Identify all entities mentioned in the text.")
 *        .addStep("Determine the relationship between each entity pair.")
 *        .setFinalAnswer("List each relationship on a separate line.");
 * std::string prompt = builder.build();
 * @endcode
 *
 * Usage – zero-shot shortcut:
 * @code
 * auto prompt = ChainOfThoughtBuilder::buildZeroShot("What is 17 × 24?");
 * @endcode
 */
class ChainOfThoughtBuilder {
public:
    /**
     * @brief Construct a builder with the given configuration.
     * @param config CoT generation settings (defaults apply if omitted).
     */
    explicit ChainOfThoughtBuilder(const CoTConfig& config = CoTConfig{});

    /**
     * @brief Append a reasoning step.
     *
     * If @p label is empty and @c config_.number_steps is true the label is
     * auto-generated as "<step_prefix><N>" where N is the 1-based step index.
     *
     * @param content  Instruction or reasoning text.
     * @param label    Optional explicit label (overrides auto-numbering).
     * @return Reference to @c *this for chaining.
     */
    ChainOfThoughtBuilder& addStep(const std::string& content,
                                   const std::string& label = "");

    /**
     * @brief Append a reasoning step whose label is "Reasoning".
     * @param reasoning  Free-form reasoning text.
     * @return Reference to @c *this for chaining.
     */
    ChainOfThoughtBuilder& addReasoningStep(const std::string& reasoning);

    /**
     * @brief Set the final answer section content.
     *
     * The final answer is appended after all steps when `build()` is called,
     * preceded by `config_.final_answer_label`.
     *
     * @param answer  Answer text or instruction.
     * @return Reference to @c *this for chaining.
     */
    ChainOfThoughtBuilder& setFinalAnswer(const std::string& answer);

    /**
     * @brief Assemble all added steps (and optional final answer) into a
     *        single prompt string.
     *
     * When a tracer is attached via `attachTracer()`, this method calls
     * `IChainOfThoughtTracer::onStepBegin()` and `onStepEnd()` for each step
     * before and after it is appended to the output buffer.  These callbacks
     * are declared `noexcept` in `IChainOfThoughtTracer`, so tracer
     * implementations must not throw; doing so would invoke `std::terminate`.
     *
     * @return The complete prompt text.
     */
    std::string build() const;

    /**
     * @brief Remove all steps and the final answer (reset to empty state).
     */
    void clear();

    /**
     * @brief Return the number of steps currently held.
     */
    size_t stepCount() const;

    /**
     * @brief Return a read-only reference to the current configuration.
     */
    const CoTConfig& getConfig() const;

    // -------------------------------------------------------------------------
    // Step Tracer
    // -------------------------------------------------------------------------

    /**
     * @brief Attach an `IChainOfThoughtTracer` to this builder.
     *
     * When set, `build()` calls `onStepBegin`/`onStepEnd` for each step.
     * Passing `nullptr` is equivalent to calling `detachTracer()`.
     *
     * @param tracer  Tracer implementation to attach.
     */
    void attachTracer(std::shared_ptr<IChainOfThoughtTracer> tracer);

    /**
     * @brief Remove the currently attached tracer (no-op if none is set).
     */
    void detachTracer();

    /**
     * @brief Return `true` when a tracer is currently attached.
     */
    bool hasTracer() const noexcept;

    // -------------------------------------------------------------------------
    // Static factory helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Build a zero-shot CoT prompt.
     *
     * Appends the standard zero-shot chain-of-thought trigger sentence
     * "Let's think step by step." after the supplied question.
     *
     * @param question  The question or task to reason about.
     * @return The zero-shot CoT prompt.
     */
    static std::string buildZeroShot(const std::string& question);

    /**
     * @brief Build a few-shot CoT prompt with worked examples.
     *
     * Each example is rendered as:
     * @code
     * Q: <question>
     * A: <reasoning_and_answer>
     * @endcode
     * followed by @c "Q: <question>\nA:" to elicit the model's response.
     *
     * @param question  The target question.
     * @param examples  Pairs of (question, reasoning+answer) serving as
     *                  in-context demonstrations.
     * @return The few-shot CoT prompt string.
     */
    static std::string buildFewShot(
        const std::string& question,
        const std::vector<std::pair<std::string, std::string>>& examples);

    /**
     * @brief Wrap an existing prompt with CoT instructions.
     *
     * Prepends an instruction block that asks the model to reason step by
     * step before answering.  When @p explicit_steps is true, numbered
     * step headings are added to the instruction block.
     *
     * @param prompt         Existing prompt text.
     * @param explicit_steps Add explicit "Step 1 / Step 2 / …" headings.
     * @return The wrapped prompt.
     */
    static std::string wrapWithCoT(const std::string& prompt,
                                   bool explicit_steps = false);

private:
    CoTConfig           config_;
    std::vector<CoTStep> steps_;
    std::string          final_answer_;
    std::shared_ptr<IChainOfThoughtTracer> tracer_;
};

} // namespace prompt_engineering
} // namespace themis
