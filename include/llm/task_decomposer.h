/**
 * @file task_decomposer.h
 * @brief Prompt-enhancement-based task decomposition for LLM inference orchestration.
 *
 * TaskDecomposer breaks a complex task description into a list of smaller,
 * independently solvable subtasks by constructing a structured meta-prompt
 * that instructs the LLM to produce a JSON decomposition plan.  The resulting
 * subtasks can then be executed sequentially or as a WorkflowDefinition.
 *
 * ## Design
 *
 * 1. A _decomposition prompt_ is built from the original task using
 *    configurable strategies (chain-of-thought, few-shot, structured-output).
 * 2. The LLM is asked to return a JSON array of `{ "id", "description",
 *    "prompt", "depends_on" }` objects.
 * 3. The decomposer validates the JSON and optionally converts the result
 *    into a `WorkflowDefinition` suitable for direct execution.
 *
 * ## Integration
 *
 * @code
 * TaskDecomposerConfig cfg;
 * cfg.strategy = DecompositionStrategy::ChainOfThought;
 * cfg.max_subtasks = 5;
 *
 * TaskDecomposer decomposer(cfg);
 * decomposer.setLLMPlugin(my_plugin);
 *
 * auto result = decomposer.decompose("Analyse and summarise the Q3 earnings report");
 * if (result.success) {
 *     for (auto& sub : result.subtasks) {
 *         std::cout << sub.id << ": " << sub.description << "\n";
 *     }
 *     auto workflow = decomposer.toWorkflow(result);
 *     // execute workflow via WorkflowExecutor …
 * }
 * @endcode
 *
 * ## Thread Safety
 *
 * TaskDecomposer is thread-safe; multiple threads may call decompose()
 * concurrently.  The LLM plugin must itself be thread-safe.
 */

#pragma once

#include "llm/i_llm_plugin.h"
#include "llm/workflow_definition.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis::llm {

using json = nlohmann::json;

// ============================================================================
// § 1  Configuration
// ============================================================================

/**
 * @brief Decomposition strategy used to build the meta-prompt.
 */
enum class DecompositionStrategy {
    /**
     * @brief Chain-of-thought: ask the LLM to reason step-by-step before
     * emitting the JSON subtask list.
     */
    ChainOfThought,

    /**
     * @brief Structured output only: emit the JSON array directly without
     * explicit reasoning (faster, lower token cost).
     */
    DirectJson,

    /**
     * @brief Few-shot: prepend user-supplied examples of task → subtask
     * decompositions to steer the LLM output format.
     */
    FewShot,
};

/** @brief Convert DecompositionStrategy to a human-readable string. */
[[nodiscard]] std::string decompositionStrategyToString(DecompositionStrategy s);

/**
 * @brief Example pair used by the FewShot strategy.
 */
struct DecompositionExample {
    std::string task;        ///< Example complex task string
    std::vector<json> steps; ///< Expected JSON subtask array for that task
};

/**
 * @brief Configuration for TaskDecomposer.
 */
struct TaskDecomposerConfig {
    /// Decomposition strategy (default: ChainOfThought).
    DecompositionStrategy strategy = DecompositionStrategy::ChainOfThought;

    /**
     * @brief Maximum number of subtasks the LLM may return.
     *
     * The decomposition prompt includes this limit as an instruction.
     * Subtasks beyond the limit are silently dropped.  0 = no limit.
     */
    int max_subtasks = 8;

    /**
     * @brief Minimum number of subtasks required.
     *
     * If the LLM returns fewer subtasks the decomposition is retried once.
     * 0 = no minimum.
     */
    int min_subtasks = 1;

    /**
     * @brief Maximum token budget for the decomposition call.
     * 0 = use model default.
     */
    int max_tokens = 1024;

    /// Temperature for the decomposition inference call (negative = model default).
    float temperature = 0.2f;

    /**
     * @brief Domain context injected into the decomposition prompt.
     *
     * Example: "database query optimisation" causes the LLM to tailor
     * subtask granularity to that domain.
     */
    std::string domain_context;

    /**
     * @brief Language for the generated subtask descriptions.
     * Empty string = let the LLM choose based on the input language.
     */
    std::string output_language;

    /**
     * @brief Few-shot examples; only used when strategy == FewShot.
     *
     * At least one example is required when the FewShot strategy is selected.
     */
    std::vector<DecompositionExample> few_shot_examples;

    /**
     * @brief Retry count when the LLM returns unparseable JSON or too few
     * subtasks (default: 1).
     */
    int max_retries = 1;
};

// ============================================================================
// § 2  Result types
// ============================================================================

/**
 * @brief A single decomposed subtask returned by the LLM.
 */
struct SubTask {
    /// Unique identifier within the decomposition (LLM-generated or assigned).
    std::string id = {};

    /// Short human-readable description of the subtask.
    std::string description;

    /**
     * @brief Ready-to-use prompt for this subtask.
     *
     * May contain `{input}` or `{<prev_id>.output}` placeholders if the LLM
     * produced chained prompts.
     */
    std::string prompt;

    /**
     * @brief IDs of subtasks that must complete before this one.
     * An empty list means the subtask can run independently.
     */
    std::vector<std::string> depends_on;

    /// Raw JSON object as returned by the LLM (preserved for diagnostics).
    json raw;
};

/**
 * @brief Result of a TaskDecomposer::decompose() call.
 */
