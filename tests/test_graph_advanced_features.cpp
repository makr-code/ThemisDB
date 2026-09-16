/**
 * @file test_graph_advanced_features.cpp
 * @brief Tests for GAP-006 graph advanced features - PathConstraints
 * 
 * These tests verify that the PathConstraints implementation correctly
 * handles various constraint types and path finding scenarios.
 * 
 * NOTE: Centrality and Community Detection algorithms already exist in
 * GraphAnalytics class (include/index/graph_analytics.h).
 */

#include "graph/path_constraints.h"
#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <gtest/gtest.h>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <memory>

// Helper function to setup a test graph
std::unique_ptr<themis::GraphIndexManager> setupTestGraph(themis::RocksDBWrapper& storage) {
    auto graph_mgr = std::make_unique<themis::GraphIndexManager>(storage);
    
    // Build topology
    assert(graph_mgr->rebuildTopology().ok);
    
    // Create a test graph:
    // A -> B -> C -> D
    //  \-> E -> F
    
    themis::BaseEntity e1("edge1");
    e1.setField("id", std::string("edge1"));
    e1.setField("_from", std::string("A"));
    e1.setField("_to", std::string("B"));
    e1.setField("_weight", 1.0);
    assert(graph_mgr->addEdge(e1).ok);
    
    themis::BaseEntity e2("edge2");
    e2.setField("id", std::string("edge2"));
    e2.setField("_from", std::string("B"));
    e2.setField("_to", std::string("C"));
    e2.setField("_weight", 2.0);
    assert(graph_mgr->addEdge(e2).ok);
    
    themis::BaseEntity e3("edge3");
    e3.setField("id", std::string("edge3"));
    e3.setField("_from", std::string("C"));
    e3.setField("_to", std::string("D"));
    e3.setField("_weight", 1.5);
    assert(graph_mgr->addEdge(e3).ok);
    
    themis::BaseEntity e4("edge4");
    e4.setField("id", std::string("edge4"));
    e4.setField("_from", std::string("A"));
    e4.setField("_to", std::string("E"));
    e4.setField("_weight", 3.0);
    assert(graph_mgr->addEdge(e4).ok);
    
    themis::BaseEntity e5("edge5");
    e5.setField("id", std::string("edge5"));
    e5.setField("_from", std::string("E"));
    e5.setField("_to", std::string("F"));
    e5.setField("_weight", 2.5);
    assert(graph_mgr->addEdge(e5).ok);
    
    // Rebuild topology after adding edges
    assert(graph_mgr->rebuildTopology().ok);
    
    return graph_mgr;
}

void test_path_constraints_interface() {
    std::cout << "Testing PathConstraints interface..." << std::endl;
    
    themis::graph::PathConstraints constraints;
    
    // Test adding constraints
    constraints.addMinLength(2);
    constraints.addMaxLength(10);
    constraints.addForbiddenNode("node_x");
    constraints.addRequiredNode("node_y");
    constraints.requireAcyclic();
    constraints.requireUniqueNodes();
    
    // Verify constraints were added
    EXPECT_EQ(constraints.getConstraints().size(), 6u);
    
    // Test description
    std::string desc = constraints.describeConstraints();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("Minimum length: 2"), std::string::npos);
    
    // Test clear
    constraints.clearConstraints();
    EXPECT_TRUE(constraints.getConstraints().empty());
    
    std::cout << "  ✓ Interface tests passed" << std::endl;
}

void test_path_validation() {
    std::cout << "Testing path validation..." << std::endl;
    
    themis::graph::PathConstraints constraints;
    
    // Test MIN_LENGTH constraint
    constraints.addMinLength(3);
    std::vector<std::string> nodes = {"A", "B"};
    std::vector<std::string> edges = {"AB"};
    auto result = constraints.validatePath(nodes, edges);
    EXPECT_FALSE(result.has_value()) << (result.has_value() ? "unexpected success" : result.error().message());
    
    // Test MAX_LENGTH constraint
    constraints.clearConstraints();
    constraints.addMaxLength(2);
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    EXPECT_FALSE(result.has_value()) << (result.has_value() ? "unexpected success" : result.error().message());
    
    // Test valid path
    constraints.clearConstraints();
    constraints.addMinLength(2);
    constraints.addMaxLength(5);
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    ASSERT_TRUE(result.has_value()) << (result.has_value() ? "" : result.error().message());
    EXPECT_TRUE(*result);
    
    // Test FORBIDDEN_NODE constraint
    constraints.clearConstraints();
    constraints.addForbiddenNode("B");
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    EXPECT_FALSE(result.has_value()) << (result.has_value() ? "unexpected success" : result.error().message());
    
    // Test REQUIRED_NODE constraint
    constraints.clearConstraints();
    constraints.addRequiredNode("X");
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    EXPECT_FALSE(result.has_value()) << (result.has_value() ? "unexpected success" : result.error().message());
    
    // Test UNIQUE_NODES constraint
    constraints.clearConstraints();
    constraints.requireUniqueNodes();
    nodes = {"A", "B", "A"}; // Duplicate
    edges = {"AB", "BA"};
    result = constraints.validatePath(nodes, edges);
    EXPECT_FALSE(result.has_value()) << (result.has_value() ? "unexpected success" : result.error().message());
    
    std::cout << "  ✓ Validation tests passed" << std::endl;
}

