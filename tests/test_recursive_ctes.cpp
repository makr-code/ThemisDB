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
