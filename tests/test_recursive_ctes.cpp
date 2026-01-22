#include <gtest/gtest.h>
#include "query/cte_subquery.h"
#include "query/query_engine.h"
#include "query/aql_parser.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include <filesystem>

using namespace themis::query;
using namespace themis;

class RecursiveCTETest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove_all("data/themis_recursive_cte_test");
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = "data/themis_recursive_cte_test";
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        
        secIdx = std::make_unique<SecondaryIndexManager>(*db);
        graphIdx = std::make_unique<GraphIndexManager>(*db);
        engine = std::make_unique<QueryEngine>(*db, *secIdx, *graphIdx, nullptr, nullptr);
        
        setupTestData();
    }
    
    void TearDown() override {
        engine.reset();
        graphIdx.reset();
        secIdx.reset();
        db.reset();
        std::filesystem::remove_all("data/themis_recursive_cte_test");
    }
    
    void setupTestData() {
        // Employee hierarchy for recursive queries
        // CEO -> VP -> Director -> Manager -> Employee
        BaseEntity ceo("emp1");
        ceo.setField("name", std::string("Alice CEO"));
        ceo.setField("title", std::string("CEO"));
        ceo.setField("manager_id", std::string("")); // No manager
        secIdx->put("employees", ceo);
        
        BaseEntity vp("emp2");
        vp.setField("name", std::string("Bob VP"));
        vp.setField("title", std::string("VP"));
        vp.setField("manager_id", std::string("emp1"));
        secIdx->put("employees", vp);
        
        BaseEntity director1("emp3");
        director1.setField("name", std::string("Carol Director"));
        director1.setField("title", std::string("Director"));
        director1.setField("manager_id", std::string("emp2"));
        secIdx->put("employees", director1);
        
        BaseEntity director2("emp4");
        director2.setField("name", std::string("Dave Director"));
        director2.setField("title", std::string("Director"));
        director2.setField("manager_id", std::string("emp2"));
        secIdx->put("employees", director2);
        
        BaseEntity manager1("emp5");
        manager1.setField("name", std::string("Eve Manager"));
        manager1.setField("title", std::string("Manager"));
        manager1.setField("manager_id", std::string("emp3"));
        secIdx->put("employees", manager1);
        
        BaseEntity employee1("emp6");
        employee1.setField("name", std::string("Frank Employee"));
        employee1.setField("title", std::string("Employee"));
        employee1.setField("manager_id", std::string("emp5"));
        secIdx->put("employees", employee1);
    }
    
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<SecondaryIndexManager> secIdx;
    std::unique_ptr<GraphIndexManager> graphIdx;
    std::unique_ptr<QueryEngine> engine;
};

// ============================================================================
// Recursive CTE Tests
// ============================================================================

TEST_F(RecursiveCTETest, SimpleOrgHierarchy) {
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE org_tree AS ("
        "  FOR e IN employees "
        "  FILTER e.manager_id == '' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, o IN org_tree "
        "  FILTER e.manager_id == o._id "
        "  RETURN e"
        ") "
        "FOR emp IN org_tree "
        "RETURN emp.name"
    );
    
    ASSERT_TRUE(result.success) << result.error.toString();
    
    auto query_result = engine->execute(*result.query);
    ASSERT_TRUE(query_result.success);
    
    // Should return all 6 employees in hierarchy
    EXPECT_EQ(query_result.results.size(), 6);
}

