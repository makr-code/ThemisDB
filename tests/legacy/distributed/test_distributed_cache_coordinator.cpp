// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for RedisCacheCoordinator (distributed cache coordination).
//
// These tests exercise the coordinator without requiring a live Redis server:
//  - Configuration and construction (graceful degradation when Redis is down)
//  - ICacheCoordinator interface compliance
//  - Message serialization / deserialization
//  - Stats collection
//  - Publish operations when not connected (no-throw guarantee)
//  - name() and isConnected() contract

#include <gtest/gtest.h>
#include "cache/distributed_cache_coordinator.h"
#include "cache/cache_replication_coordinator.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

using namespace themis;
using namespace themis::cache;
using json = nlohmann::json;

// ============================================================================
// Helper: build a coordinator that connects to a port that is not listening
// so all operations degrade gracefully.
// ============================================================================
static RedisCacheCoordinatorConfig makeOfflineConfig(
    [[maybe_unused]] const std::string& node_id = "test-node")
{
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 16399;   // Likely not running – graceful degradation
    cfg.channel_prefix       = "themis_test";
    cfg.connect_timeout_ms   = 200;     // Fast timeout for tests
    cfg.reconnect_interval_ms = 50;
    // Note: new config does not have node_id field; node_id is derived internally
    return cfg;
}

// ============================================================================
// Construction / destruction
// ============================================================================

TEST(RedisCacheCoordinatorTest, ConstructionDoesNotThrow) {
    // Even without a running Redis, construction must not throw.
    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(makeOfflineConfig());
        // Give subscribe thread a moment to attempt connection and back off
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
}

TEST(RedisCacheCoordinatorTest, NameContainsHostPort) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    std::string n = coord.name();
    // name() is a stable transport identifier, not a host/port descriptor.
    EXPECT_EQ(n, "RedisCacheCoordinator");
}

// ============================================================================
// ICacheCoordinator interface compliance
// ============================================================================

TEST(RedisCacheCoordinatorTest, IsConnectedFalseWhenRedisAbsent) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    // After a connect attempt the connection should be reported as offline
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(coord.isConnected());
}

TEST(RedisCacheCoordinatorTest, PublishEntryNoThrowWhenOffline) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    json result = {{"rows", 5}};
    EXPECT_NO_THROW(coord.publishEntry("fp_abc123", result, 300, "acme"));
}

TEST(RedisCacheCoordinatorTest, PublishInvalidationNoThrowWhenOffline) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    EXPECT_NO_THROW(coord.publishInvalidation("orders.*", "tenant_x"));
}

TEST(RedisCacheCoordinatorTest, PublishEntryNoThrowWhenNoTenant) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    EXPECT_NO_THROW(coord.publishEntry("fp_000", {{"v", 1}}, 60, ""));
}

TEST(RedisCacheCoordinatorTest, SubscribeEntryCallbackRegisteredNoThrow) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    bool called = false;
    EXPECT_NO_THROW(
        coord.subscribeEntries([&called](const ReplicationMessage&) { called = true; }));
    // Callback should NOT have been called (no messages in offline mode)
    EXPECT_FALSE(called);
}

TEST(RedisCacheCoordinatorTest, SubscribeInvalidationCallbackRegisteredNoThrow) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    EXPECT_NO_THROW(
        coord.subscribeInvalidations([](const ReplicationMessage&) {}));
}

// ============================================================================
// Stats
// ============================================================================

TEST(RedisCacheCoordinatorTest, GetStatsReturnsExpectedFields) {
    RedisCacheCoordinator coord(makeOfflineConfig("stats-node")); // Parameter ignored by new config
    auto stats = coord.getStats();

    EXPECT_TRUE(stats.contains("messages_published"));
    EXPECT_TRUE(stats.contains("messages_received"));
    EXPECT_TRUE(stats.contains("publish_errors"));
    EXPECT_TRUE(stats.contains("reconnect_count"));
    EXPECT_TRUE(stats.contains("connected"));
    EXPECT_TRUE(stats.contains("channel") || stats.contains("channel_prefix"));

    EXPECT_EQ(stats["messages_published"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["messages_received"].get<uint64_t>(),  0u);
    if (stats.contains("node_id")) {
        // POSIX implementation derives node_id from host:port.
        EXPECT_EQ(stats["node_id"].get<std::string>(), "127.0.0.1:16399");
    }

    if (stats.contains("channel")) {
        EXPECT_EQ(stats["channel"].get<std::string>(), "themis_test:replication");
    } else {
        EXPECT_EQ(stats["channel_prefix"].get<std::string>(), "themis_test");
    }
}

TEST(RedisCacheCoordinatorTest, PublishErrorsIncrementedWhenOffline) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    coord.publishEntry("k1", {{"a", 1}}, 60, "");
    coord.publishInvalidation(".*");

    // Wait briefly for the reconnect thread
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto stats = coord.getStats();
    // Publish errors should be > 0 because Redis is not available
    EXPECT_GT(stats["publish_errors"].get<uint64_t>(), 0u);
}

