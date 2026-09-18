/**
 * @file aql_query_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_query_validator.h"
#include <stdexcept>
#include "aql/aql_query_builder.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_map>

#include "aql/aql_query_builder.h"

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace aql {

// ============================================================================
// ValidationResult helpers
// ============================================================================

bool ValidationResult::hasErrors() const {
    return std::any_of(issues.begin(), issues.end(),
                       [](const ValidationIssue &i) { return i.severity == ValidationIssue::Severity::ERROR; });
}

bool ValidationResult::hasWarnings() const {
    return std::any_of(issues.begin(), issues.end(),
                       [](const ValidationIssue &i) { return i.severity == ValidationIssue::Severity::WARNING; });
}

std::string ValidationResult::summary() const {
    int errors   = 0;
    int warnings = 0;
    int infos    = 0;
    for (const auto &issue : issues) {
        switch (issue.severity) {
            case ValidationIssue::Severity::ERROR:
                ++errors;
                break;
            case ValidationIssue::Severity::WARNING:
                ++warnings;
                break;
            case ValidationIssue::Severity::INFO:
                ++infos;
                break;
        }
    }
    if (errors == 0 && warnings == 0 && infos == 0) {
        return "OK";
    }
    std::ostringstream oss = {};
    if (errors > 0) {
        oss << errors << " error" << (errors > 1 ? "s" : "");
    }
    if (warnings > 0) {
        if (errors > 0) {
            oss << ", ";
        }
        oss << warnings << " warning" << (warnings > 1 ? "s" : "");
    }
    if (infos > 0) {
        if (errors > 0 || warnings > 0) {
            oss << ", ";
        }
        oss << infos << " hint" << (infos > 1 ? "s" : "");
    }
    return oss.str();
}

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/**
 * @brief Case-insensitive search for a keyword token
 * @param[in] text Input parameter.
 * @param[in] kw Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: std::transform(), begin(), end(), re(), std::regex_search(), spdlog::debug(), find().
 */
bool containsKeyword(const std::string &text, const std::string &kw) {
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
        spdlog::debug("[AQLValidator] regex compile for keyword check failed; using substring fallback");
        return upper_text.find(upper_kw) != std::string::npos;
    }
}

// Extract the variable names declared in FOR clauses
[[maybe_unused]] std::vector<std::string> extractForVariables(const std::string &upper_query) {
    std::vector<std::string> vars;
    std::regex for_re(R"(FOR\s+([A-Za-z_][A-Za-z0-9_]*)\s+IN)", std::regex::icase);
    std::sregex_iterator it(upper_query.begin(), upper_query.end(), for_re);
    std::sregex_iterator end = {};
    for (; it != end; ++it) {
        vars.push_back((*it)[1].str());
    }
    return vars;
}

// Extract LET variable names
[[maybe_unused]] std::vector<std::string> extractLetVariables(const std::string &upper_query) {
    std::vector<std::string> vars;
    std::regex let_re(R"(LET\s+([A-Za-z_][A-Za-z0-9_]*)\s*=)", std::regex::icase);
    std::sregex_iterator it(upper_query.begin(), upper_query.end(), let_re);
    std::sregex_iterator end = {};
    for (; it != end; ++it) {
        vars.push_back((*it)[1].str());
    }
    return vars;
}

/**
 * @brief Check if LIMIT value is 0 (useless)
 * @param[in] query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: lim_re(), std::regex_search(), push_back().
 */
void checkLimitZero(const std::string &query, ValidationResult &result) {
    std::regex lim_re(R"(LIMIT\s+0\b)", std::regex::icase);
    if (std::regex_search(query, lim_re)) {
        result.issues.push_back(
            {ValidationIssue::Severity::WARNING, "LIMIT 0 returns no results; did you mean to omit LIMIT?", "LIMIT"});
    }
}

/**
 * @brief Check for COLLECT placed after SORT (usually a mistake)
 * @param[in] upper_query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: find(), push_back().
 */
void checkCollectAfterSort(const std::string &upper_query, ValidationResult &result) {
    size_t sort_pos    = upper_query.find("SORT");
    size_t collect_pos = upper_query.find("COLLECT");
    if (sort_pos != std::string::npos && collect_pos != std::string::npos && collect_pos > sort_pos) {
        result.issues.push_back({ValidationIssue::Severity::WARNING,
                                 "COLLECT appears after SORT; COLLECT resets the sort order. "
                                 "Consider moving COLLECT before SORT.",
                                 "COLLECT"});
    }
}

