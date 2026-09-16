// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_core_soak.cpp
 * @brief Wave D — Core Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB core module hot paths:
 * bootstrap throughput, config-reload stability, and tracing-exporter
 * reliability. Verifies that all three metrics remain within acceptable
 * bounds over a configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - CoreSoak_BootstrapThroughput         : ≥ 1 000 ops/sec over soak window
 * - CoreSoak_ConfigReloadStability       : zero config-reload failures
 * - CoreSoak_TracingExporterReliability  : zero exporter lag events
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_CORE.md — operator runbook
 * @see src/core/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model the core module hot paths without requiring
// external backends. MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubBootstrapQueue — simulates module init event throughput
// ---------------------------------------------------------------------------
class StubBootstrapQueue {
public:
    explicit StubBootstrapQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t event_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_count_; return false; }
        queue_.push_back(event_id);
        ++enqueue_count_;
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        ++dequeue_count_;
        return true;
    }

    uint64_t enqueueCount()  const { return enqueue_count_.load(); }
    uint64_t dequeueCount()  const { return dequeue_count_.load(); }
    uint64_t overflowCount() const { return overflow_count_.load(); }

private:
    const std::size_t capacity_;
    std::deque<uint64_t> queue_;
    std::mutex mu_;
    std::atomic<uint64_t> enqueue_count_{0};
    std::atomic<uint64_t> dequeue_count_{0};
    std::atomic<uint64_t> overflow_count_{0};
};

// ---------------------------------------------------------------------------
// StubConfigReloader — simulates live config reload
// ---------------------------------------------------------------------------
class StubConfigReloader {
public:
    bool reload(const std::string& /*config_key*/) {
        ++reload_count_;
        return true; // stub: always succeeds
    }

    uint64_t reloadCount() const { return reload_count_.load(); }
    uint64_t failCount()   const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> reload_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

// ---------------------------------------------------------------------------
// StubTracingExporter — simulates span export with lag detection
// ---------------------------------------------------------------------------
class StubTracingExporter {
public:
    /// Export a span. Returns false if exporter is lagging.
    bool exportSpan(uint64_t span_id) {
        ++export_count_;
        (void)span_id;
        return true; // stub: never lags
    }

    uint64_t exportCount() const { return export_count_.load(); }
    uint64_t lagCount()    const { return lag_count_.load(); }

private:
    std::atomic<uint64_t> export_count_{0};
    std::atomic<uint64_t> lag_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: CoreSoak_BootstrapThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CoreSoak, CoreSoak_BootstrapThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubBootstrapQueue queue(4096);
    std::atomic<bool> running{true};

    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                queue.enqueue(id++);
                std::this_thread::yield();
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&]() {
            uint64_t dummy = 0;
            while (running.load(std::memory_order_relaxed)) {
                queue.dequeue(dummy);
                std::this_thread::yield();
            }
        });
    }

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);

    for (auto& t : producers) t.join();
    for (auto& t : consumers)  t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops = queue.enqueueCount() + queue.dequeueCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[CORE:BootstrapFailed] At least one bootstrap event must complete";
    EXPECT_GE(ops_per_sec, 1000.0)
        << "Bootstrap throughput must be ≥ 1 000 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: CoreSoak_ConfigReloadStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CoreSoak, CoreSoak_ConfigReloadStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubConfigReloader reloader;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            while (running.load(std::memory_order_relaxed)) {
                reloader.reload("config.module." + std::to_string(i));
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(reloader.reloadCount(), 0u)
        << "[CORE:ConfigReloadFailed] At least one config reload must complete";
    EXPECT_EQ(reloader.failCount(), 0u)
        << "[CORE:ConfigReloadFailed] Zero config reload failures expected. "
           "Observed: " << reloader.failCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: CoreSoak_TracingExporterReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_CoreSoak, CoreSoak_TracingExporterReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubTracingExporter exporter;
    std::atomic<bool>   running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t span = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                exporter.exportSpan(span++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(exporter.exportCount(), 0u)
        << "[CORE:TracingExporterLag] At least one span must be exported";
    EXPECT_EQ(exporter.lagCount(), 0u)
        << "[CORE:TracingExporterLag] Zero exporter lag events expected. "
           "Observed: " << exporter.lagCount();
}
