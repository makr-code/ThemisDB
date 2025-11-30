/**
 * @file test_aql_functions.cpp
 * @brief Tests for the modular AQL Function Registry
 */

#include <gtest/gtest.h>
#include "query/functions/function_registry.h"
#include "query/functions/string_functions.h"
#include "query/functions/math_functions.h"
#include "query/functions/array_functions.h"
#include "query/functions/date_functions.h"
#include "query/functions/document_functions.h"
#include "query/functions/geo_functions.h"
#include "query/functions/vector_functions.h"
#include "query/functions/graph_functions.h"

using namespace themis::query::functions;

class AQLFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register all functions
        registerBuiltinFunctions();
    }
    
    FunctionContext ctx;
};

// ============================================================================
// String Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, LengthFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("LENGTH", {"hello"}, ctx), 5);
    EXPECT_EQ(reg.call("LENGTH", {nlohmann::json::array({1, 2, 3})}, ctx), 3);
    EXPECT_EQ(reg.call("LENGTH", {nlohmann::json::object({{"a", 1}, {"b", 2}})}, ctx), 2);
}

TEST_F(AQLFunctionsTest, ConcatFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("CONCAT", {"Hello", " ", "World"}, ctx), "Hello World");
    EXPECT_EQ(reg.call("CONCAT", {"Value: ", 123}, ctx), "Value: 123");
}

TEST_F(AQLFunctionsTest, SubstringFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("SUBSTRING", {"Hello World", 0, 5}, ctx), "Hello");
    EXPECT_EQ(reg.call("SUBSTRING", {"Hello World", 6}, ctx), "World");
}

TEST_F(AQLFunctionsTest, UpperLowerFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("UPPER", {"hello"}, ctx), "HELLO");
    EXPECT_EQ(reg.call("LOWER", {"HELLO"}, ctx), "hello");
}

TEST_F(AQLFunctionsTest, TrimFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("TRIM", {"  hello  "}, ctx), "hello");
    EXPECT_EQ(reg.call("LTRIM", {"  hello"}, ctx), "hello");
    EXPECT_EQ(reg.call("RTRIM", {"hello  "}, ctx), "hello");
}

TEST_F(AQLFunctionsTest, SplitFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("SPLIT", {"a,b,c", ","}, ctx);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

TEST_F(AQLFunctionsTest, ContainsFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_TRUE(reg.call("CONTAINS", {"Hello World", "World"}, ctx));
    EXPECT_FALSE(reg.call("CONTAINS", {"Hello World", "world"}, ctx));
}

TEST_F(AQLFunctionsTest, StartsEndsWithFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_TRUE(reg.call("STARTS_WITH", {"Hello World", "Hello"}, ctx));
    EXPECT_FALSE(reg.call("STARTS_WITH", {"Hello World", "World"}, ctx));
    EXPECT_TRUE(reg.call("ENDS_WITH", {"Hello World", "World"}, ctx));
    EXPECT_FALSE(reg.call("ENDS_WITH", {"Hello World", "Hello"}, ctx));
}

TEST_F(AQLFunctionsTest, ReplaceFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("REPLACE", {"Hello World", "World", "ThemisDB"}, ctx), "Hello ThemisDB");
}

TEST_F(AQLFunctionsTest, RegexTestFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_TRUE(reg.call("REGEX_TEST", {"hello123", "\\d+"}, ctx));
    EXPECT_FALSE(reg.call("REGEX_TEST", {"hello", "\\d+"}, ctx));
}

TEST_F(AQLFunctionsTest, LevenshteinDistanceFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("LEVENSHTEIN_DISTANCE", {"hello", "hallo"}, ctx), 1);
    EXPECT_EQ(reg.call("LEVENSHTEIN_DISTANCE", {"hello", "hello"}, ctx), 0);
}

// ============================================================================
// Math Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, AbsFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("ABS", {-5}, ctx), 5.0);
    EXPECT_EQ(reg.call("ABS", {5}, ctx), 5.0);
}

TEST_F(AQLFunctionsTest, CeilFloorRoundFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("CEIL", {4.3}, ctx), 5);
    EXPECT_EQ(reg.call("FLOOR", {4.7}, ctx), 4);
    EXPECT_EQ(reg.call("ROUND", {4.5}, ctx), 5.0);
    EXPECT_EQ(reg.call("ROUND", {4.567, 2}, ctx), 4.57);
}

