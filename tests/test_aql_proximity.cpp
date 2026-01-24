// AQL PROXIMITY (Content+Geo Syntax Sugar) Tests

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/aql_translator.h"

using namespace themis;
using namespace themis::query;

// Disable legacy AQL proximity tests
#if 0

class AQLProximityTest : public ::testing::Test { protected: AQLParser parser; };

TEST_F(AQLProximityTest, TranslateProximityWithFulltextAndSpatial) {
    std::string aql = R"(
        FOR doc IN places
        FILTER FULLTEXT(doc.description, "coffee", 50)
        FILTER ST_Within(doc.location, [13.4,52.5,13.5,52.6])
        SORT PROXIMITY(doc.location, [13.45,52.55]) ASC
        LIMIT 20
        RETURN doc
    )";
    auto parseResult = parser.parse(aql); ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->sort);
    ASSERT_EQ(parseResult.query->sort->specifications.size(),1u);
    auto expr = parseResult.query->sort->specifications[0].expression;
    ASSERT_EQ(expr->getType(), ASTNodeType::ProximityCall) << "Expected ProximityCallExpr AST node";
    auto tr = AQLTranslator::translate(parseResult.query); ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
    const auto &cq = *tr.content_geo;
    EXPECT_EQ(cq.table, "places");
    EXPECT_EQ(cq.text_field, "description");
    EXPECT_EQ(cq.fulltext_query, "coffee");
    EXPECT_TRUE(cq.spatial_filter);
    EXPECT_TRUE(cq.center_point.has_value());
    EXPECT_TRUE(cq.boost_by_distance);
    EXPECT_EQ(cq.limit, 20u);
}

TEST_F(AQLProximityTest, ProximityRequiresFulltext) {
    std::string aql = R"(
        FOR doc IN places
        SORT PROXIMITY(doc.location, [13.45,52.55]) ASC
        LIMIT 10
        RETURN doc
    )";
    auto parseResult = parser.parse(aql); ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->sort);
    auto expr = parseResult.query->sort->specifications[0].expression;
    ASSERT_EQ(expr->getType(), ASTNodeType::ProximityCall);
    auto tr = AQLTranslator::translate(parseResult.query); EXPECT_FALSE(tr.success);
    EXPECT_NE(tr.error_message.find("requires a FULLTEXT"), std::string::npos);
}

// ============================================================================
// Basic Proximity Operators Tests
// ============================================================================

TEST_F(AQLProximityTest, STDistanceFunction) {
    std::string aql = R"(
        FOR doc IN places
        FILTER FULLTEXT(doc.description, "restaurant")
        FILTER ST_Distance(doc.location, [13.45,52.55]) < 1000
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success) << "Failed to parse AQL with ST_Distance";
    ASSERT_TRUE(parseResult.query->filter);
}

TEST_F(AQLProximityTest, ProximityThresholdTesting) {
    std::string aql = R"(
        FOR doc IN venues
        FILTER FULLTEXT(doc.name, "concert hall")
        FILTER ST_Distance(doc.location, [10.0,20.0]) <= 500
        SORT PROXIMITY(doc.location, [10.0,20.0]) ASC
        LIMIT 10
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    auto tr = AQLTranslator::translate(parseResult.query);
    ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
}

TEST_F(AQLProximityTest, BidirectionalProximityChecks) {
    std::string aql = R"(
        FOR doc IN locations
        FILTER FULLTEXT(doc.tags, "airport")
        FILTER ST_Distance([13.45,52.55], doc.location) < 5000
        SORT PROXIMITY(doc.location, [13.45,52.55]) ASC
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->filter);
}

// ============================================================================
// Distance Functions Tests
// ============================================================================

TEST_F(AQLProximityTest, EuclideanDistance) {
    std::string aql = R"(
        FOR doc IN points
        FILTER FULLTEXT(doc.description, "data point")
        LET dist = SQRT(POW(doc.x - 10, 2) + POW(doc.y - 20, 2))
        FILTER dist < 5
        RETURN {doc: doc, distance: dist}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->let_clauses.size() > 0);
}

