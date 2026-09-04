// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file wave4_fixtures.h
 * @brief Wave 4 benchmark fixtures: deterministic setup, warmup protocol,
 *        variance control helpers, and resilience simulation primitives.
 *
 * Design goals (Wave 4 / B4-C):
 *  - Deterministic seed propagation across all wave4 benchmarks.
 *  - Explicit warmup-then-measure separation via WarmupProtocol.
 *  - Latency-injection and backpressure helpers for resilience scenarios (B4-B).
 *  - Structured per-iteration variance capture via VarianceTracker.
 *
 * Conventions:
 *  - kW4CanonicalSeed = 42  (matches kCanonicalRngSeed in bench_fixtures.h)
 *  - All I/O uses OS temp dir created through TempDir (from bench_fixtures.h).
 *  - UseRealTime() is set on I/O-bound registrations.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "bench_fixtures.h"  // RandomGenerator, TempDir, StorageBenchFixture

namespace themis {
namespace bench {
namespace wave4 {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical RNG seed for all Wave 4 benchmarks (mirrors kCanonicalRngSeed=42).
inline constexpr uint64_t kW4CanonicalSeed = 42;

/// Default warmup iterations before timed measurements begin.
inline constexpr int kDefaultWarmupIterations = 5;

/// Minimum sample count for variance analysis (mirrors framework n>=30 rule).
inline constexpr std::size_t kMinVarianceSamples = 30;

// ---------------------------------------------------------------------------
// WarmupProtocol — explicit warmup before timed measurement
// ---------------------------------------------------------------------------

/**
 * @brief Helper that executes @p warmup_fn for @p iterations before the
 *        timed benchmark loop starts.
 *
 * Usage inside a benchmark function:
 * @code
 *   WarmupProtocol::run(kDefaultWarmupIterations, [&]{ doWork(); });
 *   for (auto _ : state) { doWork(); }
 * @endcode
 *
 * @param iterations  Number of warm-up invocations (default: kDefaultWarmupIterations).
 * @param fn          Work unit to warm up (should mirror the timed path).
 */
struct WarmupProtocol {
    static void run(int iterations, std::function<void()> fn) {
        for (int i = 0; i < iterations; ++i) {
            fn();
        }
    }
};

// ---------------------------------------------------------------------------
// VarianceTracker — per-run latency sample collector
// ---------------------------------------------------------------------------

/**
 * @brief Accumulates per-iteration latency samples and reports p50/p95/p99
 *        and coefficient-of-variation (CV) as Google Benchmark counters.
 *
 * Usage:
 * @code
 *   VarianceTracker vt;
 *   for (auto _ : state) {
 *       auto t0 = std::chrono::steady_clock::now();
 *       doWork();
 *       vt.record(std::chrono::steady_clock::now() - t0);
 *   }
 *   vt.publishCounters(state);
 * @endcode
 */
class VarianceTracker {
public:
    /// @brief Record one latency sample.
    void record(std::chrono::nanoseconds latency) {
        samples_.push_back(static_cast<double>(latency.count()));
    }

    /**
     * @brief Publish p50/p95/p99/CV as benchmark counters.
     * @param state  The active Google Benchmark state object.
     */
    void publishCounters(::benchmark::State& state) {
        if (samples_.size() < 2) {
          return;
        }

        std::vector<double> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        const std::size_t n = sorted.size();
        auto percentile = [&](double p) -> double {
            const double idx = p * static_cast<double>(n - 1);
            const std::size_t lo = static_cast<std::size_t>(idx);
            const std::size_t hi = std::min(lo + 1, n - 1);
            return sorted[lo] + (idx - static_cast<double>(lo)) * (sorted[hi] - sorted[lo]);
        };

        const double mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) /
                            static_cast<double>(n);
        double sq_sum = 0.0;
        for (double v : sorted) {
          sq_sum += (v - mean) * (v - mean);
        }
        const double stddev = std::sqrt(sq_sum / static_cast<double>(n));
        const double cv = (mean > 0.0) ? (stddev / mean) : 0.0;

        state.counters["p50_ns"]  = percentile(0.50);
        state.counters["p95_ns"]  = percentile(0.95);
        state.counters["p99_ns"]  = percentile(0.99);
        state.counters["cv"]      = cv;
        state.counters["samples"] = static_cast<double>(n);
    }

    /// @brief Reset collected samples (use between sub-experiments within one fixture).
    void reset() { samples_.clear(); }

    /// @brief Number of recorded samples.
    std::size_t size() const noexcept { return samples_.size(); }

private:
    std::vector<double> samples_;
};

// ---------------------------------------------------------------------------
// LatencyInjector — deterministic artificial latency for resilience testing
// ---------------------------------------------------------------------------

/**
 * @brief Injects a configurable artificial latency to simulate degraded
 *        operating conditions (network jitter, disk contention, etc.).
 *
 * The delay is drawn from a seeded uniform distribution over
 * [min_us, max_us] so results are reproducible across runs.
 *
 * Usage:
 * @code
 *   LatencyInjector injector(/*min_us=*/50, /*max_us=*/200, kW4CanonicalSeed);
 *   for (auto _ : state) {
 *       injector.sleep();  // deterministic artificial delay
 *       doWork();
 *   }
 * @endcode
 *
 * @param min_us  Minimum sleep duration in microseconds.
 * @param max_us  Maximum sleep duration in microseconds.
 * @param seed    RNG seed (default: kW4CanonicalSeed).
 */
class LatencyInjector {
public:
    explicit LatencyInjector(int64_t min_us, int64_t max_us,
                             uint64_t seed = kW4CanonicalSeed)
        : rng_(static_cast<std::mt19937_64::result_type>(seed))
        , dist_(min_us, max_us)
    {}

