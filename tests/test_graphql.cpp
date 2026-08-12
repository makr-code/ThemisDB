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

TEST(GraphQLParser, RejectsFragmentDefinitionWithVersionGatedMessage) {
    const std::string query = R"(
        fragment userFields on User { id name }
        query GetUser { user { ...userFields } }
    )";

    auto result = Parser::parse(query);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].message.find("unsupported"), std::string::npos);
    EXPECT_NE(result.errors[0].message.find("v1.x"), std::string::npos);
}

TEST(GraphQLParser, RejectsFieldDirectivesWithVersionGatedMessage) {
    const std::string query = R"(
        query {
            user @include(if: true) { id }
        }
    )";

    auto result = Parser::parse(query);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].message.find("directives"), std::string::npos);
    EXPECT_NE(result.errors[0].message.find("v1.x"), std::string::npos);
}

TEST(GraphQLParser, RejectsInlineFragmentsWithVersionGatedMessage) {
    const std::string query = R"(
        query {
            user {
                ... on User { id }
            }
        }
    )";

    auto result = Parser::parse(query);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].message.find("inline fragments"), std::string::npos);
    EXPECT_NE(result.errors[0].message.find("v1.x"), std::string::npos);
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

