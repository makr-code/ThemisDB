/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_runner.cpp                                     ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:59:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     725                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 7d170c769  2026-03-01  feat(build): update CMake configuration for benchmarks an... ║
    • 4f86bf5cd  2026-03-01  feat(query): implement RuntimeReoptimizer for adaptive qu... ║
    • 8554702d5  2026-02-28  fix(query): code-audit – remove unused includes, fix narr... ║
    • 099187166  2026-02-26  feat(query): implement SQL dialect compatibility layer (e... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "query/aql_runner.h"
#include "query/query_plan_visualizer.h"
#include "query/runtime_reoptimizer.h"
#include "storage/base_entity.h"
#include "analytics/nlp_text_analyzer.h"
#include "security/row_level_security.h"
#include "security/access_control_manager.h"
#include <chrono>
#include <fmt/format.h>

namespace themis {

// Lazy-initialized NLP analyzer (thread-safe in C++11+)
static themis::analytics::NlpTextAnalyzer& getNlpAnalyzer() {
    static themis::analytics::NlpTextAnalyzer instance;
    return instance;
}

// Lazy-initialized RuntimeReoptimizer (thread-safe in C++11+)
static RuntimeReoptimizer& getReoptimizer() {
    static RuntimeReoptimizer instance;
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

    // Adaptive re-optimization: compute query hash and start tracking
    auto& reoptimizer = getReoptimizer();
    std::string query_hash = RuntimeReoptimizer::computeQueryHash(normalized_query);
    auto reopt_guard = reoptimizer.beginExecutionGuard(query_hash, 0);
    
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
        reopt_guard.finish(res.size());
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
        reopt_guard.finish(res.size());
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
        reopt_guard.finish(ents.size());
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
            reopt_guard.finish(paths.size());
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
        
        reopt_guard.finish(results.size());
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
        size_t row_count = rows.size();
        reopt_guard.finish(row_count);
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
    reopt_guard.finish(entities.size());
    return Ok(nlohmann::json({{"type","and"},{"results", arr}}));
}

// ── Explain helpers ───────────────────────────────────────────────────────────

namespace {

/// Build a GraphTraversal QueryPlanNode from a parsed traversal query.
///
/// Uses a static cost model (branching_factor^depth) to estimate cost and nodes
/// explored since no real graph statistics are available at plan time.  The
/// algorithm is chosen to mirror GraphQueryOptimizer::selectAlgorithm():
///   - shortestPath: BFS (depth ≤ 5) or Bidirectional (depth > 5)
///   - k-hop traversal: BFS
query::QueryPlanNode buildGraphTraversalPlanNode(
    const AQLTranslator::TranslationResult::TraversalQuery& tv)
{
    using TQ = AQLTranslator::TranslationResult::TraversalQuery;

    // Direction label
    std::string dir_name;
    switch (tv.direction) {
        case TQ::Direction::Outbound: dir_name = "OUTBOUND"; break;
        case TQ::Direction::Inbound:  dir_name = "INBOUND";  break;
        case TQ::Direction::Any:      dir_name = "ANY";       break;
    }

    // Algorithm selection (mirrors GraphQueryOptimizer::selectAlgorithm)
    const int depth = tv.maxDepth > 0 ? tv.maxDepth : 1;
    std::string algo_name;
    if (tv.shortestPath) {
        algo_name = (depth > 5) ? "Bidirectional" : "BFS";
    } else {
        algo_name = "BFS";
    }

    // Static cost estimate: branching_factor^depth (default branching = 4)
    constexpr double kBranchingFactor = 4.0;
    double est_nodes = 1.0;
    for (int d = 0; d < depth; ++d) est_nodes *= kBranchingFactor;
    double est_cost = est_nodes;

    query::QueryPlanNode node;
    node.type             = query::PlanNodeType::GraphTraversal;
    node.description      = algo_name + " on GRAPH '" + tv.graphName + "'";
    node.estimated_cost   = est_cost;
    node.estimated_rows   = static_cast<size_t>(est_nodes);

    node.attributes.push_back("start: " + tv.startVertex);
    node.attributes.push_back("depth: " + std::to_string(tv.minDepth) +
                               ".." + std::to_string(tv.maxDepth));
    node.attributes.push_back("direction: " + dir_name);
    node.attributes.push_back("algorithm: " + algo_name);
    if (tv.shortestPath && !tv.endVertex.empty()) {
        node.attributes.push_back("end: " + tv.endVertex);
    }

    return node;
}

/// Parse + translate @p aql and build the corresponding QueryPlanNode.
///
/// For graph traversal queries a proper GraphTraversal node is produced with
/// algorithm selection and cost estimates.  All other non-conjunctive forms
/// (vector+geo, content+geo, OR, join) fall back to a SeqScan node labelled
/// with the query type.  Conjunctive queries use the existing engine optimizer.
///
/// Returns Err on parse or translation failure.
Result<query::QueryPlanNode> buildExplainPlanNode(
    const std::string& aql, QueryEngine& engine)
{
    query::AQLParser parser;
    auto parseResult = parser.parse(aql);
    if (!parseResult) {
        return Err<query::QueryPlanNode>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            parseResult.error().message()
        );
    }
    auto tr = AQLTranslator::translate(parseResult.value());
    if (!tr.success) {
        return Err<query::QueryPlanNode>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            tr.error_message
        );
    }

    // Graph traversal: build a rich GraphTraversal plan node.
    if (tr.traversal.has_value()) {
        return Ok(buildGraphTraversalPlanNode(*tr.traversal));
    }

    // Non-conjunctive forms: synthetic ConjunctiveQuery → SeqScan with type label.
    if (tr.vector_geo.has_value()) {
        ConjunctiveQuery q;
        q.table = "[vector+geo] " + tr.query.table;
        return Ok(engine.buildExplainPlan(q));
    }
    if (tr.content_geo.has_value()) {
        ConjunctiveQuery q;
        q.table = "[content+geo] " + tr.query.table;
        return Ok(engine.buildExplainPlan(q));
    }
    if (tr.disjunctive.has_value()) {
        ConjunctiveQuery q;
        q.table = "[OR] " + tr.disjunctive->table;
        return Ok(engine.buildExplainPlan(q));
    }
    if (tr.join.has_value()) {
        ConjunctiveQuery q;
        q.table = "[join]";
        return Ok(engine.buildExplainPlan(q));
    }

    // Conjunctive (default) form.
    return Ok(engine.buildExplainPlan(tr.query));
}

} // anonymous namespace