/**
 * @brief Warn about missing RETURN when query is expected to be complete
 * @param[in] upper_query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: containsKeyword(), push_back().
 */
void checkMissingReturn(const std::string &upper_query, ValidationResult &result) {
    // DML statements (INSERT, UPDATE, REMOVE, REPLACE, UPSERT) replace RETURN
    bool has_dml = containsKeyword(upper_query, "INSERT") || containsKeyword(upper_query, "UPDATE")
                   || containsKeyword(upper_query, "REMOVE") || containsKeyword(upper_query, "REPLACE")
                   || containsKeyword(upper_query, "UPSERT");
    if (!has_dml && !containsKeyword(upper_query, "RETURN")) {
        result.issues.push_back({ValidationIssue::Severity::ERROR, "Query is missing a RETURN clause", "RETURN"});
        result.is_valid = false;
    }
}

/**
 * @brief Warn about missing FOR when query is expected to be complete
 * @param[in] upper_query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: containsKeyword(), push_back().
 */
void checkMissingFor(const std::string &upper_query, ValidationResult &result) {
    // All standalone DML statements do not require a FOR loop:
    //   INSERT { ... } INTO collection
    //   UPSERT { ... } INSERT { ... } UPDATE { ... } IN collection
    //   REMOVE "key" IN collection
    //   UPDATE "key" WITH { ... } IN collection
    //   REPLACE "key" WITH { ... } IN collection
    bool is_standalone_dml = !containsKeyword(upper_query, "FOR")
                             && (containsKeyword(upper_query, "INSERT") || containsKeyword(upper_query, "UPSERT")
                                 || containsKeyword(upper_query, "REMOVE") || containsKeyword(upper_query, "UPDATE")
                                 || containsKeyword(upper_query, "REPLACE"));
    if (!is_standalone_dml && !containsKeyword(upper_query, "FOR")) {
        result.issues.push_back({ValidationIssue::Severity::ERROR, "Query is missing a FOR clause", "FOR"});
        result.is_valid = false;
    }
}

/**
 * @brief Check that common filter operators use == not = for equality
 * @param[in] query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: eq_re(), std::regex_search(), push_back().
 */
void checkAssignmentInFilter(const std::string &query, ValidationResult &result) {
    // Match FILTER <identifier chain>= (single equals, not ==, !=, <=, >=)
    // e.g. "FILTER u.name = " but not "FILTER u.name == "
    std::regex eq_re(R"(FILTER\s+\w+(?:\.\w+)*\s*=(?![=]))", std::regex::icase);
    if (std::regex_search(query, eq_re)) {
        result.issues.push_back({ValidationIssue::Severity::WARNING,
                                 "FILTER condition may be using '=' (assignment) instead of '==' (equality). "
                                 "Use '==' for equality comparison in AQL.",
                                 "FILTER"});
    }
}

/**
 * @brief Info hint: suggest LIMIT when no LIMIT is set and query could be large
 * @param[in] upper_query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: containsKeyword(), push_back().
 */
void checkMissingLimit(const std::string &upper_query, ValidationResult &result) {
    bool has_for    = containsKeyword(upper_query, "FOR");
    bool has_limit  = containsKeyword(upper_query, "LIMIT");
    bool has_filter = containsKeyword(upper_query, "FILTER");

    if (has_for && !has_limit && !has_filter) {
        result.issues.push_back({ValidationIssue::Severity::INFO,
                                 "Query has no FILTER or LIMIT clause; this may return a very large result set. "
                                 "Consider adding LIMIT for safety.",
                                 "LIMIT"});
    }
}

/**
 * @brief Check that graph traversal depth range is valid (min <= max)
 * @param[in] query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: depth_re(), it(), begin(), end(), std::stoi(), str(), push_back(), std::to_string().
 */