TEST_F(AQLFunctionsTest, SqrtPowFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("SQRT", {16}, ctx), 4.0);
    EXPECT_EQ(reg.call("POW", {2, 3}, ctx), 8.0);
}

TEST_F(AQLFunctionsTest, LogFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("LOG10", {100}, ctx), 2.0);
}

TEST_F(AQLFunctionsTest, TrigFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_NEAR(reg.call("SIN", {0}, ctx).get<double>(), 0.0, 0.0001);
    EXPECT_NEAR(reg.call("COS", {0}, ctx).get<double>(), 1.0, 0.0001);
}

TEST_F(AQLFunctionsTest, PiFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_NEAR(reg.call("PI", {}, ctx).get<double>(), 3.14159, 0.0001);
}

TEST_F(AQLFunctionsTest, MinMaxFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("MIN", {3, 1, 2}, ctx), 1.0);
    EXPECT_EQ(reg.call("MAX", {3, 1, 2}, ctx), 3.0);
    EXPECT_EQ(reg.call("MIN", {nlohmann::json::array({5, 2, 8})}, ctx), 2.0);
}

TEST_F(AQLFunctionsTest, SumAvgFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("SUM", {nlohmann::json::array({1, 2, 3, 4})}, ctx), 10.0);
    EXPECT_EQ(reg.call("AVG", {nlohmann::json::array({1, 2, 3, 4})}, ctx), 2.5);
}

// ============================================================================
// Array Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, FirstLastFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("FIRST", {nlohmann::json::array({1, 2, 3})}, ctx), 1);
    EXPECT_EQ(reg.call("LAST", {nlohmann::json::array({1, 2, 3})}, ctx), 3);
}

TEST_F(AQLFunctionsTest, NthFunction) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("NTH", {nlohmann::json::array({1, 2, 3}), 1}, ctx), 2);
    EXPECT_EQ(reg.call("NTH", {nlohmann::json::array({1, 2, 3}), -1}, ctx), 3);
}

TEST_F(AQLFunctionsTest, PushPopFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto pushed = reg.call("PUSH", {nlohmann::json::array({1, 2}), 3}, ctx);
    EXPECT_EQ(pushed.size(), 3);
    EXPECT_EQ(pushed[2], 3);
    
    auto popped = reg.call("POP", {nlohmann::json::array({1, 2, 3})}, ctx);
    EXPECT_EQ(popped.size(), 2);
}

TEST_F(AQLFunctionsTest, SliceFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("SLICE", {nlohmann::json::array({1, 2, 3, 4}), 1, 3}, ctx);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 3);
}

TEST_F(AQLFunctionsTest, FlattenFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto nested = nlohmann::json::array({
        nlohmann::json::array({1, 2}),
        nlohmann::json::array({3, 4})
    });
    auto result = reg.call("FLATTEN", {nested}, ctx);
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result, nlohmann::json::array({1, 2, 3, 4}));
}

TEST_F(AQLFunctionsTest, UniqueFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("UNIQUE", {nlohmann::json::array({1, 2, 1, 3, 2})}, ctx);
    EXPECT_EQ(result.size(), 3);
}

TEST_F(AQLFunctionsTest, SortedFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("SORTED", {nlohmann::json::array({3, 1, 2})}, ctx);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 3);
}

TEST_F(AQLFunctionsTest, UnionIntersectionFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto u = reg.call("UNION", {
        nlohmann::json::array({1, 2}),
        nlohmann::json::array({2, 3})
    }, ctx);
    EXPECT_EQ(u.size(), 3);
    
    auto i = reg.call("INTERSECTION", {
        nlohmann::json::array({1, 2, 3}),
        nlohmann::json::array({2, 3, 4})
    }, ctx);
    EXPECT_EQ(i.size(), 2);
}

TEST_F(AQLFunctionsTest, RangeFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("RANGE", {1, 5}, ctx);
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result, nlohmann::json::array({1, 2, 3, 4, 5}));
    
    auto stepped = reg.call("RANGE", {0, 10, 2}, ctx);
    EXPECT_EQ(stepped, nlohmann::json::array({0, 2, 4, 6, 8, 10}));
}

