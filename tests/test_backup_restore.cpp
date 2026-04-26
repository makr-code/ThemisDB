/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_backup_restore.cpp                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:52:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     68                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static void cleanupPath(const std::string& p) {
    std::error_code ec; fs::remove_all(p, ec);
}

TEST(BackupRestoreTest, CreateAndRestoreCheckpoint) {
    const std::string db_path = "./data/vccdb_backup_test";
    const std::string cp_path = "./data/vccdb_backup_test_cp";

    cleanupPath(db_path);
    cleanupPath(cp_path);

    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    themis::RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());

    // Put initial value
    std::vector<uint8_t> v1{'v','1'};
    ASSERT_TRUE(db.put("test:key", v1));

    // Create checkpoint
    ASSERT_TRUE(db.createCheckpoint(cp_path));

    // Modify DB after checkpoint
    std::vector<uint8_t> v2{'v','2'};
    ASSERT_TRUE(db.put("test:key", v2));

    // Restore from checkpoint (should bring back v1)
    ASSERT_TRUE(db.restoreFromCheckpoint(cp_path));

    auto val = db.get("test:key");
    ASSERT_TRUE(val.has_value());
    std::string s(val->begin(), val->end());
    EXPECT_EQ(s, "v1");

    // Cleanup
    cleanupPath(db_path);
    cleanupPath(cp_path);
}
