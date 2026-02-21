/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            input_validator.h                                  ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8fbe6a439  2026-02-20  security: Production readiness – policy engine, auth, aud... ║
    • 244e56430  2025-11-17  Add merge conflict report for feature/complete-database-c... ║
    • c1eedf00d  2025-11-17  Phase 0.2: R-Tree Spatial Index (table-agnostic for all 5... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {

class InputValidator {
public:
    explicit InputValidator(std::string schema_dir);

    // JSON schema validation against a JSON Schema Draft-7 file.
    // Loads the schema from <schema_dir>/<schema_name>.json and validates `payload` against it.
    // Supported keywords:
    //   Top-level:   type ("object"), required, properties, additionalProperties
    //   Per property: type (string/object/number/integer/boolean/array/null),
    //                 enum, minLength, maxLength, pattern,
    //                 minimum, maximum, exclusiveMinimum, exclusiveMaximum
    // Returns std::nullopt if valid (or if no schema file is found), otherwise an error message.
    std::optional<std::string> validateJsonStub(
        const nlohmann::json& payload,
        const std::string& schema_name
    ) const;

    // Validate AQL request payload (expects keys like {"query": "...", "bindVars": {...}})
    // Performs minimal checks: required fields, max length, disallowed characters/patterns
    std::optional<std::string> validateAqlRequest(const nlohmann::json& payload) const;

    // Validate path segment (e.g., entity key); rejects traversal and separators
    bool validatePathSegment(const std::string& segment) const;

    // Sanitize strings for logs (strip control chars and truncate)
    std::string sanitizeForLogs(const std::string& input, size_t max_len = 512) const;

    // Configure/query schema directory
    const std::string& schemaDir() const { return schema_dir_; }

private:
    std::string schema_dir_;

    // Helper to load a stub schema from schema_dir_/name.json
    std::optional<nlohmann::json> loadSchema(const std::string& schema_name) const;
};

} // namespace utils
} // namespace themis
