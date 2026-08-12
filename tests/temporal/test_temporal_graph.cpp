#include <gtest/gtest.h>
#include "index/graph_index.h"
#include "index/temporal_graph.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

class TemporalGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_temporal_graph_test";
        fs::remove_all(test_db_path_);
        
        themis::RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 256;
        config.max_background_jobs = 2;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        graph_mgr_ = std::make_unique<themis::GraphIndexManager>(*db_);
        
        // Setup timeline: Jan 2020 to Jan 2025
        t_2020_jan = toTimestamp(2020, 1, 1);
        t_2021_jan = toTimestamp(2021, 1, 1);
        t_2022_jan = toTimestamp(2022, 1, 1);
        t_2023_jan = toTimestamp(2023, 1, 1);
        t_2024_jan = toTimestamp(2024, 1, 1);
        t_2025_jan = toTimestamp(2025, 1, 1);
    }

    void TearDown() override {
        graph_mgr_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }
    
    // Helper: Create timestamp in milliseconds
    int64_t toTimestamp(int year, int month, int day) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    }
    
    // Helper: Create temporal edge
    themis::BaseEntity createTemporalEdge(
        const std::string& id,
        const std::string& from,
        const std::string& to,
        std::optional<int64_t> valid_from = std::nullopt,
        std::optional<int64_t> valid_to = std::nullopt,
        double weight = 1.0
    ) {
        themis::BaseEntity edge(id);
        edge.setField("id", id);
        edge.setField("_from", from);
        edge.setField("_to", to);
        edge.setField("_weight", weight);
        
        if (valid_from.has_value()) {
            edge.setField("valid_from", *valid_from);
        }
        if (valid_to.has_value()) {
            edge.setField("valid_to", *valid_to);
        }
        
        return edge;
    }

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_mgr_;
    
    // Test timeline
    int64_t t_2020_jan, t_2021_jan, t_2022_jan, t_2023_jan, t_2024_jan, t_2025_jan;
};

// ===== TemporalFilter Unit Tests =====

TEST_F(TemporalGraphTest, TemporalFilter_NoFilter_AcceptsAll) {
    themis::TemporalFilter filter = themis::TemporalFilter::all();
    
    EXPECT_TRUE(filter.isValid(std::nullopt, std::nullopt));
    EXPECT_TRUE(filter.isValid(t_2020_jan, std::nullopt));
    EXPECT_TRUE(filter.isValid(std::nullopt, t_2025_jan));
    EXPECT_TRUE(filter.isValid(t_2020_jan, t_2025_jan));
}

TEST_F(TemporalGraphTest, TemporalFilter_WithTimestamp_FiltersCorrectly) {
    themis::TemporalFilter filter = themis::TemporalFilter::at(t_2023_jan);
    
    // Edge valid from 2020 to 2025: should pass
    EXPECT_TRUE(filter.isValid(t_2020_jan, t_2025_jan));
    
    // Edge valid from 2020 to 2022: should fail (ended before query time)
    EXPECT_FALSE(filter.isValid(t_2020_jan, t_2022_jan));
    
    // Edge valid from 2024 to 2025: should fail (starts after query time)
    EXPECT_FALSE(filter.isValid(t_2024_jan, t_2025_jan));
    
    // Edge valid from beginning to 2025: should pass
    EXPECT_TRUE(filter.isValid(std::nullopt, t_2025_jan));
    
    // Edge valid from 2020 forever: should pass
    EXPECT_TRUE(filter.isValid(t_2020_jan, std::nullopt));
    
    // Edge always valid: should pass
    EXPECT_TRUE(filter.isValid(std::nullopt, std::nullopt));
}

TEST_F(TemporalGraphTest, TemporalFilter_BoundaryConditions) {
    themis::TemporalFilter filter = themis::TemporalFilter::at(t_2023_jan);
    
    // Edge valid exactly at query time (start)
    EXPECT_TRUE(filter.isValid(t_2023_jan, t_2025_jan));
    
    // Edge valid exactly at query time (end)
    EXPECT_TRUE(filter.isValid(t_2020_jan, t_2023_jan));
    
    // Edge valid only at query time
    EXPECT_TRUE(filter.isValid(t_2023_jan, t_2023_jan));
}

