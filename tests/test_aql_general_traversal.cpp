// Integration test for AQL General Traversal

#include <gtest/gtest.h>
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>

using namespace themis;

class AQLGeneralTraversalTest : public ::testing::Test {
protected:
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<SecondaryIndexManager> secIdx;
    std::unique_ptr<GraphIndexManager> graphIdx;
    std::unique_ptr<query::QueryEngine> engine;
    std::string dbPath = "data/themis_aql_general_traversal_test";

    void SetUp() override {
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
        engine = std::make_unique<query::QueryEngine>(*db, *secIdx, *graphIdx);
        
        // Create a sample graph for testing
        createSampleGraph();
    }

    void TearDown() override {
        engine.reset();
        graphIdx.reset();
        secIdx.reset();
        db.reset();
        if (std::filesystem::exists(dbPath)) {
            std::filesystem::remove_all(dbPath);
        }
    }

    void createSampleGraph() {
        // Create a social network graph:
        // Alice -> Bob -> Charlie -> Dave
        //   |                 |
        //   +-> Eve ----------+
        
        BaseEntity edge1("e1");
        edge1.setField("id", "e1");
        edge1.setField("_from", "users/alice");
        edge1.setField("_to", "users/bob");
        edge1.setField("_graph", "social");
        edge1.setField("_type", "follows");
        graphIdx->addEdge(edge1);

        BaseEntity edge2("e2");
        edge2.setField("id", "e2");
        edge2.setField("_from", "users/bob");
        edge2.setField("_to", "users/charlie");
        edge2.setField("_graph", "social");
        edge2.setField("_type", "follows");
        graphIdx->addEdge(edge2);

        BaseEntity edge3("e3");
        edge3.setField("id", "e3");
        edge3.setField("_from", "users/charlie");
        edge3.setField("_to", "users/dave");
        edge3.setField("_graph", "social");
        edge3.setField("_type", "follows");
        graphIdx->addEdge(edge3);

        BaseEntity edge4("e4");
        edge4.setField("id", "e4");
        edge4.setField("_from", "users/alice");
        edge4.setField("_to", "users/eve");
        edge4.setField("_graph", "social");
        edge4.setField("_type", "follows");
        graphIdx->addEdge(edge4);

        BaseEntity edge5("e5");
        edge5.setField("id", "e5");
        edge5.setField("_from", "users/eve");
        edge5.setField("_to", "users/dave");
        edge5.setField("_graph", "social");
        edge5.setField("_type", "follows");
        graphIdx->addEdge(edge5);
    }
};

TEST_F(AQLGeneralTraversalTest, BasicOutboundTraversal) {
    // AQL: FOR v IN 1..2 OUTBOUND "users/alice" GRAPH "social" RETURN v
    std::string aql = R"(
        FOR v IN 1..2 OUTBOUND "users/alice" GRAPH "social"
        RETURN v
    )";

    auto result = executeAql(aql, *engine);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    ASSERT_TRUE(result->contains("type"));
    EXPECT_EQ((*result)["type"], "traversal");
    
    ASSERT_TRUE(result->contains("results"));
    ASSERT_TRUE((*result)["results"].is_array());
    
    // Should find users at depth 1 (bob, eve) and depth 2 (charlie, dave)
    auto results = (*result)["results"];
    EXPECT_GE(results.size(), 2);
    
    // Verify we have results at different depths
    bool hasDepth1 = false;
    bool hasDepth2 = false;
    for (const auto& item : results) {
        if (item["depth"] == 1) hasDepth1 = true;
        if (item["depth"] == 2) hasDepth2 = true;
    }
    EXPECT_TRUE(hasDepth1);
    EXPECT_TRUE(hasDepth2);
}

TEST_F(AQLGeneralTraversalTest, MinDepthFiltering) {
    // AQL: FOR v IN 2..3 OUTBOUND "users/alice" GRAPH "social" RETURN v
    std::string aql = R"(
        FOR v IN 2..3 OUTBOUND "users/alice" GRAPH "social"
        RETURN v
    )";

    auto result = executeAql(aql, *engine);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    ASSERT_TRUE(result->contains("results"));
    
    auto results = (*result)["results"];
    
    // All results should be at depth >= 2
    for (const auto& item : results) {
        ASSERT_TRUE(item.contains("depth"));
        int depth = item["depth"];
        EXPECT_GE(depth, 2) << "Found vertex at depth " << depth << " which is < minDepth(2)";
        EXPECT_LE(depth, 3) << "Found vertex at depth " << depth << " which is > maxDepth(3)";
    }
}

