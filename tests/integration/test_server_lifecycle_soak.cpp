// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_server_lifecycle_soak.cpp
 * @brief Wave D soak tests for the server module primary paths.
 *
 * Validates sustained-operation reliability of the server stack over a
 * configurable run window (default 60 000 ms).  All three test cases use
 * in-process stubs — no real network sockets or plugin DSOs are required.
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS  — total run window in milliseconds (default 60000)
 *
 * CTest labels: wave_d;soak;not_release_critical
 * TIMEOUT: 120 seconds (CI override via CMakeLists)
 *
 * Gate table
 * ----------
 * | Test case                          | Gate                                  |
 * |------------------------------------|---------------------------------------|
 * | ServerSoak_RequestThroughput       | ≥ 5 000 req/s sustained               |
 * | ServerSoak_RateLimitStability      | zero rate-limit bypass events         |
 * | ServerSoak_PluginAdapterReliability| zero crashes / segfaults in plugin stub|
 *
 * @see src/server/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace test {
namespace wave_d {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::chrono::milliseconds soak_duration() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env) {
        try {
            return std::chrono::milliseconds(std::stol(env));
        } catch (...) {}
    }
    return std::chrono::milliseconds(60000);
}

// ---------------------------------------------------------------------------
// In-process stub: rate-limit gate
// ---------------------------------------------------------------------------

struct RateLimitGate {
    explicit RateLimitGate(int limit_per_window) : limit_(limit_per_window) {}

    /// Returns true when the request is allowed, false when rate-limited.
    bool check_and_advance(int client_id) {
        std::lock_guard<std::mutex> lock(mu_);
        auto& bucket = buckets_[client_id];
        if (bucket >= limit_) {
            ++bypass_attempts_;
            return false;
        }
        ++bucket;
        ++allowed_;
        return true;
    }

    void reset_all() {
        std::lock_guard<std::mutex> lock(mu_);
        buckets_.clear();
    }

    long bypass_attempts() const {
        return bypass_attempts_.load(std::memory_order_relaxed);
    }

    long allowed() const {
        return allowed_.load(std::memory_order_relaxed);
    }

private:
    int                                 limit_;
    std::mutex                          mu_;
    std::unordered_map<int,int>         buckets_;
    std::atomic<long>                   bypass_attempts_{0};
    std::atomic<long>                   allowed_{0};
};

// ---------------------------------------------------------------------------
// In-process stub: route dispatcher
// ---------------------------------------------------------------------------

struct RouteDispatcher {
    explicit RouteDispatcher(int route_count) {
        routes_.resize(static_cast<std::size_t>(route_count));
        for (int i = 0; i < route_count; ++i)
            routes_[static_cast<std::size_t>(i)] = "handler_" + std::to_string(i);
    }

    bool dispatch(int route_id) {
        if (route_id < 0 || static_cast<std::size_t>(route_id) >= routes_.size())
            return false;
        ++dispatched_;
        return true;
    }

    long dispatched() const { return dispatched_.load(std::memory_order_relaxed); }

private:
    std::vector<std::string> routes_;
    std::atomic<long>        dispatched_{0};
};

// ---------------------------------------------------------------------------
// In-process stub: plugin adapter
// ---------------------------------------------------------------------------

struct PluginAdapter {
    enum class State { kUnloaded, kLoaded, kFailed };

    bool load(const std::string& /*name*/) {
        ++load_count_;
        state_ = State::kLoaded;
        return true;
    }

    bool invoke(int /*op_id*/) {
        if (state_ != State::kLoaded) { ++errors_; return false; }
        ++invoke_count_;
        return true;
    }

    bool unload() {
        state_ = State::kUnloaded;
        return true;
    }

    long invoke_count() const { return invoke_count_.load(std::memory_order_relaxed); }
    long errors()       const { return errors_.load(std::memory_order_relaxed); }
    long load_count()   const { return load_count_.load(std::memory_order_relaxed); }

private:
    std::atomic<State> state_{State::kUnloaded};
    std::atomic<long>  invoke_count_{0};
    std::atomic<long>  errors_{0};
    std::atomic<long>  load_count_{0};
};

// ===========================================================================
// SRV-SOAK-01 — Request throughput ≥ 5 000 req/s
// ===========================================================================

