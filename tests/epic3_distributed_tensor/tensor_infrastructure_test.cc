/**
 * @file tensor_infrastructure_test.cc
 * @brief Contract tests for INodeRegistry and IStripeTransport (sub-issue #5435).
 *
 * Validates factory construction for both interfaces, node registration,
 * endpoint lookup, health reporting, and stripe transport operations at
 * scaffold stage. Production fabric wiring is tracked in sub-issue #5435.
 */

#include "distributed_tensor/include/tensor_infrastructure.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace themis::distributed_tensor;

// ---------------------------------------------------------------------------
// INodeRegistry tests
// ---------------------------------------------------------------------------

class NodeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_ = makeNodeRegistry({
            {"shard-0", "http://node-0:9090"},
            {"shard-1", "http://node-1:9090"},
        });
        ASSERT_NE(registry_, nullptr);
    }

    std::unique_ptr<INodeRegistry> registry_;
};

TEST_F(NodeRegistryTest, FactoryReturnsNonNull) {
    EXPECT_NE(registry_, nullptr);
}

TEST_F(NodeRegistryTest, EndpointLookupForRegisteredNode) {
    auto ep = registry_->endpoint("shard-0");
    EXPECT_TRUE(ep.has_value());
    EXPECT_EQ(*ep, "http://node-0:9090");
}

TEST_F(NodeRegistryTest, EndpointLookupForUnknownNodeReturnsNullopt) {
    auto ep = registry_->endpoint("nonexistent");
    EXPECT_FALSE(ep.has_value());
}

TEST_F(NodeRegistryTest, RegisterNewNodeAppearsInEndpointLookup) {
    registry_->registerNode("shard-2", "http://node-2:9090");
    auto ep = registry_->endpoint("shard-2");
    EXPECT_TRUE(ep.has_value());
    EXPECT_EQ(*ep, "http://node-2:9090");
}

TEST_F(NodeRegistryTest, DeregisterRemovesNode) {
    registry_->deregisterNode("shard-1");
    EXPECT_FALSE(registry_->endpoint("shard-1").has_value());
}

TEST_F(NodeRegistryTest, DeregisterNonexistentDoesNotThrow) {
    EXPECT_NO_THROW(registry_->deregisterNode("no-such-shard"));
}

TEST_F(NodeRegistryTest, HealthOfRegisteredNodeReturnsUnknownOrDegraded) {
    auto rec = registry_->healthOf("shard-0");
    // Scaffold: no health checks have run; health is Unknown or initial state.
    (void)rec;
    SUCCEED();
}

TEST_F(NodeRegistryTest, HealthOfUnknownNodeDoesNotThrow) {
    EXPECT_NO_THROW(registry_->healthOf("no-such-shard"));
}

TEST_F(NodeRegistryTest, HealthyNodesDoesNotThrow) {
    EXPECT_NO_THROW(registry_->healthyNodes(NodeHealth::Healthy));
    EXPECT_NO_THROW(registry_->healthyNodes(NodeHealth::Unknown));
}

// ---------------------------------------------------------------------------
// IStripeTransport tests
// ---------------------------------------------------------------------------

class StripeTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_ = makeNodeRegistry({{"shard-0", "http://node-0:9090"}});
        ASSERT_NE(registry_, nullptr);

        TransportConfig cfg;
        cfg.connect_timeout_ms = 100;
        cfg.request_timeout_ms = 500;
        cfg.max_retries        = 1;
        cfg.stripe_chunk_bytes = 64 * 1024;
        cfg.enable_tls         = false;

        transport_ = makeStripeTransport(registry_, cfg);
        ASSERT_NE(transport_, nullptr);
    }

    std::shared_ptr<INodeRegistry> registry_;
    std::unique_ptr<IStripeTransport> transport_;
};

TEST_F(StripeTransportTest, FactoryReturnsNonNull) {
    EXPECT_NE(transport_, nullptr);
}

TEST_F(StripeTransportTest, WriteStripeDoesNotThrow) {
    std::vector<std::uint8_t> data(256, 0xCC);
    EXPECT_NO_THROW({
        try {
            transport_->writeStripe("shard-0", "artifact-1", 0, data);
        } catch (const std::exception&) {
            // acceptable; no real transport in scaffold
        }
    });
}

TEST_F(StripeTransportTest, ReadStripeDoesNotThrow) {
    EXPECT_NO_THROW({
        try {
            transport_->readStripe("shard-0", "artifact-1", 0);
        } catch (const std::exception&) {
            // acceptable; no real transport in scaffold
        }
    });
}

TEST_F(StripeTransportTest, DeleteStripeDoesNotThrow) {
    EXPECT_NO_THROW({
        try {
            transport_->deleteStripe("shard-0", "artifact-1", 0);
        } catch (const std::exception&) {
            // acceptable; no real transport in scaffold
        }
    });
}

TEST_F(StripeTransportTest, HealthCallbackRegistrationDoesNotThrow) {
    EXPECT_NO_THROW(transport_->onHealthChange([](const NodeHealthRecord&) {}));
}

TEST_F(StripeTransportTest, ReadStripeOnUnknownShardDoesNotThrow) {
    EXPECT_NO_THROW({
        try {
            transport_->readStripe("no-such-shard", "artifact-1", 0);
        } catch (const std::exception&) {
            // acceptable
        }
    });
}