void test_path_finding(themis::RocksDBWrapper& storage) {
    std::cout << "Testing path finding..." << std::endl;
    
    auto graph_mgr = setupTestGraph(storage);
    themis::graph::PathConstraints constraints(graph_mgr.get());
    
    // Test basic path finding
    auto paths_result = constraints.findConstrainedPaths("A", "C", 10);
    ASSERT_TRUE(paths_result.has_value()) << (paths_result.has_value() ? "" : paths_result.error().message());
    const auto& paths = *paths_result;
    ASSERT_FALSE(paths.empty());
    ASSERT_EQ(paths[0].nodes.size(), 3u); // A -> B -> C
    EXPECT_EQ(paths[0].nodes[0], "A");
    EXPECT_EQ(paths[0].nodes[1], "B");
    EXPECT_EQ(paths[0].nodes[2], "C");
    
    // Test with MAX_LENGTH constraint
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    constraints.addMaxLength(2);
    paths_result = constraints.findConstrainedPaths("A", "C", 10);
    EXPECT_FALSE(paths_result.has_value()) << (paths_result.has_value() ? "unexpected success" : paths_result.error().message());
    
    // Test with MIN_LENGTH constraint
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    constraints.addMinLength(2);
    paths_result = constraints.findConstrainedPaths("A", "B", 10);
    EXPECT_FALSE(paths_result.has_value()) << (paths_result.has_value() ? "unexpected success" : paths_result.error().message());
    
    // Test with FORBIDDEN_NODE constraint
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    constraints.addForbiddenNode("B");
    paths_result = constraints.findConstrainedPaths("A", "C", 10);
    EXPECT_FALSE(paths_result.has_value()) << (paths_result.has_value() ? "unexpected success" : paths_result.error().message());
    
    // Test with multiple paths
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    paths_result = constraints.findConstrainedPaths("A", "F", 10);
    ASSERT_TRUE(paths_result.has_value()) << (paths_result.has_value() ? "" : paths_result.error().message());
    EXPECT_FALSE(paths_result->empty());
    
    std::cout << "  ✓ Path finding tests passed" << std::endl;
}

void test_optimizer_integration(themis::RocksDBWrapper& storage) {
    std::cout << "Testing GraphQueryOptimizer integration..." << std::endl;
    
    auto graph_mgr = setupTestGraph(storage);
    themis::graph::GraphQueryOptimizer optimizer(*graph_mgr);
    themis::graph::PathConstraints constraints(graph_mgr.get());
    
    // Add some constraints
    constraints.addMinLength(2);
    constraints.addMaxLength(5);
    constraints.requireUniqueNodes();
    
    // Test optimization
    auto plan_result = optimizer.optimizeConstrainedPath("A", "D", constraints);
    ASSERT_TRUE(plan_result.has_value()) << (plan_result.has_value() ? "" : plan_result.error().message());
    
    const auto& plan = *plan_result;
    EXPECT_FALSE(plan.explanation.empty());
    EXPECT_GT(plan.estimated_cost, 0.0);
    
    std::cout << "  Optimization plan generated:" << std::endl;
    std::cout << "    Algorithm: " << (plan.algorithm == themis::graph::GraphQueryOptimizer::TraversalAlgorithm::BFS ? "BFS" : "DFS") << std::endl;
    std::cout << "    Estimated cost: " << plan.estimated_cost << std::endl;
    std::cout << "    Estimated time: " << plan.estimated_time_ms << "ms" << std::endl;
    
    std::cout << "  ✓ Optimizer integration tests passed" << std::endl;
}

TEST(GraphAdvancedFeatures, PathConstraintsIntegration) {
    std::cout << "\n=== GAP-006 Graph Advanced Features Tests ===" << std::endl;
    std::cout << "Testing PathConstraints implementation...\n" << std::endl;
    std::cout << "NOTE: Centrality and Community Detection already exist in GraphAnalytics.\n";
    std::cout << "      See include/index/graph_analytics.h for full implementations.\n" << std::endl;

    test_path_constraints_interface();
    test_path_validation();

    std::string db_path = "./data/themis_path_constraints_test";
    std::filesystem::remove_all(db_path);

    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.memtable_size_mb = 64;
    cfg.block_cache_size_mb = 128;
    themis::RocksDBWrapper storage(cfg);
    ASSERT_TRUE(storage.open());

    test_path_finding(storage);
    test_optimizer_integration(storage);

    storage.close();
    std::filesystem::remove_all(db_path);
}
