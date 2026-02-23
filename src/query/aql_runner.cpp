/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_runner.cpp                                     ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     306                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8ce3b5039  2026-02-21  fix(query): use error().message() consistently in explain... ║
    • 8ece79254  2026-02-21  feat(query): wire QueryPlanVisualizer into AQL pipeline v... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/aql_runner.h"
#include "query/query_plan_visualizer.h"
#include "storage/base_entity.h"
#include "analytics/nlp_text_analyzer.h"

namespace themis {

// Lazy-initialized NLP analyzer (thread-safe in C++11+)
static themis::analytics::NlpTextAnalyzer& getNlpAnalyzer() {
    static themis::analytics::NlpTextAnalyzer instance;
    return instance;
}

// GAP-002: Migrated from std::pair<Status, json> to Result<json>
Result<nlohmann::json> executeAql(const std::string& aql, QueryEngine& engine) {
    // NLP Pre-processing (PR #317 Integration Phase 1)
    // This provides query analysis for optimization and caching
    auto& nlp = getNlpAnalyzer();
    std::string normalized_query = nlp.normalizeQuery(aql);
    double query_complexity = nlp.estimateQueryComplexity(aql);
    auto query_hints = nlp.extractQueryHints(aql);
    auto suggested_indexes = nlp.suggestIndexes(aql);
    
    // Parse AQL query
    query::AQLParser parser;
    auto parseResult = parser.parse(aql);
    if (!parseResult) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            parseResult.error().message()
        );
    }
    
    // Translate to internal query representation
    auto query_ptr = parseResult.value();
    auto tr = AQLTranslator::translate(query_ptr);
    if (!tr.success) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            tr.error_message
        );
    }
    
    // Vector+Geo hybrid dispatch
    if (tr.vector_geo.has_value()) {
        auto result = engine.executeVectorGeoQuery(*tr.vector_geo);
        if (!result) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                result.error().message()
            );
        }
        auto res = *result;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : res) {
            arr.push_back({
                {"pk", r.pk},
                {"distance", r.vector_distance},
                {"entity", r.entity}
            });
        }
        return Ok(nlohmann::json({{"type","vector_geo"},{"results", arr}}));
    }
    // Content+Geo hybrid dispatch (FULLTEXT + PROXIMITY)
    if (tr.content_geo.has_value()) {
        auto result = engine.executeContentGeoQuery(*tr.content_geo);
        if (!result) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                result.error().message()
            );
        }
        auto res = *result;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : res) {
            nlohmann::json row = {
                {"pk", r.pk},
                {"bm25", r.bm25_score},
                {"entity", r.entity}
            };
            if (r.geo_distance.has_value()) row["geo_distance"] = *r.geo_distance;
            arr.push_back(std::move(row));
        }
        return Ok(nlohmann::json::object({{"type","content_geo"},{"results", arr}}));
    }

    // Disjunctive OR query
    if (tr.disjunctive.has_value()) {
        auto result = engine.executeOrEntitiesWithFallback(*tr.disjunctive, true);
        if (!result) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                result.error().message()
            );
        }
        auto ents = std::move(*result);
        nlohmann::json arr = nlohmann::json::array();
        for (auto& e : ents) {
            arr.push_back(nlohmann::json::parse(e.toJson()));
        }
        return Ok(nlohmann::json({{"type","or"},{"results", arr}}));
    }

    // Traversal / Shortest Path dispatch
    if (tr.traversal.has_value()) {
        const auto &tv = *tr.traversal;
        if (tv.shortestPath) {
            RecursivePathQuery rq; rq.start_node = tv.startVertex; rq.end_node = tv.endVertex; rq.graph_id = tv.graphName; rq.max_depth = tv.maxDepth; rq.edge_type = ""; // edge_type placeholder
            auto result = engine.executeRecursivePathQuery(rq);
            if (!result) {
                return Err<nlohmann::json>(
                    errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                    result.error().message()
                );
            }
            auto paths = std::move(*result);
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& p : paths) arr.push_back(p);
            return Ok(nlohmann::json({{"type","shortest_path"},{"paths", arr}}));
        }
        
        // General traversal (non-shortest path)
        // Convert TraversalQuery::Direction to TraversalDirection
        TraversalDirection dir;
        switch (tv.direction) {
            case AQLTranslator::TranslationResult::TraversalQuery::Direction::Outbound:
                dir = TraversalDirection::OUTBOUND;
                break;
            case AQLTranslator::TranslationResult::TraversalQuery::Direction::Inbound:
                dir = TraversalDirection::INBOUND;
                break;
            case AQLTranslator::TranslationResult::TraversalQuery::Direction::Any:
                dir = TraversalDirection::ANY;
                break;
            default:
                dir = TraversalDirection::OUTBOUND;
                break;
        }
        
        auto result = engine.executeGeneralTraversal(
            tv.startVertex,
            tv.minDepth,
            tv.maxDepth,
            dir,
            tv.graphName.empty() ? "default" : tv.graphName
        );
        
        if (!result) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                result.error().message()
            );
        }
        
        auto results = std::move(*result);
        
        // Format results as JSON array
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : results) {
            nlohmann::json item;
            item["vertex"] = r.vertex_pk;
            item["depth"] = r.depth;
            item["path"] = r.path;
            item["edges"] = r.edges;
            item["data"] = r.vertex_data;
            arr.push_back(std::move(item));
        }
        
        return Ok(nlohmann::json({{"type","traversal"},{"results", arr}}));
    }

    // Join query
    if (tr.join.has_value()) {
        auto& j = *tr.join;
        auto result = engine.executeJoin(j.for_nodes, j.filters, j.let_nodes, j.return_node, j.sort, j.limit);
        if (!result) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                result.error().message()
            );
        }
        auto rows = std::move(*result);
        return Ok(nlohmann::json({{"type","join"},{"results", rows}}));
    }

    // Conjunctive (default) query
    auto result = engine.executeAndEntitiesWithFallback(tr.query, true);
    if (!result) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            result.error().message()
        );
    }
    auto entities = std::move(*result);
    nlohmann::json arr = nlohmann::json::array();
    for (auto& e : entities) arr.push_back(nlohmann::json::parse(e.toJson()));
    return Ok(nlohmann::json({{"type","and"},{"results", arr}}));
}

