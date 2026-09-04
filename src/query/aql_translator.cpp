/**
 * @file aql_translator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=54, H=6, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/aql_translator.h"
#include "query/subquery_optimizer.h"
#include "utils/logger.h"
#include <sstream>
#include <variant>

// Import query types outside the themis namespace for convenience
using themis::query::ArrayLiteralExpr;
using themis::query::BinaryOpExpr;
using themis::query::BinaryOperator;
using themis::query::Expression;
using themis::query::FieldAccessExpr;
using themis::query::FunctionCallExpr;
using themis::query::LiteralExpr;
using themis::query::LiteralValue;
using themis::query::ProximityCallExpr;
using themis::query::SimilarityCallExpr;
using themis::query::UnaryOpExpr;
using themis::query::UnaryOperator;
using themis::query::VariableExpr;

namespace themis {

AQLTranslator::TranslationResult AQLTranslator::translate(const std::shared_ptr<Query>& ast) {
    if (!ast) {
        return TranslationResult::Error("Null AST provided");
    }

    // Phase 4.1: preprocess WITH clauses to determine CTE execution strategy
    std::vector<TranslationResult::CTEExecution> cte_executions = {};

    if (ast->with_clause) {
        query::SubqueryOptimizer optimizer;
        cte_executions.reserve(ast->with_clause->ctes.size());

        for (const auto& cte_def : ast->with_clause->ctes) {
            if (!cte_def.subquery) {
                return TranslationResult::Error("CTE '" + cte_def.name + "' has null query");
            }

            size_t ref_count = countCTEReferences(ast, cte_def.name);
            bool should_materialize = optimizer.shouldMaterializeCTE(cte_def, ref_count);

            TranslationResult::CTEExecution exec;
            exec.name = cte_def.name;
            exec.subquery = cte_def.subquery;
            exec.should_materialize = should_materialize;
            cte_executions.push_back(std::move(exec));
        }
    }

    auto finalizeResult = [&]([[maybe_unused]] TranslationResult&& result) -> TranslationResult {
        attachCTEs(result, std::move(cte_executions));
        return std::move(result);
    };
    
    // Graph-Traversal Unterstützung: Wenn Traversal-Klausel vorhanden ist,
    // übersetzen wir in eine TraversalQuery und umgehen die relationale Pfadlogik.
    if (ast->traversal) {
        TranslationResult::TraversalQuery tq;
        tq.variable = ast->traversal->varVertex;
        tq.minDepth = ast->traversal->minDepth;
        tq.maxDepth = ast->traversal->maxDepth;
        // Mappe Richtung
        switch (ast->traversal->direction) {
            case Query::TraversalNode::Direction::Outbound: tq.direction = TranslationResult::TraversalQuery::Direction::Outbound; break;
            case Query::TraversalNode::Direction::Inbound:  tq.direction = TranslationResult::TraversalQuery::Direction::Inbound;  break;
            case Query::TraversalNode::Direction::Any:      tq.direction = TranslationResult::TraversalQuery::Direction::Any;      break;
        }
        tq.startVertex = ast->traversal->startVertex;
        tq.graphName = ast->traversal->graphName;
        tq.shortestPath = ast->traversal->shortestPath;
        tq.endVertex = ast->traversal->shortestPathTarget;
        return finalizeResult(TranslationResult::SuccessTraversal(std::move(tq)));
    }
    
    // Multi-FOR Join: Wenn mehr als eine FOR-Klausel vorhanden ist, als Join behandeln
    if (ast->for_nodes.size() > 1 || !ast->let_nodes.empty() || ast->collect) {
        // Special-case LET-bound hybrid queries so they do not get treated as joins
        if (ast->for_nodes.size() == 1 && ast->let_nodes.size() == 1 && !ast->collect &&
            ast->sort && ast->sort->specifications.size() == 1) {
            const auto& letNode = ast->let_nodes[0];
            const auto& sortExpr = ast->sort->specifications[0].expression;

            if (sortExpr && sortExpr->getType() == ASTNodeType::Variable) {
                auto varName = std::static_pointer_cast<VariableExpr>(sortExpr)->name;
                if (varName == letNode.variable) {
                    // LET ... = SIMILARITY(...)
                    if (letNode.expression->getType() == ASTNodeType::SimilarityCall) {
                        auto sim = std::static_pointer_cast<SimilarityCallExpr>(letNode.expression);
                        if (sim->arguments.size() >= 2) {
                            if (sim->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                                return TranslationResult::Error("SIMILARITY() LET first arg must be field access");
                            }
                            if (sim->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
                                return TranslationResult::Error("SIMILARITY() LET second arg must be array literal");
                            }

                            std::string vectorField = extractColumnName(sim->arguments[0]);
                            auto arr = std::static_pointer_cast<ArrayLiteralExpr>(sim->arguments[1]);
                            std::vector<float> queryVec = {};

                            queryVec.reserve(arr->elements.size());
                            for (const auto& el : arr->elements) {
                                if (el->getType() != ASTNodeType::Literal) {
                                    return TranslationResult::Error("SIMILARITY() vector elements must be literals");
                                }
                                auto lit = std::static_pointer_cast<LiteralExpr>(el);
                                if (std::holds_alternative<int64_t>(lit->value)) {
                                    queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
                                } else if (std::holds_alternative<double>(lit->value)) {
                                    queryVec.push_back(static_cast<float>(std::get<double>(lit->value)));
                                } else {
                                    return TranslationResult::Error("SIMILARITY() vector must be numeric");
                                }
                            }

                            size_t k = 10;
                            if (sim->arguments.size() == 3) {
                                if (sim->arguments[2]->getType() != ASTNodeType::Literal) {
                                    return TranslationResult::Error("SIMILARITY() k must be integer literal");
                                }
                                auto kLit = std::static_pointer_cast<LiteralExpr>(sim->arguments[2]);
                                if (!std::holds_alternative<int64_t>(kLit->value)) {
                                    return TranslationResult::Error("SIMILARITY() k must be int");
                                }
                                { int64_t _kv = std::get<int64_t>(kLit->value);
                                  if (_kv < 1) {
                                    return TranslationResult::Error("SIMILARITY() k must be >= 1");
                                  }
                                  k = static_cast<size_t>(_kv); }
                            } else if (ast->limit) {
                                k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
                            }

                            std::shared_ptr<Expression> spatialExpr;
                            std::vector<std::shared_ptr<Expression>> extraPreds;
                            extraPreds.reserve(ast->filters.size());
                            for (const auto& filter : ast->filters) {
                                if (!filter || !filter->condition) {
                                    continue;
                                }
                                const auto& cond = filter->condition;
                                if (!spatialExpr && cond->getType() == ASTNodeType::FunctionCall) {
                                    auto fc = std::static_pointer_cast<FunctionCallExpr>(cond);
                                    std::string name = fc->name;
                                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                                    if (name.rfind("st_", 0) == 0) {
                                        spatialExpr = cond;
                                        continue;
                                    }
                                }
                                extraPreds.push_back(cond);
                            }

                            VectorGeoQuery vq;
                            vq.table = ast->for_node.collection;
                            vq.vector_field = vectorField;
                            vq.query_vector = std::move(queryVec);
                            vq.k = k;
                            vq.spatial_filter = spatialExpr;
                            vq.extra_filters = std::move(extraPreds);
                            return finalizeResult(TranslationResult::SuccessVectorGeo(std::move(vq)));
                        }
                    }

                    // LET ... = PROXIMITY(...)
                    if (letNode.expression->getType() == ASTNodeType::ProximityCall) {
                        auto prox = std::static_pointer_cast<ProximityCallExpr>(letNode.expression);
                        if (prox->arguments.size() == 2) {
                            if (prox->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                                return TranslationResult::Error("PROXIMITY() LET first arg must be field access");
                            }
                            if (prox->arguments[1]->getType() != ASTNodeType::ArrayLiteral) {
                                return TranslationResult::Error("PROXIMITY() LET second arg must array literal");
                            }

                            std::string geomField = extractColumnName(prox->arguments[0]);
                            auto arr = std::static_pointer_cast<ArrayLiteralExpr>(prox->arguments[1]);
                            if (arr->elements.size() < 2) {
                                return TranslationResult::Error("PROXIMITY() point needs 2 elements");
                            }

                            std::vector<float> point;
                            point.reserve(2);
                            for (size_t i = 0; i < 2; ++i) {
                                const auto& el = arr->elements[i];
                                if (el->getType() != ASTNodeType::Literal) {
                                    return TranslationResult::Error("PROXIMITY() point elements must literal");
                                }
                                auto lit = std::static_pointer_cast<LiteralExpr>(el);
                                if (std::holds_alternative<int64_t>(lit->value)) {
                                    point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
                                } else if (std::holds_alternative<double>(lit->value)) {
                                    point.push_back(static_cast<float>(std::get<double>(lit->value)));
                                } else {
                                    return TranslationResult::Error("PROXIMITY() numeric only");
                                }
                            }

                            std::shared_ptr<Expression> spatialExpr;
                            std::string fulltextQuery;
                            std::string fulltextField;
                            size_t fulltextLimit = 1000;
                            for (const auto& filter : ast->filters) {
                                if (!filter || !filter->condition) {
                                    continue;
                                }
                                const auto& cond = filter->condition;
                                if (cond->getType() == ASTNodeType::FunctionCall) {
                                    auto fc = std::static_pointer_cast<FunctionCallExpr>(cond);
                                    std::string name = fc->name;
                                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                                    if (name.rfind("st_", 0) == 0 && !spatialExpr) {
                                        spatialExpr = cond;
                                        continue;
                                    }
                                    if (name == "fulltext" && fulltextQuery.empty()) {
                                        if (fc->arguments.size() < 2 || fc->arguments.size() > 3) {
                                            return TranslationResult::Error("FULLTEXT() requires 2-3 args");
                                        }
                                        if (fc->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                                            return TranslationResult::Error("FULLTEXT() first arg field access");
                                        }
                                        if (fc->arguments[1]->getType() != ASTNodeType::Literal) {
                                            return TranslationResult::Error("FULLTEXT() second arg string literal");
                                        }
                                        fulltextField = extractColumnName(fc->arguments[0]);
                                        auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
                                        if (!std::holds_alternative<std::string>(lit->value)) {
                                            return TranslationResult::Error("FULLTEXT() query must string");
                                        }
                                        fulltextQuery = std::get<std::string>(lit->value);
                                        if (fc->arguments.size() == 3) {
                                            if (fc->arguments[2]->getType() != ASTNodeType::Literal) {
                                                return TranslationResult::Error("FULLTEXT() limit must be integer");
                                            }
                                            auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
                                            if (std::holds_alternative<int64_t>(lim->value)) {
                                                { int64_t _lv = std::get<int64_t>(lim->value);
                                                  if (_lv < 0) {
                                                    return TranslationResult::Error("FULLTEXT() limit must be non-negative");
                                                  }
                                                  fulltextLimit = static_cast<size_t>(_lv); }
                                            }
                                        }
                                        continue;
                                    }
                                }
                            }

                            if (fulltextQuery.empty()) {
                                return TranslationResult::Error("PROXIMITY LET requires FULLTEXT filter");
                            }

                            ContentGeoQuery cq;
                            cq.table = ast->for_node.collection;
                            cq.geom_field = geomField;
                            cq.spatial_filter = spatialExpr;
                            cq.boost_by_distance = true;
                            cq.center_point = std::vector<float>{point[0], point[1]};
                            cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
                            cq.text_field = fulltextField;
                            cq.fulltext_query = fulltextQuery;
                            cq.limit = std::min(cq.limit, fulltextLimit);
                            return finalizeResult(TranslationResult::SuccessContentGeo(std::move(cq)));
                        }
                    }
                }
            }
        }

        // Spatial JOIN rule: detect
        //   FOR a IN colA FOR b IN colB FILTER GEO_DISTANCE(a.f, b.f) <= threshold
        // Conditions: exactly 2 FOR clauses, exactly 1 filter, no LET/COLLECT,
        //             filter is (GEO_DISTANCE(field_a, field_b) <= literal) or (<).
        if (ast->for_nodes.size() == 2 &&
            ast->filters.size() == 1 &&
            ast->let_nodes.empty() &&
            !ast->collect)
        {
            const auto& filter_expr = ast->filters[0]->condition;
            bool spatial_join_detected = false;
            TranslationResult::SpatialJoinQuery sjq_candidate;

            if (filter_expr &&
                filter_expr->getType() == ASTNodeType::BinaryOp)
            {
                auto* bin = static_cast<BinaryOpExpr*>(filter_expr.get());
                if ((bin->op == BinaryOperator::Lte ||
                     bin->op == BinaryOperator::Lt) &&
                    bin->left &&
                    bin->left->getType() == ASTNodeType::FunctionCall &&
                    bin->right &&
                    bin->right->getType() == ASTNodeType::Literal)
                {
                    auto* fn  = static_cast<FunctionCallExpr*>(bin->left.get());
                    auto* rhs = static_cast<LiteralExpr*>(bin->right.get());
                    if (fn->name == "GEO_DISTANCE" &&
                        fn->arguments.size() == 2 &&
                        fn->arguments[0]->getType() == ASTNodeType::FieldAccess &&
                        fn->arguments[1]->getType() == ASTNodeType::FieldAccess &&
                        (std::holds_alternative<double>(rhs->value) ||
                         std::holds_alternative<int64_t>(rhs->value)))
                    {
                        double threshold_m = std::holds_alternative<double>(rhs->value)
                            ? std::get<double>(rhs->value)
                            : static_cast<double>(std::get<int64_t>(rhs->value));

                        if (threshold_m > 0.0) {
                            const auto* fa0 = static_cast<FieldAccessExpr*>(fn->arguments[0].get());
                            const auto* fa1 = static_cast<FieldAccessExpr*>(fn->arguments[1].get());
                            // Each argument must be a simple variable.field access.
                            if (fa0->object && fa0->object->getType() == ASTNodeType::Variable &&
                                fa1->object && fa1->object->getType() == ASTNodeType::Variable)
                            {
                                const std::string var0 = static_cast<VariableExpr*>(fa0->object.get())->name;
                                const std::string var1 = static_cast<VariableExpr*>(fa1->object.get())->name;
                                const std::string& bv0 = ast->for_nodes[0].variable;
                                const std::string& bv1 = ast->for_nodes[1].variable;
                                if ((var0 == bv0 && var1 == bv1) ||
                                    (var0 == bv1 && var1 == bv0))
                                {
                                    bool swapped = (var0 == bv1);
                                    sjq_candidate.outer_collection = ast->for_nodes[0].collection;
                                    sjq_candidate.inner_collection = ast->for_nodes[1].collection;
                                    sjq_candidate.outer_var        = bv0;
                                    sjq_candidate.inner_var        = bv1;
                                    sjq_candidate.outer_field      = swapped ? fa1->field : fa0->field;
                                    sjq_candidate.inner_field      = swapped ? fa0->field : fa1->field;
                                    sjq_candidate.threshold_m      = threshold_m;
                                    spatial_join_detected = true;
                                }
                            }
                        }
                    }
                }
            }

            if (spatial_join_detected) {
                return finalizeResult(TranslationResult::SuccessSpatialJoin(std::move(sjq_candidate)));
            }
        }

        TranslationResult::JoinQuery jq;
        jq.for_nodes = ast->for_nodes;
        jq.filters = ast->filters;
        jq.let_nodes = ast->let_nodes;
        jq.return_node = ast->return_node;
        jq.sort = ast->sort;
        jq.limit = ast->limit;
        jq.collect = ast->collect;
        return finalizeResult(TranslationResult::SuccessJoin(std::move(jq)));
    }

    // Hybrid vector+geo detection for single-FOR queries using SORT sugar
    if (ast->sort && ast->sort->specifications.size() == 1 && ast->for_nodes.size() == 1) {
        const auto& spec = ast->sort->specifications[0];
        if (spec.expression) {
            // Specialized SIMILARITY() node path
            if (spec.expression->getType() == ASTNodeType::SimilarityCall) {
                auto sim = std::static_pointer_cast<SimilarityCallExpr>(spec.expression);
                const auto& args = sim->arguments;
                if (args.size() < 2 || args.size() > 3) {
                    return TranslationResult::Error("SIMILARITY() requires 2-3 arguments: SIMILARITY(doc.embedding, [vector] [, k])");
                }
                if (args[0]->getType() != ASTNodeType::FieldAccess) {
                    return TranslationResult::Error("SIMILARITY() first argument must be field access (e.g. doc.embedding)");
                }
                if (args[1]->getType() != ASTNodeType::ArrayLiteral) {
                    return TranslationResult::Error("SIMILARITY() second argument must be an array literal of numbers");
                }

                std::string vectorField = extractColumnName(args[0]);
                auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);
                std::vector<float> queryVec = {};

                queryVec.reserve(arr->elements.size());
                for (const auto& el : arr->elements) {
                    if (el->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("SIMILARITY() vector elements must be numeric literals");
                    }
                    auto lit = std::static_pointer_cast<LiteralExpr>(el);
                    if (std::holds_alternative<int64_t>(lit->value)) {
                        queryVec.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
                    } else if (std::holds_alternative<double>(lit->value)) {
                        queryVec.push_back(static_cast<float>(std::get<double>(lit->value)));
                    } else {
                        return TranslationResult::Error("SIMILARITY() vector elements must be numeric (int or double)");
                    }
                }

                size_t k = 10;
                if (args.size() == 3) {
                    if (args[2]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("SIMILARITY() third argument k must be integer literal");
                    }
                    auto kLit = std::static_pointer_cast<LiteralExpr>(args[2]);
                    if (!std::holds_alternative<int64_t>(kLit->value)) {
                        return TranslationResult::Error("SIMILARITY() k must be integer literal");
                    }
                    { int64_t _kv = std::get<int64_t>(kLit->value);
                      if (_kv < 1) {
                        return TranslationResult::Error("SIMILARITY() k must be >= 1");
                      }
                      k = static_cast<size_t>(_kv); }
                } else if (ast->limit) {
                    k = static_cast<size_t>(std::max<int64_t>(0, ast->limit->count));
                }

                std::shared_ptr<Expression> spatialExpr;
                std::vector<std::shared_ptr<Expression>> extraPreds;
                extraPreds.reserve(ast->filters.size());
                for (const auto& filter : ast->filters) {
                    if (!filter || !filter->condition) {
                        continue;
                    }
                    const auto& cond = filter->condition;
                    if (!spatialExpr && cond->getType() == ASTNodeType::FunctionCall) {
                        auto fc = std::static_pointer_cast<FunctionCallExpr>(cond);
                        std::string name = fc->name;
                        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                        if (name.rfind("st_", 0) == 0) {
                            spatialExpr = cond;
                            continue;
                        }
                    }
                    extraPreds.push_back(cond);
                }

                VectorGeoQuery vq;
                vq.table = ast->for_node.collection;
                vq.vector_field = vectorField;
                vq.query_vector = std::move(queryVec);
                vq.k = k;
                vq.spatial_filter = spatialExpr;
                vq.extra_filters = std::move(extraPreds);
                return finalizeResult(TranslationResult::SuccessVectorGeo(std::move(vq)));
            }

            if (spec.expression->getType() == ASTNodeType::ProximityCall) {
                auto prox = std::static_pointer_cast<ProximityCallExpr>(spec.expression);
                const auto& args = prox->arguments;
                if (args.size() != 2) {
                    return TranslationResult::Error("PROXIMITY() requires exactly 2 arguments: PROXIMITY(doc.location, [lon,lat])");
                }
                if (args[0]->getType() != ASTNodeType::FieldAccess) {
                    return TranslationResult::Error("PROXIMITY() first argument must be field access (e.g. doc.location)");
                }
                if (args[1]->getType() != ASTNodeType::ArrayLiteral) {
                    return TranslationResult::Error("PROXIMITY() second argument must be array literal [lon, lat]");
                }

                std::string geomField = extractColumnName(args[0]);
                auto arr = std::static_pointer_cast<ArrayLiteralExpr>(args[1]);
                if (arr->elements.size() < 2) {
                    return TranslationResult::Error("PROXIMITY() point array must have at least 2 numeric elements [lon, lat]");
                }

                std::vector<float> point;
                point.reserve(2);
                for (size_t i = 0; i < 2; ++i) {
                    const auto& el = arr->elements[i];
                    if (el->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("PROXIMITY() point elements must be numeric literals");
                    }
                    auto lit = std::static_pointer_cast<LiteralExpr>(el);
                    if (std::holds_alternative<int64_t>(lit->value)) {
                        point.push_back(static_cast<float>(std::get<int64_t>(lit->value)));
                    } else if (std::holds_alternative<double>(lit->value)) {
                        point.push_back(static_cast<float>(std::get<double>(lit->value)));
                    } else {
                        return TranslationResult::Error("PROXIMITY() point elements must be numeric");
                    }
                }

                std::shared_ptr<Expression> spatialExpr;
                std::string fulltextQuery;
                std::string fulltextField;
                size_t fulltextLimit = 1000;
                for (const auto& filter : ast->filters) {
                    if (!filter || !filter->condition) {
                        continue;
                    }
                    const auto& cond = filter->condition;
                    if (cond->getType() == ASTNodeType::FunctionCall) {
                        auto fc = std::static_pointer_cast<FunctionCallExpr>(cond);
                        std::string name = fc->name;
                        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                        if (name.rfind("st_", 0) == 0 && !spatialExpr) {
                            spatialExpr = cond;
                            continue;
                        }
                        if (name == "fulltext" && fulltextQuery.empty()) {
                            if (fc->arguments.size() < 2 || fc->arguments.size() > 3) {
                                return TranslationResult::Error("FULLTEXT() requires 2-3 arguments inside PROXIMITY hybrid");
                            }
                            if (fc->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                                return TranslationResult::Error("FULLTEXT() first argument must be field access");
                            }
                            if (fc->arguments[1]->getType() != ASTNodeType::Literal) {
                                return TranslationResult::Error("FULLTEXT() second argument must be literal string");
                            }
                            fulltextField = extractColumnName(fc->arguments[0]);
                            auto lit = std::static_pointer_cast<LiteralExpr>(fc->arguments[1]);
                            if (!std::holds_alternative<std::string>(lit->value)) {
                                return TranslationResult::Error("FULLTEXT() query must be string");
                            }
                            fulltextQuery = std::get<std::string>(lit->value);
                            if (fc->arguments.size() == 3) {
                                if (fc->arguments[2]->getType() != ASTNodeType::Literal) {
                                    return TranslationResult::Error("FULLTEXT() limit must be integer");
                                }
                                auto lim = std::static_pointer_cast<LiteralExpr>(fc->arguments[2]);
                                if (std::holds_alternative<int64_t>(lim->value)) {
                                    { int64_t _lv = std::get<int64_t>(lim->value);
                                      if (_lv < 0) {
                                        return TranslationResult::Error("FULLTEXT() limit must be non-negative");
                                      }
                                      fulltextLimit = static_cast<size_t>(_lv); }
                                }
                            }
                            continue;
                        }
                    }
                }

                if (fulltextQuery.empty()) {
                    return TranslationResult::Error("PROXIMITY() requires a FULLTEXT() filter for Content+Geo hybrid");
                }

                ContentGeoQuery cq;
                cq.table = ast->for_node.collection;
                cq.geom_field = geomField;
                cq.spatial_filter = spatialExpr;
                cq.boost_by_distance = true;
                cq.center_point = std::vector<float>{point[0], point[1]};
                cq.limit = ast->limit ? static_cast<size_t>(std::max<int64_t>(0, ast->limit->count)) : 100;
                cq.text_field = fulltextField;
                cq.fulltext_query = fulltextQuery;
                cq.limit = std::min(cq.limit, fulltextLimit);
                return finalizeResult(TranslationResult::SuccessContentGeo(std::move(cq)));
            }

        }
    }

    // Single-FOR Query: Standard ConjunctiveQuery für einfache Queries
    ConjunctiveQuery query;
    query.table = ast->for_node.collection;
    
    // Process FILTER clauses
    std::string error;
    
    // Check if any filter contains OR - if so, use DisjunctiveQuery
    bool hasOr = false;
    for (const auto& filter : ast->filters) {
        if (filter && filter->condition && containsOr(filter->condition)) {
            hasOr = true;
            break;
        }
    }
    
    if (hasOr) {
        // Build DisjunctiveQuery using DNF conversion
        DisjunctiveQuery disjQuery;
        disjQuery.table = ast->for_node.collection;
        
        // Convert all filters to DNF and merge
        for (const auto& filter : ast->filters) {
            if (!filter || !filter->condition) {
                return TranslationResult::Error("Invalid filter node");
            }
            
            auto disjuncts = convertToDNF(filter->condition, disjQuery.table, error);
            if (!error.empty()) {
                return TranslationResult::Error("OR filter translation failed: " + error);
            }
            
            // Merge disjuncts (for now, just append - proper DNF merge would distribute)
            if (disjQuery.disjuncts.empty()) {
                disjQuery.disjuncts = std::move(disjuncts);
            } else {
                // Multiple FILTERs with OR: AND-combine via cartesian product (DNF expansion).
                // If existing disjuncts = [A, B] and new disjuncts = [C, D], the result is
                // [A∧C, A∧D, B∧C, B∧D] — each pair of conjuncts is merged.
                constexpr size_t kMaxDNFDisjuncts = 1000;
                if (disjQuery.disjuncts.size() * disjuncts.size() > kMaxDNFDisjuncts) {
                    return TranslationResult::Error(
                        "OR query too complex: DNF expansion would produce " +
                        std::to_string(disjQuery.disjuncts.size() * disjuncts.size()) +
                        " disjuncts (limit: " + std::to_string(kMaxDNFDisjuncts) + ")");
                }
                std::vector<ConjunctiveQuery> merged = {};

                merged.reserve(disjQuery.disjuncts.size() * disjuncts.size());
                for (const auto& existing : disjQuery.disjuncts) {
                    for (const auto& incoming : disjuncts) {
                        ConjunctiveQuery combined;
                        combined.table = existing.table;
                        // Merge equality predicates (Q3: pre-allocate)
                        combined.predicates.reserve(existing.predicates.size() + incoming.predicates.size());
                        combined.predicates = existing.predicates;
                        combined.predicates.insert(combined.predicates.end(),
                                                   incoming.predicates.begin(),
                                                   incoming.predicates.end());
                        // Merge range predicates (Q3: pre-allocate)
                        combined.rangePredicates.reserve(existing.rangePredicates.size() + incoming.rangePredicates.size());
                        combined.rangePredicates = existing.rangePredicates;
                        combined.rangePredicates.insert(combined.rangePredicates.end(),
                                                        incoming.rangePredicates.begin(),
                                                        incoming.rangePredicates.end());
                        // Preserve first non-null optional predicates
                        combined.fulltextPredicate  = existing.fulltextPredicate  ? existing.fulltextPredicate  : incoming.fulltextPredicate;
                        combined.phrasePredicate    = existing.phrasePredicate    ? existing.phrasePredicate    : incoming.phrasePredicate;
                        combined.fuzzyPredicate     = existing.fuzzyPredicate     ? existing.fuzzyPredicate     : incoming.fuzzyPredicate;
                        combined.spatialPredicate   = existing.spatialPredicate   ? existing.spatialPredicate   : incoming.spatialPredicate;
                        combined.pk_eq              = existing.pk_eq              ? existing.pk_eq              : incoming.pk_eq;
                        merged.push_back(std::move(combined));
                    }
                }
                disjQuery.disjuncts = std::move(merged);
            }
        }
        
        // Process SORT + LIMIT
        if (ast->sort) {
            disjQuery.orderBy = extractOrderBy(ast->sort, ast->limit);
        }
        
        return finalizeResult(TranslationResult::SuccessDisjunctive(std::move(disjQuery)));
    }
    
    // No OR: Standard conjunctive query
    for (const auto& filter : ast->filters) {
        if (!filter || !filter->condition) {
            return TranslationResult::Error("Invalid filter node");
        }
        
        // Check if filter is a FULLTEXT function call
        if (filter->condition->getType() == ASTNodeType::FunctionCall) {
            auto funcCall = std::static_pointer_cast<FunctionCallExpr>(filter->condition);
            std::string funcName = funcCall->name;
            std::transform(funcName.begin(), funcName.end(), funcName.begin(), ::tolower);
            
            if (funcName == "fulltext") {
                // Parse FULLTEXT(column, query [, limit])
                if (funcCall->arguments.size() < 2 || funcCall->arguments.size() > 3) {
                    return TranslationResult::Error("FULLTEXT() requires 2-3 arguments: FULLTEXT(column, query [, limit])");
                }
                
                // Extract column (must be field access: doc.column)
                if (funcCall->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                    return TranslationResult::Error("FULLTEXT() first argument must be field access (e.g., doc.content)");
                }
                std::string column = extractColumnName(funcCall->arguments[0]);
                
                // Extract query string (must be literal)
                if (funcCall->arguments[1]->getType() != ASTNodeType::Literal) {
                    return TranslationResult::Error("FULLTEXT() second argument must be string literal");
                }
                auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
                if (!std::holds_alternative<std::string>(queryLiteral->value)) {
                    return TranslationResult::Error("FULLTEXT() query must be a string");
                }
                std::string queryStr = std::get<std::string>(queryLiteral->value);
                
                // Extract optional limit
                size_t limit = 1000; // default
                if (funcCall->arguments.size() == 3) {
                    if (funcCall->arguments[2]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("FULLTEXT() third argument (limit) must be integer literal");
                    }
                    auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
                    if (std::holds_alternative<int64_t>(limitLiteral->value)) {
                        { int64_t _lv = std::get<int64_t>(limitLiteral->value);
                          if (_lv < 0) {
                            return TranslationResult::Error("limit must be non-negative");
                          }
                          limit = static_cast<size_t>(_lv); }
                    } else {
                        return TranslationResult::Error("FULLTEXT() limit must be an integer");
                    }
                }
                
                // Set fulltext predicate
                query.fulltextPredicate = PredicateFulltext{column, queryStr, limit};
                continue; // Skip normal predicate extraction for this filter
            }
            
            // Handle PHRASE function
            if (funcName == "phrase") {
                // Parse PHRASE(column, phrase [, limit])
                if (funcCall->arguments.size() < 2 || funcCall->arguments.size() > 3) {
                    return TranslationResult::Error("PHRASE() requires 2-3 arguments: PHRASE(column, phrase [, limit])");
                }
                
                // Extract column (must be field access: doc.column)
                if (funcCall->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                    return TranslationResult::Error("PHRASE() first argument must be field access (e.g., doc.content)");
                }
                std::string column = extractColumnName(funcCall->arguments[0]);
                
                // Extract phrase string (must be literal)
                if (funcCall->arguments[1]->getType() != ASTNodeType::Literal) {
                    return TranslationResult::Error("PHRASE() second argument must be string literal");
                }
                auto phraseLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
                if (!std::holds_alternative<std::string>(phraseLiteral->value)) {
                    return TranslationResult::Error("PHRASE() phrase must be a string");
                }
                std::string phraseStr = std::get<std::string>(phraseLiteral->value);
                
                // Extract optional limit
                size_t limit = 1000; // default
                if (funcCall->arguments.size() == 3) {
                    if (funcCall->arguments[2]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("PHRASE() third argument (limit) must be integer literal");
                    }
                    auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
                    if (std::holds_alternative<int64_t>(limitLiteral->value)) {
                        { int64_t _lv = std::get<int64_t>(limitLiteral->value);
                          if (_lv < 0) {
                            return TranslationResult::Error("limit must be non-negative");
                          }
                          limit = static_cast<size_t>(_lv); }
                    } else {
                        return TranslationResult::Error("PHRASE() limit must be an integer");
                    }
                }
                
                // Set phrase predicate
                query.phrasePredicate = PredicatePhrase{column, phraseStr, limit};
                continue; // Skip normal predicate extraction for this filter
            }
            
            // Handle FUZZY function
            if (funcName == "fuzzy") {
                // Parse FUZZY(column, query [, maxDistance] [, limit])
                if (funcCall->arguments.size() < 2 || funcCall->arguments.size() > 4) {
                    return TranslationResult::Error("FUZZY() requires 2-4 arguments: FUZZY(column, query [, maxDistance] [, limit])");
                }
                
                // Extract column (must be field access: doc.column)
                if (funcCall->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                    return TranslationResult::Error("FUZZY() first argument must be field access (e.g., doc.content)");
                }
                std::string column = extractColumnName(funcCall->arguments[0]);
                
                // Extract query string (must be literal)
                if (funcCall->arguments[1]->getType() != ASTNodeType::Literal) {
                    return TranslationResult::Error("FUZZY() second argument must be string literal");
                }
                auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
                if (!std::holds_alternative<std::string>(queryLiteral->value)) {
                    return TranslationResult::Error("FUZZY() query must be a string");
                }
                std::string queryStr = std::get<std::string>(queryLiteral->value);
                
                // Extract optional maxDistance (default 2)
                int maxDistance = 2;
                if (funcCall->arguments.size() >= 3) {
                    if (funcCall->arguments[2]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("FUZZY() third argument (maxDistance) must be integer literal");
                    }
                    auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
                    if (std::holds_alternative<int64_t>(distLiteral->value)) {
                        const int64_t distVal = std::get<int64_t>(distLiteral->value);
                        if (distVal < 0 || distVal > 1000) {
                            return TranslationResult::Error("FUZZY() maxDistance must be between 0 and 1000");
                        }
                        maxDistance = static_cast<int>(distVal);
                    } else {
                        return TranslationResult::Error("FUZZY() maxDistance must be an integer");
                    }
                }
                
                // Extract optional limit (default 1000)
                size_t limit = 1000;
                if (funcCall->arguments.size() == 4) {
                    if (funcCall->arguments[3]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("FUZZY() fourth argument (limit) must be integer literal");
                    }
                    auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[3]);
                    if (std::holds_alternative<int64_t>(limitLiteral->value)) {
                        const int64_t limitVal = std::get<int64_t>(limitLiteral->value);
                        if (limitVal < 0) {
                            return TranslationResult::Error("FUZZY() limit must be non-negative");
                        }
                        limit = static_cast<size_t>(limitVal);
                    } else {
                        return TranslationResult::Error("FUZZY() limit must be an integer");
                    }
                }
                
                // Set fuzzy predicate
                query.fuzzyPredicate = PredicateFuzzy{column, queryStr, maxDistance, limit};
                continue; // Skip normal predicate extraction for this filter
            }
            
            // Handle ST_* spatial functions (G3 - AQL Parser Integration)
            // Use compare for clearer intent than rfind
            if (funcName.compare(0, 3, "st_") == 0) {
                // Recognize ST_Intersects, ST_Within, ST_Contains, ST_DWithin
                PredicateSpatial::Operation operation;
                
                if (funcName == "st_intersects") {
                    operation = PredicateSpatial::Operation::Intersects;
                } else if (funcName == "st_within") {
                    operation = PredicateSpatial::Operation::Within;
                } else if (funcName == "st_contains") {
                    operation = PredicateSpatial::Operation::Contains;
                } else if (funcName == "st_dwithin") {
                    operation = PredicateSpatial::Operation::DWithin;
                } else {
                    return TranslationResult::Error("Unsupported spatial function: " + funcName);
                }
                
                // Parse ST_*(column, geometry [, distance])
                if (operation == PredicateSpatial::Operation::DWithin) {
                    if (funcCall->arguments.size() != 3) {
                        return TranslationResult::Error("ST_DWithin() requires 3 arguments: ST_DWithin(column, geometry, distance)");
                    }
                } else {
                    if (funcCall->arguments.size() != 2) {
                        return TranslationResult::Error(funcName + "() requires 2 arguments: " + funcName + "(column, geometry)");
                    }
                }
                
                // Extract column (must be field access: doc.location)
                if (funcCall->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                    return TranslationResult::Error(funcName + "() first argument must be field access (e.g., doc.location)");
                }
                std::string column = extractColumnName(funcCall->arguments[0]);
                
                // Store query geometry expression for runtime evaluation
                auto queryGeomExpr = funcCall->arguments[1];
                
                // Extract optional distance for ST_DWithin
                std::optional<double> distance = {};

                if (operation == PredicateSpatial::Operation::DWithin) {
                    if (funcCall->arguments[2]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("ST_DWithin() distance must be numeric literal");
                    }
                    auto distLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
                    if (std::holds_alternative<int64_t>(distLiteral->value)) {
                        distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
                    } else if (std::holds_alternative<double>(distLiteral->value)) {
                        distance = std::get<double>(distLiteral->value);
                    } else {
                        return TranslationResult::Error("ST_DWithin() distance must be numeric");
                    }
                }
                
                // Compute bbox from queryGeomExpr for index filtering
                // For now, support simple bbox literals in the form: [[minx,miny],[maxx,maxy]]
                std::optional<std::pair<double, double>> bbox_min;
                std::optional<std::pair<double, double>> bbox_max;
                
                // Helper to parse a simple bbox literal
                auto parseSimpleBbox = [](const std::string &text,
                                           std::optional<std::pair<double, double>> &out_min,
                                           std::optional<std::pair<double, double>> &out_max) {
                    // Extract numeric values from string like [[10.0,50.0],[11.0,51.0]]
                    std::string numericOnly;
                    numericOnly.reserve(text.size());
                    for (char c : text) {
                        if ((c >= '0' && c <= '9') || c == '-' || c == '+' ||
                            c == '.' || c == 'e' || c == 'E') {
                            numericOnly.push_back(c);
                        } else {
                            numericOnly.push_back(' ');
                        }
                    }

                    std::stringstream ss(numericOnly);
                    double minx, miny, maxx, maxy;
                    if (!(ss >> minx >> miny >> maxx >> maxy)) {
                        return; // Parse failed
                    }
                    
                    // Validate bbox is valid (min <= max)
                    if (minx > maxx || miny > maxy) {
                        THEMIS_WARN("Invalid bbox: min must be <= max (minx={}, maxx={}, miny={}, maxy={})",
                                   minx, maxx, miny, maxy);
                        return; // Invalid bbox
                    }
                    
                    out_min = std::make_pair(minx, miny);
                    out_max = std::make_pair(maxx, maxy);
                };
                
                // Try to extract simple bbox for literal geometry expressions
                if (queryGeomExpr->getType() == ASTNodeType::Literal) {
                    auto lit = std::static_pointer_cast<LiteralExpr>(queryGeomExpr);
                    if (std::holds_alternative<std::string>(lit->value)) {
                        const std::string &geojson = std::get<std::string>(lit->value);
                        // Try to parse as simple bbox literal: [[minx,miny],[maxx,maxy]]
                        parseSimpleBbox(geojson, bbox_min, bbox_max);
                    }
                }
                
                // Only set spatial predicate if we have a valid bbox
                // Note: QueryEngine::executeAndKeys requires bbox_min/bbox_max
                if (bbox_min && bbox_max) {
                    query.spatialPredicate = PredicateSpatial{
                        column,
                        operation,
                        queryGeomExpr,
                        distance,
                        bbox_min,
                        bbox_max
                    };
                    continue; // Skip normal predicate extraction for this filter
                } else {
                    // Non-literal geometry expressions cannot be resolved at translation time.
                    // Silently dropping the spatial filter would bypass the geo-fence entirely.
                    return TranslationResult::Error(
                        "ST_* spatial filter with non-literal geometry is not supported; "
                        "use a literal coordinate string or WKT geometry (e.g. [[minx,miny],[maxx,maxy]])");
                }
            }
        }
        
        // Check if filter contains FULLTEXT combined with AND
        // Helper to recursively find FULLTEXT in AND tree
        std::function<std::shared_ptr<FunctionCallExpr>(const std::shared_ptr<Expression>&)> findFulltext;
        findFulltext = [&]([[maybe_unused]] const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
            if (!e) {
              return nullptr;
            }
            
            if (e->getType() == ASTNodeType::FunctionCall) {
                auto fc = std::static_pointer_cast<FunctionCallExpr>(e);
                std::string name = fc->name;
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (name == "fulltext") {
                  return fc;
                }
            }
            
            if (e->getType() == ASTNodeType::BinaryOp) {
                auto bo = std::static_pointer_cast<BinaryOpExpr>(e);
                if (bo->op == BinaryOperator::And) {
                    auto left = findFulltext(bo->left);
                    if (left) {
                      return left;
                    }
                    return findFulltext(bo->right);
                }
            }
            
            return nullptr;
        };
        
        // Helper to recursively find ST_* spatial function in AND tree
        std::function<std::shared_ptr<FunctionCallExpr>(const std::shared_ptr<Expression>&)> findSpatial;
        findSpatial = [&]([[maybe_unused]] const std::shared_ptr<Expression>& e) -> std::shared_ptr<FunctionCallExpr> {
            if (!e) {
              return nullptr;
            }
            
            if (e->getType() == ASTNodeType::FunctionCall) {
                auto fc = std::static_pointer_cast<FunctionCallExpr>(e);
                std::string name = fc->name;
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (name.compare(0, 3, "st_") == 0) {
                  return fc;
                }
            }
            
            if (e->getType() == ASTNodeType::BinaryOp) {
                auto bo = std::static_pointer_cast<BinaryOpExpr>(e);
                if (bo->op == BinaryOperator::And) {
                    auto left = findSpatial(bo->left);
                    if (left) {
                      return left;
                    }
                    return findSpatial(bo->right);
                }
            }
            
            return nullptr;
        };
        
        // Helper to collect all non-FULLTEXT and non-spatial predicates from AND tree
        std::function<void(const std::shared_ptr<Expression>&, std::vector<std::shared_ptr<Expression>>&)> collectNonFulltext;
        collectNonFulltext = [&](const std::shared_ptr<Expression>& e, std::vector<std::shared_ptr<Expression>>& preds) {
            if (!e) {
              return;
            }
            
            if (e->getType() == ASTNodeType::FunctionCall) {
                auto fc = std::static_pointer_cast<FunctionCallExpr>(e);
                std::string name = fc->name;
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (name != "fulltext" && name.compare(0, 3, "st_") != 0) {
                    preds.push_back(e); // Non-FULLTEXT, non-spatial function
                }
                // Skip FULLTEXT and ST_* themselves
                return;
            }
            
            if (e->getType() == ASTNodeType::BinaryOp) {
                auto bo = std::static_pointer_cast<BinaryOpExpr>(e);
                if (bo->op == BinaryOperator::And) {
                    collectNonFulltext(bo->left, preds);
                    collectNonFulltext(bo->right, preds);
                    return;
                }
            }
            
            // Leaf predicate (equality, range, etc.)
            preds.push_back(e);
        };
        
        if (filter->condition->getType() == ASTNodeType::BinaryOp) {
            auto binOp = std::static_pointer_cast<BinaryOpExpr>(filter->condition);
            
            if (binOp->op == BinaryOperator::And) {
                auto fulltextFunc = findFulltext(filter->condition);
                
                if (fulltextFunc) {
                    // Parse FULLTEXT part
                    if (fulltextFunc->arguments.size() < 2 || fulltextFunc->arguments.size() > 3) {
                        return TranslationResult::Error("FULLTEXT() requires 2-3 arguments");
                    }
                    
                    if (fulltextFunc->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                        return TranslationResult::Error("FULLTEXT() first argument must be field access");
                    }
                    std::string column = extractColumnName(fulltextFunc->arguments[0]);
                    
                    if (fulltextFunc->arguments[1]->getType() != ASTNodeType::Literal) {
                        return TranslationResult::Error("FULLTEXT() second argument must be string literal");
                    }
                    auto queryLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[1]);
                    if (!std::holds_alternative<std::string>(queryLiteral->value)) {
                        return TranslationResult::Error("FULLTEXT() query must be a string");
                    }
                    std::string queryStr = std::get<std::string>(queryLiteral->value);
                    
                    size_t limit = 1000;
                    if (fulltextFunc->arguments.size() == 3) {
                        if (fulltextFunc->arguments[2]->getType() != ASTNodeType::Literal) {
                            return TranslationResult::Error("FULLTEXT() limit must be integer literal");
                        }
                        auto limitLiteral = std::static_pointer_cast<LiteralExpr>(fulltextFunc->arguments[2]);
                        if (std::holds_alternative<int64_t>(limitLiteral->value)) {
                            { int64_t _lv = std::get<int64_t>(limitLiteral->value);
                              if (_lv < 0) {
                                return TranslationResult::Error("limit must be non-negative");
                              }
                              limit = static_cast<size_t>(_lv); }
                        } else {
                            return TranslationResult::Error("FULLTEXT() limit must be an integer");
                        }
                    }
                    
                    query.fulltextPredicate = PredicateFulltext{column, queryStr, limit};
                    
                    // Collect all non-FULLTEXT predicates
                    std::vector<std::shared_ptr<Expression>> predicateExprs;
                    collectNonFulltext(filter->condition, predicateExprs);
                    
                    // Extract each predicate
                    for (const auto& predExpr : predicateExprs) {
                        if (!extractPredicates(predExpr, query.predicates, query.rangePredicates, error)) {
                            return TranslationResult::Error("Filter translation failed: " + error);
                        }
                    }
                    
                    continue; // Successfully handled FULLTEXT AND <predicates>
                }
                
                // Check for ST_* spatial predicate in AND combination
                auto spatialFunc = findSpatial(filter->condition);
                
                if (spatialFunc) {
                    // Parse ST_* spatial function part
                    std::string funcName = spatialFunc->name;
                    std::transform(funcName.begin(), funcName.end(), funcName.begin(), ::tolower);
                    
                    PredicateSpatial::Operation operation;
                    if (funcName == "st_intersects") {
                        operation = PredicateSpatial::Operation::Intersects;
                    } else if (funcName == "st_within") {
                        operation = PredicateSpatial::Operation::Within;
                    } else if (funcName == "st_contains") {
                        operation = PredicateSpatial::Operation::Contains;
                    } else if (funcName == "st_dwithin") {
                        operation = PredicateSpatial::Operation::DWithin;
                    } else {
                        return TranslationResult::Error("Unsupported spatial function: " + funcName);
                    }
                    
                    // Parse ST_*(column, geometry [, distance])
                    if (operation == PredicateSpatial::Operation::DWithin) {
                        if (spatialFunc->arguments.size() != 3) {
                            return TranslationResult::Error("ST_DWithin() requires 3 arguments: ST_DWithin(column, geometry, distance)");
                        }
                    } else {
                        if (spatialFunc->arguments.size() != 2) {
                            return TranslationResult::Error(funcName + "() requires 2 arguments: " + funcName + "(column, geometry)");
                        }
                    }
                    
                    // Extract column (must be field access: doc.location)
                    if (spatialFunc->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                        return TranslationResult::Error(funcName + "() first argument must be field access (e.g., doc.location)");
                    }
                    std::string column = extractColumnName(spatialFunc->arguments[0]);
                    
                    // Store query geometry expression for runtime evaluation
                    auto queryGeomExpr = spatialFunc->arguments[1];
                    
                    // Extract optional distance for ST_DWithin
                    std::optional<double> distance = {};

                    if (operation == PredicateSpatial::Operation::DWithin) {
                        if (spatialFunc->arguments[2]->getType() != ASTNodeType::Literal) {
                            return TranslationResult::Error("ST_DWithin() distance must be numeric literal");
                        }
                        auto distLiteral = std::static_pointer_cast<LiteralExpr>(spatialFunc->arguments[2]);
                        if (std::holds_alternative<int64_t>(distLiteral->value)) {
                            distance = static_cast<double>(std::get<int64_t>(distLiteral->value));
                        } else if (std::holds_alternative<double>(distLiteral->value)) {
                            distance = std::get<double>(distLiteral->value);
                        } else {
                            return TranslationResult::Error("ST_DWithin() distance must be numeric");
                        }
                    }
                    
                    // Compute bbox from queryGeomExpr for index filtering
                    std::optional<std::pair<double, double>> bbox_min;
                    std::optional<std::pair<double, double>> bbox_max;
                    
                    // Helper to parse a simple bbox literal (reuse from earlier)
                    auto parseSimpleBbox = [](const std::string &text,
                                               std::optional<std::pair<double, double>> &out_min,
                                               std::optional<std::pair<double, double>> &out_max) {
                        // Extract numeric values from string like [[10.0,50.0],[11.0,51.0]]
                        std::string numericOnly;
                        numericOnly.reserve(text.size());
                        for (char c : text) {
                            if ((c >= '0' && c <= '9') || c == '-' || c == '+' ||
                                c == '.' || c == 'e' || c == 'E') {
                                numericOnly.push_back(c);
                            } else {
                                numericOnly.push_back(' ');
                            }
                        }

                        std::stringstream ss(numericOnly);
                        double minx, miny, maxx, maxy;
                        if (!(ss >> minx >> miny >> maxx >> maxy)) {
                            return; // Parse failed
                        }
                        
                        // Validate bbox is valid (min <= max)
                        if (minx > maxx || miny > maxy) {
                            THEMIS_WARN("Invalid bbox: min must be <= max (minx={}, maxx={}, miny={}, maxy={})",
                                       minx, maxx, miny, maxy);
                            return; // Invalid bbox
                        }
                        
                        out_min = std::make_pair(minx, miny);
                        out_max = std::make_pair(maxx, maxy);
                    };
                    
                    // Try to extract simple bbox for literal geometry expressions
                    if (queryGeomExpr->getType() == ASTNodeType::Literal) {
                        auto lit = std::static_pointer_cast<LiteralExpr>(queryGeomExpr);
                        if (std::holds_alternative<std::string>(lit->value)) {
                            const std::string &geojson = std::get<std::string>(lit->value);
                            parseSimpleBbox(geojson, bbox_min, bbox_max);
                        }
                    }
                    
                    // Only set spatial predicate if we have a valid bbox
                    if (bbox_min && bbox_max) {
                        query.spatialPredicate = PredicateSpatial{
                            column,
                            operation,
                            queryGeomExpr,
                            distance,
                            bbox_min,
                            bbox_max
                        };
                        
                        // Collect all non-spatial predicates
                        std::vector<std::shared_ptr<Expression>> predicateExprs;
                        collectNonFulltext(filter->condition, predicateExprs);
                        
                        // Extract each predicate
                        for (const auto& predExpr : predicateExprs) {
                            if (!extractPredicates(predExpr, query.predicates, query.rangePredicates, error)) {
                                return TranslationResult::Error("Filter translation failed: " + error);
                            }
                        }
                        
                        continue; // Successfully handled ST_* AND <predicates>
                    } else {
                        // Cannot compute bbox - log warning and fall through to normal processing
                        THEMIS_WARN("Spatial predicate {} in AND requires bbox but could not compute from expression", 
                                    funcName);
                    }
                }
            }
        }
        
        if (!extractPredicates(filter->condition, query.predicates, query.rangePredicates, error)) {
            return TranslationResult::Error("Filter translation failed: " + error);
        }
    }
    
    // Process SORT + LIMIT
    if (ast->sort) {
        query.orderBy = extractOrderBy(ast->sort, ast->limit);
    }
    
    return finalizeResult(TranslationResult::Success(std::move(query)));
}

bool AQLTranslator::extractPredicates(
    const std::shared_ptr<Expression>& expr,
    std::vector<PredicateEq>& eqPredicates,
    std::vector<PredicateRange>& rangePredicates,
    std::string& error
) {
    if (!expr) {
        error = "Null expression";
        return false;
    }
    
    // Check for FULLTEXT function call in FILTER
    if (expr->getType() == ASTNodeType::FunctionCall) {
        auto funcCall = std::static_pointer_cast<FunctionCallExpr>(expr);
        
        // Check if it's FULLTEXT(...) - case insensitive
        std::string funcName = funcCall->name;
        std::transform(funcName.begin(), funcName.end(), funcName.begin(), ::tolower);
        
        if (funcName == "fulltext") {
            // FULLTEXT is now allowed in both AND and OR combinations
            // In OR: each disjunct can have its own FULLTEXT
            // In AND: handled at translate level
            // When called from extractPredicates in OR context, we shouldn't reach here
            // (DNF conversion handles FULLTEXT directly)
            error = "FULLTEXT() should be handled by DNF converter in OR context";
            return false;
        }
    }
    
    // Check expression type
    if (expr->getType() == ASTNodeType::BinaryOp) {
        auto binOp = std::static_pointer_cast<BinaryOpExpr>(expr);
        
        // Handle AND: Recursively extract from both sides
        if (binOp->op == BinaryOperator::And) {
            return extractPredicates(binOp->left, eqPredicates, rangePredicates, error) &&
                   extractPredicates(binOp->right, eqPredicates, rangePredicates, error);
        }
        
        // Handle OR: Should be handled at higher level via convertToDNF
        // If we reach here, it means OR is nested in a way that requires DNF conversion
        if (binOp->op == BinaryOperator::Or) {
            error = "OR operator detected - should be handled via DisjunctiveQuery (internal error)";
            return false;
        }
        
        if (binOp->op == BinaryOperator::Xor) {
            error = "XOR operator not supported";
            return false;
        }
        
        // Extract column name from left side (must be field access)
        if (binOp->left->getType() != ASTNodeType::FieldAccess) {
            error = "Left side of comparison must be field access (e.g., doc.age)";
            return false;
        }
        
        std::string column = extractColumnName(binOp->left);
        
        // Extract value from right side (must be literal)
        if (binOp->right->getType() != ASTNodeType::Literal) {
            error = "Right side of comparison must be literal value";
            return false;
        }
        
        auto literal = std::static_pointer_cast<LiteralExpr>(binOp->right);
        std::string value = literalToString(literal->value);
        
        // Map operator to predicate type
        switch (binOp->op) {
            case BinaryOperator::Eq:
                eqPredicates.push_back(PredicateEq{column, value});
                break;
                
            case BinaryOperator::Neq:
                // NEQ should be handled in convertToDNF, not here
                // If we reach here, it's a programming error
                error = "Internal error: NEQ should be converted to (< OR >) in convertToDNF";
                return false;
                
            case BinaryOperator::Lt:
                rangePredicates.push_back(PredicateRange{
                    column,
                    std::nullopt,   // no lower bound
                    value,          // upper bound
                    true,           // include lower (doesn't matter)
                    false           // exclude upper (< not <=)
                });
                break;
                
            case BinaryOperator::Lte:
                rangePredicates.push_back(PredicateRange{
                    column,
                    std::nullopt,   // no lower bound
                    value,          // upper bound
                    true,           // include lower (doesn't matter)
                    true            // include upper (<=)
                });
                break;
                
            case BinaryOperator::Gt:
                rangePredicates.push_back(PredicateRange{
                    column,
                    value,          // lower bound
                    std::nullopt,   // no upper bound
                    false,          // exclude lower (> not >=)
                    true            // include upper (doesn't matter)
                });
                break;
                
            case BinaryOperator::Gte:
                rangePredicates.push_back(PredicateRange{
                    column,
                    value,          // lower bound
                    std::nullopt,   // no upper bound
                    true,           // include lower (>=)
                    true            // include upper (doesn't matter)
                });
                break;
                
            default:
                error = "Unsupported operator in filter";
                return false;
        }
        
        return true;
    }
    
    error = "Unsupported expression type in filter (only binary operators supported)";
    return false;
}

std::string AQLTranslator::extractColumnName(const std::shared_ptr<Expression>& expr) {
    if (expr->getType() == ASTNodeType::FieldAccess) {
        auto fieldAccess = std::static_pointer_cast<FieldAccessExpr>(expr);
        
        // Handle nested field access: doc.address.city -> "address.city"
        std::string result;
        
        // Recursively extract parent field names
        if (fieldAccess->object->getType() == ASTNodeType::FieldAccess) {
            result = extractColumnName(fieldAccess->object) + ".";
        }
        // Skip the variable name (e.g., "doc") at the root
        
        result += fieldAccess->field;
        return result;
    }
    
    return "";
}

std::string AQLTranslator::literalToString(const LiteralValue& value) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        }
        
        return "";
    }, value);
}

std::optional<OrderBy> AQLTranslator::extractOrderBy(
    const std::shared_ptr<SortNode>& sort,
    const std::shared_ptr<LimitNode>& limit
) {
    if (!sort || sort->specifications.empty()) {
        return std::nullopt;
    }
    
    // Only support single-column sorting for now
    const auto& spec = sort->specifications[0];
    
    OrderBy orderBy;
    orderBy.column = extractColumnName(spec.expression);
    orderBy.desc = !spec.ascending;
    
    // Apply limit if present
    if (limit) {
        // For offset support, request offset+count from index scan and slice later in handler
        auto off = static_cast<size_t>(std::max<int64_t>(0, limit->offset));
        auto cnt = static_cast<size_t>(std::max<int64_t>(0, limit->count));
        orderBy.limit = off + cnt;
    } else {
        orderBy.limit = 1000; // default limit
    }
    
    return orderBy;
}

bool AQLTranslator::containsOr(const std::shared_ptr<Expression>& expr) {
    if (!expr) {
      return false;
    }
    
    if (expr->getType() == ASTNodeType::BinaryOp) {
        auto binOp = std::static_pointer_cast<BinaryOpExpr>(expr);
        if (binOp->op == BinaryOperator::Or) {
            return true;
        }
        // Recursively check both sides
        return containsOr(binOp->left) || containsOr(binOp->right);
    }
    
    return false;
}

std::vector<ConjunctiveQuery> AQLTranslator::convertToDNF(
    const std::shared_ptr<Expression>& expr,
    const std::string& table,
    std::string& error
) {
    if (!expr) {
        error = "Null expression in DNF conversion";
        return {};
    }
    
    // Handle NOT operator using De Morgan's Laws
    // NOT (A AND B) = (NOT A) OR (NOT B)
    // NOT (A OR B) = (NOT A) AND (NOT B)
    if (expr->getType() == ASTNodeType::UnaryOp) {
        auto unaryOp = std::static_pointer_cast<UnaryOpExpr>(expr);
        
        if (unaryOp->op == UnaryOperator::Not) {
            auto operand = unaryOp->operand;
            
            // NOT of binary operator - apply De Morgan's Laws
            if (operand->getType() == ASTNodeType::BinaryOp) {
                auto binOp = std::static_pointer_cast<BinaryOpExpr>(operand);
                
                // NOT (A OR B) = (NOT A) AND (NOT B)
                if (binOp->op == BinaryOperator::Or) {
                    auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
                    auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
                    auto andExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::And, notLeft, notRight);
                    
                    return convertToDNF(andExpr, table, error);
                }
                
                // NOT (A AND B) = (NOT A) OR (NOT B)
                if (binOp->op == BinaryOperator::And) {
                    auto notLeft = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->left);
                    auto notRight = std::make_shared<UnaryOpExpr>(UnaryOperator::Not, binOp->right);
                    auto orExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Or, notLeft, notRight);
                    
                    return convertToDNF(orExpr, table, error);
                }
                
                // NOT (A == B) becomes (A != B)
                if (binOp->op == BinaryOperator::Eq) {
                    // NEQ is converted to: (A < B) OR (A > B)
                    // This allows index usage via DisjunctiveQuery
                    auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
                    auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
                    auto orExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Or, ltExpr, gtExpr);
                    
                    return convertToDNF(orExpr, table, error);
                }
                
                // NOT (A != B) becomes (A == B)
                if (binOp->op == BinaryOperator::Neq) {
                    auto eqExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, binOp->left, binOp->right);
                    return convertToDNF(eqExpr, table, error);
                }
                
                // NOT (A < B) becomes (A >= B)
                if (binOp->op == BinaryOperator::Lt) {
                    auto gteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gte, binOp->left, binOp->right);
                    return convertToDNF(gteExpr, table, error);
                }
                
                // NOT (A > B) becomes (A <= B)
                if (binOp->op == BinaryOperator::Gt) {
                    auto lteExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lte, binOp->left, binOp->right);
                    return convertToDNF(lteExpr, table, error);
                }
                
                // NOT (A <= B) becomes (A > B)
                if (binOp->op == BinaryOperator::Lte) {
                    auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
                    return convertToDNF(gtExpr, table, error);
                }
                
                // NOT (A >= B) becomes (A < B)
                if (binOp->op == BinaryOperator::Gte) {
                    auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
                    return convertToDNF(ltExpr, table, error);
                }
            }
            
            // NOT of NOT - double negation elimination
            if (operand->getType() == ASTNodeType::UnaryOp) {
                auto innerUnary = std::static_pointer_cast<UnaryOpExpr>(operand);
                if (innerUnary->op == UnaryOperator::Not) {
                    return convertToDNF(innerUnary->operand, table, error);
                }
            }
            
            // NOT of literal/variable - can't convert to index predicate
            // This would require full scan with negation filter
            error = "NOT of non-comparison expression not yet supported for index queries";
            return {};
        }
    }
    
    // Base case: Single predicate (leaf node)
    if (expr->getType() == ASTNodeType::BinaryOp) {
        auto binOp = std::static_pointer_cast<BinaryOpExpr>(expr);
        
        // OR: Split into multiple disjuncts
        if (binOp->op == BinaryOperator::Or) {
            auto leftDNF = convertToDNF(binOp->left, table, error);
            if (!error.empty()) return {};
            
            auto rightDNF = convertToDNF(binOp->right, table, error);
            if (!error.empty()) return {};
            
            // Merge disjuncts (union)
            leftDNF.insert(leftDNF.end(), rightDNF.begin(), rightDNF.end());
            return leftDNF;
        }
        
        // AND: Distribute over existing disjuncts
        if (binOp->op == BinaryOperator::And) {
            auto leftDNF = convertToDNF(binOp->left, table, error);
            if (!error.empty()) return {};
            
            auto rightDNF = convertToDNF(binOp->right, table, error);
            if (!error.empty()) return {};
            
            // Cartesian product: (A OR B) AND (C OR D) = (A AND C) OR (A AND D) OR (B AND C) OR (B AND D)
            // Q3: Pre-allocate to the exact cartesian-product size to avoid incremental reallocations.
            std::vector<ConjunctiveQuery> result = {};

            result.reserve(leftDNF.size() * rightDNF.size());
            for (const auto& leftConj : leftDNF) {
                for (const auto& rightConj : rightDNF) {
                    ConjunctiveQuery merged;
                    merged.table = table;
                    
                    // Merge predicates (Q3: pre-allocate combined size)
                    merged.predicates.reserve(leftConj.predicates.size() + rightConj.predicates.size());
                    merged.predicates = leftConj.predicates;
                    merged.predicates.insert(merged.predicates.end(), 
                                            rightConj.predicates.begin(), 
                                            rightConj.predicates.end());
                    
                    // Merge range predicates (Q3: pre-allocate combined size)
                    merged.rangePredicates.reserve(leftConj.rangePredicates.size() + rightConj.rangePredicates.size());
                    merged.rangePredicates = leftConj.rangePredicates;
                    merged.rangePredicates.insert(merged.rangePredicates.end(),
                                                 rightConj.rangePredicates.begin(),
                                                 rightConj.rangePredicates.end());
                    
                    // Merge fulltext predicates
                    // Only one FULLTEXT per disjunct allowed (can't merge multiple FULLTEXT into single AND clause)
                    if (leftConj.fulltextPredicate.has_value() && rightConj.fulltextPredicate.has_value()) {
                        error = "Cannot combine multiple FULLTEXT() predicates in AND - only one FULLTEXT per clause allowed";
                        return {};
                    }
                    if (leftConj.fulltextPredicate.has_value()) {
                        merged.fulltextPredicate = leftConj.fulltextPredicate;
                    } else if (rightConj.fulltextPredicate.has_value()) {
                        merged.fulltextPredicate = rightConj.fulltextPredicate;
                    }
                    
                    result.push_back(std::move(merged));
                }
            }
            return result;
        }
        
        // Leaf comparison (==, <, >, !=, etc.)
        // Special handling for NEQ: convert to (< value) OR (> value)
        if (binOp->op == BinaryOperator::Neq) {
            // A != B is converted to: (A < B) OR (A > B)
            auto ltExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Lt, binOp->left, binOp->right);
            auto gtExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Gt, binOp->left, binOp->right);
            auto orExpr = std::make_shared<BinaryOpExpr>(BinaryOperator::Or, ltExpr, gtExpr);
            
            return convertToDNF(orExpr, table, error);
        }
        
        // Create single-predicate conjunctive query
        ConjunctiveQuery conj;
        conj.table = table;
        
        std::vector<PredicateEq> eqPreds;
        std::vector<PredicateRange> rangePreds;
        
        if (!extractPredicates(expr, eqPreds, rangePreds, error)) {
            return {};
        }
        
        conj.predicates = std::move(eqPreds);
        conj.rangePredicates = std::move(rangePreds);
        
        return {conj};
    }
    
    // FULLTEXT function call - create single-predicate query with FULLTEXT
    if (expr->getType() == ASTNodeType::FunctionCall) {
        auto funcCall = std::static_pointer_cast<FunctionCallExpr>(expr);
        std::string funcName = funcCall->name;
        std::transform(funcName.begin(), funcName.end(), funcName.begin(), ::tolower);
        
        if (funcName == "fulltext") {
            // Parse FULLTEXT(column, query [, limit])
            if (funcCall->arguments.size() < 2 || funcCall->arguments.size() > 3) {
                error = "FULLTEXT() requires 2-3 arguments: FULLTEXT(column, query [, limit])";
                return {};
            }
            
            if (funcCall->arguments[0]->getType() != ASTNodeType::FieldAccess) {
                error = "FULLTEXT() first argument must be field access (e.g., doc.content)";
                return {};
            }
            std::string column = extractColumnName(funcCall->arguments[0]);
            
            if (funcCall->arguments[1]->getType() != ASTNodeType::Literal) {
                error = "FULLTEXT() second argument must be string literal";
                return {};
            }
            auto queryLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[1]);
            if (!std::holds_alternative<std::string>(queryLiteral->value)) {
                error = "FULLTEXT() query must be a string";
                return {};
            }
            std::string queryStr = std::get<std::string>(queryLiteral->value);
            
            size_t limit = 1000; // default
            if (funcCall->arguments.size() == 3) {
                if (funcCall->arguments[2]->getType() != ASTNodeType::Literal) {
                    error = "FULLTEXT() third argument (limit) must be integer literal";
                    return {};
                }
                auto limitLiteral = std::static_pointer_cast<LiteralExpr>(funcCall->arguments[2]);
                if (std::holds_alternative<int64_t>(limitLiteral->value)) {
                    int64_t limit_value = std::get<int64_t>(limitLiteral->value);
                    if (limit_value < 0) {
                        error = "FULLTEXT() limit must be non-negative";
                        return {};
                    }
                    limit = static_cast<size_t>(limit_value);
                } else {
                    error = "FULLTEXT() limit must be an integer";
                    return {};
                }
            }
            
            // Create a ConjunctiveQuery with only the fulltext predicate
            ConjunctiveQuery conj;
            conj.table = table;
            conj.fulltextPredicate = PredicateFulltext{column, queryStr, limit};
            return {conj};
        }
    }
    
    error = "Unsupported expression type in DNF conversion";
    return {};
}

size_t AQLTranslator::countCTEReferences(
    const std::shared_ptr<Query>& ast,
    const std::string& cte_name
) {
    if (!ast) {
        return 0;
    }

    size_t count = 0;
    for (const auto& for_node : ast->for_nodes) {
        if (for_node.collection == cte_name) {
            ++count;
        }
    }
    if (ast->for_node.collection == cte_name) {
        ++count;
    }

    for (const auto& let_node : ast->let_nodes) {
        if (let_node.expression && let_node.expression->getType() == ASTNodeType::SubqueryExpr) {
            auto subq = std::static_pointer_cast<SubqueryExpr>(let_node.expression);
            count += countCTEReferences(subq->subquery, cte_name);
        }
    }

    for (const auto& filter : ast->filters) {
        if (!filter || !filter->condition) {
            continue;
        }
        count += countCTEReferencesInExpr(filter->condition, cte_name);
    }

    return count;
}

size_t AQLTranslator::countCTEReferencesInExpr(
    const std::shared_ptr<Expression>& expr,
    const std::string& cte_name
) {
    if (!expr) {
        return 0;
    }

    size_t count = 0;
    switch (expr->getType()) {
        case ASTNodeType::SubqueryExpr: {
            auto subq = std::static_pointer_cast<SubqueryExpr>(expr);
            count += countCTEReferences(subq->subquery, cte_name);
            break;
        }
        case ASTNodeType::AnyExpr: {
            auto any = std::static_pointer_cast<AnyExpr>(expr);
            count += countCTEReferencesInExpr(any->arrayExpr, cte_name);
            count += countCTEReferencesInExpr(any->condition, cte_name);
            break;
        }
        case ASTNodeType::AllExpr: {
            auto all = std::static_pointer_cast<AllExpr>(expr);
            count += countCTEReferencesInExpr(all->arrayExpr, cte_name);
            count += countCTEReferencesInExpr(all->condition, cte_name);
            break;
        }
        case ASTNodeType::BinaryOp: {
            auto binop = std::static_pointer_cast<BinaryOpExpr>(expr);
            count += countCTEReferencesInExpr(binop->left, cte_name);
            count += countCTEReferencesInExpr(binop->right, cte_name);
            break;
        }
        case ASTNodeType::FunctionCall: {
            auto func = std::static_pointer_cast<FunctionCallExpr>(expr);
            for (const auto& arg : func->arguments) {
                count += countCTEReferencesInExpr(arg, cte_name);
            }
            break;
        }
        default:
            break;
    }
    return count;
}

void AQLTranslator::attachCTEs(
    TranslationResult& result,
    std::vector<TranslationResult::CTEExecution> ctes
) {
    if (!ctes.empty() && result.success) {
        result.ctes = std::move(ctes);
    }
}

} // namespace themis


