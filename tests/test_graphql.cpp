/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_graphql.cpp                                   ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     125                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "api/graphql.h"

using namespace themis::graphql;

// ============================================================================
// Basic Parser Tests
// ============================================================================

TEST(GraphQLParser, ParseSimpleQuery) {
    std::string query = "{ user { id name } }";
    auto result = Parser::parse(query);
    
    EXPECT_TRUE(result.success) << "Simple query should parse successfully";
    EXPECT_TRUE(result.errors.empty());
    EXPECT_EQ(result.document.operations.size(), 1);
}

TEST(GraphQLParser, ParseNamedQuery) {
    std::string query = "query GetUser { user { id } }";
    auto result = Parser::parse(query);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.document.operations.size(), 1);
    EXPECT_EQ(result.document.operations[0].name, "GetUser");
}

TEST(GraphQLParser, ParseMutation) {
    std::string query = "mutation UpdateUser { updateUser(id: \"123\") { id } }";
    auto result = Parser::parse(query);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.document.operations[0].type, OperationType::Mutation);
}

TEST(GraphQLParser, ParseInvalidSyntax) {
    std::string query = "{ user { id ";  // Missing closing braces
    auto result = Parser::parse(query);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errors.empty());
}

TEST(GraphQLParser, ParseFieldWithArguments) {
    std::string query = R"({ user(id: "123") { name } })";
    auto result = Parser::parse(query);
    
    EXPECT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    EXPECT_FALSE(op.selections.empty());
    EXPECT_FALSE(op.selections[0].arguments.empty());
}

// ============================================================================
// Value Type Tests
// ============================================================================

TEST(GraphQLValue, CreateValues) {
    auto nullVal = Value::null();
    EXPECT_TRUE(nullVal->isNull());
    
    auto boolVal = Value::boolean(true);
    EXPECT_TRUE(boolVal->isBool());
    EXPECT_TRUE(boolVal->asBool());
    
    auto intVal = Value::integer(42);
    EXPECT_TRUE(intVal->isInt());
    EXPECT_EQ(intVal->asInt(), 42);
    
    auto floatVal = Value::floating(3.14);
    EXPECT_TRUE(floatVal->isFloat());
    EXPECT_DOUBLE_EQ(floatVal->asFloat(), 3.14);
    
    auto strVal = Value::string("hello");
    EXPECT_TRUE(strVal->isString());
    EXPECT_EQ(strVal->asString(), "hello");
}

TEST(GraphQLValue, CreateListValue) {
    ValueList list;
    list.push_back(Value::integer(1));
    list.push_back(Value::integer(2));
    list.push_back(Value::integer(3));
    
    auto listVal = Value::list(list);
    EXPECT_TRUE(listVal->isList());
    EXPECT_EQ(listVal->asList().size(), 3);
}

TEST(GraphQLValue, CreateObjectValue) {
    ValueMap obj;
    obj["name"] = Value::string("Alice");
    obj["age"] = Value::integer(30);
    
    auto objVal = Value::object(obj);
    EXPECT_TRUE(objVal->isObject());
    EXPECT_EQ(objVal->asObject().size(), 2);
}

