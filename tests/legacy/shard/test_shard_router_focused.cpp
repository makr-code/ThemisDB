#include <gtest/gtest.h>
#include "sharding/shard_router.h"
#include "sharding/urn_resolver.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <memory>

namespace themis { namespace sharding { 

class ShardRouterTest : public ::testing::Test {
protected:
    std::shared_ptr<ShardRouter> router_;

    void SetUp() override {
        // Create topology and hash ring
        auto topology = std::make_shared<ShardTopology>();
        auto hash_ring = std::make_shared<ConsistentHashRing>(160);
        hash_ring->addShard("shard_001");
        
        // Create resolver
        auto resolver = std::make_shared<URNResolver>(topology, hash_ring, "shard_001");
        
        // Create router config
        ShardRouter::Config config;
        config.local_shard_id = "shard_001";
        config.enable_result_caching = false;
        
        // Create router
        router_ = std::make_shared<ShardRouter>(resolver, nullptr, config);
    }
};

TEST_F(ShardRouterTest, RouteRequestFailsClosedForEmptyMethod) {
    // Create test URN
    URN test_urn("collection_001", "key_001");
    
    // Call with empty method
    ShardResult result = router_->routeRequest(test_urn, "", "/api/v1/data/key_001", std::nullopt);
    
    // Verify fail-closed: result.success should be false
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_msg, "method is empty");
}

TEST_F(ShardRouterTest, RouteRequestFailsClosedForEmptyPath) {
    // Create test URN
    URN test_urn("collection_001", "key_001");
    
    // Call with empty path
    ShardResult result = router_->routeRequest(test_urn, "GET", "", std::nullopt);
    
    // Verify fail-closed: result.success should be false
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_msg, "path is empty");
}

TEST_F(ShardRouterTest, ExecuteLocalFailsClosedForEmptyMethod) {
    // Call with empty method
    ShardResult result = router_->executeLocal("", "/api/v1/data/key_001", std::nullopt);
    
    // Verify fail-closed: result.success should be false
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_msg, "method is empty");
}

TEST_F(ShardRouterTest, ExecuteLocalFailsClosedForEmptyPath) {
    // Call with empty path
    ShardResult result = router_->executeLocal("GET", "", std::nullopt);
    
    // Verify fail-closed: result.success should be false
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_msg, "path is empty");
}

TEST_F(ShardRouterTest, RouteRequestAcceptsValidMethodAndPath) {
    // Create test URN
    URN test_urn("collection_001", "key_001");
    
    // Call with valid method and path (will fail on remote execution, but not on guard)
    ShardResult result = router_->routeRequest(test_urn, "GET", "/api/v1/data/key_001", std::nullopt);
    
    // Should not fail on empty checks (may fail on remote execution, but not on guard)
    // This test verifies the guard passes, not the full execution
    EXPECT_NE(result.error_msg, "method is empty");
    EXPECT_NE(result.error_msg, "path is empty");
}
} } // namespace themis::sharding
