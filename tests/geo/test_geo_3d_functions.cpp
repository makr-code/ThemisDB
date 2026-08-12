#include <gtest/gtest.h>
#include "query/functions/geo_functions.h"
#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>

using namespace themis::query::functions;
using json = nlohmann::json;

class Geo3DFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& registry = FunctionRegistry::instance();
        registerGeoFunctions(registry);
    }
};

// Test: ST_POINT with 3D coordinates
TEST_F(Geo3DFunctionsTest, StPoint3D) {
    auto& registry = FunctionRegistry::instance();
    ASSERT_TRUE(registry.hasFunction("ST_POINT"));
    
    FunctionContext ctx;
    std::vector<json> args = {13.4, 52.5, 100.0};  // Berlin at 100m elevation
    
    auto result = registry.call("ST_POINT", args, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result["type"], "Point");
    EXPECT_TRUE(result["coordinates"].is_array());
    EXPECT_EQ(result["coordinates"].size(), 3);
    EXPECT_DOUBLE_EQ(result["coordinates"][0].template get<double>(), 13.4);
    EXPECT_DOUBLE_EQ(result["coordinates"][1].template get<double>(), 52.5);
    EXPECT_DOUBLE_EQ(result["coordinates"][2].template get<double>(), 100.0);
}

// Test: ST_POINT with 2D coordinates (z should default to 0)
TEST_F(Geo3DFunctionsTest, StPoint2D) {
    auto& registry = FunctionRegistry::instance();
    ASSERT_TRUE(registry.hasFunction("ST_POINT"));
    
    FunctionContext ctx;
    std::vector<json> args = {13.4, 52.5};  // 2D point
    
    auto result = registry.call("ST_POINT", args, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result["type"], "Point");
    EXPECT_EQ(result["coordinates"].size(), 2);
}

// Test: ST_Z extracts Z coordinate
TEST_F(Geo3DFunctionsTest, StZ) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_Z"));
    
    // 3D point
    json point3d = {
        {"type", "Point"},
        {"coordinates", {13.4, 52.5, 100.0}}
    };
    
    FunctionContext ctx;
    std::vector<json> args = {point3d};
    
    auto result = registry.call("ST_Z", args, ctx);
    EXPECT_DOUBLE_EQ(result.template get<double>(), 100.0);
    
    // 2D point should return null
    json point2d = {
        {"type", "Point"},
        {"coordinates", {13.4, 52.5}}
    };
    
    args = {point2d};
    result = registry.call("ST_Z", args, ctx);
    EXPECT_TRUE(result.is_null());
}

// Test: ST_HASZ checks for Z coordinate
TEST_F(Geo3DFunctionsTest, StHasZ) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_HASZ"));
    
    FunctionContext ctx;
    
    // 3D point
    json point3d = {
        {"type", "Point"},
        {"coordinates", {13.4, 52.5, 100.0}}
    };
    std::vector<json> args = {point3d};
    auto result = registry.call("ST_HASZ", args, ctx);
    EXPECT_TRUE(result.template get<bool>());
    
    // 2D point
    json point2d = {
        {"type", "Point"},
        {"coordinates", {13.4, 52.5}}
    };
    args = {point2d};
    result = registry.call("ST_HASZ", args, ctx);
    EXPECT_FALSE(result.template get<bool>());
    
    // 3D LineString
    json linestring3d = {
        {"type", "LineString"},
        {"coordinates", {{0.0, 0.0, 10.0}, {1.0, 1.0, 20.0}}}
    };
    args = {linestring3d};
    result = registry.call("ST_HASZ", args, ctx);
    EXPECT_TRUE(result.template get<bool>());
}

