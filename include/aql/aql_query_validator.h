/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_query_validator.h                              ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 19:28:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     117                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace aql {

// Forward declaration so AQLQueryBuilder can be validated without a circular include
class AQLQueryBuilder;

// ============================================================================
// Validation result types
// ============================================================================

/**
 * @brief A single validation issue found in an AQL query.
 */
struct ValidationIssue {
    enum class Severity {
        ERROR,   ///< Query cannot be executed
        WARNING, ///< Query may produce unexpected results
        INFO     ///< Stylistic or performance suggestion
    };

    Severity    severity;
    std::string message;  ///< Human-readable description
    std::string clause;   ///< Which part of the query triggered the issue
};

/**
 * @brief Aggregated result returned by AQLQueryValidator.
 */
struct ValidationResult {
    bool                         is_valid; ///< true iff no ERRORs are present
    std::vector<ValidationIssue> issues;

    /// @return true if at least one ERROR issue is present
    bool hasErrors() const;
    /// @return true if at least one WARNING issue is present
    bool hasWarnings() const;
    /// @return Single-line summary string (e.g. "1 error, 2 warnings")
    std::string summary() const;
};

// ============================================================================
// Validator
// ============================================================================

/**
 * @brief Rule-based structural validator and linter for AQL queries.
 *
 * Performs purely syntactic/structural checks — no LLM required.
 * The validator operates in two modes:
 *  1. String mode  — validates a complete AQL query string
 *  2. Builder mode — validates an `AQLQueryBuilder` in progress
 *
 * Rules checked:
 * - Presence of FOR and RETURN clauses (ERRORS when missing)
 * - Variables used in FILTER/SORT/RETURN are defined in a preceding FOR/LET (WARNING)
 * - LIMIT value > 0 (WARNING on 0)
 * - COLLECT and SORT ordering that could reduce performance (INFO)
 * - Missing RETURN (ERROR)
 * - Empty collection/variable names (ERROR, only in builder mode)
 */
class AQLQueryValidator {
public:
    AQLQueryValidator()  = default;
    ~AQLQueryValidator() = default;

    /**
     * @brief Validate a fully formed AQL query string.
     * @param query AQL query to validate
     * @return ValidationResult with all issues found
     */
    ValidationResult validate(const std::string& query) const;

    /**
     * @brief Validate an AQLQueryBuilder (may be partial).
     *
     * Only issues that can be detected from the builder's current state are
     * reported; clauses not yet added are not treated as errors.
     *
     * @param builder Builder to validate
     * @return ValidationResult with all issues found
     */
    ValidationResult validate(const AQLQueryBuilder& builder) const;
};

} // namespace aql
} // namespace themis
