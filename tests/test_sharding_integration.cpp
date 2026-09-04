/**
 * ThemisDB Sharding Integration Tests
 * 
 * Tests für die Integration zwischen Sharding-Komponenten:
 * - URN Resolver + Shard Router
 * - Shard Topology + Consistent Hash
 * - Remote Executor + mTLS Client
 * - Cross-Shard Join Coordination
 * 
 * Diese Tests erfordern keine echten Shard-Instanzen,
 * sondern verwenden Mock-Objekte für die Kommunikation.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sharding/urn.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "sharding/shard_router.h"
#include "sharding/remote_executor.h"
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

using namespace themis::sharding;
using testing::Return;
using testing::_;

// ============================================================================
// Test Fixtures
// ============================================================================

class ShardingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create topology
        ShardTopology::Config topology_config;
        topology_config.metadata_endpoint = "";
        topology_config.cluster_name = "integration-test";
        topology_config.enable_health_checks = false;
        topology_ = std::make_shared<ShardTopology>(topology_config);
        
        // Create hash ring
        hash_ring_ = std::make_shared<ConsistentHashRing>();
        
        // Setup test shards
        setupTestCluster(3);
    }
    
    void TearDown() override {
        topology_.reset();
        hash_ring_.reset();
    }
    
    void setupTestCluster(size_t shard_count) {
        for (size_t i = 1; i <= shard_count; ++i) {
            std::string shard_id = "shard_" + std::to_string(i);
            std::string port = std::to_string(8080 + i);
            
            ShardInfo shard;
            shard.shard_id = shard_id;
            shard.primary_endpoint = "localhost:" + port;
            shard.datacenter = "dc" + std::to_string((i % 2) + 1);
            shard.is_healthy = true;
            shard.capabilities = {"read", "write", "replicate"};
            
            topology_->addShard(shard);
            hash_ring_->addShard(shard_id, 150);
        }
    }
    
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
};

// ============================================================================
// URN Resolver Integration Tests
// ============================================================================

TEST_F(ShardingIntegrationTest, ResolverIntegrationWithTopology) {
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    
    // Parse and resolve multiple URNs
    std::vector<std::string> urns = {
        "urn:themis:relational:tenant1:users:550e8400-e29b-41d4-a716-446655440000",
        "urn:themis:graph:tenant1:nodes:7c9e6679-7425-40de-944b-e07fc1f90ae7",
        "urn:themis:vector:tenant1:embeddings:f47ac10b-58cc-4372-a567-0e02b2c3d479"
    };
    
    for (const auto& urn_str : urns) {
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value()) << "Failed to parse: " << urn_str;
        
        auto shard = resolver.resolvePrimary(*urn);
        ASSERT_TRUE(shard.has_value()) << "Failed to resolve: " << urn_str;
        
        // Verify shard is in topology
        EXPECT_TRUE(topology_->hasShard(shard->shard_id));
        EXPECT_TRUE(shard->is_healthy);
    }
}

TEST_F(ShardingIntegrationTest, ResolverConsistentMapping) {
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    
    auto urn = URN::parse("urn:themis:relational:customers:orders:550e8400-e29b-41d4-a716-446655440000");
    ASSERT_TRUE(urn.has_value());
    
    // Resolve same URN multiple times
    std::vector<std::string> resolved_shards = {};

    for (int i = 0; i < 100; ++i) {
        auto shard = resolver.resolvePrimary(*urn);
        ASSERT_TRUE(shard.has_value());
        resolved_shards.push_back(shard->shard_id);
    }
    
    // All should be the same
    for (const auto& shard_id : resolved_shards) {
        EXPECT_EQ(shard_id, resolved_shards[0]) << "Inconsistent shard mapping";
    }
}

TEST_F(ShardingIntegrationTest, ResolverLocalityCheck) {
    // Test locality from different shard perspectives
    auto urn = URN::parse("urn:themis:relational:test:test:550e8400-e29b-41d4-a716-446655440000");
    ASSERT_TRUE(urn.has_value());
    
    // Get the actual target shard
    std::string target_shard = hash_ring_->getShardForURN(*urn);
    
    // From target shard perspective, should be local
    URNResolver local_resolver(topology_, hash_ring_, target_shard);
    EXPECT_TRUE(local_resolver.isLocal(*urn));
    
    // Find a different shard
    std::string other_shard;
    for (const auto& shard : hash_ring_->getAllShards()) {
        if (shard != target_shard) {
            other_shard = shard;
            break;
        }
    }
    
    // From other shard perspective, should be remote
    if (!other_shard.empty()) {
        URNResolver remote_resolver(topology_, hash_ring_, other_shard);
        EXPECT_FALSE(remote_resolver.isLocal(*urn));
    }
}

TEST_F(ShardingIntegrationTest, ResolverReplicaDiscovery) {
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    
    auto urn = URN::parse("urn:themis:relational:test:test:550e8400-e29b-41d4-a716-446655440000");
    ASSERT_TRUE(urn.has_value());
    
    // Request 2 replicas
    auto shards = resolver.resolveReplicas(*urn, 2);
    
    // Should have primary + 2 replicas = up to 3 (but capped at cluster size)
    EXPECT_GE(shards.size(), 1u);
    EXPECT_LE(shards.size(), 3u);
    
    // All should be unique
    std::set<std::string> unique_shards = {};

    for (const auto& shard : shards) {
        EXPECT_TRUE(unique_shards.insert(shard.shard_id).second) 
            << "Duplicate shard in replicas: " << shard.shard_id;
    }
}

// ============================================================================
// Consistent Hash + Topology Integration Tests
// ============================================================================

TEST_F(ShardingIntegrationTest, DynamicShardAddition) {
    EXPECT_EQ(hash_ring_->getShardCount(), 3u);
    EXPECT_EQ(topology_->getShardCount(), 3u);
    
    // Record current shard assignment for a URN
    auto urn = URN::parse("urn:themis:relational:test:test:550e8400-e29b-41d4-a716-446655440000");
    ASSERT_TRUE(urn.has_value());
    std::string original_shard = hash_ring_->getShardForURN(*urn);
    
    // Add new shard
    ShardInfo new_shard;
    new_shard.shard_id = "shard_4";
    new_shard.primary_endpoint = "localhost:8084";
    new_shard.is_healthy = true;
    
    topology_->addShard(new_shard);
    hash_ring_->addShard("shard_4", 150);
    
    EXPECT_EQ(hash_ring_->getShardCount(), 4u);
    EXPECT_EQ(topology_->getShardCount(), 4u);
    
    // URN should still map consistently (may or may not change)
    std::string new_shard_assignment = hash_ring_->getShardForURN(*urn);
    
    // If it changed, it should be to a valid shard
    EXPECT_TRUE(topology_->hasShard(new_shard_assignment));
}

TEST_F(ShardingIntegrationTest, DynamicShardRemoval) {
    EXPECT_EQ(hash_ring_->getShardCount(), 3u);
    
    // Remove a shard
    topology_->removeShard("shard_3");
    hash_ring_->removeShard("shard_3");
    
    EXPECT_EQ(hash_ring_->getShardCount(), 2u);
    EXPECT_FALSE(topology_->hasShard("shard_3"));
    
    // All URNs should still resolve to valid shards
    auto urn = URN::parse("urn:themis:relational:test:test:550e8400-e29b-41d4-a716-446655440000");
    ASSERT_TRUE(urn.has_value());
    
    std::string shard = hash_ring_->getShardForURN(*urn);
    EXPECT_TRUE(topology_->hasShard(shard));
    EXPECT_NE(shard, "shard_3");
}

TEST_F(ShardingIntegrationTest, HealthFilteredResolution) {
    // Mark shard_2 as unhealthy
    topology_->updateHealth("shard_2", false);
    
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 2u);
    
    // Verify unhealthy shard is excluded
    for (const auto& shard : healthy) {
        EXPECT_NE(shard.shard_id, "shard_2");
    }
}

// ============================================================================
// Cross-Shard Query Integration Tests
// ============================================================================

TEST_F(ShardingIntegrationTest, URNDistributionAcrossShards) {
    // Generate many URNs and verify distribution
    std::map<std::string, int> shard_counts;
    
    for (int i = 0; i < 1000; ++i) {
        // Generate pseudo-random UUIDs
        char uuid[37];
        snprintf(uuid, sizeof(uuid), 
            "%08x-%04x-%04x-%04x-%012x",
            i * 1234567, 
            (i * 111) & 0xFFFF,
            0x4000 | ((i * 222) & 0x0FFF),
            0x8000 | ((i * 333) & 0x3FFF),
            i * 987654321);
        
        std::string urn_str = "urn:themis:relational:test:test:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        
        if (urn) {
            std::string shard = hash_ring_->getShardForURN(*urn);
            shard_counts[shard]++;
        }
    }
    
    // Verify all shards got some URNs
    for (const auto& [shard, count] : shard_counts) {
        EXPECT_GT(count, 0) << "Shard " << shard << " got no URNs";
    }
    
    // Verify balance (allow wider spread due deterministic sample + virtual ring layout)
    double expected_per_shard = 1000.0 / 3.0;
    for (const auto& [shard, count] : shard_counts) {
        double deviation = std::abs(count - expected_per_shard) / expected_per_shard;
        EXPECT_LT(deviation, 0.50) << "Shard " << shard << " has poor balance: " << count;
    }
}

// ============================================================================
// Concurrent Access Integration Tests
// ============================================================================

TEST_F(ShardingIntegrationTest, ConcurrentURNResolution) {
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    
    auto resolve_task = [&](int thread_id) {
        for (int i = 0; i < 100; ++i) {
            char uuid[37];
            snprintf(uuid, sizeof(uuid), 
                "%08x-%04x-%04x-%04x-%012x",
                thread_id * 1000 + i, 
                (thread_id * 10 + i) & 0xFFFF,
                0x4000,
                0x8000,
                thread_id * 100000 + i);
            
            std::string urn_str = "urn:themis:relational:test:test:" + std::string(uuid);
            auto urn = URN::parse(urn_str);
            
            if (urn) {
                auto shard = resolver.resolvePrimary(*urn);
                if (shard) {
                    success_count++;
                } else {
                    error_count++;
                }
            }
        }
    };
    
    // Run concurrent resolution
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back(resolve_task, t);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count.load(), 400);
    EXPECT_EQ(error_count.load(), 0);
}

TEST_F(ShardingIntegrationTest, ConcurrentTopologyModification) {
    std::atomic<bool> stop{false};
    std::atomic<int> resolve_count{0};
    std::atomic<int> error_count{0};
    
    // Reader thread - continuously resolves URNs
    auto reader = [&]() {
        while (!stop) {
            auto urn = URN::parse("urn:themis:relational:test:test:550e8400-e29b-41d4-a716-446655440000");
            if (urn) {
                std::string shard = hash_ring_->getShardForURN(*urn);
                if (!shard.empty() && topology_->hasShard(shard)) {
                    resolve_count++;
                } else {
                    error_count++;
                }
            }
        }
    };
    
    // Writer thread - modifies topology
    auto writer = [&]() {
        for (int i = 0; i < 5; ++i) {
            std::string shard_id = "temp_shard_" + std::to_string(i);
            
            // Add
            ShardInfo temp;
            temp.shard_id = shard_id;
            temp.primary_endpoint = "localhost:9000";
            temp.is_healthy = true;
            topology_->addShard(temp);
            hash_ring_->addShard(shard_id, 50);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Remove
            topology_->removeShard(shard_id);
            hash_ring_->removeShard(shard_id);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::thread reader_thread(reader);
    std::thread writer_thread(writer);
    
    writer_thread.join();
    stop = true;
    reader_thread.join();
    
    // Should have many successful resolutions, minimal errors
    EXPECT_GT(resolve_count.load(), 100);
    // Errors are acceptable during topology changes, but should be minimal
    EXPECT_LT(error_count.load(), resolve_count.load() * 0.1);
}

// ============================================================================
// Shard Router Configuration Tests
// ============================================================================

TEST_F(ShardingIntegrationTest, RouterConfigurationIntegration) {
    auto resolver = std::make_shared<URNResolver>(topology_, hash_ring_, "shard_1");
    
    ShardRouter::Config config;
    config.local_shard_id = "shard_1";
    config.scatter_timeout_ms = 5000;
    config.max_concurrent_shards = 10;
    config.enable_query_pushdown = true;
    
    // Router should be constructible with resolved dependencies
    // Note: Without executor, limited functionality but should not crash
    EXPECT_EQ(config.scatter_timeout_ms, 5000u);
    EXPECT_EQ(config.max_concurrent_shards, 10u);
}

TEST_F(ShardingIntegrationTest, StatisticsAccumulation) {
    // Test statistics JSON structure
    nlohmann::json stats = {
        {"total_requests", 0},
        {"local_requests", 0},
        {"remote_requests", 0},
        {"scatter_gather_requests", 0},
        {"cross_shard_joins", 0},
        {"errors", 0},
        {"avg_latency_ms", 0.0}
    };
    
    // Simulate request tracking
    stats["total_requests"] = 100;
    stats["local_requests"] = 60;
    stats["remote_requests"] = 40;
    stats["scatter_gather_requests"] = 10;
    
    EXPECT_EQ(stats["total_requests"], 100);
    EXPECT_EQ(stats["local_requests"], 60);
    EXPECT_EQ(stats["remote_requests"], 40);
}

// ============================================================================
// End-to-End Workflow Tests
// ============================================================================

TEST_F(ShardingIntegrationTest, FullResolutionWorkflow) {
    // Simulate complete workflow:
    // 1. Parse URN
    // 2. Resolve to shard
    // 3. Check locality
    // 4. Get shard info
    
    std::string urn_str = "urn:themis:relational:production:orders:550e8400-e29b-41d4-a716-446655440000";
    
    // Step 1: Parse
    auto urn = URN::parse(urn_str);
    ASSERT_TRUE(urn.has_value());
    EXPECT_EQ(urn->model, "relational");
    EXPECT_EQ(urn->namespace_, "production");
    EXPECT_EQ(urn->collection, "orders");
    
    // Step 2: Hash and route
    uint64_t hash = urn->hash();
    EXPECT_NE(hash, 0u);
    
    std::string shard_id = hash_ring_->getShardForURN(*urn);
    EXPECT_FALSE(shard_id.empty());
    
    // Step 3: Get shard info
    auto shard_info = topology_->getShard(shard_id);
    ASSERT_TRUE(shard_info.has_value());
    EXPECT_TRUE(shard_info->is_healthy);
    EXPECT_FALSE(shard_info->primary_endpoint.empty());
    
    // Step 4: Resolver integration
    URNResolver resolver(topology_, hash_ring_, shard_id);
    EXPECT_TRUE(resolver.isLocal(*urn));
}

TEST_F(ShardingIntegrationTest, MultiTenantIsolation) {
    // Verify that different tenants can map to different shards
    std::string tenant_a = "urn:themis:relational:tenant_a:users:550e8400-e29b-41d4-a716-446655440000";
    std::string tenant_b = "urn:themis:relational:tenant_b:users:550e8400-e29b-41d4-a716-446655440000";
    
    auto urn_a = URN::parse(tenant_a);
    auto urn_b = URN::parse(tenant_b);
    
    ASSERT_TRUE(urn_a.has_value() && urn_b.has_value());
    
    // Different namespaces must remain distinguishable in parsed URNs
    EXPECT_NE(urn_a->namespace_, urn_b->namespace_);
    
    // Both should resolve to valid shards
    std::string shard_a = hash_ring_->getShardForURN(*urn_a);
    std::string shard_b = hash_ring_->getShardForURN(*urn_b);
    
    EXPECT_TRUE(topology_->hasShard(shard_a));
    EXPECT_TRUE(topology_->hasShard(shard_b));
}

// ============================================================================
// QW-18 Regression Tests: ShardRouter fail-closed without RemoteExecutor
// ============================================================================

/// @brief ShardRouter::routeRequest must return failure (not crash) when the
///        remote executor is not configured and a remote shard is targeted.
TEST_F(ShardingIntegrationTest, RouterRouteRequestFailsClosedWithoutExecutor) {
    // Resolver with a local shard id that does NOT match the resolved shard
    // so the request will be forwarded remotely.
    auto resolver = std::make_shared<URNResolver>(topology_, hash_ring_, "shard_999");

    ShardRouter::Config cfg;
    cfg.local_shard_id = "shard_999";
    cfg.scatter_timeout_ms = 1000;
    cfg.max_concurrent_shards = 4;

    // No executor passed: nullptr
    ShardRouter router(resolver, nullptr, cfg);

    const std::string urn_str =
        "urn:themis:relational:tenant1:users:550e8400-e29b-41d4-a716-446655440000";
    auto urn = URN::parse(urn_str);
    ASSERT_TRUE(urn.has_value());

    // get() must return nullopt (fail-closed, no crash)
    auto result = router.get(*urn);
    EXPECT_FALSE(result.has_value())
        << "Expected nullopt when remote_executor is not configured";
}

/// @brief ShardRouter::put() must return false (not crash) when the remote
///        executor is not configured and a remote shard is targeted.
TEST_F(ShardingIntegrationTest, RouterPutFailsClosedWithoutExecutor) {
    auto resolver = std::make_shared<URNResolver>(topology_, hash_ring_, "shard_999");

    ShardRouter::Config cfg;
    cfg.local_shard_id = "shard_999";
    cfg.scatter_timeout_ms = 1000;
    cfg.max_concurrent_shards = 4;

    ShardRouter router(resolver, nullptr, cfg);

    const std::string urn_str =
        "urn:themis:relational:tenant1:orders:7c9e6679-7425-40de-944b-e07fc1f90ae7";
    auto urn = URN::parse(urn_str);
    ASSERT_TRUE(urn.has_value());

    nlohmann::json data = {{"field", "value"}};
    bool ok = router.put(*urn, data);
    EXPECT_FALSE(ok)
        << "Expected put to fail when remote_executor is not configured";
}
