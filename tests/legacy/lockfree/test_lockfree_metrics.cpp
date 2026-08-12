/*
 * Tests for LockFreeMetrics (Issue #66 – Lock-Free Metrics, v1.6.0)
 *
 * Validates all three acceptance criteria:
 *   AC-1  std::atomic<int64_t> counters (lock-free increment)
 *   AC-2  Lock-free per-thread ring buffer for histograms
 *   AC-3  Thread-local aggregation with periodic background flush
 */

#include "core/concerns/lockfree_metrics.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <latch>
#include <string>
#include <thread>
#include <vector>

using namespace themis::core::concerns;
using namespace std::chrono_literals;

// =============================================================================
// Fixture
// =============================================================================

class LockFreeMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a short flush interval so background-flush tests run quickly.
        m_ = std::make_unique<LockFreeMetrics>(10ms);
    }
    void TearDown() override {
        m_.reset();
    }
    std::unique_ptr<LockFreeMetrics> m_;
};

// =============================================================================
// AC-1  std::atomic counters
// =============================================================================

TEST_F(LockFreeMetricsTest, CounterIncrementByDefault) {
    m_->incrementCounter("requests_total");
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("requests_total"), std::string::npos);
    EXPECT_NE(out.find("1"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, CounterIncrementByValue) {
    m_->incrementCounter("ops_total", 42);
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("ops_total"), std::string::npos);
    EXPECT_NE(out.find("42"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, CounterAccumulates) {
    m_->incrementCounter("hits", 3);
    m_->incrementCounter("hits", 7);
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("hits"), std::string::npos);
    EXPECT_NE(out.find("10"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, CounterWithLabels) {
    m_->incrementCounter("http_requests", 1, {{"method", "GET"}, {"status", "200"}});
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("http_requests"), std::string::npos);
    EXPECT_NE(out.find("method"), std::string::npos);
    EXPECT_NE(out.find("GET"), std::string::npos);
}

/// AC-1: Many threads increment the same counter concurrently – final value
/// must equal the sum of all increments (no lost updates).
TEST_F(LockFreeMetricsTest, CounterConcurrentIncrements_AC1) {
    constexpr int NUM_THREADS   = 16;
    constexpr int OPS_PER_THREAD = 1000;

    std::latch ready(NUM_THREADS);
    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back([&] {
            ready.arrive_and_wait();
            for (int j = 0; j < OPS_PER_THREAD; ++j) {
                m_->incrementCounter("concurrent_counter", 1);
            }
        });
    }
    for (auto& t : workers) t.join();

    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("concurrent_counter"), std::string::npos);
    const int64_t expected = static_cast<int64_t>(NUM_THREADS) * OPS_PER_THREAD;
    EXPECT_NE(out.find(std::to_string(expected)), std::string::npos);
}

TEST_F(LockFreeMetricsTest, RecordErrorCreatesCounter) {
    m_->recordError("db.write");
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("db.write_errors_total"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, RecordSuccessCreatesCounter) {
    m_->recordSuccess("db.read");
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("db.read_success_total"), std::string::npos);
}

// =============================================================================
// Gauge tests
// =============================================================================

