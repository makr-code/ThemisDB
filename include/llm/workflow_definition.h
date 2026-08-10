/**
 * @file workflow_definition.h
 * @brief YAML/JSON/BPMN-inspired workflow definition structures for LLM task orchestration.
 *
 * Provides a format-agnostic in-memory representation of multi-step LLM
 * workflows loaded from YAML, JSON, or a simplified BPMN-like XML document.
 *
 * ## Formats
 *
 * ### YAML / JSON (canonical)
 * @code{.yaml}
 * id: summarise_and_translate
 * description: Summarise a document and translate the summary
 * steps:
 *   - id: summarise
 *     prompt_template: "Summarise the following text in 3 sentences:\n{input}"
 *     mode: ask
 *   - id: translate
 *     prompt_template: "Translate the following text to German:\n{summarise.output}"
 *     mode: ask
 *     depends_on: [summarise]
 * @endcode
 *
 * ### BPMN-lite XML
 * A simplified subset that maps BPMN2 `<serviceTask>` elements to WorkflowStep
 * entries.  Sequence flows are translated to `depends_on` edges.
 *
 * ## Thread Safety
 *
 * WorkflowDefinition is immutable after construction; concurrent reads are safe.
 * WorkflowLoader methods are stateless and re-entrant.
 */

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>

namespace themis::llm {

using json = nlohmann::json;

// ============================================================================
// § 1  Core step and workflow types
// ============================================================================

/**
 * @brief Execution mode for a workflow step.
 */
enum class WorkflowStepMode {
    Ask,       ///< Plain LLM completion — no retrieval
    Rag,       ///< Retrieval-augmented generation
    Agentic,   ///< LLM with tool use
    Ethics,    ///< Constitutional reasoning pass
    Custom     ///< Caller-defined extension
};

/** @brief Convert string to WorkflowStepMode (case-insensitive). */
[[nodiscard]] WorkflowStepMode workflowStepModeFromString(const std::string& s);
/** @brief Convert WorkflowStepMode to canonical lowercase string. */
[[nodiscard]] std::string workflowStepModeToString(WorkflowStepMode m);

/**
 * @brief A single step inside a workflow.
 *
 * Steps are the atomic units of execution.  The @p prompt_template field
 * supports `{variable}` substitution where variables are either workflow
 * inputs (e.g. `{input}`) or outputs of upstream steps referenced by
 * `{step_id.output}`.
 */
struct WorkflowStep {
    /// Unique identifier within the workflow (required).
    std::string id;

    /// Human-readable description (optional).
    std::string description;

    /**
     * @brief Prompt template for this step.
     *
     * Supports `{variable}` substitution:
     *   - `{input}` — the top-level workflow input string
     *   - `{<step_id>.output}` — the text output of an upstream step
     *   - Any custom key from the initial context map
     */
    std::string prompt_template;

    /// Execution mode (default: Ask).
    WorkflowStepMode mode = WorkflowStepMode::Ask;

    /**
     * @brief IDs of steps that must complete before this step runs.
     *
     * Cycles are detected by WorkflowLoader::validate() and reported as
     * ValidationError.
     */
    std::vector<std::string> depends_on;

    /// Maximum tokens for the LLM completion (0 = use model default).
    int max_tokens = 0;

    /// Temperature override (negative = use mode default).
    float temperature = -1.0f;

    /**
     * @brief Optional JSON Schema for structured output validation.
     *
     * When set the workflow executor validates the step's output against this
     * schema.  Non-conforming output causes the step to fail.
     */
    std::optional<json> output_schema;

    /// Arbitrary extension metadata preserved from the source document.
    json extensions;
};

/**
 * @brief Complete workflow definition with steps and metadata.
 *
 * A workflow consists of an ordered list of steps.  Execution follows the
 * dependency graph (topological order); independent steps may run in parallel
 * when the executor supports it.
 */
struct WorkflowDefinition {
    /// Unique workflow identifier.
    std::string id;

    /// Human-readable name.
    std::string name;

    /// Optional description.
    std::string description;

    /// Ordered list of workflow steps (execution follows dependency graph).
    std::vector<WorkflowStep> steps;

    /**
     * @brief Global default mode applied to steps that do not specify one.
     */
    WorkflowStepMode default_mode = WorkflowStepMode::Ask;

    /**
     * @brief Initial context key–value pairs injected into template substitution.
     *
     * Keys are available in prompt templates as `{key}`.  The special key
     * `"input"` receives the top-level query string at runtime.
     */
    std::unordered_map<std::string, std::string> initial_context;

    /// Format hint recorded during load ("yaml", "json", "bpmn").
    std::string source_format;

    /// Arbitrary extension metadata.
    json extensions;