// ===== Simple Temporal Graph Tests =====

TEST_F(TemporalGraphTest, BfsAtTime_NoTemporalEdges_ReturnsAllNeighbors) {
    // Create graph without temporal constraints:
    // A -> B -> C
    auto e1 = createTemporalEdge("e1", "A", "B");
    auto e2 = createTemporalEdge("e2", "B", "C");
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    
    // Query at any time should return all nodes
    auto [status, bfsResult] = graph_mgr_->bfsAtTime("A", t_2023_jan, 10);
    if (!status.ok) FAIL() << status.message;

    EXPECT_EQ(bfsResult.size(), 3u);
    EXPECT_EQ(bfsResult[0], "A");
    EXPECT_EQ(bfsResult[1], "B");
    EXPECT_EQ(bfsResult[2], "C");
}

TEST_F(TemporalGraphTest, BfsAtTime_FiltersByValidFrom) {
    // Edge e1: A -> B, valid from 2022 onwards
    // Edge e2: B -> C, no temporal constraint
    auto e1 = createTemporalEdge("e1", "A", "B", t_2022_jan, std::nullopt);
    auto e2 = createTemporalEdge("e2", "B", "C");
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    
    // Query at 2021: e1 not yet valid, should only return A
    auto [status1, bfs2021] = graph_mgr_->bfsAtTime("A", t_2021_jan, 10);
    if (!status1.ok) FAIL() << status1.message;
    EXPECT_EQ(bfs2021.size(), 1u);
    EXPECT_EQ(bfs2021[0], "A");
    
    // Query at 2023: e1 is valid, should return A -> B -> C
    auto [status2, bfs2023] = graph_mgr_->bfsAtTime("A", t_2023_jan, 10);
    if (!status2.ok) FAIL() << status2.message;
    EXPECT_EQ(bfs2023.size(), 3u);
    EXPECT_EQ(bfs2023[0], "A");
    EXPECT_EQ(bfs2023[1], "B");
    EXPECT_EQ(bfs2023[2], "C");
}

TEST_F(TemporalGraphTest, BfsAtTime_FiltersByValidTo) {
    // Edge e1: A -> B, valid until 2022
    // Edge e2: B -> C, no temporal constraint
    auto e1 = createTemporalEdge("e1", "A", "B", std::nullopt, t_2022_jan);
    auto e2 = createTemporalEdge("e2", "B", "C");
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    
    // Query at 2021: e1 is valid, should return A -> B -> C
    auto [status1, bfs2021] = graph_mgr_->bfsAtTime("A", t_2021_jan, 10);
        EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_EQ(bfs2021.size(), 3u);

    // Query at 2023: e1 expired, should only return A
    auto [status2, bfs2023] = graph_mgr_->bfsAtTime("A", t_2023_jan, 10);
        EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_EQ(bfs2023.size(), 1u);
    EXPECT_EQ(bfs2023[0], "A");
}

TEST_F(TemporalGraphTest, BfsAtTime_FiltersByValidRange) {
    // Edge e1: A -> B, valid from 2021 to 2023
    // Edge e2: B -> C, no temporal constraint
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan);
    auto e2 = createTemporalEdge("e2", "B", "C");
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    
    // Query at 2020: e1 not yet valid
    auto [status1, bfs2020] = graph_mgr_->bfsAtTime("A", t_2020_jan, 10);
        EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_EQ(bfs2020.size(), 1u);
    EXPECT_EQ(bfs2020[0], "A");

    // Query at 2022: e1 is valid
    auto [status2, bfs2022] = graph_mgr_->bfsAtTime("A", t_2022_jan, 10);
        EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_EQ(bfs2022.size(), 3u);

    // Query at 2024: e1 expired
    auto [status3, bfs2024] = graph_mgr_->bfsAtTime("A", t_2024_jan, 10);
        EXPECT_TRUE(status3.ok) << "Status error: " << status3.message;
    EXPECT_EQ(bfs2024.size(), 1u);
    EXPECT_EQ(bfs2024[0], "A");
}

