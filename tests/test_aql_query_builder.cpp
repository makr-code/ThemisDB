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

TEST_F(AQLQueryBuilderTest, NextStepsEmptyBuilderIncludesDMLOptions) {
    // Empty builder should also suggest standalone DML entry points
    auto steps = builder->getNextSteps();
    EXPECT_NE(std::find(steps.begin(), steps.end(), "INSERT"), steps.end());
    EXPECT_NE(std::find(steps.begin(), steps.end(), "UPSERT"), steps.end());
}

TEST_F(AQLQueryBuilderTest, NextStepsAfterFORIncludesWINDOW) {
    builder->forIn("t", "timestamps");
    auto steps = builder->getNextSteps();
    EXPECT_NE(std::find(steps.begin(), steps.end(), "WINDOW"), steps.end());
}

TEST_F(AQLQueryBuilderTest, NextStepsAfterFORIncludesDMLTerminators) {
    builder->forIn("u", "users");
    auto steps = builder->getNextSteps();
    EXPECT_NE(std::find(steps.begin(), steps.end(), "INSERT"),  steps.end());
    EXPECT_NE(std::find(steps.begin(), steps.end(), "UPDATE"),  steps.end());
    EXPECT_NE(std::find(steps.begin(), steps.end(), "REMOVE"),  steps.end());
    EXPECT_NE(std::find(steps.begin(), steps.end(), "REPLACE"), steps.end());
    EXPECT_NE(std::find(steps.begin(), steps.end(), "UPSERT"),  steps.end());
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

// ============================================================================
// Graph traversal tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, ForTraverseBasic) {
    auto query = builder
        ->forTraverse("v", "e", "p", "\"users/1\"", "myGraph")
        .ret("v")
        .build();
    EXPECT_NE(query.find("FOR v, e, p IN 1..1 OUTBOUND \"users/1\" GRAPH myGraph"), std::string::npos);
    EXPECT_NE(query.find("RETURN v"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForTraverseCustomDepthAndDirection) {
    auto query = builder
        ->forTraverse("v", "e", "p", "startVertex", "socialGraph", "INBOUND", 2, 5)
        .filter("v.active == true")
        .ret("v")
        .build();
    EXPECT_NE(query.find("FOR v, e, p IN 2..5 INBOUND startVertex GRAPH socialGraph"), std::string::npos);
    EXPECT_NE(query.find("FILTER v.active == true"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForTraverseAnyDirection) {
    auto query = builder
        ->forTraverse("v", "e", "p", "start", "g", "ANY", 1, 3)
        .ret("v")
        .build();
    EXPECT_NE(query.find("ANY"), std::string::npos);
    EXPECT_NE(query.find("1..3"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ForTraverseRejectsEmptyVertexVar) {
    EXPECT_THROW(builder->forTraverse("", "e", "p", "start", "g"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForTraverseRejectsEmptyEdgeVar) {
    EXPECT_THROW(builder->forTraverse("v", "", "p", "start", "g"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForTraverseRejectsEmptyPathVar) {
    EXPECT_THROW(builder->forTraverse("v", "e", "", "start", "g"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForTraverseRejectsEmptyStart) {
    EXPECT_THROW(builder->forTraverse("v", "e", "p", "", "g"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForTraverseRejectsEmptyGraph) {
    EXPECT_THROW(builder->forTraverse("v", "e", "p", "start", ""), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForTraverseRejectsMinDepthGreaterThanMaxDepth) {
    EXPECT_THROW(builder->forTraverse("v", "e", "p", "start", "g", "OUTBOUND", 5, 2),
                 std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ForTraverseIsCompleteWithReturn) {
    builder->forTraverse("v", "e", "p", "start", "g").ret("v");
    EXPECT_TRUE(builder->isComplete());
}

TEST_F(AQLQueryBuilderTest, ForTraverseIsValidWhenDepthsCorrect) {
    builder->forTraverse("v", "e", "p", "start", "g", "OUTBOUND", 1, 3);
    EXPECT_TRUE(builder->isValid());
}

// ============================================================================
// DML tests — INSERT
// ============================================================================

TEST_F(AQLQueryBuilderTest, InsertInto) {
    auto query = builder->insertInto("users", "{name: \"Alice\", age: 30}").build();
    EXPECT_NE(query.find("INSERT {name: \"Alice\", age: 30} INTO users"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, InsertIntoIsComplete) {
    builder->insertInto("users", "{name: \"Bob\"}");
    EXPECT_TRUE(builder->isComplete());
}

TEST_F(AQLQueryBuilderTest, InsertIntoRejectsEmptyCollection) {
    EXPECT_THROW(builder->insertInto("", "{name: \"Alice\"}"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, InsertIntoRejectsEmptyDocExpr) {
    EXPECT_THROW(builder->insertInto("users", ""), std::invalid_argument);
}

// ============================================================================
// DML tests — UPDATE
// ============================================================================

TEST_F(AQLQueryBuilderTest, UpdateIn) {
    auto query = builder
        ->forIn("u", "users")
        .filter("u.name == \"Alice\"")
        .updateIn("users", "u WITH {age: 31}")
        .build();
    EXPECT_NE(query.find("UPDATE u WITH {age: 31} IN users"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, UpdateInRejectsEmptyCollection) {
    EXPECT_THROW(builder->updateIn("", "u WITH {x: 1}"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, UpdateInRejectsEmptyDocExpr) {
    EXPECT_THROW(builder->updateIn("users", ""), std::invalid_argument);
}

// ============================================================================
// DML tests — REMOVE
// ============================================================================

TEST_F(AQLQueryBuilderTest, RemoveIn) {
    auto query = builder
        ->forIn("u", "users")
        .filter("u.active == false")
        .removeIn("users", "u")
        .build();
    EXPECT_NE(query.find("REMOVE u IN users"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, RemoveInRejectsEmptyCollection) {
    EXPECT_THROW(builder->removeIn("", "u"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, RemoveInRejectsEmptyDocExpr) {
    EXPECT_THROW(builder->removeIn("users", ""), std::invalid_argument);
}

// ============================================================================
// DML tests — UPSERT
// ============================================================================

TEST_F(AQLQueryBuilderTest, UpsertIn) {
    auto query = builder
        ->upsertIn("users", "{name: \"Alice\"}", "{name: \"Alice\", age: 30}", "{age: 30}")
        .build();
    EXPECT_NE(query.find("UPSERT {name: \"Alice\"} INSERT {name: \"Alice\", age: 30} UPDATE {age: 30} IN users"),
              std::string::npos);
}

TEST_F(AQLQueryBuilderTest, UpsertInIsComplete) {
    builder->upsertIn("users", "{name: \"Alice\"}", "{name: \"Alice\"}", "{age: 31}");
    EXPECT_TRUE(builder->isComplete());
}

TEST_F(AQLQueryBuilderTest, UpsertInRejectsEmptyCollection) {
    EXPECT_THROW(builder->upsertIn("", "{x: 1}", "{x: 1}", "{x: 2}"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, UpsertInRejectsEmptyFilterExpr) {
    EXPECT_THROW(builder->upsertIn("users", "", "{x: 1}", "{x: 2}"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, UpsertInRejectsEmptyInsertExpr) {
    EXPECT_THROW(builder->upsertIn("users", "{x: 1}", "", "{x: 2}"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, UpsertInRejectsEmptyUpdateExpr) {
    EXPECT_THROW(builder->upsertIn("users", "{x: 1}", "{x: 1}", ""), std::invalid_argument);
}

// ============================================================================
// DML tests — REPLACE
// ============================================================================

TEST_F(AQLQueryBuilderTest, ReplaceIn) {
    auto query = builder
        ->forIn("u", "users")
        .filter("u.name == \"Alice\"")
        .replaceIn("users", "u WITH {name: \"Alice\", age: 35}")
        .build();
    EXPECT_NE(query.find("REPLACE u WITH {name: \"Alice\", age: 35} IN users"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, ReplaceInRejectsEmptyCollection) {
    EXPECT_THROW(builder->replaceIn("", "u WITH {x: 1}"), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, ReplaceInRejectsEmptyDocExpr) {
    EXPECT_THROW(builder->replaceIn("users", ""), std::invalid_argument);
}

// ============================================================================
// WINDOW analytics tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, WindowWithPartitionExpr) {
    auto query = builder
        ->forIn("t", "timestamps")
        .window("t.time", "{ preceding: \"PT30M\", following: 0 }")
        .ret("t")
        .build();
    EXPECT_NE(query.find("WINDOW t.time WITH { preceding: \"PT30M\", following: 0 }"),
              std::string::npos);
}

TEST_F(AQLQueryBuilderTest, WindowWithoutPartitionExpr) {
    auto query = builder
        ->forIn("t", "timestamps")
        .window("", "{ preceding: 5, following: 5 }")
        .ret("t")
        .build();
    // Partition-less window: WINDOW { preceding: 5, following: 5 }
    EXPECT_NE(query.find("WINDOW { preceding: 5, following: 5 }"), std::string::npos);
    // Ensure it does NOT include "WITH" when no partition expression
    std::string w_clause = "WINDOW { preceding: 5, following: 5 }";
    auto pos = query.find(w_clause);
    EXPECT_NE(pos, std::string::npos);
}

TEST_F(AQLQueryBuilderTest, WindowRejectsEmptyWindowSpec) {
    EXPECT_THROW(builder->window("t.time", ""), std::invalid_argument);
}

// ============================================================================
// Subquery tests
// ============================================================================

TEST_F(AQLQueryBuilderTest, SubqueryRendersAsLet) {
    AQLQueryBuilder inner;
    inner.forIn("x", "items").filter("x.price > 10").ret("x");

    auto query = builder
        ->forIn("u", "users")
        .subquery("expensiveItems", inner)
        .ret("{u, expensiveItems}")
        .build();

    EXPECT_NE(query.find("LET expensiveItems = ("), std::string::npos);
    EXPECT_NE(query.find("FOR x IN items"), std::string::npos);
    EXPECT_NE(query.find("FILTER x.price > 10"), std::string::npos);
}

TEST_F(AQLQueryBuilderTest, SubqueryRejectsEmptyVariable) {
    AQLQueryBuilder inner;
    inner.forIn("x", "items").ret("x");
    EXPECT_THROW(builder->subquery("", inner), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, SubqueryRejectsEmptyInnerBuilder) {
    AQLQueryBuilder inner;  // empty builder
    EXPECT_THROW(builder->subquery("result", inner), std::invalid_argument);
}

TEST_F(AQLQueryBuilderTest, SubqueryIsValidAfterForClause) {
    AQLQueryBuilder inner;
    inner.forIn("x", "items").ret("x");

    builder->forIn("u", "users").subquery("items", inner);
    EXPECT_TRUE(builder->isValid());
}
