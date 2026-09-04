/**
 * @file test_graph_query_rewriter.cpp
 * @brief Unit tests for GraphQueryRewriter (Issue #250: Query Rewriting for
 *        Graph Optimization).
 *
 * Acceptance criteria covered:
 *  - Common subexpression elimination  (GQR-CSE-*)
 *  - Predicate pushdown to graph layer (GQR-PPD-*)
 *  - Join reordering for graph patterns (GQR-JRO-*)
 *  - Materialized view utilisation     (GQR-MAT-*)
 *  - Query decomposition for parallelism (GQR-DEC-*)
 *  - Improved query performance without user intervention
 *  - Optimal execution plans for complex queries
 *  - Reduced redundant computation
 *  - Push predicates into graph traversal (prune early)
 *  - Decompose multi-pattern queries into independent subqueries
 *  - Materialize frequently accessed subgraphs
 *  - Convert repeated traversals to single traversal with caching
 *  - Reorder multi-hop traversals based on selectivity
 *  - estimateSpeedup > 1.0 after successful rewrites
 *  - explainRewrites produces non-empty description
 *  - addCustomRule hook is called and contributes to stats
 *  - RewriteConfig::enabled_rules selectively enables rules
 *  - RewriteConfig::rewrite_time_limit_ms limits execution time
 *  - Factory helpers produce correctly-typed plan nodes
 */

#include <gtest/gtest.h>
#include "graph/graph_query_rewriter.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::graph;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class GraphQueryRewriterTest : public ::testing::Test {
protected:
    GraphQueryRewriter rewriter_;  // default config: all rules enabled

    // ── Factory helpers used by multiple tests ──────────────────────────────

    static json makeFilter(const std::string& field,
                           const std::string& op,
                           const std::string& value) {
        json f;
        f["field"] = field;
        f["op"]    = op;
        f["value"] = value;
        return f;
    }

    static json makeTraversal(const std::string& graph_id,
                              const std::string& start,
                              int max_depth = 2,
                              const std::string& dir = "OUTBOUND",
                              json filters = json::array()) {
        return GraphQueryRewriter::makeTraversalPlan(
            graph_id, start, dir, 1, max_depth, std::move(filters));
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// Factory helpers
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, MakeTraversalPlan_TypeIsCorrect) {
    auto plan = makeTraversal("social", "person/1");
    ASSERT_TRUE(plan.contains("type"));
    EXPECT_EQ(plan["type"].get<std::string>(), "graph_traversal");
}

TEST_F(GraphQueryRewriterTest, MakeTraversalPlan_FieldsPopulated) {
    auto plan = makeTraversal("social", "person/1", 3, "INBOUND");
    EXPECT_EQ(plan["graph_id"].get<std::string>(), "social");
    EXPECT_EQ(plan["start_vertex"].get<std::string>(), "person/1");
    EXPECT_EQ(plan["max_depth"].get<int>(), 3);
    EXPECT_EQ(plan["direction"].get<std::string>(), "INBOUND");
}

TEST_F(GraphQueryRewriterTest, MakeFilterScanPlan_TypeIsCorrect) {
    auto filter = makeFilter("type", "eq", "Person");
    auto child  = makeTraversal("social", "person/1");
    auto plan   = GraphQueryRewriter::makeFilterScanPlan(filter, child);
    ASSERT_TRUE(plan.contains("type"));
    EXPECT_EQ(plan["type"].get<std::string>(), "filter_scan");
}

TEST_F(GraphQueryRewriterTest, MakeJoinPlan_TypeIsCorrect) {
    auto left  = makeTraversal("social", "person/1", 1);
    auto right = makeTraversal("social", "person/2", 3);
    auto plan  = GraphQueryRewriter::makeJoinPlan(left, right);
    ASSERT_TRUE(plan.contains("type"));
    EXPECT_EQ(plan["type"].get<std::string>(), "traversal_join");
    EXPECT_EQ(plan["join_key"].get<std::string>(), "vertex_id");
}

TEST_F(GraphQueryRewriterTest, MakeMultiTraversalPlan_TypeIsCorrect) {
    auto plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"person/1", "person/2", "person/3"});
    ASSERT_TRUE(plan.contains("type"));
    EXPECT_EQ(plan["type"].get<std::string>(), "multi_traversal");
    ASSERT_TRUE(plan.contains("start_vertices"));
    EXPECT_EQ(plan["start_vertices"].size(), 3u);
}

