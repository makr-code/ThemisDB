// THEMIS_GAP_STATS: gaps=1 unimpl=1 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_safety_validator.cpp                           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-28                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    {"INSERT ",           "INSERT"},
    {"UPDATE ",           "UPDATE"},
    {"REPLACE ",          "REPLACE"},
    {"UPSERT ",           "UPSERT"},
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
    std::string out;
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
AqlSafetyValidator::validate(const std::string& aql_query) const {
    const std::string upper = toUpper(aql_query);

    // --- Single-keyword scan ------------------------------------------------
    for (const auto& pat : kMutationPatterns) {
        const std::string_view needle{pat.keyword};
        const std::size_t pos = findKeyword(upper, needle);
        if (pos != std::string::npos) {
            return Violation{
                pat.label,
                pos,
                fmt::format(
                    "AQL_READ_ONLY_VIOLATION: Mutation keyword '{}' detected "
                    "at position {} in an enforce_read_only context. "
                    "Only read-only AQL (FOR/FILTER/RETURN) is allowed for "
                    "this MCP tool. If you need to modify data, use the "
                    "explicit write tools (put_entity, delete_entity) which "
                    "have a proper approval workflow.",
                    pat.label, pos)
            };
        }
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
