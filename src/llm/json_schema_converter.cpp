/**
 * @file json_schema_converter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/json_schema_converter.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace llm {

// ---------------------------------------------------------------------------
// Base GBNF rules shared by all generated grammars
// ---------------------------------------------------------------------------

static const char* kBaseRules = R"(ws ::= [ \t\n\r]*
string ::= "\"" ([^"\\] | "\\" (["\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))* "\""
integer ::= ("-")? ("0" | [1-9] [0-9]*)
number ::= ("-")? ("0" | [1-9] [0-9]*) ("." [0-9]+)? ([eE] [+-]? [0-9]+)?
boolean ::= "true" | "false"
null ::= "null"
value ::= object | array | string | number | boolean | null
object ::= "{" ws "}" | "{" ws string ws ":" ws value (ws "," ws string ws ":" ws value)* ws "}"
array ::= "[" ws "]" | "[" ws value (ws "," ws value)* ws "]"
)";

// Rule names that map directly to a base rule and require no new rule definition.
static const std::unordered_set<std::string> kPrimitiveRuleNames =
    {"string", "integer", "number", "boolean", "null", "value", "object", "array"};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string JsonSchemaConverter::escapeGbnfString(const std::string& s) {
    std::string result = {};
    result.reserve(static_cast<int>(s.size()) + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += static_cast<char>(c); break;
        }
    }
    return result;
}

std::string JsonSchemaConverter::sanitizeRuleName(const std::string& s) {
    std::string result = {};
    result.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '_' || c == '-') {
            result += static_cast<char>(c);
        } else {
            result += '_';
        }
    }
    return result.empty() ? "rule" : result;
}

// ---------------------------------------------------------------------------
// Core recursive schema → GBNF rule body converter
// ---------------------------------------------------------------------------

std::string JsonSchemaConverter::schemaNodeToRuleBody(
    const json& schema,
    const std::string& rule_prefix,
    std::vector<std::pair<std::string, std::string>>& new_rules)
{
    if (!schema.is_object()) {
        // Bare schema (e.g. true/false) → fall back to generic value
        return "value";
    }

    // Handle "enum" (takes precedence over "type")
    if (schema.contains("enum") && schema["enum"].is_array() && !schema["enum"].empty()) {
        std::string body = {};
        bool first = true;
        for (const auto& elem : schema["enum"]) {
            if (!first) {
              body += " | ";
            }
            first = false;
            if (elem.is_string()) {
                body += "\"\\\"" + escapeGbnfString(elem.get<std::string>()) + "\\\"\"";
            } else if (elem.is_boolean()) {
                body += elem.get<bool>() ? "\"true\"" : "\"false\"";
            } else if (elem.is_null()) {
                body += "\"null\"";
            } else if (elem.is_number()) {
                // Serialize numbers via nlohmann dump to get correct representation
                body += "\"" + elem.dump() + "\"";
            } else {
                body += "value";
            }
        }
        return body;
    }

    // Determine type(s)
    std::string type_str = {};
    if (schema.contains("type")) {
        if (schema["type"].is_string()) {
            type_str = schema["type"].get<std::string>();
        }
        // Note: array-of-types is not handled (uncommon in tool schemas)
    }

    if (type_str == "string") {
      return "string";
    }
    if (type_str == "integer") {
      return "integer";
    }
    if (type_str == "number") {
      return "number";
    }
    if (type_str == "boolean") {
      return "boolean";
    }
    if (type_str == "null") {
      return "null";
    }

    if (type_str == "array") {
        if (schema.contains("items") && schema["items"].is_object()) {
            // Create a named rule for the item type
            std::string item_rule_name = rule_prefix + "-item";
            std::string item_body = schemaNodeToRuleBody(schema["items"], item_rule_name, new_rules);
            // If item_body is a primitive reference, no new rule needed
            std::string item_ref = {};
            if (kPrimitiveRuleNames.count(item_body)) {
                item_ref = item_body;
            } else {
                new_rules.emplace_back(item_rule_name, item_body);
                item_ref = item_rule_name;
            }
            return "\"[\" ws \"]\" | \"[\" ws " + item_ref +
                   " (ws \",\" ws " + item_ref + ")* ws \"]\"";
        }
        return "array";
    }

    if ((type_str == "object" || (type_str.empty() && schema.contains("properties"))) {
        if (!schema.contains("properties") || !schema["properties"].is_object()
            || schema["properties"].empty()) {
            return "object";
        }

        // Collect required set
        std::unordered_set<std::string> required_set = {};

        if (schema.contains("required") && schema["required"].is_array()) {
            for (const auto& r : schema["required"]) {
                if (r.is_string()) {
                  required_set.insert(r.get<std::string>());
                }
            }
        }

        // Build ordered list: required fields first (in "required" order),
        // then remaining optional fields (in "properties" insertion order).
        std::vector<std::string> ordered_props = {};

        if (schema.contains("required") && schema["required"].is_array()) {
            for (const auto& r : schema["required"]) {
                if (r.is_string() && schema["properties"].contains(r.get<std::string>())) {
                    ordered_props.push_back(r.get<std::string>());
                }
            }
        }
        for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
            if (!required_set.count(it.key())) {
                ordered_props.push_back(it.key());
            }
        }

        // Build pairs for all properties
        // We include all properties (required + optional) in the grammar.
        // Optional fields are wrapped in an outer "(... )?".
        // Pattern: "{" ws req_pair ("," ws req_pair)* ("," ws opt_pair)* "}"
        std::string required_part = {};
        std::string optional_part = {};

        for (const auto& key : ordered_props) {
            const auto& prop_schema = schema["properties"][key];
            std::string val_rule_name = rule_prefix + "-" + sanitizeRuleName(key);
            std::string val_body = schemaNodeToRuleBody(prop_schema, val_rule_name, new_rules);

            std::string val_ref = {};
            if (kPrimitiveRuleNames.count(val_body)) {
                val_ref = val_body;
            } else {
                new_rules.emplace_back(val_rule_name, val_body);
                val_ref = val_rule_name;
            }

            std::string pair_fragment =
                "\"\\\"" + escapeGbnfString(key) + "\\\"\" ws \":\" ws " + val_ref;

            if (required_set.count(key)) {
                if (!required_part.empty()) {
                  required_part += " ws \",\" ws ";
                }
                required_part += pair_fragment;
            } else {
                optional_part += " (ws \",\" ws " + pair_fragment + ")?";
            }
        }

        // Assemble object rule body
        std::string body = "\"{\" ws ";
        if (!required_part.empty()) {
            body += required_part;
            body += optional_part;
        } else if (!optional_part.empty()) {
            // All properties are optional and there are no required fields.
            // Generating all 2^N orderings for N optional fields would make the grammar
            // exponentially large.  We conservatively fall back to the generic "object"
            // rule, which accepts any JSON object.  Schemas with only optional properties
            // are uncommon in tool-calling scenarios; callers that need strict optional
            // handling should provide at least one required field or use grammar_ebnf directly.
            return "object";
        }
        body += " ws \"}\"";
        return body;
    }

    // Unknown type — fall back to generic value
    return "value";
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string JsonSchemaConverter::schemaToEbnf(const json& schema) {
    if (schema.is_null() || !schema.is_object()) {
        spdlog::debug("JsonSchemaConverter::schemaToEbnf: empty/null schema, returning default grammar");
        return "value ::= .*";
    }

    std::vector<std::pair<std::string, std::string>> new_rules;
        std::string root_body = schemaNodeToRuleBody(schema, "root", new_rules);

    std::ostringstream out = {};
    out << kBaseRules;
    out << "root ::= " << root_body << "\n";
    for (const auto& [name, body] : new_rules) {
        out << name << " ::= " << body << "\n";
    }

    std::string result = out.str();
    if (static_cast<int>(result.size()) > kMaxGrammarBytes) {
            spdlog::warn("JsonSchemaConverter::schemaToEbnf: generated grammar exceeds {} bytes, rejecting",
                         kMaxGrammarBytes);
            return "value ::= .*\\n";
    }
    return result;
}

std::string JsonSchemaConverter::toolsToEbnf(const std::vector<ToolDefinition>& tools) {
    if (tools.empty()) {
        return "";
    }

    std::vector<std::pair<std::string, std::string>> new_rules;

    // Build one alternative per tool:
    // call-<name> ::= "{" ws "\"name\"" ws ":" ws "\"<name>\"" ws "," ws "\"arguments\"" ws ":" ws <args-rule> ws "}"
    std::string root_alternatives = {};
    for (const auto& tool : tools) {
        if (tool.name.empty()) {
            spdlog::warn("JsonSchemaConverter::toolsToEbnf: skipping tool with empty name");
            continue;
        }
        std::string safe_name = sanitizeRuleName(tool.name);
        std::string call_rule_name = "call-" + safe_name;
        std::string args_rule_name = "args-" + safe_name;

        // Generate argument grammar from tool's parameter schema
        std::string args_body = {};
        if (!tool.parameters.is_null() && tool.parameters.is_object()) {
            args_body = schemaNodeToRuleBody(tool.parameters, args_rule_name, new_rules);
        } else {
            args_body = "object";
        }

        std::string args_ref = {};
        if (kPrimitiveRuleNames.count(args_body)) {
            args_ref = args_body;
        } else {
            new_rules.emplace_back(args_rule_name, args_body);
            args_ref = args_rule_name;
        }

        std::string escaped_name = escapeGbnfString(tool.name);
        std::string call_body =
            "\"{\" ws \"\\\"name\\\"\" ws \":\" ws \"\\\"" + escaped_name + "\\\"\" ws \",\" ws "
            "\"\\\"arguments\\\"\" ws \":\" ws " + args_ref + " ws \"}\"";

        new_rules.emplace_back(call_rule_name, call_body);

        if (!root_alternatives.empty()) {
          root_alternatives += " | ";
        }
        root_alternatives += call_rule_name;
    }

    if (root_alternatives.empty()) {
        return "";
    }

    std::ostringstream out = {};
    out << kBaseRules;
    out << "root ::= " << root_alternatives << "\n";
    for (const auto& [name, body] : new_rules) {
        out << name << " ::= " << body << "\n";
    }

    std::string result = out.str();
    if (static_cast<int>(result.size()) > kMaxGrammarBytes) {
        spdlog::warn("JsonSchemaConverter::toolsToEbnf: generated grammar exceeds {} bytes, rejecting",
                     kMaxGrammarBytes);
        return "";
    }
    return result;
}

std::optional<ToolCall> JsonSchemaConverter::parseToolCall(const std::string& text) {
    if (text.empty()) {
      return std::nullopt;
    }

    // Find JSON boundaries: locate the first '{' and the matching '}'
    std::size_t start = text.find('{');
    if (start == std::string::npos) {
      return std::nullopt;
    }

    // Find the matching closing brace
    int depth = 0;
    std::size_t end = std::string::npos;
    for (std::size_t i = start; i < text.size(); ++i) {
        if (text[i] == '{') ++depth;
        else if (text[i] == '}') {
            --depth;
            if (depth == 0) { end = i; break; }
        }
    }
    if (end == std::string::npos) {
      return std::nullopt;
    }

    std::string json_str = text.substr(start, end - start + 1);
    try {
        json obj = json::parse(json_str);
        if (!obj.is_object()) {
          return std::nullopt;
        }

        if (!obj.contains("name") || !obj["name"].is_string()) {
          return std::nullopt;
        }

        ToolCall result;
        result.name = obj["name"].get<std::string>();
        if (obj.contains("arguments") && obj["arguments"].is_object()) {
            result.arguments = obj["arguments"];
        } else {
            result.arguments = json::object();
        }
        return result;
    } catch (const json::exception& e) {
        spdlog::debug("JsonSchemaConverter::parseToolCall: JSON parse failed: {}", e.what());
        return std::nullopt;
    }
}

} // namespace llm
} // namespace themis

