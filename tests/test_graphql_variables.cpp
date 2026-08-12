// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Unit tests covering previously untested GraphQL paths:
//  - Parser::parseVariableDefinition() via queries with variable declarations
//  - Numeric value parsing: integer and float arguments
//  - Executor::executeField() with explicit resolver functions
//  - Executor::executeField() with parent-object traversal
//  - Executor::execute() named operation selection
//  - Executor error propagation (exception in resolver)

#include <gtest/gtest.h>
#include "api/graphql.h"

using namespace themis::graphql;

// ============================================================================
// Variable Definition Parsing
// ============================================================================

TEST(GraphQLVariables, ParseSimpleStringVariable) {
    std::string query = "query GetUser($id: String) { user { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success) << "Query with string variable should parse";
    ASSERT_EQ(result.document.operations.size(), 1u);

    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 1u);
    EXPECT_EQ(op.variables[0].name, "id");
    EXPECT_EQ(op.variables[0].type_name, "String");
    EXPECT_FALSE(op.variables[0].is_non_null);
    EXPECT_FALSE(op.variables[0].is_list);
}

TEST(GraphQLVariables, ParseNonNullVariable) {
    std::string query = "query GetUser($id: String!) { user { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 1u);
    EXPECT_TRUE(op.variables[0].is_non_null);
    EXPECT_EQ(op.variables[0].type_name, "String");
}

TEST(GraphQLVariables, ParseListVariable) {
    std::string query = "query GetItems($ids: [ID]) { items { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 1u);
    EXPECT_TRUE(op.variables[0].is_list);
    EXPECT_EQ(op.variables[0].type_name, "ID");
}

TEST(GraphQLVariables, ParseIntVariableType) {
    std::string query = "query GetPage($limit: Int) { items { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 1u);
    EXPECT_EQ(op.variables[0].type_name, "Int");
}

TEST(GraphQLVariables, ParseBooleanVariableType) {
    std::string query = "query GetItems($active: Boolean) { items { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 1u);
    EXPECT_EQ(op.variables[0].type_name, "Boolean");
}

TEST(GraphQLVariables, ParseMultipleVariables) {
    std::string query = "query Search($term: String!, $limit: Int) { results { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 2u);

    EXPECT_EQ(op.variables[0].name, "term");
    EXPECT_TRUE(op.variables[0].is_non_null);
    EXPECT_EQ(op.variables[1].name, "limit");
    EXPECT_FALSE(op.variables[1].is_non_null);
}

TEST(GraphQLVariables, VariableUsedAsFieldArgument) {
    std::string query = R"(query GetUser($id: String) { user(id: $id) { name } })";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_FALSE(op.selections.empty());

    const auto& userField = op.selections[0];
    EXPECT_EQ(userField.name, "user");
    ASSERT_FALSE(userField.arguments.empty());

    auto idIt = userField.arguments.find("id");
    ASSERT_NE(idIt, userField.arguments.end());
    // Variable references are stored as VariableRef values (not plain strings)
    ASSERT_TRUE(idIt->second->isVariableRef());
    EXPECT_EQ(idIt->second->asVariableRef(), "id");
}

// ============================================================================
// Variable substitution at execution time
// ============================================================================

