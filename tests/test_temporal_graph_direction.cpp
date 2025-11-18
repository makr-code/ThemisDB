#include <gtest/gtest.h>
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <chrono>
#include <optional>

namespace fs = std::filesystem;
using themis::GraphIndexManager;

class TemporalGraphDirectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_temporal_graph_direction_test";
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
        t_2021_jan = toTimestamp(2021,1,1);
    }
    void TearDown() override {
        graph_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }
    static int64_t toTimestamp(int y,int m,int d){ std::tm tm={}; tm.tm_year=y-1900; tm.tm_mon=m-1; tm.tm_mday=d; auto tp=std::chrono::system_clock::from_time_t(std::mktime(&tm)); return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count(); }
    themis::BaseEntity edge(const std::string& id,const std::string& from,const std::string& to,double w=1.0,std::optional<int64_t> vf=std::nullopt,std::optional<int64_t> vt=std::nullopt){ themis::BaseEntity e(id); e.setField("id",id); e.setField("_from",from); e.setField("_to",to); e.setField("_weight",w); if(vf) e.setField("valid_from",*vf); if(vt) e.setField("valid_to",*vt); return e; }
    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> graph_;
    int64_t t_2021_jan;
};

TEST_F(TemporalGraphDirectionTest, DijkstraAtTime_InboundDirection_Works) {
    // A->B->C->D valid since 2020, weight=1
    auto t2020 = toTimestamp(2020,1,1);
    ASSERT_TRUE(graph_->addEdge(edge("e1","A","B",1.0,t2020)).ok);
    ASSERT_TRUE(graph_->addEdge(edge("e2","B","C",1.0,t2020)).ok);
    ASSERT_TRUE(graph_->addEdge(edge("e3","C","D",1.0,t2020)).ok);

    auto [st, res] = graph_->dijkstraAtTime("D","A", t_2021_jan, GraphIndexManager::Direction::Inbound);
    ASSERT_TRUE(st.ok) << st.message;
    ASSERT_EQ(res.path.size(), 4u);
    EXPECT_EQ(res.path[0], "D");
    EXPECT_EQ(res.path[1], "C");
    EXPECT_EQ(res.path[2], "B");
    EXPECT_EQ(res.path[3], "A");
    EXPECT_DOUBLE_EQ(res.totalCost, 3.0);
}
