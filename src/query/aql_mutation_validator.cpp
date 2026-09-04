/**
 * @file aql_mutation_validator.cpp
 * @brief Semantic validator for AQL mutation AST nodes — EPIC-004 Phase 2.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "query/aql_mutation_validator.h"

#include <cctype>
#include <string>

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// isValidCollectionName
// ---------------------------------------------------------------------------

bool AqlMutationValidator::isValidCollectionName(std::string_view name) const {
    if (name.empty() || static_cast<int>(name.size()) > 256) {
      return false;
    }

    // First character: letter or underscore
    const char first = name[0];
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_') {
        return false;
    }

    // Remaining characters: letter, digit, or underscore
    for (std::size_t i = 1; i <static_cast<int>(name.size()); ++i) {
        const char c = name[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// isValidFieldName
// ---------------------------------------------------------------------------

bool AqlMutationValidator::isValidFieldName(std::string_view name) const {
    if (name.empty() || static_cast<int>(name.size()) > 256) {
      return false;
    }

    // First character must not be a digit
    const char first = name[0];
    if (std::isdigit(static_cast<unsigned char>(first))) {
        return false;
    }

    // All characters: letter, digit, underscore, or dot (path separator)
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// validate (dispatcher)
// ---------------------------------------------------------------------------

MutationValidationResult AqlMutationValidator::validate(const MutationNode& node) const {
    switch (node.getType()) {
        case ASTNodeType::Insert:
            return validateInsert(dynamic_cast<const InsertNode&>(node));
        case ASTNodeType::Update:
            return validateUpdate(dynamic_cast<const UpdateNode&>(node));
        case ASTNodeType::Remove:
            return validateRemove(dynamic_cast<const RemoveNode&>(node));
        case ASTNodeType::Replace:
            return validateReplace(dynamic_cast<const ReplaceNode&>(node));
        case ASTNodeType::Upsert:
            return validateUpsert(dynamic_cast<const UpsertNode&>(node));
        default: {
            MutationValidationResult r;
            r.addError("Unknown mutation node type — cannot validate.");
            return r;
        }
    }
}

// ---------------------------------------------------------------------------
// validateInsert
// ---------------------------------------------------------------------------

MutationValidationResult AqlMutationValidator::validateInsert(const InsertNode& node) const {
    MutationValidationResult result = {};

    if (!isValidCollectionName(node.collection)) {
        result.addError(
            "INSERT: invalid or empty collection name '" + node.collection + "'. "
            "Collection names must start with a letter or underscore and contain "
            "only [A-Za-z0-9_], max 256 characters.");
    }

    if (node.documents.empty()) {
        result.addError(
            "INSERT: at least one document expression is required. "
            "Found zero documents in INSERT statement.");
    }

    return result;
}

// ---------------------------------------------------------------------------
// validateUpdate
// ---------------------------------------------------------------------------

MutationValidationResult AqlMutationValidator::validateUpdate(const UpdateNode& node) const {
    MutationValidationResult result = {};

    if (!isValidCollectionName(node.collection)) {
        result.addError(
            "UPDATE: invalid or empty collection name '" + node.collection + "'. "
            "Collection names must start with a letter or underscore and contain "
            "only [A-Za-z0-9_], max 256 characters.");
    }

    // Must have either SET clauses (SQL-style) or an update_expr (AQL-native)
    const bool hasClauses = !node.set_clauses.empty();
    const bool hasExpr    = (node.update_expr != nullptr);
    if (!hasClauses && !hasExpr) {
        result.addError(
            "UPDATE: no update specification provided. "
            "Supply either SQL-style SET clauses or an AQL WITH expression.");
    }

    // Validate each SET clause field name
    for (const auto& clause : node.set_clauses) {
        if (clause.field.empty()) {
            result.addError(
                "UPDATE: SET clause contains an empty field name. "
                "All field names in SET clauses must be non-empty.");
        } else if (!isValidFieldName(clause.field)) {
            result.addError(
                "UPDATE: SET clause field name '" + clause.field + "' is invalid. "
                "Field names may contain [A-Za-z0-9_.] and must not start with a digit.");
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// validateRemove
// ---------------------------------------------------------------------------

MutationValidationResult AqlMutationValidator::validateRemove(const RemoveNode& node) const {
    MutationValidationResult result = {};

    if (!isValidCollectionName(node.collection)) {
        result.addError(
            "REMOVE: invalid or empty collection name '" + node.collection + "'. "
            "Collection names must start with a letter or underscore and contain "
            "only [A-Za-z0-9_], max 256 characters.");
    }

    // No filter is a warning (could remove the entire collection)
    if (!node.filter && !node.doc_expr) {
        result.addWarning(
            "REMOVE: no FILTER or document expression provided. "
            "This may remove the entire collection '" + node.collection + "'. "
            "Add a FILTER predicate to limit the scope of deletion.");
    }

    return result;
}

// ---------------------------------------------------------------------------
// validateReplace
// ---------------------------------------------------------------------------

MutationValidationResult AqlMutationValidator::validateReplace(const ReplaceNode& node) const {
    MutationValidationResult result = {};

    if (!isValidCollectionName(node.collection)) {
        result.addError(
            "REPLACE: invalid or empty collection name '" + node.collection + "'. "
            "Collection names must start with a letter or underscore and contain "
            "only [A-Za-z0-9_], max 256 characters.");
    }

    if (!node.search_expr) {
        result.addError(
            "REPLACE: search expression is null. "
            "REPLACE requires a search expression to locate the document to replace.");
    }

    if (!node.replacement) {
        result.addError(
            "REPLACE: replacement document expression is null. "
            "REPLACE requires a replacement document expression.");
    }

    return result;
}

// ---------------------------------------------------------------------------
// validateUpsert
// ---------------------------------------------------------------------------

MutationValidationResult AqlMutationValidator::validateUpsert(const UpsertNode& node) const {
    MutationValidationResult result = {};

    if (!isValidCollectionName(node.collection)) {
        result.addError(
            "UPSERT: invalid or empty collection name '" + node.collection + "'. "
            "Collection names must start with a letter or underscore and contain "
            "only [A-Za-z0-9_], max 256 characters.");
    }

    if (!node.search_expr) {
        result.addError(
            "UPSERT: search expression is null. "
            "UPSERT requires a search expression to determine insert-vs-update branch.");
    }

    if (!node.insert_doc) {
        result.addError(
            "UPSERT: insert document is null. "
            "UPSERT INSERT clause requires a document expression.");
    }

    if (!node.update_doc) {
        result.addError(
            "UPSERT: update document is null. "
            "UPSERT UPDATE clause requires an update expression.");
    }

    return result;
}

} // namespace query
} // namespace themis
