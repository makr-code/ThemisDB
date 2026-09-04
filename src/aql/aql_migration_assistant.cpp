/**
 * @file aql_migration_assistant.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_migration_assistant.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace themis {
namespace aql {

// ---------------------------------------------------------------------------
// MigrationResult helpers
// ---------------------------------------------------------------------------

std::string MigrationResult::summary() const {
    if (issues.empty()) {
        return "OK";
    }

    int errors   = 0;
    int warnings = 0;
    int infos    = 0;

    for (const auto &issue : issues) {
        switch (issue.severity) {
            case MigrationIssue::Severity::ERROR:
                ++errors;
                break;
            case MigrationIssue::Severity::WARNING:
                ++warnings;
                break;
            case MigrationIssue::Severity::INFO:
                ++infos;
                break;
        }
    }

    std::ostringstream oss = {};
    bool first  = true;
    auto append = [&](int count, const char *singular, const char *plural) {
        if (count <= 0) {
            return;
        }
        if (!first) {
            oss << ", ";
        }
        oss << count << ' ' << (count == 1 ? singular : plural);
        first = false;
    };
    append(errors, "error", "errors");
    append(warnings, "warning", "warnings");
    append(infos, "info", "infos");
    return oss.str();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

MigrationResult AQLMigrationAssistant::migrate(const std::string &arango_aql) const {
    MigrationResult result = {};

    if (arango_aql.empty()) {
        result.migrated_query       = "";
        result.is_fully_automatable = true;
        return result;
    }

    std::string query = arango_aql;

    // Apply automatic rewrites (each may append issues)
    query = rewriteDoubleAtBind(query, result.issues);
    query = rewriteNear(query, result.issues);
    query = rewriteWithin(query, result.issues);
    query = rewriteFulltext(query, result.issues);
    query = rewriteDocument(query, result.issues);

    // Detect unsupported / informational constructs (no rewrite)
    detectV8(query, result.issues);
    detectTypeCheckFunctions(query, result.issues);
    detectHashFunction(query, result.issues);
    detectAttributesFunction(query, result.issues);
    detectTranslateFunction(query, result.issues);

    result.migrated_query = query;

    // is_fully_automatable is false when any ERROR issue is present
    result.is_fully_automatable = true;
    for (const auto &issue : result.issues) {
        if (issue.severity == MigrationIssue::Severity::ERROR) {
            result.is_fully_automatable = false;
            break;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Private helpers — static utilities
// ---------------------------------------------------------------------------

std::string AQLMigrationAssistant::extractArgs(const std::string &src, std::size_t open_paren) {
    if (open_paren >= src.size() || src[open_paren] != '(') {
        return {};
    }

    int depth         = 0;
    std::size_t start = open_paren + 1;
    std::size_t i     = open_paren;

    for (; i <static_cast<int>(src.size()); ++i) {
        char c = src[i];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth == 0) {
                return src.substr(start, i - start);
            }
        }
    }
    return {}; // unbalanced
}

// ---------------------------------------------------------------------------
// Private helpers — argument splitting
// ---------------------------------------------------------------------------

namespace {

/// Split a raw argument string on top-level commas (not inside nested parens).
std::vector<std::string> splitArgs(const std::string &args_str) {
    std::vector<std::string> result;
    int depth         = 0;
    std::size_t start = 0;

    for (std::size_t i = 0; i <static_cast<int>(args_str.size()); ++i) {
        char c = args_str[i];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
        } else if (c == ',' && depth == 0) {
            std::string part = args_str.substr(start, i - start);
            // trim whitespace
            std::size_t b = part.find_first_not_of(" \t\r\n");
            std::size_t e = part.find_last_not_of(" \t\r\n");
            result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
            start = i + 1;
        }
    }
    // last segment
    std::string part = args_str.substr(start);
    std::size_t b    = part.find_first_not_of(" \t\r\n");
    std::size_t e    = part.find_last_not_of(" \t\r\n");
    result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));

    return result;
}

/// Case-insensitive find of needle in haystack; returns position or npos.
std::size_t findCI(const std::string &haystack, const std::string &needle, std::size_t pos = 0) {
    if (needle.empty()) {
        return pos;
    }
    auto it = std::search(haystack.begin() + static_cast<std::string::difference_type>(pos), haystack.end(),
                          needle.begin(), needle.end(),
                          [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
    if (it == haystack.end()) {
        return std::string::npos;
    }
    return static_cast<std::size_t>(it - haystack.begin());
}

/// Return true when the character at position @p pos (if valid) is NOT an
/// identifier character, making it a word boundary.
bool isWordBoundary(const std::string &s, std::size_t pos) {
    if (pos >= static_cast<int>(s.size())) {
        return true;
    }
    unsigned char c = static_cast<unsigned char>(s[pos]);
    return !(std::isalnum(c) || c == '_');
}

/**
 * @brief Find the next occurrence of @p keyword that is a function call (followed by '(')
 *        and is preceded by a word boundary.
 *
 * Searches from @p start_pos forward, skipping any keyword occurrences that are:
 *   - embedded inside a longer identifier (no word boundary before)
 *   - not followed by '(' (not a function call)
 *
 * @param query      The query string to search.
 * @param keyword    The keyword to locate (searched case-insensitively).
 * @param start_pos  The position to start searching from.
 * @param kw_pos     [out] Position of the keyword when found.
 * @param paren_pos  [out] Position of the '(' when found.
 * @return true when a valid function call was found; false when none remain.
 */