    // ── Convenience helpers ─────────────────────────────────────────────────

    /**
     * @brief Return a step by ID; throws std::out_of_range if not found.
     * @param step_id  Step identifier.
     */
    [[nodiscard]] const WorkflowStep& stepById(const std::string& step_id) const;

    /**
     * @brief Return steps in topological order respecting depends_on edges.
     *
     * Steps with no dependencies are first; dependents follow.  Returns an
     * empty vector if the definition contains a cycle (use validate() first).
     */
    [[nodiscard]] std::vector<const WorkflowStep*> topologicalOrder() const;
};

// ============================================================================
// § 2  Validation
// ============================================================================

/**
 * @brief Describes a single validation finding.
 */
struct WorkflowValidationError {
    /// Step id that caused the error, or empty for workflow-level errors.
    std::string step_id;
    /// Human-readable error message.
    std::string message;
};

/**
 * @brief Result of WorkflowLoader::validate().
 */
struct WorkflowValidationResult {
    bool valid = true;
    std::vector<WorkflowValidationError> errors;

    /** @brief Helper: append an error and mark as invalid. */
    void addError(const std::string& step_id, const std::string& message) {
        errors.push_back({step_id, message});
        valid = false;
    }
};

// ============================================================================
// § 3  WorkflowLoader — format-agnostic loader
// ============================================================================

/**
 * @brief Loads WorkflowDefinition from YAML, JSON, or BPMN-lite XML strings/files.
 *
 * All load methods are stateless.  They parse the given content, apply
 * minimal normalisation (e.g. default modes), and return the definition.
 * Call validate() on the result before execution to catch structural errors
 * such as unknown depends_on references or dependency cycles.
 *
 * ### Format detection
 *
 * - loadFromFile() detects the format by file extension (.yaml/.yml, .json,
 *   .bpmn/.xml).
 * - loadFromString() requires an explicit format hint.
 *
 * ### BPMN-lite mapping
 *
 * | BPMN element          | WorkflowStep field            |
 * |------------------------|-------------------------------|
 * | `serviceTask/@id`      | `id`                          |
 * | `serviceTask/@name`    | `description`                 |
 * | `extensionElements/`   | `prompt_template`, `mode`     |
 * | `sequenceFlow`         | `depends_on`                  |
 *
 * Only `<serviceTask>` and `<sequenceFlow>` elements are interpreted;
 * gateways, events, and data objects are currently ignored.
 */
class WorkflowLoader {
public:
    /**
     * @brief Load a WorkflowDefinition from a file.
     *
     * @param path  Filesystem path; extension determines format.
     * @return Parsed workflow definition.
     * @throws std::runtime_error on I/O or parse errors.
     */
    [[nodiscard]] static WorkflowDefinition loadFromFile(const std::string& path);

    /**
     * @brief Load a WorkflowDefinition from an in-memory string.
     *
     * @param content       Raw content string.
     * @param format_hint   One of "yaml", "json", "bpmn".
     * @param source_label  Optional identifier for error messages.
     * @return Parsed workflow definition.
     * @throws std::runtime_error on parse errors.
     */
    [[nodiscard]] static WorkflowDefinition loadFromString(
        const std::string& content,
        const std::string& format_hint,
        const std::string& source_label = "<string>");

    /**
     * @brief Validate a parsed WorkflowDefinition.
     *
     * Checks:
     *   - All depends_on references resolve to defined step IDs.
     *   - No dependency cycles exist.
     *   - All step IDs are non-empty and unique.
     *   - prompt_template is non-empty for every step.
     *
     * @param def  Workflow definition to validate.
     * @return Validation result with error list.
     */
    [[nodiscard]] static WorkflowValidationResult validate(const WorkflowDefinition& def);

    /**
     * @brief Serialize a WorkflowDefinition to JSON.
     * @param def  Definition to serialize.
     * @return JSON object.
     */
    [[nodiscard]] static json toJson(const WorkflowDefinition& def);

    /**
     * @brief Deserialize a WorkflowDefinition from JSON.
     * @param j  JSON object (as produced by toJson()).
     * @return Parsed workflow definition with source_format = "json".
     * @throws std::runtime_error on schema errors.
     */
    [[nodiscard]] static WorkflowDefinition fromJson(const json& j);

private:
    [[nodiscard]] static WorkflowDefinition parseYaml(const std::string& content,
                                                       const std::string& label);
    [[nodiscard]] static WorkflowDefinition parseJson(const std::string& content,
                                                       const std::string& label);
    [[nodiscard]] static WorkflowDefinition parseBpmn(const std::string& content,
                                                       const std::string& label);
};

} // namespace themis::llm
