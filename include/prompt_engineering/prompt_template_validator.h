/**
 * @file prompt_template_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 *
 * ## Injection Detection (Phase 3 Hardening)
 * The validator detects common injection patterns:
 * - SQL injection: quoted SQL keywords, common SQL injection syntax
 * - Command injection: shell metacharacters, command separators
 * - Path traversal: "../" sequences, absolute path markers
 * - Template injection: malformed variable references, escape sequences
 *
 * All injection patterns are logged as warnings or errors depending on severity.
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
     * @param[in] templateJson Input parameter.
     */
    TemplateValidationResult validate(const nlohmann::json& templateJson) const;

    /**
     * @brief Convenience overload — parse @p json_str before validating.
     *
     * Returns an invalid result with a single error entry on parse failure.
     * @param[in] json_str Input parameter.
     * @return Return value.
     */
    TemplateValidationResult validate(const std::string& json_str) const;

    /**
     * @brief Detect injection attack patterns in template content.
     * 
     * Checks for:
     * - SQL injection patterns (UNION, DROP, INSERT, DELETE, SELECT with quotes)
     * - Command injection patterns (|, &, ;, `, $(), backticks)
     * - Path traversal patterns (../, ., ..\)
     * - Template injection patterns ({{{, }}}, malformed variables)
     *
     * @param content The template content to check for injection patterns
     * @return `TemplateValidationResult` with warnings/errors for detected patterns
     */
    TemplateValidationResult detectInjectionPatterns(const std::string& content) const;

private:
    bool require_id_;

    /// ====== INJECTION DETECTION HELPERS (Phase 3 Hardening) ======

    /**
     * Check for SQL injection patterns in the given string.
     * @return true if SQL injection pattern detected
     * @brief TBD: Describe hasSQLInjectionPattern.
     * @param[in] content Input parameter.
     */
    bool hasSQLInjectionPattern(const std::string& content) const;

    /**
     * Check for command injection patterns.
     * @return true if command injection pattern detected
     * @brief TBD: Describe hasCommandInjectionPattern.
     * @param[in] content Input parameter.
     */
    bool hasCommandInjectionPattern(const std::string& content) const;

    /**
     * Check for path traversal patterns.
     * @return true if path traversal pattern detected
     * @brief TBD: Describe hasPathTraversalPattern.
     * @param[in] content Input parameter.
     */
    bool hasPathTraversalPattern(const std::string& content) const;

    /**
     * Check for template injection patterns.
     * @return true if template injection pattern detected
     * @brief TBD: Describe hasTemplateInjectionPattern.
     * @param[in] content Input parameter.
     */
    bool hasTemplateInjectionPattern(const std::string& content) const;
};

} // namespace prompt_engineering
} // namespace themis