Result<nlohmann::json> explainAql(const std::string& aql, QueryEngine& engine, bool analyze) {
    auto pn = buildExplainPlanNode(aql, engine);
    if (!pn) {
        return Err<nlohmann::json>(pn.error().code(), pn.error().message());
    }
    return Ok(query::QueryPlanVisualizer::toJSON(*pn, analyze));
}

Result<std::string> explainAqlText(const std::string& aql, QueryEngine& engine, bool analyze) {
    auto pn = buildExplainPlanNode(aql, engine);
    if (!pn) {
        return Err<std::string>(pn.error().code(), pn.error().message());
    }
    return Ok(query::QueryPlanVisualizer::toText(*pn, analyze));
}

Result<std::string> explainAqlDot(const std::string& aql, QueryEngine& engine) {
    auto pn = buildExplainPlanNode(aql, engine);
    if (!pn) {
        return Err<std::string>(pn.error().code(), pn.error().message());
    }
    return Ok(query::QueryPlanVisualizer::toDOT(*pn));
}

Result<nlohmann::json> executeMultiStatementAql(const std::string& aql, QueryEngine& engine) {
    // Parse the multi-statement transaction block
    query::AQLParser parser;
    auto blockResult = parser.parseTransactionBlock(aql);
    if (!blockResult) {
        return Err<nlohmann::json>(blockResult.error().code(), blockResult.error().message());
    }
    const auto& block = *blockResult;

    // ROLLBACK: do not execute any statement; return metadata only
    if (block.action == query::AqlTransactionAction::Rollback) {
        return Ok(nlohmann::json({
            {"type", "rollback"},
            {"statements", block.statements.size()}
        }));
    }

    // COMMIT: execute each statement in sequence and collect results
    nlohmann::json results = nlohmann::json::array();
    for (std::size_t i = 0; i < block.statements.size(); ++i) {
        const auto& stmt = block.statements[i];
        if (!stmt) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                fmt::format("Statement {} in transaction block is null", i + 1)
            );
        }

        // Translate and dispatch through the same path as executeAql()
        auto tr = AQLTranslator::translate(stmt);
        if (!tr.success) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("Translation error for statement {} in transaction block: {}",
                            i + 1, tr.error_message)
            );
        }

        // Dispatch through the engine using the already-translated result,
        // following the same pattern as executeAql().
        nlohmann::json stmtResult;

        if (tr.vector_geo.has_value()) {
            auto res = engine.executeVectorGeoQuery(*tr.vector_geo);
            if (!res) {
                return Err<nlohmann::json>(res.error().code(),
                    fmt::format("Execution error for statement {} in transaction block: {}",
                                i + 1, res.error().message()));
            }
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& r : *res) {
                arr.push_back({{"pk", r.pk}, {"distance", r.vector_distance}, {"entity", r.entity}});
            }
            stmtResult = {{"type", "vector_geo"}, {"results", arr}};
        } else if (tr.content_geo.has_value()) {
            auto res = engine.executeContentGeoQuery(*tr.content_geo);
            if (!res) {
                return Err<nlohmann::json>(res.error().code(),
                    fmt::format("Execution error for statement {} in transaction block: {}",
                                i + 1, res.error().message()));
            }
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& r : *res) {
                nlohmann::json row = {{"pk", r.pk}, {"bm25", r.bm25_score}, {"entity", r.entity}};
                if (r.geo_distance.has_value()) row["geo_distance"] = *r.geo_distance;
                arr.push_back(std::move(row));
            }
            stmtResult = {{"type", "content_geo"}, {"results", arr}};
        } else if (tr.disjunctive.has_value()) {
            auto res = engine.executeOrEntitiesWithFallback(*tr.disjunctive, true);
            if (!res) {
                return Err<nlohmann::json>(res.error().code(),
                    fmt::format("Execution error for statement {} in transaction block: {}",
                                i + 1, res.error().message()));
            }
            nlohmann::json arr = nlohmann::json::array();
            for (auto& e : *res) arr.push_back(nlohmann::json::parse(e.toJson()));
            stmtResult = {{"type", "or"}, {"results", arr}};
        } else if (tr.traversal.has_value()) {
            const auto& tv = *tr.traversal;
            if (tv.shortestPath) {
                RecursivePathQuery rq;
                rq.start_node = tv.startVertex;
                rq.end_node   = tv.endVertex;
                rq.graph_id   = tv.graphName;
                rq.max_depth  = tv.maxDepth;
                auto res = engine.executeRecursivePathQuery(rq);
                if (!res) {
                    return Err<nlohmann::json>(res.error().code(),
                        fmt::format("Execution error for statement {} in transaction block: {}",
                                    i + 1, res.error().message()));
                }
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& p : *res) arr.push_back(p);
                stmtResult = {{"type", "shortest_path"}, {"paths", arr}};
            } else {
                TraversalDirection dir;
                switch (tv.direction) {
                    case AQLTranslator::TranslationResult::TraversalQuery::Direction::Outbound:
                        dir = TraversalDirection::OUTBOUND; break;
                    case AQLTranslator::TranslationResult::TraversalQuery::Direction::Inbound:
                        dir = TraversalDirection::INBOUND; break;
                    case AQLTranslator::TranslationResult::TraversalQuery::Direction::Any:
                        dir = TraversalDirection::ANY; break;
                    default:
                        dir = TraversalDirection::OUTBOUND; break;
                }
                auto res = engine.executeGeneralTraversal(
                    tv.startVertex, tv.minDepth, tv.maxDepth, dir,
                    tv.graphName.empty() ? "default" : tv.graphName);
                if (!res) {
                    return Err<nlohmann::json>(res.error().code(),
                        fmt::format("Execution error for statement {} in transaction block: {}",
                                    i + 1, res.error().message()));
                }
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& r : *res) {
                    arr.push_back({{"vertex", r.vertex_pk}, {"depth", r.depth},
                                   {"path", r.path}, {"edges", r.edges}, {"data", r.vertex_data}});
                }
                stmtResult = {{"type", "traversal"}, {"results", arr}};
            }
        } else if (tr.join.has_value()) {
            auto& j = *tr.join;
            auto res = engine.executeJoin(j.for_nodes, j.filters, j.let_nodes, j.return_node, j.sort, j.limit);
            if (!res) {
                return Err<nlohmann::json>(res.error().code(),
                    fmt::format("Execution error for statement {} in transaction block: {}",
                                i + 1, res.error().message()));
            }
            stmtResult = {{"type", "join"}, {"results", *res}};
        } else {
            auto res = engine.executeAndEntitiesWithFallback(tr.query, true);
            if (!res) {
                return Err<nlohmann::json>(res.error().code(),
                    fmt::format("Execution error for statement {} in transaction block: {}",
                                i + 1, res.error().message()));
            }
            nlohmann::json arr = nlohmann::json::array();
            for (auto& e : *res) arr.push_back(nlohmann::json::parse(e.toJson()));
            stmtResult = {{"type", "and"}, {"results", arr}};
        }

        results.push_back(std::move(stmtResult));
    }

    return Ok(nlohmann::json({{"type", "commit"}, {"results", results}}));
}