// ── Explain helpers ───────────────────────────────────────────────────────────

namespace {

/// Parse + translate an AQL string and return the conjunctive query to be explained.
/// For non-conjunctive forms (graph traversal, vector+geo, …) a synthetic
/// ConjunctiveQuery with a descriptive table name and no predicates is returned
/// so that the visualizer can still emit a meaningful SeqScan node.
/// Returns Err on parse or translation failure.
Result<ConjunctiveQuery> parseAndTranslateForExplain(const std::string& aql) {
    query::AQLParser parser;
    auto parseResult = parser.parse(aql);
    if (!parseResult) {
        return Err<ConjunctiveQuery>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            parseResult.error().message()
        );
    }
    auto tr = AQLTranslator::translate(parseResult.value());
    if (!tr.success) {
        return Err<ConjunctiveQuery>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            tr.error_message
        );
    }
    // Non-conjunctive forms: return a synthetic query that describes the type.
    if (tr.vector_geo.has_value()) {
        ConjunctiveQuery q;
        q.table = "[vector+geo] " + tr.query.table;
        return Ok(q);
    }
    if (tr.content_geo.has_value()) {
        ConjunctiveQuery q;
        q.table = "[content+geo] " + tr.query.table;
        return Ok(q);
    }
    if (tr.disjunctive.has_value()) {
        ConjunctiveQuery q;
        q.table = "[OR] " + tr.disjunctive->table;
        return Ok(q);
    }
    if (tr.traversal.has_value()) {
        ConjunctiveQuery q;
        q.table = "[graph-traversal] " + tr.traversal->graphName;
        return Ok(q);
    }
    if (tr.join.has_value()) {
        ConjunctiveQuery q;
        q.table = "[join]";
        return Ok(q);
    }
    return Ok(tr.query);
}

} // anonymous namespace

Result<nlohmann::json> explainAql(const std::string& aql, QueryEngine& engine, bool analyze) {
    auto qr = parseAndTranslateForExplain(aql);
    if (!qr) {
        return Err<nlohmann::json>(qr.error().code(), qr.error().message());
    }
    auto plan_node = engine.buildExplainPlan(*qr);
    return Ok(query::QueryPlanVisualizer::toJSON(plan_node, analyze));
}

Result<std::string> explainAqlText(const std::string& aql, QueryEngine& engine, bool analyze) {
    auto qr = parseAndTranslateForExplain(aql);
    if (!qr) {
        return Err<std::string>(qr.error().code(), qr.error().message());
    }
    auto plan_node = engine.buildExplainPlan(*qr);
    return Ok(query::QueryPlanVisualizer::toText(plan_node, analyze));
}

Result<std::string> explainAqlDot(const std::string& aql, QueryEngine& engine) {
    auto qr = parseAndTranslateForExplain(aql);
    if (!qr) {
        return Err<std::string>(qr.error().code(), qr.error().message());
    }
    auto plan_node = engine.buildExplainPlan(*qr);
    return Ok(query::QueryPlanVisualizer::toDOT(plan_node));
}

} // namespace themis
