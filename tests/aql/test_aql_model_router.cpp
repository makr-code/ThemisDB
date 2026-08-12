/**
 * @file test_aql_model_router.cpp
 * @brief Unit tests for AQLModelRouter (ROUTER-01..20)
 */

#include <gtest/gtest.h>
#include "aql/aql_model_router.h"

#include <algorithm>
#include <memory>
#include <string>

using namespace themis::aql;

static bool hasType(const std::vector<QueryModelType>& types, QueryModelType t) {
    return std::find(types.begin(), types.end(), t) != types.end();
}

class AQLModelRouterTest : public ::testing::Test {
protected:
    AQLModelRouter router;

    void SetUp() override {
        router.registerRoute({QueryModelType::VECTOR,    "embed-model",   200});
        router.registerRoute({QueryModelType::GRAPH,     "graph-model",   150});
        router.registerRoute({QueryModelType::GEO,       "geo-model",     140});
        router.registerRoute({QueryModelType::FULLTEXT,  "ft-model",      130});
        router.registerRoute({QueryModelType::TIMESERIES,"ts-model",      120});
        router.registerRoute({QueryModelType::RELATIONAL,"generic-model", 100});
    }
};

// ROUTER-01: Vector query routes to vector model
TEST_F(AQLModelRouterTest, ROUTER01_VectorQuery) {
    auto d = router.route("FOR d IN docs FILTER KNN(d.embedding, @q) < 0.5 RETURN d");
    EXPECT_EQ(d.primary_type, QueryModelType::VECTOR);
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "embed-model");
}

// ROUTER-02: Graph traversal routes to graph model
TEST_F(AQLModelRouterTest, ROUTER02_GraphQuery) {
    auto d = router.route("FOR v, e IN 1..3 OUTBOUND @start GRAPH 'social' RETURN v");
    EXPECT_EQ(d.primary_type, QueryModelType::GRAPH);
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "graph-model");
}

// ROUTER-03: Geo query routes to geo model
TEST_F(AQLModelRouterTest, ROUTER03_GeoQuery) {
    auto d = router.route("FOR p IN places FILTER ST_WITHIN(p.geo, @zone) RETURN p");
    EXPECT_EQ(d.primary_type, QueryModelType::GEO);
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "geo-model");
}

// ROUTER-04: Full-text query routes to fulltext model
TEST_F(AQLModelRouterTest, ROUTER04_FulltextQuery) {
    auto d = router.route("FOR d IN docs FILTER BM25(d, @terms) > 0.5 RETURN d");
    EXPECT_EQ(d.primary_type, QueryModelType::FULLTEXT);
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "ft-model");
}

// ROUTER-05: Timeseries query routes to ts model
TEST_F(AQLModelRouterTest, ROUTER05_TimeseriesQuery) {
    auto d = router.route("FOR t IN metrics FILTER TIME_TRUNC(t.ts, 'hour') == @h RETURN t");
    EXPECT_EQ(d.primary_type, QueryModelType::TIMESERIES);
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "ts-model");
}

// ROUTER-06: Plain relational query routes to generic model
TEST_F(AQLModelRouterTest, ROUTER06_RelationalQuery) {
    auto d = router.route("FOR u IN users FILTER u.active == true RETURN u.name");
    EXPECT_EQ(d.primary_type, QueryModelType::RELATIONAL);
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "generic-model");
}

// ROUTER-07: Empty query → UNKNOWN
TEST_F(AQLModelRouterTest, ROUTER07_EmptyQuery) {
    auto d = router.route("");
    EXPECT_EQ(d.primary_type, QueryModelType::UNKNOWN);
}

// ROUTER-08: classify() returns all matching types
TEST_F(AQLModelRouterTest, ROUTER08_ClassifyMultipleTypes) {
    // KNN + BM25 → should detect VECTOR and FULLTEXT
    auto types = router.classify("FOR d IN docs FILTER KNN(d.emb, @q) < 0.1 AND BM25(d, @t) > 0.3 RETURN d");
    EXPECT_TRUE(hasType(types, QueryModelType::VECTOR));
    EXPECT_TRUE(hasType(types, QueryModelType::FULLTEXT));
}

// ROUTER-09: No routes registered → selected_route is empty
TEST_F(AQLModelRouterTest, ROUTER09_NoRoute) {
    AQLModelRouter empty_router;
    auto d = empty_router.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    EXPECT_FALSE(d.selected_route.has_value());
}

