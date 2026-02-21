/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_query_validator.cpp                            ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 16:52:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     283                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "aql/aql_query_validator.h"
#include "aql/aql_query_builder.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>

namespace themis {
namespace aql {

// ============================================================================
// ValidationResult helpers
// ============================================================================

bool ValidationResult::hasErrors() const {
    return std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& i) {
        return i.severity == ValidationIssue::Severity::ERROR;
    });
}

bool ValidationResult::hasWarnings() const {
    return std::any_of(issues.begin(), issues.end(), [](const ValidationIssue& i) {
        return i.severity == ValidationIssue::Severity::WARNING;
    });
}

std::string ValidationResult::summary() const {
    int errors   = 0;
    int warnings = 0;
    int infos    = 0;
    for (const auto& issue : issues) {
        switch (issue.severity) {
            case ValidationIssue::Severity::ERROR:   ++errors;   break;
            case ValidationIssue::Severity::WARNING: ++warnings; break;
            case ValidationIssue::Severity::INFO:    ++infos;    break;
        }
    }
    if (errors == 0 && warnings == 0 && infos == 0) {
        return "OK";
    }
    std::ostringstream oss;
    if (errors   > 0) oss << errors   << " error"   << (errors   > 1 ? "s" : "");
    if (warnings > 0) {
        if (errors > 0) oss << ", ";
        oss << warnings << " warning" << (warnings > 1 ? "s" : "");
    }
    if (infos > 0) {
        if (errors > 0 || warnings > 0) oss << ", ";
        oss << infos << " hint" << (infos > 1 ? "s" : "");
    }
    return oss.str();
}

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

// Case-insensitive search for a keyword token
bool containsKeyword(const std::string& text, const std::string& kw) {
    std::string upper_text = text;
    std::transform(upper_text.begin(), upper_text.end(), upper_text.begin(), ::toupper);
    std::string upper_kw = kw;
    std::transform(upper_kw.begin(), upper_kw.end(), upper_kw.begin(), ::toupper);
    // Match keyword as a whole word (surrounded by non-alphanumeric chars or string boundaries).
    // NOTE: kw is always one of the known AQL keywords (FOR, FILTER, SORT, LIMIT, RETURN,
    // COLLECT) — all uppercase ASCII letters with no regex metacharacters — so the pattern
    // is always valid without escaping.
    std::string pattern = "(?:^|[^A-Z0-9_])" + upper_kw + "(?:$|[^A-Z0-9_])";
    try {
        std::regex re(pattern);
        return std::regex_search(upper_text, re);
    } catch (...) {
        // Fallback: simple substring search
        return upper_text.find(upper_kw) != std::string::npos;
    }
}

// Extract the variable names declared in FOR clauses
std::vector<std::string> extractForVariables(const std::string& upper_query) {
    std::vector<std::string> vars;
    std::regex for_re(R"(FOR\s+([A-Za-z_][A-Za-z0-9_]*)\s+IN)", std::regex::icase);
    std::sregex_iterator it(upper_query.begin(), upper_query.end(), for_re);
    std::sregex_iterator end;
    for (; it != end; ++it) {
        vars.push_back((*it)[1].str());
    }
    return vars;
}

// Extract LET variable names
std::vector<std::string> extractLetVariables(const std::string& upper_query) {
    std::vector<std::string> vars;
    std::regex let_re(R"(LET\s+([A-Za-z_][A-Za-z0-9_]*)\s*=)", std::regex::icase);
    std::sregex_iterator it(upper_query.begin(), upper_query.end(), let_re);
    std::sregex_iterator end;
    for (; it != end; ++it) {
        vars.push_back((*it)[1].str());
    }
    return vars;
}

// Check if LIMIT value is 0 (useless)
void checkLimitZero(const std::string& query, ValidationResult& result) {
    std::regex lim_re(R"(LIMIT\s+0\b)", std::regex::icase);
    if (std::regex_search(query, lim_re)) {
        result.issues.push_back({
            ValidationIssue::Severity::WARNING,
            "LIMIT 0 returns no results; did you mean to omit LIMIT?",
            "LIMIT"
        });
    }
}

