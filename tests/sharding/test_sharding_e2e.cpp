/**
 * ThemisDB Sharding E2E Tests
 * 
 * End-to-End Tests für das Sharding-Subsystem.
 * Diese Tests simulieren vollständige Workflows ohne echte Netzwerkverbindungen.
 * 
 * Test-Szenarien:
 * - Vollständiger CRUD-Workflow über Shards
 * - Scatter-Gather Queries
 * - Cross-Shard Joins
 * - Data Migration Simulation
 * - Health Check Workflows
 */

#include <gtest/gtest.h>
#include "sharding/urn.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "sharding/shard_router.h"
#include <memory>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

using namespace themis::sharding;

// ============================================================================
// E2E Test Fixture with Simulated Cluster
// ============================================================================

class ShardingE2ETest : public ::testing::Test {
protected:
    void SetUp() override {
        setupCluster(5);  // 5-node cluster for E2E tests
    }
    
    void TearDown() override {
        topology_.reset();
        hash_ring_.reset();
        simulated_data_.clear();
    }
    
    void setupCluster(size_t shard_count) {
        ShardTopology::Config config;
        config.cluster_name = "e2e-test-cluster";
        config.enable_health_checks = false;
        topology_ = std::make_shared<ShardTopology>(config);
        hash_ring_ = std::make_shared<ConsistentHashRing>();
        
        std::vector<std::string> datacenters = {"dc1", "dc2"};
        
        for (size_t i = 1; i <= shard_count; ++i) {
            std::string shard_id = "shard_" + std::to_string(i);
            
            ShardInfo shard;
            shard.shard_id = shard_id;
            shard.primary_endpoint = "localhost:" + std::to_string(8080 + i);
            shard.datacenter = datacenters[i % datacenters.size()];
            shard.is_healthy = true;
            shard.capabilities = {"read", "write", "replicate"};
            
            topology_->addShard(shard);
            hash_ring_->addShard(shard_id, 150);
            
            // Initialize simulated storage
            simulated_data_[shard_id] = {};
        }
    }
    
    // Simulate writing data to a shard
    bool simulateWrite(const URN& urn, const nlohmann::json& data) {
        std::string shard_id = hash_ring_->getShardForURN(urn);
        if (shard_id.empty()) {
          return false;
        }
        
        std::lock_guard<std::mutex> lock(data_mutex_);
        simulated_data_[shard_id][urn.toString()] = data;
        return true;
    }
    
    // Simulate reading data from a shard
    std::optional<nlohmann::json> simulateRead(const URN& urn) {
        std::string shard_id = hash_ring_->getShardForURN(urn);
        if (shard_id.empty()) {
          return std::nullopt;
        }
        
        std::lock_guard<std::mutex> lock(data_mutex_);
        auto shard_it = simulated_data_.find(shard_id);
        if (shard_it == simulated_data_.end()) {
          return std::nullopt;
        }
        
        auto doc_it = shard_it->second.find(urn.toString());
        if (doc_it == shard_it->second.end()) {
          return std::nullopt;
        }
        
        return doc_it->second;
    }
    
    // Simulate scatter-gather query
    std::vector<nlohmann::json> simulateScatterGather(const std::string& collection) {
        std::vector<nlohmann::json> results;
        
        std::lock_guard<std::mutex> lock(data_mutex_);
        for (const auto& [shard_id, documents] : simulated_data_) {
            for (const auto& [urn_str, data] : documents) {
                auto urn = URN::parse(urn_str);
                if (urn && urn->collection == collection) {
                    results.push_back(data);
                }
            }
        }
        
        return results;
    }
    
    // Simulate data migration between shards
    bool simulateMigration(const std::string& from_shard, const std::string& to_shard,
                          const std::vector<std::string>& urn_strings) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        
        for (const auto& urn_str : urn_strings) {
            auto from_it = simulated_data_[from_shard].find(urn_str);
            if (from_it != simulated_data_[from_shard].end()) {
                simulated_data_[to_shard][urn_str] = from_it->second;
                simulated_data_[from_shard].erase(from_it);
            }
        }
        
        return true;
    }
    
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::map<std::string, std::map<std::string, nlohmann::json>> simulated_data_;
    std::mutex data_mutex_;
};

// ============================================================================
// E2E CRUD Workflow Tests
// ============================================================================

