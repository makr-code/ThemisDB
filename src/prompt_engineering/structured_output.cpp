/**
 * @file structured_output.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/structured_output.h"

#include "prompt_engineering/markdown_utils.h"
#include <algorithm>
#include <chrono>
#include <regex>
#include <sstream>

namespace themis {
namespace prompt_engineering {

// ─────────────────────────────────────────────────────────────────────────────
// Repair helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string StructuredOutputEnforcer::stripMarkdownFences(const std::string& text) {
    // Delegate to centralized implementation from markdown_utils.h (Phase 1 consolidation)
    return themis::prompt_engineering::stripMarkdownFences(text);
}

std::string StructuredOutputEnforcer::removeTrailingCommas(const std::string& text) {
    // Replace ,<optional whitespace>} or ,<optional whitespace>]
    static const std::regex trailing_comma(R"(,(\s*[}\]]))", std::regex::ECMAScript);
    return std::regex_replace(text, trailing_comma, "$1");
}

std::string StructuredOutputEnforcer::stripLineComments(const std::string& text) {
    static const std::regex line_comment(R"(//[^\n]*)", std::regex::ECMAScript);
    return std::regex_replace(text, line_comment, "");
}

std::string StructuredOutputEnforcer::repairJson(const std::string& text) {
    std::string result = stripMarkdownFences(text);
    result = stripLineComments(result);
    result = removeTrailingCommas(result);

    // Trim leading / trailing whitespace
    const auto first = result.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
      return result;
    }
    const auto last  = result.find_last_not_of(" \t\r\n");
    return result.substr(first, last - first + 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON structural validator
// ─────────────────────────────────────────────────────────────────────────────

bool StructuredOutputEnforcer::checkJsonStructure(const std::string& text,
                                                   std::vector<std::string>& errors) {
    if (text.empty()) {
        errors.push_back("JSON output is empty");
        return false;
    }

    // Must start with { or [
    if (text.front() != '{' && text.front() != '[') {
        errors.push_back("JSON output does not start with '{' or '['");
        return false;
    }

    // Balance braces/brackets
    int brace_depth   = 0;
    int bracket_depth = 0;
    bool in_string    = false;
    bool escaped      = false;

    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];

        if (escaped) {
            escaped = false;
            continue;
        }
        if (c == '\\' && in_string) {
            escaped = true;
            continue;
        }
        if (c == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
          continue;
        }

        switch (c) {
            case '{': ++brace_depth;   break;
            case '}': --brace_depth;   break;
            case '[': ++bracket_depth; break;
            case ']': --bracket_depth; break;
            default: break;
        }
        if (brace_depth < 0 || bracket_depth < 0) {
            errors.push_back("Unmatched closing brace/bracket at position " +
                             std::to_string(i));
            return false;
        }
    }

    if (brace_depth != 0) {
        errors.push_back("Unmatched opening '{': depth=" +
                         std::to_string(brace_depth));
        return false;
    }
    if (bracket_depth != 0) {
        errors.push_back("Unmatched opening '[': depth=" +
                         std::to_string(bracket_depth));
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Schema parsing helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Extract string values from a JSON array at top-level key @p key.
/// E.g., for key="required", extracts ["name","age"] from
///   {"required":["name","age"],"properties":{…}}
std::vector<std::string> StructuredOutputEnforcer::extractStringArray(
    const std::string& json, const std::string& key) {

    std::vector<std::string> result;

    // Find the key
    const std::string search = "\"" + key + "\"";
    const auto key_pos = json.find(search);
    if (key_pos == std::string::npos) {
      return result;
    }

    // Find the opening '[' after the key
    const auto arr_start = json.find('[', key_pos + search.size());
    if (arr_start == std::string::npos) {
      return result;
    }

    const auto arr_end = json.find(']', arr_start);
    if (arr_end == std::string::npos) {
      return result;
    }

    const std::string arr_content = json.substr(arr_start + 1,
                                                 arr_end - arr_start - 1);

    // Extract quoted strings
    static const std::regex str_re(R"esc("([^"\\]*(?:\\.[^"\\]*)*)")esc",
                                   std::regex::ECMAScript);
    auto begin = std::sregex_iterator(arr_content.begin(),
                                      arr_content.end(), str_re);
    for (auto it = begin; it != std::sregex_iterator(); ++it) {
        result.push_back((*it)[1].str());
    }
    return result;
}

/// Extract property names from the "properties": { … } block in a schema.
std::vector<std::string> StructuredOutputEnforcer::extractPropertyNames(
    const std::string& schema) {

    std::vector<std::string> names;

    const std::string prop_key = "\"properties\"";
    const auto prop_pos = schema.find(prop_key);
    if (prop_pos == std::string::npos) {
      return names;
    }

    const auto obj_start = schema.find('{', prop_pos + prop_key.size());
    if (obj_start == std::string::npos) {
      return names;
    }

    // Find matching closing '}'
    int depth = 0;
    size_t obj_end = std::string::npos;
    bool in_string = false;
    bool escaped   = false;
    for (size_t i = obj_start; i < schema.size(); ++i) {
        const char c = schema[i];
        if (escaped) { escaped = false; continue; }
        if (c == '\\' && in_string) { escaped = true; continue; }
        if (c == '"') { in_string = !in_string; continue; }
        if (in_string) {
          continue;
        }
        if (c == '{') ++depth;
        else if (c == '}') { --depth; if (depth == 0) { obj_end = i; break; } }
    }
    if (obj_end == std::string::npos) {
      return names;
    }

    const std::string props_block = schema.substr(obj_start + 1,
                                                    obj_end - obj_start - 1);

    // Each top-level key in props_block is a property name.
    // We scan for "key": patterns at depth=0 (manual traversal below).
    // Flatten multi-line to single line for simpler scanning:
    std::string flat = props_block;
    std::replace(flat.begin(), flat.end(), '\n', ' ');
    std::replace(flat.begin(), flat.end(), '\r', ' ');

    // Scan manually for depth-0 keys
    bool in_s = false; bool esc = false; int dep = 0;
    std::string cur_key;
    bool reading_key = false;
    for (size_t i = 0; i < flat.size(); ++i) {
        const char c = flat[i];
        if (esc) { esc = false; if (reading_key) cur_key += c; continue; }
        if (c == '\\' && in_s) { esc = true; continue; }
        if (c == '"') {
            if (in_s) {
                in_s = false;
                if (dep == 0 && reading_key) {
                    // confirm next non-space char is ':'
                    size_t j = i + 1;
                    while (j < flat.size() && flat[j] == ' ') {
                      ++j;
                    }
                    if (j < flat.size() && flat[j] == ':') {
                        names.push_back(cur_key);
                    }
                    reading_key = false;
                }
            } else {
                in_s = true;
                if (dep == 0) { cur_key.clear(); reading_key = true; }
                else { reading_key = false; }
            }
            continue;
        }
        if (in_s) { if (reading_key) cur_key += c; continue; }
        if (c == '{' || c == '[') ++dep;
        else if (c == '}' || c == ']') --dep;
    }

    return names;
}

/// Extract all top-level key names from a flat JSON object string.
std::vector<std::string> StructuredOutputEnforcer::extractTopLevelKeys(
    const std::string& json) {

    std::vector<std::string> keys;
    if (json.empty() || json.front() != '{') return keys;

    bool in_string = false;
    bool escaped   = false;
    int  depth     = 0;       // depth inside { }, starts at 0 for outer {}
    std::string cur_key;
    bool reading_key = false;

    for (size_t i = 0; i < json.size(); ++i) {
        const char c = json[i];
        if (escaped) {
            escaped = false;
            if (reading_key) {
              cur_key += c;
            }
            continue;
        }
        if (c == '\\' && in_string) { escaped = true; continue; }
        if (c == '"') {
            if (in_string) {
                in_string = false;
                if (depth == 1 && reading_key) {
                    // Confirm ':' follows
                    size_t j = i + 1;
                    while (j < json.size() && (json[j] == ' ' || json[j] == '\t' ||
                                                json[j] == '\n' || json[j] == '\r'))
                        ++j;
                    if (j < json.size() && json[j] == ':') {
                        keys.push_back(cur_key);
                    }
                    reading_key = false;
                }
            } else {
                in_string = true;
                if (depth == 1) { cur_key.clear(); reading_key = true; }
                else             { reading_key = false; }
            }
            continue;
        }
        if (in_string) { if (reading_key) cur_key += c; continue; }
        if (c == '{' || c == '[') ++depth;
        else if (c == '}' || c == ']') --depth;
    }
    return keys;
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON schema validator
// ─────────────────────────────────────────────────────────────────────────────

bool StructuredOutputEnforcer::validateJsonSchema(
    const std::string&         output,
    const JsonSchemaConstraint& schema,
    std::vector<std::string>&   errors) {

    // Step 1: structural validity
    if (!checkJsonStructure(output, errors)) {
      return false;
    }

    // Step 2: required fields
    const auto required = extractStringArray(schema.schema_json, "required");
    const auto actual_keys = extractTopLevelKeys(output);

    for (const auto& req : required) {
        const bool found = std::find(actual_keys.begin(), actual_keys.end(), req)
                           != actual_keys.end();
        if (!found) {
            errors.push_back("Missing required field: \"" + req + "\"");
        }
    }

    // Step 3: strict mode — no unknown keys
    if (schema.strict_mode && !schema.schema_json.empty()) {
        const auto allowed = extractPropertyNames(schema.schema_json);
        if (!allowed.empty()) {
            for (const auto& k : actual_keys) {
                const bool known = std::find(allowed.begin(), allowed.end(), k)
                                   != allowed.end();
                if (!known) {
                    errors.push_back("Unknown field not in schema properties: \"" +
                                     k + "\"");
                }
            }
        }
    }

    return errors.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Regex validator
// ─────────────────────────────────────────────────────────────────────────────

bool StructuredOutputEnforcer::validateRegex(
    const std::string&             output,
    const RegexGrammarConstraint&  grammar,
    std::vector<std::string>&       errors) {

    if (grammar.pattern.empty()) return true;  // no constraint

    try {
        const std::regex re(grammar.pattern, std::regex::ECMAScript);
        const bool match = grammar.full_match
                           ? std::regex_match(output, re)
                           : std::regex_search(output, re);
        if (!match) {
            errors.push_back("Output does not match regex pattern: " +
                             grammar.pattern);
            return false;
        }
    } catch (const std::regex_error& ex) {
        errors.push_back(std::string("Invalid regex pattern: ") + ex.what());
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// IStructuredOutputEnforcer: validate()
// ─────────────────────────────────────────────────────────────────────────────

bool StructuredOutputEnforcer::validate(const std::string&            output,
                                         const StructuredOutputConfig& config,
                                         std::vector<std::string>&     errors) {
    switch (config.type) {
        case OutputConstraintType::NONE:
            return true;
        case OutputConstraintType::JSON_SCHEMA:
            return validateJsonSchema(output, config.json_schema, errors);
        case OutputConstraintType::REGEX:
            return validateRegex(output, config.regex_grammar, errors);
    }
    return true;  // unreachable
}

// ─────────────────────────────────────────────────────────────────────────────
// IStructuredOutputEnforcer: enforce()
// ─────────────────────────────────────────────────────────────────────────────

StructuredOutputResult StructuredOutputEnforcer::enforce(
    const std::string&            raw_output,
    const StructuredOutputConfig& config) {

    const auto t_start = std::chrono::steady_clock::now();

    StructuredOutputResult result;
    result.raw_output       = raw_output;
    result.validated_output = raw_output;

    if (config.type == OutputConstraintType::NONE) {
        result.is_valid      = true;
        result.attempts_used = 0;
        return result;
    }

    const int max_attempts =
        (config.type == OutputConstraintType::JSON_SCHEMA)
        ? std::max(1, config.json_schema.max_retries)
        : 1;

    std::string working = raw_output;
    if (config.strip_markdown) {
      working = stripMarkdownFences(working);
    }

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        result.attempts_used = attempt;
        result.validation_errors.clear();

        std::string candidate = working;
        if (config.type == OutputConstraintType::JSON_SCHEMA && config.repair_json) {
            candidate = repairJson(candidate);
        }

        result.validated_output = candidate;

        if (validate(candidate, config, result.validation_errors)) {
            result.is_valid = true;
            break;
        }

        // Next iteration: apply heavier repair (already done above)
        working = candidate;
    }

    const auto t_end = std::chrono::steady_clock::now();
    result.total_latency_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    return result;
}

} // namespace prompt_engineering
} // namespace themis
