/**
 * @file aql_model_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct RoutingDecision {
    QueryModelType primary_type = QueryModelType::UNKNOWN;

    std::vector<QueryModelType> detected_types;

    std::optional<ModelRoute> selected_route;

    std::optional<ModelRoute> fallback_route;

    std::string explanation;
};

class IModelRouter {
public:
    /**
     * @brief IModel Router.
     * @return Return value.
     */
    virtual ~IModelRouter() = default;

    /**
     * @brief Register Route.
     * @param[in] route Input parameter.
     */
    virtual void registerRoute(const ModelRoute& route) = 0;

    /**
     * @brief Remove Route.
     * @param[in] type Input parameter.
     */
    virtual void removeRoute(QueryModelType type) = 0;

    /**
     * @brief Route.
     * @param[in] aql_query Input parameter.
     * @return Return value.
     */
    virtual RoutingDecision route(const std::string& aql_query) const = 0;

    /**
     * @brief Classify the semantic intent of a query.
     * @param[in] aql_query Input parameter.
     * @return Return value.
     */
    virtual std::vector<QueryModelType> classify(const std::string& aql_query) const = 0;
};

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
