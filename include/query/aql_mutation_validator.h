/**
 * @file aql_mutation_validator.h
 * @brief Semantic validator for AQL mutation AST nodes — EPIC-004 Phase 2.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2 implementation
 */

#pragma once

#include "query/aql_parser.h"
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace query {

/**
 * @brief Result of semantic mutation validation.
 *
 * Carries error messages (blocking) and warnings (non-blocking) produced by
 * AqlMutationValidator::validate().  The @c valid flag is set to @c false as
 * soon as the first error is added.
 */
struct MutationValidationResult {
    bool valid = true;                     ///< False as soon as any error is added.
    std::vector<std::string> errors;       ///< Error messages (empty when valid).
    std::vector<std::string> warnings;     ///< Non-blocking warnings.

    /// @brief Record a blocking error and mark the result invalid.
    /// @param msg  Human-readable error description.
    void addError(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
    }

    /// @brief Record a non-blocking warning.
    /// @param msg  Human-readable warning description.
    void addWarning(std::string msg) {
        warnings.push_back(std::move(msg));
    }
};

/**
 * @brief Semantic validator for AQL mutation AST nodes (EPIC-004 Phase 2).
 *
 * Validates @c MutationNode instances produced by @c AQLParser::parseMutation().
 * This layer runs **after** parsing and **before** translation/execution.
 *
 * ### What is checked
 * - Collection name is non-empty and contains only valid identifier characters.
 * - **INSERT**: at least one document expression; document list is non-empty.
 * - **UPDATE**: at least one SET clause **or** an update_expr is present; no
 *   empty field names appear in set_clauses.
 * - **REMOVE**: collection name valid; absence of a filter produces a warning
 *   (full-collection removal risk).
 * - **REPLACE**: both search_expr and replacement must be non-null.
 * - **UPSERT**: search_expr, insert_doc, and update_doc must all be non-null.
 *
 * ### Thread safety
 * Stateless — all methods are @c const and produce no side effects.
 * Safe for concurrent use without external synchronisation.
 */
class AqlMutationValidator {
public:
    AqlMutationValidator()  = default;
    ~AqlMutationValidator() = default;

    // Non-copyable, non-movable (stateless — copy semantics are not meaningful)
    AqlMutationValidator(const AqlMutationValidator&)            = default;
    AqlMutationValidator& operator=(const AqlMutationValidator&) = default;

    /**
     * @brief Validate a parsed MutationNode.
     *
     * Dispatches to the type-specific validate method based on
     * @c node.getType().  Unknown node types produce a single error.
     *
     * @param node  The mutation node to validate.  Must not be a null-derived
     *              reference.
     * @return MutationValidationResult with accumulated errors and warnings.
     */
    [[nodiscard]] MutationValidationResult validate(const MutationNode& node) const;

    /**
     * @brief Validate a collection name string.
     *
     * A valid collection name:
     * - Is non-empty.
     * - Starts with a letter ([A-Za-z]) or underscore (`_`).
     * - Contains only [A-Za-z0-9_] characters.
     * - Is at most 256 characters long.
     *
     * @param name  Collection name to validate.
     * @return @c true if @p name satisfies all constraints.
     */
    [[nodiscard]] bool isValidCollectionName(std::string_view name) const;

    /**
     * @brief Validate a field name string.
     *
     * A valid field name:
     * - Is non-empty.
     * - Does not start with a digit.
     * - Contains only [A-Za-z0-9_.] characters (dots allowed for path access).
     * - Is at most 256 characters long.
     *
     * @param name  Field name (may be a dot-separated path, e.g. "address.city").
     * @return @c true if @p name satisfies all constraints.
     */
    [[nodiscard]] bool isValidFieldName(std::string_view name) const;

private:
    /// @brief Validate an InsertNode.
    /// @param node  Concrete insert node.
    /// @return Accumulated result.
    [[nodiscard]] MutationValidationResult validateInsert(const InsertNode& node) const;

    /// @brief Validate an UpdateNode.
    /// @param node  Concrete update node.
    /// @return Accumulated result.
    [[nodiscard]] MutationValidationResult validateUpdate(const UpdateNode& node) const;

    /// @brief Validate a RemoveNode.
    /// @param node  Concrete remove node.
    /// @return Accumulated result.
    [[nodiscard]] MutationValidationResult validateRemove(const RemoveNode& node) const;

    /// @brief Validate a ReplaceNode.
    /// @param node  Concrete replace node.
    /// @return Accumulated result.
    [[nodiscard]] MutationValidationResult validateReplace(const ReplaceNode& node) const;

    /// @brief Validate an UpsertNode.
    /// @param node  Concrete upsert node.
    /// @return Accumulated result.
    [[nodiscard]] MutationValidationResult validateUpsert(const UpsertNode& node) const;
};

} // namespace query
} // namespace themis
