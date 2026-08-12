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

    // orderedPredicates sorted ascending: city=200 (less selective) comes FIRST
    // because optimizer sorted [0]=most selective (smallest count).
    // Here: age=50 more selective than city=200, so orderedPredicates[0]=age, [1]=city.
    QueryOptimizer::Plan plan;
    plan.orderedPredicates = {{"age", "30"}, {"city", "Berlin"}};
    QueryOptimizer::Estimation e1; e1.pred = {"age", "30"};       e1.estimatedCount = 50;
    QueryOptimizer::Estimation e2; e2.pred = {"city", "Berlin"};  e2.estimatedCount = 200;
    plan.details = {e1, e2};

    QueryPlanNode root = QueryPlanVisualizer::buildPlan(q, plan);

    EXPECT_EQ(root.type, PlanNodeType::Return);
    // Should have at least one Filter child
    ASSERT_FALSE(root.children.empty());
    EXPECT_EQ(root.children.front()->type, PlanNodeType::Filter);

    // Leaf should be an IndexScan (because predicates are present)
    // Walk to the deepest leaf
    const QueryPlanNode* node = &root;
    while (!node->children.empty()) {
        node = node->children.front().get();
    }
    EXPECT_EQ(node->type, PlanNodeType::IndexScan);
    EXPECT_TRUE(node->index_name.has_value());
}

TEST(QueryPlanVisualizerTest, BuildPlan_MostSelectiveFilterDeepest) {
    // Verify that the most-selective predicate (smallest estimatedCount) is
    // placed DEEPEST in the tree (closest to the scan), not at the top.
    ConjunctiveQuery q;
    q.table = "events";
    // orderedPredicates: [0]=most selective (type=10), [1]=less selective (region=500)
    QueryOptimizer::Plan plan;
    plan.orderedPredicates = {{"type", "click"}, {"region", "EU"}};
    QueryOptimizer::Estimation es; es.pred = {"type", "click"};  es.estimatedCount = 10;
    QueryOptimizer::Estimation el; el.pred = {"region", "EU"};   el.estimatedCount = 500;
    plan.details = {es, el};

    QueryPlanNode root = QueryPlanVisualizer::buildPlan(q, plan);

    // The first filter (direct child of Return) should be the LEAST selective
    // (region/EU, estimatedCount=500) because it is applied last in execution.
    ASSERT_FALSE(root.children.empty());
    const QueryPlanNode& first_filter = *root.children.front();
    EXPECT_EQ(first_filter.type, PlanNodeType::Filter);
    EXPECT_NE(first_filter.description.find("region"), std::string::npos)
        << "Least-selective filter should be closest to Return";

    // The deepest filter (parent of scan) should be the MOST selective (type/click)
    const QueryPlanNode* node = &root;
    while (node->children.size() == 1 &&
           node->children.front()->type == PlanNodeType::Filter) {
        node = node->children.front().get();
    }
    EXPECT_NE(node->description.find("type"), std::string::npos)
        << "Most-selective filter should be closest to scan";
}

