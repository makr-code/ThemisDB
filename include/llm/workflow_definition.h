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

enum class WorkflowStepMode {
    Ask,       ///< Plain LLM completion — no retrieval
    Rag,       ///< Retrieval-augmented generation
    Agentic,   ///< LLM with tool use
    Ethics,    ///< Constitutional reasoning pass
    Custom     ///< Caller-defined extension
};

[[nodiscard]] WorkflowStepMode workflowStepModeFromString(const std::string& s);
[[nodiscard]] std::string workflowStepModeToString(WorkflowStepMode m);

struct WorkflowStep {
    std::string id;

    std::string description;

    std::string prompt_template;

    WorkflowStepMode mode = WorkflowStepMode::Ask;

    std::vector<std::string> depends_on;

    int max_tokens = 0;

    float temperature = -1.0f;

    std::optional<json> output_schema;

    json extensions;
};

struct WorkflowDefinition {
    std::string id;

    std::string name;

    std::string description;

    std::vector<WorkflowStep> steps;

    WorkflowStepMode default_mode = WorkflowStepMode::Ask;

    std::unordered_map<std::string, std::string> initial_context;

    std::string source_format;

    json extensions;

    // ── Convenience helpers ─────────────────────────────────────────────────

    [[nodiscard]] const WorkflowStep& stepById(const std::string& step_id) const;

    [[nodiscard]] std::vector<const WorkflowStep*> topologicalOrder() const;
};

// ============================================================================
// § 2  Validation
// ============================================================================

struct WorkflowValidationError {
    std::string step_id;
    std::string message;
};

struct WorkflowValidationResult {
    bool valid = true;
    std::vector<WorkflowValidationError> errors;

    /**
     * @brief Add Error.
     * @param[in] step_id Identifier of the step.
     * @param[in] message Input parameter.
     * @details Calls: push_back().
     */
    void addError(const std::string& step_id, const std::string& message) {
        errors.push_back({step_id, message});
        valid = false;
    }
};

// ============================================================================
// § 3  WorkflowLoader — format-agnostic loader
// ============================================================================

class WorkflowLoader {
public:
    [[nodiscard]] static WorkflowDefinition loadFromFile(const std::string& path);

    [[nodiscard]] static WorkflowDefinition loadFromString(
        const std::string& content,
        const std::string& format_hint,
        const std::string& source_label = "<string>");

    [[nodiscard]] static WorkflowValidationResult validate(const WorkflowDefinition& def);

    [[nodiscard]] static json toJson(const WorkflowDefinition& def);

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
