/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            debug_graph_keys.cpp                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     65                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/rocksdb_wrapper.h"
#include "index/graph_index.h"
#include "storage/base_entity.h"
#include <iostream>
#include <filesystem>

int main() {
    namespace fs = std::filesystem;
    fs::remove_all("./debug_graph_db");
    themis::RocksDBWrapper::Config config;
    config.db_path = "./debug_graph_db";
    config.memtable_size_mb = 64;
    config.block_cache_size_mb = 64;
    themis::RocksDBWrapper db(config);
    if (!db.open()) { std::cerr << "open failed\n"; return 1; }
    themis::GraphIndexManager gm(db);

    themis::BaseEntity e1("edge1");
    e1.setField("id","edge1");
    e1.setField("_from","user1");
    e1.setField("_to","user2");
    gm.addEdge(e1);

    themis::BaseEntity e2("edge2");
    e2.setField("id","edge2");
    e2.setField("_from","user1");
    e2.setField("_to","user3");
    gm.addEdge(e2);

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

    return 0;
}