// ===== Complex Temporal Graph Tests =====

TEST_F(TemporalGraphTest, BfsAtTime_MultiplePathsOverTime) {
    // Create graph with evolving relationships:
    // Period 2020-2021: A -> B -> D
    // Period 2022-2023: A -> C -> D
    // Period 2024+:     A -> B -> D and A -> C -> D (both active)
    
    auto e1 = createTemporalEdge("e1", "A", "B", t_2020_jan, std::nullopt);  // Always valid from 2020
    auto e2 = createTemporalEdge("e2", "B", "D", t_2020_jan, t_2021_jan);    // Only 2020-2021
    auto e3 = createTemporalEdge("e3", "A", "C", t_2022_jan, std::nullopt);  // From 2022 onwards
    auto e4 = createTemporalEdge("e4", "C", "D", t_2022_jan, std::nullopt);  // From 2022 onwards
    auto e5 = createTemporalEdge("e5", "B", "D", t_2024_jan, std::nullopt);  // From 2024 onwards (B->D reactivated)
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e3).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e4).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e5).ok);
    
    // At 2020: A -> B -> D
    auto [status1, r1] = graph_mgr_->bfsAtTime("A", t_2020_jan, 10);
        EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_GE(r1.size(), 3u);
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "A") != r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "B") != r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "D") != r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "C") == r1.end()); // C not yet connected
    
    // At 2022: A -> C -> D (B->D inactive, but B still reachable)
    auto [status2, r2] = graph_mgr_->bfsAtTime("A", t_2022_jan, 10);
        EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_GE(r2.size(), 4u);
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "A") != r2.end());
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "B") != r2.end());
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "C") != r2.end());
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "D") != r2.end());
    
    // At 2024: Both paths active
    auto [status3, r3] = graph_mgr_->bfsAtTime("A", t_2024_jan, 10);
        EXPECT_TRUE(status3.ok) << "Status error: " << status3.message;
    EXPECT_EQ(r3.size(), 4u);
    EXPECT_TRUE(std::find(r3.begin(), r3.end(), "A") != r3.end());
    EXPECT_TRUE(std::find(r3.begin(), r3.end(), "B") != r3.end());
    EXPECT_TRUE(std::find(r3.begin(), r3.end(), "C") != r3.end());
    EXPECT_TRUE(std::find(r3.begin(), r3.end(), "D") != r3.end());
}

TEST_F(TemporalGraphTest, BfsAtTime_IsolatedNodeAfterExpiration) {
    // A -> B (valid 2020-2022), B -> C (valid 2020-2022)
    // After 2022, A becomes isolated (no outgoing valid edges)
    auto e1 = createTemporalEdge("e1", "A", "B", t_2020_jan, t_2022_jan);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2020_jan, t_2022_jan);
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    
    // At 2021: Full graph accessible
    auto [status1, r1] = graph_mgr_->bfsAtTime("A", t_2021_jan, 10);
    EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_EQ(r1.size(), 3u);

    // At 2023: A is isolated
    auto [status2, r2] = graph_mgr_->bfsAtTime("A", t_2023_jan, 10);
    EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_EQ(r2.size(), 1u);
    EXPECT_EQ(r2[0], "A");
}

// ===== Dijkstra Temporal Tests =====

