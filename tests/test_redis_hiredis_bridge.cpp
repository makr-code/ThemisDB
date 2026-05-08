/**
 * @file test_redis_hiredis_bridge.cpp
 * @brief Unit tests for RedisCacheCoordinator RedisPublishFn bridge (STUB #42).
 *
 * Verifies that the RedisPublishFn injection works correctly in
 * non-hiredis builds (THEMIS_ENABLE_REDIS not defined):
 *   RCC-HR-01  No fn set           → publish_errors_ incremented (no-op fallback).
 *   RCC-HR-02  Fn returns true     → messages_published_ incremented.
 *   RCC-HR-03  Fn throws exception → fail-closed; publish_errors_ incremented.
 *
 * Tests run in builds WITHOUT THEMIS_ENABLE_REDIS.  In hiredis builds the
 * injected fn is never reached and the tests are skipped.
 */

#include <gtest/gtest.h>
#include "cache/redis_cache_coordinator.h"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using namespace themis::cache;

class RedisHiredisBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        RedisCacheCoordinator::setRedisPublishFn({});  // restore clean state
    }
};

// ── RCC-HR-01 ────────────────────────────────────────────────────────────────
// With no fn registered, publishEntry() and publishInvalidation() fall through
// to the no-op increment: getStats()["publish_errors"] must be > 0.
TEST_F(RedisHiredisBridgeTest, NoFnIncrementsPublishErrors) {
#ifdef THEMIS_ENABLE_REDIS
    GTEST_SKIP() << "THEMIS_ENABLE_REDIS is ON — hiredis path active; skip.";
#endif
    RedisCacheCoordinator::setRedisPublishFn({});  // ensure clean state

    RedisCacheCoordinator::Config cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 6379;
    RedisCacheCoordinator coord(cfg);

    coord.publishEntry("k1", nlohmann::json{{"v", 1}}, 60, "tenant");
    coord.publishInvalidation("k*", "tenant");

    auto stats = coord.getStats();
    EXPECT_GE(stats.value("publish_errors", uint64_t{0}), uint64_t{2})
        << "publish_errors must be at least 2 (one per call)";
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{0})
        << "messages_published must be 0 when no fn is set";
}

// ── RCC-HR-02 ────────────────────────────────────────────────────────────────
// An injected fn returning true causes messages_published_ to be incremented
// rather than publish_errors_.
TEST_F(RedisHiredisBridgeTest, SuccessFnIncrementsMessagesPublished) {
#ifdef THEMIS_ENABLE_REDIS
    GTEST_SKIP() << "THEMIS_ENABLE_REDIS is ON — hiredis path active; skip.";
#endif

    int call_count = 0;
    RedisCacheCoordinator::setRedisPublishFn(
        [&](const std::string& /*channel*/, const std::string& /*payload*/) -> bool {
            ++call_count;
            return true;
        });

    RedisCacheCoordinator::Config cfg;
    RedisCacheCoordinator coord(cfg);

    coord.publishEntry("k1", nlohmann::json{{"v", 42}}, 30, "t1");
    coord.publishInvalidation("k*", "t1");

    EXPECT_EQ(call_count, 2) << "fn should be called once per publish";

    auto stats = coord.getStats();
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{2});
    EXPECT_EQ(stats.value("publish_errors",     uint64_t{0}), uint64_t{0});
}

// ── RCC-HR-03 ────────────────────────────────────────────────────────────────
// If the injected fn throws, the call must be fail-closed: no exception must
// escape and publish_errors_ must be incremented.
TEST_F(RedisHiredisBridgeTest, ThrowingFnIsFailClosed) {
#ifdef THEMIS_ENABLE_REDIS
    GTEST_SKIP() << "THEMIS_ENABLE_REDIS is ON — hiredis path active; skip.";
#endif

    RedisCacheCoordinator::setRedisPublishFn(
        [](const std::string&, const std::string&) -> bool {
            throw std::runtime_error("simulated transport error");
        });

    RedisCacheCoordinator::Config cfg;
    RedisCacheCoordinator coord(cfg);

    EXPECT_NO_THROW(coord.publishEntry("k1", {}, 10, "t"));
    EXPECT_NO_THROW(coord.publishInvalidation("k*", "t"));

    auto stats = coord.getStats();
    EXPECT_GE(stats.value("publish_errors", uint64_t{0}), uint64_t{2});
    EXPECT_EQ(stats.value("messages_published", uint64_t{0}), uint64_t{0});
}
