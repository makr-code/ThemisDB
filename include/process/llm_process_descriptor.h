/**
 * @file llm_process_descriptor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "process/process_model_manager.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief Generates LLM-optimised, machine-readable descriptors for process models.
 *
 * ## Purpose
 *
 * Large Language Models need structured, concise context to reason about
 * business processes.  This class converts a ThemisDB `ProcessModelRecord`
 * into a JSON descriptor that:
 *
 * - Describes every process node with its type, role, SLA, and purpose.
 * - Lists all transitions with their conditions in plain language.
 * - Includes compliance tags, domain context, and regulatory notes.
 * - Provides a natural-language summary suitable for system prompts.
 * - Contains machine-readable fields for conformance checking queries.
 *
 * ## Output schema (abbreviated)
 *
 * ```json
 * {
 *   "process_id": "bauantrag_standard",
 *   "name": "Bauantrag (Standard)",
 *   "domain": "ADMINISTRATION",
 *   "notation": "BPMN_2_0",
 *   "state": "ACTIVE",
 *   "summary": "…one-paragraph natural language description…",
 *   "compliance": ["§34 BauO", "DSGVO", "VwVfG"],
 *   "sla_total_hours": 2256,
 *   "nodes": [
 *     {
 *       "id": "antragstellung",
 *       "type": "START_EVENT",
 *       "name": "Antragstellung",
 *       "role": null,
 *       "sla_hours": 0,
 *       "description": "Eingang des Bauantrags beim Bauordnungsamt."
 *     }
 *   ],
 *   "edges": [
 *     {
 *       "from": "antragstellung",
 *       "to": "vollstaendigkeitspruefung",
 *       "type": "SEQUENCE_FLOW",
 *       "condition": null
 *     }
 *   ],
 *   "llm_context": "…condensed version of the above for system prompt injection…"
 * }
 * ```
 *
 * ## Usage in RAG pipelines
 *
 * ```cpp
 * // Retrieve model and build descriptor
 * auto record = manager.load("bauantrag_standard").value();
 * auto desc   = LlmProcessDescriptor::generate(record);
 *
 * // Inject into LLM system prompt
 * std::string prompt = "Process context:\n" + desc["llm_context"].get<std::string>();
 * ```
 */
class LlmProcessDescriptor {
public:
    /**
     * @brief Configuration for descriptor generation.
     */
    struct Config {
        bool include_raw_payload{false};  ///< Include the original BPMN/EPK/YAML
        bool include_embedding{false};    ///< Include the float embedding vector
        size_t max_description_chars{500}; ///< Truncate long descriptions
        std::string language{"de"};        ///< Primary output language ("de" or "en")
    };

    /**
     * @brief Generate a full LLM descriptor for a process model record.
     *
     * @param record  The process model record (from ProcessModelManager::load).
     * @return JSON descriptor object ready for LLM consumption.
     */
    static nlohmann::json generate(
      const ProcessModelRecord& record
    );

    /**
     * @brief Generate a full LLM descriptor for a process model record.
     *
     * @param record  The process model record (from ProcessModelManager::load).
     * @param cfg     Optional generation configuration.
     * @return JSON descriptor object ready for LLM consumption.
     */
    static nlohmann::json generate(
      const ProcessModelRecord& record,
      const Config&             cfg
    );

    /**
     * @brief Build a compact system-prompt string from a descriptor.
     *
     * Produces a condensed text block (< 2000 tokens) that can be prepended to
     * any LLM prompt to give the model full context about a process.
     *
     * @param descriptor  Output of generate().
     * @return Plain-text context block.
     */
    static std::string buildSystemPrompt(const nlohmann::json& descriptor);

    /**
     * @brief Summarise multiple process models for a list/comparison prompt.
     *
     * Generates a compact summary of all provided records, sorted by domain.
     *
     * @param records  Collection of process model records.
     * @param language ISO 639-1 language code ("de" or "en").
     * @return JSON array of compact summary objects.
     */
    static nlohmann::json summarizeList(
        const std::vector<ProcessModelRecord>& records,
        std::string_view                       language = "de"
    );

    /**
     * @brief Build a conformance-checking prompt for an LLM.
     *
     * Combines a process model descriptor with an observed execution trace and
     * asks the LLM to identify deviations, compliance violations, and SLA
     * breaches.
     *
     * @param descriptor     Output of generate().
     * @param observed_trace JSON array of executed activity IDs in order.
     * @return Prompt string ready to send to an LLM.
     */
    static std::string buildConformancePrompt(
        const nlohmann::json& descriptor,
        const nlohmann::json& observed_trace
    );

private:
    static std::string truncate_(std::string_view s, size_t max_chars);
    static nlohmann::json nodeToJson_(
        const nlohmann::json& node_doc,
        const Config&         cfg
    );
    static nlohmann::json edgeToJson_(const nlohmann::json& edge_doc);
};

} // namespace process
} // namespace themis
