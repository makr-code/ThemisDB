/**
 * @file json_schema_converter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Definition of a callable tool / function for LLM tool calling.
 *
 * Maps to the OpenAI-style "tools" array element:
 * @code
 * {
 *   "name": "get_weather",
 *   "description": "Get weather for a location",
 *   "parameters": {
 *     "type": "object",
 *     "properties": { "location": {"type": "string"} },
 *     "required": ["location"]
 *   }
 * }
 * @endcode
 */
struct ToolDefinition {
    std::string name;          ///< Function / tool name (must be non-empty)
    std::string description;   ///< Human-readable description (optional)
    json parameters;           ///< JSON Schema for the argument object
};

/**
 * @brief A tool call produced by the model.
 *
 * Parsed from the model's JSON output when tool calling is active.
 * Format: {"name": "<tool>", "arguments": {<JSON object>}}
 */
struct ToolCall {
    std::string name;    ///< Tool name matched from ToolDefinition::name
    json arguments;      ///< Parsed argument JSON object
};

/**
 * @brief Converts JSON Schema definitions to GBNF grammars for grammar-constrained generation.
 *
 * Supports a JSON Schema draft-07 subset:
 * - type: object, array, string, number, integer, boolean, null
 * - properties, required, enum, items
 *
 * The generated GBNF grammar is compatible with llama.cpp's grammar API and
 * can be passed directly as InferenceRequest::grammar_ebnf.
 *
 * Grammar size is bounded by the input schema complexity. Adversarial inputs
 * that would produce a grammar exceeding 64 KB are rejected and an empty
 * string is returned.
 *
 * Usage:
 * @code
 * // Schema-constrained generation
 * InferenceRequest req;
 * req.prompt = "Generate a user profile";
 * req.json_schema = json::parse(R"({"type":"object","properties":{"name":{"type":"string"},"age":{"type":"integer"}},"required":["name","age"]})");
 *
 * // Tool calling
 * ToolDefinition tool;
 * tool.name = "get_weather";
 * tool.parameters = json::parse(R"({"type":"object","properties":{"location":{"type":"string"}},"required":["location"]})");
 * req.tools = {tool};
 * @endcode
 */
class JsonSchemaConverter {
public:
    /// Maximum allowed GBNF output size in bytes (64 KB).
    static constexpr size_t kMaxGrammarBytes = 65536;

    /**
     * @brief Convert a JSON Schema object to a GBNF grammar string.
     *
     * The generated grammar's root rule is named "root" and constrains the
     * LLM to produce valid JSON matching the schema.
     *
     * @param schema  JSON Schema object
     * @return GBNF grammar string, or empty string if schema is unsupported
     *         or the generated grammar would exceed kMaxGrammarBytes.
     */
    static std::string schemaToEbnf(const json& schema);

    /**
     * @brief Generate a tool call grammar for function / tool calling.
     *
     * The generated grammar constrains the model to output a JSON object
     * of the form:
     * @code
     * {"name": "<tool_name>", "arguments": {<args>}}
     * @endcode
     * where the arguments match the parameter schema of the selected tool.
     *
     * @param tools  Non-empty list of tool definitions.
     * @return GBNF grammar string, or empty string on error.
     */
    static std::string toolsToEbnf(const std::vector<ToolDefinition>& tools);

    /**
     * @brief Parse a tool call JSON object from model output text.
     *
     * Expects the model output to be a JSON object with "name" (string) and
     * "arguments" (object) keys. Whitespace surrounding the JSON is tolerated.
     *
     * @param text  Model-generated text.
     * @return Parsed ToolCall, or nullopt if the text is not a valid tool call.
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

    // Escape a plain-text property name for embedding inside a GBNF literal.
    static std::string escapeGbnfString(const std::string& s);

    // Sanitize an arbitrary string for use as a GBNF rule name identifier.
    static std::string sanitizeRuleName(const std::string& s);
};

} // namespace llm
} // namespace themis
