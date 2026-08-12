/**
 * @file aql_query_diff_explainer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_query_diff_explainer.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace aql {

// ============================================================================
// Helpers
// ============================================================================
namespace {

/// Normalise whitespace: collapse runs of whitespace to a single space and trim.
std::string normaliseWs(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    bool last_space = true; // trim leading
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_space) {
                out += ' ';
                last_space = true;
            }
        } else {
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            last_space = false;
        }
    }
    if (!out.empty() && out.back() == ' ') {
        out.pop_back(); // trim trailing
    }
    return out;
}

/// Ordered AQL clause keywords (longest-match first).
static const std::vector<std::string> kClauseKeywords = {
    "FOR",    "LET",     "FILTER", "SORT",   "LIMIT", "RETURN", "COLLECT", "INSERT",
    "UPDATE", "REPLACE", "REMOVE", "UPSERT", "WITH",  "INTO",   "IN",      "GRAPH",
};

/**
 * @brief Split a normalised (upper-cased, collapsed-whitespace) AQL query into
 *        a map of clause-keyword → clause-body.
 *
 * Only the first occurrence of each keyword is recorded; later occurrences
 * are appended to the first body.
 */
std::unordered_map<std::string, std::string> splitClauses(const std::string &norm) {
    std::unordered_map<std::string, std::string> clauses;

    // Build a single pattern that matches any clause keyword at a word boundary.
    // We need to find the offsets of each keyword to extract text slices.
    struct Match {
        size_t pos;
        std::string keyword;
    };
    std::vector<Match> matches;

    for (const auto &kw : kClauseKeywords) {
        // Simple word-boundary search: keyword must be preceded by start or space
        // and followed by end, space, or '('.
        size_t search_from = 0;
        while (true) {
            size_t p = norm.find(kw, search_from);
            if (p == std::string::npos) {
                break;
            }
            bool ok_before = (p == 0 || !std::isalnum(static_cast<unsigned char>(norm[p - 1])));
            size_t after   = p + kw.size();
            bool ok_after  = (after >= norm.size() || !std::isalnum(static_cast<unsigned char>(norm[after])));
            if (ok_before && ok_after) {
                matches.push_back({p, kw});
            }
            search_from = p + 1;
        }
    }

    // Sort by position.
    std::sort(matches.begin(), matches.end(), [](const Match &a, const Match &b) { return a.pos < b.pos; });

    // Extract bodies.
    for (size_t i = 0; i < matches.size(); ++i) {
        size_t body_start = matches[i].pos + matches[i].keyword.size();
        size_t body_end   = (i + 1 < matches.size()) ? matches[i + 1].pos : norm.size();
        std::string body  = norm.substr(body_start, body_end - body_start);
        if (!body.empty() && body.front() == ' ') {
            body = body.substr(1);
        }
        if (!body.empty() && body.back() == ' ') {
            body.pop_back();
        }
        clauses[matches[i].keyword] += body;
    }
    return clauses;
}

const char *kindLabel(QueryDiffEntry::Kind k) {
    switch (k) {
        case QueryDiffEntry::Kind::CLAUSE_ADDED:
            return "added";
        case QueryDiffEntry::Kind::CLAUSE_REMOVED:
            return "removed";
        case QueryDiffEntry::Kind::CLAUSE_CHANGED:
            return "changed";
        case QueryDiffEntry::Kind::FILTER_CHANGED:
            return "filter changed";
        case QueryDiffEntry::Kind::SORT_CHANGED:
            return "sort changed";
        case QueryDiffEntry::Kind::LIMIT_CHANGED:
            return "limit changed";
        case QueryDiffEntry::Kind::RETURN_CHANGED:
            return "return changed";
        case QueryDiffEntry::Kind::COLLECTION_CHANGED:
            return "collection changed";
        case QueryDiffEntry::Kind::FUNCTION_ADDED:
            return "function added";
        case QueryDiffEntry::Kind::FUNCTION_REMOVED:
            return "function removed";
        case QueryDiffEntry::Kind::STRUCTURAL:
            return "structural change";
    }
    return "changed";
}

QueryDiffEntry::Kind kindForClause(const std::string &kw, bool in_a, bool in_b) {
    if (!in_a && in_b) {
        return QueryDiffEntry::Kind::CLAUSE_ADDED;
    }
    if (in_a && !in_b) {
        return QueryDiffEntry::Kind::CLAUSE_REMOVED;
    }
    if (kw == "FILTER") {
        return QueryDiffEntry::Kind::FILTER_CHANGED;
    }
    if (kw == "SORT") {
        return QueryDiffEntry::Kind::SORT_CHANGED;
    }
    if (kw == "LIMIT") {
        return QueryDiffEntry::Kind::LIMIT_CHANGED;
    }
    if (kw == "RETURN") {
        return QueryDiffEntry::Kind::RETURN_CHANGED;
    }
    if (kw == "FOR" || kw == "IN" || kw == "INTO") {
        return QueryDiffEntry::Kind::COLLECTION_CHANGED;
    }
    return QueryDiffEntry::Kind::CLAUSE_CHANGED;
}