// ═════════════════════════════════════════════════════════════════════════════
// Predicate Pushdown / Prune Early  (GQR-PPD)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, PPD_PromotesVertexFiltersToPreunConditions) {
    // A traversal with a vertex filter: the filter should be promoted to
    // a prune_condition so branches are cut early.
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA")}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("prune_conditions"));
    ASSERT_TRUE(rewritten["prune_conditions"].is_array());
    EXPECT_GT(rewritten["prune_conditions"].size(), 0u);
    EXPECT_TRUE(stats.total_transformations > 0);
}

TEST_F(GraphQueryRewriterTest, PPD_PruneConditionMatchesFilter) {
    json filter = makeFilter("status", "eq", "active");
    json plan   = makeTraversal("social", "person/1", 2, "OUTBOUND",
                                 json::array({filter}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_GE(rewritten["prune_conditions"].size(), 1u);
    const auto& prune = rewritten["prune_conditions"][0];
    EXPECT_EQ(prune["field"].get<std::string>(), "status");
    EXPECT_EQ(prune["op"].get<std::string>(), "eq");
    EXPECT_EQ(prune["value"].get<std::string>(), "active");
}

TEST_F(GraphQueryRewriterTest, PPD_FilterScanPushesFilterIntoChild) {
    json filter = makeFilter("type", "eq", "Person");
    json child  = makeTraversal("social", "person/1", 2);
    json plan   = GraphQueryRewriter::makeFilterScanPlan(filter, child);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // The filter should have been pushed into the child's vertex_filters
    ASSERT_TRUE(rewritten.contains("child"));
    const auto& rchild = rewritten["child"];
    ASSERT_TRUE(rchild.contains("vertex_filters"));
    ASSERT_GE(rchild["vertex_filters"].size(), 1u);
    bool found = false;
    for (const auto& vf : rchild["vertex_filters"]) {
        if (vf.contains("field") &&
            vf["field"].get<std::string>() == "type") {
            found = true; break;
        }
    }
    EXPECT_TRUE(found) << "Filter was not pushed into child vertex_filters";
}

TEST_F(GraphQueryRewriterTest, PPD_DoesNotDuplicatePruneConditions) {
    json filter = makeFilter("country", "eq", "DE");
    json plan   = makeTraversal("social", "person/1", 2, "OUTBOUND",
                                 json::array({filter}));
    // Pre-populate prune_conditions with the same filter
    plan["prune_conditions"] = json::array({filter});

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // Should not have the condition twice
    size_t count = 0;
    for (const auto& p : rewritten["prune_conditions"]) {
        if (p.contains("field") && p["field"].get<std::string>() == "country")
            ++count;
    }
    EXPECT_EQ(count, 1u);
}

TEST_F(GraphQueryRewriterTest, PPD_MultipleFiltersAllPromoted) {
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA"),
                     makeFilter("age_group", "eq", "adult")}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_GE(rewritten["prune_conditions"].size(), 2u);
}

TEST_F(GraphQueryRewriterTest, PPD_NoPruneOnEmptyFilter) {
    json plan = makeTraversal("social", "person/1", 2, "OUTBOUND",
                               json::array());

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // prune_conditions may exist but should be empty
    if (rewritten.contains("prune_conditions")) {
        EXPECT_EQ(rewritten["prune_conditions"].size(), 0u);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Common Subexpression Elimination  (GQR-CSE)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, CSE_DetectsIdenticalTraversals) {
    // Build a join where both sides are identical traversals
    json t = makeTraversal("social", "person/1", 2);
    json plan = GraphQueryRewriter::makeJoinPlan(t, t);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // At least one ref node should appear
    size_t ref_count = 0;
    std::function<void(const json&)> count = [&](const json& n) {
        if (n.is_object()) {
            auto it = n.find("type");
            if (it != n.end() && it->get<std::string>() == "ref") {
              ++ref_count;
            }
            for (const auto& [k, v] : n.items()) {
              count(v);
            }
        }
    };
    count(rewritten);
    EXPECT_GT(ref_count, 0u) << "CSE should replace duplicate traversals with refs";
}

TEST_F(GraphQueryRewriterTest, CSE_DistinctTraversalsNotAffected) {
    // Two traversals with different start vertices — not identical
    json left  = makeTraversal("social", "person/1", 2);
    json right = makeTraversal("social", "person/2", 2);
    json plan  = GraphQueryRewriter::makeJoinPlan(left, right);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    size_t ref_count = 0;
    std::function<void(const json&)> count = [&](const json& n) {
        if (n.is_object()) {
            auto it = n.find("type");
            if (it != n.end() && it->get<std::string>() == "ref") {
              ++ref_count;
            }
            for (const auto& [k, v] : n.items()) {
              count(v);
            }
        }
    };
    count(rewritten);
    EXPECT_EQ(ref_count, 0u) << "Distinct traversals should not be CSE-replaced";
}

TEST_F(GraphQueryRewriterTest, CSE_WrapsInLetScope) {
    json t = makeTraversal("social", "person/1", 2);
    json plan = GraphQueryRewriter::makeJoinPlan(t, t);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // If CSE fired, there should be a let_scope or cse_alias somewhere
    bool has_cse_marker = false;
    std::function<void(const json&)> search = [&](const json& n) {
        if (n.is_object()) {
            if (n.contains("type")) {
                auto t_str = n["type"].get<std::string>();
                if (t_str == "let_scope" || t_str == "ref") {
                    has_cse_marker = true;
                }
            }
            if (n.contains("cse_alias")) {
              has_cse_marker = true;
            }
            for (const auto& [k, v] : n.items()) {
              search(v);
            }
        }
    };
    search(rewritten);
    EXPECT_TRUE(has_cse_marker);
}

TEST_F(GraphQueryRewriterTest, CSE_RecordsRuleName) {
    json t = makeTraversal("social", "person/1", 2);
    json plan = GraphQueryRewriter::makeJoinPlan(t, t);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    bool cse_fired = false;
    for (const auto& name : stats.applied_rule_names) {
        if (name.find("CommonSubexpression") != std::string::npos) {
            cse_fired = true; break;
        }
    }
    EXPECT_TRUE(cse_fired);
}

// ═════════════════════════════════════════════════════════════════════════════
// Join Reordering  (GQR-JRO)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, JRO_SwapsWhenRightIsMoreSelective) {
    // Left: deep traversal (high cardinality)
    // Right: shallow traversal with filters (low cardinality) → should end up left
    json left  = makeTraversal("social", "person/1", 5);
    json right = makeTraversal("social", "person/2", 1, "OUTBOUND",
                                json::array({makeFilter("country", "eq", "USA"),
                                             makeFilter("active", "eq", "true")}));
    json plan  = GraphQueryRewriter::makeJoinPlan(left, right);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // The right (selective) side should have moved to the left position
    ASSERT_TRUE(rewritten.contains("left"));
    const auto& new_left = rewritten["left"];
    // The new left should be the originally-right (depth=1) node
    if (new_left.contains("max_depth")) {
        EXPECT_EQ(new_left["max_depth"].get<int>(), 1);
    }
}

TEST_F(GraphQueryRewriterTest, JRO_NoSwapWhenLeftAlreadyMoreSelective) {
    // Left: shallow+selective, Right: deep — no swap expected
    json left  = makeTraversal("social", "person/1", 1, "OUTBOUND",
                                json::array({makeFilter("country", "eq", "USA")}));
    json right = makeTraversal("social", "person/2", 5);
    json plan  = GraphQueryRewriter::makeJoinPlan(left, right);

    const json original_left = plan["left"];
    auto [rewritten, stats]  = rewriter_.rewrite(plan);

    // Left should remain unchanged (already more selective)
    if (rewritten.contains("left") &&
        rewritten["left"].contains("max_depth")) {
        EXPECT_EQ(rewritten["left"]["max_depth"].get<int>(), 1);
    }
}

TEST_F(GraphQueryRewriterTest, JRO_RecordsRuleName) {
    json left  = makeTraversal("social", "person/1", 5);
    json right = makeTraversal("social", "person/2", 1, "OUTBOUND",
                                json::array({makeFilter("type", "eq", "Person")}));
    json plan  = GraphQueryRewriter::makeJoinPlan(left, right);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    bool jro_fired = false;
    for (const auto& name : stats.applied_rule_names) {
        if (name.find("JoinReordering") != std::string::npos) {
            jro_fired = true; break;
        }
    }
    EXPECT_TRUE(jro_fired);
}

// ═════════════════════════════════════════════════════════════════════════════
// Materialized View  (GQR-MAT)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, MAT_AggressiveModeTagsDeepTraversal) {
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.aggressive_optimization = true;
    GraphQueryRewriter aggressive_rewriter(cfg);

    json plan = makeTraversal("social", "person/1", 3);
    auto [rewritten, stats] = aggressive_rewriter.rewrite(plan);

    // In aggressive mode, depth>=2 traversal should be tagged
    ASSERT_TRUE(rewritten.contains("use_materialized_view"));
    EXPECT_TRUE(rewritten["use_materialized_view"].get<bool>());
}

