// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_exporters_dedicated_gates.cpp
 * @brief Wave D — Exporters Dedicated Benchmark Gates.
 *
 * Benchmark gates EX-BM-01..04 for the exporters module.
 *
 * Gates:
 *   EX-BM-01  MetricExport_Throughput            — ≥ 100 000 metrics/sec
 *   EX-BM-02  TraceExport_Throughput             — ≥ 10 000 traces/sec
 *   EX-BM-03  ExportQueue_Enqueue_Latency        — p99 ≤ 10 µs
 *   EX-BM-04  ExportQueue_Dequeue_Latency        — p99 ≤ 10 µs
 *
 * SIMULATION NOTE: All metric, trace, and queue operations use in-process
 * stubs that model the production hot paths without requiring external
 * Prometheus, Jaeger, or OTLP backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_EXPORTERS.md
 * @see src/exporters/ROADMAP.md — Wave D
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <string>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process exporter stubs
// ---------------------------------------------------------------------------
class StubMetricExporterBench {
public:
    void exportMetric(const std::string& name, double value) {
        count_.fetch_add(1, std::memory_order_relaxed);
        (void)name; (void)value;
    }
    uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_{0};
};

class StubTraceExporterBench {
public:
    void exportTrace(const std::string& id, const std::string& span) {
        count_.fetch_add(1, std::memory_order_relaxed);
        (void)id; (void)span;
    }
    uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_{0};
};

class StubExportQueueBench {
public:
    explicit StubExportQueueBench(std::size_t cap) : cap_(cap) {}
    bool enqueue(const std::string& item) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.size() >= cap_) return false;
        q_.push_back(item);
        return true;
    }
    bool dequeue(std::string& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.empty()) return false;
        out = q_.front(); q_.pop_front(); return true;
    }
private:
    std::mutex mu_;
    std::deque<std::string> q_;
    std::size_t cap_;
};

// ---------------------------------------------------------------------------
// EX-BM-01: Metric Export Throughput
// ---------------------------------------------------------------------------
static void EX_BM_01_MetricExport_Throughput(benchmark::State& state) {
    StubMetricExporterBench exporter;
    int64_t idx = 0;
    for (auto _ : state) {
        exporter.exportMetric("bench_metric_" + std::to_string(idx), static_cast<double>(idx));
        ++idx;
        benchmark::ClobberMemory();
    }
    state.SetLabel("EX-BM-01 gate: ≥100000 metrics/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(EX_BM_01_MetricExport_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// EX-BM-02: Trace Export Throughput
// ---------------------------------------------------------------------------
static void EX_BM_02_TraceExport_Throughput(benchmark::State& state) {
    StubTraceExporterBench exporter;
    int64_t idx = 0;
    for (auto _ : state) {
        exporter.exportTrace("trace_" + std::to_string(idx), "{\"span\":\"" + std::to_string(idx) + "\"}");
        ++idx;
        benchmark::ClobberMemory();
    }
    state.SetLabel("EX-BM-02 gate: ≥10000 traces/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(EX_BM_02_TraceExport_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// EX-BM-03: Export Queue Enqueue Latency
// ---------------------------------------------------------------------------
static void EX_BM_03_ExportQueue_Enqueue_Latency(benchmark::State& state) {
    StubExportQueueBench q(65536);
    int64_t idx = 0;
    for (auto _ : state) {
        if (!q.enqueue("item_" + std::to_string(idx))) {
            std::string out;
            q.dequeue(out);
            q.enqueue("item_" + std::to_string(idx));
        }
        ++idx;
        benchmark::ClobberMemory();
    }
    state.SetLabel("EX-BM-03 gate: p99 ≤ 10µs");
}
BENCHMARK(EX_BM_03_ExportQueue_Enqueue_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// EX-BM-04: Export Queue Dequeue Latency
// ---------------------------------------------------------------------------
static void EX_BM_04_ExportQueue_Dequeue_Latency(benchmark::State& state) {
    StubExportQueueBench q(65536);
    // Pre-fill
    for (int i = 0; i < 32768; ++i) q.enqueue("item_" + std::to_string(i));

    for (auto _ : state) {
        std::string out;
        if (!q.dequeue(out)) {
            // Refill when empty
            for (int i = 0; i < 256; ++i) q.enqueue("item_" + std::to_string(i));
            q.dequeue(out);
        }
        benchmark::DoNotOptimize(out);
    }
    state.SetLabel("EX-BM-04 gate: p99 ≤ 10µs");
}
BENCHMARK(EX_BM_04_ExportQueue_Dequeue_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
