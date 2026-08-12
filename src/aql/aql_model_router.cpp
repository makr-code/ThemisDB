/**
 * @file aql_model_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_model_router.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace themis {
namespace aql {

// ============================================================================
// Helpers
// ============================================================================
namespace {

std::string toUpper(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

/// Return true iff `upper_query` contains `keyword` at a word boundary.
bool containsKeyword(const std::string &upper_query, const std::string &kw) {
    size_t p = 0;
    while ((p = upper_query.find(kw, p)) != std::string::npos) {
        bool ok_before
            = (p == 0 || !std::isalnum(static_cast<unsigned char>(upper_query[p - 1])) && upper_query[p - 1] != '_');
        size_t after  = p + kw.size();
        bool ok_after = (after >= upper_query.size()
                         || !std::isalnum(static_cast<unsigned char>(upper_query[after])) && upper_query[after] != '_');
        if (ok_before && ok_after) {
            return true;
        }
        ++p;
    }
    return false;
}

/**
 * @brief Keyword sets used to classify a query into model type categories.
 *
 * Each entry maps a QueryModelType to a list of indicator keywords; a match
 * on ANY keyword in the list scores one point for that type.
 */
struct ClassificationRule {
    QueryModelType type;
    std::vector<std::string> keywords;
    int weight; ///< How many keyword hits count as "detected"
};

static const std::vector<ClassificationRule> kRules = {
    {QueryModelType::VECTOR, {"KNN", "ANN", "VECTOR_SEARCH", "COSINE", "EMBEDDING", "L2_DISTANCE", "KNN_SEARCH"}, 1},
    {QueryModelType::GRAPH,
     {"TRAVERSE", "TRAVERSAL", "SHORTEST_PATH", "BFS", "DFS", "K_SHORTEST_PATHS", "GRAPH", "OUTBOUND", "INBOUND",
      "ANY"},
     2},
    {QueryModelType::GEO,
     {"ST_DISTANCE", "ST_WITHIN", "ST_INTERSECTS", "ST_CONTAINS", "ST_TOUCHES", "ST_BUFFER", "WITHIN", "NEAR",
      "GEO_DISTANCE"},
     1},
    {QueryModelType::FULLTEXT,
     {"BM25", "TFIDF", "PHRASE", "FULLTEXT", "LIKE", "ANALYZER", "LEVENSHTEIN_MATCH", "NGRAM_MATCH", "SEARCH"},
     1},
    {QueryModelType::TIMESERIES,
     {"TIME_TRUNC", "HISTOGRAM", "INTERVAL", "DOWNSAMPLE", "DATE_TRUNC", "DATE_DIFF", "TIME_BUCKET", "TIMESERIES",
      "RETENTION"},
     1},
    {QueryModelType::PROCESS,
     {"PROCESS_INSTANCES", "TOKEN_SCAN", "BPMN_MODEL", "ADVANCE_TOKEN", "PROCESS_GRAPH", "PROCESS_MODEL"},
     1},
};

