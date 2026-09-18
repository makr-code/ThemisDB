/**
 * @file json_schema_converter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

struct ToolDefinition {
    std::string name;          ///< Function / tool name (must be non-empty)
    std::string description;   ///< Human-readable description (optional)
    json parameters;           ///< JSON Schema for the argument object
};

struct ToolCall {
    std::string name;    ///< Tool name matched from ToolDefinition::name
    json arguments;      ///< Parsed argument JSON object
};

class JsonSchemaConverter {
public:
    static constexpr size_t kMaxGrammarBytes = 65536;

    /**
     * @brief Schema To Ebnf.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    static std::string schemaToEbnf(const json& schema);

    /**
     * @brief Tools To Ebnf.
     * @param[in] tools Input parameter.
     * @return Return value.
     */
    static std::string toolsToEbnf(const std::vector<ToolDefinition>& tools);

    /**
     * @brief Parse Tool Call.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::optional<ToolCall> parseToolCall(const std::string& text);

private:
    // Internal helper: convert one schema node to a GBNF rule body.
    // rule_prefix is used to derive unique names for sub-rules.
    // new_rules accumulates named rules that are appended after the root.
    static std::string schemaNodeToRuleBody(
        const json& schema,
        const std::string& rule_prefix,
        std::vector<std::pair<std::string, std::string>>& new_rules);

    /**
     * @brief Escape a plain-text property name for embedding inside a GBNF literal.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string escapeGbnfString(const std::string& s);

    /**
     * @brief Sanitize an arbitrary string for use as a GBNF rule name identifier.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string sanitizeRuleName(const std::string& s);
};

} // namespace llm
} // namespace themis