// Test: ST_DISTANCE with 3D points uses Euclidean 3D distance
TEST_F(Geo3DFunctionsTest, StDistance3D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_DISTANCE"));
    
    // Two 3D points
    json point1 = {
        {"type", "Point"},
        {"coordinates", {0.0, 0.0, 0.0}}
    };
    json point2 = {
        {"type", "Point"},
        {"coordinates", {1.0, 1.0, 1.0}}
    };
    
    FunctionContext ctx;
    std::vector<json> args = {point1, point2};
    
    auto result = registry.call("ST_DISTANCE", args, ctx);
    
    // Euclidean 3D distance: sqrt(1^2 + 1^2 + 1^2) = sqrt(3) ≈ 1.732
    double expected = std::sqrt(3.0);
    EXPECT_NEAR(result.template get<double>(), expected, 0.001);
}

// Test: ST_DISTANCE with 2D points falls back to 2D distance
TEST_F(Geo3DFunctionsTest, StDistance2D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_DISTANCE"));
    
    // Two 2D projected points (not lat/lon)
    json point1 = {
        {"type", "Point"},
        {"coordinates", {0.0, 0.0}}
    };
    json point2 = {
        {"type", "Point"},
        {"coordinates", {3.0, 4.0}}
    };
    
    FunctionContext ctx;
    std::vector<json> args = {point1, point2};
    
    auto result = registry.call("ST_DISTANCE", args, ctx);
    
    // ST_DISTANCE uses geodetic (Haversine) distance, not Euclidean
    // For points (0,0) and (3,4) the geodetic distance is approximately 555807 meters
    EXPECT_NEAR(result.template get<double>(), 555807.0, 1000.0);
}

// Test: ST_DWITHIN with 3D points
TEST_F(Geo3DFunctionsTest, StDWithin3D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_DWITHIN"));
    
    json point1 = {
        {"type", "Point"},
        {"coordinates", {0.0, 0.0, 0.0}}
    };
    json point2 = {
        {"type", "Point"},
        {"coordinates", {1.0, 1.0, 1.0}}
    };
    
    FunctionContext ctx;
    
    // Distance is sqrt(3) ≈ 1.732
    // Within 2.0 should be true
    std::vector<json> args = {point1, point2, 2.0};
    auto result = registry.call("ST_DWITHIN", args, ctx);
    EXPECT_TRUE(result.template get<bool>());
    
    // Within 1.0 should be false
    args = {point1, point2, 1.0};
    result = registry.call("ST_DWITHIN", args, ctx);
    EXPECT_FALSE(result.template get<bool>());
}

// Test: ST_CENTROID preserves Z coordinate
TEST_F(Geo3DFunctionsTest, StCentroid3D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_CENTROID"));
    
    // 3D LineString
    json linestring = {
        {"type", "LineString"},
        {"coordinates", {
            {0.0, 0.0, 0.0},
            {2.0, 2.0, 10.0},
            {4.0, 0.0, 20.0}
        }}
    };
    
    FunctionContext ctx;
    std::vector<json> args = {linestring};
    
    auto result = registry.call("ST_CENTROID", args, ctx);
    
    EXPECT_EQ(result["type"], "Point");
    EXPECT_TRUE(result["coordinates"].is_array());
    EXPECT_EQ(result["coordinates"].size(), 3);
    
    // Centroid should be average: (0+2+4)/3 = 2, (0+2+0)/3 = 0.666..., (0+10+20)/3 = 10
    EXPECT_NEAR(result["coordinates"][0].template get<double>(), 2.0, 0.001);
    EXPECT_NEAR(result["coordinates"][1].template get<double>(), 0.666666, 0.001);
    EXPECT_NEAR(result["coordinates"][2].template get<double>(), 10.0, 0.001);
}

// Test: ST_CENTROID with 2D geometry doesn't add Z
TEST_F(Geo3DFunctionsTest, StCentroid2D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_CENTROID"));
    
    // 2D LineString
    json linestring = {
        {"type", "LineString"},
        {"coordinates", {
            {0.0, 0.0},
            {2.0, 2.0},
            {4.0, 0.0}
        }}
    };
    
    FunctionContext ctx;
    std::vector<json> args = {linestring};
    
    auto result = registry.call("ST_CENTROID", args, ctx);
    
    EXPECT_EQ(result["type"], "Point");
    EXPECT_EQ(result["coordinates"].size(), 2);  // Should stay 2D
}

