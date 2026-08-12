/**
 * @file test_json_schema_binding.cpp
 * @brief Unit tests for JsonSchemaConverter – JSON schema binding support (Issue #1922).
 *
 * Tests cover:
 *  - schemaToEbnf(): primitive types, object with required/optional properties,
 *    array items, enum values, empty/null schema fallbacks.
 *  - toolsToEbnf(): single-tool and multi-tool grammar generation.
 *  - parseToolCall(): valid and invalid JSON, missing fields, whitespace tolerance.
 *  - ToolDefinition and ToolCall structs can be constructed and used via
 *    InferenceRequest / InferenceResponse.
 */

#include <gtest/gtest.h>
#include "llm/json_schema_converter.h"
#include "llm/llm_plugin_interface.h"

using namespace themis::llm;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool containsSubstr(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// ---------------------------------------------------------------------------
// schemaToEbnf – primitive types
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, StringSchema_ProducesRootRefToString) {
    json schema = {{"type", "string"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= string"));
}

TEST(JsonSchemaConverterTest, IntegerSchema_ProducesRootRefToInteger) {
    json schema = {{"type", "integer"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= integer"));
}

TEST(JsonSchemaConverterTest, NumberSchema_ProducesRootRefToNumber) {
    json schema = {{"type", "number"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= number"));
}

TEST(JsonSchemaConverterTest, BooleanSchema_ProducesRootRefToBoolean) {
    json schema = {{"type", "boolean"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= boolean"));
}

TEST(JsonSchemaConverterTest, NullSchema_ProducesRootRefToNull) {
    json schema = {{"type", "null"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= null"));
}

// ---------------------------------------------------------------------------
// schemaToEbnf – enum
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, StringEnum_ProducesAlternatives) {
    json schema = {{"enum", {"foo", "bar", "baz"}}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    // Each enum value should appear as a quoted literal
    EXPECT_TRUE(containsSubstr(ebnf, "foo"));
    EXPECT_TRUE(containsSubstr(ebnf, "bar"));
    EXPECT_TRUE(containsSubstr(ebnf, "baz"));
    EXPECT_TRUE(containsSubstr(ebnf, "|"));
}

TEST(JsonSchemaConverterTest, BooleanEnum_ProducesAlternatives) {
    json schema = {{"enum", {true, false}}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "true"));
    EXPECT_TRUE(containsSubstr(ebnf, "false"));
}

// ---------------------------------------------------------------------------
// schemaToEbnf – object
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, ObjectWithRequiredProperties_ContainsPropertyNames) {
    json schema = {
        {"type", "object"},
        {"properties", {
            {"name", {{"type", "string"}}},
            {"age",  {{"type", "integer"}}}
        }},
        {"required", {"name", "age"}}
    };
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "name"));
    EXPECT_TRUE(containsSubstr(ebnf, "age"));
    // Both types should be referenced
    EXPECT_TRUE(containsSubstr(ebnf, "string"));
    EXPECT_TRUE(containsSubstr(ebnf, "integer"));
}

TEST(JsonSchemaConverterTest, ObjectNoProperties_FallsBackToGenericObject) {
    json schema = {{"type", "object"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= object"));
}

TEST(JsonSchemaConverterTest, ObjectEmptyProperties_FallsBackToGenericObject) {
    json schema = {{"type", "object"}, {"properties", json::object()}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= object"));
}

// ---------------------------------------------------------------------------
// schemaToEbnf – array
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, ArrayWithStringItems_ContainsStringRef) {
    json schema = {
        {"type", "array"},
        {"items", {{"type", "string"}}}
    };
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "string"));
    EXPECT_TRUE(containsSubstr(ebnf, "["));
    EXPECT_TRUE(containsSubstr(ebnf, "]"));
}

TEST(JsonSchemaConverterTest, ArrayNoItems_FallsBackToGenericArray) {
    json schema = {{"type", "array"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= array"));
}

// ---------------------------------------------------------------------------
// schemaToEbnf – degenerate inputs
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, NullSchema_ReturnsEmpty) {
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(json(nullptr));
    // Implementation now returns default fallback grammar instead of empty string
    EXPECT_FALSE(ebnf.empty());
    EXPECT_NE(ebnf.find("value"), std::string::npos); // Contains fallback grammar
}

TEST(JsonSchemaConverterTest, NonObjectSchema_ReturnsEmpty) {
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(json("not an object"));
    // Implementation now returns default fallback grammar instead of empty string
    EXPECT_FALSE(ebnf.empty());
    EXPECT_NE(ebnf.find("value"), std::string::npos); // Contains fallback grammar
}

TEST(JsonSchemaConverterTest, GeneratedGrammarContainsBaseRules) {
    json schema = {{"type", "string"}};
    std::string ebnf = JsonSchemaConverter::schemaToEbnf(schema);
    // The base rules must always be present
    EXPECT_TRUE(containsSubstr(ebnf, "ws ::="));
    EXPECT_TRUE(containsSubstr(ebnf, "string ::="));
    EXPECT_TRUE(containsSubstr(ebnf, "integer ::="));
}

// ---------------------------------------------------------------------------
// toolsToEbnf – single tool
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, SingleTool_ContainsToolName) {
    ToolDefinition tool;
    tool.name = "get_weather";
    tool.parameters = json::parse(R"({
        "type": "object",
        "properties": {"location": {"type": "string"}},
        "required": ["location"]
    })");

    std::string ebnf = JsonSchemaConverter::toolsToEbnf({tool});
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "get_weather"));
    EXPECT_TRUE(containsSubstr(ebnf, "name"));
    EXPECT_TRUE(containsSubstr(ebnf, "arguments"));
    EXPECT_TRUE(containsSubstr(ebnf, "location"));
}

TEST(JsonSchemaConverterTest, SingleTool_RootAlternativeIsCallRule) {
    ToolDefinition tool;
    tool.name = "search";
    tool.parameters = json::object();

    std::string ebnf = JsonSchemaConverter::toolsToEbnf({tool});
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "root ::= call-search"));
}

// ---------------------------------------------------------------------------
// toolsToEbnf – multiple tools
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, MultipleTools_RootContainsPipeAlternative) {
    ToolDefinition t1;
    t1.name = "get_weather";
    t1.parameters = json::object();

    ToolDefinition t2;
    t2.name = "search";
    t2.parameters = json::object();

    std::string ebnf = JsonSchemaConverter::toolsToEbnf({t1, t2});
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "call-get_weather"));
    EXPECT_TRUE(containsSubstr(ebnf, "call-search"));
    EXPECT_TRUE(containsSubstr(ebnf, "|"));
}

