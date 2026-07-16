/*
 * ThemisDB | File: test_sharding_gossip.cpp | Version: 0.0.14
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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