void checkTraversalDepthOrder(const std::string &query, ValidationResult &result) {
    // Match patterns like "IN 3..1 OUTBOUND" or "IN 5..2 ANY"
    static const std::regex depth_re(R"(\bIN\s+(\d+)\.\.(\d+)\s+(?:OUTBOUND|INBOUND|ANY)\b)", std::regex::icase);
    std::sregex_iterator it(query.begin(), query.end(), depth_re);
    std::sregex_iterator end = {};
    for (; it != end; ++it) {
        int min_d = std::stoi((*it)[1].str());
        int max_d = std::stoi((*it)[2].str());
        if (min_d > max_d) {
            result.is_valid = false;
            result.issues.push_back({ValidationIssue::Severity::ERROR,
                                     "Graph traversal min_depth (" + std::to_string(min_d)
                                         + ") is greater than max_depth (" + std::to_string(max_d) + ")",
                                     "FOR"});
        }
    }
}

/**
 * @brief Reject queries whose nested subquery depth exceeds the policy limit (depth > 5).
 * @param[in] query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Each LET ... = (subquery) or FOR inside a LET/FILTER subquery increments depth. The check uses a simple FOR-inside-parenthesis counter; it is conservative and does not require a full parse tree. Calls: std::transform(), begin(), end(), std::toupper(), reserve(), size(), find_first_not_of(), compare().
 */
void checkNestedSubqueryDepth(const std::string &query, ValidationResult &result) {
    constexpr int kMaxAllowedDepth = 5;
    // Track the maximum FOR subquery nesting depth.  A stack records, for each
    // open parenthesis, whether it introduced a subquery (i.e. "(FOR …").  Only
    // the matching ")" of a subquery-opening "(" decrements the depth counter;
    // all other closing parentheses (function calls, grouping expressions, etc.)
    // are ignored for depth accounting.  This prevents false under-counting when
    // function calls appear inside subqueries.
    int max_depth = 0;
    int depth     = 0;
    std::string upper = query;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    // Stack entry: true if the corresponding '(' opened a subquery.
    std::vector<bool> paren_is_subquery;
    paren_is_subquery.reserve(32);
    for (std::size_t pos = 0; pos < upper.size(); ++pos) {
        if (upper[pos] == '(') {
            // Look ahead for a FOR keyword inside this parenthesis.
            std::size_t next = upper.find_first_not_of(" \t\n\r", pos + 1);
            bool is_subquery = (next != std::string::npos && upper.compare(next, 3, "FOR") == 0);
            paren_is_subquery.push_back(is_subquery);
            if (is_subquery) {
                ++depth;
                if (depth > max_depth) max_depth = depth;
            }
        } else if (upper[pos] == ')') {
            if (!paren_is_subquery.empty()) {
                if (paren_is_subquery.back()) {
                    // Closing a subquery-opening parenthesis: decrement depth.
                    if (depth > 0) --depth;
                }
                paren_is_subquery.pop_back();
            }
        }
    }
    if (max_depth > kMaxAllowedDepth) {
        spdlog::debug("[VALIDATION:NestedSubqueryDepthExceeded] Nested subquery depth {} exceeds maximum {}",
                      max_depth, kMaxAllowedDepth);
        result.is_valid = false;
        result.issues.push_back(
            {ValidationIssue::Severity::ERROR,
             "Nested subquery depth " + std::to_string(max_depth)
                 + " exceeds maximum allowed depth " + std::to_string(kMaxAllowedDepth),
             "FOR"});
    }
}

/**
 * @brief Reject queries that reference a collection name longer than 128 characters.
 * @param[in] query Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Oversized names are a signal of LLM hallucination or injection attempts. Calls: coll_re(), it(), begin(), end(), str(), size(), spdlog::debug(), push_back().
 */
