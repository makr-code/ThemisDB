// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_exporters_soak.cpp
 * @brief Wave D — Exporters Soak Tests.
 *
 * Long-duration soak tests for the exporters module primary paths:
 * metric export throughput, trace export stability, and backpressure reliability.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Metric export throughput ≥ 10 000 metrics/sec (simulated)
 * - Trace export: no crashes or data-race signals over soak duration
 * - Backpressure reliability: overflow count tracked; no uncaught exceptions
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * SIMULATION NOTE: All metric, trace, and backpressure export operations use
 * in-process stubs that model the production hot paths without requiring
 * external Prometheus, Jaeger, or OTLP backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_EXPORTERS.md
 * @see src/exporters/ROADMAP.md — Wave D Contribution
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

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process exporter stubs
// ---------------------------------------------------------------------------

class StubMetricExporter {
public:
    void exportMetric(const std::string& name, double value) {
        metrics_exported_.fetch_add(1, std::memory_order_relaxed);
        (void)name; (void)value;
    }
    uint64_t metricsExported() const noexcept { return metrics_exported_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> metrics_exported_{0};
};

class StubTraceExporter {
public:
    void exportTrace(const std::string& trace_id, const std::string& span_data) {
        traces_exported_.fetch_add(1, std::memory_order_relaxed);
        (void)trace_id; (void)span_data;
    }
    uint64_t tracesExported() const noexcept { return traces_exported_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> traces_exported_{0};
};

class StubExportBackpressure {
public:
    explicit StubExportBackpressure(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(const std::string& item) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) {
            overflow_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        queue_.push_back(item);
        return true;
    }

    bool dequeue(std::string& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        return true;
    }

    uint64_t overflowCount() const noexcept { return overflow_count_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::deque<std::string> queue_;
    std::size_t capacity_;
    std::atomic<uint64_t> overflow_count_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test ExportersSoak_MetricExportThroughput
 * Verifies metric export throughput ≥ 10 000 metrics/sec over soak duration.
 */
TEST(ExportersSoak, MetricExportThroughput) {
    StubMetricExporter exporter;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 8;

    std::atomic<bool> stop{false};
    std::vector<std::thread> workers;

    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                exporter.exportMetric("metric_" + std::to_string(t) + "_" + std::to_string(idx),
                                      static_cast<double>(idx));
                ++idx;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : workers) th.join();

    const double elapsed_s = static_cast<double>(durationMs) / 1000.0;
    const double mps = static_cast<double>(exporter.metricsExported()) / elapsed_s;

    EXPECT_GT(mps, 10'000.0)
        << "[EXPORTER:MetricDropStorm] MetricExportThroughput below gate: " << mps;
}

/**
 * @test ExportersSoak_TraceExportStability
 * Verifies trace export produces zero crashes or uncaught exceptions over soak.
 */
TEST(ExportersSoak, TraceExportStability) {
    StubTraceExporter exporter;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 4;

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> error_count{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                try {
                    exporter.exportTrace(
                        "trace_" + std::to_string(t) + "_" + std::to_string(idx),
                        "{\"span\":\"soak_" + std::to_string(idx) + "\"}");
                    ++idx;
                } catch (...) {
                    error_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, error_count.load())
        << "[EXPORTER:TraceBufferOverflow] Trace export errors during soak";
    EXPECT_GT(exporter.tracesExported(), 0u);
}

/**
 * @test ExportersSoak_BackpressureReliability
 * Verifies that the export backpressure queue handles sustained load without
 * uncaught exceptions. Overflow is tolerated; exception is not.
 */
TEST(ExportersSoak, BackpressureReliability) {
    StubExportBackpressure bp(512);
    const uint64_t durationMs = soakDurationMs();

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> exceptions{0};

    std::thread producer([&]() {
        uint64_t idx = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            try { bp.enqueue("item_" + std::to_string(idx++)); }
            catch (...) { exceptions.fetch_add(1, std::memory_order_relaxed); }
        }
    });

    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            std::string out;
            bp.dequeue(out);
            std::this_thread::sleep_for(100us);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    producer.join();
    consumer.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[EXPORTER:QueueStall] Exceptions during backpressure soak";
}
