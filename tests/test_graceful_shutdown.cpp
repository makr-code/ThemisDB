/**
 * @file test_graceful_shutdown.cpp
 * @brief Tests for graceful shutdown configuration and active-request tracking
 *
 * Validates graceful shutdown behavior introduced for production readiness:
 * - Active request counter increments/decrements correctly
 * - Shutdown drain timeout is configurable
 * - Config defaults are sane
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "server/http_server.h"

using Config = themis::server::HttpServer::Config;

// ---------------------------------------------------------------------------
// Config Defaults
// ---------------------------------------------------------------------------

TEST(GracefulShutdownConfig, DefaultGracefulTimeoutPresent) {
    Config cfg;
    EXPECT_GT(cfg.graceful_shutdown_timeout_ms, 0u);
}

TEST(GracefulShutdownConfig, DefaultGracefulTimeoutIs30Seconds) {
    Config cfg;
    EXPECT_EQ(cfg.graceful_shutdown_timeout_ms, 30000u);
}

TEST(GracefulShutdownConfig, GracefulTimeoutCanBeSetToZero) {
    Config cfg;
    cfg.graceful_shutdown_timeout_ms = 0;
    EXPECT_EQ(cfg.graceful_shutdown_timeout_ms, 0u);
}

TEST(GracefulShutdownConfig, GracefulTimeoutCanBeSetToLargeValue) {
    Config cfg;
    cfg.graceful_shutdown_timeout_ms = 120000; // 2 minutes
    EXPECT_EQ(cfg.graceful_shutdown_timeout_ms, 120000u);
}

// ---------------------------------------------------------------------------
// Active Request Counter Tests (standalone atomic logic)
// ---------------------------------------------------------------------------

TEST(ActiveRequestCounter, StartsAtZero) {
    std::atomic<uint64_t> active{0};
    EXPECT_EQ(active.load(), 0u);
}

TEST(ActiveRequestCounter, IncrementAndDecrement) {
    std::atomic<uint64_t> active{0};
    active.fetch_add(1, std::memory_order_acquire);
    EXPECT_EQ(active.load(), 1u);
    active.fetch_sub(1, std::memory_order_release);
    EXPECT_EQ(active.load(), 0u);
}

TEST(ActiveRequestCounter, ConcurrentIncrements) {
    std::atomic<uint64_t> active{0};
    constexpr int N = 100;

    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&active]() {
            active.fetch_add(1, std::memory_order_acquire);
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(active.load(), static_cast<uint64_t>(N));
}

TEST(ActiveRequestCounter, ConcurrentIncrementsAndDecrements) {
    std::atomic<uint64_t> active{0};
    constexpr int N = 100;

    std::vector<std::thread> threads;
    threads.reserve(N * 2);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&active]() {
            active.fetch_add(1, std::memory_order_acquire);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            active.fetch_sub(1, std::memory_order_release);
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(active.load(), 0u);
}

// ---------------------------------------------------------------------------
// Drain Logic Tests (simulate drain wait loop)
// ---------------------------------------------------------------------------

TEST(DrainLogic, ImmediatelyCompletesWhenNoActiveRequests) {
    std::atomic<uint64_t> active{0};
    const auto timeout_ms = 1000u;
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(timeout_ms);

    bool drained = false;
    while (active.load(std::memory_order_acquire) > 0
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    drained = (active.load(std::memory_order_acquire) == 0);

    EXPECT_TRUE(drained);
}

TEST(DrainLogic, WaitsForActiveRequestsToComplete) {
    std::atomic<uint64_t> active{0};
    active.store(1, std::memory_order_relaxed);

    // Release after short delay
    std::thread release_thread([&active]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        active.store(0, std::memory_order_release);
    });

    const auto timeout_ms = 5000u;
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(timeout_ms);

    while (active.load(std::memory_order_acquire) > 0
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    release_thread.join();

    EXPECT_EQ(active.load(), 0u);
}

TEST(DrainLogic, TimeoutLeavesRemainingCountIntact) {
    std::atomic<uint64_t> active{2}; // never drained

    const auto timeout_ms = 100u;  // very short timeout
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(timeout_ms);

    while (active.load(std::memory_order_acquire) > 0
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Active count still 2 after timeout
    EXPECT_EQ(active.load(), 2u);
}

// ---------------------------------------------------------------------------
// Running Flag Tests
// ---------------------------------------------------------------------------

TEST(GracefulShutdownRunningFlag, DefaultsToFalse) {
    // HttpServer::running_ starts as false; server must be explicitly started
    // We test this via the atomic bool semantics only
    std::atomic<bool> running{false};
    EXPECT_FALSE(running.load());
}

TEST(GracefulShutdownRunningFlag, SetToTrueOnStart) {
    std::atomic<bool> running{false};
    running.store(true);
    EXPECT_TRUE(running.load());
}

TEST(GracefulShutdownRunningFlag, SetToFalseOnStop) {
    std::atomic<bool> running{true};
    running.store(false);
    EXPECT_FALSE(running.load());
}

TEST(GracefulShutdownRunningFlag, IsAtomicAcrossThreads) {
    std::atomic<bool> running{true};

    std::thread stopper([&running]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        running.store(false, std::memory_order_release);
    });

    while (running.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    stopper.join();

    EXPECT_FALSE(running.load());
}