void checkCollectionNameLength(const std::string &query, ValidationResult &result) {
    constexpr std::size_t kMaxCollectionNameLength = 128;
    // Match collection names only in FOR/INSERT/UPDATE/REMOVE/UPSERT/REPLACE
    // contexts to avoid false positives on FILTER x IN <array-expression>.
    // Patterns: "FOR <var> IN <coll>", "<DML> [INTO|IN] <coll>"
    try {
        static const std::regex coll_re(
            R"(\b(?:FOR\s+[A-Za-z_][A-Za-z0-9_]*\s+IN|(?:INSERT|UPDATE|REMOVE|UPSERT|REPLACE)\b.*?\b(?:IN(?:TO)?)))"
            R"(\s+([A-Za-z_][A-Za-z0-9_]*))",
            std::regex::icase);
        std::sregex_iterator it(query.begin(), query.end(), coll_re);
        std::sregex_iterator end = {};
        for (; it != end; ++it) {
            const std::string name = (*it)[1].str();
            if (name.size() > kMaxCollectionNameLength) {
                spdlog::debug("[VALIDATION:CollectionNameTooLong] Collection name length {} exceeds maximum {}",
                              name.size(), kMaxCollectionNameLength);
                result.is_valid = false;
                result.issues.push_back(
                    {ValidationIssue::Severity::ERROR,
                     "Collection name '" + name.substr(0, 32) + "...' length "
                         + std::to_string(name.size())
                         + " exceeds maximum allowed length "
                         + std::to_string(kMaxCollectionNameLength),
                     "FOR"});
            }
        }
    } catch (...) {
        spdlog::debug("[VALIDATION:CollectionNameTooLong] regex execute for collection name check failed; skipping");
    }
}

} // anonymous namespace

// ============================================================================
// AQLQueryValidator::validate(string)
// ============================================================================

ValidationResult AQLQueryValidator::validate(const std::string &query) const {
    ValidationResult result;
    result.is_valid = true;

    if (query.empty()) {
        result.is_valid = false;
        result.issues.push_back({ValidationIssue::Severity::ERROR, "Query string is empty", ""});
        return result;
    }

    checkMissingFor(query, result);
    checkMissingReturn(query, result);
    checkLimitZero(query, result);
    checkCollectAfterSort(query, result);
    checkAssignmentInFilter(query, result);
    checkMissingLimit(query, result);
    checkTraversalDepthOrder(query, result);
    checkNestedSubqueryDepth(query, result);
    checkCollectionNameLength(query, result);

    return result;
}

// ============================================================================
// AQLQueryValidator::validate(AQLQueryBuilder)
// ============================================================================

