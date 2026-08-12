/**
 * @file test_api_gateway_enhancements.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Focused unit tests for API Gateway Enhancements (v1.7.0 / v1.8.0):
//   - RequestCoalescingManager  (request_coalescing.h/.cpp)
//   - SmartRouter               (smart_routing.h/.cpp)
//
// Test suite name: APIGatewayEnhancementsFocusedTests

#include <gtest/gtest.h>

#include "server/request_coalescing.h"
#include "server/smart_routing.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::server;
namespace http = boost::beast::http;

// ============================================================================
// Helpers
// ============================================================================

static http::request<http::string_body>
makeReq(http::verb verb, const std::string& target) {
    http::request<http::string_body> req{verb, target, 11};
    req.set(http::field::host, "localhost");
    return req;
}

static auto echoHandler(const std::string& body = R"({"ok":true})") {
    return [body](const http::request<http::string_body>&) {
        http::response<http::string_body> resp{http::status::ok, 11};
        resp.set(http::field::content_type, "application/json");
        resp.body() = body;
        resp.prepare_payload();
        return resp;
    };
}

static BackendEndpoint makeBackend(const std::string& id,
                                   const std::string& addr = "127.0.0.1",
                                   uint16_t port = 8080) {
    BackendEndpoint ep;
    ep.backend_id = id;
    ep.address    = addr;
    ep.port       = port;
    return ep;
}

// ============================================================================
// RequestCoalescingManager – construction & defaults
// ============================================================================

TEST(RequestCoalescingTest, DefaultConfigDoesNotThrow) {
    EXPECT_NO_THROW({
        RequestCoalescingManager mgr;
    });
}

TEST(RequestCoalescingTest, CustomConfigConstruction) {
    RequestCoalescingManager::Config cfg;
    cfg.enabled              = true;
    cfg.max_waiters_per_key  = 50;
    cfg.waiter_timeout       = std::chrono::milliseconds{2000};
    EXPECT_NO_THROW({
        RequestCoalescingManager mgr(cfg);
    });
}

// ============================================================================
// RequestCoalescingManager – GET request goes to backend exactly once
// ============================================================================

TEST(RequestCoalescingTest, GetRequestReachesBackend) {
    RequestCoalescingManager mgr;

    std::atomic<int> call_count{0};
    auto handler = [&](const http::request<http::string_body>&) {
        call_count++;
        return echoHandler()({});
    };

    auto req  = makeReq(http::verb::get, "/api/v1/entities/1");
    auto resp = mgr.handle(req, handler);
    EXPECT_EQ(resp.result(), http::status::ok);
    EXPECT_EQ(call_count.load(), 1);
}

// ============================================================================
// RequestCoalescingManager – non-GET bypasses coalescing
// ============================================================================

TEST(RequestCoalescingTest, PostRequestBypassesCoalescing) {
    RequestCoalescingManager mgr;
    mgr.resetStats();

    std::atomic<int> call_count{0};
    auto handler = [&](const http::request<http::string_body>&) {
        call_count++;
        return echoHandler()({});
    };

    // Two POST requests to the same target must both reach the backend.
    auto req = makeReq(http::verb::post, "/api/v1/entities/1");
    mgr.handle(req, handler);
    mgr.handle(req, handler);

    EXPECT_EQ(call_count.load(), 2);

    auto stats = mgr.getStats();
    EXPECT_EQ(stats.coalesced_requests, 0ULL);
}

// ============================================================================
// RequestCoalescingManager – disabled manager is transparent
// ============================================================================

TEST(RequestCoalescingTest, DisabledManagerIsTransparent) {
    RequestCoalescingManager::Config cfg;
    cfg.enabled = false;
    RequestCoalescingManager mgr(cfg);

    std::atomic<int> call_count{0};
    auto handler = [&](const http::request<http::string_body>&) {
        call_count++;
        return echoHandler()({});
    };

    auto req = makeReq(http::verb::get, "/api/v1/entities/1");
    mgr.handle(req, handler);
    mgr.handle(req, handler);

    EXPECT_EQ(call_count.load(), 2);
}

// ============================================================================
// RequestCoalescingManager – statistics tracking
// ============================================================================

TEST(RequestCoalescingTest, StatsReflectSingleRequest) {
    RequestCoalescingManager mgr;
    mgr.resetStats();

    auto req = makeReq(http::verb::get, "/api/v1/entities/42");
    mgr.handle(req, echoHandler());

    auto stats = mgr.getStats();
    EXPECT_EQ(stats.total_requests,    1ULL);
    EXPECT_EQ(stats.backend_calls,     1ULL);
    EXPECT_EQ(stats.coalesced_requests, 0ULL);
}

TEST(RequestCoalescingTest, StatsResetWorks) {
    RequestCoalescingManager mgr;

    auto req = makeReq(http::verb::get, "/api/v1/entities/1");
    mgr.handle(req, echoHandler());
    mgr.handle(req, echoHandler());

    mgr.resetStats();
    auto stats = mgr.getStats();
    EXPECT_EQ(stats.total_requests,    0ULL);
    EXPECT_EQ(stats.backend_calls,     0ULL);
    EXPECT_EQ(stats.coalesced_requests, 0ULL);
}

// ============================================================================
// RequestCoalescingManager – inFlightCount starts at zero
// ============================================================================

TEST(RequestCoalescingTest, InitialInFlightCountIsZero) {
    RequestCoalescingManager mgr;
    EXPECT_EQ(mgr.inFlightCount(), 0UL);
}

TEST(RequestCoalescingTest, InFlightCountDropsToZeroAfterHandle) {
    RequestCoalescingManager mgr;
    auto req = makeReq(http::verb::get, "/api/v1/entities/7");
    mgr.handle(req, echoHandler());
    EXPECT_EQ(mgr.inFlightCount(), 0UL);
}

// ============================================================================
// RequestCoalescingManager – stats JSON serialisation
// ============================================================================

TEST(RequestCoalescingTest, StatsJsonContainsExpectedKeys) {
    RequestCoalescingManager mgr;
    auto stats = mgr.getStats();
    auto j = stats.toJson();
    EXPECT_TRUE(j.contains("total_requests"));
    EXPECT_TRUE(j.contains("coalesced_requests"));
    EXPECT_TRUE(j.contains("backend_calls"));
    EXPECT_TRUE(j.contains("timeout_fallbacks"));
    EXPECT_TRUE(j.contains("capacity_fallbacks"));
    EXPECT_TRUE(j.contains("coalescing_ratio"));
}

// ============================================================================
// RequestCoalescingManager – coalescing ratio computation
// ============================================================================

TEST(RequestCoalescingTest, CoalescingRatioIsZeroInitially) {
    RequestCoalescingManager mgr;
    auto stats = mgr.getStats();
    EXPECT_DOUBLE_EQ(stats.coalescingRatio(), 0.0);
}

// ============================================================================
// RequestCoalescingManager – max_waiters capacity fallback
// ============================================================================

TEST(RequestCoalescingTest, CapacityFallbackOccursWhenMaxWaitersReached) {
    // With max_waiters_per_key = 0, every incoming request that finds an
    // in-flight slot will immediately fall back to a direct backend call
    // because 0 >= 0 (current waiter_count >= max_waiters_per_key).
    RequestCoalescingManager::Config cfg;
    cfg.max_waiters_per_key = 0;
    cfg.waiter_timeout = std::chrono::milliseconds{500};
    RequestCoalescingManager mgr(cfg);
    mgr.resetStats();

    // Synchronisation: the originator signals when it has entered the handler
    // (i.e. the in-flight slot exists in the map), then waits for the second
    // request to proceed before returning.
    std::promise<void> originator_entered;
    std::promise<void> second_done;

    std::atomic<int> call_count{0};

    auto req = makeReq(http::verb::get, "/api/v1/entities/cap-test");

    std::thread originator([&] {
        mgr.handle(req, [&](const http::request<http::string_body>&) {
            call_count.fetch_add(1, std::memory_order_relaxed);
            originator_entered.set_value(); // slot is now in the map
            // Wait until the second request has been dispatched.
            second_done.get_future().wait_for(std::chrono::milliseconds{300});
            return echoHandler()({});
        });
    });

    // Wait for the originator to create the in-flight slot.
    originator_entered.get_future().wait_for(std::chrono::milliseconds{500});

    // Second request: the slot exists but max_waiters_per_key=0 forces fallback.
    mgr.handle(req, [&](const http::request<http::string_body>&) {
        call_count.fetch_add(1, std::memory_order_relaxed);
        return echoHandler()({});
    });
    second_done.set_value();

    originator.join();

    // Both requests must have reached the backend independently.
    EXPECT_EQ(call_count.load(), 2)
        << "Both requests must reach the backend when max_waiters_per_key=0";

    auto stats = mgr.getStats();
    EXPECT_GE(stats.capacity_fallbacks, 1ULL)
        << "At least one capacity fallback must be recorded";
}

// ============================================================================
// RequestCoalescingManager – concurrent requests share a single backend call
// ============================================================================

TEST(RequestCoalescingTest, ConcurrentDuplicateRequestsCoalesced) {
    RequestCoalescingManager::Config cfg;
    cfg.waiter_timeout = std::chrono::milliseconds{500};
    RequestCoalescingManager mgr(cfg);
    mgr.resetStats();

    std::atomic<int> backend_call_count{0};
    const int kClients = 5;

    // Barrier so all threads start roughly simultaneously.
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};

    auto handler = [&](const http::request<http::string_body>&) {
        backend_call_count.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds{40});
        return echoHandler()({});
    };

    std::vector<std::thread> threads;
    std::vector<http::response<http::string_body>> responses(kClients);

    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i] {
            auto req = makeReq(http::verb::get, "/api/v1/entities/shared");
            ready.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            responses[i] = mgr.handle(req, handler);
        });
    }

    // Let all threads reach the barrier.
    while (ready.load() < kClients) {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);

    for (auto& t : threads) t.join();

    // All responses must be OK.
    for (int i = 0; i < kClients; ++i) {
        EXPECT_EQ(responses[i].result(), http::status::ok)
            << "Client " << i << " must receive a valid response";
    }

    // The backend must have been called fewer times than the number of clients
    // (at least some coalescing must have happened).
    EXPECT_LT(backend_call_count.load(), kClients)
        << "At least some requests must be coalesced";

    auto stats = mgr.getStats();
    EXPECT_GT(stats.coalesced_requests, 0ULL)
        << "coalesced_requests stat must be non-zero";
    EXPECT_GT(stats.coalescingRatio(), 0.0)
        << "coalescing_ratio must be positive";
}

// ============================================================================
// SmartRouter – construction & registry
// ============================================================================

TEST(SmartRoutingTest, DefaultConfigDoesNotThrow) {
    EXPECT_NO_THROW({ SmartRouter router; });
}

TEST(SmartRoutingTest, AddAndListBackends) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));
    router.addBackend(makeBackend("shard-2"));

    auto backends = router.listBackends();
    EXPECT_EQ(backends.size(), 3UL);
}

TEST(SmartRoutingTest, AddDuplicateBackendIsIdempotent) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-0")); // duplicate

    auto backends = router.listBackends();
    EXPECT_EQ(backends.size(), 1UL);
}

TEST(SmartRoutingTest, RemoveBackend) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));
    router.removeBackend("shard-0");

    auto backends = router.listBackends();
    EXPECT_EQ(backends.size(), 1UL);
    EXPECT_EQ(backends[0].backend_id, "shard-1");
}

// ============================================================================
// SmartRouter – route returns nullopt when no backends registered
// ============================================================================

TEST(SmartRoutingTest, RouteReturnsNulloptWithNoBackends) {
    SmartRouter router;
    EXPECT_FALSE(router.route("/api/v1/entities/1").has_value());
    EXPECT_FALSE(router.routeLeastLoaded().has_value());
    EXPECT_FALSE(router.predictCachedBackend("key").has_value());
}

// ============================================================================
// SmartRouter – route returns a valid backend when one exists
// ============================================================================

TEST(SmartRoutingTest, RouteReturnsSingleBackend) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));

    auto result = router.route("/api/v1/entities/1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->backend_id, "shard-0");
}

// ============================================================================
// SmartRouter – recordLatency and stats update
// ============================================================================

TEST(SmartRoutingTest, RecordLatencyUpdatesStats) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));

    for (int i = 0; i < 10; ++i) {
        router.recordLatency("shard-0", 20.0 + i);
    }

    auto stats = router.getBackendStats("shard-0");
    EXPECT_GT(stats.avg_latency_ms, 0.0);
    EXPECT_GT(stats.p99_latency_ms, 0.0);
    EXPECT_EQ(stats.latency_samples, 10U);
    EXPECT_EQ(stats.total_requests,  10ULL);
}

TEST(SmartRoutingTest, LatencyWindowRollsOver) {
    SmartRouter::Config cfg;
    cfg.latency_window_size = 5;
    SmartRouter router(cfg);
    router.addBackend(makeBackend("shard-0"));

    for (int i = 0; i < 20; ++i) {
        router.recordLatency("shard-0", static_cast<double>(i));
    }

    auto stats = router.getBackendStats("shard-0");
    EXPECT_EQ(stats.latency_samples, 5U);
    EXPECT_EQ(stats.total_requests, 20ULL);
}

// ============================================================================
// SmartRouter – least-loaded routing
// ============================================================================

TEST(SmartRoutingTest, LeastLoadedChoosesMinActiveConnections) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));
    router.addBackend(makeBackend("shard-2"));

    // Simulate load on shard-0 and shard-1.
    router.incrementActiveConnections("shard-0");
    router.incrementActiveConnections("shard-0");
    router.incrementActiveConnections("shard-1");

    auto result = router.routeLeastLoaded();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->backend_id, "shard-2")
        << "shard-2 has fewest active connections (0)";
}

TEST(SmartRoutingTest, ActiveConnectionsDecrementWorks) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));

    router.incrementActiveConnections("shard-0");
    router.incrementActiveConnections("shard-0");
    router.decrementActiveConnections("shard-0");
    // shard-0 has 1; shard-1 has 0

    auto result = router.routeLeastLoaded();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->backend_id, "shard-1");
}

TEST(SmartRoutingTest, DecrementBelowZeroDoesNotUnderflow) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));

    // Decrement without any prior increment.
    EXPECT_NO_THROW(router.decrementActiveConnections("shard-0"));

    auto stats = router.getBackendStats("shard-0");
    EXPECT_EQ(stats.active_connections, 0U);
}

// ============================================================================
// SmartRouter – tail-latency avoidance
// ============================================================================

TEST(SmartRoutingTest, HighTailLatencyBackendExcluded) {
    SmartRouter::Config cfg;
    cfg.tail_latency_threshold_ms = 100.0;
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-fast"));
    router.addBackend(makeBackend("shard-slow"));

    // Make shard-slow high-tail.
    for (int i = 0; i < 20; ++i) {
        router.recordLatency("shard-slow", 600.0); // well above 100 ms threshold
    }
    // shard-fast has no latency samples → not considered high-tail.

    auto result = router.routeLeastLoaded();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->backend_id, "shard-fast")
        << "high-tail backend must be excluded when an alternative exists";
}

TEST(SmartRoutingTest, AllHighTailFallsBackToAnyBackend) {
    SmartRouter::Config cfg;
    cfg.tail_latency_threshold_ms = 10.0; // very tight threshold
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-0"));
    // Record latency above threshold so the backend is "high-tail".
    for (int i = 0; i < 20; ++i) {
        router.recordLatency("shard-0", 500.0);
    }

    // With only one backend that is high-tail, routing must still return it.
    auto result = router.routeLeastLoaded();
    ASSERT_TRUE(result.has_value())
        << "Must return a backend even if all are high-tail";
    EXPECT_EQ(result->backend_id, "shard-0");
}

// ============================================================================
// SmartRouter – cache-hit prediction
// ============================================================================

TEST(SmartRoutingTest, CachePredictionWithSufficientHistory) {
    SmartRouter::Config cfg;
    cfg.min_cache_prediction_hits = 3;
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));

    // shard-0 has seen the key many times.
    for (int i = 0; i < 5; ++i) {
        router.recordCacheHit("shard-0", "entity:42");
    }

    auto predicted = router.predictCachedBackend("entity:42");
    ASSERT_TRUE(predicted.has_value());
    EXPECT_EQ(predicted->backend_id, "shard-0")
        << "shard-0 should be predicted for entity:42";
}

TEST(SmartRoutingTest, CachePredictionInsufficientHistoryReturnsNullopt) {
    SmartRouter::Config cfg;
    cfg.min_cache_prediction_hits = 5;
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-0"));

    // Only 2 hits – below the minimum.
    router.recordCacheHit("shard-0", "entity:1");
    router.recordCacheHit("shard-0", "entity:1");

    EXPECT_FALSE(router.predictCachedBackend("entity:1").has_value())
        << "Below minimum hit count should return nullopt";
}

TEST(SmartRoutingTest, CachePredictionPicksMostFrequentBackend) {
    SmartRouter::Config cfg;
    cfg.min_cache_prediction_hits = 2;
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));

    router.recordCacheHit("shard-0", "hot-key");
    router.recordCacheHit("shard-0", "hot-key");
    router.recordCacheHit("shard-1", "hot-key");
    router.recordCacheHit("shard-1", "hot-key");
    router.recordCacheHit("shard-1", "hot-key"); // shard-1 wins with 3

    auto predicted = router.predictCachedBackend("hot-key");
    ASSERT_TRUE(predicted.has_value());
    EXPECT_EQ(predicted->backend_id, "shard-1");
}

// ============================================================================
// SmartRouter – route() applies cache prediction first
// ============================================================================

TEST(SmartRoutingTest, RoutePrefersCachePredictionOverLeastLoaded) {
    SmartRouter::Config cfg;
    cfg.enable_cache_prediction  = true;
    cfg.min_cache_prediction_hits = 3;
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));

    // Make shard-1 the predicted cache holder.
    for (int i = 0; i < 4; ++i) {
        router.recordCacheHit("shard-1", "entity:99");
    }

    // Load shard-1 with connections so least-loaded would prefer shard-0.
    router.incrementActiveConnections("shard-1");
    router.incrementActiveConnections("shard-1");

    auto result = router.route("entity:99");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->backend_id, "shard-1")
        << "Cache prediction must take priority over least-loaded";
}

// ============================================================================
// SmartRouter – cache miss recording
// ============================================================================

TEST(SmartRoutingTest, RecordCacheMissIncrementsMissCounter) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));

    router.recordCacheMiss("shard-0", "entity:1");
    router.recordCacheMiss("shard-0", "entity:1");

    auto stats = router.getBackendStats("shard-0");
    EXPECT_EQ(stats.cache_misses, 2ULL);
}

// ============================================================================
// SmartRouter – getAllStats returns all registered backends
// ============================================================================

TEST(SmartRoutingTest, GetAllStatsContainsAllBackends) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));
    router.addBackend(makeBackend("shard-2"));

    auto all = router.getAllStats();
    EXPECT_EQ(all.size(), 3UL);
}

TEST(SmartRoutingTest, GetBackendStatsThrowsForUnknown) {
    SmartRouter router;
    EXPECT_THROW(router.getBackendStats("unknown-shard"), std::out_of_range);
}

// ============================================================================
// SmartRouter – stats JSON serialisation
// ============================================================================

TEST(SmartRoutingTest, BackendStatsJsonContainsExpectedKeys) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.recordLatency("shard-0", 25.0);

    auto stats = router.getBackendStats("shard-0");
    auto j = stats.toJson();
    EXPECT_TRUE(j.contains("backend_id"));
    EXPECT_TRUE(j.contains("avg_latency_ms"));
    EXPECT_TRUE(j.contains("p99_latency_ms"));
    EXPECT_TRUE(j.contains("total_requests"));
    EXPECT_TRUE(j.contains("active_connections"));
    EXPECT_TRUE(j.contains("cache_hits"));
    EXPECT_TRUE(j.contains("cache_misses"));
    EXPECT_TRUE(j.contains("latency_samples"));
}

// ============================================================================
// SmartRouter – concurrent record operations (data-race safety)
// ============================================================================

TEST(SmartRoutingTest, ConcurrentLatencyRecordingNoDataRace) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));

    const int kThreads  = 4;
    const int kIter     = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kIter; ++i) {
                router.recordLatency("shard-" + std::to_string(t % 2),
                                     10.0 + i);
                router.recordCacheHit("shard-" + std::to_string(t % 2),
                                      "key-" + std::to_string(i));
                router.incrementActiveConnections("shard-" + std::to_string(t % 2));
                router.decrementActiveConnections("shard-" + std::to_string(t % 2));
            }
        });
    }

    for (auto& th : threads) th.join();

    // If we reach here without a crash/sanitizer error, the test passes.
    SUCCEED();
}

// ============================================================================
// SmartRouter – removing unknown backend is a no-op
// ============================================================================

TEST(SmartRoutingTest, RemoveUnknownBackendIsNoOp) {
    SmartRouter router;
    router.addBackend(makeBackend("shard-0"));
    EXPECT_NO_THROW(router.removeBackend("nonexistent"));

    auto backends = router.listBackends();
    EXPECT_EQ(backends.size(), 1UL);
}

// ============================================================================
// SmartRouter – route with cache prediction disabled falls back to load-aware
// ============================================================================

TEST(SmartRoutingTest, RouteLeastLoadedWhenCachePredictionDisabled) {
    SmartRouter::Config cfg;
    cfg.enable_cache_prediction = false;
    SmartRouter router(cfg);

    router.addBackend(makeBackend("shard-0"));
    router.addBackend(makeBackend("shard-1"));

    // Record hits for shard-0 to ensure prediction would choose it.
    for (int i = 0; i < 10; ++i) {
        router.recordCacheHit("shard-0", "entity:55");
    }

    // Make shard-1 the least loaded.
    router.incrementActiveConnections("shard-0");
    router.incrementActiveConnections("shard-0");

    auto result = router.route("entity:55");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->backend_id, "shard-1")
        << "When cache prediction is disabled, least-loaded must be used";
}

// ============================================================================
// RequestCoalescingManager – HEAD method is coalesced like GET
// ============================================================================

TEST(RequestCoalescingTest, HeadRequestIsCoalesced) {
    // HEAD is safe and idempotent – it must be eligible for coalescing.
    RequestCoalescingManager::Config cfg;
    cfg.waiter_timeout = std::chrono::milliseconds{500};
    RequestCoalescingManager mgr(cfg);
    mgr.resetStats();

    std::atomic<int> backend_call_count{0};
    const int kClients = 4;
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};

    auto handler = [&](const http::request<http::string_body>&) {
        backend_call_count.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds{40});
        http::response<http::string_body> resp{http::status::ok, 11};
        resp.set(http::field::content_type, "application/json");
        resp.prepare_payload();
        return resp;
    };

    std::vector<std::thread> threads;
    std::vector<http::response<http::string_body>> responses(kClients);

    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i] {
            auto req = makeReq(http::verb::head, "/api/v1/entities/head-resource");
            ready.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) std::this_thread::yield();
            responses[i] = mgr.handle(req, handler);
        });
    }

    while (ready.load() < kClients) std::this_thread::yield();
    go.store(true, std::memory_order_release);

    for (auto& t : threads) t.join();

    for (int i = 0; i < kClients; ++i) {
        EXPECT_EQ(responses[i].result(), http::status::ok)
            << "Client " << i << " must receive a valid response";
    }
    EXPECT_LT(backend_call_count.load(), kClients)
        << "HEAD requests must be coalesced (fewer backend calls than clients)";

    auto stats = mgr.getStats();
    EXPECT_GT(stats.coalesced_requests, 0ULL)
        << "coalesced_requests must be non-zero for HEAD";
}

// ============================================================================
// RequestCoalescingManager – backend exception falls back in waiters
// ============================================================================

TEST(RequestCoalescingTest, OriginatorExceptionTriggersWaiterFallback) {
    // When the originator's backend call throws, waiters must fall back to
    // their own direct backend call rather than propagating the exception.
    RequestCoalescingManager::Config cfg;
    cfg.waiter_timeout = std::chrono::milliseconds{500};
    RequestCoalescingManager mgr(cfg);
    mgr.resetStats();

    std::atomic<int> call_count{0};

    // Slow handler: the first call (originator) throws; subsequent calls succeed.
    auto handler = [&](const http::request<http::string_body>& r) {
        int n = call_count.fetch_add(1, std::memory_order_relaxed);
        if (n == 0) {
            // Originator throws after a short delay (allows a waiter to register).
            std::this_thread::sleep_for(std::chrono::milliseconds{30});
            throw std::runtime_error("backend unavailable");
        }
        return echoHandler()(r);
    };

    auto req = makeReq(http::verb::get, "/api/v1/entities/throw-test");

    // Launch originator in background.
    std::promise<void> slot_ready;
    std::thread originator([&] {
        try {
            slot_ready.set_value();
            mgr.handle(req, handler);
        } catch (...) {
            // originator exception is re-thrown to the caller – that's OK.
        }
    });

    slot_ready.get_future().wait_for(std::chrono::milliseconds{200});
    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // Waiter: the originator will fail; the waiter must recover via fallback.
    http::response<http::string_body> waiter_resp;
    EXPECT_NO_THROW(waiter_resp = mgr.handle(req, handler))
        << "Waiter must not propagate originator exception";

    originator.join();

    // The waiter fell back to its own backend call (which succeeds).
    EXPECT_EQ(waiter_resp.result(), http::status::ok)
        << "Waiter fallback must return a valid response";
    EXPECT_GE(call_count.load(), 2)
        << "Both originator and waiter fallback must have reached the backend";
}
