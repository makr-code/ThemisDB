/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            txn_smoke.cpp                                      ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:05:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ecd32844f  2025-12-27  Improve RocksDB transaction handling and add smoke test ║
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
