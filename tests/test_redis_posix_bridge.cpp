/**
 * @file test_redis_posix_bridge.cpp
 * @brief Unit tests for RedisCacheCoordinator RedisPublishBridgeFn (STUB #61).
 *
 * Verifies that the RedisPublishBridgeFn cannot claim delivery when the
 * coordinator transport is unavailable (missing hiredis or on Windows):
 *   RCC-POX-01  No fn set           → publish_errors_ incremented (no-op fallback).
 *   RCC-POX-02  Fn returns true     → transport remains unavailable.
 *   RCC-POX-03  Fn throws exception → fail-closed; publish_errors_ incremented.
 *
 * Tests run unless the hiredis profile is enabled on a POSIX platform.
 */

#include <gtest/gtest.h>
#include "cache/distributed_cache_coordinator.h"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using namespace themis::cache;

class RedisPosixBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        RedisCacheCoordinator::setRedisPublishBridgeFn({});  // restore clean state
    }
};

// ── RCC-POX-01 ───────────────────────────────────────────────────────────────
// With no fn registered the unavailable-transport path increments publish_errors_.
TEST_F(RedisPosixBridgeTest, NoFnIncrementsPublishErrors) {
#if !defined(_WIN32) && defined(THEMIS_ENABLE_REDIS)
    GTEST_SKIP() << "Redis transport is available; skip unavailable-transport test.";
#endif
    RedisCacheCoordinator::setRedisPublishBridgeFn({});  // ensure clean state

    RedisCacheCoordinatorConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 6379;
    RedisCacheCoordinator coord(cfg);

    coord.publishEntry("k1", nlohmann::json{{"v", 1}}, 60, "tenant");
    coord.publishInvalidation("k*", "tenant");

    auto stats = coord.getStats();
    EXPECT_GE(stats.value("publish_errors", uint64_t{0}), uint64_t{2})
        << "publish_errors must be at least 2";
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{0});
}

// ── RCC-POX-02 ───────────────────────────────────────────────────────────────
// An injected fn cannot claim cross-node delivery without a supported transport.
TEST_F(RedisPosixBridgeTest, BridgeCannotOverrideUnavailableTransport) {
#if !defined(_WIN32) && defined(THEMIS_ENABLE_REDIS)
    GTEST_SKIP() << "Redis transport is available; skip unavailable-transport test.";
#endif

    int call_count = 0;
    RedisCacheCoordinator::setRedisPublishBridgeFn(
        [&](const std::string&, const std::string&) -> bool {
            ++call_count;
            return true;
        });

    RedisCacheCoordinatorConfig cfg;
    RedisCacheCoordinator coord(cfg);

    coord.publishEntry("mykey", nlohmann::json{{"data", "hello"}}, 120, "t1");
    coord.publishInvalidation("mykey", "t1");

    EXPECT_EQ(call_count, 0) << "unavailable transport must not invoke a delivery bridge";

    auto stats = coord.getStats();
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{0});
    EXPECT_EQ(stats.value("publish_errors", uint64_t{0}), uint64_t{2});
    EXPECT_EQ(stats.value("invalidation_errors", uint64_t{0}), uint64_t{1});
    EXPECT_FALSE(stats.value("ready", true));
    EXPECT_FALSE(stats.value("dependency_available", true));
}

// ── RCC-POX-03 ───────────────────────────────────────────────────────────────
// Throwing fn is fail-closed: no exception escapes, publish_errors_ incremented.
TEST_F(RedisPosixBridgeTest, ThrowingFnIsFailClosed) {
#if !defined(_WIN32) && defined(THEMIS_ENABLE_REDIS)
    GTEST_SKIP() << "Redis transport is available; skip unavailable-transport test.";
#endif

    int call_count = 0;
    RedisCacheCoordinator::setRedisPublishBridgeFn(
        [&call_count](const std::string&, const std::string&) -> bool {
            ++call_count;
            throw std::runtime_error("simulated socket error");
        });

    RedisCacheCoordinatorConfig cfg;
    RedisCacheCoordinator coord(cfg);

    EXPECT_NO_THROW(coord.publishEntry("k1", {}, 10, "t"));
    EXPECT_NO_THROW(coord.publishInvalidation("k*", "t"));

    EXPECT_EQ(call_count, 0);
    auto stats = coord.getStats();
    EXPECT_GE(stats.value("publish_errors", uint64_t{0}), uint64_t{2});
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{0});
    EXPECT_FALSE(stats.value("ready", true));
    EXPECT_FALSE(stats.value("dependency_available", true));
}
