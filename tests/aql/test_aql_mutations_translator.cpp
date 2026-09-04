/**
 * @file test_aql_mutations_translator.cpp
 * @brief Unit tests for EPIC-004 Phase 3: AqlMutationTranslator.
 *
 * Verifies that MutationNode AST nodes are correctly translated into
 * MutationExecutionPlan instances with the expected step sequences,
 * latency estimates, and transaction flags.
 */

#include <gtest/gtest.h>
#include "query/aql_translator.h"
#include "query/mutation_execution_plan.h"

using namespace themis::query;
using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static std::shared_ptr<Expression> makeExpr() {
    return std::make_shared<VariableExpr>("doc");
}

static bool hasSteType(const MutationExecutionPlan& plan, MutationStepType t) {
    for (const auto& s : plan.steps)
        if (s.type == t) {
          return true;
        }
    return false;
}

static bool stepsContainInOrder(const MutationExecutionPlan& plan,
                                 std::initializer_list<MutationStepType> expected)
{
    std::size_t idx = 0;
    for (const auto& s : plan.steps) {
        if (idx < expected.size() && s.type == *(expected.begin() + idx))
            ++idx;
    }
    return idx == expected.size();
}

// ============================================================================
// TranslateInsert — 8 cases
// ============================================================================

TEST(AqlMutationTranslator, InsertPlanHasCorrectMutationType) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "users";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_EQ(plan.mutation_type, ASTNodeType::Insert);
    EXPECT_EQ(plan.collection, "users");
}

TEST(AqlMutationTranslator, InsertPlanHasAcquireLockFirst) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "users";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    ASSERT_FALSE(plan.steps.empty());
    EXPECT_EQ(plan.steps.front().type, MutationStepType::AcquireLock);
}

TEST(AqlMutationTranslator, InsertPlanHasReleaseLockLast) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "users";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    ASSERT_FALSE(plan.steps.empty());
    EXPECT_EQ(plan.steps.back().type, MutationStepType::ReleaseLock);
}

TEST(AqlMutationTranslator, InsertPlanHasGenerateKeys) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "items";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::GenerateKeys));
}

TEST(AqlMutationTranslator, InsertPlanHasSerialize) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "items";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::Serialize));
}

TEST(AqlMutationTranslator, InsertPlanHasWriteWAL) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "items";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::WriteWAL));
}

TEST(AqlMutationTranslator, InsertPlanHasRocksDbPut) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "items";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::RocksDbPut));
}

TEST(AqlMutationTranslator, InsertPlanStepsCorrectOrder) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "items";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_TRUE(stepsContainInOrder(plan, {
        MutationStepType::AcquireLock,
        MutationStepType::GenerateKeys,
        MutationStepType::Serialize,
        MutationStepType::WriteWAL,
        MutationStepType::RocksDbPut,
        MutationStepType::UpdateIndexes,
        MutationStepType::ReleaseLock
    }));
}

// ============================================================================
// TranslateUpdate — 5 cases
// ============================================================================

TEST(AqlMutationTranslator, UpdatePlanHasValidatePredicateWhenFilterPresent) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpdateNode>();
    node->collection = "users";
    node->filter     = makeExpr();
    SetClause sc; sc.field = "name"; sc.value = makeExpr();
    node->set_clauses.push_back(sc);
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::ValidatePredicate));
}

TEST(AqlMutationTranslator, UpdatePlanNoValidatePredicateWhenNoFilter) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpdateNode>();
    node->collection = "users";
    node->update_expr = makeExpr();
    // No filter, no search_expr
    auto plan = t.translate(node);
    EXPECT_FALSE(hasSteType(plan, MutationStepType::ValidatePredicate));
}

TEST(AqlMutationTranslator, UpdatePlanHasRocksDbPut) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpdateNode>();
    node->collection  = "products";
    node->update_expr = makeExpr();
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::RocksDbPut));
}

TEST(AqlMutationTranslator, UpdatePlanAffectedLimitSet) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpdateNode>();
    node->collection  = "orders";
    node->update_expr = makeExpr();
    node->limit       = 50;
    auto plan = t.translate(node);
    ASSERT_TRUE(plan.affected_limit.has_value());
    EXPECT_EQ(*plan.affected_limit, 50);
}

TEST(AqlMutationTranslator, UpdateRequiresTransactionWhenReturnNew) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpdateNode>();
    node->collection  = "items";
    node->update_expr = makeExpr();
    node->return_new  = true;
    auto plan = t.translate(node);
    EXPECT_TRUE(plan.requires_transaction);
}

// ============================================================================
// TranslateRemove — 5 cases
// ============================================================================

TEST(AqlMutationTranslator, RemovePlanHasRocksDbDelete) {
    AqlMutationTranslator t;
    auto node = std::make_shared<RemoveNode>();
    node->collection = "logs";
    node->filter     = makeExpr();
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::RocksDbDelete));
}

TEST(AqlMutationTranslator, RemovePlanHasValidatePredicateWhenFilterPresent) {
    AqlMutationTranslator t;
    auto node = std::make_shared<RemoveNode>();
    node->collection = "logs";
    node->filter     = makeExpr();
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::ValidatePredicate));
}

