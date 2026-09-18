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

enum class DecompositionStrategy {
    ChainOfThought,

    DirectJson,

    FewShot,
};

[[nodiscard]] std::string decompositionStrategyToString(DecompositionStrategy s);

struct DecompositionExample {
    std::string task;        ///< Example complex task string
    std::vector<json> steps; ///< Expected JSON subtask array for that task
};

struct TaskDecomposerConfig {
    DecompositionStrategy strategy = DecompositionStrategy::ChainOfThought;

    int max_subtasks = 8;

    int min_subtasks = 1;

    int max_tokens = 1024;

    float temperature = 0.2f;

    std::string domain_context;

    std::string output_language;

    std::vector<DecompositionExample> few_shot_examples;

    int max_retries = 1;
};

// ============================================================================
// § 2  Result types
// ============================================================================

struct SubTask {
    std::string id = {};

    std::string description;

    std::string prompt;

    std::vector<std::string> depends_on;

    json raw;
};

struct TaskDecompositionResult {
    bool success = false;

    std::vector<SubTask> subtasks;

    std::string decomposition_prompt;

    std::string raw_llm_response;

    std::string error;

    int llm_calls_made = 0;

    size_t tokens_consumed = 0;
};

// ============================================================================
// § 3  TaskDecomposer
// ============================================================================

class TaskDecomposer {
public:
    explicit TaskDecomposer(const TaskDecomposerConfig& config = TaskDecomposerConfig{});

    ~TaskDecomposer();

    // No copy — owns the LLM plugin reference.
    TaskDecomposer(const TaskDecomposer&)            = delete;
    TaskDecomposer& operator=(const TaskDecomposer&) = delete;

    // Move is supported.
    TaskDecomposer(TaskDecomposer&&)            noexcept;
    TaskDecomposer& operator=(TaskDecomposer&&) noexcept;

    /**
     * @brief ── Configuration ────────────────────────────────────────────────────────
     * @param[in] plugin Input parameter.
     */

    void setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin);

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const TaskDecomposerConfig& config);

    [[nodiscard]] TaskDecomposerConfig config() const;

    // ── Core operation ───────────────────────────────────────────────────────

    [[nodiscard]] TaskDecompositionResult decompose(
        const std::string& task,
        const std::string& extra_ctx = "") const;

    // ── Conversion ───────────────────────────────────────────────────────────

    [[nodiscard]] static WorkflowDefinition toWorkflow(
        const TaskDecompositionResult& result,
        const std::string& workflow_id = "decomposed_workflow");

    // ── Prompt inspection ────────────────────────────────────────────────────

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