// Test: WKT parsing with 3D coordinates
TEST_F(Geo3DFunctionsTest, StGeomFromText3D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_GEOMFROMTEXT"));
    
    FunctionContext ctx;
    std::vector<json> args = {"POINT(13.4 52.5 100)"};
    
    auto result = registry.call("ST_GEOMFROMTEXT", args, ctx);
    
    EXPECT_EQ(result["type"], "Point");
    EXPECT_EQ(result["coordinates"].size(), 3);
    EXPECT_DOUBLE_EQ(result["coordinates"][0].template get<double>(), 13.4);
    EXPECT_DOUBLE_EQ(result["coordinates"][1].template get<double>(), 52.5);
    EXPECT_DOUBLE_EQ(result["coordinates"][2].template get<double>(), 100.0);
}

// Test: WKT output with 3D coordinates
TEST_F(Geo3DFunctionsTest, StAsText3D) {
    auto& registry = FunctionRegistry::instance();

    ASSERT_TRUE(registry.hasFunction("ST_ASTEXT"));
    
    json point3d = {
        {"type", "Point"},
        {"coordinates", {13.4, 52.5, 100.0}}
    };
    
    FunctionContext ctx;
    std::vector<json> args = {point3d};
    
    auto result = registry.call("ST_ASTEXT", args, ctx);
    
    std::string wkt = result.template get<std::string>();
    EXPECT_TRUE(wkt.find("POINT") != std::string::npos);
    EXPECT_TRUE(wkt.find("13.4") != std::string::npos);
    EXPECT_TRUE(wkt.find("52.5") != std::string::npos);
    EXPECT_TRUE(wkt.find("100") != std::string::npos);
}

// ============================================================================
// ST_BUFFER function registry tests
// ============================================================================

TEST_F(Geo3DFunctionsTest, StBuffer_IsRegistered) {
    auto& registry = FunctionRegistry::instance();
    EXPECT_TRUE(registry.hasFunction("ST_BUFFER"));
}

TEST_F(Geo3DFunctionsTest, StBuffer_PointProducesPolygon) {
    auto& registry = FunctionRegistry::instance();
    ASSERT_TRUE(registry.hasFunction("ST_BUFFER"));

    // Berlin (~52.52 N, 13.41 E), 500 m buffer
    json point = {{"type", "Point"}, {"coordinates", {13.405, 52.52}}};
    FunctionContext ctx;
    std::vector<json> args = {point, 500.0};

    json result = registry.call("ST_BUFFER", args, ctx);

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["type"].get<std::string>(), "Polygon");
    ASSERT_TRUE(result["coordinates"].is_array());
    ASSERT_FALSE(result["coordinates"].empty());
    // Default arc_points is 36; ring has 36 unique + 1 closing vertex = 37.
    EXPECT_EQ(result["coordinates"][0].size(), 37u);
}

TEST_F(Geo3DFunctionsTest, StBuffer_ArcPointsParameter) {
    auto& registry = FunctionRegistry::instance();
    ASSERT_TRUE(registry.hasFunction("ST_BUFFER"));

    json point = {{"type", "Point"}, {"coordinates", {0.0, 0.0}}};
    FunctionContext ctx;
    std::vector<json> args = {point, 100.0, 16.0};

    json result = registry.call("ST_BUFFER", args, ctx);

    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["type"].get<std::string>(), "Polygon");
    // arc_points=16 → ring has 16 unique + 1 closing vertex = 17.
    EXPECT_EQ(result["coordinates"][0].size(), 17u);
}

TEST_F(Geo3DFunctionsTest, StBuffer_NegativeDistance_ReturnsEmptyCollection) {
    auto& registry = FunctionRegistry::instance();
    ASSERT_TRUE(registry.hasFunction("ST_BUFFER"));

    json point = {{"type", "Point"}, {"coordinates", {0.0, 0.0}}};
    FunctionContext ctx;
    std::vector<json> args = {point, -100.0};

    json result = registry.call("ST_BUFFER", args, ctx);

    // Non-positive distance must return an empty GeometryCollection.
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result["type"].get<std::string>(), "GeometryCollection");
}
