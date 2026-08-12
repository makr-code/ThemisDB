#include <gtest/gtest.h>
#include "sharding/gossip_protocol.h"

#include <memory>

using namespace themis::sharding;

TEST(GossipConfigTest, DefaultsAreReasonable) {
    GossipConfig cfg;
    EXPECT_GT(cfg.gossip_interval_sec, 0u);
    EXPECT_GT(cfg.max_peers, 0u);
    EXPECT_GT(cfg.fanout, 0u);
    EXPECT_GT(cfg.peer_timeout_sec, 0u);
}

TEST(GossipConfigTest, DisabledByDefault) {
    GossipConfig cfg;
    EXPECT_FALSE(cfg.enabled);
}