TEST_F(AQLGeneralTraversalTest, InboundDirection) {
    // AQL: FOR v IN 1..2 INBOUND "users/dave" GRAPH "social" RETURN v
    std::string aql = R"(
        FOR v IN 1..2 INBOUND "users/dave" GRAPH "social"
        RETURN v
    )";

    auto result = executeAql(aql, *engine);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    ASSERT_TRUE(result->contains("results"));
    
    auto results = (*result)["results"];
    
    // Should find users going backwards from dave
    // Depth 1: charlie, eve
    // Depth 2: bob, alice
    EXPECT_GE(results.size(), 2);
    
    // Verify some expected vertices
    bool foundCharlieOrEve = false;
    for (const auto& item : results) {
        std::string vertex = item["vertex"];
        if (vertex == "users/charlie" || vertex == "users/eve") {
            foundCharlieOrEve = true;
            EXPECT_EQ(item["depth"], 1);
        }
    }
    EXPECT_TRUE(foundCharlieOrEve);
}

TEST_F(AQLGeneralTraversalTest, AnyDirection) {
    // AQL: FOR v IN 1..1 ANY "users/bob" GRAPH "social" RETURN v
    std::string aql = R"(
        FOR v IN 1..1 ANY "users/bob" GRAPH "social"
        RETURN v
    )";

    auto result = executeAql(aql, *engine);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    
    ASSERT_TRUE(result->contains("results"));
    
    auto results = (*result)["results"];
    
    // Should find alice (inbound) and charlie (outbound)
    EXPECT_GE(results.size(), 2);
    
    bool foundAlice = false;
    bool foundCharlie = false;
    for (const auto& item : results) {
        std::string vertex = item["vertex"];
        if (vertex == "users/alice") foundAlice = true;
        if (vertex == "users/charlie") foundCharlie = true;
    }
    EXPECT_TRUE(foundAlice || foundCharlie);
}

TEST_F(AQLGeneralTraversalTest, PathAndEdgeTracking) {
    // AQL: FOR v IN 2..2 OUTBOUND "users/alice" GRAPH "social" RETURN v
    std::string aql = R"(
        FOR v IN 2..2 OUTBOUND "users/alice" GRAPH "social"
        RETURN v
    )";
    auto result = executeAql(aql, *engine);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_TRUE(result->contains("results"));
    
    auto results = (*result)["results"];
    ASSERT_GE(results.size(), 1);
    
    // Check the first result
    const auto& item = results[0];
    
    // Should have path array
    ASSERT_TRUE(item.contains("path"));
    ASSERT_TRUE(item["path"].is_array());
    auto path = item["path"];
    EXPECT_EQ(path.size(), 3);  // alice -> intermediate -> target
    EXPECT_EQ(path[0], "users/alice");
    
    // Should have edges array
    ASSERT_TRUE(item.contains("edges"));
    ASSERT_TRUE(item["edges"].is_array());
    auto edges = item["edges"];
    EXPECT_EQ(edges.size(), 2);  // 2 edges traversed
}

TEST_F(AQLGeneralTraversalTest, DepthZeroIncludesStart) {
    // AQL: FOR v IN 0..1 OUTBOUND "users/alice" GRAPH "social" RETURN v
    std::string aql = R"(
        FOR v IN 0..1 OUTBOUND "users/alice" GRAPH "social"
        RETURN v
    )";
    auto result = executeAql(aql, *engine);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_TRUE(result->contains("results"));
    
    auto results = (*result)["results"];
    
    // Should include alice herself at depth 0
    bool foundAliceAtDepth0 = false;
    for (const auto& item : results) {
        if (item["vertex"] == "users/alice" && item["depth"] == 0) {
            foundAliceAtDepth0 = true;
            break;
        }
    }
    EXPECT_TRUE(foundAliceAtDepth0);
}