// ── Row-level security (RLS) wrapper ─────────────────────────────────────────

Result<nlohmann::json> executeAqlWithRLS(
    const std::string& aql,
    QueryEngine& engine,
    security::RLSManager& rls,
    const security::SecurityContext& ctx
) {
    // Execute the AQL query normally first.
    auto result = executeAql(aql, engine);
    if (!result) {
        return result;
    }

    nlohmann::json& doc = *result;

    // Determine the queried collection from the translated query so that
    // the correct RLS policies are applied.  We re-parse the AQL to get
    // the table name.  This is cheap because parseAndTranslateForExplain()
    // is already called for EXPLAIN paths.
    std::string collection;
    {
        query::AQLParser parser;
        auto pr = parser.parse(aql);
        if (pr) {
            auto tr = AQLTranslator::translate(pr.value());
            if (tr.success) {
                if (tr.vector_geo.has_value()) {
                    collection = tr.vector_geo->table;
                } else if (tr.content_geo.has_value()) {
                    collection = tr.content_geo->table;
                } else if (tr.traversal.has_value()) {
                    collection = tr.traversal->graphName;
                } else if (tr.join.has_value()) {
                    // For joins, apply RLS against the primary (first) FOR target.
                    if (!tr.join->for_nodes.empty()) {
                        collection = tr.join->for_nodes.front().collection;
                    }
                } else if (tr.disjunctive.has_value()) {
                    collection = tr.disjunctive->table;
                } else {
                    collection = tr.query.table;
                }
            }
        }
    }

    // Apply RLS filtering to the "results" array inside the response object.
    if (doc.is_object() && doc.contains("results") && doc["results"].is_array()) {
        doc["results"] = rls.filterRows(collection, ctx, doc["results"]);
    }

    return Ok(std::move(doc));
}

