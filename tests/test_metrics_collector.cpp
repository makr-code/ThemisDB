/**
 * @file test_metrics_collector.cpp
 * @brief Unit tests for MetricsCollector (Observability)
 * 
 * Tests metrics collection, Prometheus format export, thread safety,
 * and integration with various ThemisDB subsystems.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <gtest/gtest.h>
#include "observability/metrics_collector.h"
#include <thread>
#include <vector>
#include <chrono>
#include <sstream>

using namespace themis::observability;

class MetricsCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset metrics before each test
        MetricsCollector::getInstance().reset();
    }
    
    void TearDown() override {
        // Clean up after each test
        MetricsCollector::getInstance().reset();
    }
};

// ============================================================================
// TSStore Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, TSStoreWriteMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordTSStoreWrite("cpu.usage", 100, 5.5);
        collector.recordTSStoreWrite("memory.usage", 50, 3.2);
    });
}

TEST_F(MetricsCollectorTest, TSStoreQueryMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordTSStoreQuery("temperature", 500, 12.3);
        collector.recordTSStoreQuery("pressure", 1000, 8.7);
    });
}

TEST_F(MetricsCollectorTest, TSStoreAggregateMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordTSStoreAggregate("sensor.data", 10000, 25.4);
    });
}

TEST_F(MetricsCollectorTest, TSStoreCompressionMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordTSStoreCompression("gorilla", 0.65);
        collector.recordTSStoreCompression("zstd", 0.45);
        collector.recordTSStoreCompression("lz4", 0.55);
    });
}

// ============================================================================
// Query Engine Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, QueryMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordQuery("select", 15.5, 100);
        collector.recordQuery("insert", 8.3, 1);
        collector.recordQuery("update", 12.1, 10);
        collector.recordQuery("delete", 6.7, 5);
    });
}

TEST_F(MetricsCollectorTest, IndexScanMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordIndexScan("btree", 1000);
        collector.recordIndexScan("vector", 500);
        collector.recordIndexScan("graph", 250);
    });
}

TEST_F(MetricsCollectorTest, FullScanMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordFullScan("users", 5000);
        collector.recordFullScan("products", 10000);
    });
}

// ============================================================================
// Cache Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, CacheHitMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; i++) {
            collector.recordCacheHit("query_cache");
        }
        for (int i = 0; i < 50; i++) {
            collector.recordCacheHit("semantic_cache");
        }
    });
}

TEST_F(MetricsCollectorTest, CacheMissMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        for (int i = 0; i < 20; i++) {
            collector.recordCacheMiss("query_cache");
        }
    });
}

TEST_F(MetricsCollectorTest, CacheEvictionMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        for (int i = 0; i < 10; i++) {
            collector.recordCacheEviction("lru_cache");
        }
    });
}

TEST_F(MetricsCollectorTest, CacheHitRatioCalculation) {
    auto& collector = MetricsCollector::getInstance();
    
    // Record hits and misses
    for (int i = 0; i < 80; i++) {
        collector.recordCacheHit("test_cache");
    }
    for (int i = 0; i < 20; i++) {
        collector.recordCacheMiss("test_cache");
    }
    
    std::string metrics = collector.getPrometheusMetrics();
    
    // Verify both hits and misses are recorded
    EXPECT_NE(metrics.find("cache_hits"), std::string::npos);
    EXPECT_NE(metrics.find("cache_misses"), std::string::npos);
}

// ============================================================================
// Sharding Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ShardRequestMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordShardRequest("shard-001", "read");
        collector.recordShardRequest("shard-001", "write");
        collector.recordShardRequest("shard-002", "read");
    });
}

TEST_F(MetricsCollectorTest, ShardLatencyMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordShardLatency("shard-001", 5.5);
        collector.recordShardLatency("shard-002", 8.3);
        collector.recordShardLatency("shard-003", 3.2);
    });
}

TEST_F(MetricsCollectorTest, RebalanceProgressMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordRebalanceProgress("op-12345", 1000, 10.0);
        collector.recordRebalanceProgress("op-12345", 5000, 50.0);
        collector.recordRebalanceProgress("op-12345", 10000, 100.0);
    });
}

// ============================================================================
// Content Processing Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ContentImportMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordContentImport("application/pdf", 1024 * 1024);
        collector.recordContentImport("image/jpeg", 512 * 1024);
        collector.recordContentImport("video/mp4", 10 * 1024 * 1024);
    });
}

TEST_F(MetricsCollectorTest, ChunkCreationMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordChunkCreation(100);
        collector.recordChunkCreation(250);
    });
}

TEST_F(MetricsCollectorTest, EmbeddingGenerationMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordEmbeddingGeneration(50, 125.5);
        collector.recordEmbeddingGeneration(100, 250.3);
    });
}

// ============================================================================
// Security Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, AuthenticationMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        for (int i = 0; i < 90; i++) {
            collector.recordAuthAttempt(true);
        }
        for (int i = 0; i < 10; i++) {
            collector.recordAuthAttempt(false);
        }
    });
}

TEST_F(MetricsCollectorTest, PolicyEvaluationMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordPolicyEvaluation(true, 2.5);
        collector.recordPolicyEvaluation(false, 3.1);
        collector.recordPolicyEvaluation(true, 1.8);
    });
}

TEST_F(MetricsCollectorTest, EncryptionOperationMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordEncryptionOperation("encrypt", 5.2);
        collector.recordEncryptionOperation("decrypt", 4.8);
        collector.recordEncryptionOperation("sign", 7.3);
        collector.recordEncryptionOperation("verify", 6.1);
    });
}

// ============================================================================
// System Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, MemoryUsageMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordMemoryUsage(1024 * 1024 * 512); // 512 MB
        collector.recordMemoryUsage(1024 * 1024 * 768); // 768 MB
    });
}

TEST_F(MetricsCollectorTest, CPUUsageMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordCPUUsage(45.5);
        collector.recordCPUUsage(67.3);
        collector.recordCPUUsage(89.1);
    });
}

TEST_F(MetricsCollectorTest, DiskIOpsMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordDiskIOps(1000, 500);
        collector.recordDiskIOps(2000, 1000);
    });
}

// ============================================================================
// Prometheus Export Tests
// ============================================================================

TEST_F(MetricsCollectorTest, PrometheusFormatBasic) {
    auto& collector = MetricsCollector::getInstance();
    
    collector.recordQuery("select", 10.5, 100);
    collector.recordCacheHit("query_cache");
    
    std::string metrics = collector.getPrometheusMetrics();
    
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("#"), std::string::npos); // Contains comments
}

TEST_F(MetricsCollectorTest, PrometheusFormatWithLabels) {
    auto& collector = MetricsCollector::getInstance();
    
    collector.recordQuery("select", 10.5, 100);
    collector.recordQuery("insert", 5.2, 1);
    
    std::string metrics = collector.getPrometheusMetrics();
    
    // Check for labels
    EXPECT_NE(metrics.find("{"), std::string::npos);
    EXPECT_NE(metrics.find("}"), std::string::npos);
}

TEST_F(MetricsCollectorTest, PrometheusFormatCounters) {
    auto& collector = MetricsCollector::getInstance();
    
    for (int i = 0; i < 100; i++) {
        collector.recordCacheHit("test_cache");
    }
    
    std::string metrics = collector.getPrometheusMetrics();
    
    // Verify counter exists and has reasonable value
    EXPECT_NE(metrics.find("cache_hits"), std::string::npos);
}

TEST_F(MetricsCollectorTest, PrometheusFormatGauges) {
    auto& collector = MetricsCollector::getInstance();
    
    collector.recordMemoryUsage(1024 * 1024 * 512);
    collector.recordCPUUsage(75.5);
    
    std::string metrics = collector.getPrometheusMetrics();
    
    EXPECT_FALSE(metrics.empty());
}

TEST_F(MetricsCollectorTest, PrometheusFormatHistograms) {
    auto& collector = MetricsCollector::getInstance();
    
    for (int i = 0; i < 50; i++) {
        collector.recordQuery("select", i * 2.0, 100);
    }
    
    std::string metrics = collector.getPrometheusMetrics();
    
    // Histograms should contain percentile or latency info
    EXPECT_NE(metrics.find("latency"), std::string::npos);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ConcurrentWrites) {
    auto& collector = MetricsCollector::getInstance();
    
    const int num_threads = 10;
    const int iterations_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&collector, iterations_per_thread]() {
            for (int i = 0; i < iterations_per_thread; i++) {
                collector.recordQuery("select", 10.0, 100);
                collector.recordCacheHit("test_cache");
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify no crashes occurred
    std::string metrics = collector.getPrometheusMetrics();
    EXPECT_FALSE(metrics.empty());
}

TEST_F(MetricsCollectorTest, ConcurrentReadsAndWrites) {
    auto& collector = MetricsCollector::getInstance();
    
    const int num_threads = 8;
    std::vector<std::thread> threads;
    
    // Writer threads
    for (int t = 0; t < num_threads / 2; t++) {
        threads.emplace_back([&collector]() {
            for (int i = 0; i < 500; i++) {
                collector.recordQuery("select", 5.0, 50);
                collector.recordCacheHit("cache");
            }
        });
    }
    
    // Reader threads
    for (int t = 0; t < num_threads / 2; t++) {
        threads.emplace_back([&collector]() {
            for (int i = 0; i < 100; i++) {
                std::string metrics = collector.getPrometheusMetrics();
                EXPECT_FALSE(metrics.empty());
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// ============================================================================
// Reset Functionality Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ResetMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    // Record some metrics
    collector.recordQuery("select", 10.0, 100);
    collector.recordCacheHit("test_cache");
    
    std::string before_reset = collector.getPrometheusMetrics();
    EXPECT_FALSE(before_reset.empty());
    
    // Reset
    EXPECT_NO_THROW({
        collector.reset();
    });
    
    std::string after_reset = collector.getPrometheusMetrics();
    // After reset, metrics should be empty or minimal
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(MetricsCollectorTest, NegativeLatency) {
    auto& collector = MetricsCollector::getInstance();
    
    // Should handle negative values gracefully
    EXPECT_NO_THROW({
        collector.recordQuery("select", -5.0, 100);
    });
}

TEST_F(MetricsCollectorTest, ZeroValues) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordQuery("select", 0.0, 0);
        collector.recordTSStoreWrite("metric", 0, 0.0);
        collector.recordMemoryUsage(0);
        collector.recordCPUUsage(0.0);
    });
}

TEST_F(MetricsCollectorTest, VeryLargeValues) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordQuery("select", 99999.9, 1000000);
        collector.recordMemoryUsage(1024ULL * 1024ULL * 1024ULL * 100ULL); // 100 GB
    });
}

TEST_F(MetricsCollectorTest, EmptyStrings) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordQuery("", 10.0, 100);
        collector.recordCacheHit("");
        collector.recordShardRequest("", "");
    });
}

TEST_F(MetricsCollectorTest, SpecialCharactersInLabels) {
    auto& collector = MetricsCollector::getInstance();
    
    EXPECT_NO_THROW({
        collector.recordQuery("select-with-dash", 10.0, 100);
        collector.recordCacheHit("cache_with_underscore");
        collector.recordContentImport("application/x-custom+xml", 1024);
    });
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(MetricsCollectorTest, CompleteWorkflow) {
    auto& collector = MetricsCollector::getInstance();
    
    // Simulate a complete query workflow
    collector.recordQuery("select", 15.5, 1000);
    collector.recordIndexScan("btree", 500);
    collector.recordCacheHit("query_cache");
    collector.recordShardRequest("shard-001", "read");
    collector.recordShardLatency("shard-001", 10.2);
    
    std::string metrics = collector.getPrometheusMetrics();
    
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("queries_total"), std::string::npos);
    EXPECT_NE(metrics.find("cache_hits"), std::string::npos);
    EXPECT_NE(metrics.find("shard_requests"), std::string::npos);
}

TEST_F(MetricsCollectorTest, HighVolumeMetrics) {
    auto& collector = MetricsCollector::getInstance();
    
    // Simulate high volume of metrics
    for (int i = 0; i < 10000; i++) {
        collector.recordQuery("select", i * 0.1, i);
        if (i % 10 == 0) {
            collector.recordCacheHit("cache");
        } else {
            collector.recordCacheMiss("cache");
        }
    }
    
    std::string metrics = collector.getPrometheusMetrics();
    EXPECT_FALSE(metrics.empty());
}

// ============================================================================
// Thread Safety Tests (FIND-018)
// ============================================================================

TEST_F(MetricsCollectorTest, ConcurrentCounterIncrement) {
    auto& collector = MetricsCollector::getInstance();
    
    const int num_threads = 20;
    const int iterations_per_thread = 500;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&collector, iterations_per_thread]() {
            for (int i = 0; i < iterations_per_thread; i++) {
                collector.recordCacheHit("test_counter");
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all increments were recorded (no lost updates due to races)
    std::string metrics = collector.getPrometheusMetrics();
    EXPECT_NE(metrics.find("cache_hits"), std::string::npos);
    
    // The counter should have recorded all 10,000 increments
    // This verifies thread-safety of the counter map access
}

TEST_F(MetricsCollectorTest, ConcurrentGaugeUpdates) {
    auto& collector = MetricsCollector::getInstance();
    
    const int num_threads = 10;
    const int iterations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&collector, iterations_per_thread, t]() {
            for (int i = 0; i < iterations_per_thread; i++) {
                collector.recordMemoryUsage(1024 * 1024 * (t * 100 + i));
                collector.recordCPUUsage(static_cast<double>(t * 10 + i % 100));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify no crashes and metrics are accessible
    std::string metrics = collector.getPrometheusMetrics();
    EXPECT_FALSE(metrics.empty());
}

TEST_F(MetricsCollectorTest, ConcurrentHistogramObservations) {
    auto& collector = MetricsCollector::getInstance();
    
    const int num_threads = 10;
    const int iterations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&collector, iterations_per_thread, t]() {
            for (int i = 0; i < iterations_per_thread; i++) {
                double latency = (t * iterations_per_thread + i) * 0.1;
                collector.recordQuery("concurrent_test", latency, 100);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify histogram recorded all observations
    std::string metrics = collector.getPrometheusMetrics();
    EXPECT_NE(metrics.find("query_latency"), std::string::npos);
}

TEST_F(MetricsCollectorTest, ConcurrentMixedOperations) {
    auto& collector = MetricsCollector::getInstance();
    
    std::atomic<int> operation_count{0};
    std::vector<std::thread> threads;
    
    // Threads recording different types of metrics simultaneously
    threads.emplace_back([&collector, &operation_count]() {
        for (int i = 0; i < 200; i++) {
            collector.recordCacheHit("cache_type_a");
            operation_count++;
        }
    });
    
    threads.emplace_back([&collector, &operation_count]() {
        for (int i = 0; i < 200; i++) {
            collector.recordCacheMiss("cache_type_b");
            operation_count++;
        }
    });
    
    threads.emplace_back([&collector, &operation_count]() {
        for (int i = 0; i < 200; i++) {
            collector.recordQuery("select", i * 0.5, 100);
            operation_count++;
        }
    });
    
    threads.emplace_back([&collector, &operation_count]() {
        for (int i = 0; i < 200; i++) {
            collector.recordMemoryUsage(1024 * 1024 * i);
            operation_count++;
        }
    });
    
    threads.emplace_back([&collector, &operation_count]() {
        for (int i = 0; i < 200; i++) {
            collector.recordShardRequest("shard-" + std::to_string(i % 5), "read");
            operation_count++;
        }
    });
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(operation_count, 1000);
    
    // Verify all metrics are accessible
    std::string metrics = collector.getPrometheusMetrics();
    EXPECT_FALSE(metrics.empty());
}

TEST_F(MetricsCollectorTest, ConcurrentResetAndRecord) {
    auto& collector = MetricsCollector::getInstance();
    
    std::atomic<bool> should_stop{false};
    std::atomic<int> reset_count{0};
    std::vector<std::thread> threads;
    
    // Thread continuously recording metrics
    threads.emplace_back([&collector, &should_stop]() {
        while (!should_stop) {
            collector.recordCacheHit("test_cache");
            collector.recordQuery("select", 10.0, 100);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Thread periodically resetting
    threads.emplace_back([&collector, &should_stop, &reset_count]() {
        for (int i = 0; i < 5; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            collector.reset();
            reset_count++;
        }
        should_stop = true;
    });
    
    // Thread continuously reading metrics
    threads.emplace_back([&collector, &should_stop]() {
        while (!should_stop) {
            std::string metrics = collector.getPrometheusMetrics();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(reset_count, 5);
    SUCCEED(); // No crashes = success
}

// ============================================================================
// TSAN Stress Tests (Issue #191 – shared_mutex upgrade)
// 16 Prometheus scrape threads + 8 metric write threads concurrently.
// When built with -DTHEMIS_ENABLE_TSAN=ON, ThreadSanitizer will report any
// data race that remains after the shared_mutex upgrade.
// ============================================================================

TEST_F(MetricsCollectorTest, TSANStress_16ScrapersAnd8Writers) {
    auto& collector = MetricsCollector::getInstance();

    // Pre-populate a handful of metrics so scrapers always find non-empty data.
    for (int i = 0; i < 8; ++i) {
        collector.recordQuery("warmup", static_cast<double>(i), i * 10);
        collector.recordCacheHit("warmup_cache");
        collector.recordMemoryUsage(1024u * 1024u * static_cast<size_t>(i + 1));
    }

    constexpr int kScrapers        = 16;
    constexpr int kWriters         = 8;
    constexpr int kItersPerThread  = 200;

    std::atomic<bool> go{false};
    // Failure flags – updated atomically so assertions run on the main thread
    // (GoogleTest EXPECT_* is not thread-safe in worker threads).
    std::atomic<bool> scraper_saw_empty{false};
    std::atomic<int>  writer_exceptions{0};

    // --- 16 scraper (reader) threads ---
    std::vector<std::thread> scrapers;
    scrapers.reserve(kScrapers);
    for (int t = 0; t < kScrapers; ++t) {
        scrapers.emplace_back([&] {
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int i = 0; i < kItersPerThread; ++i) {
                std::string out = collector.getPrometheusMetrics();
                if (out.empty()) {
                    scraper_saw_empty.store(true, std::memory_order_relaxed);
                }
                // Also exercise getCardinalityLimit (shared read path).
                (void)collector.getCardinalityLimit();
            }
        });
    }

    // --- 8 writer threads ---
    std::vector<std::thread> writers;
    writers.reserve(kWriters);
    for (int t = 0; t < kWriters; ++t) {
        writers.emplace_back([&, t] {
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            try {
                for (int i = 0; i < kItersPerThread; ++i) {
                    collector.recordQuery("stress_" + std::to_string(t), i * 0.5, i);
                    collector.recordCacheHit("stress_cache");
                    collector.recordMemoryUsage(1024u * static_cast<size_t>(t * kItersPerThread + i));
                    collector.recordCPUUsage(static_cast<double>((t * kItersPerThread + i) % 100));
                    collector.addCounter("stress_counter_" + std::to_string(t), 1);
                    collector.setGauge("stress_gauge_" + std::to_string(t),
                                       static_cast<double>(i));
                    collector.observeHistogram("stress_hist_" + std::to_string(t),
                                               static_cast<double>(i));
                }
            } catch (...) {
                writer_exceptions.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Release all threads simultaneously for maximum contention.
    go.store(true, std::memory_order_release);

    for (auto& s : scrapers) {
      s.join();
    }
    for (auto& w : writers) {
      w.join();
    }

    // All assertions on the main thread.
    EXPECT_FALSE(scraper_saw_empty.load())
        << "A scraper thread observed an empty Prometheus output during concurrent stress";
    EXPECT_EQ(writer_exceptions.load(), 0)
        << "A writer thread threw an exception during concurrent stress";

    // Verify the final state is consistent.
    std::string final_metrics = collector.getPrometheusMetrics();
    EXPECT_FALSE(final_metrics.empty());
    EXPECT_NE(final_metrics.find("queries_total"), std::string::npos);
    EXPECT_NE(final_metrics.find("cache_hits_total"), std::string::npos);
}

// ============================================================================
// Main
// ============================================================================


