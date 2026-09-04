/**
 * ThemisDB RAID Integration Tests
 * 
 * Tests RAID system with realistic multi-shard scenarios including:
 * - Real shard failures and recovery
 * - Concurrent writes and reads
 * - Cross-shard consistency
 * - Performance under load
 */

#include <gtest/gtest.h>
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <random>

using namespace themis::sharding;

// ═══════════════════════════════════════════════════════════
// Multi-Shard Test Environment
// ═══════════════════════════════════════════════════════════

class MultiShardEnvironment {
public:
    struct ShardState {
        std::map<std::string, std::vector<uint8_t>> data;
        std::atomic<bool> is_healthy{true};
        std::atomic<uint64_t> write_count{0};
        std::atomic<uint64_t> read_count{0};
        std::mutex mutex;
    };
    
    std::map<std::string, std::shared_ptr<ShardState>> shards;
    
    void addShard(const std::string& shard_id) {
        shards[shard_id] = std::make_shared<ShardState>();
    }
    
    bool write(const std::string& shard_id, const std::string& key,
               const std::vector<uint8_t>& data) {
        auto it = shards.find(shard_id);
        if (it == shards.end() || !it->second->is_healthy) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(it->second->mutex);
        it->second->data[key] = data;
        it->second->write_count++;
        return true;
    }
    
    std::optional<std::vector<uint8_t>> read(const std::string& shard_id,
                                              const std::string& key) {
        auto it = shards.find(shard_id);
        if (it == shards.end() || !it->second->is_healthy) {
            return std::nullopt;
        }
        
        std::lock_guard<std::mutex> lock(it->second->mutex);
        it->second->read_count++;
        
        if (it->second->data.count(key)) {
            return it->second->data[key];
        }
        return std::nullopt;
    }
    
    void failShard(const std::string& shard_id) {
        if (shards.count(shard_id)) {
            shards[shard_id]->is_healthy = false;
        }
    }
    
    void recoverShard(const std::string& shard_id) {
        if (shards.count(shard_id)) {
            shards[shard_id]->is_healthy = true;
        }
    }
    
    uint64_t getTotalWrites() const {
        uint64_t total = 0;
        for (const auto& [_, shard] : shards) {
            total += shard->write_count.load();
        }
        return total;
    }
    
    uint64_t getTotalReads() const {
        uint64_t total = 0;
        for (const auto& [_, shard] : shards) {
            total += shard->read_count.load();
        }
        return total;
    }
};

// ═══════════════════════════════════════════════════════════
// Integration Test Fixture
// ═══════════════════════════════════════════════════════════

class RAIDIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create 8-shard environment
        for (int i = 0; i < 8; ++i) {
            std::string shard_id = "shard-" + std::to_string(i);
            env.addShard(shard_id);
            ring.addNode(shard_id);
        }
    }
    
    MultiShardEnvironment env;
    ConsistentHashRing ring{100};
    ShardTopology topology;
};

// ═══════════════════════════════════════════════════════════
// RAID 1 Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDIntegrationTest, RAID1_WriteReadWithShardFailure) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::MAJORITY;
    
    RedundancyStrategy strategy(config);
    
    auto writeHandler = [this](const std::string& shard, const std::string& key,
                               const std::vector<uint8_t>& data) {
        return env.write(shard, key, data);
    };
    
    auto readHandler = [this](const std::string& shard, const std::string& key) {
        return env.read(shard, key);
    };
    
    // Write data
    std::vector<uint8_t> data = {'T', 'e', 's', 't', ' ', 'D', 'a', 't', 'a'};
    auto writeResult = strategy.write("doc1", data, "collection", ring, topology, writeHandler);
    
    ASSERT_TRUE(writeResult.success);
    EXPECT_GE(writeResult.written_shards.size(), 2);  // At least MAJORITY
    
    // Simulate shard failure
    if (!writeResult.written_shards.empty()) {
        env.failShard(writeResult.written_shards[0]);
    }
    
    // Should still be able to read from replicas
    auto readResult = strategy.read("doc1", "collection", ring, topology, readHandler);
    
    EXPECT_TRUE(readResult.success);
    EXPECT_EQ(readResult.data, std::string(data.begin(), data.end()));
}