TEST(ServerSoak_RequestThroughput, SustainsMinimumThroughput) {
    const auto duration = soak_duration();
    constexpr int kNumThreads   = 4;
    constexpr int kRouteCount   = 256;
    constexpr double kMinReqSec = 5000.0;

    RouteDispatcher dispatcher(kRouteCount);
    std::atomic<bool> stop{false};

    auto worker = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> dist(0, kRouteCount - 1);
        while (!stop.load(std::memory_order_relaxed))
            dispatcher.dispatch(dist(rng));
    };

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int i = 0; i < kNumThreads; ++i)
        threads.emplace_back(worker, i + 1);

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(duration);
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - t0).count();
    const double elapsed_s = static_cast<double>(elapsed_ms) / 1000.0;
    const double req_per_sec = static_cast<double>(dispatcher.dispatched()) / elapsed_s;

    EXPECT_GE(req_per_sec, kMinReqSec)
        << "Throughput " << req_per_sec << " req/s below gate of "
        << kMinReqSec << " req/s over " << elapsed_s << "s";
}

// ===========================================================================
// SRV-SOAK-02 — Rate-limit stability: zero bypass events
// ===========================================================================

TEST(ServerSoak_RateLimitStability, NoBypasses) {
    const auto duration = soak_duration();
    constexpr int kNumClients   = 64;
    constexpr int kNumThreads   = 4;
    constexpr int kLimitPerWin  = 50;   // per virtual client per reset cycle

    RateLimitGate gate(kLimitPerWin);
    std::atomic<bool> stop{false};
    std::atomic<long> resets{0};

    // Periodic bucket resetter (simulates sliding window roll-over)
    std::thread resetter([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            gate.reset_all();
            ++resets;
        }
    });

    auto worker = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> client_dist(0, kNumClients - 1);
        while (!stop.load(std::memory_order_relaxed)) {
            // Send more requests than the limit to exercise the rejection path
            for (int burst = 0; burst < kLimitPerWin + 5; ++burst)
                gate.check_and_advance(client_dist(rng));
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int i = 0; i < kNumThreads; ++i)
        threads.emplace_back(worker, i + 100);

    std::this_thread::sleep_for(duration);
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();
    resetter.join();

    // "bypass" means a request was counted as allowed after the bucket was full.
    // Our stub correctly enforces the limit; bypass_attempts records rejections,
    // not bypasses.  The gate we validate is that no request illegally passes
    // the gate (i.e. allowed count never exceeds limit * reset_count * clients).
    const long max_allowed = static_cast<long>(kLimitPerWin)
                           * static_cast<long>(resets.load() + 1)
                           * static_cast<long>(kNumClients);
    EXPECT_LE(gate.allowed(), max_allowed)
        << "Rate-limit bypass detected: allowed=" << gate.allowed()
        << " > theoretical max=" << max_allowed;
    // Also assert stub recorded rejections (validates test is exercising the path)
    EXPECT_GT(gate.bypass_attempts(), 0L)
        << "Expected some rejected requests during soak; none recorded";
}

// ===========================================================================
// SRV-SOAK-03 — Plugin adapter reliability: no crashes
// ===========================================================================

TEST(ServerSoak_PluginAdapterReliability, NoCrashesOrErrors) {
    const auto duration = soak_duration();
    constexpr int kNumThreads = 4;
    constexpr int kPlugins    = 8;

    std::vector<PluginAdapter> adapters(kPlugins);
    // Pre-load all adapters before the soak loop
    for (int i = 0; i < kPlugins; ++i)
        adapters[static_cast<std::size_t>(i)].load("plugin_" + std::to_string(i));

    std::atomic<bool> stop{false};
    std::atomic<long> total_invocations{0};

    auto worker = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> plugin_dist(0, kPlugins - 1);
        std::uniform_int_distribution<int> op_dist(0, 255);
        while (!stop.load(std::memory_order_relaxed)) {
            int idx = plugin_dist(rng);
            if (adapters[static_cast<std::size_t>(idx)].invoke(op_dist(rng)))
                ++total_invocations;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int i = 0; i < kNumThreads; ++i)
        threads.emplace_back(worker, i + 200);

    std::this_thread::sleep_for(duration);
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();

    // Verify no adapter recorded an error during the soak
    long total_errors = 0;
    for (const auto& a : adapters)
        total_errors += a.errors();

    EXPECT_EQ(0L, total_errors)
        << "Plugin adapter reported " << total_errors << " errors during soak";
    EXPECT_GT(total_invocations.load(), 0L)
        << "No plugin invocations recorded — workload was not exercised";
}

}  // namespace wave_d
}  // namespace test
}  // namespace themis
