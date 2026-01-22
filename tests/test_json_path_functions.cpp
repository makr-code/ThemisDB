/**
 * @file test_json_path_functions.cpp
 * @brief Tests for JSON path query functions
 */

#include <gtest/gtest.h>
#include "query/functions/json_path_functions.h"
#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>

using namespace themis::query::functions;
using json = nlohmann::json;

class JsonPathFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize function registry with JSON path functions
        auto& registry = FunctionRegistry::instance();
        registerJsonPathFunctions(registry);
    }
    
    FunctionContext context;
};

// ============================================================================
// JSONPath Parser Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, ParseSimpleField) {
    auto segments = JSONPath::parse("$.field");
    ASSERT_EQ(segments.size(), 1);
    EXPECT_EQ(segments[0].type, JSONPath::SegmentType::FIELD);
    EXPECT_EQ(segments[0].field, "field");
}

TEST_F(JsonPathFunctionsTest, ParseNestedFields) {
    auto segments = JSONPath::parse("$.a.b.c");
    ASSERT_EQ(segments.size(), 3);
    EXPECT_EQ(segments[0].field, "a");
    EXPECT_EQ(segments[1].field, "b");
    EXPECT_EQ(segments[2].field, "c");
}

TEST_F(JsonPathFunctionsTest, ParseArrayIndex) {
    auto segments = JSONPath::parse("$.arr[0]");
    ASSERT_EQ(segments.size(), 2);
    EXPECT_EQ(segments[0].type, JSONPath::SegmentType::FIELD);
    EXPECT_EQ(segments[0].field, "arr");
    EXPECT_EQ(segments[1].type, JSONPath::SegmentType::INDEX);
    EXPECT_EQ(segments[1].index, 0);
}

TEST_F(JsonPathFunctionsTest, ParseMixedPath) {
    auto segments = JSONPath::parse("$.data[0].nested.value");
    ASSERT_EQ(segments.size(), 4);
    EXPECT_EQ(segments[0].field, "data");
    EXPECT_EQ(segments[1].index, 0);
    EXPECT_EQ(segments[2].field, "nested");
    EXPECT_EQ(segments[3].field, "value");
}

TEST_F(JsonPathFunctionsTest, ParseRootPath) {
    auto segments = JSONPath::parse("$");
    EXPECT_EQ(segments.size(), 0);
}

TEST_F(JsonPathFunctionsTest, ParseInvalidPath) {
    EXPECT_THROW(JSONPath::parse("$.field["), std::runtime_error);
}

// ============================================================================
// JSON_EXTRACT Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, ExtractSimpleField) {
    json doc = {{"name", "Alice"}, {"age", 30}};
    auto result = JSONPath::extract(doc, "$.name");
    EXPECT_EQ(result, "Alice");
}

TEST_F(JsonPathFunctionsTest, ExtractNestedField) {
    json doc = {{"user", {{"profile", {{"email", "alice@example.com"}}}}}};
    auto result = JSONPath::extract(doc, "$.user.profile.email");
    EXPECT_EQ(result, "alice@example.com");
}

TEST_F(JsonPathFunctionsTest, ExtractArrayElement) {
    json doc = {{"items", json::array({1, 2, 3, 4, 5})}};
    auto result = JSONPath::extract(doc, "$.items[2]");
    EXPECT_EQ(result, 3);
}

TEST_F(JsonPathFunctionsTest, ExtractMixedPath) {
    json doc = {
        {"orders", json::array({
            {{"id", 1}, {"product", "Widget"}},
            {{"id", 2}, {"product", "Gadget"}}
        })}
    };
    auto result = JSONPath::extract(doc, "$.orders[1].product");
    EXPECT_EQ(result, "Gadget");
}

TEST_F(JsonPathFunctionsTest, ExtractNonExistentField) {
    json doc = {{"name", "Alice"}};
    auto result = JSONPath::extract(doc, "$.nonexistent");
    EXPECT_TRUE(result.is_null());
}

TEST_F(JsonPathFunctionsTest, ExtractInvalidIndex) {
    json doc = {{"items", json::array({1, 2, 3})}};
    auto result = JSONPath::extract(doc, "$.items[10]");
    EXPECT_TRUE(result.is_null());
}

