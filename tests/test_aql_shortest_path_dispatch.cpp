/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_shortest_path_dispatch.cpp                ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:40:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Dispatch test for SHORTEST_PATH execution

#include <gtest/gtest.h>
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"

using namespace themis;

class AQLShortestPathDispatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove_all("data/themis_aql_shortest_path_test");
        RocksDBWrapper::Config cfg; cfg.db_path = "data/themis_aql_shortest_path_test"; cfg.memtable_size_mb=32; cfg.block_cache_size_mb=32;
        db = std::make_unique<RocksDBWrapper>(cfg); ASSERT_TRUE(db->open());
        sec = std::make_unique<SecondaryIndexManager>(*db);
        graph = std::make_unique<GraphIndexManager>(*db);
        // Minimal graph setup (edges not strictly required for dispatcher call, execution may return empty path)
        engine = std::make_unique<QueryEngine>(*db, *sec, *graph);
    }
    void TearDown() override {
        engine.reset(); graph.reset(); sec.reset(); db.reset();
        std::filesystem::remove_all("data/themis_aql_shortest_path_test");
    }
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