struct TaskDecompositionResult {
    /// True when decomposition succeeded and at least one subtask was returned.
    bool success = false;

    /// Ordered list of subtasks (may be empty on failure).
    std::vector<SubTask> subtasks;

    /**
     * @brief The meta-prompt that was sent to the LLM.
     * Preserved for debugging and audit.
     */
    std::string decomposition_prompt;

    /**
     * @brief Raw LLM response text.
     * Preserved for debugging and audit.
     */
    std::string raw_llm_response;

    /**
     * @brief Error message when success == false.
     */
    std::string error;

    /// Number of LLM calls made (1 + retries).
    int llm_calls_made = 0;

    /// Total tokens consumed across all LLM calls.
    size_t tokens_consumed = 0;
};

// ============================================================================
// § 3  TaskDecomposer
// ============================================================================

/**
 * @brief Decomposes a complex task into subtasks using prompt enhancement.
 *
 * The decomposer constructs a structured meta-prompt from the original task
 * description and the configured strategy, sends it to the LLM, and parses
 * the response into a list of @ref SubTask objects.
 *
 * The result can optionally be converted to a @ref WorkflowDefinition for
 * execution via a workflow executor.
 *
 * ### Prompt structure (ChainOfThought)
 *
 * ```
 * You are a task-planning assistant. Break the following task into at most N
 * concrete, independently executable subtasks.  Think step by step, then
 * return ONLY a JSON array with objects:
 *   { "id": "...", "description": "...", "prompt": "...", "depends_on": [...] }
 *
 * Task: <original task>
 * ```
 */
class TaskDecomposer {
public:
    /**
     * @brief Construct a TaskDecomposer with the given configuration.
     * @param config  Decomposer configuration.
     */
    explicit TaskDecomposer(const TaskDecomposerConfig& config = TaskDecomposerConfig{});

    ~TaskDecomposer();

    // No copy — owns the LLM plugin reference.
    TaskDecomposer(const TaskDecomposer&)            = delete;
    TaskDecomposer& operator=(const TaskDecomposer&) = delete;

    // Move is supported.
    TaskDecomposer(TaskDecomposer&&)            noexcept;
    TaskDecomposer& operator=(TaskDecomposer&&) noexcept;

    // ── Configuration ────────────────────────────────────────────────────────

    /**
     * @brief Set the LLM plugin used for decomposition inference.
     * @param plugin  Plugin instance (must outlive this decomposer or be owned
     *                via shared_ptr lifetime).
     */
    void setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin);

    /**
     * @brief Update the decomposer configuration.
     * @param config  New configuration; applied from the next decompose() call.
     */
    void setConfig(const TaskDecomposerConfig& config);

    /** @brief Return a copy of the current configuration. */
    [[nodiscard]] TaskDecomposerConfig config() const;

    // ── Core operation ───────────────────────────────────────────────────────

    /**
     * @brief Decompose a complex task into subtasks.
     *
     * Builds a meta-prompt according to the configured strategy, calls the
     * LLM, parses the JSON response, and returns the subtask list.
     *
     * On parse failure or too-few-subtasks the call is retried up to
     * `config.max_retries` times with a revised prompt.
     *
     * @param task        The complex task description to decompose.
     * @param extra_ctx   Optional additional context injected into the prompt.
     * @return Decomposition result.
     */
    [[nodiscard]] TaskDecompositionResult decompose(
        const std::string& task,
        const std::string& extra_ctx = "") const;

    // ── Conversion ───────────────────────────────────────────────────────────

    /**
     * @brief Convert a successful decomposition result into a WorkflowDefinition.
     *
     * Each subtask becomes a WorkflowStep with mode = WorkflowStepMode::Ask
     * (unless the subtask raw JSON contains a "mode" field).  Dependency
     * edges are preserved from SubTask::depends_on.
     *
     * @param result       A successful TaskDecompositionResult.
     * @param workflow_id  ID to assign to the generated workflow.
     * @return WorkflowDefinition ready for validation and execution.
     * @throws std::invalid_argument when result.success == false.
     */
    [[nodiscard]] static WorkflowDefinition toWorkflow(
        const TaskDecompositionResult& result,
        const std::string& workflow_id = "decomposed_workflow");

    // ── Prompt inspection ────────────────────────────────────────────────────

    /**
     * @brief Build and return the decomposition meta-prompt without calling the LLM.
     *
     * Useful for testing and audit.
     *
     * @param task       Task to decompose.
     * @param extra_ctx  Optional extra context.
     * @return The prompt string that would be sent to the LLM.
     */
    [[nodiscard]] std::string buildDecompositionPrompt(
        const std::string& task,
        const std::string& extra_ctx = "") const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // ── Internal helpers ─────────────────────────────────────────────────────

    [[nodiscard]] std::string buildChainOfThoughtPrompt(
        const std::string& task,
        const std::string& extra_ctx) const;

    [[nodiscard]] std::string buildDirectJsonPrompt(
        const std::string& task,
        const std::string& extra_ctx) const;

    [[nodiscard]] std::string buildFewShotPrompt(
        const std::string& task,
        const std::string& extra_ctx) const;

    [[nodiscard]] TaskDecompositionResult parseResponse(
        const std::string& raw_response,
        const std::string& prompt) const;

    [[nodiscard]] std::vector<SubTask> parseSubtasksFromJson(
        const json& arr) const;
};

} // namespace themis::llm