// ============================================================================
// Date Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, DateNowFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto now = reg.call("DATE_NOW", {}, ctx);
    EXPECT_TRUE(now.is_number_integer());
    EXPECT_GT(now.get<int64_t>(), 0);
}

TEST_F(AQLFunctionsTest, DateComponentFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    // 2023-11-14 22:13:20 UTC
    int64_t ts = 1700000000000;
    
    EXPECT_EQ(reg.call("DATE_YEAR", {ts}, ctx), 2023);
    EXPECT_EQ(reg.call("DATE_MONTH", {ts}, ctx), 11);
    EXPECT_EQ(reg.call("DATE_DAY", {ts}, ctx), 14);
}

TEST_F(AQLFunctionsTest, DateAddSubtractFunction) {
    auto& reg = FunctionRegistry::instance();
    
    int64_t ts = 1700000000000;
    auto added = reg.call("DATE_ADD", {ts, 1, "day"}, ctx);
    EXPECT_EQ(added.get<int64_t>(), ts + 24 * 60 * 60 * 1000);
    
    auto subtracted = reg.call("DATE_SUBTRACT", {ts, 1, "hour"}, ctx);
    EXPECT_EQ(subtracted.get<int64_t>(), ts - 60 * 60 * 1000);
}

TEST_F(AQLFunctionsTest, DateFormatFunction) {
    auto& reg = FunctionRegistry::instance();
    
    int64_t ts = 1700000000000;
    auto formatted = reg.call("DATE_FORMAT", {ts, "%Y-%m-%d"}, ctx);
    EXPECT_EQ(formatted, "2023-11-14");
}

// ============================================================================
// Document Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, MergeFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("MERGE", {
        nlohmann::json::object({{"a", 1}}),
        nlohmann::json::object({{"b", 2}})
    }, ctx);
    EXPECT_EQ(result["a"], 1);
    EXPECT_EQ(result["b"], 2);
}

TEST_F(AQLFunctionsTest, UnsetKeepFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto obj = nlohmann::json::object({{"a", 1}, {"b", 2}, {"c", 3}});
    
    auto unset = reg.call("UNSET", {obj, nlohmann::json::array({"b"})}, ctx);
    EXPECT_FALSE(unset.contains("b"));
    EXPECT_TRUE(unset.contains("a"));
    
    auto kept = reg.call("KEEP", {obj, nlohmann::json::array({"a", "c"})}, ctx);
    EXPECT_TRUE(kept.contains("a"));
    EXPECT_TRUE(kept.contains("c"));
    EXPECT_FALSE(kept.contains("b"));
}

TEST_F(AQLFunctionsTest, HasFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto obj = nlohmann::json::object({{"a", 1}});
    EXPECT_TRUE(reg.call("HAS", {obj, "a"}, ctx));
    EXPECT_FALSE(reg.call("HAS", {obj, "b"}, ctx));
}

TEST_F(AQLFunctionsTest, AttributesValuesFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto obj = nlohmann::json::object({{"a", 1}, {"b", 2}});
    
    auto attrs = reg.call("ATTRIBUTES", {obj}, ctx);
    EXPECT_EQ(attrs.size(), 2);
    
    auto vals = reg.call("VALUES", {obj}, ctx);
    EXPECT_EQ(vals.size(), 2);
}

TEST_F(AQLFunctionsTest, ZipUnzipFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto zipped = reg.call("ZIP", {
        nlohmann::json::array({"a", "b"}),
        nlohmann::json::array({1, 2})
    }, ctx);
    EXPECT_EQ(zipped["a"], 1);
    EXPECT_EQ(zipped["b"], 2);
    
    auto unzipped = reg.call("UNZIP", {zipped}, ctx);
    EXPECT_EQ(unzipped["keys"].size(), 2);
    EXPECT_EQ(unzipped["values"].size(), 2);
}

TEST_F(AQLFunctionsTest, TypeFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("TYPENAME", {"hello"}, ctx), "string");
    EXPECT_EQ(reg.call("TYPENAME", {123}, ctx), "int");
    EXPECT_EQ(reg.call("TYPENAME", {nlohmann::json::array()}, ctx), "array");
    
    EXPECT_TRUE(reg.call("IS_STRING", {"hello"}, ctx));
    EXPECT_TRUE(reg.call("IS_NUMBER", {123}, ctx));
    EXPECT_TRUE(reg.call("IS_ARRAY", {nlohmann::json::array()}, ctx));
    EXPECT_TRUE(reg.call("IS_OBJECT", {nlohmann::json::object()}, ctx));
    EXPECT_TRUE(reg.call("IS_NULL", {nullptr}, ctx));
}

