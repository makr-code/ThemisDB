// Test Performance Helpers
// Utilities for measuring latency, throughput, and memory usage in tests
// Part of comprehensive testing infrastructure

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace themis {
namespace test {

// Centralized benchmark policy aligned with PERFORMANCE_EXPECTATIONS.md
// (independent runs and warmup can be overridden in CI via env vars).
class BenchmarkPolicy {
public:
    static int independentRuns() {
        return readPositiveIntEnv("THEMIS_BENCH_RUNS", 5);
    }

    static int warmupIterations() {
        return readPositiveIntEnv("THEMIS_BENCH_WARMUP_ITERS", 100);
    }

private:
    static int readPositiveIntEnv(const char* name, int fallback) {
        const char* raw = std::getenv(name);
        if (raw == nullptr || *raw == '\0') {
            return fallback;
        }
        char* end = nullptr;
        const long parsed = std::strtol(raw, &end, 10);
        if (end == raw || *end != '\0' || parsed <= 0 || parsed > 1000000L) {
            return fallback;
        }
        return static_cast<int>(parsed);
    }
};

// High-resolution timer for measuring latency
class LatencyMeasurement {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double, std::milli>;

    LatencyMeasurement() : start_(Clock::now()) {}

    // Get elapsed time in milliseconds
    double elapsedMs() const {
        auto end = Clock::now();
        return std::chrono::duration_cast<Duration>(end - start_).count();
    }

    // Reset the timer
    void reset() {
        start_ = Clock::now();
    }

private:
    TimePoint start_;
};

template<typename T>
T percentileValue(std::vector<T> values, int percentile) {
    if (values.empty()) {
        return T{};
    }
    std::sort(values.begin(), values.end());
    const std::size_t idx = static_cast<std::size_t>(
        std::ceil((static_cast<double>(percentile) / 100.0) * static_cast<double>(values.size()))) - 1U;
    return values[std::min(idx, values.size() - 1U)];
}

template<typename Fn>
std::vector<double> sampleLatencyMs(Fn&& fn,
                                    int runs = BenchmarkPolicy::independentRuns(),
                                    int warmup_iters = BenchmarkPolicy::warmupIterations()) {
    for (int i = 0; i < warmup_iters; ++i) {
        fn();
    }

    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(runs));
    for (int i = 0; i < runs; ++i) {
        LatencyMeasurement timer;
        fn();
        samples.push_back(timer.elapsedMs());
    }
    return samples;
}

// Throughput calculator
class ThroughputCalculator {
public:
    ThroughputCalculator() : start_(std::chrono::high_resolution_clock::now()), count_(0) {}

    void increment(size_t count = 1) {
        count_ += count;
    }

    // Get throughput in operations per second
    double getOpsPerSecond() const {
        auto duration = std::chrono::high_resolution_clock::now() - start_;
        double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        if (seconds == 0.0) {
          return 0.0;
        }
        return static_cast<double>(count_) / seconds;
    }

    // Get throughput in tokens per second (for LLM tests)
    double getTokensPerSecond() const {
        return getOpsPerSecond();
    }

    size_t getCount() const { return count_; }

private:
    std::chrono::high_resolution_clock::time_point start_;
    size_t count_;
};

// Memory usage tracker
class MemoryUsageTracker {
public:
    MemoryUsageTracker() {
        baseline_ = getCurrentMemoryUsageMB();
    }

    // Get current memory usage in MB
    static double getCurrentMemoryUsageMB() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
        }
        return 0.0;
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
#ifdef __APPLE__
            // macOS reports in bytes
            return static_cast<double>(usage.ru_maxrss) / (1024.0 * 1024.0);
#else
            // Linux reports in kilobytes
            return static_cast<double>(usage.ru_maxrss) / 1024.0;
#endif
        }
        return 0.0;
#endif
    }

    // Get memory delta since construction in MB
    double getDeltaMB() const {
        return getCurrentMemoryUsageMB() - baseline_;
    }

    void reset() {
        baseline_ = getCurrentMemoryUsageMB();
    }

private:
    double baseline_;
};

// Performance assertion macros
// Note: These macros evaluate the expression directly, so avoid passing
// expressions with side effects. For complex operations, assign to a
// variable first then pass the variable to the macro.

// Assert that operation completes within specified milliseconds
#define ASSERT_LATENCY_MS(expression, max_ms) \
    do { \
        themis::test::LatencyMeasurement timer; \
        expression; \
        double elapsed = timer.elapsedMs(); \
        ASSERT_LE(elapsed, max_ms) << "Operation took " << elapsed << "ms, expected < " << max_ms << "ms"; \
    } while(0)

// Assert that operation completes within specified milliseconds (non-fatal)
#define EXPECT_LATENCY_MS(expression, max_ms) \
    do { \
        themis::test::LatencyMeasurement timer; \
        expression; \
        double elapsed = timer.elapsedMs(); \
        EXPECT_LE(elapsed, max_ms) << "Operation took " << elapsed << "ms, expected < " << max_ms << "ms"; \
    } while(0)

// Assert throughput is above minimum threshold
#define ASSERT_THROUGHPUT(calculator, min_ops_per_sec) \
    do { \
        double ops_per_sec = calculator.getOpsPerSecond(); \
        ASSERT_GE(ops_per_sec, min_ops_per_sec) \
            << "Throughput " << ops_per_sec << " ops/sec, expected >= " << min_ops_per_sec << " ops/sec"; \
    } while(0)

// Expect throughput is above minimum threshold (non-fatal)
#define EXPECT_THROUGHPUT(calculator, min_ops_per_sec) \
    do { \
        double ops_per_sec = calculator.getOpsPerSecond(); \
        EXPECT_GE(ops_per_sec, min_ops_per_sec) \
            << "Throughput " << ops_per_sec << " ops/sec, expected >= " << min_ops_per_sec << " ops/sec"; \
    } while(0)

// Assert memory usage increase is below threshold
#define ASSERT_MEMORY_DELTA_MB(tracker, max_delta_mb) \
    do { \
        double delta = tracker.getDeltaMB(); \
        ASSERT_LE(delta, max_delta_mb) \
            << "Memory increased by " << delta << "MB, expected < " << max_delta_mb << "MB"; \
    } while(0)

// Expect memory usage increase is below threshold (non-fatal)
#define EXPECT_MEMORY_DELTA_MB(tracker, max_delta_mb) \
    do { \
        double delta = tracker.getDeltaMB(); \
        EXPECT_LE(delta, max_delta_mb) \
            << "Memory increased by " << delta << "MB, expected < " << max_delta_mb << "MB"; \
    } while(0)

// Helper to measure and print performance statistics
class PerformanceStats {
public:
    PerformanceStats(const std::string& test_name) : test_name_(test_name) {
        timer_.reset();
        memory_.reset();
    }

    ~PerformanceStats() {
        double elapsed = timer_.elapsedMs();
        double memory_delta = memory_.getDeltaMB();
        std::cout << "[" << test_name_ << "] "
                  << "Time: " << elapsed << "ms, "
                  << "Memory Delta: " << memory_delta << "MB" << std::endl;
    }

private:
    std::string test_name_;
    LatencyMeasurement timer_;
    MemoryUsageTracker memory_;
};

// RAII helper for scoped performance measurement
#define SCOPED_PERF_STATS(name) \
    themis::test::PerformanceStats perf_stats_##__LINE__(name)

} // namespace test
} // namespace themis
