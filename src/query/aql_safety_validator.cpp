/**
 * @file aql_safety_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// AI Safety Layer — Schicht 3: AQL Read-Only Enforcer
// Full documentation: docs/de/security/ai_safety/AI_SAFETY_AQL_VALIDATOR.md
// Roadmap:            src/security/ROADMAP.md § Phase 5 (ASL-3)

#include "query/aql_safety_validator.h"

#include <algorithm>
#include <cctype>
#include <fmt/format.h>

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// A mutation pattern: keyword to search for and its description.
struct MutationPattern {
    const char* keyword;    ///< Uppercase token (with trailing space where needed)
    const char* label;      ///< Human-readable label for error messages
};

/// DML and DDL mutation keywords that indicate a write operation.
/// Trailing space is intentional: it prevents false-positive matches on
/// partial tokens.  For example, "REMOVE " matches "REMOVE u IN col" but
/// NOT "REMOVES" or "REMOVE_BY".  Every keyword that can appear directly
/// before an identifier or collection name must carry this trailing space.
/// "CREATE COLLECTION" and "DROP " are exceptions that use a longer prefix
/// or rely on context to avoid false positives.
static constexpr MutationPattern kMutationPatterns[] = {
    {"UPSERT ",           "UPSERT"},
    {"INSERT ",           "INSERT"},
    {"UPDATE ",           "UPDATE"},
    {"REPLACE ",          "REPLACE"},
    {"REMOVE ",           "REMOVE"},
    {"DROP ",             "DROP"},
    {"TRUNCATE ",         "TRUNCATE"},
    {"CREATE COLLECTION", "CREATE COLLECTION"},
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// AqlSafetyValidator implementation
// ---------------------------------------------------------------------------

// static
std::string AqlSafetyValidator::toUpper(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
        out.push_back(static_cast<char>(std::toupper(c)));
    }
    return out;
}

// static
std::size_t AqlSafetyValidator::findKeyword(const std::string& haystack,
                                             std::string_view   needle) {
    const auto pos = haystack.find(needle);
    return pos;  // std::string::npos if not found
}

std::optional<AqlSafetyValidator::Violation>
AqlSafetyValidator::validateMutationSafety(std::string_view aql_query) const {
    // Convert to std::string for operations that rely on std::string APIs
    const std::string query_str(aql_query);

    // --- Embedded NUL character check ----------------------------------------
    if (query_str.find('\0') != std::string::npos) {
        return Violation{
            "NUL_INJECTION",
            static_cast<std::size_t>(query_str.find('\0')),
            "AQL_MUTATION_SAFETY: Embedded NUL character detected in query. "
            "This is a classic injection vector and is unconditionally rejected."
        };
    }

    const std::string upper = toUpper(query_str);

    // --- Multi-statement injection patterns -----------------------------------
    static constexpr const char* kInjectionPatterns[] = {
        "; DROP ",
        "; DELETE ",
        "; UPDATE ",
    };
    for (const auto* pat : kInjectionPatterns) {
        const std::size_t pos = findKeyword(upper, pat);
        if (pos != std::string::npos) {
            // Translate back to a keyword label (strip leading "; ")
            const std::string label = std::string(pat).substr(2);
            return Violation{
                label,
                pos,
                fmt::format(
                    "AQL_MUTATION_SAFETY: Multi-statement injection pattern '{}' "
                    "detected at position {}. Embedded statements after semicolons "
                    "are not permitted.",
                    pat, pos)
            };
        }
    }

    // --- Unbounded UPDATE check (UPDATE … without FILTER or WHERE) ------------
    {
        const std::size_t updatePos = findKeyword(upper, "UPDATE ");
        if (updatePos != std::string::npos) {
            const bool hasFilter = findKeyword(upper, "FILTER ") != std::string::npos ||
                                   findKeyword(upper, " WHERE ") != std::string::npos ||
                                   findKeyword(upper, "WHERE ")  != std::string::npos;
            if (!hasFilter) {
                return Violation{
                    "UNBOUNDED_UPDATE",
                    updatePos,
                    fmt::format(
                        "AQL_MUTATION_SAFETY: UPDATE at position {} has no FILTER or "
                        "WHERE clause. This could affect the entire collection. "
                        "Add a FILTER/WHERE predicate or acknowledge the risk explicitly.",
                        updatePos)
                };
            }
        }
    }

    // --- Unbounded REMOVE check (REMOVE … without FILTER or WHERE) -----------
    {
        const std::size_t removePos = findKeyword(upper, "REMOVE ");
        if (removePos != std::string::npos) {
            const bool hasFilter = findKeyword(upper, "FILTER ") != std::string::npos ||
                                   findKeyword(upper, " WHERE ") != std::string::npos ||
                                   findKeyword(upper, "WHERE ")  != std::string::npos;
            if (!hasFilter) {
                return Violation{
                    "UNBOUNDED_REMOVE",
                    removePos,
                    fmt::format(
                        "AQL_MUTATION_SAFETY: REMOVE at position {} has no FILTER or "
                        "WHERE clause. This could delete the entire collection. "
                        "Add a FILTER/WHERE predicate or acknowledge the risk explicitly.",
                        removePos)
                };
            }
        }
    }

    // --- Suspiciously large LIMIT check (> 100000) ----------------------------
    {
        std::size_t searchFrom = 0;
        while (true) {
            const std::size_t limitPos = upper.find("LIMIT ", searchFrom);
            if (limitPos == std::string::npos) {
              break;
            }
            searchFrom = limitPos + 6;

            // Skip whitespace after LIMIT
            std::size_t numStart = limitPos + 6;
            while (numStart <static_cast<int>(upper.size()) && upper[numStart] == ' ') {
              ++numStart;
            }

            // Parse the number
            std::size_t numEnd = numStart;
            while (numEnd <static_cast<int>(upper.size()) && std::isdigit(static_cast<unsigned char>(upper[numEnd])))
                ++numEnd;

            if (numEnd > numStart) {
                try {
                    const int64_t limitVal = std::stoll(upper.substr(numStart, numEnd - numStart));
                    if (limitVal > 100000) {
                        return Violation{
                            "LARGE_LIMIT",
                            limitPos,
                            fmt::format(
                                "AQL_MUTATION_SAFETY: LIMIT value {} at position {} "
                                "exceeds the safety threshold of 100000. "
                                "Large LIMIT values in mutation queries may indicate "
                                "bulk-delete or bulk-update attacks.",
                                limitVal, limitPos)
                        };
                    }
                } catch (...) {
                    // Ignore parse failures — not a valid integer, not a threat
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<AqlSafetyValidator::Violation>
AqlSafetyValidator::validate(std::string_view aql_query) const {
    const std::string query_str(aql_query);
    // When mutations are explicitly allowed, skip the keyword-blocking scan
    // but still run injection-pattern and safety checks.
    if (mode_ == ValidationMode::AllowMutations) {
        return validateMutationSafety(query_str);
    }

    const std::string upper = toUpper(query_str);

    // --- Single-keyword scan ------------------------------------------------
    const MutationPattern* first_match = nullptr;
    std::size_t first_pos = std::string::npos;
    for (const auto& pat : kMutationPatterns) {
        const std::string_view needle{pat.keyword};
        const std::size_t pos = findKeyword(upper, needle);
        if ((pos != std::string::npos && (first_match == nullptr || pos < first_pos)) {
            first_match = &pat;
            first_pos = pos;
        }
    }

    if (first_match != nullptr) {
        return Violation{
            first_match->label,
            first_pos,
            fmt::format(
                "AQL_READ_ONLY_VIOLATION: Mutation keyword '{}' detected "
                "at position {} in an enforce_read_only context. "
                "Only read-only AQL (FOR/FILTER/RETURN) is allowed for "
                "this MCP tool. If you need to modify data, use the "
                "explicit write tools (put_entity, delete_entity) which "
                "have a proper approval workflow.",
                first_match->label, first_pos)
        };
    }

    // --- FOR … REMOVE compound pattern -------------------------------------
    // Detect `FOR x IN col REMOVE x IN col` style without an explicit FILTER.
    // This catches full-collection deletes that might bypass the single-keyword
    // scan (e.g. if someone splits the query across lines without a space
    // directly before REMOVE after the last token).
    {
        const std::size_t forPos    = findKeyword(upper, "FOR ");
        const std::size_t removePos = findKeyword(upper, " REMOVE ");
        if (forPos != std::string::npos && removePos != std::string::npos &&
            forPos < removePos) {
            // Extra check: warn even if FILTER is present — REMOVE is
            // explicitly banned in read-only context regardless of scope.
            return Violation{
                "FOR...REMOVE",
                forPos,
                fmt::format(
                    "AQL_READ_ONLY_VIOLATION: Full-collection REMOVE pattern "
                    "'FOR ... REMOVE' detected (FOR at {}, REMOVE at {}) in an "
                    "enforce_read_only context. This pattern can delete entire "
                    "collections and is blocked unconditionally.",
                    forPos, removePos + 1)
            };
        }
    }

    return std::nullopt;  // Query is read-only — safe to execute
}

} // namespace query
} // namespace themis