TEST_F(TemporalGraphTest, DijkstraAtTime_FindsShortestPathAtTime) {
    // Create weighted graph:
    // A --(weight=1, valid 2020+)--> B --(weight=1, valid 2020+)--> D (total: 2)
    // A --(weight=5, valid 2020+)--> C --(weight=1, valid 2022+)--> D (total: 6)
    
    auto e1 = createTemporalEdge("e1", "A", "B", t_2020_jan, std::nullopt, 1.0);
    auto e2 = createTemporalEdge("e2", "B", "D", t_2020_jan, std::nullopt, 1.0);
    auto e3 = createTemporalEdge("e3", "A", "C", t_2020_jan, std::nullopt, 5.0);
    auto e4 = createTemporalEdge("e4", "C", "D", t_2022_jan, std::nullopt, 1.0);
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e3).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e4).ok);
    
    // At 2021: C->D not yet valid, must use A->B->D (cost 2)
    auto [status1, path1] = graph_mgr_->dijkstraAtTime("A", "D", t_2021_jan);
        EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_EQ(path1.totalCost, 2.0);
    ASSERT_EQ(path1.path.size(), 3u);
    EXPECT_EQ(path1.path[0], "A");
    EXPECT_EQ(path1.path[1], "B");
    EXPECT_EQ(path1.path[2], "D");
    
    // At 2023: C->D is valid, but A->B->D still shorter (cost 2 vs 6)
    auto [status2, path2] = graph_mgr_->dijkstraAtTime("A", "D", t_2023_jan);
        EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_EQ(path2.totalCost, 2.0);
    EXPECT_EQ(path2.path[0], "A");
    EXPECT_EQ(path2.path[1], "B");
    EXPECT_EQ(path2.path[2], "D");
}

TEST_F(TemporalGraphTest, DijkstraAtTime_PathChangesOverTime) {
    // A --(weight=2, valid 2020-2022)--> B --(weight=1, always)--> D (total: 3, only 2020-2022)
    // A --(weight=1, valid 2023+)-------> C --(weight=1, always)--> D (total: 2, from 2023+)
    
    auto e1 = createTemporalEdge("e1", "A", "B", t_2020_jan, t_2022_jan, 2.0);
    auto e2 = createTemporalEdge("e2", "B", "D", std::nullopt, std::nullopt, 1.0);
    auto e3 = createTemporalEdge("e3", "A", "C", t_2023_jan, std::nullopt, 1.0);
    auto e4 = createTemporalEdge("e4", "C", "D", std::nullopt, std::nullopt, 1.0);
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e3).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e4).ok);
    
    // At 2021: Use A->B->D (cost 3)
    auto [status1, path1] = graph_mgr_->dijkstraAtTime("A", "D", t_2021_jan);
    EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_EQ(path1.totalCost, 3.0);
    EXPECT_EQ(path1.path[1], "B");

    // At 2024: Use A->C->D (cost 2)
    auto [status2, path2] = graph_mgr_->dijkstraAtTime("A", "D", t_2024_jan);
    EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_EQ(path2.totalCost, 2.0);
    EXPECT_EQ(path2.path[1], "C");
}

TEST_F(TemporalGraphTest, DijkstraAtTime_NoPathAtTime) {
    // A -> B (valid 2020-2022), B -> C (valid 2020-2022)
    // After 2022, no path from A to C
    auto e1 = createTemporalEdge("e1", "A", "B", t_2020_jan, t_2022_jan);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2020_jan, t_2022_jan);
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    
    // At 2021: Path exists
    auto [status1, path1] = graph_mgr_->dijkstraAtTime("A", "C", t_2021_jan);
    EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_EQ(path1.path.size(), 3u);

    // At 2023: No path (edges expired)
    auto [status2, path2] = graph_mgr_->dijkstraAtTime("A", "C", t_2023_jan);
    EXPECT_FALSE(status2.ok);
    EXPECT_TRUE(status2.message.find("Kein Pfad") != std::string::npos);
}

// ===== Edge Cases =====

TEST_F(TemporalGraphTest, BfsAtTime_EmptyStartNode_ReturnsError) {
    auto [status, result] = graph_mgr_->bfsAtTime("", t_2023_jan, 10);
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("leer") != std::string::npos);
}

TEST_F(TemporalGraphTest, BfsAtTime_NegativeDepth_ReturnsError) {
    auto [status, result] = graph_mgr_->bfsAtTime("A", t_2023_jan, -1);
    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(status.message.find("maxDepth") != std::string::npos);
}

TEST_F(TemporalGraphTest, DijkstraAtTime_EmptyNodes_ReturnsError) {
    auto [status1, path1] = graph_mgr_->dijkstraAtTime("", "B", t_2023_jan);
    EXPECT_FALSE(status1.ok);
    
    auto [status2, path2] = graph_mgr_->dijkstraAtTime("A", "", t_2023_jan);
    EXPECT_FALSE(status2.ok);
}