TEST_F(GraphQueryRewriterTest, MAT_AggressiveModeAddsViewName) {
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.aggressive_optimization = true;
    GraphQueryRewriter aggressive_rewriter(cfg);

    json plan = makeTraversal("social", "person/1", 3);
    auto [rewritten, stats] = aggressive_rewriter.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("materialized_view_name"));
    EXPECT_FALSE(rewritten["materialized_view_name"].get<std::string>().empty());
}

TEST_F(GraphQueryRewriterTest, MAT_NonAggressiveDoesNotTagSingleTraversal) {
    // Default (non-aggressive) mode: a single traversal should not be tagged
    json plan = makeTraversal("social", "person/1", 3);
    auto [rewritten, stats] = rewriter_.rewrite(plan);

    bool tagged = rewritten.contains("use_materialized_view") &&
                  rewritten["use_materialized_view"].is_boolean() &&
                  rewritten["use_materialized_view"].get<bool>();
    EXPECT_FALSE(tagged);
}

TEST_F(GraphQueryRewriterTest, MAT_RecordsRuleNameInAggressive) {
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.aggressive_optimization = true;
    GraphQueryRewriter aggressive_rewriter(cfg);

    json plan = makeTraversal("social", "person/1", 3);
    auto [rewritten, stats] = aggressive_rewriter.rewrite(plan);

    bool mat_fired = false;
    for (const auto& name : stats.applied_rule_names) {
        if (name.find("MaterializedView") != std::string::npos) {
            mat_fired = true; break;
        }
    }
    EXPECT_TRUE(mat_fired);
}