TEST_F(AQLFunctionsTest, ConversionFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_EQ(reg.call("TO_NUMBER", {"123"}, ctx), 123.0);
    EXPECT_EQ(reg.call("TO_STRING", {123}, ctx), "123");
    EXPECT_EQ(reg.call("TO_BOOL", {1}, ctx), true);
    EXPECT_EQ(reg.call("TO_ARRAY", {"hello"}, ctx), nlohmann::json::array({"hello"}));
}

// ============================================================================
// Registry Tests
// ============================================================================

TEST_F(AQLFunctionsTest, FunctionExists) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_TRUE(reg.hasFunction("LENGTH"));
    EXPECT_TRUE(reg.hasFunction("UPPER"));
    EXPECT_TRUE(reg.hasFunction("ABS"));
    EXPECT_FALSE(reg.hasFunction("NONEXISTENT"));
}

TEST_F(AQLFunctionsTest, GetCategories) {
    auto& reg = FunctionRegistry::instance();
    
    auto cats = reg.getCategories();
    EXPECT_FALSE(cats.empty());
    
    // Check that we have the expected categories
    bool hasString = false, hasMath = false, hasArray = false;
    for (const auto& cat : cats) {
        if (cat == "String") hasString = true;
        if (cat == "Math") hasMath = true;
        if (cat == "Array") hasArray = true;
    }
    EXPECT_TRUE(hasString);
    EXPECT_TRUE(hasMath);
    EXPECT_TRUE(hasArray);
}

TEST_F(AQLFunctionsTest, GetFunctionsByCategory) {
    auto& reg = FunctionRegistry::instance();
    
    auto stringFuncs = reg.getByCategory("String");
    EXPECT_FALSE(stringFuncs.empty());
    
    // Check that UPPER is in the String category
    bool hasUpper = false;
    for (const auto& sig : stringFuncs) {
        if (sig.name == "UPPER") hasUpper = true;
    }
    EXPECT_TRUE(hasUpper);
}

TEST_F(AQLFunctionsTest, UnknownFunctionThrows) {
    auto& reg = FunctionRegistry::instance();
    
    EXPECT_THROW(reg.call("NONEXISTENT", {}, ctx), std::runtime_error);
}

TEST_F(AQLFunctionsTest, InvalidArgsThrows) {
    auto& reg = FunctionRegistry::instance();
    
    // SUBSTRING requires 2 arguments
    EXPECT_THROW(reg.call("SUBSTRING", {"hello"}, ctx), std::runtime_error);
}

// ============================================================================
// Geo Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, StPointFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto point = reg.call("ST_POINT", {13.4, 52.5}, ctx);
    EXPECT_EQ(point["type"], "Point");
    EXPECT_EQ(point["coordinates"][0], 13.4);
    EXPECT_EQ(point["coordinates"][1], 52.5);
    
    // With Z coordinate
    auto point3d = reg.call("ST_POINT", {13.4, 52.5, 100.0}, ctx);
    EXPECT_EQ(point3d["coordinates"].size(), 3);
    EXPECT_EQ(point3d["coordinates"][2], 100.0);
}

TEST_F(AQLFunctionsTest, StDistanceFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Create two points
    nlohmann::json p1 = {{"type", "Point"}, {"coordinates", {0.0, 0.0}}};
    nlohmann::json p2 = {{"type", "Point"}, {"coordinates", {3.0, 4.0}}};
    
    // Euclidean distance
    auto distance = reg.call("ST_DISTANCE", {p1, p2}, ctx).get<double>();
    EXPECT_NEAR(distance, 5.0, 0.001);
}

