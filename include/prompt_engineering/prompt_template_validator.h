/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_template_validator.h                        ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:18:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 240a2c1d8b  2026-04-12  feat(prompt_engineering): Typed Template DSL — PromptTemp... ║
    • b80f6d5ecc  2026-02-20  Create PromptTemplateValidator class with pragma guards ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_template_validator.h
 * @brief Structural validator for serialised PromptTemplate JSON documents.
 *
 * `PromptTemplateValidator` checks that a `nlohmann::json` object produced by
 * `PromptTemplate::toJson()` satisfies a configurable set of field-presence and
 * type rules.  It is independent of the `PromptManager` struct hierarchy so that
 * external tooling (CI pipelines, admin APIs, import/export layer) can validate
 * raw JSON payloads without linking the full `PromptManager`.
 *
 * ## Validation rules (always enforced)
 *
 * | Field         | Rule                                       |
 * |---------------|--------------------------------------------|
 * | `id`          | string, non-empty                          |
 * | `name`        | string, non-empty                          |
 * | `version`     | string, non-empty                          |
 * | `content`     | string, non-empty                          |
 * | `description` | string (may be empty – warning only)       |
 * | `active`      | boolean                                    |
 * | `images`      | array; each element must have `alt_text`   |
 * | `metadata`    | object or null                             |
 *
 * Additional field rules can be supplied at construction time via `FieldRule`
 * entries; unknown extra fields in the payload are ignored.
 *
 * ## Usage
 * ```cpp
 * PromptTemplateValidator v;
 * auto result = v.validate(my_json);
 * if (!result.valid) { for (auto& e : result.errors) { ... } }
 * ```
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// TemplateValidationResult
// ============================================================================

/**
 * @brief Result of a `PromptTemplateValidator::validate()` call.
 *
 * Mirrors `PromptManager::ValidationResult` so that callers that already work
 * with the manager's type can use the same pattern here.
 */
struct TemplateValidationResult {
    bool valid = true;
    std::vector<std::string> errors;   ///< Hard failures; template must not be used.
    std::vector<std::string> warnings; ///< Non-fatal advisories.
};

// ============================================================================
// PromptTemplateValidator
// ============================================================================

/**
 * @brief Validates a serialised `PromptTemplate` JSON document.
 *
 * The default constructor enforces the built-in field rules described above.
 * Pass `require_id = false` to skip the `id` check when validating templates
 * that have not been persisted yet (i.e., before `PromptManager::createTemplate`
 * assigns an id).
 */
class PromptTemplateValidator {
public:
    /**
     * @brief Construct a validator.
     * @param require_id  When true (default), an empty or missing `id` field is
     *                    an error.  Set to false for pre-persist validation.
     */
    explicit PromptTemplateValidator(bool require_id = true);

    /**
     * @brief Validate @p templateJson against the built-in field rules.
     * @return `TemplateValidationResult` with zero errors when the document is
     *         structurally valid.
     */
    TemplateValidationResult validate(const nlohmann::json& templateJson) const;

    /**
     * @brief Convenience overload — parse @p json_str before validating.
     *
     * Returns an invalid result with a single error entry on parse failure.
     */
    TemplateValidationResult validate(const std::string& json_str) const;

private:
    bool require_id_;
};

} // namespace prompt_engineering
} // namespace themis
