/**
 * @file process_model_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – Process Modeling Module
 *
 * File:    process_model_generator.h
 * Module:  include/process/
 * Purpose: LLM-to-BPMN generator — converts natural language process
 *          descriptions into ProcessModelRecord objects via iterative
 *          LLM calls with BPMN semantic validation.
 *
 * Scientific basis: Busch, K. et al. (2023). *ProcessGPT: Transforming
 * Business Process Management with Generative AI.* IEEE Big Data 2023.
 */

#pragma once

#include "process/process_model_manager.h"
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// ProcessModelGenerator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Generate @c ProcessModelRecord instances from natural language
 *        descriptions via an LLM backend.
 *
 * The generation pipeline (based on ProcessGPT, Busch 2023):
 * 1. Build a structured prompt instructing the LLM to produce a JSON
 *    process model description.
 * 2. Parse the LLM response and convert to a @c ProcessModelRecord.
 * 3. Validate BPMN semantics: ≥1 start event, ≥1 end event, no isolated
 *    nodes, all gateways have ≥1 outgoing edge.
 * 4. On validation failure, re-prompt with error messages (up to
 *    @c Config::max_retries times).
 *
 * The LLM backend is injected as a callable to avoid a hard dependency on
 * any particular HTTP client or LLM library.
 *
 * @code
 * // Example usage:
 * ProcessModelGenerator gen;
 * gen.setLlmBackend([](const std::string& prompt) -> std::string {
 *     // call your LLM endpoint here
 *     return llm_response_json;
 * });
 * auto [ok, model] = gen.generateFromDescription("Bauantragsverfahren ...");
 * @endcode
 */
class ProcessModelGenerator {
public:
    // ── Types ──────────────────────────────────────────────────────────────

    /**
     * @brief LLM backend callable: receives a prompt string, returns the
     *        LLM's text response.
     */
    using LlmBackend = std::function<std::string(const std::string& prompt)>;

    /**
     * @brief Configuration for the generation pipeline.
     */
    struct Config {
        int         max_retries{3};           ///< Max generate-validate-fix cycles
        std::string language{"de"};           ///< Prompt language: "de" or "en"
        ProcessDomain domain{ProcessDomain::BUSINESS}; ///< Default domain for generated models
        std::string llm_model;                ///< Model name hint (passed to backend)
        size_t      max_prompt_tokens{2000};  ///< Approximate token budget for generation
    };

    /**
     * @brief Validation error category.
     */
    enum class ValidationError {
        NO_START_EVENT,      ///< No start event node found
        NO_END_EVENT,        ///< No end event node found
        ISOLATED_NODE,       ///< Node with no incoming or outgoing edges
        GATEWAY_NO_OUTGOING, ///< Gateway node with no outgoing edges
        INVALID_JSON,        ///< LLM response is not valid JSON
    };

    /**
     * @brief Result of BPMN semantic validation.
     */
    struct ValidationResult {
        bool ok{true};
        std::vector<std::string> errors;
    };

    // ── Construction ───────────────────────────────────────────────────────

    ProcessModelGenerator() = default;

    /**
     * @brief Register the LLM backend callable.
     *
     * Must be called before @c generateFromDescription() or @c refine().
     * If not set, both methods return @c {false, {}}.
     */
    void setLlmBackend(LlmBackend backend);

    // ── Public API ─────────────────────────────────────────────────────────

    /**
     * @brief Generate a @c ProcessModelRecord from a free-text description.
     *
     * Calls the LLM backend up to @c Config::max_retries times, validating
     * the result after each attempt and feeding errors back to the LLM.
     *
     * @param description  Natural language process description (DE or EN).
     * @param cfg          Generation configuration.
     * @return             {true, record} on success; {false, {}} on failure.
     */
    [[nodiscard]] std::pair<bool, ProcessModelRecord> generateFromDescription(
        std::string_view description,
        const Config&    cfg = {}
    ) const;

    /**
     * @brief Refine an existing @c ProcessModelRecord based on textual
     *        feedback (e.g. from a user review).
     *
     * Sends the current model definition together with the feedback to the
     * LLM and returns the updated model after validation.
     *
     * @param existing  Current model to refine.
     * @param feedback  Natural language correction instructions.
     * @param cfg       Generation configuration.
     * @return          {true, refined_record} on success; {false, existing}
     *                  on failure (original model unchanged).
     */
    [[nodiscard]] std::pair<bool, ProcessModelRecord> refine(
        const ProcessModelRecord& existing,
        std::string_view          feedback,
        const Config&             cfg = {}
    ) const;

    // ── Validation helpers (also usable standalone) ────────────────────────

    /**
     * @brief Validate BPMN semantic constraints on a normalised process graph.
     *
     * Rules checked:
     * - At least one @c startEvent node.
     * - At least one @c endEvent node.
     * - No isolated nodes (every node must have ≥1 edge).
     * - Every gateway node has ≥1 outgoing edge.
     *
     * @param normalized_graph  JSON object with @c "nodes" and @c "edges".
     * @return                  ValidationResult with error list.
     */
    [[nodiscard]] static ValidationResult validate(
        const nlohmann::json& normalized_graph
    );

    /**
     * @brief Convert an LLM JSON response to a @c ProcessModelRecord.
     *
     * Expected LLM JSON schema:
     * @code{.json}
     * {
     *   "id":   "proc_id",
     *   "name": "Prozessname",
     *   "domain": "ADMINISTRATION",
     *   "activities": [{"id":"a1","name":"Schritt 1","type":"userTask","sla_hours":24}],
     *   "gateways":   [{"id":"g1","name":"Entscheidung","type":"exclusiveGateway"}],
     *   "events":     [{"id":"s1","type":"startEvent"},{"id":"e1","type":"endEvent"}],
     *   "edges":      [{"from":"s1","to":"a1","type":"sequenceFlow"}]
     * }
     * @endcode
     *
     * @param llm_json  Parsed LLM JSON response.
     * @param domain    Default domain if not present in JSON.
     * @return          Populated @c ProcessModelRecord.
     */
    [[nodiscard]] static ProcessModelRecord fromLlmJson(
        const nlohmann::json& llm_json,
        ProcessDomain         domain = ProcessDomain::BUSINESS
    );

private:
    LlmBackend llm_backend_;

    /// Build the initial generation prompt.
    [[nodiscard]] std::string buildGenerationPrompt_(
        std::string_view description,
        const Config&    cfg
    ) const;

    /// Build a fix/correction prompt given the current model JSON and errors.
    [[nodiscard]] static std::string buildFixPrompt_(
        const nlohmann::json&        current_json,
        const std::vector<std::string>& errors,
        std::string_view             language
    );

    /// Parse the LLM text response, extracting the JSON block.
    [[nodiscard]] static nlohmann::json extractJson_(const std::string& llm_text);
};

} // namespace process
} // namespace themis
