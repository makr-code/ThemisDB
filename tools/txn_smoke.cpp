/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            txn_smoke.cpp                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/rocksdb_wrapper.h"
#include <iostream>

int main() {
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = "./data/txn_smoke";
    cfg.enable_blobdb = false;
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;
    cfg.write_policy = themis::RocksDBWrapper::Config::WritePolicy::WritePrepared;

    themis::RocksDBWrapper db(cfg);
    std::cout << "Opening DB at: " << cfg.db_path << std::endl;
    if (!db.open()) {
        std::cout << "open() failed" << std::endl;
        return 1;
    }
    std::cout << "DB open ok" << std::endl;

    std::vector<uint8_t> val{0x01,0x02,0x03};
    bool ok = db.put("smoke_key", val);
    std::cout << "put() returned: " << (ok?"true":"false") << std::endl;

    db.close();
    return ok ? 0 : 2;
}
