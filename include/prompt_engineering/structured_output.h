/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            structured_output.h                                ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// Structured output enforcement: JSON schema + regex grammar
#include <string>
#include <vector>

namespace themis { namespace prompt_engineering {

struct JsonSchemaConstraint {
    std::string schema_json;
    bool strict_mode = true;
    int max_retries = 3;
};

struct RegexGrammarConstraint {
    std::string pattern;
    bool full_match = true;
    int max_tokens = 512;
};

enum class OutputConstraintType { JSON_SCHEMA, REGEX, NONE };

struct StructuredOutputConfig {
    OutputConstraintType type = OutputConstraintType::NONE;
    JsonSchemaConstraint json_schema;
    RegexGrammarConstraint regex_grammar;
    bool repair_json = true;
    bool strip_markdown = true;
};

struct StructuredOutputResult {
    std::string raw_output;
    std::string validated_output;
    bool is_valid = false;
    std::vector<std::string> validation_errors;
    int attempts_used = 1;
    double total_latency_ms = 0.0;
};

class IStructuredOutputEnforcer {
public:
    virtual ~IStructuredOutputEnforcer() = default;
    virtual StructuredOutputResult enforce(
        const std::string& raw_output,
        const StructuredOutputConfig& config) = 0;
    virtual bool validate(const std::string& output,
                           const StructuredOutputConfig& config,
                           std::vector<std::string>& errors) = 0;
};

}} // namespace themis::prompt_engineering