TEST_F(AQLFunctionsTest, GeoDistanceFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Berlin and Munich (approximate coordinates)
    nlohmann::json berlin = {{"type", "Point"}, {"coordinates", {13.4, 52.5}}};
    nlohmann::json munich = {{"type", "Point"}, {"coordinates", {11.6, 48.1}}};
    
    // Should return distance in meters (haversine)
    auto distance = reg.call("GEO_DISTANCE", {berlin, munich}, ctx).get<double>();
    // Berlin to Munich is approximately 500 km
    EXPECT_GT(distance, 400000); // > 400 km
    EXPECT_LT(distance, 600000); // < 600 km
}

TEST_F(AQLFunctionsTest, StContainsFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Create a polygon and a point inside it
    nlohmann::json polygon = {
        {"type", "Polygon"},
        {"coordinates", {{{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}, {0.0, 0.0}}}}
    };
    nlohmann::json pointInside = {{"type", "Point"}, {"coordinates", {5.0, 5.0}}};
    nlohmann::json pointOutside = {{"type", "Point"}, {"coordinates", {15.0, 15.0}}};
    
    EXPECT_TRUE(reg.call("ST_CONTAINS", {polygon, pointInside}, ctx).get<bool>());
    EXPECT_FALSE(reg.call("ST_CONTAINS", {polygon, pointOutside}, ctx).get<bool>());
}

TEST_F(AQLFunctionsTest, StGeomFromTextFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto point = reg.call("ST_GEOMFROMTEXT", {"POINT(1 2)"}, ctx);
    EXPECT_EQ(point["type"], "Point");
    EXPECT_EQ(point["coordinates"][0], 1.0);
    EXPECT_EQ(point["coordinates"][1], 2.0);
    
    auto line = reg.call("ST_GEOMFROMTEXT", {"LINESTRING(0 0, 1 1, 2 0)"}, ctx);
    EXPECT_EQ(line["type"], "LineString");
    EXPECT_EQ(line["coordinates"].size(), 3);
}

TEST_F(AQLFunctionsTest, StAsTextFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json point = {{"type", "Point"}, {"coordinates", {1.0, 2.0}}};
    auto wkt = reg.call("ST_ASTEXT", {point}, ctx).get<std::string>();
    EXPECT_EQ(wkt, "POINT(1 2)");
}

TEST_F(AQLFunctionsTest, StHasZFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json point2d = {{"type", "Point"}, {"coordinates", {1.0, 2.0}}};
    nlohmann::json point3d = {{"type", "Point"}, {"coordinates", {1.0, 2.0, 3.0}}};
    
    EXPECT_FALSE(reg.call("ST_HASZ", {point2d}, ctx).get<bool>());
    EXPECT_TRUE(reg.call("ST_HASZ", {point3d}, ctx).get<bool>());
}

TEST_F(AQLFunctionsTest, StCentroidFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json polygon = {
        {"type", "Polygon"},
        {"coordinates", {{{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}, {0.0, 0.0}}}}
    };
    
    auto centroid = reg.call("ST_CENTROID", {polygon}, ctx);
    EXPECT_EQ(centroid["type"], "Point");
    // Centroid of square [0,4]x[0,4] is approximately (2,2)
    EXPECT_NEAR(centroid["coordinates"][0].get<double>(), 1.6, 0.5);
    EXPECT_NEAR(centroid["coordinates"][1].get<double>(), 1.6, 0.5);
}

// ============================================================================
// Vector Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, CosineSimilarityFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Identical vectors should have similarity 1.0
    auto identical = reg.call("COSINE_SIMILARITY", {
        nlohmann::json::array({1.0, 0.0, 0.0}),
        nlohmann::json::array({1.0, 0.0, 0.0})
    }, ctx).get<double>();
    EXPECT_NEAR(identical, 1.0, 0.001);
    
    // Orthogonal vectors should have similarity 0.0
    auto orthogonal = reg.call("COSINE_SIMILARITY", {
        nlohmann::json::array({1.0, 0.0}),
        nlohmann::json::array({0.0, 1.0})
    }, ctx).get<double>();
    EXPECT_NEAR(orthogonal, 0.0, 0.001);
    
    // Opposite vectors should have similarity -1.0
    auto opposite = reg.call("COSINE_SIMILARITY", {
        nlohmann::json::array({1.0, 0.0}),
        nlohmann::json::array({-1.0, 0.0})
    }, ctx).get<double>();
    EXPECT_NEAR(opposite, -1.0, 0.001);
}