TEST_F(TemporalGraphTest, BfsAtTime_MaxDepthZero_ReturnsOnlyStart) {
    auto e1 = createTemporalEdge("e1", "A", "B");
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    
    auto [status, bfsResult] = graph_mgr_->bfsAtTime("A", t_2023_jan, 0);
    EXPECT_TRUE(status.ok) << "Status error: " << status.message;
    EXPECT_EQ(bfsResult.size(), 1u);
    EXPECT_EQ(bfsResult[0], "A");
}

// ===== Real-World Scenario Tests =====

TEST_F(TemporalGraphTest, RealWorld_EmploymentHistory) {
    // Model employment relationships over time:
    // Alice worked at CompanyA (2020-2022), then CompanyB (2023+)
    // Bob worked at CompanyA (2021-2024)
    
    auto e1 = createTemporalEdge("alice_compA", "Alice", "CompanyA", t_2020_jan, t_2022_jan);
    auto e2 = createTemporalEdge("alice_compB", "Alice", "CompanyB", t_2023_jan, std::nullopt);
    auto e3 = createTemporalEdge("bob_compA", "Bob", "CompanyA", t_2021_jan, t_2024_jan);
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e3).ok);
    
    // Query: Who worked at CompanyA in 2021?
    // Answer: Alice and Bob (check via inbound edges - not implemented in BFS, but concept valid)
    
    // Query: Where did Alice work in 2021?
    auto [status1, r1] = graph_mgr_->bfsAtTime("Alice", t_2021_jan, 1);
    EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "CompanyA") != r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "CompanyB") == r1.end());

    // Query: Where did Alice work in 2023?
    auto [status2, r2] = graph_mgr_->bfsAtTime("Alice", t_2023_jan, 1);
    EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "CompanyB") != r2.end());
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "CompanyA") == r2.end());
}

TEST_F(TemporalGraphTest, RealWorld_KnowledgeGraphEvolution) {
    // Model evolving knowledge:
    // Document1 CITES Document2 (2020-2022, then retracted)
    // Document1 CITES Document3 (2023+, new citation)
    
    auto e1 = createTemporalEdge("cite1", "Doc1", "Doc2", t_2020_jan, t_2022_jan);
    auto e2 = createTemporalEdge("cite2", "Doc1", "Doc3", t_2023_jan, std::nullopt);
    auto e3 = createTemporalEdge("cite3", "Doc2", "Doc4", t_2020_jan, std::nullopt);
    auto e4 = createTemporalEdge("cite4", "Doc3", "Doc5", t_2023_jan, std::nullopt);
    
    EXPECT_TRUE(graph_mgr_->addEdge(e1).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e2).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e3).ok);
    EXPECT_TRUE(graph_mgr_->addEdge(e4).ok);
    
    // At 2021: Doc1 cites Doc2, which cites Doc4
    auto [status1, r1] = graph_mgr_->bfsAtTime("Doc1", t_2021_jan, 10);
    EXPECT_TRUE(status1.ok) << "Status error: " << status1.message;
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "Doc2") != r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "Doc4") != r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "Doc3") == r1.end());
    EXPECT_TRUE(std::find(r1.begin(), r1.end(), "Doc5") == r1.end());

    // At 2024: Doc1 cites Doc3, which cites Doc5
    auto [status2, r2] = graph_mgr_->bfsAtTime("Doc1", t_2024_jan, 10);
    EXPECT_TRUE(status2.ok) << "Status error: " << status2.message;
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "Doc3") != r2.end());
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "Doc5") != r2.end());
    EXPECT_TRUE(std::find(r2.begin(), r2.end(), "Doc2") == r2.end()); // Citation retracted
}

// ===== TimeRangeFilter Unit Tests =====

TEST_F(TemporalGraphTest, TimeRangeFilter_NoFilter_AcceptsAll) {
    themis::TimeRangeFilter filter = themis::TimeRangeFilter::all();

    EXPECT_TRUE(filter.hasOverlap(std::nullopt, std::nullopt));
    EXPECT_TRUE(filter.hasOverlap(t_2020_jan, std::nullopt));
    EXPECT_TRUE(filter.hasOverlap(std::nullopt, t_2025_jan));
    EXPECT_TRUE(filter.hasOverlap(t_2020_jan, t_2025_jan));
}

