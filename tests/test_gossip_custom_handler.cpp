// Copyright 2026 ThemisDB — Licensed under MIT License
// DK-2 / S-5: GossipProtocol::registerCustomHandler unit tests
//
// Tests:
//   GP-CUSTOM-01  registerCustomHandler() dispatches correctly for a matching message_type
//   GP-CUSTOM-02  handleMessage() does NOT invoke a handler for an unregistered message_type
//   GP-CUSTOM-03  Duplicate registration overwrites the previous handler (last-wins)

#include <gtest/gtest.h>
#include "sharding/gossip_protocol.h"
#include "sharding/shard_topology.h"
#include <atomic>
#include <chrono>
#include <memory>

using namespace themis::sharding;

// ---------------------------------------------------------------------------
// Helper – build a GossipMessage with a current timestamp so that the
// replay-protection check in handleMessage() passes.
// ---------------------------------------------------------------------------
static GossipMessage makeMsg(const std::string& type, const std::string& sender = "peer-test") {
    GossipMessage m;
    m.message_id   = "test-msg-id";
    m.sender_id    = sender;
    m.message_type = type;
    m.payload      = nlohmann::json::object();
    m.timestamp    = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return m;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
class GossipCustomHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ShardTopology::Config topo_config;
        topo_config.cluster_name       = "test_cluster";
        topo_config.enable_health_checks = false;
        topology = std::make_shared<ShardTopology>(topo_config);

        GossipConfig config;
        config.enabled               = false;  // don't start background threads
        config.validate_certificates = false;  // skip signature check in tests
        config.max_message_age_sec   = 300;    // generous age window for tests
        config.local_peer_id         = "local-test-peer";
        config.require_mtls          = false;

        protocol = std::make_unique<GossipProtocol>(config, topology);
    }

    std::shared_ptr<ShardTopology>  topology;
    std::unique_ptr<GossipProtocol> protocol;
};

// ---------------------------------------------------------------------------
// GP-CUSTOM-01: handler is invoked for the registered message_type
// ---------------------------------------------------------------------------
TEST_F(GossipCustomHandlerTest, GP_CUSTOM_01_DispatchesForMatchingType) {
    std::atomic<int> call_count{0};
    std::string received_type;

    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage& msg) {
            ++call_count;
            received_type = msg.message_type;
        });

    auto msg = makeMsg("adapter_capability");
    protocol->handleMessage(msg);

    EXPECT_EQ(call_count.load(), 1);
    EXPECT_EQ(received_type, "adapter_capability");
}

// ---------------------------------------------------------------------------
// GP-CUSTOM-02: handler is NOT invoked for an unregistered message_type
// ---------------------------------------------------------------------------
TEST_F(GossipCustomHandlerTest, GP_CUSTOM_02_NoDispatchForUnknownType) {
    std::atomic<int> call_count{0};

    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage&) { ++call_count; });

    auto msg = makeMsg("some_other_type");
    protocol->handleMessage(msg);

    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// GP-CUSTOM-03: duplicate registration overwrites the previous handler
// ---------------------------------------------------------------------------
TEST_F(GossipCustomHandlerTest, GP_CUSTOM_03_DuplicateOverwritesPrevious) {
    std::atomic<int> first_calls{0};
    std::atomic<int> second_calls{0};

    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage&) { ++first_calls; });

    // Second registration for the same type — last handler wins
    protocol->registerCustomHandler("adapter_capability",
        [&](const GossipMessage&) { ++second_calls; });

    auto msg = makeMsg("adapter_capability");
    protocol->handleMessage(msg);

    EXPECT_EQ(first_calls.load(),  0) << "First (overwritten) handler must NOT be called";
    EXPECT_EQ(second_calls.load(), 1) << "Second (active) handler must be called exactly once";
}
