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
    assert(constraints.getConstraints().size() == 6);
    
    // Test description
    std::string desc = constraints.describeConstraints();
    assert(!desc.empty());
    assert(desc.find("Minimum length: 2") != std::string::npos);
    
    // Test clear
    constraints.clearConstraints();
    assert(constraints.getConstraints().empty());
    
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
    assert(!result.has_value()); // Should fail - too short
    
    // Test MAX_LENGTH constraint
    constraints.clearConstraints();
    constraints.addMaxLength(2);
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    assert(!result.has_value()); // Should fail - too long
    
    // Test valid path
    constraints.clearConstraints();
    constraints.addMinLength(2);
    constraints.addMaxLength(5);
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    assert(result.has_value() && *result); // Should pass
    
    // Test FORBIDDEN_NODE constraint
    constraints.clearConstraints();
    constraints.addForbiddenNode("B");
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    assert(!result.has_value()); // Should fail - contains forbidden node
    
    // Test REQUIRED_NODE constraint
    constraints.clearConstraints();
    constraints.addRequiredNode("X");
    nodes = {"A", "B", "C"};
    edges = {"AB", "BC"};
    result = constraints.validatePath(nodes, edges);
    assert(!result.has_value()); // Should fail - missing required node
    
    // Test UNIQUE_NODES constraint
    constraints.clearConstraints();
    constraints.requireUniqueNodes();
    nodes = {"A", "B", "A"}; // Duplicate
    edges = {"AB", "BA"};
    result = constraints.validatePath(nodes, edges);
    assert(!result.has_value()); // Should fail - duplicate node
    
    std::cout << "  ✓ Validation tests passed" << std::endl;
}

void test_path_finding(themis::RocksDBWrapper& storage) {
    std::cout << "Testing path finding..." << std::endl;
    
    auto graph_mgr = setupTestGraph(storage);
    themis::graph::PathConstraints constraints(graph_mgr.get());
    
    // Test basic path finding
    auto paths_result = constraints.findConstrainedPaths("A", "C", 10);
    assert(paths_result.has_value());
    auto& paths = *paths_result;
    assert(paths.size() > 0);
    assert(paths[0].nodes.size() == 3); // A -> B -> C
    assert(paths[0].nodes[0] == "A");
    assert(paths[0].nodes[1] == "B");
    assert(paths[0].nodes[2] == "C");
    
    // Test with MAX_LENGTH constraint
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    constraints.addMaxLength(2);
    paths_result = constraints.findConstrainedPaths("A", "C", 10);
    assert(!paths_result.has_value()); // Should fail - path is 3 nodes (too long)
    
    // Test with MIN_LENGTH constraint
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    constraints.addMinLength(2);
    paths_result = constraints.findConstrainedPaths("A", "B", 10);
    assert(!paths_result.has_value()); // Should fail - path is 2 nodes (equal to min)
    
    // Test with FORBIDDEN_NODE constraint
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    constraints.addForbiddenNode("B");
    paths_result = constraints.findConstrainedPaths("A", "C", 10);
    assert(!paths_result.has_value()); // Should fail - must go through B
    
    // Test with multiple paths
    constraints.clearConstraints();
    constraints.setGraphManager(graph_mgr.get());
    paths_result = constraints.findConstrainedPaths("A", "F", 10);
    assert(paths_result.has_value());
    assert(paths_result->size() > 0);
    
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
    assert(plan_result.has_value());
    
    auto& plan = *plan_result;
    assert(!plan.explanation.empty());
    assert(plan.estimated_cost > 0);
    
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

    std::error_code ec;
    std::filesystem::remove_all(db_path, ec);
}
