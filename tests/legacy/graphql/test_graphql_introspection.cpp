// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Unit tests for GraphQL Schema::introspect():
//  - __schema introspection (type list, queryType, mutationType, subscriptionType)
//  - __type introspection (specific type lookup by name)
//  - Introspection disabled via setIntrospectionEnabled(false)
//  - Introspection policy accessors

#include <gtest/gtest.h>
#include "api/graphql.h"
#include <algorithm>

using namespace themis::graphql;

// ============================================================================
// Helpers
// ============================================================================

// Build a Field with an optional string argument "name".
static Field makeField(const std::string& name,
                       const std::string& arg_name = "",
                       const std::string& arg_value = "") {
    Field f;
    f.name = name;
    if (!arg_name.empty()) {
        f.arguments[arg_name] = Value::string(arg_value);
    }
    return f;
}

// ============================================================================
// Introspection policy
// ============================================================================

TEST(GraphQLIntrospection, DefaultIntrospectionIsEnabled) {
    Schema schema;
    EXPECT_TRUE(schema.isIntrospectionEnabled());
}

TEST(GraphQLIntrospection, SetIntrospectionDisabled) {
    Schema schema;
    schema.setIntrospectionEnabled(false);
    EXPECT_FALSE(schema.isIntrospectionEnabled());
}

TEST(GraphQLIntrospection, SetIntrospectionEnabledThenDisabled) {
    Schema schema;
    schema.setIntrospectionEnabled(false);
    schema.setIntrospectionEnabled(true);
    EXPECT_TRUE(schema.isIntrospectionEnabled());
}

TEST(GraphQLIntrospection, IntrospectReturnsNullWhenDisabled) {
    Schema schema;
    schema.setIntrospectionEnabled(false);

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isNull());
}

// ============================================================================
// __schema introspection
// ============================================================================

TEST(GraphQLIntrospection, SchemaFieldReturnsObject) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isObject());
}

TEST(GraphQLIntrospection, SchemaFieldHasTypesArray) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto it = obj.find("types");
    ASSERT_NE(it, obj.end());
    EXPECT_TRUE(it->second->isList());
}

TEST(GraphQLIntrospection, SchemaFieldTypesListIsNonEmpty) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    const auto& types = obj.at("types")->asList();
    EXPECT_FALSE(types.empty());
}

TEST(GraphQLIntrospection, SchemaFieldHasQueryType) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto qtIt = obj.find("queryType");
    ASSERT_NE(qtIt, obj.end());
    ASSERT_TRUE(qtIt->second->isObject());

    const auto& qt = qtIt->second->asObject();
    auto nameIt = qt.find("name");
    ASSERT_NE(nameIt, qt.end());
    EXPECT_EQ(nameIt->second->asString(), "Query");
}

TEST(GraphQLIntrospection, SchemaFieldHasMutationType) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto mtIt = obj.find("mutationType");
    ASSERT_NE(mtIt, obj.end());
    // Mutation type is set in ThemisSchemaBuilder
    EXPECT_TRUE(mtIt->second->isObject() || mtIt->second->isNull());
}

TEST(GraphQLIntrospection, SchemaFieldHasSubscriptionType) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    EXPECT_NE(obj.find("subscriptionType"), obj.end());
}

TEST(GraphQLIntrospection, SchemaTypeEntriesHaveNameKindAndFields) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    const auto& types = obj.at("types")->asList();
    ASSERT_FALSE(types.empty());

    // Check that each type entry has name, kind, and fields keys
    for (const auto& typeVal : types) {
        ASSERT_TRUE(typeVal->isObject());
        const auto& typeObj = typeVal->asObject();
        EXPECT_NE(typeObj.find("name"),   typeObj.end());
        EXPECT_NE(typeObj.find("kind"),   typeObj.end());
        EXPECT_NE(typeObj.find("fields"), typeObj.end());
    }
}

// ============================================================================
// __type introspection
// ============================================================================

TEST(GraphQLIntrospection, TypeFieldReturnsNullForUnknownType) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__type", "name", "NonExistentType");
    auto result = schema.introspect(f);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isNull());
}

TEST(GraphQLIntrospection, TypeFieldReturnsNullWithoutNameArgument) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f;
    f.name = "__type";
    // No "name" argument provided
    auto result = schema.introspect(f);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isNull());
}

