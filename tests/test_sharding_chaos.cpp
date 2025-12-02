/**
 * ThemisDB Sharding Chaos Tests
 * 
 * Chaos Engineering Tests für das Sharding-Subsystem.
 * Diese Tests simulieren Fehlerbedingungen und prüfen die Resilienz.
 * 
 * Test-Szenarien:
 * - Zufällige Shard-Ausfälle
 * - Netzwerk-Partitionen (simuliert)
 * - Certificate Revocation
 * - Rebalancing unter Last
 * - Split-Brain Szenarien
 * - Cascading Failures
 */

#include <gtest/gtest.h>
#include "sharding/urn.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include <memory>
#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <chrono>
#include <set>

using namespace themis::sharding;

// ============================================================================
// Chaos Test Fixture
// ============================================================================

class ShardingChaosTest : public ::testing::Test {
protected:
    void SetUp() override {
        setupCluster(7);  // 7-node cluster for chaos tests
        rng_.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    void TearDown() override {
        stop_chaos_ = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        topology_.reset();
        hash_ring_.reset();
    }
    
    void setupCluster(size_t shard_count) {
        ShardTopology::Config config;
        config.cluster_name = "chaos-test-cluster";
        config.enable_health_checks = false;
        topology_ = std::make_shared<ShardTopology>(config);
        hash_ring_ = std::make_shared<ConsistentHashRing>();
        
        for (size_t i = 1; i <= shard_count; ++i) {
            std::string shard_id = "shard_" + std::to_string(i);
            
            ShardInfo shard;
            shard.shard_id = shard_id;
            shard.primary_endpoint = "localhost:" + std::to_string(8080 + i);
            shard.datacenter = "dc" + std::to_string((i % 3) + 1);
            shard.is_healthy = true;
            shard.capabilities = {"read", "write", "replicate"};
            shard.cert_serial = "CERT-" + std::to_string(i);
            
            topology_->addShard(shard);
            hash_ring_->addShard(shard_id, 150);
            
            shard_ids_.push_back(shard_id);
        }
    }
    
    // Randomly fail a shard
    std::string failRandomShard() {
        std::uniform_int_distribution<size_t> dist(0, shard_ids_.size() - 1);
        std::string shard_id = shard_ids_[dist(rng_)];
        topology_->updateHealth(shard_id, false);
        return shard_id;
    }
    
    // Recover a shard
    void recoverShard(const std::string& shard_id) {
        topology_->updateHealth(shard_id, true);
    }
    
    // Fail multiple shards
    std::vector<std::string> failMultipleShards(size_t count) {
        std::vector<std::string> failed;
        std::set<std::string> failed_set;
        
        while (failed.size() < count && failed.size() < shard_ids_.size()) {
            std::uniform_int_distribution<size_t> dist(0, shard_ids_.size() - 1);
            std::string shard_id = shard_ids_[dist(rng_)];
            
            if (failed_set.insert(shard_id).second) {
                topology_->updateHealth(shard_id, false);
                failed.push_back(shard_id);
            }
        }
        
        return failed;
    }
    
    // Recover all shards
    void recoverAllShards() {
        for (const auto& shard_id : shard_ids_) {
            topology_->updateHealth(shard_id, true);
        }
    }
    
    // Generate random URN
    std::optional<URN> generateRandomURN() {
        std::uniform_int_distribution<uint32_t> dist(1, 1000000);
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%012x",
            dist(rng_), dist(rng_) & 0xFFFF, 0x4000 | (dist(rng_) & 0x0FFF),
            0x8000 | (dist(rng_) & 0x3FFF), dist(rng_));
        
        return URN::parse("urn:themis:relational:chaos:test:" + std::string(uuid));
    }
    
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::vector<std::string> shard_ids_;
    std::mt19937 rng_;
    std::atomic<bool> stop_chaos_{false};
};

// ============================================================================
// Random Shard Failure Tests
// ============================================================================

