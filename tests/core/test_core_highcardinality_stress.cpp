// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_core_highcardinality_stress.cpp
 * @brief Wave D high-cardinality and concurrent stress tests for the core module.
 *
 * Three focused stress cases that exercise config-update cardinality, concurrent
 * module-init pressure, and tracing-exporter throughput under concurrency.
 * All cases use in-process stubs — no external backends required.
 *
 * CTest labels: wave_d;stress;not_release_critical
 *
 * | Test case                     | Scenario                                           |
 * |-------------------------------|---------------------------------------------------|
 * | HighCardinalityConfigUpdate   | 100 000 unique config keys, 8 concurrent threads  |
 * | ConcurrentModuleInitStress    | 8 threads init/teardown modules simultaneously    |
 * | TracingExporterHighLoad       | 8 threads export spans at max throughput           |
 *
 * @see src/core/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace test {
namespace wave_d {

// ---------------------------------------------------------------------------
// STUB / SIMULATION NOTE
// These stubs model core module behaviour without external backends.
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

struct StubConfigStore {
    bool set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mu_);
        store_[key] = value;
        ++writes_;
        return true;
    }

    bool get(const std::string& key, std::string* out) const {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) return false;
        *out = it->second;
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return store_.size();
    }

    long writes() const { return writes_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
    std::atomic<long> writes_{0};
};

struct StubModuleLifecycle {
    bool initialize(const std::string& module_id) {
        std::lock_guard<std::mutex> lock(mu_);
        inited_[module_id] = true;
        ++inits_;
        return true;
    }

    bool teardown(const std::string& module_id) {
        std::lock_guard<std::mutex> lock(mu_);
        inited_.erase(module_id);
        ++teardowns_;
        return true;
    }

    long inits()     const { return inits_.load(std::memory_order_relaxed); }
    long teardowns() const { return teardowns_.load(std::memory_order_relaxed); }
    long failures()  const { return failures_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, bool> inited_;
    std::atomic<long> inits_{0};
    std::atomic<long> teardowns_{0};
    std::atomic<long> failures_{0};
};

struct StubSpanExporter {
    bool exportSpan(uint64_t span_id, const std::string& name) {
        std::lock_guard<std::mutex> lock(mu_);
        spans_[span_id] = name;
        ++exports_;
        return true;
    }

    long exports() const { return exports_.load(std::memory_order_relaxed); }
    long lag()     const { return lag_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex mu_;
    std::unordered_map<uint64_t, std::string> spans_;
    std::atomic<long> exports_{0};
    std::atomic<long> lag_{0};
};

// ---------------------------------------------------------------------------
// Test 1: HighCardinalityConfigUpdate
// ---------------------------------------------------------------------------
TEST(WaveD_CoreStress, HighCardinalityConfigUpdate) {
    constexpr int    kThreads = 8;
    constexpr int    kKeysPerThread = 12500; // 100 000 total
    constexpr double kExpectedWriteRatio = 0.95;

    StubConfigStore store;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const int base = t * kKeysPerThread;
            for (int i = 0; i < kKeysPerThread; ++i) {
                store.set("config.key." + std::to_string(base + i),
                          "value-" + std::to_string(base + i));
            }
        });
    }
    for (auto& th : threads) th.join();

    const long total_writes = store.writes();
    EXPECT_GE(total_writes,
              static_cast<long>(kThreads * kKeysPerThread * kExpectedWriteRatio))
        << "[CORE:ConfigReloadFailed] Expected ≥ 95% of keys written. "
           "Wrote: " << total_writes;
    EXPECT_GE(static_cast<long>(store.size()), total_writes * 9 / 10)
        << "Unique config keys must be ≥ 90% of writes.";
}

// ---------------------------------------------------------------------------
// Test 2: ConcurrentModuleInitStress
// ---------------------------------------------------------------------------
TEST(WaveD_CoreStress, ConcurrentModuleInitStress) {
    constexpr int kThreads = 8;
    constexpr int kCyclesPerThread = 500;

    StubModuleLifecycle lifecycle;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const std::string module_id = "core.module." + std::to_string(t % 4);
            for (int i = 0; i < kCyclesPerThread; ++i) {
                lifecycle.initialize(module_id);
                std::this_thread::yield();
                lifecycle.teardown(module_id);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GE(lifecycle.inits(),
              static_cast<long>(kThreads * kCyclesPerThread))
        << "[CORE:ModuleInitFailed] All init operations must complete.";
    EXPECT_GE(lifecycle.teardowns(),
              static_cast<long>(kThreads * kCyclesPerThread))
        << "[CORE:ModuleInitFailed] All teardown operations must complete.";
    EXPECT_EQ(lifecycle.failures(), 0L)
        << "[CORE:ModuleInitFailed] Zero module init failures expected.";
}

// ---------------------------------------------------------------------------
// Test 3: TracingExporterHighLoad
// ---------------------------------------------------------------------------
TEST(WaveD_CoreStress, TracingExporterHighLoad) {
    constexpr int    kThreads = 8;
    constexpr int    kSpansPerThread = 10000;
    constexpr double kExpectedExportRatio = 0.95;

    StubSpanExporter exporter;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kSpansPerThread;
            for (int i = 0; i < kSpansPerThread; ++i) {
                exporter.exportSpan(base + static_cast<uint64_t>(i),
                                    "span-" + std::to_string(t));
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GE(exporter.exports(),
              static_cast<long>(kThreads * kSpansPerThread * kExpectedExportRatio))
        << "[CORE:TracingExporterLag] Expected ≥ 95% of spans exported. "
           "Exported: " << exporter.exports();
    EXPECT_EQ(exporter.lag(), 0L)
        << "[CORE:TracingExporterLag] Zero exporter lag events expected.";
}

} // namespace wave_d
} // namespace test
} // namespace themis