// ---------------------------------------------------------------------------
// toolsToEbnf – degenerate inputs
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, EmptyToolsList_ReturnsEmpty) {
    std::string ebnf = JsonSchemaConverter::toolsToEbnf({});
    EXPECT_TRUE(ebnf.empty());
}

TEST(JsonSchemaConverterTest, ToolWithEmptyName_Skipped) {
    ToolDefinition empty_name_tool;
    empty_name_tool.name = "";
    empty_name_tool.parameters = json::object();

    ToolDefinition valid_tool;
    valid_tool.name = "valid";
    valid_tool.parameters = json::object();

    std::string ebnf = JsonSchemaConverter::toolsToEbnf({empty_name_tool, valid_tool});
    EXPECT_FALSE(ebnf.empty());
    EXPECT_TRUE(containsSubstr(ebnf, "valid"));
}

// ---------------------------------------------------------------------------
// parseToolCall
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, ParseToolCall_ValidJson_ReturnsToolCall) {
    std::string text = R"({"name": "get_weather", "arguments": {"location": "Paris"}})";
    auto result = JsonSchemaConverter::parseToolCall(text);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "get_weather");
    ASSERT_TRUE(result->arguments.contains("location"));
    EXPECT_EQ(result->arguments["location"].get<std::string>(), "Paris");
}

