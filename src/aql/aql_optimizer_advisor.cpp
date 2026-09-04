/**
 * @file aql_optimizer_advisor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_optimizer_advisor.h"
#include <stdexcept>

#include <algorithm>
#include <cctype>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>

#include "analytics/nlp_text_analyzer.h"
#include "query/query_optimizer.h"

namespace themis {
namespace aql {

// Bring QueryOptimizer into scope (from query namespace)
using query::QueryOptimizer;

namespace {

// ============================================================================
// Internal helpers
// ============================================================================

std::string toUpper(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

bool queryContains(const std::string &upper_query, const std::string &keyword) {
    std::string pattern = "(?:^|[^A-Z0-9_])" + keyword + "(?:$|[^A-Z0-9_])";
    try {
        std::regex re(pattern);
        return std::regex_search(upper_query, re);
    } catch (...) {
        spdlog::debug("[AQLOptimizerAdvisor] regex compile for keyword check failed; using substring fallback");
        return upper_query.find(keyword) != std::string::npos;
    }
}

// Estimate max traversal depth from TRAVERSE … 1..N patterns
size_t extractMaxTraversalDepth(const std::string &query) {
    std::regex depth_re(R"(\b(\d+)\.\.(\d+)\b)");
    std::sregex_iterator it(query.begin(), query.end(), depth_re);
    std::sregex_iterator end;
    size_t max_depth = 0;
    for (; it != end; ++it) {
        size_t upper = static_cast<size_t>(std::stoul((*it)[2].str()));
        if (upper > max_depth)
            max_depth = upper;
    }
    return max_depth;
}

// Retrieve (or lazily create) the shared NLP analyzer
const themis::analytics::NlpTextAnalyzer &getNlpAnalyzer() {
    static themis::analytics::NlpTextAnalyzer instance;
    return instance;
}

} // anonymous namespace

// ============================================================================
// AQLOptimizerAdvisor::suggest
// ============================================================================

std::vector<ValidationIssue> AQLOptimizerAdvisor::suggest(const std::string &query) const {
    std::vector<ValidationIssue> suggestions;

    if (query.empty()) {
        return suggestions;
    }

    const std::string upper = toUpper(query);
    const auto &nlp         = getNlpAnalyzer();

    // Detect AQL-specific keyword groups once (reused across multiple checks)
    const bool has_vector = queryContains(upper, "SIMILARITY") || queryContains(upper, "NEAREST");
    const bool has_geo    = queryContains(upper, "PROXIMITY") || queryContains(upper, "GEO_CONTAINS")
                            || queryContains(upper, "ST_WITHIN") || queryContains(upper, "WITHIN_RECTANGLE");
    const bool has_fulltext
        = queryContains(upper, "FULLTEXT") || queryContains(upper, "SEARCH") || queryContains(upper, "LIKE");
    // AQL graph traversal: direction keyword (OUTBOUND/INBOUND) is the canonical
    // indicator; TRAVERSE / SHORTEST_PATH / K_SHORTEST_PATHS are also accepted.
    const bool has_traverse = queryContains(upper, "OUTBOUND") || queryContains(upper, "INBOUND")
                              || queryContains(upper, "TRAVERSE") || queryContains(upper, "SHORTEST_PATH")
                              || queryContains(upper, "K_SHORTEST_PATHS");

    // ------------------------------------------------------------------
    // 1. Query complexity warning
    // ------------------------------------------------------------------
    double complexity = nlp.estimateQueryComplexity(query);
    if (complexity > 0.7) {
        suggestions.push_back({ValidationIssue::Severity::WARNING,
                               "Query complexity is high (score " + std::to_string(static_cast<int>(complexity * 100))
                                   + "/100). "
                                     "Consider breaking it into smaller sub-queries or adding "
                                     "intermediate LET clauses to aid the optimizer.",
                               "OPTIMIZER"});
    }

    // ------------------------------------------------------------------
    // 2. Index suggestions
    //    Start with NLP-based suggestions then supplement with direct AQL
    //    keyword detection: NlpTextAnalyzer only matches generic terms like
    //    "geo", "distance", "similarity" — not AQL-specific function names
    //    such as PROXIMITY, ST_WITHIN, or SIMILARITY.
    // ------------------------------------------------------------------
    std::vector<std::string> index_hints = nlp.suggestIndexes(query);

    auto containsIndexHint = [&]([[maybe_unused]] const std::string &h) {
        return std::find(index_hints.begin(), index_hints.end(), h) != index_hints.end();
    };

    if (has_geo && !containsIndexHint("spatial")) {
        index_hints.push_back("spatial");
    }
    if (has_vector && !containsIndexHint("hnsw")) {
        index_hints.push_back("hnsw");
    }

    for (const auto &idx : index_hints) {
        std::string msg;
        if (idx == "fulltext") {
            msg = "Query contains a fulltext search pattern. "
                  "Ensure a fulltext index exists on the searched field(s).";
        } else if (idx == "hnsw") {
            msg = "Query contains a vector similarity pattern. "
                  "Ensure an HNSW vector index exists on the embedding field.";
        } else if (idx == "spatial") {
            msg = "Query contains a geospatial pattern. "
                  "Ensure a spatial (geo) index exists on the coordinate field.";
        } else {
            // btree / hash / other — generic hint
            msg = "Query may benefit from a " + idx + " index.";
        }
        suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});
    }

    // ------------------------------------------------------------------
    // 3. Vector + Geo hybrid plan ordering
    // ------------------------------------------------------------------
    if (has_vector && has_geo) {
        QueryOptimizer::VectorGeoCostInput cost_in;
        cost_in.hasVectorIndex  = true;
        cost_in.hasSpatialIndex = true;
        cost_in.bboxRatio       = 0.1; // assume moderate spatial selectivity
        cost_in.k               = 10;

        auto cost_out         = QueryOptimizer::chooseVectorGeoPlan(cost_in);
        std::string plan_name = (cost_out.plan == QueryOptimizer::VectorGeoPlan::SpatialThenVector)
                                    ? "spatial filter first, then vector search"
                                    : "vector search first, then spatial filter";

        suggestions.push_back({ValidationIssue::Severity::INFO,
                               "Hybrid vector+geo query detected. "
                               "The cost model recommends: "
                                   + plan_name
                                   + ". "
                                     "Use a LET clause to compute the cheaper predicate first.",
                               "OPTIMIZER"});
    }

    // ------------------------------------------------------------------
    // 4. Fulltext + Geo hybrid plan ordering
    // ------------------------------------------------------------------
    if (has_fulltext && has_geo) {
        QueryOptimizer::ContentGeoCostInput cost_in;
        cost_in.hasFulltextIndex = true;
        cost_in.hasSpatialIndex  = true;
        cost_in.fulltextHits     = 1000; // conservative estimate
        cost_in.bboxRatio        = 0.1;

        auto cost_out         = QueryOptimizer::estimateContentGeo(cost_in);
        std::string plan_name = cost_out.chooseFulltextFirst ? "fulltext filter first, then spatial filter"
                                                             : "spatial filter first, then fulltext filter";

        suggestions.push_back({ValidationIssue::Severity::INFO,
                               "Hybrid fulltext+geo query detected. "
                               "The cost model recommends: "
                                   + plan_name + ".",
                               "OPTIMIZER"});
    }

    // ------------------------------------------------------------------
    // 5. Graph traversal depth risk
    // ------------------------------------------------------------------
    if (has_traverse) {
        size_t max_depth = extractMaxTraversalDepth(query);
        if (max_depth == 0) {
            max_depth = 5; // default when no range found
        }

        QueryOptimizer::GraphPathCostInput cost_in;
        cost_in.maxDepth        = max_depth;
        cost_in.branchingFactor = 4;

        auto cost_out = QueryOptimizer::estimateGraphPath(cost_in);

        if (cost_out.estimatedExpandedVertices > 50000) {
            suggestions.push_back({ValidationIssue::Severity::WARNING,
                                   "Graph traversal may expand up to ~"
                                       + std::to_string(static_cast<size_t>(cost_out.estimatedExpandedVertices))
                                       + " vertices at depth " + std::to_string(max_depth)
                                       + ". Consider adding FILTER predicates or reducing max depth "
                                         "to avoid excessive memory use.",
                                   "OPTIMIZER"});
        } else if (cost_out.estimatedExpandedVertices > 5000) {
            suggestions.push_back({ValidationIssue::Severity::INFO,
                                   "Graph traversal may expand ~"
                                       + std::to_string(static_cast<size_t>(cost_out.estimatedExpandedVertices))
                                       + " vertices at depth " + std::to_string(max_depth)
                                       + ". Adding early FILTER predicates can reduce expansion.",
                                   "OPTIMIZER"});
        }
    }

    return suggestions;
}

} // namespace aql
} // namespace themis