TEST(QueryPlanVisualizerTest, BuildPlan_SelectivityLessThanOne_ForSelectiveFilters) {
    // With 2+ predicates, the most-selective filter should have selectivity < 1.
    ConjunctiveQuery q;
    q.table = "logs";
    QueryOptimizer::Plan plan;
    plan.orderedPredicates = {{"level", "ERROR"}, {"service", "api"}};
    QueryOptimizer::Estimation e_sel; e_sel.pred = {"level", "ERROR"};  e_sel.estimatedCount = 5;
    QueryOptimizer::Estimation e_less; e_less.pred = {"service", "api"}; e_less.estimatedCount = 1000;
    plan.details = {e_sel, e_less};

    QueryPlanNode root = QueryPlanVisualizer::buildPlan(q, plan);

    // Walk to the deepest filter (most selective)
    const QueryPlanNode* node = &root;
    const QueryPlanNode* deepest_filter = nullptr;
    while (!node->children.empty() &&
           node->children.front()->type == PlanNodeType::Filter) {
        node = node->children.front().get();
        deepest_filter = node;
    }
    ASSERT_NE(deepest_filter, nullptr);
    EXPECT_LT(deepest_filter->selectivity, 1.0)
        << "Most-selective filter should have selectivity < 1";
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

TEST(QueryPlanVisualizerTest, ToDOT_EscapesSpecialCharsInLabels) {
    QueryPlanNode node;
    node.type = PlanNodeType::Filter;
    // Description with characters that must be escaped in DOT label strings
    node.description = "col == \"value\"\nwith newline\ttab";
    node.estimated_cost = 50.0;
    node.estimated_rows = 100;

    std::string dot = QueryPlanVisualizer::toDOT(node);

    // The overall DOT string must form a valid digraph
    EXPECT_NE(dot.find("digraph QueryPlan"), std::string::npos);
    EXPECT_NE(dot.find('}'), std::string::npos);

    // Raw double-quotes, newlines and tabs must NOT appear bare inside the label
    auto label_pos = dot.find("label=\"");
    ASSERT_NE(label_pos, std::string::npos);
    // Extract everything after label=" up to the closing " shape= portion
    std::string after_label = dot.substr(label_pos + 7);
    auto close_pos = after_label.find("\" shape=");
    ASSERT_NE(close_pos, std::string::npos);
    std::string label_content = after_label.substr(0, close_pos);

    // Must not contain bare newline or bare tab
    EXPECT_EQ(label_content.find('\n'), std::string::npos) << "Raw newline in DOT label";
    EXPECT_EQ(label_content.find('\t'), std::string::npos) << "Raw tab in DOT label";
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

// ============================================================================
// Tests: recursion depth guard
// ============================================================================

// Build a linear chain of n Filter nodes to exercise the depth guard.
static std::shared_ptr<QueryPlanNode> makeDeepFilterChain(int depth) {
    auto node = std::make_shared<QueryPlanNode>();
    node->type = PlanNodeType::Filter;
    node->description = "filter_d" + std::to_string(depth);
    node->estimated_cost = 1.0;
    node->estimated_rows = 1;
    if (depth > 0) {
        node->children.push_back(makeDeepFilterChain(depth - 1));
    }
    return node;
}

TEST(QueryPlanVisualizerTest, DeepTree_ToText_DoesNotCrash) {
    // Build a tree that exceeds kMaxPlanDepth (128); renderers should truncate.
    QueryPlanNode root;
    root.type = PlanNodeType::Return;
    root.description = "Return";
    root.estimated_cost = 1.0;
    root.estimated_rows = 1;
    root.children.push_back(makeDeepFilterChain(200));  // 200 > 128

    // Must complete without stack overflow / crash
    std::string text = QueryPlanVisualizer::toText(root, false);
    EXPECT_NE(text.find("EXPLAIN"), std::string::npos);
    EXPECT_NE(text.find("max depth exceeded"), std::string::npos);
}

TEST(QueryPlanVisualizerTest, DeepTree_ToJSON_DoesNotCrash) {
    QueryPlanNode root;
    root.type = PlanNodeType::Return;
    root.description = "Return";
    root.estimated_cost = 1.0;
    root.estimated_rows = 1;
    root.children.push_back(makeDeepFilterChain(200));

    auto j = QueryPlanVisualizer::toJSON(root, false);
    EXPECT_TRUE(j.contains("plan"));
}

TEST(QueryPlanVisualizerTest, DeepTree_ToDOT_DoesNotCrash) {
    QueryPlanNode root;
    root.type = PlanNodeType::Return;
    root.description = "Return";
    root.estimated_cost = 1.0;
    root.estimated_rows = 1;
    root.children.push_back(makeDeepFilterChain(200));

    std::string dot = QueryPlanVisualizer::toDOT(root);
    EXPECT_NE(dot.find("digraph QueryPlan"), std::string::npos);
    EXPECT_NE(dot.find("max depth exceeded"), std::string::npos);
}
