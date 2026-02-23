// Tests for query result type annotations (Issue #1432)
//
// Validates:
//   - inferFieldType() classifies all JSON value kinds correctly
//   - inferResultSchema() builds a well-formed schema from a result array
//   - Nullable detection when fields are absent or null in some rows
//   - VECTOR detection for numeric arrays
//   - ResultFieldAnnotation::toJson() / QueryResultSchema::toJson()
//   - QueryResultSchema::find() lookup

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "query/result_type_annotation.h"

using namespace themis::query;
using json = nlohmann::json;

// ────────────────────────────────────────────────────────────────────────────
// inferFieldType
// ────────────────────────────────────────────────────────────────────────────

TEST(ResultFieldTypeTest, NullValue) {
    EXPECT_EQ(inferFieldType(json(nullptr)), ResultFieldType::NULL_TYPE);
}

TEST(ResultFieldTypeTest, BoolValue) {
    EXPECT_EQ(inferFieldType(json(true)),  ResultFieldType::BOOL);
    EXPECT_EQ(inferFieldType(json(false)), ResultFieldType::BOOL);
}

TEST(ResultFieldTypeTest, IntValue) {
    EXPECT_EQ(inferFieldType(json(42)),    ResultFieldType::INT);
    EXPECT_EQ(inferFieldType(json(-7)),    ResultFieldType::INT);
    EXPECT_EQ(inferFieldType(json(0)),     ResultFieldType::INT);
}

TEST(ResultFieldTypeTest, FloatValue) {
    EXPECT_EQ(inferFieldType(json(3.14)), ResultFieldType::FLOAT);
    EXPECT_EQ(inferFieldType(json(-0.5)), ResultFieldType::FLOAT);
}

TEST(ResultFieldTypeTest, IntegerStoredAsFloat) {
    // 2.0 should be classified as INT (no fractional part)
    EXPECT_EQ(inferFieldType(json(2.0)), ResultFieldType::INT);
}

TEST(ResultFieldTypeTest, StringValue) {
    EXPECT_EQ(inferFieldType(json("hello")), ResultFieldType::STRING);
    EXPECT_EQ(inferFieldType(json("")),      ResultFieldType::STRING);
}

TEST(ResultFieldTypeTest, ObjectValue) {
    EXPECT_EQ(inferFieldType(json{{"a", 1}}), ResultFieldType::OBJECT);
}

TEST(ResultFieldTypeTest, ArrayValue) {
    // Mixed / non-numeric array → ARRAY
    EXPECT_EQ(inferFieldType(json{"a", "b", "c"}), ResultFieldType::ARRAY);
    // Single-element numeric array → ARRAY (not VECTOR; needs >1 element)
    EXPECT_EQ(inferFieldType(json{1.0}), ResultFieldType::ARRAY);
}

TEST(ResultFieldTypeTest, VectorValue) {
    // Numeric array with >1 elements → VECTOR
    json vec = json{0.1, 0.2, 0.3, 0.4};
    EXPECT_EQ(inferFieldType(vec), ResultFieldType::VECTOR);
}

TEST(ResultFieldTypeTest, EmptyArray) {
    EXPECT_EQ(inferFieldType(json::array()), ResultFieldType::ARRAY);
}

// ────────────────────────────────────────────────────────────────────────────
// inferResultSchema – basic cases
// ────────────────────────────────────────────────────────────────────────────

TEST(InferResultSchemaTest, EmptyRowArray) {
    QueryResultSchema schema = inferResultSchema(json::array(), "conjunctive");
    EXPECT_EQ(schema.query_type, "conjunctive");
    EXPECT_TRUE(schema.fields.empty());
}

TEST(InferResultSchemaTest, NonArrayInput) {
    // If a non-array is passed the function should return an empty schema
    QueryResultSchema schema = inferResultSchema(json{{"type", "x"}}, "unknown");
    EXPECT_TRUE(schema.fields.empty());
}

