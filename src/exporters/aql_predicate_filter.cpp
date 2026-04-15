/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_predicate_filter.cpp                           ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:40:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     110                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 4c1b88380f  2026-02-27  fix: correct comment spelling in aql_predicate_filter.cpp ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/aql_predicate_filter.h"
#include "query/aql_parser.h"
#include "query/let_evaluator.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>

namespace themis::exporters {

AqlPredicateFilter::AqlPredicateFilter(const std::string& predicate)
    : predicate_(predicate)
{
    if (predicate.empty()) {
        return;  // Empty predicate always passes
    }

    // Wrap the predicate in a minimal AQL query to leverage the full parser
    const std::string wrapped =
        "FOR doc IN _export FILTER " + predicate + " RETURN doc";

    themis::query::AQLParser parser;
    auto result = parser.parse(wrapped);
    if (!result) {
        throw AqlPredicateFilterException(
            "Failed to parse AQL predicate '" + predicate + "': " +
            result.error().message()
        );
    }

    auto query = *result;
    if (!query || query->filters.empty()) {
        throw AqlPredicateFilterException(
            "AQL predicate '" + predicate + "' produced no filter conditions"
        );
    }

    filter_nodes_ = query->filters;
}

AqlPredicateFilter::~AqlPredicateFilter() = default;

bool AqlPredicateFilter::evaluate(const BaseEntity& entity) const {
    if (filter_nodes_.empty()) {
        return true;  // No filter → accept all
    }

    // Serialize entity to JSON once; the variable name "doc" is automatically
    // resolved by LetEvaluator when it encounters a VariableExpr named "doc".
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(entity.toJson());
    } catch (const std::exception& e) {
        THEMIS_WARN("AqlPredicateFilter: failed to parse entity JSON for '{}': {}",
                    entity.getPrimaryKey(), e.what());
        return false;
    }

    themis::query::LetEvaluator evaluator;

    for (const auto& filter_node : filter_nodes_) {
        if (!filter_node || !filter_node->condition) {
            continue;
        }
        try {
            auto result = evaluator.evaluateExpression(filter_node->condition, doc);
            // evaluateExpression returns a JSON value; treat it as bool
            if (result.is_boolean()) {
                if (!result.get<bool>()) return false;
            } else if (result.is_null()) {
                return false;
            } else if (result.is_number()) {
                if (result.get<double>() == 0.0) return false;
            } else if (result.is_string()) {
                if (result.get<std::string>().empty()) return false;
            }
            // Arrays/objects are truthy
        } catch (const std::exception& e) {
            THEMIS_WARN("AqlPredicateFilter: predicate evaluation error for '{}': {}",
                        entity.getPrimaryKey(), e.what());
            return false;
        }
    }

    return true;
}

} // namespace themis::exporters