TEST_F(ShardingE2ETest, CompleteWriteReadCycle) {
    // Create URN
    std::string urn_str = "urn:themis:relational:production:orders:550e8400-e29b-41d4-a716-446655440000";
    auto urn = URN::parse(urn_str);
    ASSERT_TRUE(urn.has_value());
    
    // Write data
    nlohmann::json order_data = {
        {"order_id", "ORD-001"},
        {"customer_id", "CUST-123"},
        {"amount", 199.99},
        {"status", "pending"}
    };
    
    EXPECT_TRUE(simulateWrite(*urn, order_data));
    
    // Read back
    auto retrieved = simulateRead(*urn);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ((*retrieved)["order_id"], "ORD-001");
    EXPECT_EQ((*retrieved)["amount"], 199.99);
}

TEST_F(ShardingE2ETest, MultiDocumentWorkflow) {
    std::vector<std::string> uuids = {
        "550e8400-e29b-41d4-a716-446655440001",
        "550e8400-e29b-41d4-a716-446655440002",
        "550e8400-e29b-41d4-a716-446655440003",
        "550e8400-e29b-41d4-a716-446655440004",
        "550e8400-e29b-41d4-a716-446655440005"
    };
    
    // Write multiple documents
    for (size_t i = 0; i < uuids.size(); ++i) {
        std::string urn_str = "urn:themis:relational:production:users:" + uuids[i];
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        nlohmann::json user_data = {
            {"user_id", i + 1},
            {"name", "User " + std::to_string(i + 1)},
            {"email", "user" + std::to_string(i + 1) + "@example.com"}
        };
        
        EXPECT_TRUE(simulateWrite(*urn, user_data));
    }
    
    // Read all back
    for (size_t i = 0; i < uuids.size(); ++i) {
        std::string urn_str = "urn:themis:relational:production:users:" + uuids[i];
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        auto data = simulateRead(*urn);
        ASSERT_TRUE(data.has_value()) << "Failed to read user " << (i + 1);
        EXPECT_EQ((*data)["user_id"], static_cast<int>(i + 1));
    }
}

TEST_F(ShardingE2ETest, CrossModelWorkflow) {
    // Write relational data
    auto rel_urn = URN::parse("urn:themis:relational:app:customers:550e8400-e29b-41d4-a716-446655440001");
    ASSERT_TRUE(rel_urn.has_value());
    EXPECT_TRUE(simulateWrite(*rel_urn, {{"type", "relational"}, {"name", "Customer 1"}}));
    
    // Write graph data
    auto graph_urn = URN::parse("urn:themis:graph:app:nodes:550e8400-e29b-41d4-a716-446655440002");
    ASSERT_TRUE(graph_urn.has_value());
    EXPECT_TRUE(simulateWrite(*graph_urn, {{"type", "graph"}, {"label", "Person"}}));
    
    // Write vector data
    auto vector_urn = URN::parse("urn:themis:vector:app:embeddings:550e8400-e29b-41d4-a716-446655440003");
    ASSERT_TRUE(vector_urn.has_value());
    EXPECT_TRUE(simulateWrite(*vector_urn, {{"type", "vector"}, {"dimensions", 512}}));
    
    // Verify all models accessible
    auto rel_data = simulateRead(*rel_urn);
    auto graph_data = simulateRead(*graph_urn);
    auto vector_data = simulateRead(*vector_urn);
    
    ASSERT_TRUE(rel_data.has_value());
    ASSERT_TRUE(graph_data.has_value());
    ASSERT_TRUE(vector_data.has_value());
    
    EXPECT_EQ((*rel_data)["type"], "relational");
    EXPECT_EQ((*graph_data)["type"], "graph");
    EXPECT_EQ((*vector_data)["type"], "vector");
}

// ============================================================================
// E2E Scatter-Gather Tests
// ============================================================================

TEST_F(ShardingE2ETest, ScatterGatherQuery) {
    // Write documents to different shards
    for (int i = 1; i <= 20; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        
        std::string urn_str = "urn:themis:relational:app:products:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        nlohmann::json product = {
            {"product_id", i},
            {"name", "Product " + std::to_string(i)},
            {"price", 10.0 * i}
        };
        
        EXPECT_TRUE(simulateWrite(*urn, product));
    }
    
    // Scatter-gather to find all products
    auto results = simulateScatterGather("products");
    
    EXPECT_EQ(results.size(), 20u);
    
    // Verify all products found
    std::set<int> found_ids;
    for (const auto& product : results) {
        found_ids.insert(product["product_id"].get<int>());
    }
    
    for (int i = 1; i <= 20; ++i) {
        EXPECT_TRUE(found_ids.count(i) > 0) << "Product " << i << " not found";
    }
}

