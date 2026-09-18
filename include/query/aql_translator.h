/**
 * @file aql_translator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "aql_parser.h"
#include "query/mutation_execution_plan.h"
#include "query_engine.h"
#include <memory>
#include <string>

namespace themis {

// Bring query types into scope
using Query = ::themis::query::Query;
using Expression = ::themis::query::Expression;
using SortNode = ::themis::query::SortNode;
using LimitNode = ::themis::query::LimitNode;
using FilterNode = ::themis::query::FilterNode;
using BinaryOpExpr = ::themis::query::BinaryOpExpr;
using FieldAccessExpr = ::themis::query::FieldAccessExpr;
using LiteralExpr = ::themis::query::LiteralExpr;
using LiteralValue = ::themis::query::LiteralValue;
using ASTNodeType = ::themis::query::ASTNodeType;
using FunctionCallExpr = ::themis::query::FunctionCallExpr;
using BinaryOperator = ::themis::query::BinaryOperator;
using UnaryOperator = ::themis::query::UnaryOperator;
using UnaryOpExpr = ::themis::query::UnaryOpExpr;
using VariableExpr = ::themis::query::VariableExpr;
using SimilarityCallExpr = ::themis::query::SimilarityCallExpr;
using ProximityCallExpr = ::themis::query::ProximityCallExpr;
using ArrayLiteralExpr = ::themis::query::ArrayLiteralExpr;
using SubqueryExpr = ::themis::query::SubqueryExpr;
using AnyExpr = ::themis::query::AnyExpr;
using AllExpr = ::themis::query::AllExpr;
using ConjunctiveQuery = ::themis::query::ConjunctiveQuery;
using DisjunctiveQuery = ::themis::query::DisjunctiveQuery;
using VectorGeoQuery = ::themis::query::VectorGeoQuery;
using ContentGeoQuery = ::themis::query::ContentGeoQuery;
using ForNode = ::themis::query::ForNode;
using LetNode = ::themis::query::LetNode;
using ReturnNode = ::themis::query::ReturnNode;
using CollectNode = ::themis::query::CollectNode;
using PredicateEq = ::themis::query::PredicateEq;
using PredicateRange = ::themis::query::PredicateRange;
using PredicateFulltext = ::themis::query::PredicateFulltext;
using PredicatePhrase = ::themis::query::PredicatePhrase;
using PredicateFuzzy = ::themis::query::PredicateFuzzy;
using PredicateSpatial = ::themis::query::PredicateSpatial;
using OrderBy = ::themis::query::OrderBy;

class AQLTranslator {
public:
    struct TranslationResult {
        bool success = false;
        std::string error_message;
        ConjunctiveQuery conjunctive_query; // fuer relationale AQL (single-FOR)
        
        // Graph-Traversal Query (optional)
        struct TraversalQuery {
            enum class Direction { Outbound, Inbound, Any };
            std::string variable;
            int minDepth = 1;
            int maxDepth = 1;
            Direction direction = Direction::Outbound;
            std::string startVertex;
            std::string graphName;
            bool shortestPath = false;
            std::string endVertex; // gesetzt wenn shortestPath
        };
        std::optional<TraversalQuery> traversal;
        
        // Join Query (multi-FOR)
        struct JoinQuery {
            std::vector<query::ForNode> for_nodes;                                    // Multiple FOR clauses
            std::vector<std::shared_ptr<query::FilterNode>> filters;                  // JOIN conditions + filters
            std::vector<query::LetNode> let_nodes;                                    // LET bindings
            std::shared_ptr<query::ReturnNode> return_node;                           // RETURN expression
            std::shared_ptr<query::SortNode> sort;                                    // SORT clause
            std::shared_ptr<query::LimitNode> limit;                                  // LIMIT clause
            std::shared_ptr<query::CollectNode> collect;                       // COLLECT/GROUP BY
        };
        std::optional<JoinQuery> join;
        
        // Spatial JOIN query: FOR a IN colA FOR b IN colB FILTER GEO_DISTANCE(a.f, b.f) <= threshold
        struct SpatialJoinQuery {
            std::string outer_collection; ///< Collection name for the outer (left) loop.
            std::string inner_collection; ///< Collection name for the inner (right) loop.
            std::string outer_var;        ///< Variable name bound by the outer FOR clause.
            std::string inner_var;        ///< Variable name bound by the inner FOR clause.
            std::string outer_field;      ///< Geometry field on the outer variable (e.g. "loc").
            std::string inner_field;      ///< Geometry field on the inner variable (e.g. "loc").
            double threshold_m = 0.0;     ///< Distance threshold in metres.
            std::size_t max_pairs = 1'000'000; ///< Maximum result pairs (default 1 M).
        };
        std::optional<SpatialJoinQuery> spatial_join;

        // Disjunctive Query (OR support)
        std::optional<DisjunctiveQuery> disjunctive;

        // Hybrid Vector+Geo Query (SIMILARITY + ST_*)
        std::optional<VectorGeoQuery> vector_geo;
        // Hybrid Content+Geo Query (FULLTEXT + PROXIMITY + optional ST_*)
        std::optional<ContentGeoQuery> content_geo;
        
        // Phase 4: CTE execution metadata
        struct CTEExecution {
            std::string name;                          // CTE name
            std::shared_ptr<query::Query> subquery;    // AST for execution
            bool should_materialize = false;           // Based on heuristic
        };
        std::vector<CTEExecution> ctes;                // CTEs to execute before main query
        
        /**
         * @brief Success.
         * @param[in] q Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult Success(ConjunctiveQuery q) {
            TranslationResult r;
            r.success = true;
            r.conjunctive_query = std::move(q);
            return r;
        }
        
        /**
         * @brief Success Disjunctive.
         * @param[in] d Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult SuccessDisjunctive(DisjunctiveQuery d) {
            TranslationResult r;
            r.success = true;
            r.disjunctive = std::move(d);
            return r;
        }
        
        /**
         * @brief Success Join.
         * @param[in] j Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult SuccessJoin(JoinQuery j) {
            TranslationResult r;
            r.success = true;
            r.join = std::move(j);
            return r;
        }

        /**
         * @brief Success Spatial Join.
         * @param[in] sj Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult SuccessSpatialJoin(SpatialJoinQuery sj) {
            TranslationResult r;
            r.success = true;
            r.spatial_join = std::move(sj);
            return r;
        }
        
        /**
         * @brief Success Traversal.
         * @param[in] t Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult SuccessTraversal(TraversalQuery t) {
            TranslationResult r;
            r.success = true;
            r.traversal = std::move(t);
            return r;
        }

        /**
         * @brief Success Vector Geo.
         * @param[in] v Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult SuccessVectorGeo(VectorGeoQuery v) {
            TranslationResult r;
            r.success = true;
            r.vector_geo = std::move(v);
            return r;
        }
        /**
         * @brief Success Content Geo.
         * @param[in] c Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult SuccessContentGeo(ContentGeoQuery c) {
            TranslationResult r; r.success = true; r.content_geo = std::move(c); return r; }
        
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static TranslationResult Error(std::string msg) {
            TranslationResult r;
            r.success = false;
            r.error_message = std::move(msg);
            return r;
        }
    };
    
    /**
     * @brief Translate.
     * @param[in] ast Input parameter.
     * @return Return value.
     */
    static TranslationResult translate(const std::shared_ptr<Query>& ast);