TEST_F(TemporalGraphTest, TimeRangeFilter_HasOverlap_DetectsOverlap) {
    themis::TimeRangeFilter filter = themis::TimeRangeFilter::between(t_2021_jan, t_2023_jan);

    // Edge fully within range: overlap
    EXPECT_TRUE(filter.hasOverlap(t_2021_jan, t_2022_jan));

    // Edge starts before range and ends within: overlap
    EXPECT_TRUE(filter.hasOverlap(t_2020_jan, t_2022_jan));

    // Edge starts within range and ends after: overlap
    EXPECT_TRUE(filter.hasOverlap(t_2022_jan, t_2025_jan));

    // Edge fully covers range: overlap
    EXPECT_TRUE(filter.hasOverlap(t_2020_jan, t_2025_jan));

    // Edge ends before range starts: no overlap
    EXPECT_FALSE(filter.hasOverlap(t_2020_jan, t_2020_jan));

    // Edge starts after range ends: no overlap
    EXPECT_FALSE(filter.hasOverlap(t_2024_jan, t_2025_jan));
}

TEST_F(TemporalGraphTest, TimeRangeFilter_HasOverlap_UnboundedEdge) {
    themis::TimeRangeFilter filter = themis::TimeRangeFilter::between(t_2021_jan, t_2023_jan);

    // Edge valid from past to future: overlap with any range
    EXPECT_TRUE(filter.hasOverlap(std::nullopt, std::nullopt));

    // Edge valid from past, ends within range
    EXPECT_TRUE(filter.hasOverlap(std::nullopt, t_2022_jan));

    // Edge valid from past, ends before range
    EXPECT_FALSE(filter.hasOverlap(std::nullopt, t_2020_jan));

    // Edge starts within range, valid forever
    EXPECT_TRUE(filter.hasOverlap(t_2022_jan, std::nullopt));

    // Edge starts after range, valid forever
    EXPECT_FALSE(filter.hasOverlap(t_2024_jan, std::nullopt));
}

TEST_F(TemporalGraphTest, TimeRangeFilter_FullyContains_DetectsContainment) {
    themis::TimeRangeFilter filter = themis::TimeRangeFilter::between(t_2021_jan, t_2023_jan);

    // Edge fully within range: contained
    EXPECT_TRUE(filter.fullyContains(t_2021_jan, t_2022_jan));
    EXPECT_TRUE(filter.fullyContains(t_2021_jan, t_2023_jan));

    // Edge starts before range: not contained
    EXPECT_FALSE(filter.fullyContains(t_2020_jan, t_2022_jan));

    // Edge ends after range: not contained
    EXPECT_FALSE(filter.fullyContains(t_2022_jan, t_2025_jan));

    // Edge unbounded: not contained
    EXPECT_FALSE(filter.fullyContains(std::nullopt, t_2022_jan));
    EXPECT_FALSE(filter.fullyContains(t_2022_jan, std::nullopt));
}

TEST_F(TemporalGraphTest, TimeRangeFilter_Since_OneSidedBound) {
    themis::TimeRangeFilter filter = themis::TimeRangeFilter::since(t_2022_jan);

    // Edge starting after lower bound: overlap
    EXPECT_TRUE(filter.hasOverlap(t_2023_jan, std::nullopt));

    // Edge ending before lower bound: no overlap
    EXPECT_FALSE(filter.hasOverlap(t_2020_jan, t_2021_jan));
}

TEST_F(TemporalGraphTest, TimeRangeFilter_Until_OneSidedBound) {
    themis::TimeRangeFilter filter = themis::TimeRangeFilter::until(t_2022_jan);

    // Edge starting before upper bound: overlap
    EXPECT_TRUE(filter.hasOverlap(t_2020_jan, t_2021_jan));

    // Edge starting after upper bound: no overlap
    EXPECT_FALSE(filter.hasOverlap(t_2023_jan, t_2025_jan));
}

