/**
 * @file test_redis_posix_bridge.cpp
 * @brief Unit tests for RedisCacheCoordinator RedisPublishBridgeFn (STUB #61).
 *
 * Verifies the RedisPublishBridgeFn injection in non-POSIX builds
 * (THEMIS_POSIX_SOCKETS not defined):
 *   RCC-POX-01  No fn set           → publish_errors_ incremented (no-op fallback).
 *   RCC-POX-02  Fn returns true     → messages_published_ incremented.
 *   RCC-POX-03  Fn throws exception → fail-closed; publish_errors_ incremented.
 *
 * Tests run in builds WITHOUT THEMIS_POSIX_SOCKETS.  On POSIX hosts (Linux/macOS)
 * the full socket path is active and the tests are skipped.
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
// With no fn registered the non-POSIX stub increments publish_errors_.
TEST_F(RedisPosixBridgeTest, NoFnIncrementsPublishErrors) {
#if defined(THEMIS_POSIX_SOCKETS)
    GTEST_SKIP() << "THEMIS_POSIX_SOCKETS is ON — full socket path active; skip.";
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
// Injected fn returning true increments messages_published_.
TEST_F(RedisPosixBridgeTest, SuccessFnIncrementsMessagesPublished) {
#if defined(THEMIS_POSIX_SOCKETS)
    GTEST_SKIP() << "THEMIS_POSIX_SOCKETS is ON — full socket path active; skip.";
#endif

    int call_count = 0;
    RedisCacheCoordinator::setRedisPublishBridgeFn(
        [&](const std::string& channel, const std::string& payload) -> bool {
            ++call_count;
            EXPECT_FALSE(channel.empty())  << "channel must not be empty";
            EXPECT_FALSE(payload.empty())  << "payload must not be empty";
            return true;
        });

    RedisCacheCoordinatorConfig cfg;
    RedisCacheCoordinator coord(cfg);

    coord.publishEntry("mykey", nlohmann::json{{"data", "hello"}}, 120, "t1");
    coord.publishInvalidation("mykey", "t1");

    EXPECT_EQ(call_count, 2) << "fn must be called once per publish";

    auto stats = coord.getStats();
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{2});
    EXPECT_EQ(stats.value("publish_errors",     uint64_t{0}), uint64_t{0});
}

// ── RCC-POX-03 ───────────────────────────────────────────────────────────────
// Throwing fn is fail-closed: no exception escapes, publish_errors_ incremented.
TEST_F(RedisPosixBridgeTest, ThrowingFnIsFailClosed) {
#if defined(THEMIS_POSIX_SOCKETS)
    GTEST_SKIP() << "THEMIS_POSIX_SOCKETS is ON — full socket path active; skip.";
#endif

    RedisCacheCoordinator::setRedisPublishBridgeFn(
        [](const std::string&, const std::string&) -> bool {
            throw std::runtime_error("simulated socket error");
        });

    RedisCacheCoordinatorConfig cfg;
    RedisCacheCoordinator coord(cfg);

    EXPECT_NO_THROW(coord.publishEntry("k1", {}, 10, "t"));
    EXPECT_NO_THROW(coord.publishInvalidation("k*", "t"));

    auto stats = coord.getStats();
    EXPECT_GE(stats.value("publish_errors", uint64_t{0}), uint64_t{2});
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{0});
}