TEST_F(LockFreeMetricsTest, SetGauge) {
    m_->setGauge("active_connections", 99.0);
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("active_connections"), std::string::npos);
    EXPECT_NE(out.find("99"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, GaugeSetOverwrites) {
    m_->setGauge("queue_depth", 5.0);
    m_->setGauge("queue_depth", 25.0);
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("25"), std::string::npos);
    // Previous value should not appear (it was overwritten).
    EXPECT_EQ(out.find(" 5\n"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, IncrementGauge) {
    m_->setGauge("workers", 5.0);
    m_->incrementGauge("workers", 3.0);
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("8"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, DecrementGauge) {
    m_->setGauge("workers", 10.0);
    m_->decrementGauge("workers", 4.0);
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("6"), std::string::npos);
}

// =============================================================================
// AC-2  Lock-free ring buffer for histograms
// =============================================================================

TEST_F(LockFreeMetricsTest, HistogramObservation_AC2) {
    m_->observeHistogram("request_duration_ms", 12.5);
    m_->flush();
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("request_duration_ms"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, HistogramAggregatesMultipleSamples_AC2) {
    for (int i = 1; i <= 5; ++i) {
        m_->observeHistogram("latency_ms", static_cast<double>(i * 10));
    }
    m_->flush();
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("latency_ms_count"), std::string::npos);
    EXPECT_NE(out.find("5"), std::string::npos);
    // sum = 10+20+30+40+50 = 150
    EXPECT_NE(out.find("150"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, HistogramWithLabels_AC2) {
    m_->observeHistogram("db_query_ms", 8.0, {{"op", "SELECT"}});
    m_->flush();
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("db_query_ms"), std::string::npos);
    EXPECT_NE(out.find("op"), std::string::npos);
    EXPECT_NE(out.find("SELECT"), std::string::npos);
}

/// AC-2: Fill the ring buffer beyond capacity; verify dropped count is recorded.
TEST_F(LockFreeMetricsTest, HistogramRingBufferOverflowTracked_AC2) {
    // Stop the flush thread so the ring never drains during this test.
    // We create a separate instance with a very long flush interval.
    LockFreeMetrics slow(100'000ms);

    constexpr size_t OVER_CAPACITY = LockFreeMetrics::HISTOGRAM_RING_CAPACITY * 2;
    for (size_t i = 0; i < OVER_CAPACITY; ++i) {
        slow.observeHistogram("overflow_test", static_cast<double>(i));
    }
    // At least HISTOGRAM_RING_CAPACITY observations were dropped (ring full).
    EXPECT_GT(slow.droppedObservations(), 0u);
    slow.shutdown();
}

TEST_F(LockFreeMetricsTest, RecordLatencyCreatesHistogram) {
    m_->recordLatency("db.query", 5.5);
    m_->flush();
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("db.query_latency_ms"), std::string::npos);
}

// =============================================================================
// AC-3  Thread-local aggregation with periodic flush
// =============================================================================

/// AC-3: Multiple threads each observe histograms via their own thread-local
/// ring buffer; after a periodic flush all observations must appear in the
/// global aggregates.
TEST_F(LockFreeMetricsTest, ThreadLocalAggregation_AC3) {
    constexpr int NUM_THREADS    = 8;
    constexpr int OPS_PER_THREAD = 50;

    std::latch ready(NUM_THREADS);
    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back([&] {
            ready.arrive_and_wait();
            for (int j = 0; j < OPS_PER_THREAD; ++j) {
                m_->observeHistogram("tl_latency", static_cast<double>(j + 1));
            }
        });
    }
    for (auto& t : workers) t.join();

    // Allow at least one background flush cycle.
    std::this_thread::sleep_for(50ms);
    m_->flush();

    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("tl_latency"), std::string::npos);

    // Total count must equal NUM_THREADS * OPS_PER_THREAD.
    const uint64_t expected_count = static_cast<uint64_t>(NUM_THREADS) * OPS_PER_THREAD;
    EXPECT_NE(out.find(std::to_string(expected_count)), std::string::npos);
}

/// AC-3: Flush is idempotent – calling flush() repeatedly does not
/// double-count observations.
TEST_F(LockFreeMetricsTest, RepeatedFlushDoesNotDoubleCounts_AC3) {
    m_->observeHistogram("idempotent_metric", 1.0);
    m_->flush();
    m_->flush();
    m_->flush();

    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("idempotent_metric_count 1"), std::string::npos);
}

/// AC-3: Background thread drains ring buffers within the configured interval.
TEST_F(LockFreeMetricsTest, BackgroundFlushDrainsWithinInterval_AC3) {
    // Observe without calling flush() manually.
    m_->observeHistogram("bg_flush_metric", 7.0);

    // Wait for at least two flush cycles (flush_interval = 10 ms).
    std::this_thread::sleep_for(60ms);

    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("bg_flush_metric"), std::string::npos);
}

// =============================================================================
// Export format
// =============================================================================

TEST_F(LockFreeMetricsTest, ExportContainsTypeAnnotations) {
    m_->incrementCounter("requests_total");
    m_->setGauge("memory_bytes", 1024.0);
    m_->observeHistogram("latency", 1.0);
    m_->flush();
    std::string out = m_->exportMetrics();
    EXPECT_NE(out.find("# TYPE"), std::string::npos);
}

TEST_F(LockFreeMetricsTest, ExportEmptyWhenNoMetrics) {
    std::string out = m_->exportMetrics();
    EXPECT_TRUE(out.empty());
}

// =============================================================================
// Lifecycle
// =============================================================================

TEST_F(LockFreeMetricsTest, ResetClearsAllMetrics) {
    m_->incrementCounter("counter", 10);
    m_->setGauge("gauge", 5.0);
    m_->observeHistogram("histo", 1.0);
    m_->flush();
    m_->reset();
    std::string out = m_->exportMetrics();
    EXPECT_TRUE(out.empty());
}

TEST_F(LockFreeMetricsTest, FlushDoesNotThrow) {
    EXPECT_NO_THROW(m_->flush());
}

TEST_F(LockFreeMetricsTest, ShutdownDoesNotThrow) {
    EXPECT_NO_THROW(m_->shutdown());
}

TEST_F(LockFreeMetricsTest, IsHealthyReturnsTrue) {
    EXPECT_TRUE(m_->isHealthy().ok);
}

TEST_F(LockFreeMetricsTest, DroppedObservationsInitiallyZero) {
    EXPECT_EQ(m_->droppedObservations(), 0u);
}

TEST_F(LockFreeMetricsTest, MultipleShutdownCallsAreSafe) {
    m_->shutdown();
    EXPECT_NO_THROW(m_->shutdown());
}