// ===== Time-Range Query Tests =====

TEST_F(TemporalGraphTest, GetEdgesInTimeRange_ReturnsOverlappingEdges) {
    // e1: A->B valid [2021, 2023]
    // e2: B->C valid [2022, 2024]
    // e3: C->D valid [2020, 2020] (before query range)
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2022_jan, t_2024_jan);
    auto e3 = createTemporalEdge("e3", "C", "D", t_2020_jan, t_2020_jan);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e3).ok);

    // Query range [2022, 2023]: e1 and e2 overlap, e3 does not
    auto [status, edges] = graph_mgr_->getEdgesInTimeRange(t_2022_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    std::vector<std::string> ids;
    for (const auto& e : edges) ids.push_back(e.edgeId);

    EXPECT_NE(std::find(ids.begin(), ids.end(), "e1"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "e2"), ids.end());
    EXPECT_EQ(std::find(ids.begin(), ids.end(), "e3"), ids.end());
}

TEST_F(TemporalGraphTest, GetEdgesInTimeRange_FullContainment) {
    // e1: A->B valid [2021, 2023] — fully contained in [2021, 2023]
    // e2: B->C valid [2020, 2024] — NOT fully contained in [2021, 2023]
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2020_jan, t_2024_jan);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);

    auto [status, edges] = graph_mgr_->getEdgesInTimeRange(t_2021_jan, t_2023_jan, true);
    ASSERT_TRUE(status.ok) << status.message;

    std::vector<std::string> ids;
    for (const auto& e : edges) ids.push_back(e.edgeId);

    EXPECT_NE(std::find(ids.begin(), ids.end(), "e1"), ids.end());
    EXPECT_EQ(std::find(ids.begin(), ids.end(), "e2"), ids.end());
}

TEST_F(TemporalGraphTest, GetOutEdgesInTimeRange_FiltersFromNode) {
    // A->B valid [2021, 2023], A->C valid [2024, 2025], B->D valid [2021, 2023]
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan);
    auto e2 = createTemporalEdge("e2", "A", "C", t_2024_jan, t_2025_jan);
    auto e3 = createTemporalEdge("e3", "B", "D", t_2021_jan, t_2023_jan);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e3).ok);

    // From A, range [2021, 2023]: only e1 qualifies
    auto [status, edges] = graph_mgr_->getOutEdgesInTimeRange("A", t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    std::vector<std::string> ids;
    for (const auto& e : edges) ids.push_back(e.edgeId);

    EXPECT_NE(std::find(ids.begin(), ids.end(), "e1"), ids.end());
    EXPECT_EQ(std::find(ids.begin(), ids.end(), "e2"), ids.end());
    EXPECT_EQ(std::find(ids.begin(), ids.end(), "e3"), ids.end()); // from B, not A
}

TEST_F(TemporalGraphTest, GetOutEdgesInTimeRange_EmptyNodeReturnsError) {
    auto [status, edges] = graph_mgr_->getOutEdgesInTimeRange("", t_2021_jan, t_2023_jan, false);
    EXPECT_FALSE(status.ok);
}

TEST_F(TemporalGraphTest, GetTemporalStats_ReturnsCorrectCounts) {
    // Add 3 edges: 2 overlap range [2021,2023], 1 is fully contained
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan); // fully contained
    auto e2 = createTemporalEdge("e2", "B", "C", t_2020_jan, t_2024_jan); // overlap only
    auto e3 = createTemporalEdge("e3", "C", "D", t_2024_jan, t_2025_jan); // no overlap

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e3).ok);

    auto [status, stats] = graph_mgr_->getTemporalStats(t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_EQ(stats.edge_count, 2u);          // e1 and e2 overlap range
    EXPECT_EQ(stats.fully_contained_count, 1u); // only e1 is fully contained
}

