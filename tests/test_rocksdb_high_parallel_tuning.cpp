/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_rocksdb_high_parallel_tuning.cpp              ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:53:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     79                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "storage/rocksdb_wrapper.h"

using themis::RocksDBWrapper;

TEST(RocksDBConfigHybrid, AppliesPresetsWhenEnabled) {
    RocksDBWrapper::Config cfg;
    cfg.enable_high_parallel_tuning = true;
    cfg.max_background_compactions = -1;
    cfg.max_background_flushes = -1;
    cfg.background_threads_low = -1;
    cfg.background_threads_high = -1;
    cfg.max_subcompactions = 1;
    cfg.level0_file_num_compaction_trigger = -1;
    cfg.level0_slowdown_writes_trigger = -1;
    cfg.level0_stop_writes_trigger = -1;
    cfg.block_cache_shard_bits = -1;
    cfg.db_write_buffer_size_mb = 0;

    RocksDBWrapper wrapper(cfg);
    const auto& applied = wrapper.getConfig();

    EXPECT_EQ(applied.max_background_compactions, 8);
    EXPECT_EQ(applied.max_background_flushes, 2);
    EXPECT_EQ(applied.background_threads_low, 8);
    EXPECT_EQ(applied.background_threads_high, 2);
    EXPECT_EQ(applied.max_subcompactions, 2);
    EXPECT_EQ(applied.level0_file_num_compaction_trigger, 2);
    EXPECT_EQ(applied.level0_slowdown_writes_trigger, 8);
    EXPECT_EQ(applied.level0_stop_writes_trigger, 16);
    EXPECT_EQ(applied.block_cache_shard_bits, 6);
    EXPECT_EQ(applied.db_write_buffer_size_mb, 512u);
}

TEST(RocksDBConfigHybrid, KeepsDefaultsWhenDisabled) {
    RocksDBWrapper::Config cfg;
    cfg.enable_high_parallel_tuning = false;

    RocksDBWrapper wrapper(cfg);
    const auto& applied = wrapper.getConfig();

    EXPECT_EQ(applied.max_background_compactions, -1);
    EXPECT_EQ(applied.max_background_flushes, -1);
    EXPECT_EQ(applied.background_threads_low, 2);
    EXPECT_EQ(applied.background_threads_high, 2);
    EXPECT_EQ(applied.max_subcompactions, 1);
    EXPECT_EQ(applied.level0_file_num_compaction_trigger, 4);
    EXPECT_EQ(applied.level0_slowdown_writes_trigger, 20);
    EXPECT_EQ(applied.level0_stop_writes_trigger, 36);
    EXPECT_EQ(applied.block_cache_shard_bits, -1);
    EXPECT_EQ(applied.db_write_buffer_size_mb, 0u);
}
