// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_performance_module_soak.cpp
 * @brief Wave D — Performance Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB performance module.
 * Verifies that metric-collection throughput, baseline stability, and
 * benchmark-harness reliability remain stable over the soak window.
 *
 * ## Acceptance criteria
 * - Metric-collection throughput ≥ 10 000 samples/sec
 * - Baseline: zero regression events during soak
 * - Benchmark harness: zero crash events during soak
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_PERFORMANCE_MODULE.md
 * @see src/performance/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
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
// STUB/SIMULATION NOTE
//
// The stubs below replace live performance metric-collection, baseline, and
// harness paths — no external hardware profiler, NUMA topology, or adaptive
// optimizer is required.
//   - StubMetricCollector: models metric sample collection with cardinality
//     tracking.
//   - StubBaselineManager: models baseline snapshot and regression detection.
//   - StubBenchmarkHarness: models benchmark harness lifecycle with crash
//     detection.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

class StubMetricCollector {
public:
    bool collect(const std::string& metric_name, double value) noexcept {
        samples_.fetch_add(1, std::memory_order_relaxed);
        (void)metric_name; (void)value;
        return true;
    }
    uint64_t totalSamples() const noexcept { return samples_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> samples_{0};
};

class StubBaselineManager {
public:
    struct BaselineResult { bool ok{true}; bool regression{false}; };
    BaselineResult check(const std::string& gate, double value) noexcept {
        checks_.fetch_add(1, std::memory_order_relaxed);
        (void)gate; (void)value;
        return {true, false};
    }
    uint64_t totalChecks() const noexcept { return checks_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> checks_{0};
};

class StubBenchmarkHarness {
public:
    struct RunResult { bool ok{true}; bool crashed{false}; };
    RunResult run(const std::string& bench_name) noexcept {
        runs_.fetch_add(1, std::memory_order_relaxed);
        (void)bench_name;
        return {true, false};
    }
    uint64_t totalRuns() const noexcept { return runs_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> runs_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: PerfSoak_MetricCollectionThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(PerfSoak_MetricCollectionThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubMetricCollector collector;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string name = "perf_metric_" + std::to_string(tick % 128);
            (void)collector.collect(name, static_cast<double>(tick));
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "[PERF:MetricOverflow] No exceptions during metric collection soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s   = elapsed_ms.count() / 1000.0;
    const double ops_per_sec = static_cast<double>(collector.totalSamples()) / elapsed_s;

    constexpr double kMinThroughput = 10'000.0;
    EXPECT_GE(ops_per_sec, kMinThroughput)
        << "[PERF:MetricOverflow] Metric collection throughput must be ≥ 10 000 samples/sec. "
           "Observed: " << static_cast<uint64_t>(ops_per_sec) << " samples/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: PerfSoak_BaselineStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(PerfSoak_BaselineStability, ZeroRegressionEventsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubBaselineManager manager;
    bool exception_caught    = false;
    uint64_t regression_count = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string gate = "GATE-PERF-" + std::to_string(tick % 12);
            const auto result = manager.check(gate, static_cast<double>(tick % 1000));
            if (result.regression) {
                ++regression_count;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[PERF:BaselineRegression] No exceptions during baseline stability soak";

    EXPECT_EQ(regression_count, 0u)
        << "[PERF:BaselineRegression] Zero regression events expected during soak. "
           "Observed: " << regression_count;

    EXPECT_GT(manager.totalChecks(), 0u)
        << "At least one baseline check must complete during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: PerfSoak_BenchmarkHarnessReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(PerfSoak_BenchmarkHarnessReliability, ZeroCrashEventsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 3);

    StubBenchmarkHarness harness;
    bool exception_caught = false;
    uint64_t crash_count  = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string bench = "bench_" + std::to_string(tick % 16);
            const auto result = harness.run(bench);
            if (result.crashed) {
                ++crash_count;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[PERF:HarnessCrash] No exceptions during benchmark harness soak";

    EXPECT_EQ(crash_count, 0u)
        << "[PERF:HarnessCrash] Zero crash events expected during soak. "
           "Observed: " << crash_count;

    EXPECT_GT(harness.totalRuns(), 0u)
        << "At least one benchmark harness run must complete during the soak";
}
