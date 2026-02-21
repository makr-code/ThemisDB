/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rocksdb_stats_json.cpp                        ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "storage/rocksdb_wrapper.h"

using themis::RocksDBWrapper;
using json = nlohmann::json;

TEST(RocksDBStats, JsonHasExpectedKeys) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = "C:\\tmp\\test_stats_json";
    cfg.enable_wal = true;
    cfg.disable_wal_for_benchmark = false;

    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    // Write a few keys
    for (int i = 0; i < 10; ++i) {
        db.put("k" + std::to_string(i), std::string("v") + std::to_string(i));
    }

    auto stats_str = db.getStats();
    json j = json::parse(stats_str);

    ASSERT_TRUE(j.contains("rocksdb"));
    const auto& r = j["rocksdb"];

    EXPECT_TRUE(r.contains("block_cache_usage_bytes"));
    EXPECT_TRUE(r.contains("estimate_num_keys"));
    EXPECT_TRUE(r.contains("estimate_live_data_size_bytes"));
    EXPECT_TRUE(r.contains("estimate_pending_compaction_bytes"));
    EXPECT_TRUE(r.contains("num_running_compactions"));
    EXPECT_TRUE(r.contains("num_running_flushes"));

    db.close();
}
