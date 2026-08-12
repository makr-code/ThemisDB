#include <gtest/gtest.h>
#include "api/graphql.h"
#include "api/graphql_metrics.h"
#include <thread>
#include <chrono>

using namespace themis::graphql;

// ============================================================================
// Geo Scalar Types Tests
// ============================================================================

TEST(GraphQLGeoScalars, SchemaIncludesLatitudeLongitude) {
    Schema schema = ThemisSchemaBuilder::build();
    
    // Check that Latitude scalar exists
    const auto* latType = schema.getType("Latitude");
    ASSERT_NE(latType, nullptr);
    EXPECT_EQ(latType->kind, TypeDefinition::Kind::Scalar);
    EXPECT_FALSE(latType->description.empty());
    EXPECT_TRUE(latType->description.find("-90") != std::string::npos);
    EXPECT_TRUE(latType->description.find("90") != std::string::npos);
    
    // Check that Longitude scalar exists
    const auto* lonType = schema.getType("Longitude");
    ASSERT_NE(lonType, nullptr);
    EXPECT_EQ(lonType->kind, TypeDefinition::Kind::Scalar);
    EXPECT_FALSE(lonType->description.empty());
    EXPECT_TRUE(lonType->description.find("-180") != std::string::npos);
    EXPECT_TRUE(lonType->description.find("180") != std::string::npos);
}

TEST(GraphQLGeoScalars, SchemaIncludesGeoPoint) {
    Schema schema = ThemisSchemaBuilder::build();
    
    const auto* geoPointType = schema.getType("GeoPoint");
    ASSERT_NE(geoPointType, nullptr);
    EXPECT_EQ(geoPointType->kind, TypeDefinition::Kind::Object);
    EXPECT_FALSE(geoPointType->description.empty());
    
    // Check fields
    ASSERT_EQ(geoPointType->fields.size(), 2);
    
    // Find lat field
    auto latIt = std::find_if(geoPointType->fields.begin(), geoPointType->fields.end(),
                              [](const FieldDefinition& f) { return f.name == "lat"; });
    ASSERT_NE(latIt, geoPointType->fields.end());
    EXPECT_EQ(latIt->type.name, "Latitude");
    EXPECT_TRUE(latIt->type.is_non_null);
    
    // Find lon field
    auto lonIt = std::find_if(geoPointType->fields.begin(), geoPointType->fields.end(),
                              [](const FieldDefinition& f) { return f.name == "lon"; });
    ASSERT_NE(lonIt, geoPointType->fields.end());
    EXPECT_EQ(lonIt->type.name, "Longitude");
    EXPECT_TRUE(lonIt->type.is_non_null);
}

TEST(GraphQLGeoScalars, SchemaIncludesGeoPointInput) {
    Schema schema = ThemisSchemaBuilder::build();
    
    const auto* geoPointInputType = schema.getType("GeoPointInput");
    ASSERT_NE(geoPointInputType, nullptr);
    EXPECT_EQ(geoPointInputType->kind, TypeDefinition::Kind::InputObject);
    
    // Check fields
    ASSERT_EQ(geoPointInputType->fields.size(), 2);
}

TEST(GraphQLGeoScalars, SchemaIncludesGeoJSON) {
    Schema schema = ThemisSchemaBuilder::build();
    
    const auto* geoJSONType = schema.getType("GeoJSON");
    ASSERT_NE(geoJSONType, nullptr);
    EXPECT_EQ(geoJSONType->kind, TypeDefinition::Kind::Scalar);
    EXPECT_TRUE(geoJSONType->description.find("GeoJSON") != std::string::npos);
    EXPECT_TRUE(geoJSONType->description.find("RFC") != std::string::npos);
}

TEST(GraphQLGeoScalars, SDLGenerationIncludesGeoTypes) {
    Schema schema = ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();
    
    // Check that geo types are in SDL
    EXPECT_TRUE(sdl.find("scalar Latitude") != std::string::npos);
    EXPECT_TRUE(sdl.find("scalar Longitude") != std::string::npos);
    EXPECT_TRUE(sdl.find("scalar GeoJSON") != std::string::npos);
    EXPECT_TRUE(sdl.find("type GeoPoint") != std::string::npos);
    EXPECT_TRUE(sdl.find("input GeoPointInput") != std::string::npos);
}

// ============================================================================
// Introspection Policy Tests
// ============================================================================

TEST(GraphQLIntrospection, DefaultIntrospectionEnabled) {
    Schema schema;
    EXPECT_TRUE(schema.isIntrospectionEnabled());
}

TEST(GraphQLIntrospection, CanDisableIntrospection) {
    Schema schema;
    schema.setIntrospectionEnabled(false);
    EXPECT_FALSE(schema.isIntrospectionEnabled());
}

TEST(GraphQLIntrospection, CanEnableIntrospection) {
    Schema schema;
    schema.setIntrospectionEnabled(false);
    schema.setIntrospectionEnabled(true);
    EXPECT_TRUE(schema.isIntrospectionEnabled());
}