// Check for COLLECT placed after SORT (usually a mistake)
void checkCollectAfterSort(const std::string& upper_query, ValidationResult& result) {
    size_t sort_pos    = upper_query.find("SORT");
    size_t collect_pos = upper_query.find("COLLECT");
    if (sort_pos != std::string::npos && collect_pos != std::string::npos
        && collect_pos > sort_pos) {
        result.issues.push_back({
            ValidationIssue::Severity::WARNING,
            "COLLECT appears after SORT; COLLECT resets the sort order. "
            "Consider moving COLLECT before SORT.",
            "COLLECT"
        });
    }
}

// Warn about missing RETURN when query is expected to be complete
void checkMissingReturn(const std::string& upper_query, ValidationResult& result) {
    if (!containsKeyword(upper_query, "RETURN")) {
        result.issues.push_back({
            ValidationIssue::Severity::ERROR,
            "Query is missing a RETURN clause",
            "RETURN"
        });
        result.is_valid = false;
    }
}

// Warn about missing FOR when query is expected to be complete
void checkMissingFor(const std::string& upper_query, ValidationResult& result) {
    if (!containsKeyword(upper_query, "FOR")) {
        result.issues.push_back({
            ValidationIssue::Severity::ERROR,
            "Query is missing a FOR clause",
            "FOR"
        });
        result.is_valid = false;
    }
}

// Check that common filter operators use == not = for equality
void checkAssignmentInFilter(const std::string& query, ValidationResult& result) {
    // Match FILTER <identifier chain> = (single equals, not ==, !=, <=, >=)
    // e.g. "FILTER u.name = " but not "FILTER u.name == "
    std::regex eq_re(R"(FILTER\s+\w+(?:\.\w+)*\s*=(?![=]))", std::regex::icase);
    if (std::regex_search(query, eq_re)) {
        result.issues.push_back({
            ValidationIssue::Severity::WARNING,
            "FILTER condition may be using '=' (assignment) instead of '==' (equality). "
            "Use '==' for equality comparison in AQL.",
            "FILTER"
        });
    }
}

// Info hint: suggest LIMIT when no LIMIT is set and query could be large
void checkMissingLimit(const std::string& upper_query, ValidationResult& result) {
    bool has_for    = containsKeyword(upper_query, "FOR");
    bool has_limit  = containsKeyword(upper_query, "LIMIT");
    bool has_filter = containsKeyword(upper_query, "FILTER");

    if (has_for && !has_limit && !has_filter) {
        result.issues.push_back({
            ValidationIssue::Severity::INFO,
            "Query has no FILTER or LIMIT clause; this may return a very large result set. "
            "Consider adding LIMIT for safety.",
            "LIMIT"
        });
    }
}

} // anonymous namespace

// ============================================================================
// AQLQueryValidator::validate(string)
// ============================================================================

ValidationResult AQLQueryValidator::validate(const std::string& query) const {
    ValidationResult result;
    result.is_valid = true;

    if (query.empty()) {
        result.is_valid = false;
        result.issues.push_back({
            ValidationIssue::Severity::ERROR,
            "Query string is empty",
            ""
        });
        return result;
    }

    checkMissingFor(query, result);
    checkMissingReturn(query, result);
    checkLimitZero(query, result);
    checkCollectAfterSort(query, result);
    checkAssignmentInFilter(query, result);
    checkMissingLimit(query, result);

    return result;
}

// ============================================================================
// AQLQueryValidator::validate(AQLQueryBuilder)
// ============================================================================

ValidationResult AQLQueryValidator::validate(const AQLQueryBuilder& builder) const {
    ValidationResult result;
    result.is_valid = true;

    // Leverage the builder's own state checks first
    if (!builder.isValid()) {
        result.is_valid = false;
        result.issues.push_back({
            ValidationIssue::Severity::ERROR,
            "Builder state is invalid: clauses require a preceding FOR clause",
            "FOR"
        });
        return result;
    }

    // Delegate to string validation on the partial query
    std::string partial = builder.getPartialQuery();
    if (!partial.empty()) {
        // Only run the checks that are meaningful on a partial (possibly incomplete) query
        checkLimitZero(partial, result);
        checkCollectAfterSort(partial, result);
        checkAssignmentInFilter(partial, result);
    }

    // If the builder claims to be complete, also enforce FOR + RETURN
    if (builder.isComplete()) {
        std::string current_query = builder.getPartialQuery();
        checkMissingFor(current_query, result);
        checkMissingReturn(current_query, result);
        checkMissingLimit(current_query, result);
    }

    return result;
}

} // namespace aql
} // namespace themis
