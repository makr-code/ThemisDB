/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_query_builder.cpp                         ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 18:59:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     376                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_aql_query_builder.cpp
 * @brief Unit tests for AQLQueryBuilder (interactive AQL builder with LLM suggestions)
 */

#include <gtest/gtest.h>
#include "aql/aql_query_builder.h"
#include "aql/llm_aql_handler.h"

using namespace themis::aql;

// ============================================================================
// Test fixture
// ============================================================================

class AQLQueryBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        builder = std::make_unique<AQLQueryBuilder>();
    }

    void TearDown() override {
        builder.reset();
    }

    std::unique_ptr<AQLQueryBuilder> builder;
};

// ============================================================================
// Basic build tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, SimpleForReturn) {
    auto query = builder->forIn("doc", "users").ret("doc").build();
    EXPECT_NE(query.find("FOR doc IN users"), std::string::npos);
    EXPECT_NE(query.find("RETURN doc"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithFilter) {
    auto query = builder
        ->forIn("user", "users")
        .filter("user.age > 18")
        .ret("user")
        .build();
    EXPECT_NE(query.find("FILTER user.age > 18"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithSortAsc) {
    auto query = builder
        ->forIn("u", "users")
        .sort("u.name")
        .ret("u")
        .build();
    EXPECT_NE(query.find("SORT u.name ASC"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithSortDesc) {
    auto query = builder
        ->forIn("u", "users")
        .sort("u.age", false)
        .ret("u")
        .build();
    EXPECT_NE(query.find("SORT u.age DESC"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithLimit) {
    auto query = builder
        ->forIn("doc", "documents")
        .limit(10)
        .ret("doc")
        .build();
    EXPECT_NE(query.find("LIMIT 10"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithLimitAndOffset) {
    auto query = builder
        ->forIn("doc", "documents")
        .limit(10, 20)
        .ret("doc")
        .build();
    EXPECT_NE(query.find("LIMIT 20, 10"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithLet) {
    auto query = builder
        ->forIn("order", "orders")
        .let("total", "SUM(order.items[*].price)")
        .ret("{order, total}")
        .build();
    EXPECT_NE(query.find("LET total = SUM(order.items[*].price)"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForWithCollect) {
    auto query = builder
        ->forIn("order", "orders")
        .collect("city", "order.city")
        .ret("city")
        .build();
    EXPECT_NE(query.find("COLLECT city = order.city"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, MultipleFilters) {
    auto query = builder
        ->forIn("u", "users")
        .filter("u.age > 18")
        .filter("u.active == true")
        .ret("u")
        .build();
    EXPECT_NE(query.find("FILTER u.age > 18"), std::string::npos);
    EXPECT_NE(query.find("FILTER u.active == true"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, FullQuery) {
    auto query = builder
        ->forIn("user", "users")
        .filter("user.age >= 21")
        .sort("user.name")
        .limit(5)
        .ret("{name: user.name, age: user.age}")
        .build();

    EXPECT_NE(query.find("FOR user IN users"), std::string::npos);
    EXPECT_NE(query.find("FILTER user.age >= 21"), std::string::npos);
    EXPECT_NE(query.find("SORT user.name ASC"), std::string::npos);
    EXPECT_NE(query.find("LIMIT 5"), std::string::npos);
    EXPECT_NE(query.find("RETURN {name: user.name, age: user.age}"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ChainedFluentCalls) {
    // Ensure that every method returns *this for fluent chaining
    AQLQueryBuilder& ref = builder
        ->forIn("x", "col")
        .filter("x.val > 0")
        .sort("x.val")
        .limit(1)
        .ret("x");

    // The reference should be to the same builder
    auto query = ref.build();
    EXPECT_FALSE(query.empty());
}

// ============================================================================
// Reset tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, ResetClearsState) {
    builder->forIn("u", "users").filter("u.active == true").ret("u");
    EXPECT_TRUE(builder->isComplete());

    builder->reset();
    EXPECT_FALSE(builder->isComplete());
    EXPECT_TRUE(builder->getPartialQuery().empty());
}

TEST_F(AQLQueryBuilderTest, ReuseAfterReset) {
    builder->forIn("u", "users").ret("u");
    EXPECT_TRUE(builder->isComplete());

    builder->reset();
    auto query = builder->forIn("doc", "documents").ret("doc").build();
    EXPECT_NE(query.find("FOR doc IN documents"), std::string::npos);
    EXPECT_EQ(query.find("users"), std::string::npos);
}

// ============================================================================
// Partial query tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, PartialQueryBeforeReturn) {
    builder->forIn("u", "users").filter("u.age > 18");
    std::string partial = builder->getPartialQuery();
    EXPECT_NE(partial.find("FOR u IN users"), std::string::npos);
    EXPECT_NE(partial.find("FILTER u.age > 18"), std::string::npos);
    EXPECT_EQ(partial.find("RETURN"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, EmptyBuilderPartialQuery) {
    EXPECT_TRUE(builder->getPartialQuery().empty());
}

// ============================================================================
// Completeness / validity tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, IsCompleteRequiresForAndReturn) {
    EXPECT_FALSE(builder->isComplete());

    builder->forIn("u", "users");
    EXPECT_FALSE(builder->isComplete());

    builder->ret("u");
    EXPECT_TRUE(builder->isComplete());
}

TEST_F(AQLQueryBuilderTest, IsValidEmptyBuilder) {
    EXPECT_TRUE(builder->isValid());
}

TEST_F(AQLQueryBuilderTest, IsValidWithForClause) {
    builder->forIn("u", "users");
    EXPECT_TRUE(builder->isValid());
}

TEST_F(AQLQueryBuilderTest, BuildThrowsWithoutForClause) {
    builder->ret("doc");
    EXPECT_THROW(builder->build(), std::logic_error);
}

TEST_F(AQLQueryBuilderTest, BuildThrowsWithoutReturnClause) {
    builder->forIn("doc", "documents");
    EXPECT_THROW(builder->build(), std::logic_error);
}

// ============================================================================
// Input validation tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, ForInRejectsEmptyVariable) {
    EXPECT_THROW(builder->forIn("", "users"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForInRejectsEmptyCollection) {
    EXPECT_THROW(builder->forIn("u", ""), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, FilterRejectsEmptyCondition) {
    EXPECT_THROW(builder->filter(""), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, SortRejectsEmptyField) {
    EXPECT_THROW(builder->sort(""), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, LimitRejectsNegativeCount) {
    EXPECT_THROW(builder->limit(-1), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, LimitRejectsNegativeOffset) {
    EXPECT_THROW(builder->limit(10, -1), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, RetRejectsEmptyExpression) {
    EXPECT_THROW(builder->ret(""), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, LetRejectsEmptyVariable) {
    EXPECT_THROW(builder->let("", "expr"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, LetRejectsEmptyExpression) {
    EXPECT_THROW(builder->let("x", ""), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, CollectRejectsEmptyVariable) {
    EXPECT_THROW(builder->collect("", "field"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, CollectRejectsEmptyExpression) {
    EXPECT_THROW(builder->collect("city", ""), std::invalid_argument);
}

// ============================================================================
// Rule-based next steps tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, NextStepsEmptyBuilderRequiresFOR) {
    auto steps = builder->getNextSteps();
    ASSERT_FALSE(steps.empty());
    EXPECT_EQ(steps.front(), "FOR");
}

TEST_F(AQLQueryBuilderTest, NextStepsAfterFORIncludesReturnAndFilter) {
    builder->forIn("u", "users");
    auto steps = builder->getNextSteps();
    EXPECT_NE(std::find(steps.begin(), steps.end(), "RETURN"), steps.end());
    EXPECT_NE(std::find(steps.begin(), steps.end(), "FILTER"), steps.end());
}

TEST_F(AQLQueryBuilderTest, NextStepsAfterFORIncludesLIMIT) {
    builder->forIn("u", "users");
    auto steps = builder->getNextSteps();
    EXPECT_NE(std::find(steps.begin(), steps.end(), "LIMIT"), steps.end());
}

TEST_F(AQLQueryBuilderTest, NextStepsLIMITAbsentAfterLimitSet) {
    builder->forIn("u", "users").limit(10);
    auto steps = builder->getNextSteps();
    // LIMIT should not be suggested again once set
    EXPECT_EQ(std::find(steps.begin(), steps.end(), "LIMIT"), steps.end());
}

TEST_F(AQLQueryBuilderTest, NextStepsAlwaysAllowsNestedFOR) {
    builder->forIn("u", "users").ret("u");
    auto steps = builder->getNextSteps();
    EXPECT_NE(std::find(steps.begin(), steps.end(), "FOR"), steps.end());
}

// ============================================================================
// LLM suggestion tests (gracefully handle absent model)
// ============================================================================

TEST_F(AQLQueryBuilderTest, GetCompletionSuggestionsWithoutModel) {
    builder->forIn("user", "users").filter("user.active == true");

    LLMAQLHandler handler;
    // No model is loaded; we expect either an empty vector (graceful) or an exception.
    // The builder should NOT propagate LLM exceptions to the caller.
    std::vector<std::string> suggestions;
    EXPECT_NO_THROW({
        suggestions = builder->getCompletionSuggestions(handler);
    });
    // With no model loaded the list may be empty; that is acceptable.
    // The important thing is no crash or unhandled exception.
    (void)suggestions;
}

TEST_F(AQLQueryBuilderTest, GetLLMSuggestionWithoutModel) {
    LLMAQLHandler handler;
    std::string suggestion;
    EXPECT_NO_THROW({
        suggestion = builder->getLLMSuggestion(handler, "find all active users");
    });
    // With no model loaded the result may be empty; that is acceptable.
    (void)suggestion;
}

TEST_F(AQLQueryBuilderTest, GetLLMSuggestionWithSchemaContext) {
    std::string schema = "Collections:\n- users: {name, email, age, active}\n";
    LLMAQLHandler handler;
    std::string suggestion;
    EXPECT_NO_THROW({
        suggestion = builder->getLLMSuggestion(
            handler, "find users older than 30", schema
        );
    });
    (void)suggestion;
}

TEST_F(AQLQueryBuilderTest, GetCompletionSuggestionsWithSchemaAndPartialQuery) {
    builder->forIn("u", "users");
    std::string schema = "Collections:\n- users: {name, email, city}\n";

    LLMAQLHandler handler;
    std::vector<std::string> suggestions;
    EXPECT_NO_THROW({
        suggestions = builder->getCompletionSuggestions(handler, schema, 2);
    });
    (void)suggestions;
}
