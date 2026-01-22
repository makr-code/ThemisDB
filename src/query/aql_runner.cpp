#include "query/aql_runner.h"
#include "storage/base_entity.h"
#include "analytics/nlp_text_analyzer.h"

namespace themis {

// Lazy-initialized NLP analyzer (thread-safe in C++11+)
static themis::analytics::NlpTextAnalyzer& getNlpAnalyzer() {
    static themis::analytics::NlpTextAnalyzer instance;
    return instance;
}

std::pair<QueryEngine::Status, nlohmann::json> executeAql(const std::string& aql, QueryEngine& engine) {
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
    if (!parseResult.success) {
        return { QueryEngine::Status::Error(parseResult.error.toString()), nlohmann::json{{"error","parse"}} };
    }
    
    // Translate to internal query representation
    auto tr = AQLTranslator::translate(parseResult.query);
    if (!tr.success) {
        return { QueryEngine::Status::Error(tr.error_message), nlohmann::json{{"error","translate"}} };
    }

    // Vector+Geo hybrid dispatch
    if (tr.vector_geo.has_value()) {
        auto [st, res] = engine.executeVectorGeoQuery(*tr.vector_geo);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : res) {
            arr.push_back({
                {"pk", r.pk},
                {"distance", r.vector_distance},
                {"entity", r.entity}
            });
        }
        return { st, nlohmann::json{{"type","vector_geo"},{"results", arr}} };
    }
    // Content+Geo hybrid dispatch (FULLTEXT + PROXIMITY)
    if (tr.content_geo.has_value()) {
        auto [st, res] = engine.executeContentGeoQuery(*tr.content_geo);
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
        return { st, nlohmann::json{{"type","content_geo"},{"results", arr}} };
    }

    // Disjunctive OR query
    if (tr.disjunctive.has_value()) {
        auto [st, ents] = engine.executeOrEntitiesWithFallback(*tr.disjunctive, true);
        nlohmann::json arr = nlohmann::json::array();
        for (auto& e : ents) {
            arr.push_back(nlohmann::json::parse(e.toJson()));
        }
        return { st, nlohmann::json{{"type","or"},{"results", arr}} };
    }

    // Traversal / Shortest Path dispatch
    if (tr.traversal.has_value()) {
        const auto &tv = *tr.traversal;
        if (tv.shortestPath) {
            RecursivePathQuery rq; rq.start_node = tv.startVertex; rq.end_node = tv.endVertex; rq.graph_id = tv.graphName; rq.max_depth = tv.maxDepth; rq.edge_type = ""; // edge_type placeholder
            auto [st, paths] = engine.executeRecursivePathQuery(rq);
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& p : paths) arr.push_back(p);
            return { st, nlohmann::json{{"type","shortest_path"},{"paths", arr}} };
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
        
        auto [st, results] = engine.executeGeneralTraversal(
            tv.variable,
            tv.startVertex,
            tv.minDepth,
            tv.maxDepth,
            dir,
            tv.graphName.empty() ? "default" : tv.graphName
        );
        
        if (!st.ok) {
            return { st, nlohmann::json{{"error", "traversal_failed"}, {"message", st.message}} };
        }
        
        // Format results as JSON array
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& result : results) {
            nlohmann::json item;
            item["vertex"] = result.vertex_pk;
            item["depth"] = result.depth;
            item["path"] = result.path;
            item["edges"] = result.edges;
            item["data"] = result.vertex_data;
            arr.push_back(std::move(item));
        }
        
        return { st, nlohmann::json{{"type","traversal"},{"results", arr}} };
    }

    // Join query
    if (tr.join.has_value()) {
        auto& j = *tr.join;
        auto result = engine.executeJoin(j.for_nodes, j.filters, j.let_nodes, j.return_node, j.sort, j.limit);
        if (!result) {
            return { QueryEngine::Status{false, result.error().message()}, nlohmann::json{} };
        }
        auto rows = std::move(*result);
        return { QueryEngine::Status::OK(), nlohmann::json{{"type","join"},{"results", rows}} };
    }

    // Conjunctive (default) query
    auto [st, entities] = engine.executeAndEntitiesWithFallback(tr.query, true);
    nlohmann::json arr = nlohmann::json::array();
    for (auto& e : entities) arr.push_back(nlohmann::json::parse(e.toJson()));
    return { st, nlohmann::json{{"type","and"},{"results", arr}} };
}

} // namespace themis