TEST_F(TemporalGraphTest, GetTemporalStats_DurationStatistics) {
    // e1: A->B valid [2021, 2023] — duration = 2 years
    // e2: B->C valid [2022, 2023] — duration = 1 year
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2022_jan, t_2023_jan);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);

    auto [status, stats] = graph_mgr_->getTemporalStats(t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_EQ(stats.bounded_edge_count, 2u);
    EXPECT_GT(stats.total_duration_ms, 0.0);
    EXPECT_GT(stats.avg_duration_ms, 0.0);
    ASSERT_TRUE(stats.min_duration_ms.has_value());
    ASSERT_TRUE(stats.max_duration_ms.has_value());
    EXPECT_LE(*stats.min_duration_ms, *stats.max_duration_ms);
}

TEST_F(TemporalGraphTest, TemporalStats_ToString_ContainsMetrics) {
    themis::TemporalStats stats;
    stats.edge_count = 5;
    stats.fully_contained_count = 3;
    stats.bounded_edge_count = 4;
    stats.avg_duration_ms = 1000.0;
    stats.total_duration_ms = 4000.0;
    stats.min_duration_ms = 500;
    stats.max_duration_ms = 2000;
    stats.earliest_start = t_2020_jan;
    stats.latest_end = t_2025_jan;

    std::string output = stats.toString();
    EXPECT_NE(output.find("5"), std::string::npos);       // edge_count
    EXPECT_NE(output.find("3"), std::string::npos);       // fully_contained_count
    EXPECT_NE(output.find("1000"), std::string::npos);    // avg_duration_ms
}

TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_Count) {
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan, 2.0);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2022_jan, t_2024_jan, 3.0);
    auto e3 = createTemporalEdge("e3", "C", "D", t_2024_jan, t_2025_jan, 5.0);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e3).ok);

    auto [status, result] = graph_mgr_->aggregateEdgePropertyInTimeRange(
        "_weight", themis::GraphIndexManager::Aggregation::COUNT,
        t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    // e1 and e2 overlap range [2021, 2023]
    EXPECT_EQ(result.count, 2u);
}

TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_Sum) {
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan, 2.0);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2022_jan, t_2024_jan, 3.0);
    auto e3 = createTemporalEdge("e3", "C", "D", t_2024_jan, t_2025_jan, 5.0);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e3).ok);

    auto [status, result] = graph_mgr_->aggregateEdgePropertyInTimeRange(
        "_weight", themis::GraphIndexManager::Aggregation::SUM,
        t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    // Sum of weights for e1 (2.0) and e2 (3.0)
    EXPECT_NEAR(result.value, 5.0, 1e-9);
}

TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_Avg) {
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan, 2.0);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2022_jan, t_2024_jan, 4.0);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);

    auto [status, result] = graph_mgr_->aggregateEdgePropertyInTimeRange(
        "_weight", themis::GraphIndexManager::Aggregation::AVG,
        t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(status.ok) << status.message;

    EXPECT_NEAR(result.value, 3.0, 1e-9); // (2 + 4) / 2
}

TEST_F(TemporalGraphTest, AggregateEdgePropertyInTimeRange_MinMax) {
    auto e1 = createTemporalEdge("e1", "A", "B", t_2021_jan, t_2023_jan, 2.0);
    auto e2 = createTemporalEdge("e2", "B", "C", t_2022_jan, t_2024_jan, 4.0);
    auto e3 = createTemporalEdge("e3", "C", "D", t_2021_jan, t_2023_jan, 6.0);

    ASSERT_TRUE(graph_mgr_->addEdge(e1).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e2).ok);
    ASSERT_TRUE(graph_mgr_->addEdge(e3).ok);

    auto [s1, min_result] = graph_mgr_->aggregateEdgePropertyInTimeRange(
        "_weight", themis::GraphIndexManager::Aggregation::MIN,
        t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(s1.ok) << s1.message;
    EXPECT_NEAR(min_result.value, 2.0, 1e-9);

    auto [s2, max_result] = graph_mgr_->aggregateEdgePropertyInTimeRange(
        "_weight", themis::GraphIndexManager::Aggregation::MAX,
        t_2021_jan, t_2023_jan, false);
    ASSERT_TRUE(s2.ok) << s2.message;
    EXPECT_NEAR(max_result.value, 6.0, 1e-9);
}

