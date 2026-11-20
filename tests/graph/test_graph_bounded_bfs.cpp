#include "index/graph_index.h"
#include "index/property_graph.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

class BoundedBfsTest : public ::testing::Test {
protected:
    void SetUp() override {
        static std::atomic_uint64_t sequence{0};
        const auto timestamp = std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count();
        dbPath_ = (std::filesystem::temp_directory_path() /
                   ("themis_bounded_bfs_" + std::to_string(timestamp) + "_" +
                    std::to_string(sequence.fetch_add(1)))).string();
        themis::RocksDBWrapper::Config config;
        config.db_path = dbPath_;
        config.enable_blobdb = false;
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        graph_ = std::make_unique<themis::GraphIndexManager>(*db_);
        propertyGraph_ = std::make_unique<themis::PropertyGraphManager>(*db_);
    }

    void TearDown() override {
        propertyGraph_.reset();
        graph_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code error;
        std::filesystem::remove_all(dbPath_, error);
    }

    void addNode(const std::string& id, const std::string& graphId) {
        themis::BaseEntity node(id);
        node.setField("id", id);
        ASSERT_TRUE(propertyGraph_->addNode(node, graphId).ok);
    }

    void addEdge(const std::string& id,
                 const std::string& from,
                 const std::string& to,
                 const std::string& type,
                 const std::string& graphId) {
        themis::BaseEntity edge(id);
        edge.setField("id", id);
        edge.setField("_from", from);
        edge.setField("_to", to);
        edge.setField("_type", type);
        ASSERT_TRUE(propertyGraph_->addEdge(edge, graphId).ok);
    }

    std::string dbPath_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_;
    std::unique_ptr<themis::PropertyGraphManager> propertyGraph_;
};

TEST_F(BoundedBfsTest, AppliesDepthTypeAndGraphScope) {
    addNode("root", "assets");
    addNode("pump", "assets");
    addNode("sensor", "assets");
    addNode("other", "assets");
    addNode("foreign", "other-graph");

    addEdge("root-pump", "root", "pump", "DEPENDS_ON", "assets");
    addEdge("pump-sensor", "pump", "sensor", "DEPENDS_ON", "assets");
    addEdge("root-other-type", "root", "other", "MONITORS", "assets");
    addEdge("foreign-edge", "root", "foreign", "DEPENDS_ON", "other-graph");

    const auto [status, result] = graph_->boundedBfs(
        "root", "assets", 2, 8, 16, std::chrono::milliseconds(500), "DEPENDS_ON");
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(result.nodes.size(), 3u);
    EXPECT_EQ(result.nodes[0].vertexId, "root");
    EXPECT_EQ(result.nodes[0].depth, 0u);
    EXPECT_EQ(result.nodes[1].vertexId, "pump");
    EXPECT_EQ(result.nodes[1].depth, 1u);
    EXPECT_EQ(result.nodes[2].vertexId, "sensor");
    EXPECT_EQ(result.nodes[2].depth, 2u);
    EXPECT_EQ(result.edgesExamined, 3u);
}

TEST_F(BoundedBfsTest, MetersAdjacencyCandidatesAndEdgeDocumentBytes) {
    addNode("root", "assets");
    addNode("pump", "assets");
    addEdge("root-pump", "root", "pump", "DEPENDS_ON", "assets");

    std::size_t candidates = 0;
    std::size_t contentBytes = 0;
    const auto [status, result] = graph_->boundedBfs(
        "root", "assets", 1, 8, 8, std::chrono::milliseconds(500), "DEPENDS_ON",
        [&candidates, &contentBytes](std::size_t newCandidates,
                                     std::size_t newContentBytes) {
            candidates += newCandidates;
            contentBytes += newContentBytes;
            return true;
        });

    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(result.nodes.size(), 2u);
    EXPECT_EQ(candidates, 1u);
    EXPECT_GT(contentBytes, 0u);
}

TEST_F(BoundedBfsTest, MeterDenialReturnsNoPartialTraversal) {
    addNode("root", "assets");
    addNode("pump", "assets");
    addEdge("root-pump", "root", "pump", "DEPENDS_ON", "assets");

    const auto [status, result] = graph_->boundedBfs(
        "root", "assets", 1, 8, 8, std::chrono::milliseconds(500), "DEPENDS_ON",
        [](std::size_t, std::size_t contentBytes) {
            return contentBytes == 0;
        });

    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(result.nodes.empty());
}

TEST_F(BoundedBfsTest, MeterExceptionFailsClosedWithoutPartialTraversal) {
    addNode("root", "assets");
    addNode("pump", "assets");
    addEdge("root-pump", "root", "pump", "DEPENDS_ON", "assets");

    const auto [status, result] = graph_->boundedBfs(
        "root", "assets", 1, 8, 8, std::chrono::milliseconds(500), "DEPENDS_ON",
        [](std::size_t, std::size_t) -> bool {
            throw std::runtime_error("meter failure");
        });

    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(result.nodes.empty());
}

TEST_F(BoundedBfsTest, FailsWithoutReturningPartialResultsWhenLimitsAreExceeded) {
    addNode("root", "assets");
    addNode("pump", "assets");
    addNode("sensor", "assets");
    addEdge("root-pump", "root", "pump", "DEPENDS_ON", "assets");
    addEdge("root-sensor", "root", "sensor", "DEPENDS_ON", "assets");

    const auto [vertexStatus, vertexResult] = graph_->boundedBfs(
        "root", "assets", 1, 2, 8, std::chrono::milliseconds(500), "DEPENDS_ON");
    EXPECT_FALSE(vertexStatus.ok);
    EXPECT_TRUE(vertexResult.nodes.empty());

    const auto [edgeStatus, edgeResult] = graph_->boundedBfs(
        "root", "assets", 1, 8, 1, std::chrono::milliseconds(500), "DEPENDS_ON");
    EXPECT_FALSE(edgeStatus.ok);
    EXPECT_TRUE(edgeResult.nodes.empty());
}

TEST_F(BoundedBfsTest, RejectsInvalidBoundsAndReturnsOnlyStartAtZeroDepth) {
    addNode("root", "assets");

    const auto [invalidStatus, invalidResult] = graph_->boundedBfs(
        "root", "assets", 9, 8, 16, std::chrono::milliseconds(500));
    EXPECT_FALSE(invalidStatus.ok);
    EXPECT_TRUE(invalidResult.nodes.empty());

    const auto [status, result] = graph_->boundedBfs(
        "root", "assets", 0, 1, 1, std::chrono::milliseconds(500));
    ASSERT_TRUE(status.ok) << status.message;
    ASSERT_EQ(result.nodes.size(), 1u);
    EXPECT_EQ(result.nodes.front().vertexId, "root");
    EXPECT_EQ(result.edgesExamined, 0u);
}

} // namespace