TEST_F(AQLFunctionsTest, EuclideanDistanceFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto dist = reg.call("EUCLIDEAN_DISTANCE", {
        nlohmann::json::array({0.0, 0.0}),
        nlohmann::json::array({3.0, 4.0})
    }, ctx).get<double>();
    EXPECT_NEAR(dist, 5.0, 0.001);
}

TEST_F(AQLFunctionsTest, DotProductFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto dot = reg.call("DOT_PRODUCT", {
        nlohmann::json::array({1.0, 2.0, 3.0}),
        nlohmann::json::array({4.0, 5.0, 6.0})
    }, ctx).get<double>();
    EXPECT_NEAR(dot, 32.0, 0.001); // 1*4 + 2*5 + 3*6 = 32
}

TEST_F(AQLFunctionsTest, L2NormalizeFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto normalized = reg.call("L2_NORMALIZE", {
        nlohmann::json::array({3.0, 4.0})
    }, ctx);
    EXPECT_NEAR(normalized[0].get<double>(), 0.6, 0.001);
    EXPECT_NEAR(normalized[1].get<double>(), 0.8, 0.001);
}

TEST_F(AQLFunctionsTest, VectorArithmeticFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    auto v1 = nlohmann::json::array({1.0, 2.0, 3.0});
    auto v2 = nlohmann::json::array({4.0, 5.0, 6.0});
    
    auto add = reg.call("VECTOR_ADD", {v1, v2}, ctx);
    EXPECT_EQ(add, nlohmann::json::array({5.0, 7.0, 9.0}));
    
    auto sub = reg.call("VECTOR_SUB", {v2, v1}, ctx);
    EXPECT_EQ(sub, nlohmann::json::array({3.0, 3.0, 3.0}));
    
    auto scale = reg.call("VECTOR_SCALE", {v1, 2.0}, ctx);
    EXPECT_EQ(scale, nlohmann::json::array({2.0, 4.0, 6.0}));
}

TEST_F(AQLFunctionsTest, VectorAggregationFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    auto vec = nlohmann::json::array({1.0, 2.0, 3.0, 4.0});
    
    EXPECT_NEAR(reg.call("VECTOR_SUM", {vec}, ctx).get<double>(), 10.0, 0.001);
    EXPECT_NEAR(reg.call("VECTOR_AVG", {vec}, ctx).get<double>(), 2.5, 0.001);
    EXPECT_NEAR(reg.call("VECTOR_MIN", {vec}, ctx).get<double>(), 1.0, 0.001);
    EXPECT_NEAR(reg.call("VECTOR_MAX", {vec}, ctx).get<double>(), 4.0, 0.001);
    EXPECT_EQ(reg.call("VECTOR_DIM", {vec}, ctx).get<int>(), 4);
}

TEST_F(AQLFunctionsTest, VectorUtilityFunctions) {
    auto& reg = FunctionRegistry::instance();
    
    auto zeros = reg.call("VECTOR_ZEROS", {3}, ctx);
    EXPECT_EQ(zeros, nlohmann::json::array({0.0, 0.0, 0.0}));
    
    auto ones = reg.call("VECTOR_ONES", {3}, ctx);
    EXPECT_EQ(ones, nlohmann::json::array({1.0, 1.0, 1.0}));
    
    auto slice = reg.call("VECTOR_SLICE", {
        nlohmann::json::array({1.0, 2.0, 3.0, 4.0, 5.0}),
        1, 4
    }, ctx);
    EXPECT_EQ(slice, nlohmann::json::array({2.0, 3.0, 4.0}));
    
    auto concat = reg.call("VECTOR_CONCAT", {
        nlohmann::json::array({1.0, 2.0}),
        nlohmann::json::array({3.0, 4.0})
    }, ctx);
    EXPECT_EQ(concat, nlohmann::json::array({1.0, 2.0, 3.0, 4.0}));
}

// ============================================================================
// Graph Function Tests
// ============================================================================

TEST_F(AQLFunctionsTest, IsEdgeFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edge = {{"_from", "users/1"}, {"_to", "users/2"}, {"type", "follows"}};
    nlohmann::json vertex = {{"_id", "users/1"}, {"name", "Alice"}};
    
    EXPECT_TRUE(reg.call("IS_EDGE", {edge}, ctx).get<bool>());
    EXPECT_FALSE(reg.call("IS_EDGE", {vertex}, ctx).get<bool>());
}

