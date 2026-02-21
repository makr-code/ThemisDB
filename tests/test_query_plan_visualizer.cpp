// Tests for the Query Plan Visualization API (EXPLAIN / EXPLAIN ANALYZE)

#include <gtest/gtest.h>
#include <string>

#include "query/query_plan_visualizer.h"

using namespace themis;
using namespace themis::query;

// ============================================================================
// Helper: build a simple plan fixture
// ============================================================================

static QueryPlanNode makeSamplePlan() {
    // Root: Return
    QueryPlanNode root;
    root.type = PlanNodeType::Return;
    root.description = "Return";
    root.estimated_cost = 260.0;
    root.estimated_rows = 10;
    root.actual_time_ms = 2.5;
    root.actual_rows = 8;

    // Child 1: Filter
    auto filter = std::make_shared<QueryPlanNode>();
    filter->type = PlanNodeType::Filter;
    filter->description = "age == 30";
    filter->estimated_cost = 60.0;
    filter->estimated_rows = 100;
    filter->selectivity = 0.1;
    filter->attributes.push_back("age");
    filter->actual_time_ms = 1.0;
    filter->actual_rows = 10;

    // Child 2: IndexScan
    auto scan = std::make_shared<QueryPlanNode>();
    scan->type = PlanNodeType::IndexScan;
    scan->description = "IndexScan on users";
    scan->estimated_cost = 200.0;
    scan->estimated_rows = 1000;
    scan->index_name = "age_idx";
    scan->actual_time_ms = 0.8;
    scan->actual_rows = 1000;

    filter->children.push_back(scan);
    root.children.push_back(filter);
    return root;
}

// ============================================================================
// Tests: buildPlan
// ============================================================================

TEST(QueryPlanVisualizerTest, BuildPlan_EmptyPredicates) {
    ConjunctiveQuery q;
    q.table = "items";

    QueryOptimizer::Plan plan;  // no predicates

    QueryPlanNode root = QueryPlanVisualizer::buildPlan(q, plan);

    // Without predicates we should get a Return -> SeqScan tree
    EXPECT_EQ(root.type, PlanNodeType::Return);
    ASSERT_FALSE(root.children.empty());
    EXPECT_EQ(root.children.front()->type, PlanNodeType::SeqScan);
}

TEST(QueryPlanVisualizerTest, BuildPlan_WithPredicates_UsesIndexScan) {
    ConjunctiveQuery q;
    q.table = "users";
    q.predicates.push_back({"city", "Berlin"});
    q.predicates.push_back({"age", "30"});

    QueryOptimizer::Plan plan;
    plan.orderedPredicates = {{"city", "Berlin"}, {"age", "30"}};
    QueryOptimizer::Estimation e1; e1.pred = {"city", "Berlin"}; e1.estimatedCount = 200;
    QueryOptimizer::Estimation e2; e2.pred = {"age", "30"};       e2.estimatedCount = 50;
    plan.details = {e1, e2};

    QueryPlanNode root = QueryPlanVisualizer::buildPlan(q, plan);

    EXPECT_EQ(root.type, PlanNodeType::Return);
    // Should have at least one Filter child
    ASSERT_FALSE(root.children.empty());
    EXPECT_EQ(root.children.front()->type, PlanNodeType::Filter);

    // Leaf should be an IndexScan (because predicates are present)
    // Walk to the leaf
    const QueryPlanNode* node = &root;
    while (!node->children.empty()) {
        node = node->children.front().get();
    }
    EXPECT_EQ(node->type, PlanNodeType::IndexScan);
    EXPECT_TRUE(node->index_name.has_value());
}

TEST(QueryPlanVisualizerTest, BuildPlan_TotalCostPositive) {
    ConjunctiveQuery q;
    q.table = "orders";
    q.predicates.push_back({"status", "paid"});

    QueryOptimizer::Plan plan;
    plan.orderedPredicates = {{"status", "paid"}};
    QueryOptimizer::Estimation e; e.pred = {"status", "paid"}; e.estimatedCount = 300;
    plan.details = {e};

    QueryPlanNode root = QueryPlanVisualizer::buildPlan(q, plan);
    EXPECT_GT(root.estimated_cost, 0.0);
}

// ============================================================================
// Tests: toText
// ============================================================================

TEST(QueryPlanVisualizerTest, ToText_ContainsExplainHeader) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_NE(text.find("EXPLAIN"), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToText_ContainsNodeTypes) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_NE(text.find("Return"), std::string::npos);
    EXPECT_NE(text.find("Filter"), std::string::npos);
    EXPECT_NE(text.find("IndexScan"), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToText_ContainsCostAndRows) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_NE(text.find("cost="), std::string::npos);
    EXPECT_NE(text.find("rows="), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToText_AnalyzeMode_ContainsActualStats) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, true);
    EXPECT_NE(text.find("EXPLAIN ANALYZE"), std::string::npos);
    EXPECT_NE(text.find("actual time="), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToText_NoAnalyze_NoActualStats) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_EQ(text.find("actual time="), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToText_ContainsSummaryLine) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_NE(text.find("Estimated total cost:"), std::string::npos);
    EXPECT_NE(text.find("Estimated rows:"), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToText_ContainsIndexName) {
    auto root = makeSamplePlan();
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_NE(text.find("age_idx"), std::string::npos);
}

// ============================================================================
// Tests: toJSON
// ============================================================================

TEST(QueryPlanVisualizerTest, ToJSON_TopLevelMode) {
    auto root = makeSamplePlan();
    auto j = QueryPlanVisualizer::toJSON(root, false);
    ASSERT_TRUE(j.contains("mode"));
    EXPECT_EQ(j["mode"].get<std::string>(), "EXPLAIN");
}

