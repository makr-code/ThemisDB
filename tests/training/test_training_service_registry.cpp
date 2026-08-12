/**
 * @file test_training_service_registry.cpp
 * @brief Unit tests for TrainingServiceRegistry
 * 
 * Tests dependency injection functionality including:
 * - Registry singleton pattern
 * - Thread-safe registration and retrieval
 * - Null handling
 * - Infrastructure availability checks
 * 
 * @note Requires GTest: vcpkg install gtest OR apt-get install libgtest-dev
 * @build cmake -DTHEMIS_BUILD_TESTS=ON ..
 * @run ./tests/test_training_service_registry
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "llm/lora_framework/training_service_registry.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include <memory>
#include <thread>
#include <vector>

using namespace themis::llm::lora;
using namespace themis::sharding;

// ============================================================================
// Test Fixtures
// ============================================================================

class TrainingServiceRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear registry before each test
        TrainingServiceRegistry::getInstance().clear();
    }
    
    void TearDown() override {
        // Clean up after each test
        TrainingServiceRegistry::getInstance().clear();
    }
};

// ============================================================================
// Basic Registry Tests
// ============================================================================

TEST_F(TrainingServiceRegistryTest, Singleton_ReturnsSameInstance) {
    auto& registry1 = TrainingServiceRegistry::getInstance();
    auto& registry2 = TrainingServiceRegistry::getInstance();
    
    // Should be the exact same instance
    EXPECT_EQ(&registry1, &registry2);
}

TEST_F(TrainingServiceRegistryTest, InitialState_NoInfrastructure) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    EXPECT_FALSE(registry.hasShardInfrastructure());
    EXPECT_EQ(registry.getShardRouter(), nullptr);
    EXPECT_EQ(registry.getShardTopology(), nullptr);
}

TEST_F(TrainingServiceRegistryTest, RegisterShardRouter_Success) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    // Create a mock ShardRouter (using default constructor for test)
    auto router = std::make_shared<ShardRouter>(
        nullptr,  // resolver
        nullptr,  // executor
        ShardRouter::Config{}
    );
    
    registry.registerShardRouter(router);
    
    auto retrieved = registry.getShardRouter();
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, router);
}

TEST_F(TrainingServiceRegistryTest, RegisterShardTopology_Success) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    // Create a mock ShardTopology
    auto topology = std::make_shared<ShardTopology>();
    
    registry.registerShardTopology(topology);
    
    auto retrieved = registry.getShardTopology();
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, topology);
}

TEST_F(TrainingServiceRegistryTest, RegisterBoth_InfrastructureAvailable) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    auto router = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    auto topology = std::make_shared<ShardTopology>();
    
    registry.registerShardRouter(router);
    registry.registerShardTopology(topology);
    
    EXPECT_TRUE(registry.hasShardInfrastructure());
}

TEST_F(TrainingServiceRegistryTest, RegisterOnlyRouter_InfrastructureNotAvailable) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    auto router = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    
    registry.registerShardRouter(router);
    
    // Only router, no topology
    EXPECT_FALSE(registry.hasShardInfrastructure());
}

TEST_F(TrainingServiceRegistryTest, RegisterOnlyTopology_InfrastructureNotAvailable) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    auto topology = std::make_shared<ShardTopology>();
    
    registry.registerShardTopology(topology);
    
    // Only topology, no router
    EXPECT_FALSE(registry.hasShardInfrastructure());
}

TEST_F(TrainingServiceRegistryTest, Clear_RemovesAllRegistrations) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    auto router = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    auto topology = std::make_shared<ShardTopology>();
    
    registry.registerShardRouter(router);
    registry.registerShardTopology(topology);
    
    EXPECT_TRUE(registry.hasShardInfrastructure());
    
    registry.clear();
    
    EXPECT_FALSE(registry.hasShardInfrastructure());
    EXPECT_EQ(registry.getShardRouter(), nullptr);
    EXPECT_EQ(registry.getShardTopology(), nullptr);
}

TEST_F(TrainingServiceRegistryTest, RegisterNull_HandledCorrectly) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    // Register null values
    registry.registerShardRouter(nullptr);
    registry.registerShardTopology(nullptr);
    
    EXPECT_FALSE(registry.hasShardInfrastructure());
    EXPECT_EQ(registry.getShardRouter(), nullptr);
    EXPECT_EQ(registry.getShardTopology(), nullptr);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(TrainingServiceRegistryTest, ConcurrentRegistration_ThreadSafe) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    std::vector<std::thread> threads;
    const int num_threads = 10;
    
    // Concurrent registration
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&registry, i]() {
            auto router = std::make_shared<ShardRouter>(
                nullptr, nullptr, ShardRouter::Config{}
            );
            auto topology = std::make_shared<ShardTopology>();
            
            registry.registerShardRouter(router);
            registry.registerShardTopology(topology);
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have valid infrastructure (last one wins)
    EXPECT_TRUE(registry.hasShardInfrastructure());
}

TEST_F(TrainingServiceRegistryTest, ConcurrentAccess_ThreadSafe) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    // Register once
    auto router = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    auto topology = std::make_shared<ShardTopology>();
    
    registry.registerShardRouter(router);
    registry.registerShardTopology(topology);
    
    std::vector<std::thread> threads;
    const int num_threads = 10;
    std::atomic<int> success_count{0};
    
    // Concurrent reads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&registry, &success_count]() {
            for (int j = 0; j < 100; ++j) {
                auto r = registry.getShardRouter();
                auto t = registry.getShardTopology();
                bool has_infra = registry.hasShardInfrastructure();
                
                if (r && t && has_infra) {
                    success_count++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have many successful reads
    EXPECT_GT(success_count.load(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(TrainingServiceRegistryTest, RegistryLifecycle_CompleteFlow) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    // 1. Initial state - empty
    EXPECT_FALSE(registry.hasShardInfrastructure());
    
    // 2. Register infrastructure
    auto router = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    auto topology = std::make_shared<ShardTopology>();
    
    registry.registerShardRouter(router);
    registry.registerShardTopology(topology);
    
    // 3. Verify available
    EXPECT_TRUE(registry.hasShardInfrastructure());
    
    // 4. Retrieve and verify same instances
    auto retrieved_router = registry.getShardRouter();
    auto retrieved_topology = registry.getShardTopology();
    
    EXPECT_EQ(retrieved_router, router);
    EXPECT_EQ(retrieved_topology, topology);
    
    // 5. Clear
    registry.clear();
    
    // 6. Verify cleared
    EXPECT_FALSE(registry.hasShardInfrastructure());
}

TEST_F(TrainingServiceRegistryTest, ReplaceRegistration_NewInstanceReplacesPrevious) {
    auto& registry = TrainingServiceRegistry::getInstance();
    
    // Register first instance
    auto router1 = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    registry.registerShardRouter(router1);
    
    auto retrieved1 = registry.getShardRouter();
    EXPECT_EQ(retrieved1, router1);
    
    // Register second instance (replace)
    auto router2 = std::make_shared<ShardRouter>(
        nullptr, nullptr, ShardRouter::Config{}
    );
    registry.registerShardRouter(router2);
    
    auto retrieved2 = registry.getShardRouter();
    EXPECT_EQ(retrieved2, router2);
    EXPECT_NE(retrieved2, router1);
}

// ============================================================================
// Main
// ============================================================================