// ── Type-annotated execution ──────────────────────────────────────────────

Result<query::AnnotatedQueryResult> executeAqlAnnotated(
    const std::string& aql,
    QueryEngine&       engine)
{
    auto result = executeAql(aql, engine);
    if (!result) {
        return Err<query::AnnotatedQueryResult>(
            result.error().code(),
            result.error().message()
        );
    }

    nlohmann::json doc = std::move(*result);

    // Determine query_type label from the "type" field when present.
    std::string query_type = "unknown";
    if (doc.is_object() && doc.contains("type") && doc["type"].is_string()) {
        query_type = doc["type"].get<std::string>();
    }

    // Infer schema from the "results" array when present; otherwise treat
    // the whole document as a single-row result for schema inference.
    nlohmann::json rows = nlohmann::json::array();
    if (doc.is_object() && doc.contains("results") && doc["results"].is_array()) {
        rows = doc["results"];
    } else if (doc.is_array()) {
        rows = doc;
    }

    query::AnnotatedQueryResult annotated;
    annotated.result = std::move(doc);
    annotated.schema = query::inferResultSchema(rows, query_type);

    return Ok(std::move(annotated));
}

// ── Per-query resource limits ─────────────────────────────────────────────────

Result<nlohmann::json> executeAqlWithLimits(
    const std::string& aql,
    QueryEngine& engine,
    const query::QueryResourceLimits& limits)
{
    // Record start time for timeout enforcement.
    auto start = std::chrono::steady_clock::now();

    // Execute the query normally.
    auto result = executeAql(aql, engine);
    if (!result) {
        return result; // propagate execution errors unchanged
    }

    // Check timeout after execution completes.
    if (limits.timeout_ms > 0) {
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed_ms >= static_cast<long long>(limits.timeout_ms)) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "query exceeded timeout of " + std::to_string(limits.timeout_ms) + " ms"
            );
        }
    }

    nlohmann::json& doc = *result;

    // Determine the results array from the standard response envelope.
    nlohmann::json* rows_ptr = nullptr;
    if (doc.is_object() && doc.contains("results") && doc["results"].is_array()) {
        rows_ptr = &doc["results"];
    } else if (doc.is_array()) {
        rows_ptr = &doc;
    }

    if (rows_ptr != nullptr) {
        // Enforce max_rows limit.
        if (limits.max_rows > 0 && rows_ptr->size() > limits.max_rows) {
            return Err<nlohmann::json>(
                errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED,
                "result row count " + std::to_string(rows_ptr->size()) +
                " exceeds max_rows limit of " + std::to_string(limits.max_rows)
            );
        }

        // Enforce max_memory_bytes limit using serialised JSON size as a proxy.
        if (limits.max_memory_bytes > 0) {
            const std::string serialised = rows_ptr->dump();
            if (serialised.size() > limits.max_memory_bytes) {
                return Err<nlohmann::json>(
                    errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED,
                    "result memory estimate " + std::to_string(serialised.size()) +
                    " bytes exceeds max_memory_bytes limit of " +
                    std::to_string(limits.max_memory_bytes)
                );
            }
        }
    }

    return result;
}

