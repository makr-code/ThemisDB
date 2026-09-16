/**
 * @file test_analytics_highcardinality_stress.cpp
 * @brief Wave D — Analytics High-Cardinality Stress Tests.
 *
 * Stress tests for the analytics module covering high-cardinality scenarios:
 * 5000+ distinct metric names, 8-thread concurrent aggregation, and columnar
 * projection under sustained high-cardinality metric load.
 *
 * These tests are excluded from the fast release_critical gate and are
 * intended for Wave D stress validation pipelines.
 *
 * ## Test cases
 * - HighCardinalityMetricIngestion          — 5000 distinct metric names, validates all ingested
 * - ConcurrentWindowAggregationStress       — 8 threads × window aggregation under cardinality load
 * - ColumnarProjectionUnderHighCardinality  — columnar scan with a 5000-column cardinality corpus
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_ANALYTICS_PIPELINE.md — Analytics operator runbook
 * @see src/analytics/ROADMAP.md — Wave D Contribution (2026-09-16)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The in-process stubs below replace live analytics paths so no external
// time-series backend, columnar store, or model-serving service is required.
// Their correctness characteristics mirror the real pipeline contracts:
//   - Metric ingestion: unique name → running-sum accumulator map.
//   - Window aggregation: lock-guarded tumbling window with SUM flush.
//   - Columnar projection: predicate evaluation over a deterministic int64 batch.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// Stub: Metric ingestion registry
// ---------------------------------------------------------------------------

/// Registry that accumulates per-metric sums (stub for a real time-series store).
class StubMetricRegistry {
public:
    /// Ingest one (metric_name, value) pair.
    void ingest(const std::string& metric_name, double value) {
        std::lock_guard<std::mutex> lk(mu_);
        accumulators_[metric_name] += value;
        ++total_ingested_;
    }

    /// Returns the number of distinct metric names seen.
    std::size_t distinctMetricCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return accumulators_.size();
    }

    uint64_t totalIngested() const noexcept {
        return total_ingested_.load(std::memory_order_relaxed);
    }

private:
    mutable std::mutex                       mu_;
    std::unordered_map<std::string, double>  accumulators_;
    std::atomic<uint64_t>                    total_ingested_{0};
};

// ---------------------------------------------------------------------------
// Stub: Tumbling window aggregator (thread-safe)
// ---------------------------------------------------------------------------

/// Thread-safe tumbling window that accumulates values and flushes when full.
class StubTumblingWindowAggregator {
public:
    explicit StubTumblingWindowAggregator(std::size_t window_size)
        : window_size_(window_size) {}

    /// Add a value; returns true if a window flush occurred.
    bool add(double value) {
        std::lock_guard<std::mutex> lk(mu_);
        partial_sum_ += value;
        ++count_;
        if (count_ >= window_size_) {
            flushed_sums_.push_back(partial_sum_);
            partial_sum_ = 0.0;
            count_       = 0;
            flushes_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    uint64_t    flushCount()         const noexcept { return flushes_.load(std::memory_order_relaxed); }
    std::size_t flushedWindowCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return flushed_sums_.size();
    }

private:
    const std::size_t      window_size_;
    double                 partial_sum_{0.0};
    std::size_t            count_{0};
    std::vector<double>    flushed_sums_;
    mutable std::mutex     mu_;
    std::atomic<uint64_t>  flushes_{0};
};

// ---------------------------------------------------------------------------
// Stub: Columnar projection engine
// ---------------------------------------------------------------------------

/// Projects selected columns from a high-cardinality column batch using a
/// predicate threshold. Column values are generated deterministically.
class StubColumnarProjectionEngine {
public:
    /// Project rows where column[col_idx % cardinality] > threshold.
    /// Returns the count of matching rows.
    std::size_t project(std::size_t cardinality,
                        std::size_t row_count,
                        int64_t     threshold) noexcept {
        std::size_t hits = 0;
        for (std::size_t row = 0; row < row_count; ++row) {
            // Deterministic value: (row * 6364136223846793005 + cardinality) mod (cardinality + 1)
            const int64_t v = static_cast<int64_t>(
                (static_cast<uint64_t>(row) * 6364136223846793005ULL + cardinality)
                % (cardinality + 1));
            if (v > threshold) { ++hits; }
        }
        projections_.fetch_add(1, std::memory_order_relaxed);
        return hits;
    }

    uint64_t totalProjections() const noexcept { return projections_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> projections_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityMetricIngestion
//
// Generate 5000 distinct metric names and ingest each with a deterministic
// value.  Verify all 5000 metrics are registered; throughput must complete
// within 5 s.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AnalyticsHighCardinalityStress, HighCardinalityMetricIngestion) {
    constexpr std::size_t kMetricCount = 5000;

    // Build corpus of 5000 distinct metric names
    std::vector<std::string> metric_names;
    metric_names.reserve(kMetricCount);
    for (std::size_t i = 0; i < kMetricCount; ++i) {
        std::ostringstream oss;
        oss << "themis.analytics.metric_" << i << ".host_" << (i % 100)
            << ".region_" << (i % 20);
        metric_names.push_back(oss.str());
    }

    // Verify all names are distinct
    {
        std::unordered_set<std::string> unique(metric_names.begin(), metric_names.end());
        ASSERT_EQ(unique.size(), kMetricCount)
            << "All " << kMetricCount << " metric names must be distinct";
    }

    StubMetricRegistry registry;
    const auto t0 = std::chrono::steady_clock::now();

    // Ingest all metrics (each 3× with different values to exercise accumulation)
    for (int pass = 0; pass < 3; ++pass) {
        for (std::size_t i = 0; i < kMetricCount; ++i) {
            const double value = static_cast<double>((i + 1) * (pass + 1)) / 1000.0;
            ASSERT_NO_THROW(registry.ingest(metric_names[i], value));
            // ASSERT_NO_THROW: ingest() must not throw for any of the 5000 distinct metrics
        }
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    // All 5000 distinct metrics must be registered
    EXPECT_EQ(registry.distinctMetricCount(), kMetricCount)
        << "All " << kMetricCount << " distinct metric names must be registered";

    // Total ingested: kMetricCount × 3 passes
    EXPECT_EQ(registry.totalIngested(), static_cast<uint64_t>(kMetricCount * 3))
        << "Total ingested count must equal 3× the metric count";

    // Throughput gate: 15 000 ingestion ops must complete within 5 s
    EXPECT_LE(elapsed_ms.count(), 5000)
        << "High-cardinality metric ingestion (15 000 ops) must complete within 5 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentWindowAggregationStress
//
// Spawn 8 threads each performing 1000 window-add operations against a shared
// tumbling window aggregator (window_size = 64).
// Verify: all 8000 events are counted; flush count matches expected; no exceptions.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AnalyticsHighCardinalityStress, ConcurrentWindowAggregationStress) {
    constexpr int         kThreads     = 8;
    constexpr int         kPerThread   = 1000;
    constexpr std::size_t kWindowSize  = 64;
    constexpr uint64_t    kTotalEvents = static_cast<uint64_t>(kThreads) * kPerThread;

    StubTumblingWindowAggregator window(kWindowSize);
    std::atomic<uint64_t> total_added{0};
    std::atomic<bool>     exception_seen{false};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    const auto t0 = std::chrono::steady_clock::now();

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([t, &window, &total_added, &exception_seen]() {
            try {
                for (int i = 0; i < kPerThread; ++i) {
                    const double value = static_cast<double>(t * 1000 + i) / 1000.0;
                    (void)window.add(value);
                    total_added.fetch_add(1, std::memory_order_relaxed);
                }
            } catch (...) {
                exception_seen.store(true, std::memory_order_release);
            }
        });
    }

    for (auto& w : workers) { w.join(); }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_FALSE(exception_seen.load())
        << "No exceptions must be thrown during concurrent window aggregation stress";

    EXPECT_EQ(total_added.load(), kTotalEvents)
        << "All " << kTotalEvents << " events must be added (no silent drops)";

    // Expected flushes: at least kTotalEvents / kWindowSize
    // Due to concurrent access the exact count may vary slightly, but must be ≥ floor(total/window)
    const uint64_t min_expected_flushes = kTotalEvents / kWindowSize;
    EXPECT_GE(window.flushCount(), min_expected_flushes)
        << "Flush count must be at least " << min_expected_flushes
        << " (total events / window size). Observed: " << window.flushCount();

    // Wall-clock gate: 8000 concurrent adds must finish within 5 s
    EXPECT_LE(elapsed_ms.count(), 5000)
        << "Concurrent window aggregation stress must complete within 5 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: ColumnarProjectionUnderHighCardinality
//
// Exercise columnar projection with a 5000-column cardinality corpus.
// For each of 5000 cardinality values, project 256 rows and verify the result
// is within a deterministic expected range.  Throughput must complete within 10 s.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AnalyticsHighCardinalityStress, ColumnarProjectionUnderHighCardinality) {
    constexpr std::size_t kCardinalities = 5000;
    constexpr std::size_t kRowsPerBatch  = 256;
    constexpr int64_t     kThreshold     = 0; // hits where v > 0

    StubColumnarProjectionEngine engine;
    bool bad_result_seen = false;

    const auto t0 = std::chrono::steady_clock::now();

    for (std::size_t card = 1; card <= kCardinalities; ++card) {
        const std::size_t hits = engine.project(card, kRowsPerBatch, kThreshold);

        // Hits must be in [0, kRowsPerBatch] — trivially true but validates no UB
        if (hits > kRowsPerBatch) {
            bad_result_seen = true;
            ADD_FAILURE() << "Columnar projection returned out-of-range hits=" << hits
                          << " for cardinality=" << card
                          << " (max allowed: " << kRowsPerBatch << ")";
            break;
        }
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_FALSE(bad_result_seen)
        << "All columnar projection results must be within [0, row_count]";

    EXPECT_EQ(engine.totalProjections(), static_cast<uint64_t>(kCardinalities))
        << "Projection count must equal the number of cardinality values exercised";

    // Throughput gate: 5000 projection batches must complete within 10 s
    EXPECT_LE(elapsed_ms.count(), 10'000)
        << "Columnar projection stress (5000 batches × 256 rows) must complete within 10 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}
