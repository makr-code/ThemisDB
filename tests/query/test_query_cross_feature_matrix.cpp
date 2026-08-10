/**
 * @file test_query_cross_feature_matrix.cpp
 * @brief Cross-feature regression matrix bootstrap for Query Block D.
 *
 * Validates parser/planner-level interoperability across:
 *  - AQL + FTS SEARCH clause
 *  - AQL + geospatial predicates
 *  - Multi-FOR / join-style query shapes
 *  - Plan-cache deadline handling on cross-feature query text
 *
 * The suite is intentionally parser/planner focused so it can run as an early
 * regression gate while executor/backend wiring expands toward the broader
 * 1,000+ test target tracked in `src/query/ROADMAP.md`.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "query/aql_parser.h"
#include "query/geospatial_cost_model.h"
#include "query/plan_cache.h"
#include "query/query_optimizer.h"

using namespace themis::query;
using namespace std::chrono_literals;

namespace {

std::shared_ptr<Query> parseOrFail(AQLParser& parser, const std::string& aql) {
    auto result = parser.parse(aql);
    EXPECT_TRUE(result.has_value()) << "parse() failed for: " << aql
                                    << "\nerror: "
                                    << (result.has_value() ? "" : result.error().message());
    if (!result.has_value()) {
        return nullptr;
    }
    return result.value();
}

PlanCache::Config crossFeatureCacheConfig() {
    PlanCache::Config cfg;
    cfg.max_entries = 32;
    cfg.max_plan_age = 1h;
    cfg.statistics_drift_factor = 10.0;
    return cfg;
}

PlanCache::Statistics makeStats() {
    return PlanCache::Statistics({{"articles", 50'000}, {"locations", 10'000}});
}

QueryOptimizer::Plan makePlan() {
    QueryOptimizer::Plan plan;
    plan.nlp_complexity = 0.25;
    return plan;
}

}  // namespace

class QueryCrossFeatureMatrixTest : public ::testing::Test {
protected:
    AQLParser parser_;
};

TEST_F(QueryCrossFeatureMatrixTest, QCFM01_SearchAndGeoFilterCompose) {
    auto query = parseOrFail(
        parser_,
        R"(FOR doc IN articles
           SEARCH PHRASE(doc, "vector database") ANALYZER "text_en"
           FILTER ST_DISTANCE(doc.location, [13.4050, 52.5200]) < 5000
           SORT doc.rank DESC
           LIMIT 5
           RETURN doc)");

    ASSERT_NE(query, nullptr);
    ASSERT_NE(query->search_clause, nullptr);
    ASSERT_EQ(query->search_clause->predicates.size(), 1u);
    EXPECT_EQ(query->search_clause->predicates[0].pred_type, FtsPredType::PHRASE);
    EXPECT_EQ(query->search_clause->predicates[0].analyzer, "text_en");
    ASSERT_EQ(query->filters.size(), 1u);
    EXPECT_NE(query->filters[0]->toJSON().dump().find("ST_DISTANCE"), std::string::npos);
    ASSERT_NE(query->sort, nullptr);
    ASSERT_NE(query->limit, nullptr);
    EXPECT_EQ(query->limit->count, 5);
}

TEST_F(QueryCrossFeatureMatrixTest, QCFM02_SearchAndJoinShapeRemainStable) {
    auto query = parseOrFail(
        parser_,
        R"(FOR doc IN articles
           FOR edge IN article_edges
           SEARCH STARTS_WITH(doc, "ops") BOOST 1.5
           FILTER doc.id == edge.article_id
           RETURN {doc, edge})");

    ASSERT_NE(query, nullptr);
    EXPECT_GE(query->for_nodes.size(), 2u);
    ASSERT_NE(query->search_clause, nullptr);
    ASSERT_EQ(query->search_clause->predicates.size(), 1u);
    EXPECT_EQ(query->search_clause->predicates[0].pred_type, FtsPredType::PREFIX);
    EXPECT_DOUBLE_EQ(query->search_clause->predicates[0].boost, 1.5);
    ASSERT_EQ(query->filters.size(), 1u);
    EXPECT_NE(query->filters[0]->toJSON().dump().find("article_id"), std::string::npos);
}

TEST_F(QueryCrossFeatureMatrixTest, QCFM03_GeospatialPlanningInputsStayMeasurable) {
    auto query = parseOrFail(
        parser_,
        R"(FOR doc IN locations
           SEARCH doc == "warehouse"
           FILTER ST_DISTANCE(doc.location, [8.6821, 50.1109]) < 10000
           RETURN doc)");

    ASSERT_NE(query, nullptr);
    ASSERT_NE(query->search_clause, nullptr);
    ASSERT_EQ(query->filters.size(), 1u);

    std::vector<std::pair<double, double>> points = {
        {8.68, 50.11}, {8.69, 50.12}, {8.70, 50.10}, {8.71, 50.09}};
    auto histogram = GeospatialCostEstimator::buildSpatialHistogram(points, 2);
    histogram.totalPoints = 10'000;

    const auto estimate = GeospatialCostEstimator::estimateDistanceCost(
        histogram.totalPoints, 10'000.0, true, &histogram);

    EXPECT_GT(estimate.cpuCostUs, 0.0);
    EXPECT_GT(estimate.estimatedRows, 0u);
    EXPECT_NE(query->filters[0]->toJSON().dump().find("ST_DISTANCE"), std::string::npos);
}

TEST_F(QueryCrossFeatureMatrixTest, QCFM04_CrossFeaturePlanCacheHonoursDeadlines) {
    const std::string aql =
        R"(FOR doc IN articles
           SEARCH PHRASE(doc, "federated retry")
           FILTER ST_DISTANCE(doc.location, [7.0, 50.0]) < 25000
           LIMIT 10
           RETURN doc)";

    auto query = parseOrFail(parser_, aql);
    ASSERT_NE(query, nullptr);

    PlanCache cache(crossFeatureCacheConfig());
    cache.put(aql, makePlan(), makeStats());

    const auto past_deadline = std::chrono::steady_clock::now() - 50ms;
    const auto past = cache.get(aql, makeStats(), "", past_deadline);
    EXPECT_FALSE(past.has_value());

    const auto future_deadline = std::chrono::steady_clock::now() + 5s;
    const auto future = cache.get(aql, makeStats(), "", future_deadline);
    EXPECT_TRUE(future.has_value());
}
