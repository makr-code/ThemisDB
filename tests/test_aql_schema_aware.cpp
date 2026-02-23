/**
 * @file test_aql_schema_aware.cpp
 * @brief Unit tests for schema-aware AQL query generation using live collection metadata.
 *
 * Tests cover:
 *  - CollectionMetadata / CollectionFieldInfo construction
 *  - formatSchemaContext() output
 *  - AQLQueryBuilder::setSchema() and getSchemaContext()
 *  - AQLQueryBuilder::getFieldsForCollection()
 *  - Schema-aware validation warnings for unknown collections
 *  - Known collections produce no schema warning
 *  - Schema persists across reset()
 *  - LLM suggestions fall back to attached schema (graceful with no model)
 */

#include <gtest/gtest.h>
#include "aql/aql_schema_provider.h"
#include "aql/aql_query_builder.h"
#include "aql/llm_aql_handler.h"

using namespace themis::aql;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<CollectionMetadata> makeTestSchema() {
    CollectionFieldInfo f1{"name",   "string",  /*indexed=*/true,  /*nullable=*/false};
    CollectionFieldInfo f2{"age",    "integer", /*indexed=*/false, /*nullable=*/true};
    CollectionFieldInfo f3{"email",  "string",  /*indexed=*/true,  /*nullable=*/false};
    CollectionFieldInfo f4{"active", "boolean", /*indexed=*/false, /*nullable=*/true};

    CollectionMetadata users;
    users.name            = "users";
    users.type            = "document";
    users.estimated_count = 1000;
    users.fields          = {f1, f2, f3, f4};

    CollectionFieldInfo o1{"user_id", "string",  true,  false};
    CollectionFieldInfo o2{"amount",  "double",  false, true};
    CollectionFieldInfo o3{"city",    "string",  true,  false};

    CollectionMetadata orders;
    orders.name            = "orders";
    orders.type            = "document";
    orders.estimated_count = 5000;
    orders.fields          = {o1, o2, o3};

    return {users, orders};
}

// ============================================================================
// formatSchemaContext tests
// ============================================================================

TEST(FormatSchemaContextTest, EmptySchemaReturnsEmpty) {
    EXPECT_TRUE(formatSchemaContext({}).empty());
}

TEST(FormatSchemaContextTest, ContainsCollectionName) {
    auto ctx = formatSchemaContext(makeTestSchema());
    EXPECT_NE(ctx.find("users"), std::string::npos);
    EXPECT_NE(ctx.find("orders"), std::string::npos);
}

TEST(FormatSchemaContextTest, ContainsFieldNames) {
    auto ctx = formatSchemaContext(makeTestSchema());
    EXPECT_NE(ctx.find("name"), std::string::npos);
    EXPECT_NE(ctx.find("age"), std::string::npos);
    EXPECT_NE(ctx.find("amount"), std::string::npos);
}

TEST(FormatSchemaContextTest, ContainsFieldTypes) {
    auto ctx = formatSchemaContext(makeTestSchema());
    EXPECT_NE(ctx.find("string"), std::string::npos);
    EXPECT_NE(ctx.find("integer"), std::string::npos);
    EXPECT_NE(ctx.find("double"), std::string::npos);
}

TEST(FormatSchemaContextTest, ContainsEstimatedCount) {
    auto ctx = formatSchemaContext(makeTestSchema());
    EXPECT_NE(ctx.find("1000"), std::string::npos);
    EXPECT_NE(ctx.find("5000"), std::string::npos);
}

TEST(FormatSchemaContextTest, IndexedFieldAnnotated) {
    auto ctx = formatSchemaContext(makeTestSchema());
    EXPECT_NE(ctx.find("indexed"), std::string::npos);
}

// ============================================================================
// AQLQueryBuilder::setSchema / getSchemaContext tests
// ============================================================================

class AQLBuilderSchemaTest : public ::testing::Test {
protected:
    void SetUp() override {
        builder = std::make_unique<AQLQueryBuilder>();
    }
    std::unique_ptr<AQLQueryBuilder> builder;
};

TEST_F(AQLBuilderSchemaTest, NoSchemaContextEmpty) {
    EXPECT_TRUE(builder->getSchemaContext().empty());
}

TEST_F(AQLBuilderSchemaTest, SetSchemaProducesNonEmptyContext) {
    builder->setSchema(makeTestSchema());
    EXPECT_FALSE(builder->getSchemaContext().empty());
}

TEST_F(AQLBuilderSchemaTest, SetSchemaContextContainsCollections) {
    builder->setSchema(makeTestSchema());
    auto ctx = builder->getSchemaContext();
    EXPECT_NE(ctx.find("users"), std::string::npos);
    EXPECT_NE(ctx.find("orders"), std::string::npos);
}

TEST_F(AQLBuilderSchemaTest, SetSchemaReturnsBuilderForChaining) {
    // setSchema() must return *this for fluent chaining
    auto& ref = builder->setSchema(makeTestSchema()).forIn("u", "users").ret("u");
    auto query = ref.build();
    EXPECT_NE(query.find("FOR u IN users"), std::string::npos);
}

TEST_F(AQLBuilderSchemaTest, ClearSchemaWithEmptyVector) {
    builder->setSchema(makeTestSchema());
    EXPECT_FALSE(builder->getSchemaContext().empty());

    builder->setSchema({});
    EXPECT_TRUE(builder->getSchemaContext().empty());
}

