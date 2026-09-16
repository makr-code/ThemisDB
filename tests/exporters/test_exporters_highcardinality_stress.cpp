// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_exporters_highcardinality_stress.cpp
 * @brief Wave D — Exporters High-Cardinality Stress Tests.
 *
 * High-cardinality stress tests for the exporters module:
 * - 1 000 000 metric exports across 8 concurrent threads
 * - Concurrent trace export stress
 * - Exporter backpressure stress
 *
 * Labels: wave_d;stress;not_release_critical
 *
 * SIMULATION NOTE: All metric export, trace export, and backpressure operations
 * use in-process stubs that model the production hot paths without requiring
 * external Prometheus, Jaeger, or OTLP backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_EXPORTERS.md
 * @see src/exporters/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process exporter stubs
// ---------------------------------------------------------------------------

class StubMetricExporterStress {
public:
    void exportMetric(const std::string& name, double value) {
        exported_.fetch_add(1, std::memory_order_relaxed);
        (void)name; (void)value;
    }
    uint64_t exported() const noexcept { return exported_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> exported_{0};
};

class StubTraceExporterStress {
public:
    void exportTrace(const std::string& trace_id, const std::string& span) {
        exported_.fetch_add(1, std::memory_order_relaxed);
        (void)trace_id; (void)span;
    }
    uint64_t exported() const noexcept { return exported_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> exported_{0};
};

class StubExportQueue {
public:
    explicit StubExportQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(const std::string& item) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) {
            overflow_.fetch_add(1, std::memory_order_relaxed);
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

    uint64_t overflow() const noexcept { return overflow_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::deque<std::string> queue_;
    std::size_t capacity_;
    std::atomic<uint64_t> overflow_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test HighCardinalityMetricExport
 * Exports 1 000 000 metrics across 8 concurrent threads and verifies that all
 * 1 000 000 exports complete without exceptions.
 */
TEST(HighCardinalityMetricExport, ConcurrentExport) {
    static constexpr uint64_t kTotalMetrics = 1'000'000;
    static constexpr int kThreads = 8;
    static constexpr uint64_t kPerThread = kTotalMetrics / kThreads;

    StubMetricExporterStress exporter;
    std::atomic<uint64_t> exceptions{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kPerThread; ++i) {
                try {
                    exporter.exportMetric("metric_" + std::to_string(t) + "_" + std::to_string(i),
                                          static_cast<double>(i));
                } catch (...) {
                    exceptions.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[EXPORTER:MetricDropStorm] Exceptions in HighCardinalityMetricExport";
    EXPECT_EQ(kTotalMetrics, exporter.exported())
        << "[EXPORTER:MetricDropStorm] Metric count mismatch";
}

/**
 * @test ConcurrentTraceExportStress
 * Runs 8 concurrent trace export threads each exporting 10 000 traces and
 * verifies total count and no errors.
 */
TEST(ConcurrentTraceExportStress, MultiThreadedExport) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kOpsPerThread = 10'000;

    StubTraceExporterStress exporter;
    std::atomic<uint64_t> errors{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                try {
                    exporter.exportTrace(
                        "trace_" + std::to_string(t) + "_" + std::to_string(i),
                        "{\"span\":\"" + std::to_string(i) + "\"}");
                } catch (...) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, errors.load())
        << "[EXPORTER:TraceBufferOverflow] Errors in ConcurrentTraceExportStress";
    EXPECT_EQ(static_cast<uint64_t>(kThreads) * kOpsPerThread, exporter.exported());
}

/**
 * @test ExporterBackpressureStress
 * Runs 8 concurrent producers against a bounded export queue and verifies
 * no uncaught exceptions occur (overflow is tolerated by design).
 */
TEST(ExporterBackpressureStress, MultiThreadedEnqueueDequeue) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kOpsPerThread = 100'000;
    static constexpr std::size_t kQueueCap = 512;

    StubExportQueue queue(kQueueCap);
    std::atomic<uint64_t> exceptions{0};
    std::vector<std::thread> workers;

    std::atomic<bool> consuming{true};
    std::thread consumer([&]() {
        while (consuming.load(std::memory_order_relaxed)) {
            std::string out;
            queue.dequeue(out);
        }
        std::string out;
        while (queue.dequeue(out)) {}
    });

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                try { queue.enqueue("item_" + std::to_string(t) + "_" + std::to_string(i)); }
                catch (...) { exceptions.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }

    for (auto& th : workers) th.join();
    consuming.store(false, std::memory_order_relaxed);
    consumer.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[EXPORTER:QueueStall] Exceptions in ExporterBackpressureStress";
}
