/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            debug_graph_keys_test.cpp                          ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:57:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     66                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "index/graph_index.h"
#include "storage/base_entity.h"
#include <iostream>
#include <filesystem>

TEST(DebugGraphKeys, DumpKeys) {
    namespace fs = std::filesystem;
    fs::remove_all("./data/debug_graph_keys_test");
    themis::RocksDBWrapper::Config config;
    config.db_path = "./data/debug_graph_keys_test";
    themis::RocksDBWrapper db(config);
    ASSERT_TRUE(db.open());
    themis::GraphIndexManager gm(db);

    themis::BaseEntity e1("edge1");
    e1.setField("id","edge1");
    e1.setField("_from","user1");
    e1.setField("_to","user2");
    ASSERT_TRUE(gm.addEdge(e1).ok);

    themis::BaseEntity e2("edge2");
    e2.setField("id","edge2");
    e2.setField("_from","user1");
    e2.setField("_to","user3");
    ASSERT_TRUE(gm.addEdge(e2).ok);

    std::cout << "--- scanPrefix graph:out: ---\n";
    db.scanPrefix("graph:out:", [](std::string_view k, std::string_view v){
        std::cout << "key='" << std::string(k) << "' val='" << std::string(v) << "'\n";
        return true;
    });

    std::cout << "--- scanPrefix graph:in: ---\n";
    db.scanPrefix("graph:in:", [](std::string_view k, std::string_view v){
        std::cout << "key='" << std::string(k) << "' val='" << std::string(v) << "'\n";
        return true;
    });
}