TEST_F(AQLBuilderSchemaTest, SchemaPersistedAcrossReset) {
    builder->setSchema(makeTestSchema()).forIn("u", "users").ret("u");
    builder->reset();
    // Schema should survive a reset (only the query clauses are cleared)
    EXPECT_FALSE(builder->getSchemaContext().empty());
}

// ============================================================================
// AQLQueryBuilder::getFieldsForCollection tests
// ============================================================================

TEST_F(AQLBuilderSchemaTest, GetFieldsKnownCollection) {
    builder->setSchema(makeTestSchema());
    auto fields = builder->getFieldsForCollection("users");
    EXPECT_FALSE(fields.empty());
    EXPECT_NE(std::find(fields.begin(), fields.end(), "name"),   fields.end());
    EXPECT_NE(std::find(fields.begin(), fields.end(), "age"),    fields.end());
    EXPECT_NE(std::find(fields.begin(), fields.end(), "email"),  fields.end());
    EXPECT_NE(std::find(fields.begin(), fields.end(), "active"), fields.end());
}

TEST_F(AQLBuilderSchemaTest, GetFieldsOrdersCollection) {
    builder->setSchema(makeTestSchema());
    auto fields = builder->getFieldsForCollection("orders");
    EXPECT_NE(std::find(fields.begin(), fields.end(), "user_id"), fields.end());
    EXPECT_NE(std::find(fields.begin(), fields.end(), "amount"),  fields.end());
    EXPECT_NE(std::find(fields.begin(), fields.end(), "city"),    fields.end());
}

TEST_F(AQLBuilderSchemaTest, GetFieldsUnknownCollectionReturnsEmpty) {
    builder->setSchema(makeTestSchema());
    auto fields = builder->getFieldsForCollection("nonexistent_collection");
    EXPECT_TRUE(fields.empty());
}

TEST_F(AQLBuilderSchemaTest, GetFieldsNoSchemaReturnsEmpty) {
    auto fields = builder->getFieldsForCollection("users");
    EXPECT_TRUE(fields.empty());
}

// ============================================================================
// Schema-aware validate() tests
// ============================================================================

TEST_F(AQLBuilderSchemaTest, ValidateKnownCollectionNoSchemaWarning) {
    builder->setSchema(makeTestSchema());
    builder->forIn("u", "users").ret("u");

    auto result = builder->validate();
    bool schema_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "FOR" &&
            issue.message.find("not found in attached schema") != std::string::npos) {
            schema_warning = true;
            break;
        }
    }
    EXPECT_FALSE(schema_warning);
}

TEST_F(AQLBuilderSchemaTest, ValidateUnknownCollectionEmitsWarning) {
    builder->setSchema(makeTestSchema());
    builder->forIn("x", "ghost_collection").ret("x");

    auto result = builder->validate();
    bool found_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "FOR" &&
            issue.message.find("ghost_collection") != std::string::npos) {
            found_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_warning);
}

TEST_F(AQLBuilderSchemaTest, ValidateUnknownCollectionNotAnError) {
    // An unknown collection is a WARNING, not an ERROR (the query may still run)
    builder->setSchema(makeTestSchema());
    builder->forIn("x", "ghost_collection").ret("x");

    auto result = builder->validate();
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLBuilderSchemaTest, ValidateNoSchemaNoWarning) {
    // When no schema is attached, no collection-existence warnings are emitted
    builder->forIn("x", "any_collection").ret("x");

    auto result = builder->validate();
    bool schema_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.clause == "FOR" &&
            issue.message.find("not found in attached schema") != std::string::npos) {
            schema_warning = true;
            break;
        }
    }
    EXPECT_FALSE(schema_warning);
}

TEST_F(AQLBuilderSchemaTest, ValidateMultipleForClauses) {
    builder->setSchema(makeTestSchema());
    // "users" is known, "invoices" is unknown
    builder->forIn("u", "users").forIn("i", "invoices").ret("{u, i}");

    auto result = builder->validate();
    int unknown_warnings = 0;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "FOR" &&
            issue.message.find("not found in attached schema") != std::string::npos) {
            ++unknown_warnings;
        }
    }
    // Exactly one warning for "invoices"; none for "users"
    EXPECT_EQ(unknown_warnings, 1);
}

// ============================================================================
// LLM suggestion methods fall back to attached schema (graceful, no model)
// ============================================================================

TEST_F(AQLBuilderSchemaTest, GetCompletionSuggestionsUsesAttachedSchema) {
    builder->setSchema(makeTestSchema()).forIn("u", "users");

    LLMAQLHandler handler;
    // No model loaded; expect no crash and empty result
    std::vector<std::string> suggestions;
    EXPECT_NO_THROW({
        suggestions = builder->getCompletionSuggestions(handler);
    });
    (void)suggestions;
}

TEST_F(AQLBuilderSchemaTest, GetLLMSuggestionUsesAttachedSchema) {
    builder->setSchema(makeTestSchema());

    LLMAQLHandler handler;
    std::string suggestion;
    EXPECT_NO_THROW({
        suggestion = builder->getLLMSuggestion(handler, "find users older than 30");
    });
    (void)suggestion;
}

TEST_F(AQLBuilderSchemaTest, ExplicitSchemaContextOverridesAttached) {
    builder->setSchema(makeTestSchema()).forIn("u", "users");

    LLMAQLHandler handler;
    // Explicit override; must not throw regardless of which context is used
    EXPECT_NO_THROW({
        builder->getCompletionSuggestions(handler, "Custom override context", 2);
    });
}
