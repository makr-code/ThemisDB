/**
 * @file aql_utils.h
 * @brief Utility functions for AQL (Attribute Query Language) operations.
 *
 * @details Provides helper functions for AQL query construction, parameter binding,
 * validation, serialization, and response parsing.
 *
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once
/*
 * aql_utils.h – lightweight AQL escaping and identifier validation helpers.
 *
 * These utilities are intentionally header-only so that they can be used both
 * from production code and from unit tests without requiring link-time coupling
 * to the gRPC service library.
 */

#include <cctype>
#include <string>

namespace themis::api {

/// Escape a string for safe embedding inside an AQL single-quoted literal.
/// Replaces `\` and `'` to prevent AQL string-literal injection.
inline std::string aqlEscapeLiteral(const std::string& raw) {
    std::string out;
    out.reserve(raw.size() + 4);
    for (char c : raw) {
        if (c == '\\') { out += "\\\\"; }
        else if (c == '\'') { out += "\\'"; }
        else { out += c; }
    }
    return out;
}

/// Validate a name that will be used as an AQL collection identifier
/// (i.e. directly after `FOR doc IN <name>` or `INSERT ... INTO <name>`).
///
/// AQL identifiers must:
///   - not be empty
///   - start with a letter (`a-z`, `A-Z`) or underscore (`_`)
///   - contain only letters, digits (`0-9`), or underscores
///
/// Returns `true` if the name is safe to embed as a bare AQL identifier.
inline bool isValidAqlIdentifier(const std::string& name) {
    if (name.empty()) {
      return false;
    }
    // Check underscore before isalpha to avoid UB on negative char values.
    if (name[0] != '_' && !std::isalpha(static_cast<unsigned char>(name[0])))
        return false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
    }
    return true;
}

} // namespace themis::api

