/**
 * @file aql_query_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct ValidationResult {
    bool                         is_valid = 0; ///< true iff no ERRORs are present
    std::vector<ValidationIssue> issues;

    /**
     * @brief Has Errors.
     * @return True when the operation succeeds.
     */
    bool hasErrors() const;
    /**
     * @brief Has Warnings.
     * @return True when the operation succeeds.
     */
    bool hasWarnings() const;
    /**
     * @brief Summary.
     * @return Return value.
     */
    std::string summary() const;
};

// ============================================================================
// Validator
// ============================================================================

class AQLQueryValidator {
public:
    AQLQueryValidator()  = default;
    ~AQLQueryValidator() = default;

    /**
     * @brief Validate.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const std::string& query) const;

    /**
     * @brief Validate.
     * @param[in] query Input parameter.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    ValidationResult validate(
        const std::string& query,
        const std::vector<CollectionMetadata>& schema
    ) const;

    /**
     * @brief Validate.
     * @param[in] builder Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const AQLQueryBuilder& builder) const;

    /**
     * @brief Validate.
     * @param[in] builder Input parameter.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    ValidationResult validate(
        const AQLQueryBuilder& builder,
        const std::vector<CollectionMetadata>& schema
    ) const;

private:
    /**
     * @brief Check Unknown Collections.
     * @param[in] query Input parameter.
     * @param[in] schema Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void checkUnknownCollections(
        const std::string& query,
        const std::vector<CollectionMetadata>& schema,
        ValidationResult& result
    ) const;

    /**
     * @brief Check Unknown Fields.
     * @param[in] query Input parameter.
     * @param[in] schema Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void checkUnknownFields(
        const std::string& query,
        const std::vector<CollectionMetadata>& schema,
        ValidationResult& result
    ) const;
};

} // namespace aql
} // namespace themis
