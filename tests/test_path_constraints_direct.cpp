#include <gtest/gtest.h>
#include <filesystem>
#include "storage/rocksdb_wrapper.h"
#include "index/graph_index.h"
#include "storage/base_entity.h"

class PathConstraintsTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_path_constraints_test";
        std::filesystem::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        graph_index_ = std::make_unique<themis::GraphIndexManager>(*storage_);
        
        // Build in-memory topology for faster tests
        ASSERT_TRUE(graph_index_->rebuildTopology().ok);
        
        setupGraph();
    }

    void TearDown() override {
        graph_index_.reset();
        storage_->close();
        std::filesystem::remove_all(db_path_);
    }

    void setupGraph() {
        // Create a simple graph: user1 -> user2 -> user3 -> user4
        //                                 \-> user5
        themis::BaseEntity e1("edge1");
        e1.setField("id", std::string("edge1"));
        e1.setField("_from", std::string("user1"));
        e1.setField("_to", std::string("user2"));
        e1.setField("_weight", 1.0);
        e1.setField("type", std::string("follows"));
        ASSERT_TRUE(graph_index_->addEdge(e1).ok);

        themis::BaseEntity e2("edge2");
        e2.setField("id", std::string("edge2"));
        e2.setField("_from", std::string("user2"));
        e2.setField("_to", std::string("user3"));
        e2.setField("_weight", 2.0);
        e2.setField("type", std::string("likes"));
        ASSERT_TRUE(graph_index_->addEdge(e2).ok);

        themis::BaseEntity e3("edge3");
        e3.setField("id", std::string("edge3"));
        e3.setField("_from", std::string("user3"));
        e3.setField("_to", std::string("user4"));
        e3.setField("_weight", 1.5);
        e3.setField("type", std::string("follows"));
        ASSERT_TRUE(graph_index_->addEdge(e3).ok);

        themis::BaseEntity e4("edge4");
        e4.setField("id", std::string("edge4"));
        e4.setField("_from", std::string("user2"));
        e4.setField("_to", std::string("user5"));
        e4.setField("_weight", 3.0);
        e4.setField("type", std::string("follows"));
        ASSERT_TRUE(graph_index_->addEdge(e4).ok);
        
        // Rebuild topology after adding edges
        ASSERT_TRUE(graph_index_->rebuildTopology().ok);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> storage_;
    std::unique_ptr<themis::GraphIndexManager> graph_index_;
};

// Test basic BFS with unique vertices constraint (cycle detection)
TEST_F(PathConstraintsTest, BFS_UniqueVertices) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.unique_vertices = true;
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_FALSE(result.empty());
    
    // Check no duplicates
    std::unordered_set<std::string> unique(result.begin(), result.end());
    EXPECT_EQ(unique.size(), result.size()) << "Vertices should be unique";
    
    // Should visit user1, user2, user3, user5, user4 (in BFS order)
    EXPECT_GE(result.size(), 4);
}

// Test BFS with unique edges constraint
TEST_F(PathConstraintsTest, BFS_UniqueEdges) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.unique_edges = true;
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_FALSE(result.empty());
    
    // With unique edges, we should still visit all reachable vertices
    EXPECT_GE(result.size(), 4);
}

// Test BFS with forbidden vertices
TEST_F(PathConstraintsTest, BFS_ForbiddenVertices) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_vertices.insert("user2");
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    
    // Should not contain user2
    EXPECT_EQ(std::count(result.begin(), result.end(), "user2"), 0);
    
    // Should only contain user1 (since user2 is the only neighbor)
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "user1");
}

// Test BFS with forbidden edges
TEST_F(PathConstraintsTest, BFS_ForbiddenEdges) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_edges.insert("edge1");
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    
    // Should only contain user1 (since edge1 connects to user2)
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "user1");
}

