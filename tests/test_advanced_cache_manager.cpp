// Copyright 2026 ThemisDB — AdvancedCacheManager focused tests (Issue #229)
#include "performance/advanced_cache_manager.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace themis::performance;

namespace {

static CacheConfig make_config(size_t hot_mb = 2, size_t cold_mb = 1) {
    CacheConfig cfg;
    cfg.total_size_mb = hot_mb + cold_mb;
    cfg.enable_bloom_filters = true;
    cfg.partitions.push_back({"hot",  hot_mb,  EvictionPolicy::LRU, false});
    cfg.partitions.push_back({"cold", cold_mb, EvictionPolicy::LRU, true, CompressionAlgorithm::LZ4});
    return cfg;
}

class ACMTest : public ::testing::Test {
protected:
    AdvancedCacheManager mgr{make_config()};
};

// --- Construction & partitions -----------------------------------------------

TEST_F(ACMTest, DefaultConstructionSucceeds) {
    AdvancedCacheManager def;
    auto names = def.partition_names();
    EXPECT_FALSE(names.empty());
}

TEST_F(ACMTest, PartitionNamesReturned) {
    auto names = mgr.partition_names();
    EXPECT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "hot");
    EXPECT_EQ(names[1], "cold");
}

TEST_F(ACMTest, CreatePartitionsReplaces) {
    mgr.create_partitions(make_config(4, 4));
    auto names = mgr.partition_names();
    EXPECT_EQ(names.size(), 2u);
}

// --- get / put ---------------------------------------------------------------

TEST_F(ACMTest, PutAndGetHitHotPartition) {
    mgr.put("k1", "v1", "hot");
    auto val = mgr.get("k1", "hot");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "v1");
}

TEST_F(ACMTest, MissReturnsNullopt) {
    auto val = mgr.get("missing", "hot");
    EXPECT_FALSE(val.has_value());
}

TEST_F(ACMTest, PutAndGetColdPartition) {
    mgr.put("ck1", "cold_value", "cold");
    auto val = mgr.get("ck1", "cold");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "cold_value");
}

TEST_F(ACMTest, CompressionBridgeCanOverrideFallbackCodec) {
    CacheConfig cfg;
    cfg.total_size_mb = 2;
    cfg.enable_bloom_filters = false;
    cfg.partitions.push_back({"cold", 1, EvictionPolicy::LRU, true, CompressionAlgorithm::None});
    AdvancedCacheManager bridged(cfg);

    AdvancedCacheManager::setCompressFn([](const std::string& val, CompressionAlgorithm algo) {
        if (algo != CompressionAlgorithm::None) return std::string{};
        return std::string("B") + val;
    });
    AdvancedCacheManager::setDecompressFn([](const std::string& val, CompressionAlgorithm algo) {
        if (algo != CompressionAlgorithm::None || val.empty() || val[0] != 'B') return std::string{};
        return val.substr(1);
    });

    bridged.put("bridge-key", "bridge-value", "cold");
    auto val = bridged.get("bridge-key", "cold");

    AdvancedCacheManager::setCompressFn(nullptr);
    AdvancedCacheManager::setDecompressFn(nullptr);

    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "bridge-value");
}

TEST_F(ACMTest, UpdateExistingKey) {
    mgr.put("k2", "old", "hot");
    mgr.put("k2", "new", "hot");
    auto val = mgr.get("k2", "hot");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "new");
}

TEST_F(ACMTest, UnknownPartitionGetReturnsNullopt) {
    auto val = mgr.get("k", "nonexistent");
    EXPECT_FALSE(val.has_value());
}

TEST_F(ACMTest, UnknownPartitionPutNocrash) {
    EXPECT_NO_THROW(mgr.put("k", "v", "nonexistent"));
}

// --- contains / evict --------------------------------------------------------

TEST_F(ACMTest, ContainsTrueAfterPut) {
    mgr.put("ck", "cv", "hot");
    EXPECT_TRUE(mgr.contains("ck", "hot"));
}

TEST_F(ACMTest, ContainsFalseForMissing) {
    EXPECT_FALSE(mgr.contains("missing_key_xyz", "hot"));
}

TEST_F(ACMTest, EvictReturnsTrueWhenPresent) {
    mgr.put("ek", "ev", "hot");
    EXPECT_TRUE(mgr.evict("ek", "hot"));
    EXPECT_FALSE(mgr.contains("ek", "hot"));
}