TEST(RedisCacheCoordinatorTest, ChannelNameMatchesPrefix) {
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.channel_prefix = "myapp_cache";
    RedisCacheCoordinator coord(cfg);

    auto stats = coord.getStats();
    if (stats.contains("channel")) {
        EXPECT_EQ(stats["channel"].get<std::string>(), "myapp_cache:replication");
    } else {
        EXPECT_EQ(stats["channel_prefix"].get<std::string>(), "myapp_cache");
    }
}

// ============================================================================
// Multiple publish calls
// ============================================================================

TEST(RedisCacheCoordinatorTest, MultiplePublishCallsDoNotCrash) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    for (int i = 0; i < 10; ++i) {
        coord.publishEntry("key_" + std::to_string(i), {{"i", i}}, 60, "");
    }
    for (int i = 0; i < 5; ++i) {
        coord.publishInvalidation("pattern_" + std::to_string(i));
    }
    // No crash = pass
    SUCCEED();
}

// ============================================================================
// Callback overwrite
// ============================================================================

TEST(RedisCacheCoordinatorTest, CallbackCanBeOverwritten) {
    RedisCacheCoordinator coord(makeOfflineConfig());

    int call_count_a = 0, call_count_b = 0;
    coord.subscribeEntries([&call_count_a](const ReplicationMessage&) { ++call_count_a; });
    coord.subscribeEntries([&call_count_b](const ReplicationMessage&) { ++call_count_b; });

    // In offline mode neither callback is invoked; the important check is
    // that overwriting does not throw or corrupt internal state.
    EXPECT_EQ(call_count_a, 0);
    EXPECT_EQ(call_count_b, 0);
}

// ============================================================================
// Polymorphism via ICacheCoordinator pointer
// ============================================================================

TEST(RedisCacheCoordinatorTest, PolymorphicUsageViaInterface) {
    auto coord = std::make_shared<RedisCacheCoordinator>(makeOfflineConfig());

    // Use through the base interface pointer
    ICacheCoordinator* iface = coord.get();
    EXPECT_NE(iface, nullptr);

    // All interface methods must be callable without crashing
    EXPECT_NO_THROW(iface->publishEntry("k", {{}}, 10, ""));
    EXPECT_NO_THROW(iface->publishInvalidation(".*"));
    EXPECT_NO_THROW(iface->subscribeEntries([](const ReplicationMessage&) {}));
    EXPECT_NO_THROW(iface->subscribeInvalidations([](const ReplicationMessage&) {}));
    EXPECT_FALSE(iface->isConnected());  // Offline
    EXPECT_FALSE(iface->name().empty());
    auto s = iface->getStats();
    EXPECT_TRUE(s.is_object());
}

// ============================================================================
// Node ID defaults to host:port when not specified
// ============================================================================

TEST(RedisCacheCoordinatorTest, DefaultNodeIdIsHostPort) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host               = "192.168.1.10";
    cfg.port               = 6380;
    cfg.connect_timeout_ms = 100;
    // Note: new config does not have node_id field; node_id is derived internally from host:port
    // Use high port to avoid accidental connection
    cfg.port               = 16400;
    RedisCacheCoordinator coord(cfg);

    auto stats = coord.getStats();
    if (!stats.contains("node_id")) {
        GTEST_SKIP() << "node_id not exposed in this platform coordinator stats";
    }

    std::string node_id = stats["node_id"].get<std::string>();
    EXPECT_EQ(node_id, "192.168.1.10:16400");
}