// ROUTER-10: Fallback route is used when primary type has no route
TEST_F(AQLModelRouterTest, ROUTER10_FallbackRoute) {
    AQLModelRouter r;
    r.registerRoute({QueryModelType::RELATIONAL, "fallback-model", 100});
    // VECTOR route not registered; should fallback to RELATIONAL
    auto d = r.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    EXPECT_EQ(d.primary_type, QueryModelType::VECTOR);
    EXPECT_FALSE(d.selected_route.has_value());  // no VECTOR route
    ASSERT_TRUE(d.fallback_route.has_value());
    EXPECT_EQ(d.fallback_route->model_alias, "fallback-model");
}

// ROUTER-11: removeRoute removes the route
TEST_F(AQLModelRouterTest, ROUTER11_RemoveRoute) {
    router.removeRoute(QueryModelType::VECTOR);
    auto d = router.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    EXPECT_EQ(d.primary_type, QueryModelType::VECTOR);
    EXPECT_FALSE(d.selected_route.has_value());
}

// ROUTER-12: registerRoute replaces existing route for same type
TEST_F(AQLModelRouterTest, ROUTER12_ReplaceRoute) {
    router.registerRoute({QueryModelType::VECTOR, "new-embed-model", 200});
    auto d = router.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    ASSERT_TRUE(d.selected_route.has_value());
    EXPECT_EQ(d.selected_route->model_alias, "new-embed-model");
}

// ROUTER-13: Disabled route is skipped
TEST_F(AQLModelRouterTest, ROUTER13_DisabledRoute) {
    ModelRoute disabled_route{QueryModelType::VECTOR, "disabled-model", 200};
    disabled_route.enabled = false;
    AQLModelRouter r;
    r.registerRoute(disabled_route);
    r.registerRoute({QueryModelType::RELATIONAL, "generic-model", 100});
    auto d = r.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    // disabled VECTOR route → no selected_route for VECTOR; fallback to RELATIONAL
    EXPECT_FALSE(d.selected_route.has_value());
}

// ROUTER-14: explanation is not empty
TEST_F(AQLModelRouterTest, ROUTER14_ExplanationNotEmpty) {
    auto d = router.route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    EXPECT_FALSE(d.explanation.empty());
}

// ROUTER-15: ANN keyword detected as VECTOR
TEST_F(AQLModelRouterTest, ROUTER15_ANNKeyword) {
    auto types = router.classify("FOR d IN docs FILTER ANN(d.emb, @q) < 0.3 RETURN d");
    EXPECT_TRUE(hasType(types, QueryModelType::VECTOR));
}

// ROUTER-16: TRAVERSE keyword detected as GRAPH
TEST_F(AQLModelRouterTest, ROUTER16_TraverseKeyword) {
    auto types = router.classify("FOR v IN TRAVERSAL(startNode, 'outbound') RETURN v");
    EXPECT_TRUE(hasType(types, QueryModelType::GRAPH));
}

// ROUTER-17: WITHIN keyword detected as GEO
TEST_F(AQLModelRouterTest, ROUTER17_WithinKeyword) {
    auto types = router.classify("FOR p IN places FILTER WITHIN(p.lat, p.lon, 5000) RETURN p");
    EXPECT_TRUE(hasType(types, QueryModelType::GEO));
}

// ROUTER-18: RELATIONAL always included as fallback type
TEST_F(AQLModelRouterTest, ROUTER18_RelationalAlwaysPresent) {
    // A GEO query should still have RELATIONAL as last-resort fallback type
    auto types = router.classify("FOR p IN places FILTER ST_DISTANCE(p.geo, @pt) < 100 RETURN p");
    EXPECT_TRUE(hasType(types, QueryModelType::RELATIONAL));
}

// ROUTER-19: IModelRouter interface accessible via polymorphism
TEST_F(AQLModelRouterTest, ROUTER19_PolymorphicAccess) {
    std::unique_ptr<IModelRouter> iface = std::make_unique<AQLModelRouter>();
    iface->registerRoute({QueryModelType::VECTOR, "v-model", 200});
    auto d = iface->route("FOR d IN docs FILTER KNN(d.emb, @q) < 0.5 RETURN d");
    EXPECT_EQ(d.primary_type, QueryModelType::VECTOR);
}

// ROUTER-20: removeRoute on non-existent type is a no-op
TEST_F(AQLModelRouterTest, ROUTER20_RemoveNonExistent) {
    EXPECT_NO_THROW(router.removeRoute(QueryModelType::PROCESS));
}
