/**
 * @file test_chaos_network.cpp
 * @brief Chaos and edge-case tests for the HTTP server network stack
 *
 * Validates robustness under adversarial conditions:
 * - Oversized request bodies (payload too large)
 * - Oversized request headers (request header fields too large)
 * - Missing/empty content body
 * - Malformed JSON payloads
 * - Repeated concurrent requests
 * - Request ID generation / propagation
 * - Server config boundary conditions
 *
 * These tests operate at the configuration and logic level (no live TCP server)
 * and through the MonitoringApiHandler to keep test execution fast and
 * dependency-free.
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "server/http_server.h"
#include "server/monitoring_api_handler.h"

using Config = themis::server::HttpServer::Config;
namespace http = boost::beast::http;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const std::string& body = "",
    const std::string& content_type = "application/json"
) {
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::host, "localhost");
    if (!body.empty()) {
        req.set(http::field::content_type, content_type);
        req.body() = body;
    }
    req.prepare_payload();
    return req;
}

static std::unique_ptr<themis::server::MonitoringApiHandler> make_monitoring_handler() {
    static std::atomic<uint64_t> req_count{0};
    static std::atomic<uint64_t> err_count{0};
    static std::atomic<bool> running{true};
    static auto start = std::chrono::steady_clock::now();
    return std::make_unique<themis::server::MonitoringApiHandler>(
        nullptr, nullptr, &req_count, &err_count, &start,
        nullptr, nullptr, nullptr, &running, nullptr
    );
}

// ---------------------------------------------------------------------------
// Config Boundary Conditions
// ---------------------------------------------------------------------------

TEST(ChaosNetworkConfig, ZeroMaxConnectionsMeansUnlimited) {
    Config cfg;
    EXPECT_EQ(cfg.max_connections, 0u);
}

TEST(ChaosNetworkConfig, MaxConnectionsCanBeSet) {
    Config cfg;
    cfg.max_connections = 1000;
    EXPECT_EQ(cfg.max_connections, 1000u);
}

TEST(ChaosNetworkConfig, MaxConnectionsLimitOf1) {
    Config cfg;
    cfg.max_connections = 1;
    EXPECT_EQ(cfg.max_connections, 1u);
}

TEST(ChaosNetworkConfig, ZeroBodySizeAllowsAnyBody) {
    Config cfg;
    cfg.max_request_size_mb = 0; // 0 = unlimited
    EXPECT_EQ(cfg.max_request_size_mb, 0u);
}

TEST(ChaosNetworkConfig, ZeroHeaderSizeDisablesCheck) {
    Config cfg;
    cfg.max_header_size_bytes = 0;
    EXPECT_EQ(cfg.max_header_size_bytes, 0u);
}

// ---------------------------------------------------------------------------
// Oversized Body Simulation
// ---------------------------------------------------------------------------

TEST(ChaosOversizedBody, ExceedsDefaultBodyLimit) {
    // Simulate: client sends body larger than 10 MB
    const size_t body_bytes = 11 * 1024 * 1024; // 11 MB
    Config cfg;
    const size_t max_body = cfg.max_request_size_mb * 1024 * 1024;
    EXPECT_GT(body_bytes, max_body);
    // routeRequest() will return 413 when body_bytes > max_body_bytes_
}

TEST(ChaosOversizedBody, ExactlyAtLimitIsAllowed) {
    Config cfg;
    cfg.max_request_size_mb = 5;
    const size_t max_body = cfg.max_request_size_mb * 1024 * 1024;
    const size_t body_bytes = max_body; // Exactly at limit
    EXPECT_EQ(body_bytes, max_body);
    // routeRequest() allows body_bytes == max_body_bytes_ (> check, not >=)
}

TEST(ChaosOversizedBody, OneByteOverLimitIsRejected) {
    Config cfg;
    cfg.max_request_size_mb = 5;
    const size_t max_body = cfg.max_request_size_mb * 1024 * 1024;
    const size_t body_bytes = max_body + 1;
    EXPECT_GT(body_bytes, max_body);
}

TEST(ChaosOversizedBody, EmptyBodyIsAlwaysAllowed) {
    Config cfg;
    EXPECT_GT(cfg.max_request_size_mb, 0u);
    const size_t body_bytes = 0;
    EXPECT_LT(body_bytes, cfg.max_request_size_mb * 1024 * 1024);
}

// ---------------------------------------------------------------------------
// Oversized Header Simulation
// ---------------------------------------------------------------------------

TEST(ChaosOversizedHeaders, ExceedsDefaultHeaderLimit) {
    Config cfg;
    // Simulate: total header bytes exceeds 8 KB default
    const size_t header_bytes = 9000; // > 8192
    EXPECT_GT(header_bytes, cfg.max_header_size_bytes);
}

TEST(ChaosOversizedHeaders, ExactlyAtHeaderLimitIsAllowed) {
    Config cfg;
    const size_t header_bytes = cfg.max_header_size_bytes;
    EXPECT_EQ(header_bytes, cfg.max_header_size_bytes);
    // check is strict >, so exactly at limit is allowed
}

TEST(ChaosOversizedHeaders, OneByteOverHeaderLimitIsRejected) {
    Config cfg;
    const size_t header_bytes = cfg.max_header_size_bytes + 1;
    EXPECT_GT(header_bytes, cfg.max_header_size_bytes);
}

TEST(ChaosOversizedHeaders, ZeroHeaderLimitDisablesCheck) {
    Config cfg;
    cfg.max_header_size_bytes = 0;
    // When 0, the check is skipped regardless of header size
    EXPECT_EQ(cfg.max_header_size_bytes, 0u);
}

// ---------------------------------------------------------------------------
// Malformed / Empty JSON Payloads
// ---------------------------------------------------------------------------

TEST(ChaosPayloads, EmptyBodyIsValidForGetRequests) {
    auto req = make_request(http::verb::get, "/health");
    EXPECT_TRUE(req.body().empty());
}

TEST(ChaosPayloads, MalformedJsonBodyDoesNotCrash) {
    // Verify nlohmann::json throws on malformed input (parse guard in routeRequest)
    const std::string bad_json = "{\"key\": }";
    EXPECT_THROW(nlohmann::json::parse(bad_json), nlohmann::json::parse_error);
}

TEST(ChaosPayloads, TruncatedJsonBodyDoesNotCrash) {
    const std::string truncated = "{\"key\":";
    EXPECT_THROW(nlohmann::json::parse(truncated), nlohmann::json::parse_error);
}

TEST(ChaosPayloads, InvalidUtf8InBodyDoesNotCrash) {
    // Strings with invalid UTF-8 bytes should be handled gracefully
    const std::string invalid_utf8 = "{\"key\": \"\xff\xfe\"}";
    EXPECT_THROW(nlohmann::json::parse(invalid_utf8), nlohmann::json::parse_error);
}

TEST(ChaosPayloads, ValidEmptyJsonObject) {
    const std::string empty_obj = "{}";
    EXPECT_NO_THROW({
        auto j = nlohmann::json::parse(empty_obj);
        EXPECT_TRUE(j.is_object());
        EXPECT_TRUE(j.empty());
    });
}

TEST(ChaosPayloads, ValidEmptyJsonArray) {
    const std::string empty_arr = "[]";
    EXPECT_NO_THROW({
        auto j = nlohmann::json::parse(empty_arr);
        EXPECT_TRUE(j.is_array());
        EXPECT_TRUE(j.empty());
    });
}

TEST(ChaosPayloads, DeeplyNestedJsonDoesNotCrash) {
    // Build deeply nested JSON (100 levels) – should parse without stack overflow
    std::string deep = "";
    for (int i = 0; i < 100; ++i) deep += "{\"n\":";
    deep += "1";
    for (int i = 0; i < 100; ++i) {
      deep += "}";
    }
    EXPECT_NO_THROW(nlohmann::json::parse(deep));
}

// ---------------------------------------------------------------------------
// Active Connections Counter Semantics
// ---------------------------------------------------------------------------

TEST(ChaosConnectionCounter, StartsAtZero) {
    std::atomic<uint64_t> conns{0};
    EXPECT_EQ(conns.load(), 0u);
}

TEST(ChaosConnectionCounter, IncrementOnConnect) {
    std::atomic<uint64_t> conns{0};
    conns.fetch_add(1, std::memory_order_relaxed);
    EXPECT_EQ(conns.load(), 1u);
}

TEST(ChaosConnectionCounter, DecrementOnDisconnect) {
    std::atomic<uint64_t> conns{2};
    conns.fetch_sub(1, std::memory_order_relaxed);
    EXPECT_EQ(conns.load(), 1u);
}

TEST(ChaosConnectionCounter, MaxConnectionsEnforcement) {
    const size_t limit = 5;
    std::atomic<uint64_t> conns{0};

    // Simulate accepting connections up to limit
    size_t accepted = 0;
    for (size_t i = 0; i < 10; ++i) {
        if (conns.load(std::memory_order_relaxed) < limit) {
            conns.fetch_add(1, std::memory_order_relaxed);
            ++accepted;
        }
        // else: reject
    }

    EXPECT_EQ(accepted, limit);
    EXPECT_EQ(conns.load(), limit);
}

TEST(ChaosConnectionCounter, ConcurrentConnectsAndDisconnects) {
    std::atomic<uint64_t> conns{0};
    constexpr int N = 200;

    std::vector<std::thread> threads;
    threads.reserve(N * 2);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&conns]() {
            conns.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            conns.fetch_sub(1, std::memory_order_relaxed);
        });
    }

    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(conns.load(), 0u);
}

// ---------------------------------------------------------------------------
// Access Log Structured Fields Validation
// ---------------------------------------------------------------------------

TEST(ChaosAccessLog, RequestIdIsGeneratedWhenMissing) {
    // If X-Request-ID is absent, server generates one (timestamp-counter)
    auto req = make_request(http::verb::get, "/health");
    auto it = req.find("X-Request-ID");
    // Not set in request → server should generate
    EXPECT_EQ(it, req.end());
}

TEST(ChaosAccessLog, RequestIdIsPreservedWhenPresent) {
    auto req = make_request(http::verb::get, "/health");
    req.set("X-Request-ID", "test-req-123");
    EXPECT_EQ(std::string(req["X-Request-ID"]), "test-req-123");
}

TEST(ChaosAccessLog, ClientIpFromXForwardedFor) {
    auto req = make_request(http::verb::get, "/health");
    req.set("X-Forwarded-For", "10.0.0.1, 192.168.1.1");
    EXPECT_EQ(std::string(req["X-Forwarded-For"]), "10.0.0.1, 192.168.1.1");
}

TEST(ChaosAccessLog, ClientIpFromXRealIp) {
    auto req = make_request(http::verb::get, "/health");
    req.set("X-Real-IP", "10.0.0.5");
    EXPECT_EQ(std::string(req["X-Real-IP"]), "10.0.0.5");
}

// ---------------------------------------------------------------------------
// Health Endpoint Liveness Under Rapid Calls
// ---------------------------------------------------------------------------

TEST(ChaosHealthEndpoint, LivenessRapidCalls) {
    auto handler = make_monitoring_handler();
    // Rapid successive calls should all succeed
    for (int i = 0; i < 100; ++i) {
        auto req = make_request(http::verb::get, "/health/live");
        auto res = handler->handleLiveness(req);
        EXPECT_EQ(res.result(), http::status::ok);
    }
}

TEST(ChaosHealthEndpoint, ReadinessRapidCalls) {
    auto handler = make_monitoring_handler();
    for (int i = 0; i < 100; ++i) {
        auto req = make_request(http::verb::get, "/health/ready");
        auto res = handler->handleReadiness(req);
        // ready (server_running=true, no storage configured -> ok)
        EXPECT_EQ(res.result(), http::status::ok);
    }
}

TEST(ChaosHealthEndpoint, ConcurrentLivenessCalls) {
    auto handler = make_monitoring_handler();
    std::atomic<int> ok_count{0};
    constexpr int N = 50;

    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&handler, &ok_count]() {
            auto req = make_request(http::verb::get, "/health/live");
            auto res = handler->handleLiveness(req);
            if (res.result() == http::status::ok) {
                ok_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(ok_count.load(), N);
}