TEST(InferResultSchemaTest, SingleRowAllTypes) {
    json rows = json::array({
        {{"id",       "abc"},
         {"count",    42},
         {"score",    9.5},
         {"active",   true},
         {"deleted",  nullptr},
         {"meta",     {{"k", "v"}}},
         {"tags",     {"a", "b"}}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "conjunctive");
    ASSERT_EQ(schema.fields.size(), 7u);

    auto* id      = schema.find("id");
    auto* count   = schema.find("count");
    auto* score   = schema.find("score");
    auto* active  = schema.find("active");
    auto* deleted = schema.find("deleted");
    auto* meta    = schema.find("meta");
    auto* tags    = schema.find("tags");

    ASSERT_NE(id,      nullptr);  EXPECT_EQ(id->type,     ResultFieldType::STRING);
    ASSERT_NE(count,   nullptr);  EXPECT_EQ(count->type,  ResultFieldType::INT);
    ASSERT_NE(score,   nullptr);  EXPECT_EQ(score->type,  ResultFieldType::FLOAT);
    ASSERT_NE(active,  nullptr);  EXPECT_EQ(active->type, ResultFieldType::BOOL);
    ASSERT_NE(deleted, nullptr);  EXPECT_EQ(deleted->type,ResultFieldType::NULL_TYPE);
    ASSERT_NE(meta,    nullptr);  EXPECT_EQ(meta->type,   ResultFieldType::OBJECT);
    ASSERT_NE(tags,    nullptr);  EXPECT_EQ(tags->type,   ResultFieldType::ARRAY);
}

// ────────────────────────────────────────────────────────────────────────────
// Nullable detection
// ────────────────────────────────────────────────────────────────────────────

TEST(InferResultSchemaTest, NullableWhenExplicitNullInRow) {
    json rows = json::array({
        {{"name", "Alice"}, {"age", 30}},
        {{"name", "Bob"},   {"age", nullptr}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "conjunctive");
    auto* age = schema.find("age");
    ASSERT_NE(age, nullptr);
    EXPECT_TRUE(age->nullable);
    EXPECT_EQ(age->type, ResultFieldType::INT); // null row doesn't override the INT type
}

TEST(InferResultSchemaTest, NullableWhenFieldAbsentInSomeRows) {
    json rows = json::array({
        {{"name", "Alice"}, {"email", "a@b.c"}},
        {{"name", "Bob"}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "conjunctive");
    auto* email = schema.find("email");
    ASSERT_NE(email, nullptr);
    EXPECT_TRUE(email->nullable);

    auto* name = schema.find("name");
    ASSERT_NE(name, nullptr);
    EXPECT_FALSE(name->nullable);
}

TEST(InferResultSchemaTest, NotNullableWhenPresentInAllRows) {
    json rows = json::array({
        {{"pk", "u1"}, {"city", "Berlin"}},
        {{"pk", "u2"}, {"city", "Munich"}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "conjunctive");
    auto* city = schema.find("city");
    ASSERT_NE(city, nullptr);
    EXPECT_FALSE(city->nullable);
}

// ────────────────────────────────────────────────────────────────────────────
// Type promotion across rows
// ────────────────────────────────────────────────────────────────────────────

TEST(InferResultSchemaTest, IntPromotedToFloatAcrossRows) {
    json rows = json::array({
        {{"val", 1}},
        {{"val", 2.5}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "t");
    auto* val = schema.find("val");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->type, ResultFieldType::FLOAT);
}

TEST(InferResultSchemaTest, VectorField) {
    json rows = json::array({
        {{"pk", "u1"}, {"embedding", {0.1, 0.2, 0.3, 0.4}}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "vector_geo");
    auto* emb = schema.find("embedding");
    ASSERT_NE(emb, nullptr);
    EXPECT_EQ(emb->type, ResultFieldType::VECTOR);
    EXPECT_TRUE(emb->is_array);
}

// ────────────────────────────────────────────────────────────────────────────
// Serialisation
// ────────────────────────────────────────────────────────────────────────────

TEST(ResultFieldAnnotationTest, ToJson_NonArray) {
    ResultFieldAnnotation ann;
    ann.name     = "age";
    ann.type     = ResultFieldType::INT;
    ann.nullable = false;

    json j = ann.toJson();
    EXPECT_EQ(j["name"],     "age");
    EXPECT_EQ(j["type"],     "INT");
    EXPECT_EQ(j["nullable"], false);
    EXPECT_FALSE(j.contains("element_type"));
}

TEST(ResultFieldAnnotationTest, ToJson_Array) {
    ResultFieldAnnotation ann;
    ann.name         = "tags";
    ann.type         = ResultFieldType::ARRAY;
    ann.is_array     = true;
    ann.element_type = ResultFieldType::STRING;
    ann.nullable     = true;

    json j = ann.toJson();
    EXPECT_EQ(j["name"],         "tags");
    EXPECT_EQ(j["type"],         "ARRAY");
    EXPECT_EQ(j["nullable"],     true);
    EXPECT_EQ(j["element_type"], "STRING");
}

TEST(QueryResultSchemaTest, ToJson) {
    json rows = json::array({
        {{"id", "x"}, {"score", 1.0}}
    });

    QueryResultSchema schema = inferResultSchema(rows, "conjunctive");
    json j = schema.toJson();

    EXPECT_EQ(j["query_type"], "conjunctive");
    ASSERT_TRUE(j["fields"].is_array());
    ASSERT_GE(j["fields"].size(), 2u);
}

TEST(QueryResultSchemaTest, FindReturnsNullForMissingField) {
    QueryResultSchema schema;
    EXPECT_EQ(schema.find("nonexistent"), nullptr);
}

// ────────────────────────────────────────────────────────────────────────────
// resultFieldTypeName
// ────────────────────────────────────────────────────────────────────────────

TEST(ResultFieldTypeNameTest, AllTypes) {
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::UNKNOWN),   "UNKNOWN");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::NULL_TYPE), "NULL");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::BOOL),      "BOOL");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::INT),       "INT");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::FLOAT),     "FLOAT");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::STRING),    "STRING");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::ARRAY),     "ARRAY");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::OBJECT),    "OBJECT");
    EXPECT_EQ(resultFieldTypeName(ResultFieldType::VECTOR),    "VECTOR");
}