TEST(GraphQLVariables, ExecutorSubstitutesStringVariable) {
    auto parseResult = Parser::parse(
        R"(query GetUser($id: String) { user(id: $id) { name } })");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.variables["id"] = Value::string("user-42");
    ctx.resolvers["user"] = [](const Field& f,
                                const std::shared_ptr<Value>&,
                                const ExecutionContext&) {
        auto idIt = f.arguments.find("id");
        if (idIt == f.arguments.end() || !idIt->second->isString()) {
            return Value::null();
        }
        return Value::object({{"name", Value::string("user:" + idIt->second->asString())}});
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    auto userIt = result.data->asObject().find("user");
    ASSERT_NE(userIt, result.data->asObject().end());
    ASSERT_TRUE(userIt->second->isObject());
    auto nameIt = userIt->second->asObject().find("name");
    ASSERT_NE(nameIt, userIt->second->asObject().end());
    EXPECT_EQ(nameIt->second->asString(), "user:user-42");
}

TEST(GraphQLVariables, ExecutorSubstitutesIntVariable) {
    auto parseResult = Parser::parse(
        R"(query GetPage($limit: Int) { items(limit: $limit) { id } })");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.variables["limit"] = Value::int_(10LL);
    int64_t capturedLimit = -1;
    ctx.resolvers["items"] = [&capturedLimit](const Field& f,
                                               const std::shared_ptr<Value>&,
                                               const ExecutionContext&) {
        auto it = f.arguments.find("limit");
        if (it != f.arguments.end() && it->second->isInt()) {
            capturedLimit = it->second->asInt();
        }
        return Value::list({});
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    EXPECT_EQ(capturedLimit, 10LL);
}

TEST(GraphQLVariables, ExecutorUsesDefaultValueWhenVariableNotSupplied) {
    auto parseResult = Parser::parse(
        R"(query GetUser($id: String = "default-id") { user(id: $id) { name } })");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;  // No variables supplied at runtime
    std::string capturedId;
    ctx.resolvers["user"] = [&capturedId](const Field& f,
                                           const std::shared_ptr<Value>&,
                                           const ExecutionContext&) {
        auto it = f.arguments.find("id");
        if (it != f.arguments.end() && it->second->isString()) {
            capturedId = it->second->asString();
        }
        return Value::null();
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    EXPECT_EQ(capturedId, "default-id");
}

TEST(GraphQLVariables, ExecutorRuntimeVariableOverridesDefault) {
    auto parseResult = Parser::parse(
        R"(query GetUser($id: String = "default-id") { user(id: $id) { name } })");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.variables["id"] = Value::string("runtime-id");
    std::string capturedId;
    ctx.resolvers["user"] = [&capturedId](const Field& f,
                                           const std::shared_ptr<Value>&,
                                           const ExecutionContext&) {
        auto it = f.arguments.find("id");
        if (it != f.arguments.end() && it->second->isString()) {
            capturedId = it->second->asString();
        }
        return Value::null();
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    EXPECT_EQ(capturedId, "runtime-id");
}

TEST(GraphQLVariables, ExecutorReturnsNullForUnboundVariable) {
    auto parseResult = Parser::parse(
        R"(query GetUser($id: String) { user(id: $id) { name } })");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;  // No variable bound → resolver sees null
    bool sawNull = false;
    ctx.resolvers["user"] = [&sawNull](const Field& f,
                                        const std::shared_ptr<Value>&,
                                        const ExecutionContext&) {
        auto it = f.arguments.find("id");
        if (it != f.arguments.end() && it->second->isNull()) {
            sawNull = true;
        }
        return Value::null();
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    EXPECT_TRUE(sawNull);
}

TEST(GraphQLVariables, VariableWithDefaultStringValue) {
    std::string query = R"(query GetUser($id: String = "default") { user { id } })";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& op = result.document.operations[0];
    ASSERT_EQ(op.variables.size(), 1u);
    ASSERT_NE(op.variables[0].default_value, nullptr);
    ASSERT_TRUE(op.variables[0].default_value->isString());
    EXPECT_EQ(op.variables[0].default_value->asString(), "default");
}

// ============================================================================
// Numeric Value Parsing (integer and float arguments)
// ============================================================================

TEST(GraphQLValueParsing, IntegerArgument) {
    std::string query = "{ items(limit: 42) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    EXPECT_EQ(field.name, "items");

    auto it = field.arguments.find("limit");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isInt());
    EXPECT_EQ(it->second->asInt(), 42LL);
}

TEST(GraphQLValueParsing, NegativeIntegerArgument) {
    std::string query = "{ items(offset: -5) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("offset");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isInt());
    EXPECT_EQ(it->second->asInt(), -5LL);
}

TEST(GraphQLValueParsing, FloatArgument) {
    std::string query = "{ search(radius: 1.5) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("radius");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isFloat());
    EXPECT_DOUBLE_EQ(it->second->asFloat(), 1.5);
}

TEST(GraphQLValueParsing, NegativeFloatArgument) {
    std::string query = "{ geo(lon: -73.935242) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("lon");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isFloat());
    EXPECT_NEAR(it->second->asFloat(), -73.935242, 1e-6);
}

TEST(GraphQLValueParsing, BooleanTrueArgument) {
    std::string query = "{ items(active: true) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("active");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isBool());
    EXPECT_TRUE(it->second->asBool());
}

TEST(GraphQLValueParsing, BooleanFalseArgument) {
    std::string query = "{ items(active: false) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("active");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isBool());
    EXPECT_FALSE(it->second->asBool());
}

TEST(GraphQLValueParsing, NullArgument) {
    std::string query = "{ items(filter: null) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("filter");
    ASSERT_NE(it, field.arguments.end());
    EXPECT_TRUE(it->second->isNull());
}

TEST(GraphQLValueParsing, ListArgument) {
    std::string query = "{ items(ids: [\"a\", \"b\"]) { id } }";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("ids");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isList());
    EXPECT_EQ(it->second->asList().size(), 2u);
}

TEST(GraphQLValueParsing, ObjectArgument) {
    std::string query = R"({ items(filter: {status: "active"}) { id } })";
    auto result = Parser::parse(query);

    ASSERT_TRUE(result.success);
    const auto& field = result.document.operations[0].selections[0];
    auto it = field.arguments.find("filter");
    ASSERT_NE(it, field.arguments.end());
    ASSERT_TRUE(it->second->isObject());
    const auto& obj = it->second->asObject();
    ASSERT_NE(obj.find("status"), obj.end());
    EXPECT_EQ(obj.at("status")->asString(), "active");
}

// ============================================================================
// Executor with resolver functions
// ============================================================================

TEST(GraphQLExecutor, ExecutorCallsRegisteredResolver) {
    auto parseResult = Parser::parse("{ greeting }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.mask_errors = false;
    ctx.resolvers["greeting"] = [](const Field&,
                                   const std::shared_ptr<Value>&,
                                   const ExecutionContext&) {
        return Value::string("hello");
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    ASSERT_TRUE(result.data->isObject());
    auto it = result.data->asObject().find("greeting");
    ASSERT_NE(it, result.data->asObject().end());
    EXPECT_EQ(it->second->asString(), "hello");
}

TEST(GraphQLExecutor, ExecutorUsesFieldAliasAsResponseKey) {
    auto parseResult = Parser::parse("{ hi: greeting }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.resolvers["greeting"] = [](const Field&,
                                   const std::shared_ptr<Value>&,
                                   const ExecutionContext&) {
        return Value::string("world");
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    ASSERT_TRUE(result.data->isObject());
    // Result key is the alias "hi"
    EXPECT_NE(result.data->asObject().find("hi"), result.data->asObject().end());
    EXPECT_EQ(result.data->asObject().find("hi")->second->asString(), "world");
    // Original field name should not be present
    EXPECT_EQ(result.data->asObject().find("greeting"), result.data->asObject().end());
}

TEST(GraphQLExecutor, ExecutorReturnsNullForUnresolvedField) {
    auto parseResult = Parser::parse("{ unknown }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;  // No resolvers registered

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    ASSERT_TRUE(result.data->isObject());
    auto it = result.data->asObject().find("unknown");
    ASSERT_NE(it, result.data->asObject().end());
    EXPECT_TRUE(it->second->isNull());
}

TEST(GraphQLExecutor, ExecutorTraversesParentObject) {
    // Build a parent object that has a "name" field
    auto parseResult = Parser::parse("{ user { name } }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.resolvers["user"] = [](const Field&,
                               const std::shared_ptr<Value>&,
                               const ExecutionContext&) {
        return Value::object({{"name", Value::string("Alice")}});
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    ASSERT_TRUE(result.data->isObject());

    auto userIt = result.data->asObject().find("user");
    ASSERT_NE(userIt, result.data->asObject().end());
    ASSERT_TRUE(userIt->second->isObject());
    auto nameIt = userIt->second->asObject().find("name");
    ASSERT_NE(nameIt, userIt->second->asObject().end());
    EXPECT_EQ(nameIt->second->asString(), "Alice");
}

TEST(GraphQLExecutor, ExecutorTraversesListField) {
    auto parseResult = Parser::parse("{ users { name } }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.resolvers["users"] = [](const Field&,
                                const std::shared_ptr<Value>&,
                                const ExecutionContext&) {
        ValueList users;
        users.push_back(Value::object({{"name", Value::string("Alice")}}));
        users.push_back(Value::object({{"name", Value::string("Bob")}}));
        return Value::list(std::move(users));
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    ASSERT_TRUE(result.data->isObject());

    auto usersIt = result.data->asObject().find("users");
    ASSERT_NE(usersIt, result.data->asObject().end());
    ASSERT_TRUE(usersIt->second->isList());
    EXPECT_EQ(usersIt->second->asList().size(), 2u);
}

TEST(GraphQLExecutor, ExecutorHandlesExceptionInResolver) {
    auto parseResult = Parser::parse("{ broken }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    ctx.mask_errors = false;
    ctx.resolvers["broken"] = [](const Field&,
                                 const std::shared_ptr<Value>&,
                                 const ExecutionContext&) -> std::shared_ptr<Value> {
        throw std::runtime_error("resolver exploded");
    };

    Executor executor;
    auto result = executor.execute(parseResult.document, ctx);

    EXPECT_TRUE(result.hasErrors());
    EXPECT_FALSE(result.errors.empty());
}

// ============================================================================
// Named operation selection
// ============================================================================

TEST(GraphQLExecutor, ExecutorSelectsNamedOperation) {
    // Two named operations in one document; executor must pick the right one.
    std::string query =
        "query GetA { a } "
        "query GetB { b }";
    auto parseResult = Parser::parse(query);
    ASSERT_TRUE(parseResult.success);
    ASSERT_EQ(parseResult.document.operations.size(), 2u);

    ExecutionContext ctx;
    ctx.resolvers["a"] = [](const Field&, const std::shared_ptr<Value>&,
                             const ExecutionContext&) {
        return Value::string("A-value");
    };
    ctx.resolvers["b"] = [](const Field&, const std::shared_ptr<Value>&,
                             const ExecutionContext&) {
        return Value::string("B-value");
    };

    Executor executor;
    // Execute the second operation "GetB"
    auto result = executor.execute(parseResult.document, ctx, "GetB");

    EXPECT_FALSE(result.hasErrors());
    ASSERT_NE(result.data, nullptr);
    ASSERT_TRUE(result.data->isObject());
    auto bIt = result.data->asObject().find("b");
    ASSERT_NE(bIt, result.data->asObject().end());
    EXPECT_EQ(bIt->second->asString(), "B-value");
}

TEST(GraphQLExecutor, ExecutorReturnsErrorForMissingOperation) {
    auto parseResult = Parser::parse("query GetA { a }");
    ASSERT_TRUE(parseResult.success);

    ExecutionContext ctx;
    Executor executor;

    // Request a non-existent operation name
    auto result = executor.execute(parseResult.document, ctx, "GetMissing");

    EXPECT_TRUE(result.hasErrors());
    EXPECT_FALSE(result.errors.empty());
}

// ============================================================================
// Comment handling
// ============================================================================

TEST(GraphQLParser, SkipsInlineComments) {
    std::string query =
        "{ # this is a comment\n"
        "  user { id } }";
    auto result = Parser::parse(query);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.document.operations.size(), 1u);
    ASSERT_FALSE(result.document.operations[0].selections.empty());
    EXPECT_EQ(result.document.operations[0].selections[0].name, "user");
}
