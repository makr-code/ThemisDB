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