const char *modelTypeName(QueryModelType t) {
    switch (t) {
        case QueryModelType::VECTOR:
            return "VECTOR";
        case QueryModelType::GRAPH:
            return "GRAPH";
        case QueryModelType::GEO:
            return "GEO";
        case QueryModelType::FULLTEXT:
            return "FULLTEXT";
        case QueryModelType::TIMESERIES:
            return "TIMESERIES";
        case QueryModelType::RELATIONAL:
            return "RELATIONAL";
        case QueryModelType::PROCESS:
            return "PROCESS";
        case QueryModelType::UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // anonymous namespace

// ============================================================================
// AQLModelRouter
// ============================================================================

void AQLModelRouter::registerRoute(const ModelRoute &route) {
    // Replace existing route for the same type.
    for (auto &r : routes_) {
        if (r.model_type == route.model_type) {
            r = route;
            // Keep sorted by priority descending.
            std::stable_sort(routes_.begin(), routes_.end(),
                             [](const ModelRoute &a, const ModelRoute &b) { return a.priority > b.priority; });
            return;
        }
    }
    routes_.push_back(route);
    std::stable_sort(routes_.begin(), routes_.end(),
                     [](const ModelRoute &a, const ModelRoute &b) { return a.priority > b.priority; });
}

void AQLModelRouter::removeRoute(QueryModelType type) {
    routes_.erase(
        std::remove_if(routes_.begin(), routes_.end(), [type](const ModelRoute &r) { return r.model_type == type; }),
        routes_.end());
}

std::vector<QueryModelType> AQLModelRouter::classify(const std::string &aql_query) const {
    if (aql_query.empty()) {
        return {QueryModelType::UNKNOWN};
    }

    const std::string upper = toUpper(aql_query);

    struct Score {
        QueryModelType type;
        int hits;
    };
    std::vector<Score> scores;

    for (const auto &rule : kRules) {
        int hits = 0;
        for (const auto &kw : rule.keywords) {
            if (containsKeyword(upper, kw)) {
                ++hits;
            }
        }
        if (hits >= rule.weight) {
            scores.push_back({rule.type, hits});
        }
    }

    if (scores.empty()) {
        // Anything with FOR…RETURN is relational.
        if (containsKeyword(upper, "FOR") && containsKeyword(upper, "RETURN")) {
            return {QueryModelType::RELATIONAL};
        }
        return {QueryModelType::UNKNOWN};
    }

    // Sort by hit count descending.
    std::sort(scores.begin(), scores.end(), [](const Score &a, const Score &b) { return a.hits > b.hits; });

    std::vector<QueryModelType> result;
    result.reserve(scores.size() + 1);
    for (const auto &s : scores) {
        result.push_back(s.type);
    }

    // Always append RELATIONAL as a fallback category if it wasn't detected.
    bool has_relational
        = std::any_of(result.begin(), result.end(), [](QueryModelType t) { return t == QueryModelType::RELATIONAL; });
    if (!has_relational) {
        result.push_back(QueryModelType::RELATIONAL);
    }

    return result;
}

RoutingDecision AQLModelRouter::route(const std::string &aql_query) const {
    RoutingDecision decision;
    decision.detected_types = classify(aql_query);

    if (decision.detected_types.empty()) {
        decision.primary_type = QueryModelType::UNKNOWN;
        decision.explanation  = "Query could not be classified.";
        return decision;
    }

    decision.primary_type = decision.detected_types.front();

    // Find best enabled route for primary type.
    auto findRoute = [&](QueryModelType target) -> std::optional<ModelRoute> {
        for (const auto &r : routes_) { // already sorted by priority desc
            if (r.model_type == target && r.enabled) {
                return r;
            }
        }
        return std::nullopt;
    };

    decision.selected_route = findRoute(decision.primary_type);

    // Build explanation.
    std::ostringstream ss;
    ss << "Classified as " << modelTypeName(decision.primary_type);
    if (decision.detected_types.size() > 1) {
        ss << " (also: ";
        for (size_t i = 1; i < decision.detected_types.size(); ++i) {
            if (i > 1) {
                ss << ", ";
            }
            ss << modelTypeName(decision.detected_types[i]);
        }
        ss << ")";
    }
    ss << ".";

    if (decision.selected_route) {
        ss << " Routed to model '" << decision.selected_route->model_alias << "'.";
    } else {
        // Try fallback: first enabled route in priority order.
        for (const auto &r : routes_) {
            if (r.enabled) {
                decision.fallback_route = r;
                break;
            }
        }
        if (decision.fallback_route) {
            ss << " No route registered for " << modelTypeName(decision.primary_type) << "; falling back to '"
               << decision.fallback_route->model_alias << "'.";
        } else {
            ss << " No routes registered.";
        }
    }

    decision.explanation = ss.str();
    return decision;
}

} // namespace aql
} // namespace themis
