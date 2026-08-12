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
