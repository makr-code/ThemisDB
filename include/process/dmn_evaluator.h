/**
 * @file dmn_evaluator.h
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
 * File:    dmn_evaluator.h
 * Module:  include/process/
 * Purpose: DMN 1.5 decision table evaluator.
 *
 * Supports:
 * - Decision tables in JSON or XML format
 * - FEEL subset: numeric comparisons, string equality, range [a..b],
 *   boolean and/or, null checks
 * - Hit policies: UNIQUE, FIRST, COLLECT
 * - Integration with ProcessGraphRag::checkCompliance() via node
 *   metadata["dmn_ref"] references
 *
 * Scientific basis: OMG (2023). *Decision Model and Notation (DMN) 1.5.*
 * Object Management Group Specification.
 */

#pragma once

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// DmnRule
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single rule row in a decision table.
 *
 * Each @c input_expressions element corresponds to one input column;
 * @c output_values is a JSON object mapping output column names to values.
 */
struct DmnRule {
    std::string                id;                   ///< Optional rule ID
    std::string                description;          ///< Optional annotation
    std::vector<std::string>   input_expressions;    ///< FEEL expressions per input
    nlohmann::json             output_values;        ///< Output column → value
};

// ─────────────────────────────────────────────────────────────────────────────
// DecisionTable
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Represents a single DMN 1.5 decision table.
 */
struct DecisionTable {
    std::string              id;             ///< Decision table identifier
    std::string              name;           ///< Human-readable name
    std::vector<std::string> input_columns;  ///< Ordered input column names
    std::vector<std::string> output_columns; ///< Ordered output column names
    std::vector<DmnRule>     rules;          ///< Ordered rule rows
    std::string              hit_policy;     ///< "UNIQUE" | "FIRST" | "COLLECT"
};

// ─────────────────────────────────────────────────────────────────────────────
// DmnEvaluator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DMN 1.5 decision table evaluator.
 *
 * Supports loading decision tables from JSON (native format) or from a
 * simplified DMN XML subset.  Evaluates decision tables against an input
 * context using the FEEL subset supported by this implementation.
 *
 * FEEL subset supported:
 * - Number comparisons: @c >5, @c >=5, @c <5, @c <=5, @c =5, @c !=5
 * - String equality:    @c "value"
 * - Range:              @c [a..b], @c (a..b], @c [a..b), @c (a..b)
 * - Any / wildcard:     @c - (matches anything)
 * - Null check:         @c null, @c not(null)
 * - Boolean:            @c true, @c false
 *
 * Usage:
 * @code
 * DmnEvaluator eval;
 * eval.loadFromJson(dmn_table_json);
 * auto result = eval.evaluate("risk_assessment", {{"amount", 5000}, {"type", "credit"}});
 * // result["risk_level"] == "HIGH"
 * @endcode
 */
class DmnEvaluator {
public:
    DmnEvaluator() = default;

    // ── Loading ────────────────────────────────────────────────────────────

    /**
     * @brief Load a decision table from a JSON object.
     *
     * Expected JSON schema:
     * @code{.json}
     * {
     *   "id":             "risk_assessment",
     *   "name":           "Risk Assessment",
     *   "hit_policy":     "UNIQUE",
     *   "input_columns":  ["amount", "type"],
     *   "output_columns": ["risk_level", "action"],
     *   "rules": [
     *     { "id": "r1",
     *       "inputs":  [">1000", "\"credit\""],
     *       "outputs": {"risk_level": "HIGH", "action": "manual_review"} },
     *     { "id": "r2",
     *       "inputs":  ["[100..1000]", "-"],
     *       "outputs": {"risk_level": "MEDIUM", "action": "auto_approve"} }
     *   ]
     * }
     * @endcode
     *
     * @param dmn_json  JSON object with the schema above.
     * @return          @c true on success; @c false on schema error.
     */
    bool loadFromJson(const nlohmann::json& dmn_json);

    /**
     * @brief Load a decision table from a simplified DMN 1.5 XML string.
     *
     * Parses the @c \<decision\> / @c \<decisionTable\> elements using the
     * same state-machine tokenizer as @c BpmnSerializer.
     *
     * @param dmn_xml  DMN 1.5 XML string.
     * @return         @c true on success; @c false on parse error.
     */
    bool loadFromXml(std::string_view dmn_xml);

    // ── Evaluation ─────────────────────────────────────────────────────────

    /**
     * @brief Evaluate a named decision table against an input context.
     *
     * @param decision_id    ID of the decision table to evaluate.
     * @param input_context  JSON object mapping input column names to values.
     * @return               JSON object with output column values on match;
     *                       JSON array of objects for COLLECT hit policy;
     *                       empty object @c {} when no rule matches.
     */
    [[nodiscard]] nlohmann::json evaluate(
        std::string_view         decision_id,
        const nlohmann::json&    input_context
    ) const;

    /**
     * @brief Evaluate a single FEEL expression against a JSON value.
     *
     * @param feel_expr  FEEL expression string (see class doc for supported subset).
     * @param value      The value to test the expression against.
     * @return           @c true if the expression matches the value.
     */
    [[nodiscard]] static bool evaluateFeel(
        std::string_view      feel_expr,
        const nlohmann::json& value
    );

    // ── Inspection ────────────────────────────────────────────────────────

    /**
     * @brief Return all loaded decision table IDs.
     */
    [[nodiscard]] std::vector<std::string> listDecisions() const;

    /**
     * @brief Return the loaded decision table with the given ID, or
     *        @c std::nullopt if not found.
     */
    [[nodiscard]] std::optional<DecisionTable> getDecision(
        std::string_view decision_id
    ) const;

private:
    mutable std::mutex tables_mutex_;  ///< Protects tables_ for thread-safe concurrent access
    std::map<std::string, DecisionTable> tables_;

    /// Evaluate a rule row against the input context.
    [[nodiscard]] static bool matchRule_(
        const DmnRule&              rule,
        const std::vector<std::string>& input_columns,
        const nlohmann::json&       input_context
    );
};

} // namespace process
} // namespace themis