TEST_F(ShardingE2ETest, ScatterGatherWithFiltering) {
    // Write mixed data
    for (int i = 1; i <= 10; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        
        // Write to orders collection
        std::string order_urn = "urn:themis:relational:app:orders:" + std::string(uuid);
        auto urn1 = URN::parse(order_urn);
        ASSERT_TRUE(urn1.has_value());
        EXPECT_TRUE(simulateWrite(*urn1, {{"type", "order"}, {"id", i}}));
        
        // Write to customers collection
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i + 100);
        std::string cust_urn = "urn:themis:relational:app:customers:" + std::string(uuid);
        auto urn2 = URN::parse(cust_urn);
        ASSERT_TRUE(urn2.has_value());
        EXPECT_TRUE(simulateWrite(*urn2, {{"type", "customer"}, {"id", i}}));
    }
    
    // Query only orders
    auto orders = simulateScatterGather("orders");
    EXPECT_EQ(orders.size(), 10u);
    for (const auto& order : orders) {
        EXPECT_EQ(order["type"], "order");
    }
    
    // Query only customers
    auto customers = simulateScatterGather("customers");
    EXPECT_EQ(customers.size(), 10u);
    for (const auto& customer : customers) {
        EXPECT_EQ(customer["type"], "customer");
    }
}

// ============================================================================
// E2E Data Migration Tests
// ============================================================================

TEST_F(ShardingE2ETest, DataMigrationWorkflow) {
    // Write data to shard_1
    std::vector<std::string> urns_to_migrate;
    for (int i = 1; i <= 5; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        std::string urn_str = "urn:themis:relational:app:migrate_test:" + std::string(uuid);
        urns_to_migrate.push_back(urn_str);
        
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        // Force write to shard_1 (bypass hash routing for test)
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            simulated_data_["shard_1"][urn_str] = {{"migrated", false}, {"id", i}};
        }
    }
    
    // Verify data is on shard_1
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        EXPECT_EQ(simulated_data_["shard_1"].size(), 5u);
    }
    
    // Migrate to shard_2
    EXPECT_TRUE(simulateMigration("shard_1", "shard_2", urns_to_migrate));
    
    // Verify migration
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        EXPECT_EQ(simulated_data_["shard_1"].size(), 0u);
        EXPECT_EQ(simulated_data_["shard_2"].size(), 5u);
    }
}

TEST_F(ShardingE2ETest, RebalancingSimulation) {
    // Simulate adding a new shard and rebalancing
    
    // Initial data distribution
    std::map<std::string, int> initial_counts;
    for (int i = 1; i <= 100; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%012x",
            i, 0x1234, 0x4567, 0x89ab, i * 12345);
        
        std::string urn_str = "urn:themis:relational:app:rebalance:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        std::string shard = hash_ring_->getShardForURN(*urn);
        initial_counts[shard]++;
        
        EXPECT_TRUE(simulateWrite(*urn, {{"id", i}}));
    }
    
    // Add new shard
    ShardInfo new_shard;
    new_shard.shard_id = "shard_6";
    new_shard.primary_endpoint = "localhost:8086";
    new_shard.is_healthy = true;
    
    topology_->addShard(new_shard);
    hash_ring_->addShard("shard_6", 150);
    simulated_data_["shard_6"] = {};
    
    // After adding shard, some URNs should route to new shard
    int would_move = 0;
    for (int i = 1; i <= 100; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%012x",
            i, 0x1234, 0x4567, 0x89ab, i * 12345);
        
        std::string urn_str = "urn:themis:relational:app:rebalance:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        std::string new_target = hash_ring_->getShardForURN(*urn);
        if (new_target == "shard_6") {
            would_move++;
        }
    }
    
    // Approximately 1/6 of data should move to new shard
    EXPECT_GT(would_move, 5);
    EXPECT_LT(would_move, 35);
}

// ============================================================================
// E2E Health and Failover Tests
// ============================================================================

TEST_F(ShardingE2ETest, ShardFailoverScenario) {
    URNResolver resolver(topology_, hash_ring_, "shard_1");
    
    // Find a URN that routes to shard_2
    std::optional<URN> target_urn;
    for (int i = 1; i <= 1000; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        std::string urn_str = "urn:themis:relational:app:test:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        if (urn && hash_ring_->getShardForURN(*urn) == "shard_2") {
            target_urn = urn;
            break;
        }
    }
    
    ASSERT_TRUE(target_urn.has_value());
    
    // Write data
    EXPECT_TRUE(simulateWrite(*target_urn, {{"test", "data"}}));
    
    // Mark shard_2 as unhealthy
    topology_->updateHealth("shard_2", false);
    
    // Verify shard is marked unhealthy
    auto shard_info = topology_->getShard("shard_2");
    ASSERT_TRUE(shard_info.has_value());
    EXPECT_FALSE(shard_info->is_healthy);
    
    // Get healthy shards
    auto healthy = topology_->getHealthyShards();
    EXPECT_EQ(healthy.size(), 4u);
    
    for (const auto& s : healthy) {
        EXPECT_NE(s.shard_id, "shard_2");
    }
}

