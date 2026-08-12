/**
 * @file aql_predicate_filter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/aql_predicate_filter.h"

#include <nlohmann/json.hpp>

#include "query/aql_parser.h"
#include "query/let_evaluator.h"
#include "utils/logger.h"

namespace themis::exporters {

AqlPredicateFilter::AqlPredicateFilter(const std::string &predicate) : predicate_(predicate) {
    if (predicate.empty()) {
        return; // Empty predicate always passes
    }

    // Wrap the predicate in a minimal AQL query to leverage the full parser
    const std::string wrapped = "FOR doc IN _export FILTER " + predicate + " RETURN doc";

    themis::query::AQLParser parser;
    auto result = parser.parse(wrapped);
    if (!result) {
        throw AqlPredicateFilterException("Failed to parse AQL predicate '" + predicate
                                          + "': " + result.error().message());
    }

    auto query = *result;
    if (!query || query->filters.empty()) {
        throw AqlPredicateFilterException("AQL predicate '" + predicate + "' produced no filter conditions");
    }

    filter_nodes_ = query->filters;
}

AqlPredicateFilter::~AqlPredicateFilter() = default;

bool AqlPredicateFilter::evaluate(const BaseEntity &entity) const {
    if (filter_nodes_.empty()) {
        return true; // No filter → accept all
    }

    // Serialize entity to JSON once; the variable name "doc" is automatically
    // resolved by LetEvaluator when it encounters a VariableExpr named "doc".
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(entity.toJson());
    } catch (const std::exception &e) {
        THEMIS_WARN("AqlPredicateFilter: failed to parse entity JSON for '{}': {}", entity.getPrimaryKey(), e.what());
        return false;
    }

    themis::query::LetEvaluator evaluator;

    for (const auto &filter_node : filter_nodes_) {
        if (!filter_node || !filter_node->condition) {
            continue;
        }
        try {
            auto result = evaluator.evaluateExpression(filter_node->condition, doc);
            // evaluateExpression returns a JSON value; treat it as bool
            if (result.is_boolean()) {
                if (!result.get<bool>()) {
                    return false;
                }
            } else if (result.is_null()) {
                return false;
            } else if (result.is_number()) {
                if (result.get<double>() == 0.0) {
                    return false;
                }
            } else if (result.is_string()) {
                if (result.get<std::string>().empty()) {
                    return false;
                }
            }
            // Arrays/objects are truthy
        } catch (const std::exception &e) {
            THEMIS_WARN("AqlPredicateFilter: predicate evaluation error for '{}': {}", entity.getPrimaryKey(),
                        e.what());
            return false;
        }
    }

    return true;
}

} // namespace themis::exporters