private:
    /**
     * @brief Extract Predicates.
     * @param[in] expr Input parameter.
     * @param[in,out] eqPredicates Input/output parameter.
     * @param[in,out] rangePredicates Input/output parameter.
     * @param[in,out] error Input/output parameter.
     * @return True when the operation succeeds.
     */
    static bool extractPredicates(
        const std::shared_ptr<Expression>& expr,
        std::vector<PredicateEq>& eqPredicates,
        std::vector<PredicateRange>& rangePredicates,
        std::string& error
    );
    
    /**
     * @brief Contains Or.
     * @param[in] expr Input parameter.
     * @return True when the operation succeeds.
     */
    static bool containsOr(const std::shared_ptr<Expression>& expr);
    
    /**
     * @brief Convert To DNF.
     * @param[in] expr Input parameter.
     * @param[in] table Input parameter.
     * @param[in,out] error Input/output parameter.
     * @return Return value.
     */
    static std::vector<ConjunctiveQuery> convertToDNF(
        const std::shared_ptr<Expression>& expr,
        const std::string& table,
        std::string& error
    );
    
    /**
     * @brief Extract Column Name.
     * @param[in] expr Input parameter.
     * @return Return value.
     */
    static std::string extractColumnName(const std::shared_ptr<Expression>& expr);
    
    /**
     * @brief Literal To String.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static std::string literalToString(const LiteralValue& value);
    
    /**
     * @brief Extract Order By.
     * @param[in] sort Input parameter.
     * @param[in] limit Input parameter.
     * @return Return value.
     */
    static std::optional<OrderBy> extractOrderBy(
        const std::shared_ptr<SortNode>& sort,
        const std::shared_ptr<LimitNode>& limit
    );
    
    /**
     * @brief Count CTEReferences.
     * @param[in] ast Input parameter.
     * @param[in] cte_name Name of the cte.
     * @return Return value.
     */
    static size_t countCTEReferences(
        const std::shared_ptr<Query>& ast,
        const std::string& cte_name
    );
    
    /**
     * @brief Count CTEReferences In Expr.
     * @param[in] expr Input parameter.
     * @param[in] cte_name Name of the cte.
     * @return Return value.
     */
    static size_t countCTEReferencesInExpr(
        const std::shared_ptr<Expression>& expr,
        const std::string& cte_name
    );
    
    /**
     * @brief Attach CTEs.
     * @param[in,out] result Input/output parameter.
     * @param[in] ctes Input parameter.
     */
    static void attachCTEs(
        TranslationResult& result,
        std::vector<TranslationResult::CTEExecution> ctes
    );
};

} // namespace themis

// ============================================================================
// AqlMutationTranslator — EPIC-004 Phase 3
// ============================================================================

namespace themis {

class AqlMutationTranslator {
public:
    AqlMutationTranslator()  = default;
    ~AqlMutationTranslator() = default;

    [[nodiscard]] query::MutationExecutionPlan translate(
        const std::shared_ptr<query::MutationNode>& node) const;

private:
    [[nodiscard]] query::MutationExecutionPlan translateInsert(
        const query::InsertNode& n) const;

    [[nodiscard]] query::MutationExecutionPlan translateUpdate(
        const query::UpdateNode& n) const;

    [[nodiscard]] query::MutationExecutionPlan translateRemove(
        const query::RemoveNode& n) const;

    [[nodiscard]] query::MutationExecutionPlan translateReplace(
        const query::ReplaceNode& n) const;

    [[nodiscard]] query::MutationExecutionPlan translateUpsert(
        const query::UpsertNode& n) const;
};

} // namespace themis