TEST_F(JsonPathFunctionsTest, JsonExtractFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_EXTRACT");
    ASSERT_NE(func, nullptr);
    
    json doc = {{"user", {{"name", "Bob"}}}};
    json result = func->execute({doc, "$.user.name"}, context);
    EXPECT_EQ(result, "Bob");
}

// ============================================================================
// JSON_SET Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, SetSimpleField) {
    json doc = {{"name", "Alice"}};
    JSONPath::set(doc, "$.age", 30);
    
    EXPECT_EQ(doc["name"], "Alice");
    EXPECT_EQ(doc["age"], 30);
}

TEST_F(JsonPathFunctionsTest, SetNestedField) {
    json doc = {{"user", {{"name", "Alice"}}}};
    JSONPath::set(doc, "$.user.age", 30);
    
    EXPECT_EQ(doc["user"]["name"], "Alice");
    EXPECT_EQ(doc["user"]["age"], 30);
}

TEST_F(JsonPathFunctionsTest, SetCreatesIntermediatePaths) {
    json doc = json::object();
    JSONPath::set(doc, "$.a.b.c", 123);
    
    EXPECT_EQ(doc["a"]["b"]["c"], 123);
}

TEST_F(JsonPathFunctionsTest, SetArrayElement) {
    json doc = {{"items", json::array({1, 2, 3})}};
    JSONPath::set(doc, "$.items[1]", 99);
    
    EXPECT_EQ(doc["items"][0], 1);
    EXPECT_EQ(doc["items"][1], 99);
    EXPECT_EQ(doc["items"][2], 3);
}

TEST_F(JsonPathFunctionsTest, SetExpandsArray) {
    json doc = {{"items", json::array()}};
    JSONPath::set(doc, "$.items[2]", "third");
    
    EXPECT_EQ(doc["items"].size(), 3);
    EXPECT_TRUE(doc["items"][0].is_null());
    EXPECT_TRUE(doc["items"][1].is_null());
    EXPECT_EQ(doc["items"][2], "third");
}

TEST_F(JsonPathFunctionsTest, JsonSetFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_SET");
    ASSERT_NE(func, nullptr);
    
    json doc = {{"count", 5}};
    json result = func->execute({doc, "$.count", 10}, context);
    
    EXPECT_EQ(result["count"], 10);
}

// ============================================================================
// JSON_REMOVE Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, RemoveSimpleField) {
    json doc = {{"name", "Alice"}, {"age", 30}};
    bool removed = JSONPath::remove(doc, "$.age");
    
    EXPECT_TRUE(removed);
    EXPECT_TRUE(doc.contains("name"));
    EXPECT_FALSE(doc.contains("age"));
}

TEST_F(JsonPathFunctionsTest, RemoveNestedField) {
    json doc = {{"user", {{"name", "Alice"}, {"age", 30}}}};
    bool removed = JSONPath::remove(doc, "$.user.age");
    
    EXPECT_TRUE(removed);
    EXPECT_TRUE(doc["user"].contains("name"));
    EXPECT_FALSE(doc["user"].contains("age"));
}

TEST_F(JsonPathFunctionsTest, RemoveArrayElement) {
    json doc = {{"items", json::array({1, 2, 3, 4})}};
    bool removed = JSONPath::remove(doc, "$.items[1]");
    
    EXPECT_TRUE(removed);
    EXPECT_EQ(doc["items"].size(), 3);
    EXPECT_EQ(doc["items"][0], 1);
    EXPECT_EQ(doc["items"][1], 3);  // Element shifted
    EXPECT_EQ(doc["items"][2], 4);
}

TEST_F(JsonPathFunctionsTest, RemoveNonExistentField) {
    json doc = {{"name", "Alice"}};
    bool removed = JSONPath::remove(doc, "$.nonexistent");
    
    EXPECT_FALSE(removed);
}

TEST_F(JsonPathFunctionsTest, JsonRemoveFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_REMOVE");
    ASSERT_NE(func, nullptr);
    
    json doc = {{"name", "Alice"}, {"age", 30}};
    json result = func->execute({doc, "$.age"}, context);
    
    EXPECT_TRUE(result.contains("name"));
    EXPECT_FALSE(result.contains("age"));
}

