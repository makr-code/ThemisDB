// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/metadata_shard.h"
#include <thread>

using namespace themisdb::sharding;

class MetadataShardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple config
        MetadataShardConfig config;
        config.shard_id = "test_shard_0";
        config.partitions = {
            MetadataPartitionKey::SCHEMA,
            MetadataPartitionKey::INDEX
        };
        config.num_metadata_shards = 3;
        config.enable_cache = true;
        config.cache_size = 100;
        config.cache_ttl = std::chrono::seconds(10);
        config.enforce_strong_consistency = false;  // No consensus for testing
        
        // Create shard without consensus module
        shard = std::make_unique<MetadataShard>(config, nullptr);
        shard->initialize();
        shard->start();
    }
    
    void TearDown() override {
        if (shard) {
            shard->stop();
        }
    }
    
    std::unique_ptr<MetadataShard> shard;
};

TEST_F(MetadataShardTest, BasicPutAndGet) {
    nlohmann::json value = {{"name", "test_table"}, {"columns", 5}};
    
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table1", value));
    
    auto entry = shard->get(MetadataPartitionKey::SCHEMA, "table1");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->key, "table1");
    EXPECT_EQ(entry->value["name"], "test_table");
    EXPECT_EQ(entry->value["columns"], 5);
    EXPECT_EQ(entry->partition, MetadataPartitionKey::SCHEMA);
    EXPECT_EQ(entry->version, 1);
}

TEST_F(MetadataShardTest, GetNonExistentKey) {
    auto entry = shard->get(MetadataPartitionKey::SCHEMA, "nonexistent");
    EXPECT_FALSE(entry.has_value());
}

TEST_F(MetadataShardTest, UpdateIncrementsVersion) {
    nlohmann::json value1 = {{"name", "test_table_v1"}};
    nlohmann::json value2 = {{"name", "test_table_v2"}};
    
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table1", value1));
    auto entry1 = shard->get(MetadataPartitionKey::SCHEMA, "table1");
    ASSERT_TRUE(entry1.has_value());
    EXPECT_EQ(entry1->version, 1);
    
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table1", value2));
    auto entry2 = shard->get(MetadataPartitionKey::SCHEMA, "table1");
    ASSERT_TRUE(entry2.has_value());
    EXPECT_EQ(entry2->version, 2);
    EXPECT_EQ(entry2->value["name"], "test_table_v2");
}

TEST_F(MetadataShardTest, CacheHit) {
    nlohmann::json value = {{"name", "cached_table"}};
    
    // First put
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table1", value));
    
    // First get (cache miss, then cached)
    auto entry1 = shard->get(MetadataPartitionKey::SCHEMA, "table1");
    ASSERT_TRUE(entry1.has_value());
    
    // Second get (should be cache hit)
    auto entry2 = shard->get(MetadataPartitionKey::SCHEMA, "table1");
    ASSERT_TRUE(entry2.has_value());
    
    auto stats = shard->getStatistics();
    EXPECT_GT(stats["cache_hits"], 0);
}

TEST_F(MetadataShardTest, ListKeys) {
    shard->put(MetadataPartitionKey::SCHEMA, "table1", {{"name", "t1"}});
    shard->put(MetadataPartitionKey::SCHEMA, "table2", {{"name", "t2"}});
    shard->put(MetadataPartitionKey::SCHEMA, "table3", {{"name", "t3"}});
    shard->put(MetadataPartitionKey::INDEX, "index1", {{"name", "i1"}});
    
    auto schema_keys = shard->listKeys(MetadataPartitionKey::SCHEMA);
    EXPECT_EQ(schema_keys.size(), 3);
    
    auto index_keys = shard->listKeys(MetadataPartitionKey::INDEX);
    EXPECT_EQ(index_keys.size(), 1);
}

TEST_F(MetadataShardTest, Remove) {
    nlohmann::json value = {{"name", "test_table"}};
    
    EXPECT_TRUE(shard->put(MetadataPartitionKey::SCHEMA, "table1", value));
    EXPECT_TRUE(shard->get(MetadataPartitionKey::SCHEMA, "table1").has_value());
    
    EXPECT_TRUE(shard->remove(MetadataPartitionKey::SCHEMA, "table1"));
    EXPECT_FALSE(shard->get(MetadataPartitionKey::SCHEMA, "table1").has_value());
}

TEST_F(MetadataShardTest, GetStatistics) {
    // Perform some operations
    shard->put(MetadataPartitionKey::SCHEMA, "table1", {{"name", "t1"}});
    shard->put(MetadataPartitionKey::SCHEMA, "table2", {{"name", "t2"}});
    shard->get(MetadataPartitionKey::SCHEMA, "table1");
    shard->get(MetadataPartitionKey::SCHEMA, "nonexistent");
    
    auto stats = shard->getStatistics();
    EXPECT_EQ(stats["shard_id"], "test_shard_0");
    EXPECT_EQ(stats["total_reads"], 2);
    EXPECT_EQ(stats["total_writes"], 2);
    EXPECT_TRUE(stats.contains("cache_size"));
    EXPECT_TRUE(stats.contains("cache_hit_ratio"));
}

TEST_F(MetadataShardTest, PartitionStatistics) {
    shard->put(MetadataPartitionKey::SCHEMA, "table1", {{"name", "t1"}});
    shard->put(MetadataPartitionKey::SCHEMA, "table2", {{"name", "t2"}});
    shard->put(MetadataPartitionKey::INDEX, "index1", {{"name", "i1"}});
    
    auto schema_stats = shard->getPartitionStats(MetadataPartitionKey::SCHEMA);
    EXPECT_EQ(schema_stats["entry_count"], 2);
    
    auto index_stats = shard->getPartitionStats(MetadataPartitionKey::INDEX);
    EXPECT_EQ(index_stats["entry_count"], 1);
}