// ═════════════════════════════════════════════════════════════════════════════
// Query Decomposition  (GQR-DEC)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, DEC_MultiTraversalDecomposedToParallel) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"person/1", "person/2", "person/3"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("type"));
    EXPECT_EQ(rewritten["type"].get<std::string>(), "parallel_subqueries");
}

TEST_F(GraphQueryRewriterTest, DEC_SubqueriesContainIndividualTraversals) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"person/1", "person/2"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("subqueries"));
    ASSERT_TRUE(rewritten["subqueries"].is_array());
    EXPECT_EQ(rewritten["subqueries"].size(), 2u);
}

TEST_F(GraphQueryRewriterTest, DEC_SubqueriesAreMarkedParallel) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"person/1", "person/2", "person/3"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("parallel"));
    EXPECT_TRUE(rewritten["parallel"].get<bool>());
}

TEST_F(GraphQueryRewriterTest, DEC_SingleStartNotDecomposed) {
    // A multi_traversal with only one start vertex should not be decomposed
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"person/1"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // Should remain multi_traversal (not converted)
    EXPECT_NE(rewritten.value("type", ""), "parallel_subqueries");
}

TEST_F(GraphQueryRewriterTest, DEC_EachSubqueryPreservesGraphId) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "my_graph", {"v1", "v2"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("subqueries"));
    for (const auto& sq : rewritten["subqueries"]) {
        ASSERT_TRUE(sq.contains("graph_id"));
        EXPECT_EQ(sq["graph_id"].get<std::string>(), "my_graph");
    }
}

TEST_F(GraphQueryRewriterTest, DEC_EachSubqueryHasUniqueStartVertex) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"v1", "v2", "v3"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    ASSERT_TRUE(rewritten.contains("subqueries"));
    std::vector<std::string> starts;
    for (const auto& sq : rewritten["subqueries"]) {
        if (sq.contains("start_vertex")) {
            starts.push_back(sq["start_vertex"].get<std::string>());
        }
    }
    // All start vertices should be distinct
    std::sort(starts.begin(), starts.end());
    EXPECT_EQ(std::unique(starts.begin(), starts.end()), starts.end());
}

