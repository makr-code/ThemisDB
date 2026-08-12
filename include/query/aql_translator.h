/**
 * @file aql_translator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * Translates AQL AST to QueryEngine ConjunctiveQuery
 * 
 * Example:
 *   FOR user IN users 
 *   FILTER user.age > 18 AND user.city == "Berlin"
 *   SORT user.created_at DESC
 *   LIMIT 10
 *   RETURN user
 * 
 * Translates to:
 *   ConjunctiveQuery {
 *     table: "users",
 *     predicates: [{ column: "city", value: "Berlin" }],
 *     rangePredicates: [{ column: "age", lower: "18", includeLower: false }],
 *     orderBy: { column: "created_at", desc: true, limit: 10 }
 *   }
 */
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
        
        static TranslationResult Success(ConjunctiveQuery q) {
            TranslationResult r;
            r.success = true;
            r.conjunctive_query = std::move(q);
            return r;
        }
        
        static TranslationResult SuccessDisjunctive(DisjunctiveQuery d) {
            TranslationResult r;
            r.success = true;
            r.disjunctive = std::move(d);
            return r;
        }
        
        static TranslationResult SuccessJoin(JoinQuery j) {
            TranslationResult r;
            r.success = true;
            r.join = std::move(j);
            return r;
        }

        static TranslationResult SuccessSpatialJoin(SpatialJoinQuery sj) {
            TranslationResult r;
            r.success = true;
            r.spatial_join = std::move(sj);
            return r;
        }
        
        static TranslationResult SuccessTraversal(TraversalQuery t) {
            TranslationResult r;
            r.success = true;
            r.traversal = std::move(t);
            return r;
        }

        static TranslationResult SuccessVectorGeo(VectorGeoQuery v) {
            TranslationResult r;
            r.success = true;
            r.vector_geo = std::move(v);
            return r;
        }
        static TranslationResult SuccessContentGeo(ContentGeoQuery c) {
            TranslationResult r; r.success = true; r.content_geo = std::move(c); return r; }
        
        static TranslationResult Error(std::string msg) {
            TranslationResult r;
            r.success = false;
            r.error_message = std::move(msg);
            return r;
        }
    };
    
    /**
     * Translate AQL AST to QueryEngine query
     * 
     * Supported:
     * - Conjunctive queries (AND combinations)
     * - Disjunctive queries (OR combinations in DNF)
     * - Mixed AND/OR expressions
     * 
     * Limitations:
     * - Functions in FILTER limited (FULLTEXT supported)
     */
    static TranslationResult translate(const std::shared_ptr<Query>& ast);

private:
    /**
     * Extract predicates from FILTER conditions
     * Supports AND/OR and converts to Disjunctive Normal Form (DNF)
     * Returns false if unsupported expression found
     */
    static bool extractPredicates(
        const std::shared_ptr<Expression>& expr,
        std::vector<PredicateEq>& eqPredicates,
        std::vector<PredicateRange>& rangePredicates,
        std::string& error
    );
    
    /**
     * Check if expression contains OR operator (requires DisjunctiveQuery)
     */
    static bool containsOr(const std::shared_ptr<Expression>& expr);
    
    /**
     * Convert expression to Disjunctive Normal Form (DNF)
     * Returns list of conjunctive clauses (disjuncts)
     * Example: (A AND B) OR (C AND D) -> [[A,B], [C,D]]
     */
    static std::vector<ConjunctiveQuery> convertToDNF(
        const std::shared_ptr<Expression>& expr,
        const std::string& table,
        std::string& error
    );
    
    /**
     * Extract column name from field access expression
     * E.g., "user.age" -> "age"
     */
    static std::string extractColumnName(const std::shared_ptr<Expression>& expr);
    
    /**
     * Convert literal value to string for query engine
     */
    static std::string literalToString(const LiteralValue& value);
    
    /**
     * Extract ORDER BY from SORT clause
     */
    static std::optional<OrderBy> extractOrderBy(
        const std::shared_ptr<SortNode>& sort,
        const std::shared_ptr<LimitNode>& limit
    );
    
    /**
     * Count CTE references in AST (Phase 4.1)
     * Scans FOR nodes to see how many times a CTE name appears as collection
     */
    static size_t countCTEReferences(
        const std::shared_ptr<Query>& ast,
        const std::string& cte_name
    );
    
    /**
     * Count CTE references recursively in expressions (Phase 4.1)
     * Used for subqueries in FILTER, LET, etc.
     */
    static size_t countCTEReferencesInExpr(
        const std::shared_ptr<Expression>& expr,
        const std::string& cte_name
    );
    
    /**
     * Attach CTE execution metadata to translation result (Phase 4.1)
     * Helper to avoid duplicating CTE attachment logic across all return paths
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

/**
 * @brief Translates a parsed MutationNode AST into a MutationExecutionPlan.
 *
 * This class is the Phase 3 translation layer.  It accepts a validated
 * @c MutationNode produced by @c AQLParser::parseMutation() and converts it
 * into an ordered sequence of @c MutationStep objects (a
 * @c MutationExecutionPlan) ready for @c MutationExecutor::execute().
 *
 * The translator does **not** execute the plan — that is exclusively the
 * responsibility of @c MutationExecutor.
 *
 * ### Thread safety
 * Stateless — all methods are @c const.  Safe for concurrent use without
 * external synchronisation.
 */
class AqlMutationTranslator {
public:
    AqlMutationTranslator()  = default;
    ~AqlMutationTranslator() = default;

    /**
     * @brief Translate a MutationNode to a MutationExecutionPlan.
     *
     * If @p node is @c nullptr an error plan is returned with an empty
     * collection name and a single @c ValidatePredicate step carrying an
     * error description.
     *
     * @param node  Validated MutationNode shared pointer (may be nullptr).
     * @return MutationExecutionPlan ready for MutationExecutor.
     */
    [[nodiscard]] query::MutationExecutionPlan translate(
        const std::shared_ptr<query::MutationNode>& node) const;

private:
    /// @brief Build execution plan for an INSERT node.
    [[nodiscard]] query::MutationExecutionPlan translateInsert(
        const query::InsertNode& n) const;

    /// @brief Build execution plan for an UPDATE node.
    [[nodiscard]] query::MutationExecutionPlan translateUpdate(
        const query::UpdateNode& n) const;

    /// @brief Build execution plan for a REMOVE node.
    [[nodiscard]] query::MutationExecutionPlan translateRemove(
        const query::RemoveNode& n) const;

    /// @brief Build execution plan for a REPLACE node.
    [[nodiscard]] query::MutationExecutionPlan translateReplace(
        const query::ReplaceNode& n) const;

    /// @brief Build execution plan for an UPSERT node.
    [[nodiscard]] query::MutationExecutionPlan translateUpsert(
        const query::UpsertNode& n) const;
};

} // namespace themis

