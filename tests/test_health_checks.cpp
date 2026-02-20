/**
 * @file test_health_checks.cpp
 * @brief Tests for health, liveness and readiness check endpoints
 *
 * Validates the MonitoringApiHandler liveness and readiness probes introduced
 * as part of the Server Production Readiness initiative.
 */

#include <gtest/gtest.h>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <memory>

#include "server/monitoring_api_handler.h"

namespace http = boost::beast::http;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> make_get(const std::string& target) {
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "localhost");
    req.prepare_payload();
    return req;
}

// Minimal MonitoringApiHandler factory that requires no storage
static std::unique_ptr<themis::server::MonitoringApiHandler> make_handler(
    const std::atomic<bool>* is_running = nullptr,
    const std::atomic<uint64_t>* active_requests = nullptr
) {
    static std::atomic<uint64_t> req_count{0};
    static std::atomic<uint64_t> err_count{0};
    static auto start = std::chrono::steady_clock::now();

    return std::make_unique<themis::server::MonitoringApiHandler>(
        nullptr,  // no storage
        nullptr,  // no auth
        &req_count,
        &err_count,
        &start,
        nullptr,  // no secondary index
        nullptr,  // no schema manager
        nullptr,  // no sharding metrics
        is_running,
        active_requests
    );
}

// ---------------------------------------------------------------------------
// Liveness Tests
// ---------------------------------------------------------------------------

class LivenessTest : public ::testing::Test {};

TEST_F(LivenessTest, ReturnsAliveWhenRunningIsTrue) {
    std::atomic<bool> running{true};
    auto handler = make_handler(&running);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "alive");
    EXPECT_TRUE(body["checks"]["server_running"]);
}

TEST_F(LivenessTest, Returns503WhenRunningIsFalse) {
    std::atomic<bool> running{false};
    auto handler = make_handler(&running);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::service_unavailable);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "dead");
    EXPECT_FALSE(body["checks"]["server_running"]);
}

TEST_F(LivenessTest, ReturnsAliveWhenIsRunningIsNull) {
    // When no is_running pointer supplied, assume alive (e.g. lightweight usage)
    auto handler = make_handler(nullptr);

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "alive");
}

TEST_F(LivenessTest, ResponseBodyIsJson) {
    auto handler = make_handler();

    auto res = handler->handleLiveness(make_get("/health/live"));

    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NO_THROW(json::parse(res.body()));
}

TEST_F(LivenessTest, ResponseContainsChecksObject) {
    auto handler = make_handler();

    auto res = handler->handleLiveness(make_get("/health/live"));

    auto body = json::parse(res.body());
    ASSERT_TRUE(body.contains("checks"));
    ASSERT_TRUE(body["checks"].is_object());
}

TEST_F(LivenessTest, TransitionFromAliveToDeadReflectedImmediately) {
    std::atomic<bool> running{true};
    auto handler = make_handler(&running);

    // Alive
    auto res1 = handler->handleLiveness(make_get("/health/live"));
    EXPECT_EQ(res1.result(), http::status::ok);

    // Now stop
    running.store(false);

    auto res2 = handler->handleLiveness(make_get("/health/live"));
    EXPECT_EQ(res2.result(), http::status::service_unavailable);
}

// ---------------------------------------------------------------------------
// Readiness Tests
// ---------------------------------------------------------------------------

class ReadinessTest : public ::testing::Test {};

TEST_F(ReadinessTest, ReturnsReadyWhenRunningAndNoStorage) {
    // No storage configured: treated as ready (lightweight deployment)
    std::atomic<bool> running{true};
    auto handler = make_handler(&running);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "ready");
}

TEST_F(ReadinessTest, Returns503WhenNotRunning) {
    std::atomic<bool> running{false};
    auto handler = make_handler(&running);

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res.result(), http::status::service_unavailable);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "not_ready");
}

TEST_F(ReadinessTest, ResponseBodyIsJson) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NO_THROW(json::parse(res.body()));
}

TEST_F(ReadinessTest, ResponseContainsChecksObject) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    ASSERT_TRUE(body.contains("checks"));
    ASSERT_TRUE(body["checks"].is_object());
}

TEST_F(ReadinessTest, ChecksContainServerRunningField) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    EXPECT_TRUE(body["checks"].contains("server_running"));
}

TEST_F(ReadinessTest, ChecksContainStorageAvailableField) {
    auto handler = make_handler();

    auto res = handler->handleReadiness(make_get("/health/ready"));

    auto body = json::parse(res.body());
    EXPECT_TRUE(body["checks"].contains("storage_available"));
}

// ---------------------------------------------------------------------------
// Health Check (existing /health endpoint) Tests
// ---------------------------------------------------------------------------

class HealthCheckTest : public ::testing::Test {};

TEST_F(HealthCheckTest, ReturnsHealthyStatus) {
    auto handler = make_handler();

    auto res = handler->handleHealthCheck(make_get("/health"));

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"], "healthy");
}

TEST_F(HealthCheckTest, ResponseContainsUptimeSeconds) {
    auto handler = make_handler();

    auto res = handler->handleHealthCheck(make_get("/health"));

    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("uptime_seconds"));
    EXPECT_GE(body["uptime_seconds"].get<int64_t>(), 0);
}

TEST_F(HealthCheckTest, ResponseBodyIsJson) {
    auto handler = make_handler();

    auto res = handler->handleHealthCheck(make_get("/health"));

    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_NO_THROW(json::parse(res.body()));
}