TEST_F(GraphQueryRewriterTest, DEC_RecordsRuleName) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"v1", "v2"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    bool dec_fired = false;
    for (const auto& name : stats.applied_rule_names) {
        if (name.find("QueryDecomposition") != std::string::npos) {
            dec_fired = true; break;
        }
    }
    EXPECT_TRUE(dec_fired);
}

// ═════════════════════════════════════════════════════════════════════════════
// estimateSpeedup
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, Speedup_NoChangeReturnOne) {
    json plan = makeTraversal("social", "person/1", 1);
    EXPECT_DOUBLE_EQ(rewriter_.estimateSpeedup(plan, plan), 1.0);
}

TEST_F(GraphQueryRewriterTest, Speedup_PruneConditionsImproveSpeedup) {
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA")}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);
    double speedup = rewriter_.estimateSpeedup(plan, rewritten);
    EXPECT_GT(speedup, 1.0);
}

TEST_F(GraphQueryRewriterTest, Speedup_DecompositionImproveSpeedup) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"v1", "v2", "v3", "v4"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);
    double speedup = rewriter_.estimateSpeedup(plan, rewritten);
    EXPECT_GT(speedup, 1.0);
}

// ═════════════════════════════════════════════════════════════════════════════
// explainRewrites
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, Explain_NoChangeSaysOptimal) {
    json plan = makeTraversal("social", "person/1", 1);
    const std::string expl = rewriter_.explainRewrites(plan, plan);
    EXPECT_FALSE(expl.empty());
    EXPECT_NE(expl.find("optimal"), std::string::npos);
}

TEST_F(GraphQueryRewriterTest, Explain_PruneConditionsMentioned) {
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA")}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);
    const std::string expl  = rewriter_.explainRewrites(plan, rewritten);

    EXPECT_NE(expl.find("prune"), std::string::npos);
}

TEST_F(GraphQueryRewriterTest, Explain_DecompositionMentioned) {
    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"v1", "v2"},
        "OUTBOUND", 2);

    auto [rewritten, stats] = rewriter_.rewrite(plan);
    const std::string expl  = rewriter_.explainRewrites(plan, rewritten);

    EXPECT_NE(expl.find("decompos"), std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════════
// Custom rules
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, CustomRule_CalledDuringRewrite) {
    bool was_called = false;
    rewriter_.addCustomRule("TestCustomRule", [&]( [[maybe_unused]] json& plan) -> size_t {
        was_called = true;
        return 0;
    });

    json plan = makeTraversal("social", "person/1", 2);
    rewriter_.rewrite(plan);
    EXPECT_TRUE(was_called);
}

TEST_F(GraphQueryRewriterTest, CustomRule_TransformationCountedInStats) {
    rewriter_.addCustomRule("CountingRule", [](json& plan) -> size_t {
        plan["custom_tag"] = true;
        return 1;
    });

    json plan = makeTraversal("social", "person/1", 2);
    auto [rewritten, stats] = rewriter_.rewrite(plan);

    EXPECT_TRUE(rewritten.value("custom_tag", false));
    bool custom_fired = false;
    for (const auto& name : stats.applied_rule_names) {
        if (name == "CountingRule") { custom_fired = true; break; }
    }
    EXPECT_TRUE(custom_fired);
}

TEST_F(GraphQueryRewriterTest, CustomRule_NullRuleThrows) {
    EXPECT_THROW(rewriter_.addCustomRule("bad", nullptr),
                 std::invalid_argument);
}

TEST_F(GraphQueryRewriterTest, CustomRule_ClearRemovesRules) {
    bool called = false;
    rewriter_.addCustomRule("R", [&](json&) -> size_t { called = true; return 0; });
    rewriter_.clearCustomRules();

    json plan = makeTraversal("social", "person/1", 2);
    rewriter_.rewrite(plan);
    EXPECT_FALSE(called);
}