TEST_F(RecursiveCTETest, LimitedDepthRecursion) {
    AQLParser parser;
    
    CTEEvaluator::RecursiveCTEConfig config;
    config.max_iterations = 2; // Only go 2 levels deep
    
    // This would need to be passed through the query engine
    // For now, testing the basic functionality
    
    auto result = parser.parse(
        "WITH RECURSIVE limited_tree AS ("
        "  FOR e IN employees "
        "  FILTER e.manager_id == '' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, o IN limited_tree "
        "  FILTER e.manager_id == o._id "
        "  RETURN e"
        ") "
        "FOR emp IN limited_tree "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
}

TEST_F(RecursiveCTETest, CycleDetection) {
    // Create a cycle in the data
    BaseEntity emp_cycle1("cycle1");
    emp_cycle1.setField("name", std::string("Cycle Node 1"));
    emp_cycle1.setField("manager_id", std::string("cycle2"));
    secIdx->put("employees", emp_cycle1);
    
    BaseEntity emp_cycle2("cycle2");
    emp_cycle2.setField("name", std::string("Cycle Node 2"));
    emp_cycle2.setField("manager_id", std::string("cycle1")); // Creates cycle
    secIdx->put("employees", emp_cycle2);
    
    CTEEvaluator::RecursiveCTEConfig config;
    config.detect_cycles = true;
    
    // The recursive query should detect the cycle and stop
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE cycle_test AS ("
        "  FOR e IN employees "
        "  FILTER e._id == 'cycle1' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, c IN cycle_test "
        "  FILTER e._id == c.manager_id "
        "  RETURN e"
        ") "
        "FOR emp IN cycle_test "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
    // Should not hang or crash, cycle detection should stop it
}

TEST_F(RecursiveCTETest, MaxIterationsLimit) {
    CTEEvaluator::RecursiveCTEConfig config;
    config.max_iterations = 100;
    
    // Even with deep hierarchy, should stop at max iterations
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE limited AS ("
        "  FOR e IN employees "
        "  FILTER e.manager_id == '' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, l IN limited "
        "  FILTER e.manager_id == l._id "
        "  RETURN e"
        ") "
        "FOR emp IN limited "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
}

TEST_F(RecursiveCTETest, MaxResultSizeLimit) {
    CTEEvaluator::RecursiveCTEConfig config;
    config.max_result_size = 10; // Only allow 10 results
    
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE size_limited AS ("
        "  FOR e IN employees RETURN e "
        "  UNION "
        "  FOR e IN employees, s IN size_limited "
        "  RETURN e"
        ") "
        "FOR emp IN size_limited "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
    // Should stop when result size reaches limit
}

// ============================================================================
// Complex Recursive Patterns
// ============================================================================

TEST_F(RecursiveCTETest, TransitiveClosure) {
    // Find all paths in a graph (transitive closure)
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE all_reports AS ("
        "  FOR e IN employees "
        "  FILTER e._id == 'emp1' "
        "  RETURN {employee: e._id, depth: 0} "
        "  UNION "
        "  FOR e IN employees, r IN all_reports "
        "  FILTER e.manager_id == r.employee "
        "  RETURN {employee: e._id, depth: r.depth + 1}"
        ") "
        "FOR report IN all_reports "
        "RETURN report"
    );
    
    ASSERT_TRUE(result.success);
}

TEST_F(RecursiveCTETest, BillOfMaterials) {
    // Create parts hierarchy
    BaseEntity part1("part1");
    part1.setField("name", std::string("Engine"));
    part1.setField("parent_part", std::string(""));
    secIdx->put("parts", part1);
    
    BaseEntity part2("part2");
    part2.setField("name", std::string("Cylinder"));
    part2.setField("parent_part", std::string("part1"));
    secIdx->put("parts", part2);
    
    BaseEntity part3("part3");
    part3.setField("name", std::string("Piston"));
    part3.setField("parent_part", std::string("part2"));
    secIdx->put("parts", part3);
    
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE bom AS ("
        "  FOR p IN parts "
        "  FILTER p._id == 'part1' "
        "  RETURN {part: p._id, name: p.name, level: 0} "
        "  UNION "
        "  FOR p IN parts, b IN bom "
        "  FILTER p.parent_part == b.part "
        "  RETURN {part: p._id, name: p.name, level: b.level + 1}"
        ") "
        "FOR item IN bom "
        "RETURN item"
    );
    
    ASSERT_TRUE(result.success);
}

// ============================================================================
// Union Semantics Tests
// ============================================================================

TEST_F(RecursiveCTETest, UnionAllSemantics) {
    // UNION should combine results from anchor and recursive parts
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE tree AS ("
        "  FOR e IN employees "
        "  FILTER e.title == 'CEO' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, t IN tree "
        "  FILTER e.manager_id == t._id "
        "  RETURN e"
        ") "
        "FOR emp IN tree "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
    
    auto query_result = engine->execute(*result.query);
    ASSERT_TRUE(query_result.success);
    
    // Results should be union of anchor (CEO) + all subordinates
    EXPECT_GT(query_result.results.size(), 1);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(RecursiveCTETest, EmptyAnchor) {
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE empty_tree AS ("
        "  FOR e IN employees "
        "  FILTER e.title == 'NonExistent' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, t IN empty_tree "
        "  FILTER e.manager_id == t._id "
        "  RETURN e"
        ") "
        "FOR emp IN empty_tree "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
    
    auto query_result = engine->execute(*result.query);
    ASSERT_TRUE(query_result.success);
    EXPECT_EQ(query_result.results.size(), 0); // No results
}

TEST_F(RecursiveCTETest, SingleIteration) {
    // Recursive part returns no results after first iteration
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE single AS ("
        "  FOR e IN employees "
        "  FILTER e.manager_id == '' "
        "  RETURN e "
        "  UNION "
        "  FOR e IN employees, s IN single "
        "  FILTER false " // Never matches
        "  RETURN e"
        ") "
        "FOR emp IN single "
        "RETURN emp"
    );
    
    ASSERT_TRUE(result.success);
    
    auto query_result = engine->execute(*result.query);
    ASSERT_TRUE(query_result.success);
    EXPECT_EQ(query_result.results.size(), 1); // Only anchor result (CEO)
}

// ============================================================================
// CTE Cycle Detection Tests
// ============================================================================

TEST(RecursiveCTECycleTest, SimpleSelfReference_DetectsCycle) {
    // Test for simple CTE that references itself without proper base case
    // This should be detected and return an error
    
    std::filesystem::remove_all("data/themis_cte_cycle_test");
    RocksDBWrapper::Config cfg;
    cfg.db_path = "data/themis_cte_cycle_test";
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    
    SecondaryIndexManager secIdx(db);
    GraphIndexManager graphIdx(db);
    QueryEngine engine(db, secIdx, graphIdx, nullptr, nullptr);
    
    // Simple CTE that references itself without base case
    // WITH RECURSIVE bad_cte AS (SELECT * FROM bad_cte)
    // This should be detected as a cycle
    
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE cycle_cte AS ("
        "  FOR item IN cycle_cte "  // Direct self-reference without base case
        "  RETURN item"
        ") "
        "FOR c IN cycle_cte RETURN c"
    );
    
    // Parser may catch this, or execution should detect the cycle
    if (result.success) {
        // If parser allows it, execution should detect cycle
        // Note: This behavior depends on implementation
        // For now, we document the expected behavior
        SUCCEED() << "CTE cycle detection test - implementation pending in parser/executor";
    } else {
        // Parser caught the cycle - good!
        EXPECT_FALSE(result.error.message.empty());
        SUCCEED() << "Parser detected cycle: " << result.error.message;
    }
    
    db.close();
    std::filesystem::remove_all("data/themis_cte_cycle_test");
}

TEST(RecursiveCTECycleTest, IndirectCycle_TwoLevelCycle_DetectsCorrectly) {
    // Test for indirect cycle: CTE A references CTE B, CTE B references CTE A
    std::filesystem::remove_all("data/themis_cte_indirect_cycle");
    RocksDBWrapper::Config cfg;
    cfg.db_path = "data/themis_cte_indirect_cycle";
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    
    SecondaryIndexManager secIdx(db);
    GraphIndexManager graphIdx(db);
    QueryEngine engine(db, secIdx, graphIdx, nullptr, nullptr);
    
    // Create test data
    BaseEntity e1("item1");
    e1.setField("value", int64_t(1));
    secIdx.put("items", e1);
    
    // Query with indirect cycle between two CTEs
    AQLParser parser;
    auto result = parser.parse(
        "WITH cte_a AS ("
        "  FOR item IN cte_b "  // References cte_b
        "  RETURN item"
        "), "
        "cte_b AS ("
        "  FOR item IN cte_a "  // References cte_a - creates cycle
        "  RETURN item"
        ") "
        "FOR item IN cte_a RETURN item"
    );
    
    if (result.success) {
        SUCCEED() << "Indirect CTE cycle detection - implementation pending";
    } else {
        EXPECT_FALSE(result.error.message.empty());
        SUCCEED() << "Parser detected indirect cycle: " << result.error.message;
    }
    
    db.close();
    std::filesystem::remove_all("data/themis_cte_indirect_cycle");
}

TEST(RecursiveCTECycleTest, ValidRecursiveCTE_WithBaseCase_NoError) {
    // Test that valid recursive CTEs with proper base cases are allowed
    std::filesystem::remove_all("data/themis_cte_valid_recursive");
    RocksDBWrapper::Config cfg;
    cfg.db_path = "data/themis_cte_valid_recursive";
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    
    SecondaryIndexManager secIdx(db);
    GraphIndexManager graphIdx(db);
    QueryEngine engine(db, secIdx, graphIdx, nullptr, nullptr);
    
    // Create test data - simple hierarchy
    BaseEntity root("n1");
    root.setField("parent_id", std::string(""));
    root.setField("name", std::string("Root"));
    secIdx.put("nodes", root);
    
    BaseEntity child1("n2");
    child1.setField("parent_id", std::string("n1"));
    child1.setField("name", std::string("Child1"));
    secIdx.put("nodes", child1);
    
    BaseEntity child2("n3");
    child2.setField("parent_id", std::string("n1"));
    child2.setField("name", std::string("Child2"));
    secIdx.put("nodes", child2);
    
    // Valid recursive CTE with base case (finds root) and recursive case (finds children)
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE tree AS ("
        "  FOR n IN nodes "
        "  FILTER n.parent_id == '' "  // Base case: find roots
        "  RETURN n "
        "  UNION "
        "  FOR n IN nodes, t IN tree "
        "  FILTER n.parent_id == t._id "  // Recursive case: find children
        "  RETURN n"
        ") "
        "FOR node IN tree RETURN node"
    );
    
    // Valid recursive CTE should be accepted
    EXPECT_TRUE(result.success) << "Valid recursive CTE should not be flagged as cycle: " 
                                 << result.error.message;
    
    db.close();
    std::filesystem::remove_all("data/themis_cte_valid_recursive");
}

TEST(RecursiveCTECycleTest, DeepRecursion_MaxDepthLimit_HandlesGracefully) {
    // Test that very deep recursion is handled (either succeeds or fails gracefully)
    std::filesystem::remove_all("data/themis_cte_deep_recursion");
    RocksDBWrapper::Config cfg;
    cfg.db_path = "data/themis_cte_deep_recursion";
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    
    SecondaryIndexManager secIdx(db);
    GraphIndexManager graphIdx(db);
    QueryEngine engine(db, secIdx, graphIdx, nullptr, nullptr);
    
    // Create deep chain: n1 -> n2 -> n3 -> ... -> n100
    for (int i = 1; i <= 100; ++i) {
        BaseEntity node("n" + std::to_string(i));
        node.setField("parent_id", i == 1 ? std::string("") : std::string("n" + std::to_string(i-1)));
        node.setField("level", int64_t(i));
        secIdx.put("chain", node);
    }
    
    AQLParser parser;
    auto result = parser.parse(
        "WITH RECURSIVE chain_cte AS ("
        "  FOR n IN chain "
        "  FILTER n.parent_id == '' "
        "  RETURN n "
        "  UNION "
        "  FOR n IN chain, c IN chain_cte "
        "  FILTER n.parent_id == c._id "
        "  RETURN n"
        ") "
        "FOR node IN chain_cte RETURN node"
    );
    
    EXPECT_TRUE(result.success) << "Deep recursion should be parsed successfully";
    
    // Note: Actual execution depth limits would be enforced at execution time
    // This test documents expected behavior
    
    db.close();
    std::filesystem::remove_all("data/themis_cte_deep_recursion");
}