TEST_F(ShardingChaosTest, SingleShardFailure) {
    // Fail a random shard
    std::string failed = failRandomShard();
    
    // Cluster should still have 6 healthy shards
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 6u);
    
    // All URNs should still resolve (to healthy shards)
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    
    int resolved = 0;
    int failed_resolve = 0;
    
    for (int i = 0; i < 100; ++i) {
        auto urn = generateRandomURN();
        if (urn) {
            auto shard = resolver.resolvePrimary(*urn);
            if (shard && shard->is_healthy) {
                resolved++;
            } else {
                failed_resolve++;
            }
        }
    }
    
    // Some resolutions may fail if they route to the failed shard
    // But the system should still function
    EXPECT_GT(resolved, 0);
    
    // Recover
    recoverShard(failed);
    EXPECT_EQ(topology_->getHealthyShards().size(), 7u);
}

TEST_F(ShardingChaosTest, MultipleShardFailures) {
    // Fail 3 shards (minority)
    auto failed = failMultipleShards(3);
    EXPECT_EQ(failed.size(), 3u);
    
    // Should have 4 healthy (majority)
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 4u);
    
    // Verify failed shards are not in healthy list
    std::set<std::string> healthy_ids;
    for (const auto& s : healthy) {
        healthy_ids.insert(s.shard_id);
    }
    
    for (const auto& f : failed) {
        EXPECT_EQ(healthy_ids.count(f), 0u) << "Failed shard " << f << " in healthy list";
    }
    
    recoverAllShards();
}

TEST_F(ShardingChaosTest, MajorityFailure) {
    // Fail 5 shards (majority)
    auto failed = failMultipleShards(5);
    
    // Should have only 2 healthy
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 2u);
    
    // System degraded but should not crash
    URNResolver resolver(topology_, hash_ring_, healthy[0].shard_id);
    
    // Can still resolve URNs
    auto urn = generateRandomURN();
    ASSERT_TRUE(urn.has_value());
    
    auto shard = resolver.resolvePrimary(*urn);
    // May or may not resolve depending on target
    // Key point: no crash
    
    recoverAllShards();
}

TEST_F(ShardingChaosTest, AllShardsFailure) {
    // Fail all shards
    for (const auto& shard_id : shard_ids_) {
        topology_->updateHealth(shard_id, false);
    }
    
    // No healthy shards
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 0u);
    
    // Resolver should handle gracefully (return nullopt, not crash)
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    auto urn = generateRandomURN();
    ASSERT_TRUE(urn.has_value());
    
    // This may return unhealthy shard or nullopt depending on implementation
    // Key: no crash
    auto shard = resolver.resolvePrimary(*urn);
    
    recoverAllShards();
    EXPECT_EQ(topology_->getHealthyShards().size(), 7u);
}

// ============================================================================
// Flapping Shard Tests
// ============================================================================

