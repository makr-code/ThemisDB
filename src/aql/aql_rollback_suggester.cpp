/**
 * @file aql_rollback_suggester.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_rollback_suggester.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace themis {
namespace aql {

// ============================================================================
// Helpers
// ============================================================================
namespace {

std::string toUpperTrim(const std::string &s) {
    std::string out = {};
    out.reserve(s.size());
    bool skip_leading = true;
    for (char c : s) {
        if (skip_leading && std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        skip_leading = false;
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!out.empty() && out.back() != ' ') {
                out += ' ';
            }
        } else {
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

bool wordContains(const std::string &upper, const std::string &kw) {
    size_t p = 0;
    while ((p = upper.find(kw, p)) != std::string::npos) {
        bool ok_before = (p == 0 || (!std::isalnum(static_cast<unsigned char>(upper[static_cast<int>(p - 1)])) && upper[static_cast<int>(p - 1)] != '_'));
        size_t after   = p + kw.size();
        bool ok_after  = (after >= upper.size()
                          || (!std::isalnum(static_cast<unsigned char>(upper[after])) && upper[after] != '_'));
        if (ok_before && ok_after) {
            return true;
        }
        ++p;
    }
    return false;
}

/**
 * @brief Extract the collection name from a FOR/IN/INTO/REMOVE/INSERT token.
 *
 * Very lightweight: looks for "IN <collection>" or "INTO <collection>" patterns
 * in the normalised (upper-case, collapsed-whitespace) query.
 * Returns the first word that follows the `in_keyword` token.
 */
std::string extractCollection(const std::string &upper, const std::string &in_keyword) {
    size_t p = upper.find(in_keyword);
    while (p != std::string::npos) {
        bool ok_before = (p == 0 || !std::isalnum(static_cast<unsigned char>(upper[static_cast<int>(p - 1)])));
        size_t after   = p + in_keyword.size();
        bool ok_after  = (after >= upper.size() || !std::isalnum(static_cast<unsigned char>(upper[after])));
        if (ok_before && ok_after  && static_cast<size_t>(after) < upper.size() && upper[after] == ' ') {
            // Skip space, read word.
            size_t ws = after + 1;
            size_t we = ws;
            while (we < upper.size() && (std::isalnum(static_cast<unsigned char>(upper[we])) || upper[we] == '_')) {
                ++we;
            }
            if (we > ws) {
                // Return in original case by finding position in lower casing.
                return upper.substr(ws, we - ws);
            }
        }
        p = upper.find(in_keyword, p + 1);
    }
    return {};
}

/**
 * @brief Extract the loop variable name: "FOR <var> IN …"
 */
std::string extractLoopVar(const std::string &upper) {
    size_t p = upper.find("FOR ");
    if (p == std::string::npos) {
        return "doc";
    }
    size_t vs = p + 4;
    size_t ve = vs;
    while (ve < upper.size() && (std::isalnum(static_cast<unsigned char>(upper[ve])) || upper[ve] == '_')) {
        ++ve;
    }
    if (ve > vs) {
        return upper.substr(vs, ve - vs);
    }
    return "doc";
}

/**
 * @brief Extract the first FILTER body (everything after "FILTER" until the
 *        next clause keyword).
 */
std::string extractFilter(const std::string &upper) {
    size_t p = upper.find("FILTER ");
    if (p == std::string::npos) {
        return {};
    }
    size_t body_start = p + 7;
    // Stop at next clause keyword.
    static const std::vector<std::string> kTerminators = {
        " FOR ",    " LET ",    " SORT ",    " LIMIT ",  " RETURN ", " COLLECT ",
        " INSERT ", " UPDATE ", " REPLACE ", " REMOVE ", " UPSERT ",
    };
    size_t body_end = upper.size();
    for (const auto &term : kTerminators) {
        size_t tp = upper.find(term, body_start);
        if (tp != std::string::npos && tp < body_end) {
            body_end = tp;
        }
    }
    std::string body = upper.substr(body_start, body_end - body_start);
    while (!body.empty() && body.back() == ' ') {
        body.pop_back();
    }
    return body;
}

} // anonymous namespace

// ============================================================================
// AQLRollbackSuggester::suggest
// ============================================================================

