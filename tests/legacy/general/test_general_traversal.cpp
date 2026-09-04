// Unit tests for General Graph Traversal (Non-Shortest Path)

#include <gtest/gtest.h>
#include "query/query_engine.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>

using namespace themis;
using namespace themis::query;

class GeneralTraversalTest : public ::testing::Test {
protected:
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<SecondaryIndexManager> secIdx;
    std::unique_ptr<GraphIndexManager> graphIdx;
    std::unique_ptr<QueryEngine> engine;
    std::string dbPath = "data/themis_general_traversal_test";

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable GeneralTraversal tests on Windows";
#endif
        // Cleanup from previous tests
        if (std::filesystem::exists(dbPath)) {
            std::filesystem::remove_all(dbPath);
        }

        RocksDBWrapper::Config config;
        config.db_path = dbPath;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 256;
        
        db = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db->open());

        secIdx = std::make_unique<SecondaryIndexManager>(*db);
        graphIdx = std::make_unique<GraphIndexManager>(*db);
        engine = std::make_unique<QueryEngine>(*db, *secIdx, *graphIdx);
    }

    void TearDown() override {
#ifdef _WIN32
        return; // SetUp() was skipped on Windows; nothing to clean up
#endif
        engine.reset();
        graphIdx.reset();
        secIdx.reset();
        db.reset();
        if (std::filesystem::exists(dbPath)) {
            std::filesystem::remove_all(dbPath);
        }
    }

    // Helper: Create a simple graph A -> B -> C -> D
    void createLinearGraph() {
        BaseEntity edge1("e1");
        edge1.setField("id", "e1");
        edge1.setField("_from", "A");
        edge1.setField("_to", "B");
        graphIdx->addEdge(edge1);

        BaseEntity edge2("e2");
        edge2.setField("id", "e2");
        edge2.setField("_from", "B");
        edge2.setField("_to", "C");
        graphIdx->addEdge(edge2);

        BaseEntity edge3("e3");
        edge3.setField("id", "e3");
        edge3.setField("_from", "C");
        edge3.setField("_to", "D");
        graphIdx->addEdge(edge3);
    }

    // Helper: Create a graph with branches
    //     B
    //    / \
    //   A   D
    //    \ /
    //     C
    void createDiamondGraph() {
        BaseEntity edge1("e1");
        edge1.setField("id", "e1");
        edge1.setField("_from", "A");
        edge1.setField("_to", "B");
        graphIdx->addEdge(edge1);

        BaseEntity edge2("e2");
        edge2.setField("id", "e2");
        edge2.setField("_from", "A");
        edge2.setField("_to", "C");
        graphIdx->addEdge(edge2);

        BaseEntity edge3("e3");
        edge3.setField("id", "e3");
        edge3.setField("_from", "B");
        edge3.setField("_to", "D");
        graphIdx->addEdge(edge3);

        BaseEntity edge4("e4");
        edge4.setField("id", "e4");
        edge4.setField("_from", "C");
        edge4.setField("_to", "D");
        graphIdx->addEdge(edge4);
    }
};