// ============================================================================
// JSON_TYPE Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, JsonTypeFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_TYPE");
    ASSERT_NE(func, nullptr);
    
    json doc = {
        {"null_val", nullptr},
        {"bool_val", true},
        {"int_val", 42},
        {"float_val", 3.14},
        {"str_val", "hello"},
        {"arr_val", json::array({1, 2, 3})},
        {"obj_val", json::object({{"key", "value"}})}
    };
    
    EXPECT_EQ(func->execute({doc, "$.null_val"}, context), "null");
    EXPECT_EQ(func->execute({doc, "$.bool_val"}, context), "boolean");
    EXPECT_EQ(func->execute({doc, "$.int_val"}, context), "integer");
    EXPECT_EQ(func->execute({doc, "$.float_val"}, context), "number");
    EXPECT_EQ(func->execute({doc, "$.str_val"}, context), "string");
    EXPECT_EQ(func->execute({doc, "$.arr_val"}, context), "array");
    EXPECT_EQ(func->execute({doc, "$.obj_val"}, context), "object");
}

// ============================================================================
// JSON_CONTAINS Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, JsonContainsTrue) {
    json doc = {
        {"name", "Alice"},
        {"tags", json::array({"developer", "designer"})},
        {"nested", {{"value", 42}}}
    };
    
    EXPECT_TRUE(JSONPath::contains(doc, "Alice"));
    EXPECT_TRUE(JSONPath::contains(doc, "developer"));
    EXPECT_TRUE(JSONPath::contains(doc, 42));
}

TEST_F(JsonPathFunctionsTest, JsonContainsFalse) {
    json doc = {{"name", "Alice"}};
    
    EXPECT_FALSE(JSONPath::contains(doc, "Bob"));
    EXPECT_FALSE(JSONPath::contains(doc, 123));
}

TEST_F(JsonPathFunctionsTest, JsonContainsFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_CONTAINS");
    ASSERT_NE(func, nullptr);
    
    json doc = {{"items", json::array({1, 2, 3})}};
    
    EXPECT_EQ(func->execute({doc, 2}, context), true);
    EXPECT_EQ(func->execute({doc, 5}, context), false);
}

// ============================================================================
// JSON_DEPTH Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, JsonDepthFlat) {
    json doc = {{"a", 1}, {"b", 2}};
    EXPECT_EQ(JSONPath::depth(doc), 1);
}

TEST_F(JsonPathFunctionsTest, JsonDepthNested) {
    json doc = {{"a", {{"b", {{"c", 1}}}}}};
    EXPECT_EQ(JSONPath::depth(doc), 3);
}

TEST_F(JsonPathFunctionsTest, JsonDepthArray) {
    json doc = json::array({1, json::array({2, json::array({3})})});
    EXPECT_EQ(JSONPath::depth(doc), 3);
}

TEST_F(JsonPathFunctionsTest, JsonDepthPrimitive) {
    json doc = 42;
    EXPECT_EQ(JSONPath::depth(doc), 0);
}

TEST_F(JsonPathFunctionsTest, JsonDepthFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_DEPTH");
    ASSERT_NE(func, nullptr);
    
    json doc = {{"a", {{"b", {{"c", 1}}}}}};
    EXPECT_EQ(func->execute({doc}, context), 3);
}

// ============================================================================
// JSON_PARSE and JSON_STRINGIFY Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, JsonParseFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_PARSE");
    ASSERT_NE(func, nullptr);
    
    std::string json_str = R"({"name": "Alice", "age": 30})";
    json result = func->execute({json_str}, context);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["age"], 30);
}

TEST_F(JsonPathFunctionsTest, JsonParseArray) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_PARSE");
    ASSERT_NE(func, nullptr);
    
    std::string json_str = "[1, 2, 3, 4, 5]";
    json result = func->execute({json_str}, context);
    
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 1);
}

TEST_F(JsonPathFunctionsTest, JsonParseInvalid) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_PARSE");
    ASSERT_NE(func, nullptr);
    
    std::string json_str = "{invalid json}";
    EXPECT_THROW(func->execute({json_str}, context), std::runtime_error);
}

