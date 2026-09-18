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

struct MutationValidationResult {
    bool valid = true;                     ///< False as soon as any error is added.
    std::vector<std::string> errors;       ///< Error messages (empty when valid).
    std::vector<std::string> warnings;     ///< Non-blocking warnings.

    /**
     * @brief Add Error.
     * @param[in] msg Input parameter.
     * @details Calls: push_back(), std::move().
     */
    void addError(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
    }

    /**
     * @brief Add Warning.
     * @param[in] msg Input parameter.
     * @details Calls: push_back(), std::move().
     */
    void addWarning(std::string msg) {
        warnings.push_back(std::move(msg));
    }
};

class AqlMutationValidator {
public:
    AqlMutationValidator()  = default;
    ~AqlMutationValidator() = default;

    // Non-copyable, non-movable (stateless — copy semantics are not meaningful)
    AqlMutationValidator(const AqlMutationValidator&)            = default;
    AqlMutationValidator& operator=(const AqlMutationValidator&) = default;

    [[nodiscard]] MutationValidationResult validate(const MutationNode& node) const;

    [[nodiscard]] bool isValidCollectionName(std::string_view name) const;

    [[nodiscard]] bool isValidFieldName(std::string_view name) const;

private:
    [[nodiscard]] MutationValidationResult validateInsert(const InsertNode& node) const;

    [[nodiscard]] MutationValidationResult validateUpdate(const UpdateNode& node) const;

    [[nodiscard]] MutationValidationResult validateRemove(const RemoveNode& node) const;

    [[nodiscard]] MutationValidationResult validateReplace(const ReplaceNode& node) const;

    [[nodiscard]] MutationValidationResult validateUpsert(const UpsertNode& node) const;
};

} // namespace query
} // namespace themis
