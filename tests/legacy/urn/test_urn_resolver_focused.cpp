#include <gtest/gtest.h>
#include "sharding/urn_resolver.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <memory>

namespace themis { namespace sharding { 

class URNResolverTest : public ::testing::Test {
protected:
    std::shared_ptr<URNResolver> resolver_;

    void SetUp() override {
        // Create a shard topology
        auto topology = std::make_shared<ShardTopology>();
        
        // Create a hash ring with virtual nodes for distribution
        auto hash_ring = std::make_shared<ConsistentHashRing>(160);
        hash_ring->addShard("shard_001");
        
        // Create the resolver
        resolver_ = std::make_shared<URNResolver>(topology, hash_ring, "shard_001");
    }
};

TEST_F(URNResolverTest, GetShardForKeyFailsClosedForEmptyKey) {
    // Verify that empty key is rejected fail-closed (returns empty string, logs error)
    std::string result = resolver_->getShardForKey("test_collection", "");
    
    // Should return empty string for empty key (fail-closed behavior)
    EXPECT_TRUE(result.empty());
}

TEST_F(URNResolverTest, GetShardForKeyReturnsValidShardForNonEmptyKey) {
    // Verify that non-empty key returns a valid shard ID
    std::string result = resolver_->getShardForKey("test_collection", "key_001");
    
    // Should return a non-empty shard ID for valid key
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result, "shard_001");  // Only one shard in ring
}
} } // namespace themis::sharding
