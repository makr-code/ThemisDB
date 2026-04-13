/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_model_router.h                                 ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-13                                         ║
  Author:          copilot-swe-agent                                  ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace aql {

// ============================================================================
// IModelRouter
// ============================================================================

/**
 * @brief Model type categories that can be matched against an AQL query.
 *
 * A single query may match multiple categories; the router picks the highest-
 * priority match using the registered route table.
 */
enum class QueryModelType {
    VECTOR,      ///< K-NN / ANN vector search (KNN, ANN, VECTOR_SEARCH keywords)
    GRAPH,       ///< Graph traversal (TRAVERSE, SHORTEST_PATH, BFS, DFS)
    GEO,         ///< Geospatial predicates (ST_DISTANCE, ST_WITHIN, ST_INTERSECTS, WITHIN)
    FULLTEXT,    ///< Full-text search (BM25, LIKE, CONTAINS, PHRASE, FULLTEXT)
    TIMESERIES,  ///< Time-series aggregates (TIME_TRUNC, HISTOGRAM, INTERVAL, DOWNSAMPLE)
    RELATIONAL,  ///< Plain relational / document query (FOR…FILTER…RETURN)
    PROCESS,     ///< Process mining / BPMN queries (PROCESS_INSTANCES, TOKEN_SCAN)
    UNKNOWN,     ///< Classification failed or query is empty
};

/**
 * @brief A single registered route entry.
 *
 * Routes are evaluated in descending priority order (higher value = first).
 */
struct ModelRoute {
    QueryModelType model_type;  ///< Model category this route targets
    std::string    model_alias; ///< Alias string passed to the LLM backend
    int            priority;    ///< Routing priority (higher wins; default = 100)
    bool           enabled;     ///< When false, route is skipped during selection

    ModelRoute() : model_type(QueryModelType::RELATIONAL),
                   priority(100), enabled(true) {}
    ModelRoute(QueryModelType t, std::string alias, int prio = 100)
        : model_type(t), model_alias(std::move(alias)),
          priority(prio), enabled(true) {}
};

/**
 * @brief Result of a routing decision.
 */
struct RoutingDecision {
    /// Detected primary model type for the query.
    QueryModelType primary_type = QueryModelType::UNKNOWN;

    /// All detected model types (ordered by confidence descending).
    std::vector<QueryModelType> detected_types;

    /// Winning route (empty model_alias iff no route was registered for
    /// @c primary_type and no fallback was found).
    std::optional<ModelRoute> selected_route;

    /// Fallback route used when the primary route was unavailable.
    std::optional<ModelRoute> fallback_route;

    /// Human-readable explanation of the routing decision.
    std::string explanation;
};

/**
 * @brief Interface for AQL multi-model routing with priority and fallback.
 *
 * The router classifies an AQL query into one or more QueryModelType categories
 * by scanning for keyword patterns, then selects the best-matching ModelRoute
 * from its registry.  When the primary route is unavailable (not registered or
 * disabled), the router falls back to the next-highest-priority registered route
 * that is enabled.
 *
 * Thread safety: implementations MUST be safe to call concurrently from multiple
 * threads once routes are registered.
 *
 * Typical usage:
 * @code
 *   AQLModelRouter router;
 *   router.registerRoute({QueryModelType::VECTOR, "openai/text-embedding-3-small", 200});
 *   router.registerRoute({QueryModelType::GRAPH,  "llama-3.1-8b-instruct",         150});
 *   router.registerRoute({QueryModelType::RELATIONAL, "llama-3.1-8b-instruct",     100});
 *   auto decision = router.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
 *   // decision.primary_type == QueryModelType::VECTOR
 *   // decision.selected_route->model_alias == "openai/text-embedding-3-small"
 * @endcode
 *
 * @see QueryModelType  for the set of classifiable query categories
 * @see ModelRoute      for route registration parameters
 */
class IModelRouter {
public:
    virtual ~IModelRouter() = default;

    /**
     * @brief Register a route for a given model type.
     *
     * If a route for the same @c model_type already exists it is replaced.
     *
     * @param route  Route to register
     */
    virtual void registerRoute(const ModelRoute& route) = 0;

    /**
     * @brief Remove the route for the given model type.
     *
     * No-op if no route is registered for @c type.
     */
    virtual void removeRoute(QueryModelType type) = 0;

    /**
     * @brief Classify an AQL query and select the best matching route.
     *
     * @param aql_query  AQL query string to route
     * @return           RoutingDecision (selected_route is empty when no
     *                   applicable route is registered)
     */
    virtual RoutingDecision route(const std::string& aql_query) const = 0;

    /**
     * @brief Classify an AQL query without selecting a route.
     *
     * Useful for introspection / logging without routing side-effects.
     *
     * @param aql_query  AQL query string
     * @return           All detected model types, ordered by confidence descending
     */
    virtual std::vector<QueryModelType> classify(const std::string& aql_query) const = 0;
};

/**
 * @brief Default production implementation of IModelRouter.
 *
 * Keyword-pattern classification runs in O(n) time (n = query length).
 * Route selection is O(k) where k is the number of registered routes.
 */
class AQLModelRouter : public IModelRouter {
public:
    AQLModelRouter()  = default;
    ~AQLModelRouter() override = default;

    void             registerRoute(const ModelRoute& route) override;
    void             removeRoute(QueryModelType type) override;
    RoutingDecision  route(const std::string& aql_query) const override;
    std::vector<QueryModelType> classify(const std::string& aql_query) const override;

private:
    std::vector<ModelRoute> routes_; ///< Registered routes, sorted by priority descending
};

} // namespace aql
} // namespace themis