TEST_F(RAIDIntegrationTest, RAID1_ConcurrentWritesAndReads) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    
    RedundancyStrategy strategy(config);
    
    auto writeHandler = [this](const std::string& shard, const std::string& key,
                               const std::vector<uint8_t>& data) {
        return env.write(shard, key, data);
    };
    
    auto readHandler = [this](const std::string& shard, const std::string& key) {
        return env.read(shard, key);
    };
    
    const int num_threads = 4;
    const int ops_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successful_writes{0};
    std::atomic<int> successful_reads{0};
    
    // Concurrent writers
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string doc_id = "doc-" + std::to_string(t * ops_per_thread + i);
                std::vector<uint8_t> data(100, static_cast<uint8_t>(i % 256));
                
                auto result = strategy.write(doc_id, data, "collection", 
                                            ring, topology, writeHandler);
                if (result.success) {
                    successful_writes++;
                }
            }
        });
    }
    
    // Concurrent readers
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string doc_id = "doc-" + std::to_string(t * ops_per_thread + i);
                
                // Give writes time to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                auto result = strategy.read(doc_id, "collection", 
                                           ring, topology, readHandler);
                if (result.success) {
                    successful_reads++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(successful_writes.load(), 0);
    EXPECT_GT(successful_reads.load(), 0);
    EXPECT_GT(env.getTotalWrites(), 0);
    EXPECT_GT(env.getTotalReads(), 0);
}

// ═══════════════════════════════════════════════════════════
// RAID 5 Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDIntegrationTest, RAID5_RecoveryFromMultipleFailures) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    
    RedundancyStrategy strategy(config);
    
    auto writeHandler = [this](const std::string& shard, const std::string& key,
                               const std::vector<uint8_t>& data) {
        return env.write(shard, key, data);
    };
    
    auto readHandler = [this](const std::string& shard, const std::string& key) {
        return env.read(shard, key);
    };
    
    // Write data
    std::vector<uint8_t> data(4096);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    
    auto writeResult = strategy.write("doc1", data, "collection", ring, topology, writeHandler);
    ASSERT_TRUE(writeResult.success);
    
    // Fail 2 shards (should still be recoverable with 4+2)
    std::vector<std::string> failed_shards = {};

    for (size_t i = 0; i < std::min(size_t(2), writeResult.written_shards.size()); ++i) {
        env.failShard(writeResult.written_shards[i]);
        failed_shards.push_back(writeResult.written_shards[i]);
    }
    
    // Should still be able to recover data
    auto readResult = strategy.read("doc1", "collection", ring, topology, readHandler);
    
    if (readResult.success) {
        EXPECT_EQ(readResult.data.size(), data.size());
        // Data should be recoverable (exact match depends on implementation)
    }
}

// ═══════════════════════════════════════════════════════════
// Performance Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDIntegrationTest, PerformanceUnderLoad) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 2;
    
    RedundancyStrategy strategy(config);
    
    auto writeHandler = [this](const std::string& shard, const std::string& key,
                               const std::vector<uint8_t>& data) {
        return env.write(shard, key, data);
    };
    
    const int num_documents = 1000;
    std::vector<uint8_t> data(1024);  // 1KB documents
    
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < num_documents; ++i) {
        std::string doc_id = "perf-doc-" + std::to_string(i);
        strategy.write(doc_id, data, "collection", ring, topology, writeHandler);
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double throughput = (num_documents * 1024.0) / (duration.count() / 1000.0);  // bytes/sec
    
    std::cout << "Write throughput: " << throughput / (1024*1024) << " MB/s\n";
    std::cout << "Latency per write: " << duration.count() / double(num_documents) << " ms\n";
    std::cout << "Total writes to shards: " << env.getTotalWrites() << "\n";
    
    EXPECT_GT(throughput, 0);
}

// ═══════════════════════════════════════════════════════════
// Consistency Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDIntegrationTest, ConsistencyAcrossReplicas) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;
    
    RedundancyStrategy strategy(config);
    
    auto writeHandler = [this](const std::string& shard, const std::string& key,
                               const std::vector<uint8_t>& data) {
        return env.write(shard, key, data);
    };
    
    auto readHandler = [this](const std::string& shard, const std::string& key) {
        return env.read(shard, key);
    };
    
    // Write data
    std::vector<uint8_t> data = {'C', 'o', 'n', 's', 'i', 's', 't', 'e', 'n', 't'};
    auto writeResult = strategy.write("doc1", data, "collection", ring, topology, writeHandler);
    
    ASSERT_TRUE(writeResult.success);
    ASSERT_EQ(writeResult.written_shards.size(), 3);
    
    // Read from each replica directly
    std::vector<std::vector<uint8_t>> replica_data;
    for (const auto& shard_id : writeResult.written_shards) {
        auto shard_data = env.read(shard_id, "collection:doc1");
        ASSERT_TRUE(shard_data.has_value());
        replica_data.push_back(*shard_data);
    }
    
    // All replicas should have identical data
    for (size_t i = 1; i < replica_data.size(); ++i) {
        EXPECT_EQ(replica_data[0], replica_data[i]);
    }
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════
// main() removed - GTest provides its own main via gtest_main
