// Tests for Graph Path Constraints
// Tests unique_vertices, unique_edges, forbidden_vertices, forbidden_edges, required_vertices

#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace themis;

class GraphPathConstraintsTest : public ::testing::Test {
protected:
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<GraphIndexManager> graphIdx;
    std::filesystem::path testDbPath;

    void SetUp() override {
        testDbPath = std::filesystem::temp_directory_path() / "test_graph_path_constraints";
        std::filesystem::remove_all(testDbPath);
        
        db = std::make_unique<RocksDBWrapper>();
        ASSERT_TRUE(db->open(testDbPath.string()));
        
        graphIdx = std::make_unique<GraphIndexManager>(*db);
        
        // Create test graph: A -> B -> C -> D -> E
        //                     A -> C (shortcut)
        //                     B -> D (shortcut)
        //                     E -> A (creates cycle)
        
        BaseEntity edgeAB;
        edgeAB.setField("id", "e_ab");
        edgeAB.setField("_from", "A");
        edgeAB.setField("_to", "B");
        edgeAB.setField("_weight", 1.0);
        graphIdx->addEdge(edgeAB);
        
        BaseEntity edgeBC;
        edgeBC.setField("id", "e_bc");
        edgeBC.setField("_from", "B");
        edgeBC.setField("_to", "C");
        edgeBC.setField("_weight", 1.0);
        graphIdx->addEdge(edgeBC);
        
        BaseEntity edgeCD;
        edgeCD.setField("id", "e_cd");
        edgeCD.setField("_from", "C");
        edgeCD.setField("_to", "D");
        edgeCD.setField("_weight", 1.0);
        graphIdx->addEdge(edgeCD);
        
        BaseEntity edgeDE;
        edgeDE.setField("id", "e_de");
        edgeDE.setField("_from", "D");
        edgeDE.setField("_to", "E");
        edgeDE.setField("_weight", 1.0);
        graphIdx->addEdge(edgeDE);
        
        // Shortcuts
        BaseEntity edgeAC;
        edgeAC.setField("id", "e_ac");
        edgeAC.setField("_from", "A");
        edgeAC.setField("_to", "C");
        edgeAC.setField("_weight", 2.0);
        graphIdx->addEdge(edgeAC);
        
        BaseEntity edgeBD;
        edgeBD.setField("id", "e_bd");
        edgeBD.setField("_from", "B");
        edgeBD.setField("_to", "D");
        edgeBD.setField("_weight", 2.0);
        graphIdx->addEdge(edgeBD);
        
        // Cycle
        BaseEntity edgeEA;
        edgeEA.setField("id", "e_ea");
        edgeEA.setField("_from", "E");
        edgeEA.setField("_to", "A");
        edgeEA.setField("_weight", 1.0);
        graphIdx->addEdge(edgeEA);
        
        // Rebuild topology for in-memory tests
        graphIdx->rebuildTopology();
    }

    void TearDown() override {
        graphIdx.reset();
        db->close();
        db.reset();
        std::filesystem::remove_all(testDbPath);
    }
};

// Test 1: unique_vertices constraint (cycle detection)
TEST_F(GraphPathConstraintsTest, UniqueVertices_PreventsCycles) {
    GraphIndexManager::PathConstraints constraints;
    constraints.unique_vertices = true;
    
    // Path from A to A (through cycle E->A) should fail with unique_vertices
    auto [status, result] = graphIdx->dijkstra("A", "A", constraints);
    
    // With unique_vertices, we cannot revisit A
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("Kein Pfad gefunden") != std::string::npos);
}

// Test 2: forbidden_vertices constraint
TEST_F(GraphPathConstraintsTest, ForbiddenVertices_BlocksNodes) {
    GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_vertices.insert("B");
    constraints.forbidden_vertices.insert("C");
    
    // Path from A to D normally goes A->B->C->D
    // With B and C forbidden, no path should exist
    auto [status, result] = graphIdx->dijkstra("A", "D", constraints);
    
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("Kein Pfad gefunden") != std::string::npos);
}

// Test 3: forbidden_edges constraint
TEST_F(GraphPathConstraintsTest, ForbiddenEdges_BlocksSpecificEdges) {
    GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_edges.insert("e_ab");
    constraints.forbidden_edges.insert("e_bc");
    
    // Normal path A->B->C->D is blocked
    // Alternative: A->C->D (using shortcut e_ac)
    auto [status, result] = graphIdx->dijkstra("A", "D", constraints);
    
    ASSERT_TRUE(status.ok) << "Error: " << status.message;
    ASSERT_EQ(result.path.size(), 3); // A, C, D
    EXPECT_EQ(result.path[0], "A");
    EXPECT_EQ(result.path[1], "C");
    EXPECT_EQ(result.path[2], "D");
}

// Test 4: required_vertices constraint
TEST_F(GraphPathConstraintsTest, RequiredVertices_MustVisit) {
    GraphIndexManager::PathConstraints constraints;
    constraints.required_vertices.insert("B");
    constraints.required_vertices.insert("C");
    
    // Path from A to D must go through B and C
    auto [status, result] = graphIdx->dijkstra("A", "D", constraints);
    
    ASSERT_TRUE(status.ok) << "Error: " << status.message;
    
    // Verify path contains required vertices
    std::unordered_set<std::string> path_set(result.path.begin(), result.path.end());
    EXPECT_TRUE(path_set.count("B"));
    EXPECT_TRUE(path_set.count("C"));
}

// Test 5: max_path_length constraint
TEST_F(GraphPathConstraintsTest, MaxPathLength_LimitsEdges) {
    GraphIndexManager::PathConstraints constraints;
    constraints.max_path_length = 2;
    
    // Path from A to D with max 2 edges
    // Normal path A->B->C->D has 3 edges
    // Alternative A->B->D has 2 edges
    auto [status, result] = graphIdx->dijkstra("A", "D", constraints);
    
    ASSERT_TRUE(status.ok) << "Error: " << status.message;
    
    // Path should have at most 3 vertices (2 edges)
    EXPECT_LE(result.path.size(), 3);
}

// Test 6: Combined constraints
TEST_F(GraphPathConstraintsTest, CombinedConstraints) {
    GraphIndexManager::PathConstraints constraints;
    constraints.unique_vertices = true;
    constraints.forbidden_vertices.insert("B");
    constraints.max_path_length = 3;
    
    // Path from A to E
    // Cannot use B, must have unique vertices, max 3 edges
    // Possible: A->C->D->E
    auto [status, result] = graphIdx->dijkstra("A", "E", constraints);
    
    ASSERT_TRUE(status.ok) << "Error: " << status.message;
    EXPECT_LE(result.path.size(), 4); // max 3 edges = 4 vertices
    
    // Verify B is not in path
    std::unordered_set<std::string> path_set(result.path.begin(), result.path.end());
    EXPECT_FALSE(path_set.count("B"));
}

// Test 7: Start node forbidden
TEST_F(GraphPathConstraintsTest, StartNodeForbidden_ReturnsError) {
    GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_vertices.insert("A");
    
    auto [status, result] = graphIdx->dijkstra("A", "D", constraints);
    
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("Startknoten ist verboten") != std::string::npos);
}

// Test 8: Target node forbidden
TEST_F(GraphPathConstraintsTest, TargetNodeForbidden_ReturnsError) {
    GraphIndexManager::PathConstraints constraints;
    constraints.forbidden_vertices.insert("D");
    
    auto [status, result] = graphIdx->dijkstra("A", "D", constraints);
    
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("Zielknoten ist verboten") != std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