    /// @brief Sleep for a deterministically sampled duration.
    void sleep() {
        std::this_thread::sleep_for(std::chrono::microseconds(dist_(rng_)));
    }

    /// @brief Sample the next delay without sleeping (for manual control).
    std::chrono::microseconds next() {
        return std::chrono::microseconds(dist_(rng_));
    }

private:
    std::mt19937_64 rng_;
    std::uniform_int_distribution<int64_t> dist_;
};

// ---------------------------------------------------------------------------
// BackpressureSimulator — token-bucket backpressure for throughput capping
// ---------------------------------------------------------------------------

/**
 * @brief Simple token-bucket backpressure simulator for throughput-limiting
 *        scenarios (B4-B).
 *
 * Limits throughput to @p ops_per_second by blocking when the bucket is
 * empty.  Thread-safe: acquire() is protected by an internal mutex.
 *
 * @param ops_per_second  Maximum allowed operations per second.
 */
class BackpressureSimulator {
public:
    explicit BackpressureSimulator(double ops_per_second)
        : interval_ns_(static_cast<int64_t>(1.0e9 / ops_per_second))
        , last_tick_(std::chrono::steady_clock::now())
    {}

    /**
     * @brief Block until the next token is available.
     *
     * Thread-safe: may be called from multiple threads concurrently.
     * Must be called before each operation that should be rate-limited.
     */
    void acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_tick_);
        if (elapsed.count() < interval_ns_) {
            std::this_thread::sleep_for(
                std::chrono::nanoseconds(interval_ns_ - elapsed.count()));
        }
        last_tick_ = std::chrono::steady_clock::now();
    }

private:
    int64_t interval_ns_;
    std::chrono::steady_clock::time_point last_tick_;
    std::mutex mutex_;
};

// ---------------------------------------------------------------------------
// DegradedStorageFixture — StorageBenchFixture + configurable I/O degradation
// ---------------------------------------------------------------------------

/**
 * @brief Extends StorageBenchFixture with deterministic latency injection and
 *        backpressure simulation for resilience benchmarks (B4-B).
 *
 * The degradation profile is controlled via state.range(0):
 *  - 0  → no degradation (baseline)
 *  - 1  → mild latency injection  (50–200 µs)
 *  - 2  → severe latency injection (500–2000 µs)
 *  - 3  → backpressure at 500 ops/s
 */
class DegradedStorageFixture : public StorageBenchFixture {
public:
    void SetUp(::benchmark::State& state) override {
        StorageBenchFixture::SetUp(state);

        const int mode = static_cast<int>(state.range(0));
        switch (mode) {
            case 1:
                injector_ = std::make_unique<LatencyInjector>(50, 200, kW4CanonicalSeed);
                break;
            case 2:
                injector_ = std::make_unique<LatencyInjector>(500, 2000, kW4CanonicalSeed);
                break;
            case 3:
                backpressure_ = std::make_unique<BackpressureSimulator>(500.0);
                break;
            default:
                break;  // mode 0: no degradation
        }
    }

    void TearDown(::benchmark::State& state) override {
        injector_.reset();
        backpressure_.reset();
        StorageBenchFixture::TearDown(state);
    }

    /**
     * @brief Apply the configured degradation before an operation.
     *
     * If latency injection is active, sleeps for a deterministically sampled
     * duration.  If backpressure is active, blocks until a token is available.
     */
    void applyDegradation() {
        if (injector_) {
          injector_->sleep();
        }
        if (backpressure_) {
          backpressure_->acquire();
        }
    }

protected:
    std::unique_ptr<LatencyInjector>       injector_;
    std::unique_ptr<BackpressureSimulator> backpressure_;
};

// ---------------------------------------------------------------------------
// DeterministicFixture — base for variance-controlled benchmarks (B4-C)
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for variance-controlled benchmarks.
 *
 * Provides:
 *  - Seeded RNG reset before each iteration group.
 *  - Embedded VarianceTracker for p50/p95/p99 publishing.
 *  - WarmupProtocol helper.
 *
 * Subclasses call resetSeed() in SetUp() and publishVariance() at the end
 * of the benchmark loop.
 */
class DeterministicFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        resetSeed();
        tracker_.reset();
    }

    void TearDown(::benchmark::State& state) override {
        tracker_.publishCounters(state);
    }

    /// @brief Re-seed the RNG to kW4CanonicalSeed (call in SetUp or per-iteration).
    void resetSeed() {
        rng_.engine().seed(kW4CanonicalSeed);
    }

protected:
    RandomGenerator rng_{kW4CanonicalSeed};
    VarianceTracker tracker_;
};

}  // namespace wave4
}  // namespace bench
}  // namespace themis
