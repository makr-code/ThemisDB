#include <gtest/gtest.h>
#include "api/graphql.h"
#include <algorithm>

using namespace themis::graphql;

// ============================================================================
// Timeseries Query Fields
// ============================================================================

TEST(GraphQLMultiModel, QueryTypeHasTimeseriesRange) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* queryType = schema.getType("Query");
    ASSERT_NE(queryType, nullptr);

    auto it = std::find_if(queryType->fields.begin(), queryType->fields.end(),
                           [](const FieldDefinition& f) { return f.name == "timeseriesRange"; });
    ASSERT_NE(it, queryType->fields.end());

    EXPECT_EQ(it->type.name, "TimeseriesPoint");
    EXPECT_TRUE(it->type.is_list);
    EXPECT_TRUE(it->type.is_non_null);

    EXPECT_TRUE(it->arguments.count("series") > 0);
    EXPECT_TRUE(it->arguments.count("from") > 0);
    EXPECT_TRUE(it->arguments.count("to") > 0);
    EXPECT_TRUE(it->arguments.count("limit") > 0);

    EXPECT_TRUE(it->arguments.at("series").is_non_null);
    EXPECT_TRUE(it->arguments.at("from").is_non_null);
    EXPECT_TRUE(it->arguments.at("to").is_non_null);
    EXPECT_FALSE(it->arguments.at("limit").is_non_null);
}

TEST(GraphQLMultiModel, QueryTypeHasTimeseriesLatest) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* queryType = schema.getType("Query");
    ASSERT_NE(queryType, nullptr);

    auto it = std::find_if(queryType->fields.begin(), queryType->fields.end(),
                           [](const FieldDefinition& f) { return f.name == "timeseriesLatest"; });
    ASSERT_NE(it, queryType->fields.end());

    EXPECT_EQ(it->type.name, "TimeseriesPoint");
    EXPECT_TRUE(it->type.is_list);
    EXPECT_TRUE(it->type.is_non_null);

    EXPECT_TRUE(it->arguments.count("series") > 0);
    EXPECT_TRUE(it->arguments.at("series").is_non_null);
    EXPECT_TRUE(it->arguments.count("count") > 0);
    EXPECT_FALSE(it->arguments.at("count").is_non_null);
}

// ============================================================================
// Geo Query Fields
// ============================================================================

TEST(GraphQLMultiModel, QueryTypeHasNearbyDocuments) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* queryType = schema.getType("Query");
    ASSERT_NE(queryType, nullptr);

    auto it = std::find_if(queryType->fields.begin(), queryType->fields.end(),
                           [](const FieldDefinition& f) { return f.name == "nearbyDocuments"; });
    ASSERT_NE(it, queryType->fields.end());

    EXPECT_EQ(it->type.name, "Document");
    EXPECT_TRUE(it->type.is_list);
    EXPECT_TRUE(it->type.is_non_null);

    EXPECT_TRUE(it->arguments.count("collection") > 0);
    EXPECT_TRUE(it->arguments.count("center") > 0);
    EXPECT_TRUE(it->arguments.count("radiusKm") > 0);
    EXPECT_TRUE(it->arguments.count("limit") > 0);

    EXPECT_TRUE(it->arguments.at("collection").is_non_null);
    EXPECT_TRUE(it->arguments.at("center").is_non_null);
    EXPECT_TRUE(it->arguments.at("radiusKm").is_non_null);
    EXPECT_FALSE(it->arguments.at("limit").is_non_null);

    EXPECT_EQ(it->arguments.at("center").name, "GeoPointInput");
}

// ============================================================================
// Timeseries Mutation
// ============================================================================

TEST(GraphQLMultiModel, MutationTypeHasInsertTimeseriesPoint) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* mutationType = schema.getType("Mutation");
    ASSERT_NE(mutationType, nullptr);

    auto it = std::find_if(mutationType->fields.begin(), mutationType->fields.end(),
                           [](const FieldDefinition& f) { return f.name == "insertTimeseriesPoint"; });
    ASSERT_NE(it, mutationType->fields.end());

    EXPECT_EQ(it->type.name, "TimeseriesPoint");
    EXPECT_FALSE(it->type.is_list);
    EXPECT_TRUE(it->type.is_non_null);

    EXPECT_TRUE(it->arguments.count("series") > 0);
    EXPECT_TRUE(it->arguments.count("timestamp") > 0);
    EXPECT_TRUE(it->arguments.count("value") > 0);
    EXPECT_TRUE(it->arguments.count("tags") > 0);

    EXPECT_TRUE(it->arguments.at("series").is_non_null);
    EXPECT_TRUE(it->arguments.at("timestamp").is_non_null);
    EXPECT_TRUE(it->arguments.at("value").is_non_null);
    EXPECT_FALSE(it->arguments.at("tags").is_non_null);
}

// ============================================================================
// Subscription Type
// ============================================================================

TEST(GraphQLMultiModel, SubscriptionTypeExists) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* subType = schema.getType("Subscription");
    ASSERT_NE(subType, nullptr);
    EXPECT_EQ(subType->kind, TypeDefinition::Kind::Object);
    EXPECT_FALSE(subType->description.empty());
}