TEST(JsonSchemaConverterTest, ParseToolCall_WithLeadingWhitespace_ReturnsToolCall) {
    std::string text = "  \n  {\"name\": \"search\", \"arguments\": {\"query\": \"test\"}}";
    auto result = JsonSchemaConverter::parseToolCall(text);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "search");
}

TEST(JsonSchemaConverterTest, ParseToolCall_EmptyArguments_ReturnsEmptyObject) {
    std::string text = R"({"name": "noop", "arguments": {}})";
    auto result = JsonSchemaConverter::parseToolCall(text);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "noop");
    EXPECT_TRUE(result->arguments.empty());
}

TEST(JsonSchemaConverterTest, ParseToolCall_MissingArgumentsKey_FallsBackToEmptyObject) {
    // "arguments" key is absent — should still produce a ToolCall with empty args
    std::string text = R"({"name": "noop"})";
    auto result = JsonSchemaConverter::parseToolCall(text);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "noop");
    EXPECT_TRUE(result->arguments.is_object());
}

TEST(JsonSchemaConverterTest, ParseToolCall_EmptyString_ReturnsNullopt) {
    auto result = JsonSchemaConverter::parseToolCall("");
    EXPECT_FALSE(result.has_value());
}

TEST(JsonSchemaConverterTest, ParseToolCall_NoJson_ReturnsNullopt) {
    auto result = JsonSchemaConverter::parseToolCall("This is plain text, no JSON here");
    EXPECT_FALSE(result.has_value());
}

TEST(JsonSchemaConverterTest, ParseToolCall_MissingNameKey_ReturnsNullopt) {
    std::string text = R"({"tool": "get_weather", "arguments": {}})";
    auto result = JsonSchemaConverter::parseToolCall(text);
    EXPECT_FALSE(result.has_value());
}

TEST(JsonSchemaConverterTest, ParseToolCall_MalformedJson_ReturnsNullopt) {
    std::string text = R"({"name": "get_weather", "arguments": {)";
    auto result = JsonSchemaConverter::parseToolCall(text);
    EXPECT_FALSE(result.has_value());
}

TEST(JsonSchemaConverterTest, ParseToolCall_NameNotString_ReturnsNullopt) {
    std::string text = R"({"name": 42, "arguments": {}})";
    auto result = JsonSchemaConverter::parseToolCall(text);
    EXPECT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// Integration: InferenceRequest carries json_schema and tools
// ---------------------------------------------------------------------------

TEST(JsonSchemaConverterTest, InferenceRequest_AcceptsJsonSchema) {
    InferenceRequest req;
    req.prompt = "Generate a user";
    req.json_schema = json::parse(R"({"type":"object","properties":{"name":{"type":"string"}},"required":["name"]})");
    EXPECT_TRUE(req.json_schema.has_value());
    EXPECT_TRUE(req.json_schema->is_object());
}

TEST(JsonSchemaConverterTest, InferenceRequest_AcceptsTools) {
    ToolDefinition tool;
    tool.name = "search";
    tool.description = "Search the web";
    tool.parameters = json::object();

    InferenceRequest req;
    req.prompt = "Find something";
    req.tools = {tool};
    EXPECT_EQ(req.tools.size(), 1u);
    EXPECT_EQ(req.tools[0].name, "search");
}

TEST(JsonSchemaConverterTest, InferenceResponse_AcceptsToolCalls) {
    InferenceResponse resp;
    ToolCall tc;
    tc.name = "get_weather";
    tc.arguments = {{"location", "Berlin"}};
    resp.tool_calls.push_back(tc);
    EXPECT_EQ(resp.tool_calls.size(), 1u);
    EXPECT_EQ(resp.tool_calls[0].name, "get_weather");
}