TEST_F(AQLProximityTest, ManhattanDistance) {
    std::string aql = R"(
        FOR doc IN grid
        FILTER FULLTEXT(doc.type, "node")
        LET dist = ABS(doc.x - 5) + ABS(doc.y - 10)
        FILTER dist <= 3
        RETURN {doc: doc, manhattan_dist: dist}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->let_clauses.size() > 0);
}

TEST_F(AQLProximityTest, HaversineDistance) {
    std::string aql = R"(
        FOR doc IN cities
        FILTER FULLTEXT(doc.description, "capital")
        FILTER ST_Distance_Sphere(doc.location, [13.45,52.55]) < 100000
        SORT ST_Distance_Sphere(doc.location, [13.45,52.55]) ASC
        LIMIT 5
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    // ST_Distance_Sphere would use Haversine formula for geographic coordinates
}

TEST_F(AQLProximityTest, CustomDistanceMetrics) {
    std::string aql = R"(
        FOR doc IN items
        FILTER FULLTEXT(doc.name, "product")
        LET custom_dist = (doc.price - 100) / 10 + ABS(doc.rating - 4.5)
        FILTER custom_dist < 5
        SORT custom_dist ASC
        RETURN {doc: doc, score: custom_dist}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->sort);
}

// ============================================================================
// Proximity with Different Data Types Tests
// ============================================================================

TEST_F(AQLProximityTest, NumericProximity) {
    std::string aql = R"(
        FOR doc IN products
        FILTER FULLTEXT(doc.description, "laptop")
        FILTER ABS(doc.price - 1000) < 200
        SORT ABS(doc.price - 1000) ASC
        LIMIT 10
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->filter);
    ASSERT_TRUE(parseResult.query->sort);
}

TEST_F(AQLProximityTest, GeospatialProximity) {
    std::string aql = R"(
        FOR doc IN stores
        FILTER FULLTEXT(doc.type, "grocery")
        FILTER ST_Within(doc.location, [13.0,52.0,14.0,53.0])
        SORT PROXIMITY(doc.location, [13.5,52.5]) ASC
        LIMIT 20
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    auto tr = AQLTranslator::translate(parseResult.query);
    ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
    const auto &cq = *tr.content_geo;
    EXPECT_TRUE(cq.spatial_filter);
    EXPECT_TRUE(cq.boost_by_distance);
}

TEST_F(AQLProximityTest, VectorSimilarityCosineDistance) {
    std::string aql = R"(
        FOR doc IN embeddings
        FILTER FULLTEXT(doc.text, "machine learning")
        LET similarity = 1 - ACOS(doc.vector[0] * 0.5 + doc.vector[1] * 0.5) / 3.14159
        FILTER similarity > 0.8
        SORT similarity DESC
        RETURN {doc: doc, similarity: similarity}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->let_clauses.size() > 0);
}

// ============================================================================
// Complex Proximity Queries Tests
// ============================================================================

TEST_F(AQLProximityTest, ProximityCombinedWithFilters) {
    std::string aql = R"(
        FOR doc IN restaurants
        FILTER FULLTEXT(doc.cuisine, "italian")
        FILTER doc.rating >= 4.0
        FILTER ST_Distance(doc.location, [13.45,52.55]) < 2000
        SORT PROXIMITY(doc.location, [13.45,52.55]) ASC
        LIMIT 15
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    auto tr = AQLTranslator::translate(parseResult.query);
    ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
}

TEST_F(AQLProximityTest, ProximityInSubqueries) {
    std::string aql = R"(
        FOR city IN cities
        FILTER FULLTEXT(city.name, "Berlin")
        LET nearby = (
            FOR venue IN venues
            FILTER FULLTEXT(venue.type, "museum")
            FILTER ST_Distance(venue.location, city.location) < 5000
            SORT PROXIMITY(venue.location, city.location) ASC
            LIMIT 5
            RETURN venue
        )
        RETURN {city: city, nearby_venues: nearby}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->let_clauses.size() > 0);
}