TEST_F(AQLFunctionsTest, IsVertexFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edge = {{"_from", "users/1"}, {"_to", "users/2"}};
    nlohmann::json vertex = {{"_id", "users/1"}, {"name", "Alice"}};
    
    EXPECT_FALSE(reg.call("IS_VERTEX", {edge}, ctx).get<bool>());
    EXPECT_TRUE(reg.call("IS_VERTEX", {vertex}, ctx).get<bool>());
}

TEST_F(AQLFunctionsTest, ParseIdentifierFunction) {
    auto& reg = FunctionRegistry::instance();
    
    auto result = reg.call("PARSE_IDENTIFIER", {"users/123"}, ctx);
    EXPECT_EQ(result["collection"], "users");
    EXPECT_EQ(result["key"], "123");
}

TEST_F(AQLFunctionsTest, GraphDegreeFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "users/1"}, {"_to", "users/2"}},
        {{"_from", "users/1"}, {"_to", "users/3"}},
        {{"_from", "users/2"}, {"_to", "users/1"}}
    });
    
    EXPECT_EQ(reg.call("GRAPH_DEGREE", {"users/1", edges, "outbound"}, ctx).get<int>(), 2);
    EXPECT_EQ(reg.call("GRAPH_DEGREE", {"users/1", edges, "inbound"}, ctx).get<int>(), 1);
    EXPECT_EQ(reg.call("GRAPH_DEGREE", {"users/1", edges, "any"}, ctx).get<int>(), 3);
}

TEST_F(AQLFunctionsTest, GraphNeighborsFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "users/1"}, {"_to", "users/2"}},
        {{"_from", "users/2"}, {"_to", "users/3"}}
    });
    
    auto neighbors = reg.call("GRAPH_NEIGHBORS", {"users/1", edges, "outbound", 1}, ctx);
    EXPECT_EQ(neighbors.size(), 1);
    EXPECT_EQ(neighbors[0], "users/2");
    
    // Depth 2 should also include users/3
    auto deep = reg.call("GRAPH_NEIGHBORS", {"users/1", edges, "outbound", 2}, ctx);
    EXPECT_EQ(deep.size(), 2);
}

TEST_F(AQLFunctionsTest, ShortestPathFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "a/1"}, {"_to", "a/2"}},
        {{"_from", "a/2"}, {"_to", "a/3"}},
        {{"_from", "a/3"}, {"_to", "a/4"}}
    });
    
    auto path = reg.call("SHORTEST_PATH", {"a/1", "a/4", edges, "outbound"}, ctx);
    EXPECT_EQ(path["distance"].get<int>(), 3);
    EXPECT_EQ(path["vertices"].size(), 4);
    
    // No path exists in the opposite direction
    auto noPath = reg.call("SHORTEST_PATH", {"a/4", "a/1", edges, "outbound"}, ctx);
    EXPECT_EQ(noPath["distance"].get<int>(), -1);
}

TEST_F(AQLFunctionsTest, GraphConnectedFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "a/1"}, {"_to", "a/2"}},
        {{"_from", "a/2"}, {"_to", "a/3"}}
    });
    
    EXPECT_TRUE(reg.call("GRAPH_CONNECTED", {"a/1", "a/3", edges, "any"}, ctx).get<bool>());
    EXPECT_TRUE(reg.call("GRAPH_CONNECTED", {"a/1", "a/2", edges, "outbound"}, ctx).get<bool>());
}

TEST_F(AQLFunctionsTest, PageRankFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "a/1"}, {"_to", "a/2"}},
        {{"_from", "a/2"}, {"_to", "a/3"}},
        {{"_from", "a/3"}, {"_to", "a/1"}}
    });
    
    auto ranks = reg.call("PAGERANK", {edges}, ctx);
    // All vertices should have positive rank
    EXPECT_GT(ranks["a/1"].get<double>(), 0.0);
    EXPECT_GT(ranks["a/2"].get<double>(), 0.0);
    EXPECT_GT(ranks["a/3"].get<double>(), 0.0);
}