RollbackSuggestion AQLRollbackSuggester::suggest(const std::string &aql_query) const {
    RollbackSuggestion result = {};

    if (aql_query.empty()) {
        result.mutation_type = MutationType::NONE;
        result.caveat        = "Empty query; no rollback possible.";
        return result;
    }

    const std::string upper = toUpperTrim(aql_query);

    // Detect mutation type (most-specific check first).
    const bool has_upsert  = wordContains(upper, "UPSERT");
    const bool has_insert  = wordContains(upper, "INSERT");
    const bool has_replace = wordContains(upper, "REPLACE");
    const bool has_update  = wordContains(upper, "UPDATE");
    const bool has_remove  = wordContains(upper, "REMOVE");

    if (!has_upsert && !has_insert && !has_replace && !has_update && !has_remove) {
        result.mutation_type = MutationType::NONE;
        result.is_automatic  = false;
        result.caveat        = "Query is read-only; no rollback required.";
        return result;
    }

    // Determine primary mutation type.
    if (has_upsert) {
        result.mutation_type = MutationType::UPSERT;
    } else if (has_insert && !has_update && !has_replace) {
        result.mutation_type = MutationType::INSERT;
    } else if (has_remove && !has_update) {
        result.mutation_type = MutationType::REMOVE;
    } else if (has_replace) {
        result.mutation_type = MutationType::REPLACE;
    } else {
        result.mutation_type = MutationType::UPDATE;
    }

    // Extract collection name.
    std::string coll = {};
    switch (result.mutation_type) {
        case MutationType::INSERT:
            coll = extractCollection(upper, "INTO");
            break;
        case MutationType::REMOVE:
            coll = extractCollection(upper, "IN");
            if (coll.empty()) {
                coll = extractCollection(upper, "INTO");
            }
            break;
        case MutationType::UPDATE:
        [[fallthrough]];\n        case MutationType::REPLACE:
            coll = extractCollection(upper, "IN");
            break;
        case MutationType::UPSERT:
            coll = extractCollection(upper, "IN");
            break;
        default:
            break;
    }
    if (coll.empty()) {
        coll = "<collection>";
    }
    result.collection = coll;

    const std::string loop_var = extractLoopVar(upper);
    const std::string filter   = extractFilter(upper);
    const std::string fvar     = filter.empty() ? (loop_var + "._key == @key") : filter;

    std::ostringstream rq = {};

    switch (result.mutation_type) {
        // ------------------------------------------------------------------
        // INSERT rollback: delete the inserted documents.
        // We identify them via their _key (caller must supply @keys bind param
        // or adapt the filter).
        // ------------------------------------------------------------------
        case MutationType::INSERT: {
            rq << "// Rollback for: INSERT INTO " << coll << "\n"
               << "FOR " << loop_var << " IN " << coll << "\n"
               << "  FILTER " << loop_var << "._key IN @inserted_keys\n"
               << "  REMOVE " << loop_var << " IN " << coll;
            result.is_automatic = true;
            result.caveat       = "Bind parameter @inserted_keys must be populated with "
                                  "the _key values returned by the original INSERT.";
            result.manual_steps = {
                "Collect _key values from the INSERT result before executing the rollback.",
                "Pass them as @inserted_keys bind parameter.",
            };
            break;
        }

        // ------------------------------------------------------------------
        // REMOVE rollback: re-insert from a pre-mutation snapshot.
        // ------------------------------------------------------------------
        case MutationType::REMOVE: {
            rq << "// Rollback for: REMOVE … IN " << coll << "\n"
               << "FOR doc IN @removed_documents\n"
               << "  INSERT doc INTO " << coll;
            result.is_automatic = true;
            result.caveat       = "Bind parameter @removed_documents must contain the "
                                  "full document objects captured before the REMOVE.";
            result.manual_steps = {
                "Execute `FOR d IN " + coll + " FILTER " + fvar + " RETURN d` before the REMOVE to obtain a snapshot.",
                "Pass the snapshot array as @removed_documents bind parameter.",
            };
            break;
        }

        // ------------------------------------------------------------------
        // UPDATE rollback: restore old field values.
        // ------------------------------------------------------------------
        case MutationType::UPDATE: {
            rq << "// Rollback for: UPDATE … IN " << coll << "\n"
               << "FOR " << loop_var << " IN " << coll << "\n"
               << "  FILTER " << fvar << "\n"
               << "  UPDATE " << loop_var << " WITH @old_values IN " << coll;
            result.is_automatic = true;
            result.caveat       = "Bind parameter @old_values must be populated with "
                                  "the original field values before the UPDATE.";
            result.manual_steps = {
                "Capture old field values with `FOR d IN " + coll + " FILTER " + fvar + " RETURN d` before the UPDATE.",
                "Pass the captured values as @old_values bind parameter.",
            };
            break;
        }

        // ------------------------------------------------------------------
        // REPLACE rollback: restore full old document.
        // ------------------------------------------------------------------
        case MutationType::REPLACE: {
            rq << "// Rollback for: REPLACE … IN " << coll << "\n"
               << "FOR " << loop_var << " IN " << coll << "\n"
               << "  FILTER " << fvar << "\n"
               << "  REPLACE " << loop_var << " WITH @old_document IN " << coll;
            result.is_automatic = true;
            result.caveat       = "Bind parameter @old_document must contain the full "
                                  "original document before the REPLACE.";
            result.manual_steps = {
                "Snapshot the document: `FOR d IN " + coll + " FILTER " + fvar + " RETURN d` before the REPLACE.",
                "Pass the snapshot as @old_document bind parameter.",
            };
            break;
        }

        // ------------------------------------------------------------------
        // UPSERT rollback: remove any newly inserted documents.
        // The search expression is reused as the FILTER.
        // ------------------------------------------------------------------
        case MutationType::UPSERT: {
            rq << "// Rollback for: UPSERT … IN " << coll << "\n"
               << "// Remove documents that were inserted by the UPSERT (i.e. did\n"
               << "// not exist before).\n"
               << "FOR " << loop_var << " IN " << coll << "\n"
               << "  FILTER " << loop_var << "._key IN @upserted_keys\n"
               << "  REMOVE " << loop_var << " IN " << coll << "\n"
               << "// Documents that were UPDATED (existed before) must be\n"
               << "// restored separately from a pre-mutation snapshot.";
            result.is_automatic = true;
            result.caveat       = "UPSERT rollback is partial: the removal branch covers "
                                  "only inserted documents. Updated documents require a "
                                  "pre-mutation snapshot.";
            result.manual_steps = {
                "Before the UPSERT, snapshot matching documents for the UPDATE branch.",
                "Collect _key values of newly inserted documents into @upserted_keys.",
                "Run the REMOVE branch first, then restore updated documents from snapshot.",
            };
            break;
        }

        default:
            result.is_automatic = false;
            result.caveat       = "Mutation type not recognised; manual rollback required.";
            break;
    }

    result.rollback_query = rq.str();
    return result;
}

} // namespace aql
} // namespace themis