TEST_F(ShardingChaosTest, ShardFlapping) {
    // Simulate shard flapping (going up/down rapidly)
    std::string flapping_shard = "shard_3";
    
    std::atomic<int> resolve_count{0};
    std::atomic<int> error_count{0};
    
    // Reader thread
    auto reader = [&]() {
        URNResolver resolver(topology_, hash_ring_, "shard_1");
        while (!stop_chaos_) {
            auto urn = generateRandomURN();
            if (urn) {
                auto shard = resolver.resolvePrimary(*urn);
                if (shard) {
                    resolve_count++;
                } else {
                    error_count++;
                }
            }
        }
    };
    
    // Flapper thread
    auto flapper = [&]() {
        for (int i = 0; i < 20 && !stop_chaos_; ++i) {
            topology_->updateHealth(flapping_shard, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            topology_->updateHealth(flapping_shard, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };
    
    std::thread reader_thread(reader);
    std::thread flapper_thread(flapper);
    
    flapper_thread.join();
    stop_chaos_ = true;
    reader_thread.join();
    
    // Should have many successful resolutions despite flapping
    EXPECT_GT(resolve_count.load(), 100);
    
    // Final state should be healthy
    auto shard = topology_->getShard(flapping_shard);
    ASSERT_TRUE(shard.has_value());
    EXPECT_TRUE(shard->is_healthy);
}

// ============================================================================
// Network Partition Simulation Tests
// ============================================================================

TEST_F(ShardingChaosTest, DatacenterPartition) {
    // Simulate DC1 partition (shards 1, 4, 7 are in DC1)
    std::vector<std::string> dc1_shards;
    for (const auto& shard_id : shard_ids_) {
        auto shard = topology_->getShard(shard_id);
        if (shard && shard->datacenter == "dc1") {
            dc1_shards.push_back(shard_id);
            topology_->updateHealth(shard_id, false);
        }
    }
    
    // DC1 shards should be unhealthy
    for (const auto& shard_id : dc1_shards) {
        auto shard = topology_->getShard(shard_id);
        ASSERT_TRUE(shard.has_value());
        EXPECT_FALSE(shard->is_healthy);
    }
    
    // Other DCs should be healthy
    auto healthy = topology_->getHealthyShards();
    for (const auto& shard : healthy) {
        EXPECT_NE(shard.datacenter, "dc1");
    }
    
    recoverAllShards();
}

TEST_F(ShardingChaosTest, SplitBrainScenario) {
    // Simulate split-brain: partition cluster into two groups
    
    // Group A: shards 1, 2, 3
    // Group B: shards 4, 5, 6, 7
    
    std::set<std::string> group_a = {"shard_1", "shard_2", "shard_3"};
    std::set<std::string> group_b = {"shard_4", "shard_5", "shard_6", "shard_7"};
    
    // From Group A's perspective, Group B is down
    for (const auto& shard_id : group_b) {
        topology_->updateHealth(shard_id, false);
    }
    
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 3u);
    
    // Only Group A shards visible
    for (const auto& shard : healthy) {
        EXPECT_TRUE(group_a.count(shard.shard_id) > 0);
    }
    
    // Partition heals
    recoverAllShards();
    EXPECT_EQ(topology_->getHealthyShards().size(), 7u);
}

// ============================================================================
// Dynamic Topology Change Tests
// ============================================================================

TEST_F(ShardingChaosTest, RapidShardAddRemove) {
    std::atomic<int> resolve_count{0};
    std::atomic<int> error_count{0};
    
    // Reader thread
    auto reader = [&]() {
        URNResolver resolver(topology_, hash_ring_, "shard_1");
        while (!stop_chaos_) {
            auto urn = generateRandomURN();
            if (urn) {
                try {
                    auto shard = resolver.resolvePrimary(*urn);
                    if (shard) {
                        resolve_count++;
                    } else {
                        error_count++;
                    }
                } catch (...) {
                    error_count++;
                }
            }
        }
    };
    
    // Modifier thread
    auto modifier = [&]() {
        for (int i = 0; i < 10 && !stop_chaos_; ++i) {
            std::string temp_shard = "temp_shard_" + std::to_string(i);
            
            // Add
            ShardInfo shard;
            shard.shard_id = temp_shard;
            shard.primary_endpoint = "localhost:9000";
            shard.is_healthy = true;
            topology_->addShard(shard);
            hash_ring_->addShard(temp_shard, 50);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Remove
            topology_->removeShard(temp_shard);
            hash_ring_->removeShard(temp_shard);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::thread reader_thread(reader);
    std::thread modifier_thread(modifier);
    
    modifier_thread.join();
    stop_chaos_ = true;
    reader_thread.join();
    
    // Should have many successful resolutions
    EXPECT_GT(resolve_count.load(), 100);
    
    // Original topology should be intact
    EXPECT_EQ(hash_ring_->getShardCount(), 7u);
}

// ============================================================================
// Cascading Failure Tests
// ============================================================================

TEST_F(ShardingChaosTest, CascadingFailure) {
    // Simulate cascading failure: fail shards one by one
    std::vector<std::string> failure_order;
    
    for (size_t i = 0; i < shard_ids_.size(); ++i) {
        std::string shard_id = shard_ids_[i];
        topology_->updateHealth(shard_id, false);
        failure_order.push_back(shard_id);
        
        // Check remaining healthy count
        auto healthy = topology_->getHealthyShards();
        EXPECT_EQ(healthy.size(), shard_ids_.size() - i - 1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // All failed
    EXPECT_EQ(topology_->getHealthyShards().size(), 0u);
    
    // Cascading recovery
    for (auto it = failure_order.rbegin(); it != failure_order.rend(); ++it) {
        topology_->updateHealth(*it, true);
        
        auto healthy = topology_->getHealthyShards();
        EXPECT_GT(healthy.size(), 0u);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // All recovered
    EXPECT_EQ(topology_->getHealthyShards().size(), 7u);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(ShardingChaosTest, HighConcurrencyStress) {
    std::atomic<int> operations{0};
    std::atomic<int> errors{0};
    
    const int NUM_THREADS = 16;
    const int OPS_PER_THREAD = 500;
    
    auto worker = [&]() {
        URNResolver resolver(topology_, hash_ring_, "shard_1");
        
        for (int i = 0; i < OPS_PER_THREAD && !stop_chaos_; ++i) {
            auto urn = generateRandomURN();
            if (urn) {
                try {
                    auto shard = resolver.resolvePrimary(*urn);
                    if (shard) {
                        operations++;
                    } else {
                        errors++;
                    }
                } catch (...) {
                    errors++;
                }
            }
        }
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int total_ops = operations.load();
    double ops_per_sec = (total_ops * 1000.0) / duration.count();
    
    std::cout << "Completed " << total_ops << " ops in " << duration.count() << "ms" << std::endl;
    std::cout << "Throughput: " << ops_per_sec << " ops/sec" << std::endl;
    
    // Should complete all operations
    EXPECT_EQ(total_ops + errors.load(), NUM_THREADS * OPS_PER_THREAD);
    
    // High success rate
    EXPECT_GT(total_ops, NUM_THREADS * OPS_PER_THREAD * 0.99);
}

TEST_F(ShardingChaosTest, ChaosMonkey) {
    // Full chaos: random failures while operations run
    std::atomic<int> resolve_count{0};
    std::atomic<int> error_count{0};
    
    // Multiple reader threads
    auto reader = [&]() {
        URNResolver resolver(topology_, hash_ring_, "shard_1");
        while (!stop_chaos_) {
            auto urn = generateRandomURN();
            if (urn) {
                try {
                    auto shard = resolver.resolvePrimary(*urn);
                    if (shard) {
                        resolve_count++;
                    } else {
                        error_count++;
                    }
                } catch (...) {
                    error_count++;
                }
            }
        }
    };
    
    // Chaos monkey thread
    auto chaos_monkey = [&]() {
        std::uniform_int_distribution<int> action_dist(0, 2);
        std::uniform_int_distribution<size_t> shard_dist(0, shard_ids_.size() - 1);
        
        for (int i = 0; i < 50 && !stop_chaos_; ++i) {
            int action = action_dist(rng_);
            std::string shard_id = shard_ids_[shard_dist(rng_)];
            
            switch (action) {
                case 0:  // Fail shard
                    topology_->updateHealth(shard_id, false);
                    break;
                case 1:  // Recover shard
                    topology_->updateHealth(shard_id, true);
                    break;
                case 2:  // Toggle
                    {
                        auto shard = topology_->getShard(shard_id);
                        if (shard) {
                            topology_->updateHealth(shard_id, !shard->is_healthy);
                        }
                    }
                    break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };
    
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back(reader);
    }
    
    std::thread monkey_thread(chaos_monkey);
    
    monkey_thread.join();
    stop_chaos_ = true;
    
    for (auto& t : readers) {
        t.join();
    }
    
    // Should have many operations despite chaos
    EXPECT_GT(resolve_count.load(), 500);
    
    // Clean up
    recoverAllShards();
}

// ============================================================================
// Recovery Tests
// ============================================================================

TEST_F(ShardingChaosTest, QuickRecovery) {
    // Measure recovery time
    
    // Fail all
    for (const auto& shard_id : shard_ids_) {
        topology_->updateHealth(shard_id, false);
    }
    
    EXPECT_EQ(topology_->getHealthyShards().size(), 0u);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Recover all
    recoverAllShards();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto recovery_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_EQ(topology_->getHealthyShards().size(), 7u);
    
    std::cout << "Recovery time: " << recovery_time.count() << " microseconds" << std::endl;
    
    // Should be fast (< 1ms for in-memory)
    EXPECT_LT(recovery_time.count(), 1000);
}

TEST_F(ShardingChaosTest, PartialRecovery) {
    // Fail 5, recover 3
    auto failed = failMultipleShards(5);
    EXPECT_EQ(topology_->getHealthyShards().size(), 2u);
    
    // Recover first 3
    for (size_t i = 0; i < 3 && i < failed.size(); ++i) {
        recoverShard(failed[i]);
    }
    
    EXPECT_EQ(topology_->getHealthyShards().size(), 5u);
    
    // Full recover
    recoverAllShards();
    EXPECT_EQ(topology_->getHealthyShards().size(), 7u);
}