TEST(AqlMutationTranslator, RemovePlanNoValidatePredicateWithoutFilter) {
    AqlMutationTranslator t;
    auto node = std::make_shared<RemoveNode>();
    node->collection = "logs";
    auto plan = t.translate(node);
    EXPECT_FALSE(hasSteType(plan, MutationStepType::ValidatePredicate));
}

TEST(AqlMutationTranslator, RemovePlanAffectedLimitFromLimitNode) {
    AqlMutationTranslator t;
    auto node = std::make_shared<RemoveNode>();
    node->collection = "logs";
    node->filter     = makeExpr();
    node->limit      = 100;
    auto plan = t.translate(node);
    ASSERT_TRUE(plan.affected_limit.has_value());
    EXPECT_EQ(*plan.affected_limit, 100);
}

TEST(AqlMutationTranslator, RemovePlanHasCorrectMutationType) {
    AqlMutationTranslator t;
    auto node = std::make_shared<RemoveNode>();
    node->collection = "archive";
    auto plan = t.translate(node);
    EXPECT_EQ(plan.mutation_type, ASTNodeType::Remove);
}

// ============================================================================
// TranslateReplace — 4 cases
// ============================================================================

TEST(AqlMutationTranslator, ReplacePlanHasSerializeAndPut) {
    AqlMutationTranslator t;
    auto node = std::make_shared<ReplaceNode>();
    node->collection  = "orders";
    node->search_expr = makeExpr();
    node->replacement = makeExpr();
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::Serialize));
    EXPECT_TRUE(hasSteType(plan, MutationStepType::RocksDbPut));
}

TEST(AqlMutationTranslator, ReplacePlanCorrectMutationType) {
    AqlMutationTranslator t;
    auto node = std::make_shared<ReplaceNode>();
    node->collection = "orders";
    auto plan = t.translate(node);
    EXPECT_EQ(plan.mutation_type, ASTNodeType::Replace);
}

TEST(AqlMutationTranslator, ReplacePlanRequiresTransactionWithReturnOld) {
    AqlMutationTranslator t;
    auto node = std::make_shared<ReplaceNode>();
    node->collection = "orders";
    node->return_old = true;
    auto plan = t.translate(node);
    EXPECT_TRUE(plan.requires_transaction);
}

TEST(AqlMutationTranslator, ReplacePlanNoTransactionByDefault) {
    AqlMutationTranslator t;
    auto node = std::make_shared<ReplaceNode>();
    node->collection = "orders";
    auto plan = t.translate(node);
    EXPECT_FALSE(plan.requires_transaction);
}

// ============================================================================
// TranslateUpsert — 4 cases
// ============================================================================

TEST(AqlMutationTranslator, UpsertPlanHasValidatePredicateForSearchExpr) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpsertNode>();
    node->collection  = "products";
    node->search_expr = makeExpr();
    node->insert_doc  = makeExpr();
    node->update_doc  = makeExpr();
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::ValidatePredicate));
}

TEST(AqlMutationTranslator, UpsertPlanHasGenerateKeys) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpsertNode>();
    node->collection  = "products";
    node->search_expr = makeExpr();
    auto plan = t.translate(node);
    EXPECT_TRUE(hasSteType(plan, MutationStepType::GenerateKeys));
}

TEST(AqlMutationTranslator, UpsertPlanCorrectMutationType) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpsertNode>();
    node->collection = "catalog";
    auto plan = t.translate(node);
    EXPECT_EQ(plan.mutation_type, ASTNodeType::Upsert);
}

TEST(AqlMutationTranslator, UpsertPlanRequiresTransactionWithReturnNew) {
    AqlMutationTranslator t;
    auto node = std::make_shared<UpsertNode>();
    node->collection = "catalog";
    node->return_new = true;
    auto plan = t.translate(node);
    EXPECT_TRUE(plan.requires_transaction);
}

// ============================================================================
// Null node — 2 cases
// ============================================================================

TEST(AqlMutationTranslator, NullNodeReturnsErrorPlan) {
    AqlMutationTranslator t;
    auto plan = t.translate(nullptr);
    EXPECT_TRUE(plan.collection.empty());
}

TEST(AqlMutationTranslator, NullNodePlanHasValidatePredicateStep) {
    AqlMutationTranslator t;
    auto plan = t.translate(nullptr);
    ASSERT_FALSE(plan.steps.empty());
    EXPECT_EQ(plan.steps.front().type, MutationStepType::ValidatePredicate);
}

// ============================================================================
// estimated_latency_ms — 2 cases
// ============================================================================

TEST(AqlMutationTranslator, InsertLatencyIsPositive) {
    AqlMutationTranslator t;
    auto node = std::make_shared<InsertNode>();
    node->collection = "x";
    node->documents.push_back(makeExpr());
    auto plan = t.translate(node);
    EXPECT_GT(plan.estimated_latency_ms, 0);
}

TEST(AqlMutationTranslator, UpsertLatencyGreaterThanInsert) {
    AqlMutationTranslator t;

    auto ins = std::make_shared<InsertNode>();
    ins->collection = "x";
    ins->documents.push_back(makeExpr());

    auto ups = std::make_shared<UpsertNode>();
    ups->collection = "x";

    auto insertPlan = t.translate(ins);
    auto upsertPlan = t.translate(ups);

    EXPECT_GE(upsertPlan.estimated_latency_ms, insertPlan.estimated_latency_ms);
}
