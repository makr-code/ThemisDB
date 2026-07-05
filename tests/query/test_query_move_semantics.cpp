/**
 * @file test_query_move_semantics.cpp
 * @brief Tests for Query module move semantics (QueryPlanNode hierarchy)
 * @version 1.0.0
 * @date 2026-07-05
 */

#include <gtest/gtest.h>
#include "query/query_plan_node_move.h"
#include <memory>
#include <utility>

namespace themis {
namespace query {

class QueryModuleMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// QueryPlanNode Base Class Tests (Gap 2.1)
// ============================================================================

TEST_F(QueryModuleMoveTest, QueryPlanNode_MoveConstruction) {
    // Create and move construct
    auto node1 = std::make_unique<SelectNode>();
    node1->addColumn("col1");
    
    SelectNode node2(std::move(*node1));
    
    // Verify node2 is valid
    EXPECT_EQ(node2.getNodeType(), "SELECT");
    
    // Verify node1 is in valid empty state
    EXPECT_TRUE(node1->getNodeType().empty());
    EXPECT_EQ(node1->getChildCount(), 0);
}

TEST_F(QueryModuleMoveTest, QueryPlanNode_MoveAssignment) {
    auto node1 = std::make_unique<SelectNode>();
    auto node2 = std::make_unique<SelectNode>();
    
    *node2 = std::move(*node1);
    
    // Verify node2 has valid state
    EXPECT_EQ(node2->getNodeType(), "SELECT");
    
    // Verify node1 is empty
    EXPECT_TRUE(node1->getNodeType().empty());
}

TEST_F(QueryModuleMoveTest, QueryPlanNode_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<QueryPlanNode>,
                  "QueryPlanNode should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<QueryPlanNode>,
                  "QueryPlanNode should not be copy assignable");
}

TEST_F(QueryModuleMoveTest, QueryPlanNode_IsMove) {
    static_assert(std::is_move_constructible_v<QueryPlanNode>,
                  "QueryPlanNode should be move constructible");
    static_assert(std::is_move_assignable_v<QueryPlanNode>,
                  "QueryPlanNode should be move assignable");
}

// ============================================================================
// SelectNode Tests (Gap 2.2)
// ============================================================================

TEST_F(QueryModuleMoveTest, SelectNode_MoveConstruction) {
    auto node1 = std::make_unique<SelectNode>();
    node1->addColumn("id");
    node1->addColumn("name");
    node1->addPredicate("age > 18");
    
    SelectNode node2(std::move(*node1));
    
    // Verify data transferred
    EXPECT_EQ(node2.getColumns().size(), 2);
    EXPECT_EQ(node2.getPredicates().size(), 1);
    
    // Verify source is empty
    EXPECT_EQ(node1->getColumns().size(), 0);
    EXPECT_EQ(node1->getPredicates().size(), 0);
}

TEST_F(QueryModuleMoveTest, SelectNode_MoveAssignment) {
    auto node1 = std::make_unique<SelectNode>();
    node1->addColumn("col1");
    
    auto node2 = std::make_unique<SelectNode>();
    *node2 = std::move(*node1);
    
    EXPECT_EQ(node2->getColumns().size(), 1);
    EXPECT_EQ(node1->getColumns().size(), 0);
}

// ============================================================================
// JoinNode Tests (Gap 2.3)
// ============================================================================

TEST_F(QueryModuleMoveTest, JoinNode_MoveConstruction) {
    auto node1 = std::make_unique<JoinNode>();
    node1->setJoinType("INNER");
    node1->addJoinCondition("t1.id = t2.id");
    
    JoinNode node2(std::move(*node1));
    
    // Verify data transferred
    EXPECT_EQ(node2.getJoinType(), "INNER");
    EXPECT_EQ(node2.getJoinConditions().size(), 1);
    
    // Verify source is empty
    EXPECT_TRUE(node1->getJoinType().empty());
    EXPECT_EQ(node1->getJoinConditions().size(), 0);
}

TEST_F(QueryModuleMoveTest, JoinNode_MoveAssignment) {
    auto node1 = std::make_unique<JoinNode>();
    node1->setJoinType("LEFT");
    
    auto node2 = std::make_unique<JoinNode>();
    *node2 = std::move(*node1);
    
    EXPECT_EQ(node2->getJoinType(), "LEFT");
    EXPECT_TRUE(node1->getJoinType().empty());
}

// ============================================================================
// AggregateNode Tests (Gap 2.4)
// ============================================================================

TEST_F(QueryModuleMoveTest, AggregateNode_MoveConstruction) {
    auto node1 = std::make_unique<AggregateNode>();
    node1->addGroupExpression("category");
    node1->addAggregateFunction("COUNT(*)");
    
    AggregateNode node2(std::move(*node1));
    
    // Verify data transferred
    EXPECT_EQ(node2.getGroupExpressions().size(), 1);
    EXPECT_EQ(node2.getAggregateFunctions().size(), 1);
    
    // Verify source is empty
    EXPECT_EQ(node1->getGroupExpressions().size(), 0);
    EXPECT_EQ(node1->getAggregateFunctions().size(), 0);
}

TEST_F(QueryModuleMoveTest, AggregateNode_MoveAssignment) {
    auto node1 = std::make_unique<AggregateNode>();
    node1->addGroupExpression("dept");
    
    auto node2 = std::make_unique<AggregateNode>();
    *node2 = std::move(*node1);
    
    EXPECT_EQ(node2->getGroupExpressions().size(), 1);
    EXPECT_EQ(node1->getGroupExpressions().size(), 0);
}

// ============================================================================
// FilterNode Tests (Gap 2.5)
// ============================================================================

TEST_F(QueryModuleMoveTest, FilterNode_MoveConstruction) {
    auto node1 = std::make_unique<FilterNode>();
    node1->addFilterExpression("status = 'active'");
    node1->addFilterExpression("date > '2023-01-01'");
    
    FilterNode node2(std::move(*node1));
    
    // Verify data transferred
    EXPECT_EQ(node2.getFilterExpressions().size(), 2);
    
    // Verify source is empty
    EXPECT_EQ(node1->getFilterExpressions().size(), 0);
}

TEST_F(QueryModuleMoveTest, FilterNode_MoveAssignment) {
    auto node1 = std::make_unique<FilterNode>();
    node1->addFilterExpression("price < 100");
    
    auto node2 = std::make_unique<FilterNode>();
    *node2 = std::move(*node1);
    
    EXPECT_EQ(node2->getFilterExpressions().size(), 1);
    EXPECT_EQ(node1->getFilterExpressions().size(), 0);
}

// ============================================================================
// Tree Operation Tests
// ============================================================================

TEST_F(QueryModuleMoveTest, QueryPlanTree_MoveConstruction) {
    // Create parent with children
    auto parent = std::make_unique<SelectNode>();
    auto child1 = std::make_unique<FilterNode>();
    auto child2 = std::make_unique<JoinNode>();
    
    child1->addFilterExpression("x > 0");
    child2->setJoinType("LEFT");
    
    parent->addChild(std::move(child1));
    parent->addChild(std::move(child2));
    
    EXPECT_EQ(parent->getChildCount(), 2);
    
    // Move parent
    SelectNode moved_parent(std::move(*parent));
    
    // Verify children moved with parent
    EXPECT_EQ(moved_parent.getChildCount(), 2);
    EXPECT_EQ(parent->getChildCount(), 0);
}

// ============================================================================
// Exception Safety Tests
// ============================================================================

TEST_F(QueryModuleMoveTest, QueryNode_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<QueryPlanNode>,
                  "QueryPlanNode move constructor must be noexcept");
    static_assert(std::is_nothrow_move_assignable_v<QueryPlanNode>,
                  "QueryPlanNode move assignment must be noexcept");
    
    static_assert(std::is_nothrow_move_constructible_v<SelectNode>,
                  "SelectNode move constructor must be noexcept");
    static_assert(std::is_nothrow_move_assignable_v<SelectNode>,
                  "SelectNode move assignment must be noexcept");
}

}  // namespace query
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