TEST(QueryPlanVisualizerTest, ToJSON_AnalyzeMode) {
    auto root = makeSamplePlan();
    auto j = QueryPlanVisualizer::toJSON(root, true);
    ASSERT_TRUE(j.contains("mode"));
    EXPECT_EQ(j["mode"].get<std::string>(), "EXPLAIN ANALYZE");
}

TEST(QueryPlanVisualizerTest, ToJSON_PlanStructure) {
    auto root = makeSamplePlan();
    auto j = QueryPlanVisualizer::toJSON(root, false);
    ASSERT_TRUE(j.contains("plan"));
    const auto& plan = j["plan"];
    EXPECT_EQ(plan["type"].get<std::string>(), "Return");
    ASSERT_TRUE(plan.contains("estimated_cost"));
    ASSERT_TRUE(plan.contains("estimated_rows"));
    ASSERT_TRUE(plan.contains("children"));
    EXPECT_FALSE(plan["children"].empty());
}

TEST(QueryPlanVisualizerTest, ToJSON_NestedChildren) {
    auto root = makeSamplePlan();
    auto j = QueryPlanVisualizer::toJSON(root, false);
    const auto& filter_j = j["plan"]["children"][0];
    EXPECT_EQ(filter_j["type"].get<std::string>(), "Filter");
    const auto& scan_j = filter_j["children"][0];
    EXPECT_EQ(scan_j["type"].get<std::string>(), "IndexScan");
    EXPECT_TRUE(scan_j.contains("index"));
    EXPECT_EQ(scan_j["index"].get<std::string>(), "age_idx");
}

TEST(QueryPlanVisualizerTest, ToJSON_Analyze_IncludesActualStats) {
    auto root = makeSamplePlan();
    auto j = QueryPlanVisualizer::toJSON(root, true);
    const auto& plan = j["plan"];
    EXPECT_TRUE(plan.contains("actual_time_ms"));
    EXPECT_TRUE(plan.contains("actual_rows"));
    EXPECT_NEAR(plan["actual_time_ms"].get<double>(), 2.5, 1e-9);
    EXPECT_EQ(plan["actual_rows"].get<size_t>(), 8u);
}

TEST(QueryPlanVisualizerTest, ToJSON_NoAnalyze_NoActualStats) {
    auto root = makeSamplePlan();
    auto j = QueryPlanVisualizer::toJSON(root, false);
    const auto& plan = j["plan"];
    EXPECT_FALSE(plan.contains("actual_time_ms"));
    EXPECT_FALSE(plan.contains("actual_rows"));
}

// ============================================================================
// Tests: toDOT
// ============================================================================

TEST(QueryPlanVisualizerTest, ToDOT_IsValidDigraph) {
    auto root = makeSamplePlan();
    std::string dot = QueryPlanVisualizer::toDOT(root);
    EXPECT_NE(dot.find("digraph QueryPlan"), std::string::npos);
    EXPECT_NE(dot.find("rankdir=TB"), std::string::npos);
    EXPECT_NE(dot.find("->"), std::string::npos);
    // Ends with closing brace
    EXPECT_NE(dot.rfind('}'), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToDOT_ContainsAllNodeLabels) {
    auto root = makeSamplePlan();
    std::string dot = QueryPlanVisualizer::toDOT(root);
    EXPECT_NE(dot.find("Return"), std::string::npos);
    EXPECT_NE(dot.find("Filter"), std::string::npos);
    EXPECT_NE(dot.find("IndexScan"), std::string::npos);
}

TEST(QueryPlanVisualizerTest, ToDOT_EmptyPlan_ValidOutput) {
    QueryPlanNode leaf;
    leaf.type = PlanNodeType::SeqScan;
    leaf.description = "SeqScan on test";
    leaf.estimated_cost = 100.0;
    leaf.estimated_rows = 500;

    std::string dot = QueryPlanVisualizer::toDOT(leaf);
    EXPECT_NE(dot.find("digraph"), std::string::npos);
    EXPECT_NE(dot.find("SeqScan"), std::string::npos);
}

// ============================================================================
// Tests: planNodeTypeName (via round-trip through toText/toJSON)
// ============================================================================

TEST(QueryPlanVisualizerTest, AllNodeTypes_NamedCorrectly) {
    const std::vector<std::pair<PlanNodeType, std::string>> cases = {
        {PlanNodeType::SeqScan,         "SeqScan"},
        {PlanNodeType::IndexScan,       "IndexScan"},
        {PlanNodeType::Filter,          "Filter"},
        {PlanNodeType::Sort,            "Sort"},
        {PlanNodeType::Limit,           "Limit"},
        {PlanNodeType::Return,          "Return"},
        {PlanNodeType::Aggregate,       "Aggregate"},
        {PlanNodeType::HashJoin,        "HashJoin"},
        {PlanNodeType::NestedLoopJoin,  "NestedLoopJoin"},
        {PlanNodeType::GraphTraversal,  "GraphTraversal"},
        {PlanNodeType::VectorSearch,    "VectorSearch"},
        {PlanNodeType::SpatialFilter,   "SpatialFilter"},
        {PlanNodeType::CTE,             "CTE"},
        {PlanNodeType::Subquery,        "Subquery"},
        {PlanNodeType::Unknown,         "Unknown"},
    };

    for (const auto& [type, expected_name] : cases) {
        QueryPlanNode node;
        node.type = type;
        node.description = expected_name;
        node.estimated_cost = 1.0;
        node.estimated_rows = 1;

        auto j = QueryPlanVisualizer::toJSON(node, false);
        EXPECT_EQ(j["plan"]["type"].get<std::string>(), expected_name)
            << "Node type name mismatch for " << expected_name;
    }
}
