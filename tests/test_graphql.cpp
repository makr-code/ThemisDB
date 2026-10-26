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

TEST(GraphQLParser, ParsesFragmentDefinitionAndSpreadWhenDefinedBeforeUse) {
    const std::string query = R"(
        fragment userFields on User { id name }
        query GetUser { user { ...userFields } }
    )";

    auto result = Parser::parse(query);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.document.fragments.size(), 1);
    ASSERT_EQ(result.document.operations.size(), 1);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.selections.size(), 1);
    const auto& userField = op.selections[0];
    ASSERT_EQ(userField.selections.size(), 2);
    EXPECT_EQ(userField.selections[0].name, "id");
    EXPECT_EQ(userField.selections[1].name, "name");
}

TEST(GraphQLParser, RejectsUnknownFragmentSpread) {
    const std::string query = R"(
        query GetUser { user { ...userFields } }
    )";

    auto result = Parser::parse(query);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].message.find("Unknown fragment"), std::string::npos);
}

TEST(GraphQLParser, ParseIncludeAndSkipDirectives) {
    const std::string query = R"(
        query {
            user @include(if: true) @skip(if: false) { id }
        }
    )";

    auto result = Parser::parse(query);
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.document.operations.size(), 1);
    ASSERT_EQ(result.document.operations[0].selections.size(), 1);
    const auto& field = result.document.operations[0].selections[0];
    ASSERT_EQ(field.directives.size(), 2);
    EXPECT_EQ(field.directives[0].type, Field::Directive::Type::Include);
    EXPECT_EQ(field.directives[1].type, Field::Directive::Type::Skip);
}

TEST(GraphQLParser, RejectsUnsupportedDirectiveName) {
    const std::string query = R"(
        query {
            user @deprecated(if: true) { id }
        }
    )";

    auto result = Parser::parse(query);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].message.find("Unsupported directive"), std::string::npos);
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

TEST(GraphQLExecutor, AppliesIncludeDirectiveWithVariableCondition) {
    const std::string query = R"(
        query($showName: Boolean!) {
            user {
                id
                name @include(if: $showName)
            }
        }
    )";
    auto parseResult = Parser::parse(query);
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.mask_errors = false;
    ctx.variables["showName"] = Value::boolean(false);
    ctx.resolvers["user"] = [](const Field&, const std::shared_ptr<Value>&, const ExecutionContext&) {
        ValueMap user;
        user["id"] = Value::string("u1");
        user["name"] = Value::string("Ada");
        return Value::object(std::move(user));
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);
    ASSERT_FALSE(result.hasErrors());
    ASSERT_TRUE(result.data->isObject());
    const auto& root = result.data->asObject();
    ASSERT_TRUE(root.contains("user"));
    ASSERT_TRUE(root.at("user")->isObject());
    const auto& user = root.at("user")->asObject();
    EXPECT_TRUE(user.contains("id"));
    EXPECT_FALSE(user.contains("name"));
}

TEST(GraphQLExecutor, AppliesSkipDirectiveWithVariableCondition) {
    const std::string query = R"(
        query($skipId: Boolean!) {
            user {
                id @skip(if: $skipId)
                name
            }
        }
    )";
    auto parseResult = Parser::parse(query);
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.mask_errors = false;
    ctx.variables["skipId"] = Value::boolean(true);
    ctx.resolvers["user"] = [](const Field&, const std::shared_ptr<Value>&, const ExecutionContext&) {
        ValueMap user;
        user["id"] = Value::string("u1");
        user["name"] = Value::string("Ada");
        return Value::object(std::move(user));
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);
    ASSERT_FALSE(result.hasErrors());
    ASSERT_TRUE(result.data->isObject());
    const auto& root = result.data->asObject();
    ASSERT_TRUE(root.contains("user"));
    ASSERT_TRUE(root.at("user")->isObject());
    const auto& user = root.at("user")->asObject();
    EXPECT_FALSE(user.contains("id"));
    EXPECT_TRUE(user.contains("name"));
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