ValidationResult AQLQueryValidator::validate(const AQLQueryBuilder &builder) const {
    ValidationResult result;
    result.is_valid = true;

    // Leverage the builder's own state checks first
    if (!builder.isValid()) {
        result.is_valid = false;
        result.issues.push_back({ValidationIssue::Severity::ERROR,
                                 "Builder state is invalid: clauses require a preceding FOR clause", "FOR"});
        return result;
    }

    // Delegate to string validation on the partial query
    std::string partial = builder.getPartialQuery();
    if (!partial.empty()) {
        // Only run the checks that are meaningful on a partial (possibly incomplete) query
        checkLimitZero(partial, result);
        checkCollectAfterSort(partial, result);
        checkAssignmentInFilter(partial, result);
        checkTraversalDepthOrder(partial, result);
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

// ============================================================================
// AQLQueryValidator::checkUnknownCollections (private)
// ============================================================================

void AQLQueryValidator::checkUnknownCollections(const std::string &query, const std::vector<CollectionMetadata> &schema,
                                                ValidationResult &result) const {
    if (schema.empty()) {
        return;
    }

    // Extract collection names from FOR x IN <collection> clauses
    static const std::regex for_in_re(R"(\bFOR\s+[A-Za-z_][A-Za-z0-9_]*\s+IN\s+([A-Za-z_][A-Za-z0-9_]*))",
                                      std::regex::icase);

    std::sregex_iterator it(query.begin(), query.end(), for_in_re);
    std::sregex_iterator end_it = {};

    for (; it != end_it; ++it) {
        std::string collection_name = (*it)[1].str();

        // Check if the collection exists in the schema (case-insensitive)
        bool found = std::any_of(schema.begin(), schema.end(), [&collection_name](const CollectionMetadata &meta) {
            std::string a = meta.name, b = collection_name;
            std::transform(a.begin(), a.end(), a.begin(), ::tolower);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);
            return a == b;
        });

        if (!found) {
            result.issues.push_back({ValidationIssue::Severity::WARNING,
                                     "Collection '" + collection_name + "' is not present in the schema", "FOR"});
        }
    }
}

// ============================================================================
// AQLQueryValidator::checkUnknownFields (private)
// ============================================================================

void AQLQueryValidator::checkUnknownFields(const std::string &query, const std::vector<CollectionMetadata> &schema,
                                           ValidationResult &result) const {
    if (schema.empty()) {
        return;
    }

    // Build a map: FOR variable -> collection name
    static const std::regex for_in_re(R"(\bFOR\s+([A-Za-z_][A-Za-z0-9_]*)\s+IN\s+([A-Za-z_][A-Za-z0-9_]*))",
                                      std::regex::icase);
    std::unordered_map<std::string, std::string> var_to_collection;
    {
        std::sregex_iterator it(query.begin(), query.end(), for_in_re);
        std::sregex_iterator end_it = {};
        for (; it != end_it; ++it) {
            std::string var = (*it)[1].str();
            std::string col = (*it)[2].str();
            std::transform(var.begin(), var.end(), var.begin(), ::tolower);
            std::transform(col.begin(), col.end(), col.begin(), ::tolower);
            var_to_collection[var] = col;
        }
    }

    if (var_to_collection.empty()) {
        return;
    }

    // Build a map: collection name (lower) -> set of known field names (lower)
    std::unordered_map<std::string, std::vector<std::string>> collection_fields;
    for (const auto &meta : schema) {
        std::string col_lower = meta.name;
        std::transform(col_lower.begin(), col_lower.end(), col_lower.begin(), ::tolower);
        for (const auto &f : meta.fields) {
            std::string field_lower = f.name;
            std::transform(field_lower.begin(), field_lower.end(), field_lower.begin(), ::tolower);
            collection_fields[col_lower].push_back(field_lower);
        }
    }

    // Extract <var>.<field> accesses
    static const std::regex field_access_re(R"(\b([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z_][A-Za-z0-9_]*))",
                                            std::regex::icase);
    std::sregex_iterator it(query.begin(), query.end(), field_access_re);
    std::sregex_iterator end_it = {};

    std::set<std::string> already_warned; // avoid duplicate warnings

    for (; it != end_it; ++it) {
        std::string var_orig   = (*it)[1].str();
        std::string field_orig = (*it)[2].str();

        std::string var   = var_orig;
        std::string field = field_orig;
        std::transform(var.begin(), var.end(), var.begin(), ::tolower);
        std::transform(field.begin(), field.end(), field.begin(), ::tolower);

        auto var_it = var_to_collection.find(var);
        if (var_it == var_to_collection.end()) {
            continue; // not a known loop variable
        }

        const std::string &col = var_it->second;
        auto col_it            = collection_fields.find(col);
        if (col_it == collection_fields.end()) {
            continue; // collection not in schema
        }

        const auto &known_fields = col_it->second;
        if (std::find(known_fields.begin(), known_fields.end(), field) == known_fields.end()) {
            std::string warn_key = var + "." + field;
            if (already_warned.insert(warn_key).second) {
                result.issues.push_back({ValidationIssue::Severity::WARNING,
                                         "Field '" + field_orig + "' is not a known field of collection '"
                                             + var_it->second + "' (accessed as '" + var_orig + "." + field_orig + "')",
                                         "FILTER"});
            }
        }
    }
}

// ============================================================================
// AQLQueryValidator::validate(string, schema)
// ============================================================================

ValidationResult AQLQueryValidator::validate(const std::string &query,
                                             const std::vector<CollectionMetadata> &schema) const {
    // Run all standard structural checks first
    ValidationResult result = validate(query);

    // Then apply schema-aware checks
    checkUnknownCollections(query, schema, result);
    checkUnknownFields(query, schema, result);

    return result;
}

// ============================================================================
// AQLQueryValidator::validate(AQLQueryBuilder, schema)
// ============================================================================

ValidationResult AQLQueryValidator::validate(const AQLQueryBuilder &builder,
                                             const std::vector<CollectionMetadata> &schema) const {
    // Run all structural builder checks first (same as validate(builder))
    ValidationResult result = validate(builder);

    // Then apply schema-aware checks against the provided schema snapshot.
    // The partial query is used because the builder may be incomplete; only
    // collections and field accesses already present are checked.
    const std::string partial = builder.getPartialQuery();
    if (!partial.empty()) {
        checkUnknownCollections(partial, schema, result);
        checkUnknownFields(partial, schema, result);
    }

    return result;
}

} // namespace aql
} // namespace themis