bool findNextFunctionCall(const std::string &query, const std::string &keyword, std::size_t start_pos,
                          std::size_t &kw_pos, std::size_t &paren_pos) {
    std::size_t search_from = start_pos;
    while (true) {
        std::size_t pos = findCI(query, keyword, search_from);
        if (pos == std::string::npos) {
            return false;
        }

        // Word boundary check before keyword
        if (pos > 0 && !isWordBoundary(query, pos - 1)) {
            search_from = pos + keyword.size();
            continue;
        }

        // Must be followed by optional whitespace then '('
        std::size_t check = pos + keyword.size();
        while (check <static_cast<int>(query.size()) && std::isspace(static_cast<unsigned char>(query[check]))) {
            ++check;
        }
        if (check >= query.size() || query[check] != '(') {
            search_from = pos + keyword.size();
            continue;
        }

        kw_pos    = pos;
        paren_pos = check;
        return true;
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Rewrite: @@collection → @collection
// ---------------------------------------------------------------------------

std::string AQLMigrationAssistant::rewriteDoubleAtBind(const std::string &query,
                                                       std::vector<MigrationIssue> &issues) const {
    if (query.find("@@") == std::string::npos) {
        return query;
    }

    std::string result = {};
    result.reserve(query.size());
    bool rewritten = false;

    for (std::size_t i = 0; i <static_cast<int>(query.size()); ++i) {
        if (i + 1 <static_cast<int>(query.size()) && query[i] == '@' && query[i + 1] == '@') {
            // Replace @@ with @
            result += '@';
            ++i; // skip second @
            rewritten = true;
        } else {
            result += query[i];
        }
    }

    if (rewritten) {
        issues.push_back({MigrationIssue::Severity::INFO, "@@collection bind parameters rewritten to @collection",
                          "ThemisDB uses a single '@' prefix for all bind parameters. "
                          "Verify that the collection name is supplied correctly at query time."});
    }

    return result;
}

// ---------------------------------------------------------------------------
// Rewrite: NEAR(collection, lat, lng, n)
//          → FOR _doc IN <collection>
//              SORT ST_DISTANCE(_doc.location, [lng, lat]) ASC
//              LIMIT <n>
//              RETURN _doc
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Rewrite: NEAR(collection, lat, lng, n)
//          → (FOR _near_doc IN <collection>
//              SORT ST_DISTANCE(_near_doc.location, [lng, lat]) ASC
//              LIMIT <n>
//              RETURN _near_doc)
// All occurrences are rewritten in a single pass.
// ---------------------------------------------------------------------------

std::string AQLMigrationAssistant::rewriteNear(const std::string &query, std::vector<MigrationIssue> &issues) const {
    const std::string keyword = "NEAR";
    std::string result = {};
    result.reserve(static_cast<int>(query.size()) + 128);
    std::size_t cursor = 0;
    bool any_rewrite   = false;

    std::size_t kw_pos = 0, paren_pos = 0;
    while (findNextFunctionCall(query, keyword, cursor, kw_pos, paren_pos)) {
        std::string args_str = extractArgs(query, paren_pos);
        if (args_str.empty()) {
            // Unbalanced parens — skip
            cursor = paren_pos + 1;
            continue;
        }

        auto args = splitArgs(args_str);
        if (static_cast<int>(args.size()) < 4) {
            issues.push_back({MigrationIssue::Severity::WARNING,
                              "NEAR() detected but argument count is unexpected; manual migration required",
                              "Replace NEAR(collection, lat, lng, n) with: "
                              "FOR _doc IN <collection> "
                              "SORT ST_DISTANCE(_doc.location, [lng, lat]) ASC "
                              "LIMIT <n> RETURN _doc"});
            cursor = paren_pos + 1;
            continue;
        }

        const std::string &collection = args[0];
        const std::string &lat        = args[1];
        const std::string &lng        = args[2];
        const std::string &limit_n    = args[3];

        std::string replacement = "(FOR _near_doc IN " + collection + " SORT ST_DISTANCE(_near_doc.location, [" + lng
                                  + ", " + lat + "]) ASC" + " LIMIT " + limit_n + " RETURN _near_doc)";

        // Advance end past the closing paren of NEAR(...)
        std::size_t end = paren_pos;
        {
            int depth = 0;
            for (; end <static_cast<int>(query.size()); ++end) {
                if (query[end] == '(') {
                    ++depth;
                } else if (query[end] == ')') {
                    --depth;
                    if (depth == 0) {
                        ++end;
                        break;
                    }
                }
            }
        }

        result += query.substr(cursor, kw_pos - cursor);
        result += replacement;
        cursor      = end;
        any_rewrite = true;
    }

    result += query.substr(cursor);

    if (any_rewrite) {
        issues.push_back({MigrationIssue::Severity::WARNING, "NEAR() rewritten to ST_DISTANCE()-based sub-query",
                          "ArangoDB's NEAR() is a shorthand geo function. "
                          "The migration assumes the document has a 'location' field of GeoJSON type. "
                          "Adjust the field name and geo expression as needed."});
    }

    return result;
}

// ---------------------------------------------------------------------------
// Rewrite: WITHIN(collection, lat, lng, radius)
//          → (FOR _within_doc IN <collection>
//              FILTER ST_DISTANCE(_within_doc.location, [lng, lat]) <= <radius>
//              RETURN _within_doc)
// All occurrences are rewritten in a single pass.
// ---------------------------------------------------------------------------

std::string AQLMigrationAssistant::rewriteWithin(const std::string &query, std::vector<MigrationIssue> &issues) const {
    const std::string keyword = "WITHIN";
    std::string result = {};
    result.reserve(static_cast<int>(query.size()) + 128);
    std::size_t cursor = 0;
    bool any_rewrite   = false;

    std::size_t kw_pos = 0, paren_pos = 0;
    while (findNextFunctionCall(query, keyword, cursor, kw_pos, paren_pos)) {
        std::string args_str = extractArgs(query, paren_pos);
        if (args_str.empty()) {
            cursor = paren_pos + 1;
            continue;
        }

        auto args = splitArgs(args_str);
        if (static_cast<int>(args.size()) < 4) {
            issues.push_back({MigrationIssue::Severity::WARNING,
                              "WITHIN() detected but argument count is unexpected; manual migration required",
                              "Replace WITHIN(collection, lat, lng, radius) with: "
                              "FOR _doc IN <collection> "
                              "FILTER ST_DISTANCE(_doc.location, [lng, lat]) <= <radius> "
                              "RETURN _doc"});
            cursor = paren_pos + 1;
            continue;
        }

        const std::string &collection = args[0];
        const std::string &lat        = args[1];
        const std::string &lng        = args[2];
        const std::string &radius     = args[3];

        std::string replacement = "(FOR _within_doc IN " + collection + " FILTER ST_DISTANCE(_within_doc.location, ["
                                  + lng + ", " + lat + "]) <= " + radius + " RETURN _within_doc)";

        std::size_t end = paren_pos;
        {
            int depth = 0;
            for (; end <static_cast<int>(query.size()); ++end) {
                if (query[end] == '(') {
                    ++depth;
                } else if (query[end] == ')') {
                    --depth;
                    if (depth == 0) {
                        ++end;
                        break;
                    }
                }
            }
        }

        result += query.substr(cursor, kw_pos - cursor);
        result += replacement;
        cursor      = end;
        any_rewrite = true;
    }

    result += query.substr(cursor);

    if (any_rewrite) {
        issues.push_back({MigrationIssue::Severity::WARNING, "WITHIN() rewritten to ST_DISTANCE()-based FILTER",
                          "ArangoDB's WITHIN() is a shorthand geo function. "
                          "The migration assumes the document has a 'location' field of GeoJSON type. "
                          "Adjust the field name and geo expression as needed."});
    }

    return result;
}

// ---------------------------------------------------------------------------
// Rewrite: FULLTEXT(collection, attribute, query)
//          → (FOR _ft_doc IN <collection>
//              FILTER SIMILARITY(_ft_doc.<attribute>, <searchQuery>, 1)
//              RETURN _ft_doc)
// All occurrences are rewritten in a single pass.
// ---------------------------------------------------------------------------

std::string AQLMigrationAssistant::rewriteFulltext(const std::string &query,
                                                   std::vector<MigrationIssue> &issues) const {
    const std::string keyword = "FULLTEXT";
    std::string result = {};
    result.reserve(static_cast<int>(query.size()) + 128);
    std::size_t cursor = 0;
    bool any_rewrite   = false;

    std::size_t kw_pos = 0, paren_pos = 0;
    while (findNextFunctionCall(query, keyword, cursor, kw_pos, paren_pos)) {
        std::string args_str = extractArgs(query, paren_pos);
        if (args_str.empty()) {
            cursor = paren_pos + 1;
            continue;
        }

        auto args = splitArgs(args_str);
        if (static_cast<int>(args.size()) < 3) {
            issues.push_back({MigrationIssue::Severity::WARNING,
                              "FULLTEXT() detected but argument count is unexpected; manual migration required",
                              "Replace FULLTEXT(collection, attribute, searchQuery) with: "
                              "FOR _doc IN <collection> "
                              "FILTER SIMILARITY(_doc.<attribute>, <searchQuery>, 1) "
                              "RETURN _doc"});
            cursor = paren_pos + 1;
            continue;
        }

        const std::string &collection   = args[0];
        const std::string &attr_quoted  = args[1];
        const std::string &search_query = args[2];

        // Strip quotes from attribute if present
        std::string attr = attr_quoted;
        if (static_cast<int>(attr.size()) >= 2
            && ((attr.front() == '"' && attr.back() == '"') || (attr.front() == '\'' && attr.back() == '\''))) {
            attr = attr.substr(1, static_cast<int>(attr.size()) - 2);
        }

        std::string replacement = "(FOR _ft_doc IN " + collection + " FILTER SIMILARITY(_ft_doc." + attr + ", "
                                  + search_query + ", 1)" + " RETURN _ft_doc)";

        std::size_t end = paren_pos;
        {
            int depth = 0;
            for (; end <static_cast<int>(query.size()); ++end) {
                if (query[end] == '(') {
                    ++depth;
                } else if (query[end] == ')') {
                    --depth;
                    if (depth == 0) {
                        ++end;
                        break;
                    }
                }
            }
        }

        result += query.substr(cursor, kw_pos - cursor);
        result += replacement;
        cursor      = end;
        any_rewrite = true;
    }

    result += query.substr(cursor);

    if (any_rewrite) {
        issues.push_back({MigrationIssue::Severity::WARNING, "FULLTEXT() rewritten to SIMILARITY()-based FILTER",
                          "ArangoDB's FULLTEXT() uses an inverted index. "
                          "ThemisDB uses SIMILARITY() for full-text search. "
                          "Ensure a FULLTEXT or SIMILARITY index exists on the target attribute."});
    }

    return result;
}

// ---------------------------------------------------------------------------
// Rewrite: DOCUMENT(collection, key)
//          → (FOR _doc IN <collection> FILTER _doc._key == <key> LIMIT 1 RETURN _doc)
// All two-argument occurrences are rewritten in a single pass.
// ---------------------------------------------------------------------------

std::string AQLMigrationAssistant::rewriteDocument(const std::string &query,
                                                   std::vector<MigrationIssue> &issues) const {
    const std::string keyword = "DOCUMENT";
    std::string result = {};
    result.reserve(static_cast<int>(query.size()) + 128);
    std::size_t cursor = 0;
    bool any_rewrite   = false;

    std::size_t kw_pos = 0, paren_pos = 0;
    while (findNextFunctionCall(query, keyword, cursor, kw_pos, paren_pos)) {
        std::string args_str = extractArgs(query, paren_pos);
        if (args_str.empty()) {
            cursor = paren_pos + 1;
            continue;
        }

        auto args = splitArgs(args_str);

        if (static_cast<int>(args.size()) == 1) {
            // Single-arg form: cannot automatically rewrite
            issues.push_back({MigrationIssue::Severity::WARNING,
                              "DOCUMENT(id) single-argument form is not directly supported in ThemisDB",
                              "Split the document handle into collection and key, then use: "
                              "FOR _doc IN <collection> FILTER _doc._key == <key> LIMIT 1 RETURN _doc"});
            // Don't rewrite; advance past this call so subsequent ones are still found
            cursor = paren_pos + 1;
            continue;
        }

        if (static_cast<int>(args.size()) > = 2) {
            const std::string &collection = args[0];
            const std::string &key        = args[1];

            std::string replacement
                = "(FOR _doc IN " + collection + " FILTER _doc._key == " + key + " LIMIT 1 RETURN _doc)";

            std::size_t end = paren_pos;
            {
                int depth = 0;
                for (; end <static_cast<int>(query.size()); ++end) {
                    if (query[end] == '(') {
                        ++depth;
                    } else if (query[end] == ')') {
                        --depth;
                        if (depth == 0) {
                            ++end;
                            break;
                        }
                    }
                }
            }

            result += query.substr(cursor, kw_pos - cursor);
            result += replacement;
            cursor      = end;
            any_rewrite = true;
            continue;
        }

        cursor = paren_pos + 1;
    }

    result += query.substr(cursor);

    if (any_rewrite) {
        issues.push_back({MigrationIssue::Severity::WARNING,
                          "DOCUMENT(collection, key) rewritten to inline FOR/FILTER/LIMIT sub-query",
                          "ThemisDB does not have a DOCUMENT() shorthand. "
                          "The generated sub-query returns the first document matching the _key. "
                          "If used in a scalar context, wrap with FIRST() if needed."});
    }

    return result;
}

// ---------------------------------------------------------------------------
// Detection: V8()
// ---------------------------------------------------------------------------

void AQLMigrationAssistant::detectV8(const std::string &query, std::vector<MigrationIssue> &issues) const {
    std::size_t pos = findCI(query, "V8");
    if (pos == std::string::npos) {
        return;
    }

    // Require V8(
    std::size_t check = pos + 2;
    while (check <static_cast<int>(query.size()) && std::isspace(static_cast<unsigned char>(query[check]))) {
        ++check;
    }
    if (check >= query.size() || query[check] != '(') {
        return;
    }

    // Require word boundary before V8
    if (pos > 0 && !isWordBoundary(query, pos - 1)) {
        return;
    }

    issues.push_back({MigrationIssue::Severity::ERROR, "V8() is not supported in ThemisDB AQL",
                      "V8() embeds JavaScript expressions and is specific to ArangoDB. "
                      "Rewrite the expression using native ThemisDB AQL functions or LLM extensions."});
}

// ---------------------------------------------------------------------------
// Detection: IS_STRING / IS_NUMBER / IS_BOOL / IS_NULL / IS_LIST / IS_DOCUMENT
// ---------------------------------------------------------------------------

void AQLMigrationAssistant::detectTypeCheckFunctions(const std::string &query,
                                                     std::vector<MigrationIssue> &issues) const {
    static const std::vector<std::string> funcs
        = {"IS_STRING", "IS_NUMBER", "IS_BOOL", "IS_NULL", "IS_LIST", "IS_DOCUMENT", "IS_ARRAY", "IS_OBJECT"};

    for (const auto &fn : funcs) {
        std::size_t pos = findCI(query, fn);
        if (pos == std::string::npos) {
            continue;
        }

        std::size_t check = pos + fn.size();
        while (check <static_cast<int>(query.size()) && std::isspace(static_cast<unsigned char>(query[check]))) {
            ++check;
        }
        if (check >= query.size() || query[check] != '(') {
            continue;
        }

        if (pos > 0 && !isWordBoundary(query, pos - 1)) {
            continue;
        }

        issues.push_back({MigrationIssue::Severity::INFO, fn + "() is an ArangoDB-specific type-check function",
                          "ThemisDB uses TYPENAME(expr) for runtime type introspection. "
                          "Replace "
                              + fn + "(x) with TYPENAME(x) == '<type>' as appropriate."});
    }
}

// ---------------------------------------------------------------------------
// Detection: HASH()
// ---------------------------------------------------------------------------

void AQLMigrationAssistant::detectHashFunction(const std::string &query, std::vector<MigrationIssue> &issues) const {
    const std::string fn = "HASH";
    std::size_t pos      = findCI(query, fn);
    if (pos == std::string::npos) {
        return;
    }

    std::size_t check = pos + fn.size();
    while (check <static_cast<int>(query.size()) && std::isspace(static_cast<unsigned char>(query[check]))) {
        ++check;
    }
    if (check >= query.size() || query[check] != '(') {
        return;
    }

    if (pos > 0 && !isWordBoundary(query, pos - 1)) {
        return;
    }

    issues.push_back({MigrationIssue::Severity::INFO, "HASH() is not available in ThemisDB AQL",
                      "Use SHA256(expr) for cryptographic hashing, or MD5(expr) if MD5 is sufficient. "
                      "Note that HASH() in ArangoDB returns a numeric CityHash, not a hex string."});
}

// ---------------------------------------------------------------------------
// Detection: ATTRIBUTES()
// ---------------------------------------------------------------------------

void AQLMigrationAssistant::detectAttributesFunction(const std::string &query,
                                                     std::vector<MigrationIssue> &issues) const {
    const std::string fn = "ATTRIBUTES";
    std::size_t pos      = findCI(query, fn);
    if (pos == std::string::npos) {
        return;
    }

    std::size_t check = pos + fn.size();
    while (check <static_cast<int>(query.size()) && std::isspace(static_cast<unsigned char>(query[check]))) {
        ++check;
    }
    if (check >= query.size() || query[check] != '(') {
        return;
    }

    if (pos > 0 && !isWordBoundary(query, pos - 1)) {
        return;
    }

    issues.push_back({MigrationIssue::Severity::INFO, "ATTRIBUTES() is not available in ThemisDB AQL",
                      "Use KEYS(doc) to retrieve the attribute names of a document object."});
}

// ---------------------------------------------------------------------------
// Detection: TRANSLATE()
// ---------------------------------------------------------------------------

void AQLMigrationAssistant::detectTranslateFunction(const std::string &query,
                                                    std::vector<MigrationIssue> &issues) const {
    const std::string fn = "TRANSLATE";
    std::size_t pos      = findCI(query, fn);
    if (pos == std::string::npos) {
        return;
    }

    std::size_t check = pos + fn.size();
    while (check <static_cast<int>(query.size()) && std::isspace(static_cast<unsigned char>(query[check]))) {
        ++check;
    }
    if (check >= query.size() || query[check] != '(') {
        return;
    }

    if (pos > 0 && !isWordBoundary(query, pos - 1)) {
        return;
    }

    issues.push_back({MigrationIssue::Severity::INFO, "TRANSLATE() is not available in ThemisDB AQL",
                      "TRANSLATE(value, lookup) maps a value to another via a lookup object. "
                      "Replace with a FOR/FILTER lookup query on a mapping collection."});
}

} // namespace aql
} // namespace themis