TEST_F(GeneralTraversalTest, BasicOutboundTraversal) {
    createLinearGraph();

    auto result = engine->executeGeneralTraversal(
        "A",  // startVertex
        1,  // minDepth
        2,  // maxDepth
        TraversalDirection::OUTBOUND
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    
    // Should find B (depth 1) and C (depth 2)
    EXPECT_GE(results.size(), 2);
    
    // Check that we have both depths
    bool hasDepth1 = false;
    bool hasDepth2 = false;
    for (const auto& result : results) {
        if (result.depth == 1) {
          hasDepth1 = true;
        }
        if (result.depth == 2) {
          hasDepth2 = true;
        }
    }
    EXPECT_TRUE(hasDepth1);
    EXPECT_TRUE(hasDepth2);
}

TEST_F(GeneralTraversalTest, MinDepthFiltering) {
    createLinearGraph();

    auto result = engine->executeGeneralTraversal(
        "A",  // startVertex
        2,  // minDepth - only vertices at depth 2+
        3,  // maxDepth
        TraversalDirection::OUTBOUND
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    
    // Should find C (depth 2) and D (depth 3), but NOT B (depth 1)
    for (const auto& result : results) {
        EXPECT_GE(result.depth, 2) << "Found vertex at depth " << result.depth << " which is < minDepth(2)";
        EXPECT_LE(result.depth, 3) << "Found vertex at depth " << result.depth << " which is > maxDepth(3)";
    }
    
    // Should have at least C and D
    EXPECT_GE(results.size(), 2);
}

TEST_F(GeneralTraversalTest, InboundDirection) {
    createLinearGraph();

    // Traverse backwards from D
    auto result = engine->executeGeneralTraversal(
        "D",  // startVertex
        1,  // minDepth
        2,  // maxDepth
        TraversalDirection::INBOUND
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    
    // Should find C (depth 1) and B (depth 2)
    EXPECT_GE(results.size(), 2);
    
    // Verify we're going backwards
    bool foundC = false;
    bool foundB = false;
    for (const auto& result : results) {
        if (result.vertex_pk == "C" && result.depth == 1) {
          foundC = true;
        }
        if (result.vertex_pk == "B" && result.depth == 2) {
          foundB = true;
        }
    }
    EXPECT_TRUE(foundC);
    EXPECT_TRUE(foundB);
}

TEST_F(GeneralTraversalTest, AnyDirection) {
    createLinearGraph();

    // From B, ANY direction should find both A (inbound) and C (outbound)
    auto result = engine->executeGeneralTraversal(
        "B",
        1,  // minDepth
        1,  // maxDepth
        TraversalDirection::ANY,
        "v"
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    
    // Should find at least A and C
    EXPECT_GE(results.size(), 2);
    
    bool foundA = false;
    bool foundC = false;
    for (const auto& result : results) {
        if (result.vertex_pk == "A") {
          foundA = true;
        }
        if (result.vertex_pk == "C") {
          foundC = true;
        }
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundC);
}

TEST_F(GeneralTraversalTest, PathTracking) {
    createLinearGraph();

    auto result = engine->executeGeneralTraversal(
        "B",  // startVertex
        2,  // minDepth
        2,  // maxDepth
        TraversalDirection::OUTBOUND
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    ASSERT_GE(results.size(), 1);
    
    // Find result for vertex C (depth 2)
    const TraversalResult* cResult = nullptr;
    for (const auto& result : results) {
        if (result.vertex_pk == "C") {
            cResult = &result;
            break;
        }
    }
    
    ASSERT_NE(cResult, nullptr) << "Should find vertex C";
    EXPECT_EQ(cResult->depth, 2);
    
    // Path should be [A, B, C]
    ASSERT_EQ(cResult->path.size(), 3);
    EXPECT_EQ(cResult->path[0], "A");
    EXPECT_EQ(cResult->path[1], "B");
    EXPECT_EQ(cResult->path[2], "C");
    
    // Should have 2 edges
    EXPECT_EQ(cResult->edges.size(), 2);
}

TEST_F(GeneralTraversalTest, EdgeTracking) {
    createLinearGraph();

    auto result = engine->executeGeneralTraversal(
        "B",  // startVertex
        1,  // minDepth
        1,  // maxDepth
        TraversalDirection::OUTBOUND
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    ASSERT_GE(results.size(), 1);
    
    // Find result for vertex B (depth 1)
    const TraversalResult* bResult = nullptr;
    for (const auto& result : results) {
        if (result.vertex_pk == "B") {
            bResult = &result;
            break;
        }
    }
    
    ASSERT_NE(bResult, nullptr) << "Should find vertex B";
    
    // Should have 1 edge in path
    ASSERT_EQ(bResult->edges.size(), 1);
    EXPECT_EQ(bResult->edges[0], "e1");
}

TEST_F(GeneralTraversalTest, DiamondGraphMultiplePaths) {
    createDiamondGraph();

    // From A, depth 2 should reach D via both paths (through B and C)
    auto result = engine->executeGeneralTraversal(
        "A",  // startVertex
        0,  // minDepth - include start
        2,  // maxDepth
        TraversalDirection::OUTBOUND
    );
    ASSERT_TRUE(result.has_value()) << result.error().message();
    auto results = std::move(*result);
    
    // Should find: A (depth 0), B (depth 1), C (depth 1), D (depth 2)
    EXPECT_GE(results.size(), 4);
    
    bool foundA = false, foundB = false, foundC = false, foundD = false;
    for (const auto& result : results) {
        if (result.vertex_pk == "A") {
          foundA = true;
        }
        if (result.vertex_pk == "B") {
          foundB = true;
        }
        if (result.vertex_pk == "C") {
          foundC = true;
        }
        if (result.vertex_pk == "D") {
          foundD = true;
        }
    }
    
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);
    EXPECT_TRUE(foundC);
    EXPECT_TRUE(foundD);
}

TEST_F(GeneralTraversalTest, InvalidDepthRange) {
    createLinearGraph();

    // minDepth > maxDepth should return error
    auto result = engine->executeGeneralTraversal(
        "A",  // startVertex
        3,  // minDepth
        2,  // maxDepth (less than minDepth)
        TraversalDirection::OUTBOUND
    );

    EXPECT_FALSE(result.has_value());
}

TEST_F(GeneralTraversalTest, EmptyStartVertex) {
    createLinearGraph();

    auto result = engine->executeGeneralTraversal(
        "",  // empty start vertex
        1,  // minDepth
        2,  // maxDepth
        TraversalDirection::OUTBOUND
    );

    EXPECT_FALSE(result.has_value());
}
