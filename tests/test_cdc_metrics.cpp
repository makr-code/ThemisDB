// Test: CDC Metrics and Observability
// Tests for latency histograms, throughput tracking, and enhanced metrics

#include <gtest/gtest.h>
#include "cdc/cdc_metrics.h"
#include <thread>
#include <chrono>

using namespace themis::cdc;

// ===== Latency Histogram Tests =====

TEST(LatencyHistogramTest, BasicRecording) {
    LatencyHistogram hist;
    
    hist.record(50);   // 50us
    hist.record(200);  // 200us
    hist.record(1500); // 1.5ms
    
    EXPECT_EQ(hist.count(), 3);
}

TEST(LatencyHistogramTest, Average) {
    LatencyHistogram hist;
    
    hist.record(100);
    hist.record(200);
    hist.record(300);
    
    EXPECT_DOUBLE_EQ(hist.average(), 200.0);
}

TEST(LatencyHistogramTest, Percentiles) {
    LatencyHistogram hist;
    
    // Record 100 samples from 0-99 microseconds
    for (int i = 0; i < 100; i++) {
        hist.record(i);
    }
    
    EXPECT_EQ(hist.count(), 100);
    
    // P50 should be around 50us
    uint64_t p50 = hist.p50();
    EXPECT_GE(p50, 40);
    EXPECT_LE(p50, 60);
    
    // Bucketed histogram returns bucket midpoints, not exact order-statistics.
    // For 0..99us all samples fall into the first bucket (<100us), midpoint 50us.
    uint64_t p95 = hist.p95();
    EXPECT_GE(p95, 40);
    EXPECT_LE(p95, 60);
    
    // Same bucket behavior applies to P99 for this dataset.
    uint64_t p99 = hist.p99();
    EXPECT_GE(p99, 40);
    EXPECT_LE(p99, 60);
}

TEST(LatencyHistogramTest, HighLatencies) {
    LatencyHistogram hist;
    
    hist.record(100);     // 100us
    hist.record(10000);   // 10ms
    hist.record(100000);  // 100ms
    hist.record(1000000); // 1s
    
    EXPECT_EQ(hist.count(), 4);
    
    // Average should be high
    EXPECT_GT(hist.average(), 50000);
    
    // P99 should be very high
    EXPECT_GT(hist.p99(), 100000);
}

TEST(LatencyHistogramTest, JsonSerialization) {
    LatencyHistogram hist;
    
    for (int i = 0; i < 100; i++) {
        hist.record(i * 100);  // 0-9900us
    }
    
    auto json = hist.toJson();
    
    EXPECT_EQ(json["count"], 100);
    EXPECT_GT(json["average_us"], 0);
    EXPECT_GT(json["p50_us"], 0);
    EXPECT_GT(json["p95_us"], 0);
    EXPECT_GT(json["p99_us"], 0);
}

TEST(LatencyHistogramTest, Reset) {
    LatencyHistogram hist;
    
    hist.record(100);
    hist.record(200);
    EXPECT_EQ(hist.count(), 2);
    
    hist.reset();
    
    EXPECT_EQ(hist.count(), 0);
    EXPECT_DOUBLE_EQ(hist.average(), 0.0);
    EXPECT_EQ(hist.p50(), 0);
}

TEST(LatencyHistogramTest, EmptyHistogram) {
    LatencyHistogram hist;
    
    EXPECT_EQ(hist.count(), 0);
    EXPECT_DOUBLE_EQ(hist.average(), 0.0);
    EXPECT_EQ(hist.p50(), 0);
    EXPECT_EQ(hist.p95(), 0);
    EXPECT_EQ(hist.p99(), 0);
}

// ===== Throughput Tracker Tests =====

TEST(ThroughputTrackerTest, BasicRecording) {
    ThroughputTracker tracker;
    
    tracker.recordEvent(100);
    tracker.recordEvent(200);
    tracker.recordEvent(300);
    
    // Should have recorded 3 events with 600 bytes total
    auto json = tracker.toJson();
    EXPECT_EQ(json["events_in_window"], 3);
    EXPECT_EQ(json["bytes_in_window"], 600);
}

TEST(ThroughputTrackerTest, EventsPerSecond) {
    ThroughputTracker tracker;
    
    // Record events
    for (int i = 0; i < 100; i++) {
        tracker.recordEvent(100);
    }
    
    // Wait a tiny bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Should have positive throughput
    double eps = tracker.eventsPerSecond();
    EXPECT_GT(eps, 0);
    
    // With 100 events in ~0.1s, should be around 1000 eps
    EXPECT_GT(eps, 500);
}