TEST_F(ShardingE2ETest, RollingUpgradeScenario) {
    // Simulate rolling upgrade: take each shard offline temporarily
    
    // Write initial data
    for (int i = 1; i <= 10; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        std::string urn_str = "urn:themis:relational:app:upgrade_test:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        EXPECT_TRUE(simulateWrite(*urn, {{"version", 1}, {"id", i}}));
    }
    
    // Simulate rolling upgrade
    for (int shard_num = 1; shard_num <= 5; ++shard_num) {
        std::string shard_id = "shard_" + std::to_string(shard_num);
        
        // Take offline
        topology_->updateHealth(shard_id, false);
        
        // Verify cluster still has 4 healthy nodes
        auto healthy = topology_->getHealthyShards();
        EXPECT_EQ(healthy.size(), 4u);
        
        // Simulate upgrade time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Bring back online
        topology_->updateHealth(shard_id, true);
        
        // Verify back to 5 healthy
        healthy = topology_->getHealthyShards();
        EXPECT_EQ(healthy.size(), 5u);
    }
    
    // All data should still be accessible
    for (int i = 1; i <= 10; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "550e8400-e29b-41d4-a716-%012d", i);
        std::string urn_str = "urn:themis:relational:app:upgrade_test:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        auto data = simulateRead(*urn);
        ASSERT_TRUE(data.has_value()) << "Data lost for id " << i;
        EXPECT_EQ((*data)["id"], i);
    }
}

// ============================================================================
// E2E Performance Tests
// ============================================================================

TEST_F(ShardingE2ETest, HighVolumeWriteRead) {
    const int NUM_DOCUMENTS = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Write
    for (int i = 1; i <= NUM_DOCUMENTS; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%012x",
            i, (i >> 16) & 0xFFFF, 0x4000 | (i & 0x0FFF), 0x8000 | ((i >> 12) & 0x3FFF), i);
        
        std::string urn_str = "urn:themis:relational:perf:test:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        EXPECT_TRUE(simulateWrite(*urn, {{"id", i}, {"data", std::string(100, 'x')}}));
    }
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // Read all back
    for (int i = 1; i <= NUM_DOCUMENTS; ++i) {
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%012x",
            i, (i >> 16) & 0xFFFF, 0x4000 | (i & 0x0FFF), 0x8000 | ((i >> 12) & 0x3FFF), i);
        
        std::string urn_str = "urn:themis:relational:perf:test:" + std::string(uuid);
        auto urn = URN::parse(urn_str);
        ASSERT_TRUE(urn.has_value());
        
        auto data = simulateRead(*urn);
        ASSERT_TRUE(data.has_value());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto write_time = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto read_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
    
    std::cout << "Write " << NUM_DOCUMENTS << " docs: " << write_time.count() << "ms" << std::endl;
    std::cout << "Read " << NUM_DOCUMENTS << " docs: " << read_time.count() << "ms" << std::endl;
    
    // Should be fast (< 1s for 1000 docs in memory)
    EXPECT_LT(write_time.count(), 1000);
    EXPECT_LT(read_time.count(), 1000);
}

TEST_F(ShardingE2ETest, ConcurrentWorkload) {
    std::atomic<int> write_success{0};
    std::atomic<int> write_fail{0};
    std::atomic<int> read_success{0};
    std::atomic<int> read_fail{0};
    
    auto worker = [&](int thread_id) {
        for (int i = 0; i < 100; ++i) {
            char uuid[37];
            snprintf(uuid, sizeof(uuid), "%08x-%04x-%04x-%04x-%012x",
                thread_id * 1000 + i, 0x1234, 0x4567, 0x89ab, thread_id * 100 + i);
            
            std::string urn_str = "urn:themis:relational:concurrent:test:" + std::string(uuid);
            auto urn = URN::parse(urn_str);
            
            if (urn) {
                // Write
                if (simulateWrite(*urn, {{"thread", thread_id}, {"iter", i}})) {
                    write_success++;
                } else {
                    write_fail++;
                }
                
                // Read
                auto data = simulateRead(*urn);
                if (data.has_value()) {
                    read_success++;
                } else {
                    read_fail++;
                }
            }
        }
    };
    
    // Run 8 concurrent workers
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back(worker, t);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All operations should succeed
    EXPECT_EQ(write_success.load(), 800);
    EXPECT_EQ(write_fail.load(), 0);
    EXPECT_EQ(read_success.load(), 800);
    EXPECT_EQ(read_fail.load(), 0);
}