TEST_F(ACMTest, EvictReturnsFalseWhenAbsent) {
    EXPECT_FALSE(mgr.evict("absent_key_xyz", "hot"));
}

// --- LRU eviction under capacity ---------------------------------------------

TEST_F(ACMTest, LRUEvictionUnderPressure) {
    // Use a tiny partition
    CacheConfig cfg;
    cfg.total_size_mb = 1;
    cfg.enable_bloom_filters = false;
    // capacity = ~4096 entries for 1MB — force with very small size_mb
    cfg.partitions.push_back({"tiny", 0, EvictionPolicy::LRU, false});
    // Override: create directly via create_partitions
    AdvancedCacheManager small;
    CacheConfig smcfg;
    smcfg.total_size_mb = 1;
    smcfg.enable_bloom_filters = false;
    smcfg.partitions.push_back({"p", 1, EvictionPolicy::LRU, false});
    small.create_partitions(smcfg);
    // Insert many entries to trigger eviction
    for (int i = 0; i < 200; ++i) {
        small.put("key" + std::to_string(i), "value", "p");
    }
    // At minimum the last inserted key should be present
    EXPECT_TRUE(small.contains("key199", "p"));
}

// --- bloom filter -----------------------------------------------------------

TEST_F(ACMTest, BloomFilterFastMissOnFreshPartition) {
    // Fresh partition — key never inserted — should miss
    auto val = mgr.get("never_inserted_bloom_key", "hot");
    EXPECT_FALSE(val.has_value());
}

// --- statistics --------------------------------------------------------------

TEST_F(ACMTest, StatsHitCountIncremented) {
    mgr.put("sk", "sv", "hot");
    mgr.get("sk", "hot");
    auto stats = mgr.get_partition_stats("hot");
    EXPECT_GE(stats.hits, 1u);
}

TEST_F(ACMTest, StatsMissCountIncremented) {
    mgr.get("definitely_absent_stat_test", "hot");
    auto stats = mgr.get_partition_stats("hot");
    EXPECT_GE(stats.misses, 1u);
}

TEST_F(ACMTest, StatsHitRateInRange) {
    mgr.put("hr", "v", "hot");
    mgr.get("hr", "hot");
    auto stats = mgr.get_partition_stats("hot");
    EXPECT_GE(stats.hit_rate, 0.0);
    EXPECT_LE(stats.hit_rate, 1.0);
}

TEST_F(ACMTest, ResetStatsZeroesHitMiss) {
    mgr.put("rk", "rv", "hot");
    mgr.get("rk", "hot");
    mgr.reset_stats();
    auto stats = mgr.get_partition_stats("hot");
    EXPECT_EQ(stats.hits, 0u);
    EXPECT_EQ(stats.misses, 0u);
}

// --- flush -------------------------------------------------------------------

TEST_F(ACMTest, FlushPartitionClearsEntries) {
    mgr.put("fk", "fv", "hot");
    mgr.flush_partition("hot");
    EXPECT_FALSE(mgr.contains("fk", "hot"));
}

TEST_F(ACMTest, FlushAllClearsAllPartitions) {
    mgr.put("fk1", "v1", "hot");
    mgr.put("fk2", "v2", "cold");
    mgr.flush_all();
    EXPECT_FALSE(mgr.contains("fk1", "hot"));
    EXPECT_FALSE(mgr.contains("fk2", "cold"));
}

// --- cache-oblivious scan ---------------------------------------------------

TEST_F(ACMTest, CacheObliviousScanAllElements) {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8};
    int sum = 0;
    AdvancedCacheManager::cache_oblivious_scan(
        data.begin(), data.end(), [&](int v){ sum += v; }, 3);
    EXPECT_EQ(sum, 36);
}

// --- thread safety -----------------------------------------------------------

TEST_F(ACMTest, ConcurrentPutGetThreadSafe) {
    constexpr int kThreads = 4;
    std::vector<std::thread> threads = {};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, t](){
            for (int i = 0; i < 50; ++i) {
                std::string key = "t" + std::to_string(t) + "k" + std::to_string(i);
                mgr.put(key, "val", "hot");
                mgr.get(key, "hot");
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
}

} // namespace
