/**
 * @file aql_safety_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// AI Safety Layer — Schicht 3: AQL Read-Only Enforcer
//
// AqlSafetyValidator scans a raw AQL query string for mutation operations
// BEFORE the query is handed to the query engine. It is a fast, deterministic,
// token-based filter with no external dependencies and p99 < 0.1 ms for
// queries up to 64 KB.
//
// Activated when a tool spec carries `enforce_read_only: true` (e.g. the
// `aql_execute` MCP tool when called from the `agentic` LLM mode).
//
// Full documentation: docs/de/security/ai_safety/AI_SAFETY_AQL_VALIDATOR.md
// Roadmap:            src/security/ROADMAP.md § Phase 5 (ASL-3)

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace query {

class AqlSafetyValidator {
public:
    enum class ValidationMode {
        ReadOnly,        ///< Default — block all mutation keywords (existing behaviour)
        AllowMutations,  ///< Permit DML (INSERT/UPDATE/REMOVE/REPLACE/UPSERT/DELETE)
    };

    struct Violation {
        std::string keyword;
        std::size_t position;
        std::string message;
    };

    explicit AqlSafetyValidator(ValidationMode mode = ValidationMode::ReadOnly)
        : mode_(mode) {}

    ~AqlSafetyValidator()                                    = default;
    AqlSafetyValidator(const AqlSafetyValidator&)            = default;
    AqlSafetyValidator& operator=(const AqlSafetyValidator&) = default;

    [[nodiscard]] std::optional<Violation> validate(std::string_view aql_query) const;

    [[nodiscard]] std::optional<Violation> validateMutationSafety(
        std::string_view aql_query) const;

    [[nodiscard]] bool isSafe(std::string_view aql_query) const {
        return !validate(aql_query).has_value();
    }

private:
    ValidationMode mode_ = ValidationMode::ReadOnly;

    /**
     * @brief To Upper.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string toUpper(const std::string& s);

    /**
     * @brief Find Keyword.
     * @param[in] haystack Input parameter.
     * @param[in] needle Input parameter.
     * @return Return value.
     */
    static std::size_t findKeyword(const std::string& haystack,
                                   std::string_view   needle);
};

} // namespace query
} // namespace themis
