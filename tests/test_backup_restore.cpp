/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_backup_restore.cpp                            ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
