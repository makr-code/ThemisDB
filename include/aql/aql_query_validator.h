/**
 * @file aql_query_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "aql/aql_schema_provider.h"
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
    bool                         is_valid = 0; ///< true iff no ERRORs are present
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
 * The validator operates in three modes:
 *  1. String mode  — validates a complete AQL query string
 *  2. Builder mode — validates an `AQLQueryBuilder` in progress
 *  3. Schema-aware mode — validates a query string against known collections
 *
 * Rules checked:
 * - Presence of FOR and RETURN clauses (ERRORS when missing)
 * - Variables used in FILTER/SORT/RETURN are defined in a preceding FOR/LET (WARNING)
 * - LIMIT value > 0 (WARNING on 0)
 * - COLLECT and SORT ordering that could reduce performance (INFO)
 * - Missing RETURN (ERROR)
 * - Empty collection/variable names (ERROR, only in builder mode)
 * - Schema-aware: unknown collection names (WARNING, only in schema-aware mode)
 * - Schema-aware: unknown field names in FILTER/SORT/RETURN (WARNING, only in schema-aware mode)
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
     * @brief Validate a fully formed AQL query string against a schema.
     *
     * Runs all standard structural checks, then additionally:
     *  - Warns when a collection used in a FOR clause is not present in
     *    @p schema (@c WARNING severity).
     *  - Warns when a field access (@c variable.field) refers to a field
     *    that is not listed in the schema for that collection (@c WARNING).
     *
     * @param query   AQL query to validate.
     * @param schema  Collection metadata snapshot to validate against.
     * @return ValidationResult with all issues found.
     */
    ValidationResult validate(
        const std::string& query,
        const std::vector<CollectionMetadata>& schema
    ) const;

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

    /**
     * @brief Validate an AQLQueryBuilder against an explicit schema snapshot.
     *
     * Runs all structural checks (same as @c validate(builder)), then applies
     * schema-aware checks against the provided @p schema instead of any schema
     * that may be attached to the builder via @c AQLQueryBuilder::setSchema():
     *  - Warns when a collection used in a FOR clause is absent from @p schema.
     *  - Warns when a field access (@c variable.field) refers to a field not
     *    listed in the schema for that collection.
     *
     * @param builder Builder to validate (may be partial or complete).
     * @param schema  External collection metadata snapshot to validate against.
     * @return ValidationResult with all issues found.
     */
    ValidationResult validate(
        const AQLQueryBuilder& builder,
        const std::vector<CollectionMetadata>& schema
    ) const;

private:
    void checkUnknownCollections(
        const std::string& query,
        const std::vector<CollectionMetadata>& schema,
        ValidationResult& result
    ) const;

    void checkUnknownFields(
        const std::string& query,
        const std::vector<CollectionMetadata>& schema,
        ValidationResult& result
    ) const;
};

} // namespace aql
} // namespace themis