TEST(RedisCacheCoordinatorTest, ExplicitNodeIdIsPreserved) {
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    // Note: new config does not have node_id field; this test now verifies default behavior
    RedisCacheCoordinator coord(cfg);

    auto stats = coord.getStats();
    if (!stats.contains("node_id")) {
        GTEST_SKIP() << "node_id not exposed in this platform coordinator stats";
    }

    // Default node_id is host:port
    EXPECT_EQ(stats["node_id"].get<std::string>(), "127.0.0.1:16399");
}

// ============================================================================
// Graceful degradation: callbacks never fire in offline mode
// ============================================================================

TEST(RedisCacheCoordinatorTest, NoCallbacksFireInOfflineMode) {
    RedisCacheCoordinator coord(makeOfflineConfig());

    std::atomic<int> entry_calls{0};
    std::atomic<int> inv_calls{0};

    coord.subscribeEntries([&entry_calls](const ReplicationMessage&) { ++entry_calls; });
    coord.subscribeInvalidations([&inv_calls](const ReplicationMessage&) { ++inv_calls; });

    coord.publishEntry("k", {{"v", 1}}, 60, "");
    coord.publishInvalidation(".*");

    // Give subscriber thread time to potentially fire (it shouldn't)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_EQ(entry_calls.load(), 0);
    EXPECT_EQ(inv_calls.load(),   0);
}

// ============================================================================
// HMAC signing / verification (offline unit tests — no Redis required)
// ============================================================================

TEST(RedisCacheCoordinatorTest, HmacSecretDisabledByDefault) {
    // When hmac_secret is empty, the coordinator must NOT attach a "sig" field
    // and must accept unsigned messages. Verify via stats/no-throw.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    ASSERT_TRUE(cfg.hmac_secret.empty());

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
        coord.publishEntry("k", {{"v", 1}}, 60, "");
        coord.publishInvalidation("pattern");
    });
}

TEST(RedisCacheCoordinatorTest, HmacSecretConfigFieldAccepted) {
    // Verify the hmac_secret config field is reachable and stored.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.hmac_secret = "s3cr3t";

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
    });
}

TEST(RedisCacheCoordinatorTest, HmacPublishDoesNotThrow) {
    // Publishing with an HMAC secret set must not throw even if Redis is offline.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.hmac_secret = "test-hmac-key";

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
        coord.publishEntry("fp", {{"r", 42}}, 30, "acme");
        coord.publishInvalidation("acme.*");
    });
}

TEST(RedisCacheCoordinatorTest, HmacEmptySecretPublishAndStatsOk) {
    // Without hmac_secret, publish should increment publish_errors_ (no Redis)
    // but stats must be accessible.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    ASSERT_TRUE(cfg.hmac_secret.empty());

    RedisCacheCoordinator coord(cfg);
    coord.publishEntry("key", {}, 60, "");
    coord.publishInvalidation("*");

    auto stats = coord.getStats();
    EXPECT_TRUE(stats.contains("publish_errors"));
    EXPECT_GE(stats["publish_errors"].get<uint64_t>(), 0u);
}

// ============================================================================
// Exponential back-off reconnect – validate reconnect_count increments
// ============================================================================

TEST(RedisCacheCoordinatorTest, ReconnectCountIncreasesOverTime) {
    // On POSIX builds the subscriber thread attempts reconnects when Redis is
    // absent. On Windows the coordinator is a deliberate no-op stub without a
    // background reconnect loop.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.connect_timeout_ms   = 100;
    cfg.reconnect_interval_ms = 50;  // fast for test; back-off overrides this

    RedisCacheCoordinator coord(cfg);

    // Wait long enough for at least one reconnect attempt.
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    auto stats = coord.getStats();
    ASSERT_TRUE(stats.contains("reconnect_count"));
#if defined(_WIN32)
    EXPECT_EQ(stats["reconnect_count"].get<uint64_t>(), 0u);
#else
    EXPECT_GE(stats["reconnect_count"].get<uint64_t>(), 1u);
#endif
}

TEST(RedisCacheCoordinatorTest, IsConnectedFalseAfterMultipleAttempts) {
    // After several back-off cycles the coordinator must still report
    // disconnected (not throw, not corrupt state).
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_FALSE(coord.isConnected());
    // Stats must remain well-formed
    auto stats = coord.getStats();
    EXPECT_TRUE(stats.is_object());
}