// ── SQL dialect compatibility layer ──────────────────────────────────────────

Result<nlohmann::json> executeSQL(const std::string& sql, QueryEngine& engine) {
    // Parse the SQL statement into an AST.
    query::SQLParser parser;
    auto parse_result = parser.parse(sql);
    if (!parse_result) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            parse_result.error().message()
        );
    }

    // Transpile the SQL AST to an AQL query string.
    query::SQLToAQLTranspiler transpiler;
    auto transpile_result = transpiler.transpile(parse_result.value());
    if (!transpile_result) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            transpile_result.error().message()
        );
    }

    // Execute the generated AQL query through the standard pipeline.
    return executeAql(transpile_result.value(), engine);
}

// ── Query cancellation via request ID ────────────────────────────────────────

Result<nlohmann::json> executeAqlCancellable(
    const std::string& aql,
    QueryEngine& engine,
    const std::string& request_id,
    query::QueryCanceller& canceller)
{
    // registerQuery inserts a fresh token into the canceller map and returns
    // the strong shared_ptr.  ScopedRegistration only stores the request_id
    // for cleanup; it does NOT call registerQuery again (no double registration).
    auto token = canceller.registerQuery(request_id);
    // RAII guard: calls canceller.unregisterQuery(request_id) on scope exit.
    query::QueryCanceller::ScopedRegistration guard(request_id, canceller);

    // Pre-execution cancellation check: the token may have been cancelled
    // between registration and reaching this point.
    if (token->isCancelled()) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_CANCELLED,
            request_id
        );
    }

    // Execute the query through the standard pipeline.
    auto result = executeAql(aql, engine);

    // Post-execution cancellation check: if the query was cancelled while
    // running, report the cancellation rather than a partial result.
    if (token->isCancelled()) {
        return Err<nlohmann::json>(
            errors::ErrorCode::ERR_QUERY_CANCELLED,
            request_id
        );
    }

    return result;
}

} // namespace themis