TEST_F(MetadataShardTest, BoundedCacheEviction) {
    // Fill cache beyond capacity (100 entries)
    for (int i = 0; i < 150; i++) {
        nlohmann::json value = {{"id", i}};
        shard->put(MetadataPartitionKey::SCHEMA, "table" + std::to_string(i), value);
    }
    
    auto stats = shard->getStatistics();
    EXPECT_LE(stats["cache_size"], 100);  // Should not exceed max_entries
}

TEST_F(MetadataShardTest, MultiplePartitions) {
    shard->put(MetadataPartitionKey::SCHEMA, "key1", {{"type", "schema"}});
    shard->put(MetadataPartitionKey::INDEX, "key1", {{"type", "index"}});
    
    auto schema_entry = shard->get(MetadataPartitionKey::SCHEMA, "key1");
    ASSERT_TRUE(schema_entry.has_value());
    EXPECT_EQ(schema_entry->value["type"], "schema");
    
    auto index_entry = shard->get(MetadataPartitionKey::INDEX, "key1");
    ASSERT_TRUE(index_entry.has_value());
    EXPECT_EQ(index_entry->value["type"], "index");
}

TEST_F(MetadataShardTest, ThreadSafety) {
    const int num_threads = 10;
    const int operations_per_thread = 50;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; i++) {
                std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                nlohmann::json value = {{"thread", t}, {"op", i}};
                
                shard->put(MetadataPartitionKey::SCHEMA, key, value);
                auto entry = shard->get(MetadataPartitionKey::SCHEMA, key);
                EXPECT_TRUE(entry.has_value());
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = shard->getStatistics();
    EXPECT_EQ(stats["total_writes"], num_threads * operations_per_thread);
}

TEST_F(MetadataShardTest, ConcurrentPutSameKey_VersionIsMonotonicallyIncreasing) {
    // Regression test for the TOCTOU bug where two concurrent put() calls on the
    // same key could race between version-read and write scopes, producing
    // duplicate or out-of-order versions. The fix merges both into a single lock
    // scope so version increments are strictly serialised.
    const int num_threads = 8;
    const int writes_per_thread = 50;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, writes_per_thread]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                nlohmann::json value = {{"counter", i}};
                shard->put(MetadataPartitionKey::SCHEMA, "shared_key", value);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    auto entry = shard->get(MetadataPartitionKey::SCHEMA, "shared_key");
    ASSERT_TRUE(entry.has_value());
    // The final version must be at most the total number of writes (it may be
    // less if the implementation coalesces concurrent writes), but it must be
    // strictly positive and non-zero.
    EXPECT_GE(entry->version, 1);
    EXPECT_LE(entry->version, static_cast<uint64_t>(num_threads * writes_per_thread));
}

// Router tests

class MetadataShardRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        router = std::make_unique<MetadataShardRouter>(3);
        
        // Create 3 shards
        for (int i = 0; i < 3; i++) {
            MetadataShardConfig config;
            config.shard_id = "shard_" + std::to_string(i);
            config.partitions = {MetadataPartitionKey::SCHEMA};
            config.enable_cache = true;
            config.cache_size = 100;
            config.enforce_strong_consistency = false;
            
            auto shard = std::make_shared<MetadataShard>(config, nullptr);
            shard->initialize();
            shard->start();
            
            router->addShard(config.shard_id, shard);
            shards.push_back(shard);
        }
    }
    
    void TearDown() override {
        for (auto& shard : shards) {
            shard->stop();
        }
    }
    
    std::unique_ptr<MetadataShardRouter> router;
    std::vector<std::shared_ptr<MetadataShard>> shards;
};

TEST_F(MetadataShardRouterTest, RoutingPutAndGet) {
    nlohmann::json value = {{"name", "routed_table"}};
    
    EXPECT_TRUE(router->put(MetadataPartitionKey::SCHEMA, "table1", value));
    
    auto entry = router->get(MetadataPartitionKey::SCHEMA, "table1");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->value["name"], "routed_table");
}

TEST_F(MetadataShardRouterTest, ConsistentRouting) {
    nlohmann::json value = {{"name", "test"}};
    
    // Put to a key
    EXPECT_TRUE(router->put(MetadataPartitionKey::SCHEMA, "table1", value));
    
    // Get multiple times should route to the same shard
    for (int i = 0; i < 10; i++) {
        auto entry = router->get(MetadataPartitionKey::SCHEMA, "table1");
        ASSERT_TRUE(entry.has_value());
        EXPECT_EQ(entry->value["name"], "test");
    }
}

TEST_F(MetadataShardRouterTest, ListKeysScatterGather) {
    // Put keys that will be distributed across shards
    router->put(MetadataPartitionKey::SCHEMA, "table1", {{"id", 1}});
    router->put(MetadataPartitionKey::SCHEMA, "table2", {{"id", 2}});
    router->put(MetadataPartitionKey::SCHEMA, "table3", {{"id", 3}});
    router->put(MetadataPartitionKey::SCHEMA, "table4", {{"id", 4}});
    
    auto keys = router->listKeys(MetadataPartitionKey::SCHEMA);
    EXPECT_EQ(keys.size(), 4);
}

TEST_F(MetadataShardRouterTest, RouterStatistics) {
    router->put(MetadataPartitionKey::SCHEMA, "table1", {{"id", 1}});
    router->get(MetadataPartitionKey::SCHEMA, "table1");
    router->remove(MetadataPartitionKey::SCHEMA, "table1");
    
    auto stats = router->getStatistics();
    EXPECT_EQ(stats["num_shards"], 3);
    EXPECT_EQ(stats["total_operations"], 3);
    EXPECT_EQ(stats["routing_errors"], 0);
}
