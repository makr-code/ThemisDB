#include <gtest/gtest.h>
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>

namespace fs = std::filesystem;
using themis::GraphIndexManager;

class GraphDirectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_graph_direction_test";
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
        graph_ = std::make_unique<GraphIndexManager>(*db_);
    }
    void TearDown() override {
        graph_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }
    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> graph_;
};

TEST_F(GraphDirectionTest, Dijkstra_InboundDirection_FindsReversePath) {
    // Build A -> B -> C -> D (all weight=1)
    auto mk = [](const char* id, const char* from, const char* to) {
        themis::BaseEntity e(id);
        e.setField("id", id); e.setField("_from", from); e.setField("_to", to); e.setField("_weight", 1.0);
        return e;
    };
    ASSERT_TRUE(graph_->addEdge(mk("e1","A","B")).ok);
    ASSERT_TRUE(graph_->addEdge(mk("e2","B","C")).ok);
    ASSERT_TRUE(graph_->addEdge(mk("e3","C","D")).ok);

    // Inbound: from D back to A following incoming edges
    auto [st, res] = graph_->dijkstra("D", "A", GraphIndexManager::Direction::Inbound);
    ASSERT_TRUE(st.ok) << st.message;
    ASSERT_EQ(res.path.size(), 4u);
    EXPECT_EQ(res.path[0], "D");
    EXPECT_EQ(res.path[1], "C");
    EXPECT_EQ(res.path[2], "B");
    EXPECT_EQ(res.path[3], "A");
    EXPECT_DOUBLE_EQ(res.totalCost, 3.0);
}

TEST_F(GraphDirectionTest, Dijkstra_AnyDirection_MatchesOutboundOnForwardPath) {
    auto mk = [](const char* id, const char* from, const char* to) {
        themis::BaseEntity e(id);
        e.setField("id", id); e.setField("_from", from); e.setField("_to", to); e.setField("_weight", 1.0);
        return e;
    };
    ASSERT_TRUE(graph_->addEdge(mk("e1","A","B")).ok);
    ASSERT_TRUE(graph_->addEdge(mk("e2","B","C")).ok);
    ASSERT_TRUE(graph_->addEdge(mk("e3","C","D")).ok);

    auto [st1, outRes] = graph_->dijkstra("A", "D", GraphIndexManager::Direction::Outbound);
    auto [st2, anyRes] = graph_->dijkstra("A", "D", GraphIndexManager::Direction::Any);
    ASSERT_TRUE(st1.ok);
    ASSERT_TRUE(st2.ok);
    ASSERT_EQ(outRes.path, anyRes.path);
    EXPECT_DOUBLE_EQ(outRes.totalCost, anyRes.totalCost);
}