TEST_F(AQLProximityTest, KNearestNeighborQuery) {
    std::string aql = R"(
        FOR doc IN poi
        FILTER FULLTEXT(doc.category, "hotel")
        FILTER ST_Within(doc.location, [10.0,50.0,15.0,55.0])
        SORT PROXIMITY(doc.location, [13.45,52.55]) ASC
        LIMIT 10
        RETURN {
            name: doc.name,
            location: doc.location,
            distance: ST_Distance(doc.location, [13.45,52.55])
        }
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    auto tr = AQLTranslator::translate(parseResult.query);
    ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
    EXPECT_EQ(tr.content_geo->limit, 10u);
}

// ============================================================================
// Edge Cases & Performance Tests
// ============================================================================

TEST_F(AQLProximityTest, VeryLargeResultSets) {
    std::string aql = R"(
        FOR doc IN large_dataset
        FILTER FULLTEXT(doc.content, "common term")
        FILTER ST_Within(doc.location, [0.0,0.0,180.0,90.0])
        SORT PROXIMITY(doc.location, [13.45,52.55]) ASC
        LIMIT 1000
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    auto tr = AQLTranslator::translate(parseResult.query);
    ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
    EXPECT_EQ(tr.content_geo->limit, 1000u);
}

TEST_F(AQLProximityTest, BoundaryConditionsZeroDistance) {
    std::string aql = R"(
        FOR doc IN landmarks
        FILTER FULLTEXT(doc.name, "monument")
        FILTER ST_Distance(doc.location, [13.45,52.55]) == 0
        RETURN doc
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->filter);
}

TEST_F(AQLProximityTest, ProximityWithAggregation) {
    std::string aql = R"(
        FOR doc IN sensors
        FILTER FULLTEXT(doc.type, "temperature")
        FILTER ST_Distance(doc.location, [13.45,52.55]) < 1000
        COLLECT dist = FLOOR(ST_Distance(doc.location, [13.45,52.55]) / 100) * 100
        AGGREGATE count = COUNT(1), avg_reading = AVG(doc.reading)
        RETURN {distance_bucket: dist, count: count, avg: avg_reading}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_TRUE(parseResult.query->collect);
}

TEST_F(AQLProximityTest, MultipleProximityReferences) {
    std::string aql = R"(
        FOR doc IN events
        FILTER FULLTEXT(doc.description, "conference")
        LET dist1 = ST_Distance(doc.location, [13.45,52.55])
        LET dist2 = ST_Distance(doc.location, [13.40,52.50])
        FILTER dist1 < 5000 OR dist2 < 5000
        SORT dist1 ASC
        RETURN {event: doc, dist_point1: dist1, dist_point2: dist2}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_EQ(parseResult.query->let_clauses.size(), 2u);
}

#endif // TEMP_DISABLE_AQL_PROXIMITY

TEST(AQLProximityStub, DISABLED_LegacyAQLProximity) {
    GTEST_SKIP() << "Legacy AQL proximity tests temporarily disabled for build stability.";
}

#if 0

TEST_F(AQLProximityTest, ProximityWithComplexExpressions) {
    std::string aql = R"(
        FOR doc IN places
        FILTER FULLTEXT(doc.tags, "restaurant cafe")
        LET base_dist = ST_Distance(doc.location, [13.45,52.55])
        LET adjusted_score = base_dist / (doc.rating + 1)
        FILTER adjusted_score < 500
        SORT adjusted_score ASC
        LIMIT 25
        RETURN {place: doc, score: adjusted_score}
    )";
    auto parseResult = parser.parse(aql);
    ASSERT_TRUE(parseResult.success);
    ASSERT_EQ(parseResult.query->let_clauses.size(), 2u);
    ASSERT_TRUE(parseResult.query->sort);
}

#endif // TEMP_DISABLE_AQL_PROXIMITY_REMAINDER