TEST(GraphQLIntrospection, ProductionModeDisablesIntrospection) {
    Schema schema = ThemisSchemaBuilder::build();
    
    // Simulate production configuration
    schema.setIntrospectionEnabled(false);
    EXPECT_FALSE(schema.isIntrospectionEnabled());
    
    // Development mode re-enables it
    schema.setIntrospectionEnabled(true);
    EXPECT_TRUE(schema.isIntrospectionEnabled());
}

// ============================================================================
// Metrics Tests
// ============================================================================

TEST(GraphQLMetrics, RecordQueryMetrics) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    // Record a successful query
    metrics.recordQuery("Query", 100, true, 3, 5);
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    EXPECT_EQ(queryMetrics.total_queries.load(), 1);
    EXPECT_EQ(queryMetrics.failed_queries.load(), 0);
    EXPECT_EQ(queryMetrics.total_execution_time_ms.load(), 100);
    EXPECT_EQ(queryMetrics.max_execution_time_ms.load(), 100);
    EXPECT_DOUBLE_EQ(queryMetrics.avgExecutionTimeMs(), 100.0);
    EXPECT_DOUBLE_EQ(queryMetrics.errorRate(), 0.0);
}

TEST(GraphQLMetrics, RecordFailedQuery) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    // Record a failed query
    metrics.recordQuery("Query", 50, false, 2, 3);
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    EXPECT_EQ(queryMetrics.total_queries.load(), 1);
    EXPECT_EQ(queryMetrics.failed_queries.load(), 1);
    EXPECT_DOUBLE_EQ(queryMetrics.errorRate(), 1.0);
}

TEST(GraphQLMetrics, MultipleQueries) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    // Record multiple queries
    metrics.recordQuery("Query", 100, true, 3, 5);
    metrics.recordQuery("Query", 200, true, 4, 6);
    metrics.recordQuery("Query", 150, false, 2, 4);
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    EXPECT_EQ(queryMetrics.total_queries.load(), 3);
    EXPECT_EQ(queryMetrics.failed_queries.load(), 1);
    EXPECT_EQ(queryMetrics.max_execution_time_ms.load(), 200);
    EXPECT_DOUBLE_EQ(queryMetrics.avgExecutionTimeMs(), 150.0);  // (100+200+150)/3
    EXPECT_DOUBLE_EQ(queryMetrics.errorRate(), 1.0/3.0);
}

TEST(GraphQLMetrics, TrackQueryDepthAndFields) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    metrics.recordQuery("Query", 100, true, 5, 10);
    metrics.recordQuery("Query", 100, true, 3, 8);
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    EXPECT_DOUBLE_EQ(queryMetrics.avgQueryDepth(), 4.0);  // (5+3)/2
    EXPECT_DOUBLE_EQ(queryMetrics.avgFieldCount(), 9.0);  // (10+8)/2
}

TEST(GraphQLMetrics, SeparateMetricsByOperationType) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    metrics.recordQuery("Query", 100, true, 3, 5);
    metrics.recordQuery("Mutation", 200, true, 2, 4);
    metrics.recordQuery("Subscription", 50, true, 1, 2);
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    const auto& mutationMetrics = metrics.getMetrics("Mutation");
    const auto& subscriptionMetrics = metrics.getMetrics("Subscription");
    
    EXPECT_EQ(queryMetrics.total_queries.load(), 1);
    EXPECT_EQ(mutationMetrics.total_queries.load(), 1);
    EXPECT_EQ(subscriptionMetrics.total_queries.load(), 1);
    
    EXPECT_EQ(queryMetrics.total_execution_time_ms.load(), 100);
    EXPECT_EQ(mutationMetrics.total_execution_time_ms.load(), 200);
    EXPECT_EQ(subscriptionMetrics.total_execution_time_ms.load(), 50);
}

TEST(GraphQLMetrics, QueryTimerAutoRecords) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    {
        QueryTimer timer("Query", 3, 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timer.setSuccess(true);
    }  // Timer destructor records metrics
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    EXPECT_EQ(queryMetrics.total_queries.load(), 1);
    EXPECT_GE(queryMetrics.total_execution_time_ms.load(), 10);
}

TEST(GraphQLMetrics, ResetClearsAllMetrics) {
    Metrics& metrics = Metrics::instance();
    
    metrics.recordQuery("Query", 100, true, 3, 5);
    metrics.recordQuery("Mutation", 200, true, 2, 4);
    
    metrics.reset();
    
    const auto& queryMetrics = metrics.getMetrics("Query");
    const auto& mutationMetrics = metrics.getMetrics("Mutation");
    
    EXPECT_EQ(queryMetrics.total_queries.load(), 0);
    EXPECT_EQ(mutationMetrics.total_queries.load(), 0);
}

TEST(GraphQLMetrics, GetAllMetrics) {
    Metrics& metrics = Metrics::instance();
    metrics.reset();
    
    metrics.recordQuery("Query", 100, true, 3, 5);
    metrics.recordQuery("Mutation", 200, true, 2, 4);
    
    auto allMetrics = metrics.getAllMetrics();
    EXPECT_EQ(allMetrics.size(), 2);
    EXPECT_TRUE(allMetrics.find("Query") != allMetrics.end());
    EXPECT_TRUE(allMetrics.find("Mutation") != allMetrics.end());
}