TEST_F(AQLFunctionsTest, ConnectedComponentsFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Two disconnected components
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "a/1"}, {"_to", "a/2"}},
        {{"_from", "b/1"}, {"_to", "b/2"}}
    });
    
    auto components = reg.call("CONNECTED_COMPONENTS", {edges}, ctx);
    EXPECT_EQ(components.size(), 2);
}

TEST_F(AQLFunctionsTest, EdgesFunction) {
    auto& reg = FunctionRegistry::instance();
    
    nlohmann::json edges = nlohmann::json::array({
        {{"_from", "users/1"}, {"_to", "users/2"}, {"type", "follows"}},
        {{"_from", "users/2"}, {"_to", "users/1"}, {"type", "follows"}},
        {{"_from", "users/3"}, {"_to", "users/1"}, {"type", "likes"}}
    });
    
    auto outEdges = reg.call("EDGES", {"users/1", edges, "outbound"}, ctx);
    EXPECT_EQ(outEdges.size(), 1);
    
    auto inEdges = reg.call("EDGES", {"users/1", edges, "inbound"}, ctx);
    EXPECT_EQ(inEdges.size(), 2);
    
    auto anyEdges = reg.call("EDGES", {"users/1", edges, "any"}, ctx);
    EXPECT_EQ(anyEdges.size(), 3);
}

// ============================================================================
// Category Tests for New Functions
// ============================================================================

TEST_F(AQLFunctionsTest, GeoFunctionsRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Check that Geo functions are registered
    EXPECT_TRUE(reg.hasFunction("ST_POINT"));
    EXPECT_TRUE(reg.hasFunction("ST_DISTANCE"));
    EXPECT_TRUE(reg.hasFunction("GEO_DISTANCE"));
    EXPECT_TRUE(reg.hasFunction("ST_CONTAINS"));
    EXPECT_TRUE(reg.hasFunction("ST_WITHIN"));
    EXPECT_TRUE(reg.hasFunction("ST_GEOMFROMTEXT"));
    EXPECT_TRUE(reg.hasFunction("ST_ASGEOJSON"));
    EXPECT_TRUE(reg.hasFunction("ST_CENTROID"));
}

TEST_F(AQLFunctionsTest, VectorFunctionsRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Check that Vector functions are registered
    EXPECT_TRUE(reg.hasFunction("COSINE_SIMILARITY"));
    EXPECT_TRUE(reg.hasFunction("EUCLIDEAN_DISTANCE"));
    EXPECT_TRUE(reg.hasFunction("DOT_PRODUCT"));
    EXPECT_TRUE(reg.hasFunction("L2_NORMALIZE"));
    EXPECT_TRUE(reg.hasFunction("VECTOR_ADD"));
    EXPECT_TRUE(reg.hasFunction("VECTOR_SCALE"));
    EXPECT_TRUE(reg.hasFunction("SIMILARITY"));
}

TEST_F(AQLFunctionsTest, GraphFunctionsRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Check that Graph functions are registered
    EXPECT_TRUE(reg.hasFunction("IS_EDGE"));
    EXPECT_TRUE(reg.hasFunction("IS_VERTEX"));
    EXPECT_TRUE(reg.hasFunction("GRAPH_DEGREE"));
    EXPECT_TRUE(reg.hasFunction("GRAPH_NEIGHBORS"));
    EXPECT_TRUE(reg.hasFunction("SHORTEST_PATH"));
    EXPECT_TRUE(reg.hasFunction("PAGERANK"));
    EXPECT_TRUE(reg.hasFunction("CONNECTED_COMPONENTS"));
}

TEST_F(AQLFunctionsTest, AllCategoriesPresent) {
    auto& reg = FunctionRegistry::instance();
    
    auto cats = reg.getCategories();
    
    std::unordered_set<std::string> catSet(cats.begin(), cats.end());
    
    // All 8 categories should be present
    EXPECT_TRUE(catSet.count("String") > 0);
    EXPECT_TRUE(catSet.count("Math") > 0);
    EXPECT_TRUE(catSet.count("Array") > 0);
    EXPECT_TRUE(catSet.count("Date") > 0);
    EXPECT_TRUE(catSet.count("Document") > 0);
    EXPECT_TRUE(catSet.count("Geo") > 0);
    EXPECT_TRUE(catSet.count("Vector") > 0);
    EXPECT_TRUE(catSet.count("Graph") > 0);
}
