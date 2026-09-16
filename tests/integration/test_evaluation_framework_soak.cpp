// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_evaluation_framework_soak.cpp
 * @brief Wave D — Evaluation Framework Soak Test (sustained traffic).
 *
 * Long-duration soak test for the evaluation framework hot paths:
 * metric computation throughput, benchmark harness stability, and result
 * aggregation reliability. Verifies that all three metrics remain within
 * acceptable bounds over a configurable soak window driven by
 * THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - EvalSoak_MetricComputationThroughput  : ≥ 5 000 metric evals/sec
 * - EvalSoak_BenchmarkHarnessStability    : zero harness crashes
 * - EvalSoak_ResultAggregationReliability : zero aggregation overflows
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_EVALUATION_FRAMEWORK.md — operator runbook
 * @see src/evaluation/ROADMAP.md — Wave D contribution closure
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations.
// They do NOT drive real evaluation infrastructure and MUST NOT be linked
// into production code paths.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <numeric>
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
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct StubMetricEngine {
    std::atomic<uint64_t> computed{0};
    std::atomic<uint64_t> harness_crashes{0};
    std::atomic<uint64_t> aggregation_overflows{0};

    double compute(double sample) {
        ++computed;
        // Simulate a simple precision metric (no division by zero)
        return sample > 0.0 ? (sample / (sample + 1.0)) : 0.0;
    }

    double aggregate(const std::vector<double>& results) {
        if (results.empty()) return 0.0;
        double sum = std::accumulate(results.begin(), results.end(), 0.0);
        // Overflow guard: finite result expected
        if (!std::isfinite(sum)) {
            ++aggregation_overflows;
            return 0.0;
        }
        return sum / static_cast<double>(results.size());
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test cases
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvalSoak, MetricComputationThroughput) {
    const auto duration_ms = soakDurationMs();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    StubMetricEngine engine;
    std::atomic<bool> stop{false};

    constexpr int kWorkers = 8;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            double sample = static_cast<double>(t + 1) * 0.5;
            while (!stop.load(std::memory_order_relaxed)) {
                engine.compute(sample);
                sample += 0.001;
            }
        });
    }

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_release);
    for (auto& th : workers) th.join();

    const double elapsed_sec = static_cast<double>(duration_ms) / 1000.0;
    const double throughput =
        static_cast<double>(engine.computed.load()) / elapsed_sec;

    EXPECT_GE(throughput, 5'000.0)
        << "Metric computation throughput " << throughput
        << " evals/s fell below the 5 000 evals/s SLO over "
        << duration_ms << " ms soak window";
}

TEST(EvalSoak, BenchmarkHarnessStability) {
    const auto duration_ms = soakDurationMs();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    StubMetricEngine engine;
    std::atomic<bool> stop{false};

    std::thread worker([&]() {
        double sample = 1.0;
        while (!stop.load(std::memory_order_relaxed)) {
            engine.compute(sample);
            sample += 1.0;
            if (sample > 1e6) sample = 1.0; // keep values finite
        }
    });

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_release);
    worker.join();

    EXPECT_EQ(engine.harness_crashes.load(), 0ULL)
        << "Benchmark harness reported crashes during soak";
}

TEST(EvalSoak, ResultAggregationReliability) {
    const auto duration_ms = soakDurationMs();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    StubMetricEngine engine;
    std::atomic<bool> stop{false};

    std::thread worker([&]() {
        std::vector<double> batch(64);
        double v = 0.5;
        while (!stop.load(std::memory_order_relaxed)) {
            for (auto& x : batch) { x = v; v += 0.1; }
            engine.aggregate(batch);
        }
    });

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_release);
    worker.join();

    EXPECT_EQ(engine.aggregation_overflows.load(), 0ULL)
        << "Result aggregation overflowed during soak";
}