/// Detect which AQL built-in function calls are present in a normalised query.
std::unordered_set<std::string> detectFunctions(const std::string &norm) {
    // Match word-boundary "WORD(" patterns.
    static const std::regex kFnRe(R"(\b([A-Z][A-Z0-9_]+)\s*\()");
    std::unordered_set<std::string> fns;
    auto begin = std::sregex_iterator(norm.begin(), norm.end(), kFnRe);
    for (auto it = begin; it != std::sregex_iterator(); ++it) {
        fns.insert((*it)[1].str());
    }
    return fns;
}

} // anonymous namespace

// ============================================================================
// QueryDiffResult helpers
// ============================================================================

int QueryDiffResult::count(QueryDiffEntry::Kind kind) const {
    int n = 0;
    for (const auto &d : diffs) {
        if (d.kind == kind) {
            ++n;
        }
    }
    return n;
}

// ============================================================================
// AQLQueryDiffExplainer::explain
// ============================================================================

QueryDiffResult AQLQueryDiffExplainer::explain(const std::string &query_a, const std::string &query_b) const {
    QueryDiffResult result;

    // Fast path: identical strings.
    const std::string norm_a = normaliseWs(query_a);
    const std::string norm_b = normaliseWs(query_b);
    if (norm_a == norm_b) {
        result.is_equivalent = true;
        result.summary       = "Queries are structurally equivalent.";
        return result;
    }

    // Clause-level diff.
    const auto clauses_a = splitClauses(norm_a);
    const auto clauses_b = splitClauses(norm_b);

    // Gather all clause keywords present in either query.
    std::unordered_set<std::string> all_keys;
    for (const auto &[k, _] : clauses_a) {
        all_keys.insert(k);
    }
    for (const auto &[k, _] : clauses_b) {
        all_keys.insert(k);
    }

    // Visit in canonical order.
    for (const auto &kw : kClauseKeywords) {
        if (all_keys.find(kw) == all_keys.end()) {
            continue;
        }
        auto it_a       = clauses_a.find(kw);
        auto it_b       = clauses_b.find(kw);
        const bool in_a = (it_a != clauses_a.end());
        const bool in_b = (it_b != clauses_b.end());

        if (!in_a && !in_b) {
            continue;
        }

        const std::string body_a = in_a ? it_a->second : std::string{};
        const std::string body_b = in_b ? it_b->second : std::string{};

        if (in_a == in_b && body_a == body_b) {
            continue; // no change
        }

        QueryDiffEntry entry;
        entry.kind     = kindForClause(kw, in_a, in_b);
        entry.clause_a = in_a ? (kw + " " + body_a) : std::string{};
        entry.clause_b = in_b ? (kw + " " + body_b) : std::string{};

        if (!in_a) {
            entry.explanation = "Clause `" + kw + "` was added in query B.";
        } else if (!in_b) {
            entry.explanation = "Clause `" + kw + "` was removed in query B.";
        } else {
            entry.explanation = "Clause `" + kw + "` changed between the two queries.";
        }
        result.diffs.push_back(std::move(entry));
    }

    // Function-level diff.
    const auto fns_a = detectFunctions(norm_a);
    const auto fns_b = detectFunctions(norm_b);

    for (const auto &fn : fns_b) {
        if (!fns_a.count(fn)) {
            QueryDiffEntry e;
            e.kind        = QueryDiffEntry::Kind::FUNCTION_ADDED;
            e.clause_b    = fn + "(…)";
            e.explanation = "Function `" + fn + "` was added in query B.";
            result.diffs.push_back(std::move(e));
        }
    }
    for (const auto &fn : fns_a) {
        if (!fns_b.count(fn)) {
            QueryDiffEntry e;
            e.kind        = QueryDiffEntry::Kind::FUNCTION_REMOVED;
            e.clause_a    = fn + "(…)";
            e.explanation = "Function `" + fn + "` was removed in query B.";
            result.diffs.push_back(std::move(e));
        }
    }

    // Build summary.
    if (result.diffs.empty()) {
        // Normalisation caught them above; this means only whitespace diff.
        result.is_equivalent = true;
        result.summary       = "Queries are structurally equivalent (whitespace differences only).";
    } else {
        std::ostringstream ss;
        ss << result.diffs.size() << " difference" << (result.diffs.size() != 1 ? "s" : "") << ": ";
        for (size_t i = 0; i < result.diffs.size(); ++i) {
            if (i) {
                ss << "; ";
            }
            ss << kindLabel(result.diffs[i].kind);
        }
        ss << ".";
        result.summary = ss.str();
    }
    return result;
}

} // namespace aql
} // namespace themis