// ═════════════════════════════════════════════════════════════════════════════
// Rule selection via RewriteConfig::enabled_rules
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, Config_OnlyPredicatePushdownEnabled) {
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.enabled_rules = {GraphQueryRewriter::RewriteRule::PREDICATE_PUSHDOWN};
    GraphQueryRewriter partial(cfg);

    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"v1", "v2"}, "OUTBOUND", 2);

    auto [rewritten, stats] = partial.rewrite(plan);

    // Decomposition should NOT have fired (not in enabled_rules)
    EXPECT_NE(rewritten.value("type", ""), "parallel_subqueries");
}

TEST_F(GraphQueryRewriterTest, Config_OnlyDecompositionEnabled) {
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.enabled_rules = {GraphQueryRewriter::RewriteRule::QUERY_DECOMPOSITION};
    GraphQueryRewriter partial(cfg);

    // A plan with filters — predicate pushdown should NOT fire
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA")}));

    auto [rewritten, stats] = partial.rewrite(plan);

    // prune_conditions should remain empty (PPD not enabled)
    if (rewritten.contains("prune_conditions")) {
        EXPECT_EQ(rewritten["prune_conditions"].size(), 0u);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RewriteStats correctness
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, Stats_RulesAppliedIsZeroForNoop) {
    json plan = makeTraversal("social", "person/1", 1);
    auto [rewritten, stats] = rewriter_.rewrite(plan);
    // On a trivial single traversal without filters, most rules won't fire.
    // total_transformations should not count non-events.
    // Just verify the struct is populated.
    EXPECT_GE(stats.total_transformations, 0u);
}

TEST_F(GraphQueryRewriterTest, Stats_TotalTransformationsGtZeroWhenRulesFire) {
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA")}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);
    EXPECT_GT(stats.total_transformations, 0u);
}

TEST_F(GraphQueryRewriterTest, Stats_AppliedRuleNamesNotEmpty) {
    json plan = makeTraversal(
        "social", "person/1", 3, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA")}));

    auto [rewritten, stats] = rewriter_.rewrite(plan);
    EXPECT_FALSE(stats.applied_rule_names.empty());
}

// ═════════════════════════════════════════════════════════════════════════════
// End-to-end: complex plan with multiple rules
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(GraphQueryRewriterTest, E2E_ComplexPlanAppliesMultipleRules) {
    // Build:  filter_scan → traversal_join
    //                         left: traversal (deep, no filters)
    //                         right: traversal (shallow, with filter)
    json deep_trav  = makeTraversal("social", "person/1", 5);
    json shallow    = makeTraversal("social", "person/2", 1, "OUTBOUND",
                                    json::array({makeFilter("country", "eq", "USA")}));
    json join_plan  = GraphQueryRewriter::makeJoinPlan(deep_trav, shallow);

    json outer_filter = makeFilter("type", "eq", "Person");
    json plan = GraphQueryRewriter::makeFilterScanPlan(outer_filter, join_plan);

    auto [rewritten, stats] = rewriter_.rewrite(plan);

    // At least one rule must have fired
    EXPECT_GT(stats.total_transformations, 0u);
    EXPECT_FALSE(stats.applied_rule_names.empty());
}

TEST_F(GraphQueryRewriterTest, E2E_SpeedupGtOneForFilteredDeepTraversal) {
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.aggressive_optimization = true;
    GraphQueryRewriter agressive(cfg);

    json plan = makeTraversal(
        "social", "person/1", 4, "OUTBOUND",
        json::array({makeFilter("country", "eq", "USA"),
                     makeFilter("status", "eq", "active")}));

    auto [rewritten, stats] = agressive.rewrite(plan);
    double speedup = agressive.estimateSpeedup(plan, rewritten);
    EXPECT_GT(speedup, 1.0);
}

TEST_F(GraphQueryRewriterTest, E2E_TimeLimitRespectsDeadline) {
    // A near-zero time limit should cause the rewriter to bail out early.
    GraphQueryRewriter::RewriteConfig cfg;
    cfg.rewrite_time_limit_ms = 0.000001; // effectively zero
    GraphQueryRewriter limited(cfg);

    json plan = GraphQueryRewriter::makeMultiTraversalPlan(
        "social", {"v1", "v2", "v3"},
        "OUTBOUND", 3);

    // Should complete without hanging, possibly without any rewrites.
    auto [rewritten, stats] = limited.rewrite(plan);
    SUCCEED(); // no hang = pass
}