TEST_F(JsonPathFunctionsTest, JsonStringifyFunction) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_STRINGIFY");
    ASSERT_NE(func, nullptr);
    
    json doc = {{"name", "Alice"}, {"age", 30}};
    json result = func->execute({doc}, context);
    
    EXPECT_TRUE(result.is_string());
    std::string json_str = result.get<std::string>();
    EXPECT_TRUE(json_str.find("Alice") != std::string::npos);
    EXPECT_TRUE(json_str.find("30") != std::string::npos);
}

TEST_F(JsonPathFunctionsTest, JsonStringifyArray) {
    auto& registry = FunctionRegistry::instance();
    auto func = registry.getFunction("JSON_STRINGIFY");
    ASSERT_NE(func, nullptr);
    
    json arr = json::array({1, 2, 3});
    json result = func->execute({arr}, context);
    
    EXPECT_EQ(result, "[1,2,3]");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(JsonPathFunctionsTest, ComplexNestedOperations) {
    json doc = {
        {"users", json::array({
            {{"id", 1}, {"name", "Alice"}, {"role", "admin"}},
            {{"id", 2}, {"name", "Bob"}, {"role", "user"}},
            {{"id", 3}, {"name", "Charlie"}, {"role", "user"}}
        })}
    };
    
    // Extract user name
    auto name = JSONPath::extract(doc, "$.users[1].name");
    EXPECT_EQ(name, "Bob");
    
    // Update user role
    JSONPath::set(doc, "$.users[1].role", "moderator");
    EXPECT_EQ(doc["users"][1]["role"], "moderator");
    
    // Remove user
    JSONPath::remove(doc, "$.users[2]");
    EXPECT_EQ(doc["users"].size(), 2);
}

TEST_F(JsonPathFunctionsTest, RoundTripParseStringify) {
    auto& registry = FunctionRegistry::instance();
    auto parse_func = registry.getFunction("JSON_PARSE");
    auto stringify_func = registry.getFunction("JSON_STRINGIFY");
    
    std::string original = R"({"name":"Alice","age":30})";
    json parsed = parse_func->execute({original}, context);
    json stringified = stringify_func->execute({parsed}, context);
    json reparsed = parse_func->execute({stringified}, context);
    
    EXPECT_EQ(parsed, reparsed);
}

TEST_F(JsonPathFunctionsTest, EmptyDocument) {
    json doc = json::object();
    
    // Set on empty doc
    JSONPath::set(doc, "$.newField", "value");
    EXPECT_EQ(doc["newField"], "value");
    
    // Extract from empty doc
    json empty = json::object();
    auto result = JSONPath::extract(empty, "$.nonexistent");
    EXPECT_TRUE(result.is_null());
}

TEST_F(JsonPathFunctionsTest, DeepNesting) {
    json doc = json::object();
    
    // Create deeply nested structure
    JSONPath::set(doc, "$.a.b.c.d.e.f", "deep");
    
    // Verify depth
    EXPECT_EQ(JSONPath::depth(doc), 6);
    
    // Extract deep value
    auto result = JSONPath::extract(doc, "$.a.b.c.d.e.f");
    EXPECT_EQ(result, "deep");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(JsonPathFunctionsTest, SpecialCharactersInFieldNames) {
    // Note: This test may need adjustment based on JSONPath spec support
    json doc = {{"field-with-dashes", 123}};
    auto result = JSONPath::extract(doc, "$.field-with-dashes");
    EXPECT_EQ(result, 123);
}

TEST_F(JsonPathFunctionsTest, NegativeArrayIndex) {
    // Negative indices should not work (not supported in this implementation)
    json doc = {{"items", json::array({1, 2, 3})}};
    auto result = JSONPath::extract(doc, "$.items[-1]");
    EXPECT_TRUE(result.is_null());
}

TEST_F(JsonPathFunctionsTest, LargeArrayIndex) {
    json doc = {{"items", json::array({1, 2, 3})}};
    auto result = JSONPath::extract(doc, "$.items[1000]");
    EXPECT_TRUE(result.is_null());
}