TEST(ThroughputTrackerTest, BytesPerSecond) {
    ThroughputTracker tracker;
    
    // Record 100 events with 1KB each
    for (int i = 0; i < 100; i++) {
        tracker.recordEvent(1024);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    double bps = tracker.bytesPerSecond();
    EXPECT_GT(bps, 0);
    
    // With 100KB in ~0.1s, should be around 1MB/s
    EXPECT_GT(bps, 500000);
}

TEST(ThroughputTrackerTest, JsonSerialization) {
    ThroughputTracker tracker;
    
    tracker.recordEvent(500);
    tracker.recordEvent(1000);
    
    auto json = tracker.toJson();
    
    EXPECT_TRUE(json.contains("events_per_second"));
    EXPECT_TRUE(json.contains("bytes_per_second"));
    EXPECT_TRUE(json.contains("events_in_window"));
    EXPECT_TRUE(json.contains("bytes_in_window"));
}

TEST(ThroughputTrackerTest, Reset) {
    ThroughputTracker tracker;
    
    tracker.recordEvent(100);
    tracker.recordEvent(200);
    
    tracker.reset();
    
    auto json = tracker.toJson();
    EXPECT_EQ(json["events_in_window"], 0);
    EXPECT_EQ(json["bytes_in_window"], 0);
}

// ===== CDC Metrics Tests =====

TEST(CDCMetricsTest, AllMetricsPresent) {
    CDCMetrics metrics;
    
    // Record some data
    metrics.record_event_latency.record(100);
    metrics.flush_latency.record(1000);
    metrics.throughput.recordEvent(500);
    metrics.events_recorded = 10;
    metrics.events_flushed = 8;
    
    auto json = metrics.toJson();
    
    // Check structure
    EXPECT_TRUE(json.contains("latency"));
    EXPECT_TRUE(json.contains("throughput"));
    EXPECT_TRUE(json.contains("counters"));
    
    // Check nested latency
    EXPECT_TRUE(json["latency"].contains("record_event"));
    EXPECT_TRUE(json["latency"].contains("flush"));
    EXPECT_TRUE(json["latency"].contains("compression"));
    EXPECT_TRUE(json["latency"].contains("decompression"));
    
    // Check counters
    EXPECT_EQ(json["counters"]["events_recorded"], 10);
    EXPECT_EQ(json["counters"]["events_flushed"], 8);
}

TEST(CDCMetricsTest, Reset) {
    CDCMetrics metrics;
    
    metrics.record_event_latency.record(100);
    metrics.events_recorded = 10;
    metrics.throughput.recordEvent(500);
    
    metrics.reset();
    
    EXPECT_EQ(metrics.record_event_latency.count(), 0);
    EXPECT_EQ(metrics.events_recorded.load(), 0);
    EXPECT_EQ(metrics.throughput.toJson()["events_in_window"], 0);
}

TEST(CDCMetricsTest, Counters) {
    CDCMetrics metrics;
    
    metrics.events_recorded++;
    metrics.events_flushed++;
    metrics.flush_count++;
    metrics.errors++;
    metrics.retries++;
    
    auto json = metrics.toJson();
    auto counters = json["counters"];
    
    EXPECT_EQ(counters["events_recorded"], 1);
    EXPECT_EQ(counters["events_flushed"], 1);
    EXPECT_EQ(counters["flush_count"], 1);
    EXPECT_EQ(counters["errors"], 1);
    EXPECT_EQ(counters["retries"], 1);
}

// ===== Scoped Timer Tests =====

TEST(ScopedTimerTest, AutomaticRecording) {
    LatencyHistogram hist;
    
    {
        ScopedTimer timer(hist);
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }  // Timer destructor records latency
    
    EXPECT_EQ(hist.count(), 1);
    EXPECT_GT(hist.average(), 50);  // Should be at least 50us
}

TEST(ScopedTimerTest, MultipleTimers) {
    LatencyHistogram hist;
    
    for (int i = 0; i < 10; i++) {
        ScopedTimer timer(hist);
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    
    EXPECT_EQ(hist.count(), 10);
}

// ===== Integration Tests =====

TEST(CDCMetricsTest, RealWorldScenario) {
    CDCMetrics metrics;
    
    // Simulate recording 1000 events
    for (int i = 0; i < 1000; i++) {
        // Measure record event latency
        {
            ScopedTimer timer(metrics.record_event_latency);
            // Simulate event recording (1-10us)
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        
        metrics.events_recorded++;
        metrics.throughput.recordEvent(512);  // 512 bytes per event
    }
    
    // Simulate 10 flushes
    for (int i = 0; i < 10; i++) {
        ScopedTimer timer(metrics.flush_latency);
        // Simulate flush (100-500us)
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        metrics.flush_count++;
        metrics.events_flushed += 100;
    }
    
    // Verify metrics
    EXPECT_EQ(metrics.record_event_latency.count(), 1000);
    EXPECT_EQ(metrics.flush_latency.count(), 10);
    EXPECT_EQ(metrics.events_recorded.load(), 1000);
    EXPECT_EQ(metrics.events_flushed.load(), 1000);
    EXPECT_EQ(metrics.flush_count.load(), 10);
    
    // Check latencies are reasonable
    EXPECT_GT(metrics.record_event_latency.average(), 0);
    EXPECT_GT(metrics.flush_latency.average(), 0);
    
    // Check throughput
    EXPECT_GT(metrics.throughput.eventsPerSecond(), 0);
    EXPECT_GT(metrics.throughput.bytesPerSecond(), 0);
    
    // Verify JSON serialization works
    auto json = metrics.toJson();
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("latency"));
    EXPECT_TRUE(json.contains("throughput"));
    EXPECT_TRUE(json.contains("counters"));
}

TEST(CDCMetricsTest, HighThroughputScenario) {
    CDCMetrics metrics;
    
    auto start = std::chrono::steady_clock::now();
    
    // Simulate very fast event recording
    for (int i = 0; i < 10000; i++) {
        metrics.record_event_latency.record(1);  // 1us per event
        metrics.events_recorded++;
        metrics.throughput.recordEvent(100);
    }
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    
    // Should process quickly
    EXPECT_LT(ms, 1000);  // Less than 1 second
    
    // Check metrics
    EXPECT_EQ(metrics.record_event_latency.count(), 10000);
    EXPECT_EQ(metrics.events_recorded.load(), 10000);
    
    // Average latency should be very low
    EXPECT_LT(metrics.record_event_latency.average(), 100);
}