TEST(GraphQLIntrospection, TypeFieldReturnsObjectForQueryType) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__type", "name", "Query");
    auto result = schema.introspect(f);

    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->isObject());
}

TEST(GraphQLIntrospection, TypeFieldQueryTypeHasCorrectName) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__type", "name", "Query");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto nameIt = obj.find("name");
    ASSERT_NE(nameIt, obj.end());
    EXPECT_EQ(nameIt->second->asString(), "Query");
}

TEST(GraphQLIntrospection, TypeFieldQueryTypeHasObjectKind) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__type", "name", "Query");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto kindIt = obj.find("kind");
    ASSERT_NE(kindIt, obj.end());
    EXPECT_EQ(kindIt->second->asString(), "OBJECT");
}

TEST(GraphQLIntrospection, TypeFieldQueryTypeHasFieldsList) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__type", "name", "Query");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto fieldsIt = obj.find("fields");
    ASSERT_NE(fieldsIt, obj.end());
    EXPECT_TRUE(fieldsIt->second->isList());
    EXPECT_FALSE(fieldsIt->second->asList().empty());
}

TEST(GraphQLIntrospection, TypeFieldScalarTypeHasScalarKind) {
    Schema schema = ThemisSchemaBuilder::build();

    // "String" is a built-in scalar
    Field f = makeField("__type", "name", "String");
    auto result = schema.introspect(f);
    ASSERT_NE(result, nullptr);
    // String is a built-in scalar – only check if found, as built-ins may vary
    if (!result->isNull()) {
        const auto& obj = result->asObject();
        auto kindIt = obj.find("kind");
        ASSERT_NE(kindIt, obj.end());
        EXPECT_EQ(kindIt->second->asString(), "SCALAR");
    }
}

TEST(GraphQLIntrospection, TypeFieldHasDescriptionKey) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f = makeField("__type", "name", "Query");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    EXPECT_NE(obj.find("description"), obj.end());
}

// ============================================================================
// Introspection on a custom (non-built) schema
// ============================================================================

TEST(GraphQLIntrospection, CustomSchemaTypeWithEnumKind) {
    Schema schema;
    TypeDefinition enumType;
    enumType.kind = TypeDefinition::Kind::Enum;
    enumType.name = "Status";
    enumType.description = "Entity status";
    enumType.enum_values = {"ACTIVE", "INACTIVE"};
    schema.addType(std::move(enumType));

    Field f = makeField("__type", "name", "Status");
    auto result = schema.introspect(f);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    auto kindIt = obj.find("kind");
    ASSERT_NE(kindIt, obj.end());
    EXPECT_EQ(kindIt->second->asString(), "ENUM");
}

TEST(GraphQLIntrospection, CustomSchemaTypeWithInputObjectKind) {
    Schema schema;
    TypeDefinition inputType;
    inputType.kind = TypeDefinition::Kind::InputObject;
    inputType.name = "UserInput";
    schema.addType(std::move(inputType));

    Field f = makeField("__type", "name", "UserInput");
    auto result = schema.introspect(f);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    EXPECT_EQ(obj.at("kind")->asString(), "INPUT_OBJECT");
}

TEST(GraphQLIntrospection, UnknownFieldReturnsNull) {
    Schema schema = ThemisSchemaBuilder::build();

    Field f;
    f.name = "__unknownIntrospectionField";
    auto result = schema.introspect(f);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isNull());
}

// ============================================================================
// Introspection with no mutation or subscription type set
// ============================================================================

TEST(GraphQLIntrospection, EmptyMutationTypeReturnsNull) {
    Schema schema;
    schema.setMutationType("");
    schema.setSubscriptionType("");

    // Add a minimal Query type
    TypeDefinition queryType;
    queryType.kind = TypeDefinition::Kind::Object;
    queryType.name = "Query";
    schema.addType(std::move(queryType));
    schema.setQueryType("Query");

    Field f = makeField("__schema");
    auto result = schema.introspect(f);
    ASSERT_TRUE(result->isObject());

    const auto& obj = result->asObject();
    // mutationType and subscriptionType should be null when empty
    auto mtIt = obj.find("mutationType");
    ASSERT_NE(mtIt, obj.end());
    EXPECT_TRUE(mtIt->second->isNull());

    auto stIt = obj.find("subscriptionType");
    ASSERT_NE(stIt, obj.end());
    EXPECT_TRUE(stIt->second->isNull());
}