// Test BFS with max edge count
TEST_F(PathConstraintsTest, BFS_MaxEdgeCount) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.max_edge_count = 1;
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    
    // Should only reach depth 1 (user1 and its direct neighbors)
    // user1 at depth 0, user2 at depth 1
    EXPECT_LE(result.size(), 3); // user1, user2 (and possibly user5 if processed)
}

// Test BFS with required vertices (should fail if not found)
TEST_F(PathConstraintsTest, BFS_RequiredVertices_NotFound) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.required_vertices.insert("user99"); // doesn't exist
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    // Should fail because required vertex is not reachable
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("required"), std::string::npos);
}

// Test BFS with required vertices (should succeed when found)
TEST_F(PathConstraintsTest, BFS_RequiredVertices_Found) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.required_vertices.insert("user3");
    
    auto [status, result] = graph_index_->bfsWithConstraints("user1", 3, constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    
    // Should contain user3
    EXPECT_NE(std::find(result.begin(), result.end(), "user3"), result.end());
}

// Test Dijkstra with unique vertices constraint
TEST_F(PathConstraintsTest, Dijkstra_UniqueVertices) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.unique_vertices = true;
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user4", constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_FALSE(result.path.empty());
    
    // Check no duplicates in path
    std::unordered_set<std::string> unique(result.path.begin(), result.path.end());
    EXPECT_EQ(unique.size(), result.path.size()) << "Path should not have duplicate vertices";
    
    // Path should be: user1 -> user2 -> user3 -> user4
    EXPECT_EQ(result.path.size(), 4);
    EXPECT_EQ(result.path[0], "user1");
    EXPECT_EQ(result.path.back(), "user4");
}

// Test Dijkstra with forbidden vertices
TEST_F(PathConstraintsTest, Dijkstra_ForbiddenVertices) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_vertices.insert("user3");
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user4", constraints);
    
    // Should fail because user3 is on the only path to user4
    EXPECT_FALSE(status.ok);
}

// Test Dijkstra with forbidden edges
TEST_F(PathConstraintsTest, Dijkstra_ForbiddenEdges) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_edges.insert("edge2");
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user4", constraints);
    
    // Should fail because edge2 (user2->user3) is on the only path to user4
    EXPECT_FALSE(status.ok);
}

// Test Dijkstra with max edge count
TEST_F(PathConstraintsTest, Dijkstra_MaxEdgeCount) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.max_edge_count = 2;
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user4", constraints);
    
    // Should fail because path to user4 requires 3 edges
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("max_edge_count"), std::string::npos);
}

// Test Dijkstra with min edge count
TEST_F(PathConstraintsTest, Dijkstra_MinEdgeCount) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.min_edge_count = 5;
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user2", constraints);
    
    // Should fail because shortest path to user2 is only 1 edge
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("min_edge_count"), std::string::npos);
}

// Test Dijkstra with required vertices
TEST_F(PathConstraintsTest, Dijkstra_RequiredVertices) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.required_vertices.insert("user2");
    constraints.required_vertices.insert("user3");
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user4", constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    
    // Path should contain both required vertices
    EXPECT_NE(std::find(result.path.begin(), result.path.end(), "user2"), result.path.end());
    EXPECT_NE(std::find(result.path.begin(), result.path.end(), "user3"), result.path.end());
}

// Test combined constraints
TEST_F(PathConstraintsTest, Combined_Constraints) {
    themis::GraphIndexManager::PathConstraints constraints;
    constraints.unique_vertices = true;
    constraints.unique_edges = true;
    constraints.max_edge_count = 3;
    
    auto [status, result] = graph_index_->dijkstraWithConstraints("user1", "user4", constraints);
    
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(result.path.size(), 4);
    
    // Verify path integrity
    EXPECT_EQ(result.path[0], "user1");
    EXPECT_EQ(result.path.back(), "user4");
    
    // Cost should be sum of weights: 1.0 + 2.0 + 1.5 = 4.5
    EXPECT_NEAR(result.totalCost, 4.5, 0.01);
}