TEST(GraphQLMultiModel, SubscriptionTypeHasOnChange) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* subType = schema.getType("Subscription");
    ASSERT_NE(subType, nullptr);

    auto it = std::find_if(subType->fields.begin(), subType->fields.end(),
                           [](const FieldDefinition& f) { return f.name == "onChange"; });
    ASSERT_NE(it, subType->fields.end());

    EXPECT_EQ(it->type.name, "ChangeEvent");
    EXPECT_TRUE(it->type.is_non_null);
    EXPECT_FALSE(it->type.is_list);

    EXPECT_TRUE(it->arguments.count("collection") > 0);
    EXPECT_TRUE(it->arguments.at("collection").is_non_null);
    EXPECT_TRUE(it->arguments.count("filter") > 0);
    EXPECT_FALSE(it->arguments.at("filter").is_non_null);
}

TEST(GraphQLMultiModel, ChangeEventTypeExists) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* changeEventType = schema.getType("ChangeEvent");
    ASSERT_NE(changeEventType, nullptr);
    EXPECT_EQ(changeEventType->kind, TypeDefinition::Kind::Object);

    auto findField = [&](const std::string& name) {
        return std::find_if(changeEventType->fields.begin(), changeEventType->fields.end(),
                            [&](const FieldDefinition& f) { return f.name == name; });
    };

    EXPECT_NE(findField("sequence"), changeEventType->fields.end());
    EXPECT_NE(findField("type"), changeEventType->fields.end());
    EXPECT_NE(findField("key"), changeEventType->fields.end());
    EXPECT_NE(findField("document"), changeEventType->fields.end());
    EXPECT_NE(findField("timestampMs"), changeEventType->fields.end());
}

TEST(GraphQLMultiModel, ChangeTypeEnumExists) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* changeTypeEnum = schema.getType("ChangeType");
    ASSERT_NE(changeTypeEnum, nullptr);
    EXPECT_EQ(changeTypeEnum->kind, TypeDefinition::Kind::Enum);

    const auto& values = changeTypeEnum->enum_values;
    EXPECT_TRUE(std::find(values.begin(), values.end(), "CREATED") != values.end());
    EXPECT_TRUE(std::find(values.begin(), values.end(), "UPDATED") != values.end());
    EXPECT_TRUE(std::find(values.begin(), values.end(), "DELETED") != values.end());
}

TEST(GraphQLMultiModel, ChangeFilterInputTypeExists) {
    Schema schema = ThemisSchemaBuilder::build();

    const auto* filterType = schema.getType("ChangeFilter");
    ASSERT_NE(filterType, nullptr);
    EXPECT_EQ(filterType->kind, TypeDefinition::Kind::InputObject);
    EXPECT_FALSE(filterType->fields.empty());
}

// ============================================================================
// SDL Generation
// ============================================================================

TEST(GraphQLMultiModel, SDLIncludesSubscriptionType) {
    Schema schema = ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();

    EXPECT_TRUE(sdl.find("subscription: Subscription") != std::string::npos);
    EXPECT_TRUE(sdl.find("type Subscription") != std::string::npos);
    EXPECT_TRUE(sdl.find("onChange") != std::string::npos);
}

TEST(GraphQLMultiModel, SDLIncludesTimeseriesQueryFields) {
    Schema schema = ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();

    EXPECT_TRUE(sdl.find("timeseriesRange") != std::string::npos);
    EXPECT_TRUE(sdl.find("timeseriesLatest") != std::string::npos);
}

TEST(GraphQLMultiModel, SDLIncludesGeoQueryField) {
    Schema schema = ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();

    EXPECT_TRUE(sdl.find("nearbyDocuments") != std::string::npos);
}

TEST(GraphQLMultiModel, SDLIncludesChangeEventType) {
    Schema schema = ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();

    EXPECT_TRUE(sdl.find("ChangeEvent") != std::string::npos);
    EXPECT_TRUE(sdl.find("enum ChangeType") != std::string::npos);
    EXPECT_TRUE(sdl.find("CREATED") != std::string::npos);
    EXPECT_TRUE(sdl.find("UPDATED") != std::string::npos);
    EXPECT_TRUE(sdl.find("DELETED") != std::string::npos);
}

TEST(GraphQLMultiModel, SDLListTypesUseCorrectBracketSyntax) {
    Schema schema = ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();

    // List fields must use '[Type!]!' notation, not 'Type]!' (pre-existing SDL bug fix)
    EXPECT_TRUE(sdl.find("[TimeseriesPoint!]!") != std::string::npos)
        << "timeseriesRange/timeseriesLatest should render as [TimeseriesPoint!]!";
    EXPECT_TRUE(sdl.find("[Document!]!") != std::string::npos)
        << "documents/nearbyDocuments should render as [Document!]!";
    EXPECT_TRUE(sdl.find("[Node!]!") != std::string::npos)
        << "graphTraversal should render as [Node!]!";
    EXPECT_TRUE(sdl.find("[VectorSearchResult!]!") != std::string::npos)
        << "vectorSearch should render as [VectorSearchResult!]!";

    // Make sure the buggy form is not present
    EXPECT_EQ(sdl.find("TimeseriesPoint]!"), std::string::npos)
        << "Invalid SDL 'TimeseriesPoint]!' must not appear";
    EXPECT_EQ(sdl.find("Document]!"), std::string::npos)
        << "Invalid SDL 'Document]!' must not appear";
}

// ============================================================================
// Schema Registration (subscription_type_ set correctly)
// ============================================================================

TEST(GraphQLMultiModel, SchemaSubscriptionTypeRegistered) {
    Schema schema = ThemisSchemaBuilder::build();
    EXPECT_EQ(schema.subscriptionType(), "Subscription");
}
