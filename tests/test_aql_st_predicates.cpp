#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include <iostream>

using namespace themis::query;

// ============================================================================
// ST_* Spatial Predicate Parsing Tests
// ============================================================================

TEST(AQLSpatialPredicateTest, ST_Intersects_SimpleBbox) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Intersects(doc.location, [[10.0, 50.0], [11.0, 51.0]]) "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    // Translate to ConjunctiveQuery
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    EXPECT_EQ(query.table, "places");
    ASSERT_TRUE(query.spatialPredicate.has_value());
    
    auto& sp = *query.spatialPredicate;
    EXPECT_EQ(sp.column, "location");
    EXPECT_EQ(sp.operation, PredicateSpatial::Operation::Intersects);
    ASSERT_TRUE(sp.bbox_min.has_value());
    ASSERT_TRUE(sp.bbox_max.has_value());
    EXPECT_EQ(sp.bbox_min->first, 10.0);
    EXPECT_EQ(sp.bbox_min->second, 50.0);
    EXPECT_EQ(sp.bbox_max->first, 11.0);
    EXPECT_EQ(sp.bbox_max->second, 51.0);
}

TEST(AQLSpatialPredicateTest, ST_Within_SimpleBbox) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Within(doc.geometry, [[0, 0], [100, 100]]) "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    ASSERT_TRUE(query.spatialPredicate.has_value());
    EXPECT_EQ(query.spatialPredicate->operation, PredicateSpatial::Operation::Within);
    EXPECT_EQ(query.spatialPredicate->column, "geometry");
}

TEST(AQLSpatialPredicateTest, ST_Contains_SimpleBbox) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Contains(doc.location, [[-180, -90], [180, 90]]) "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    ASSERT_TRUE(query.spatialPredicate.has_value());
    EXPECT_EQ(query.spatialPredicate->operation, PredicateSpatial::Operation::Contains);
}

TEST(AQLSpatialPredicateTest, ST_DWithin_WithDistance) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_DWithin(doc.location, [[10, 50], [11, 51]], 1000.0) "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    ASSERT_TRUE(query.spatialPredicate.has_value());
    EXPECT_EQ(query.spatialPredicate->operation, PredicateSpatial::Operation::DWithin);
    ASSERT_TRUE(query.spatialPredicate->distance.has_value());
    EXPECT_EQ(*query.spatialPredicate->distance, 1000.0);
}

TEST(AQLSpatialPredicateTest, ST_Intersects_WithAND_EqualityPredicate) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Intersects(doc.location, [[10, 50], [11, 51]]) AND doc.type == \"restaurant\" "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success) << result.errorMessage;
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    
    // Should have spatial predicate
    ASSERT_TRUE(query.spatialPredicate.has_value());
    EXPECT_EQ(query.spatialPredicate->column, "location");
    EXPECT_EQ(query.spatialPredicate->operation, PredicateSpatial::Operation::Intersects);
    
    // Should also have equality predicate
    ASSERT_EQ(query.predicates.size(), 1);
    EXPECT_EQ(query.predicates[0].column, "type");
    EXPECT_EQ(query.predicates[0].value, "restaurant");
}

TEST(AQLSpatialPredicateTest, ST_Within_WithAND_RangePredicate) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Within(doc.location, [[0, 0], [100, 100]]) AND doc.rating > 4.0 "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success) << result.errorMessage;
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    
    // Should have spatial predicate
    ASSERT_TRUE(query.spatialPredicate.has_value());
    EXPECT_EQ(query.spatialPredicate->operation, PredicateSpatial::Operation::Within);
    
    // Should also have range predicate
    ASSERT_EQ(query.rangePredicates.size(), 1);
    EXPECT_EQ(query.rangePredicates[0].column, "rating");
}

TEST(AQLSpatialPredicateTest, ST_Intersects_WithAND_MultiplePredicates) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Intersects(doc.location, [[10, 50], [11, 51]]) AND doc.type == \"restaurant\" AND doc.rating >= 4.0 "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    ASSERT_TRUE(result.success) << result.errorMessage;
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    
    // Should have spatial predicate
    ASSERT_TRUE(query.spatialPredicate.has_value());
    
    // Should have equality and range predicates
    ASSERT_EQ(query.predicates.size(), 1);
    ASSERT_EQ(query.rangePredicates.size(), 1);
}

TEST(AQLSpatialPredicateTest, ST_Intersects_InvalidBbox_MissingValues) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Intersects(doc.location, [[10, 50]]) "  // Missing second pair
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    // Should succeed but skip spatial predicate (no valid bbox)
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    EXPECT_FALSE(query.spatialPredicate.has_value());
}

TEST(AQLSpatialPredicateTest, ST_Intersects_InvalidBbox_MinMaxSwapped) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Intersects(doc.location, [[11, 51], [10, 50]]) "  // max < min
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    // Should succeed but skip spatial predicate (invalid bbox)
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ConjunctiveQuery>(result.query));
    
    auto& query = std::get<ConjunctiveQuery>(result.query);
    EXPECT_FALSE(query.spatialPredicate.has_value());
}

TEST(AQLSpatialPredicateTest, ST_DWithin_MissingDistance) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_DWithin(doc.location, [[10, 50], [11, 51]]) "  // Missing distance
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    // Should fail - ST_DWithin requires 3 arguments
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.errorMessage.find("requires 3 arguments"), std::string::npos);
}

TEST(AQLSpatialPredicateTest, UnsupportedSpatialFunction) {
    AQLParser parser;
    auto ast = parser.parse(
        "FOR doc IN places "
        "FILTER ST_Unknown(doc.location, [[10, 50], [11, 51]]) "
        "RETURN doc"
    );
    
    ASSERT_TRUE(ast.has_value()) << ast.error().message();
    
    AQLTranslator translator;
    auto result = translator.translate(*ast.value());
    
    // Should fail with unsupported function error
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.errorMessage.find("Unsupported spatial function"), std::string::npos);
}
