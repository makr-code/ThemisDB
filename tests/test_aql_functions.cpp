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
    
    // LENGTH requires 1 argument
    EXPECT_THROW(reg.call("SUBSTRING", {"hello"}, ctx), std::runtime_error);
}
