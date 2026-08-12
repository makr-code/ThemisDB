// Dispatch test for SHORTEST_PATH execution

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"

using namespace themis;
using namespace themis::query;

class AQLShortestPathDispatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_aql_shortest_path_dispatch_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::remove_all(db_path_);
        RocksDBWrapper::Config cfg; cfg.db_path = db_path_; cfg.memtable_size_mb=32; cfg.block_cache_size_mb=32;
        db = std::make_unique<RocksDBWrapper>(cfg); ASSERT_TRUE(db->open());
        sec = std::make_unique<SecondaryIndexManager>(*db);
        graph = std::make_unique<GraphIndexManager>(*db);
        // Minimal graph setup (edges not strictly required for dispatcher call, execution may return empty path)
        engine = std::make_unique<QueryEngine>(*db, *sec, *graph);
    }
    void TearDown() override {
        engine.reset(); graph.reset(); sec.reset(); db.reset();
        std::filesystem::remove_all(db_path_);
    }
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db; std::unique_ptr<SecondaryIndexManager> sec; std::unique_ptr<GraphIndexManager> graph; std::unique_ptr<QueryEngine> engine;
};

TEST_F(AQLShortestPathDispatchTest, ExecuteShortestPathSugar) {
    std::string aql = R"(
        FOR v IN 1..3 OUTBOUND "city:berlin" GRAPH "cities"
        SHORTEST_PATH TO "city:dresden"
        RETURN v
    )";
    auto jsonRes = executeAql(aql, *engine);
    // Even if graph lacks edges, we should get OK status or empty path list.
    ASSERT_TRUE(jsonRes.has_value()) << jsonRes.error().message();
    ASSERT_EQ((*jsonRes)["type"], "shortest_path");
    ASSERT_TRUE(jsonRes->contains("paths"));
}
