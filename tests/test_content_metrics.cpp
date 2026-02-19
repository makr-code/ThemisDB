#include <gtest/gtest.h>
#include "content/content_metrics.h"
#include <nlohmann/json.hpp>
#include <thread>

using namespace themis::content;
using json = nlohmann::json;

// ============================================================================
// Basic Metrics Tests
// ============================================================================

TEST(ContentMetricsTest, InitialState) {
    ContentMetrics metrics;
    
    EXPECT_EQ(metrics.getTotalIngestions(), 0);
    EXPECT_EQ(metrics.getTotalBytesProcessed(), 0);
    EXPECT_EQ(metrics.getTotalValidations(), 0);
    EXPECT_EQ(metrics.getTotalProcessing(), 0);
    EXPECT_EQ(metrics.getTotalErrors(), 0);
    EXPECT_EQ(metrics.getTotalTimeouts(), 0);
}

TEST(ContentMetricsTest, RecordIngestion) {
    ContentMetrics metrics;
    
    metrics.recordIngestion("application/pdf", 1000);
    metrics.recordIngestion("image/png", 2000);
    metrics.recordIngestion("application/pdf", 1500);
    
    EXPECT_EQ(metrics.getTotalIngestions(), 3);
    EXPECT_EQ(metrics.getTotalBytesProcessed(), 4500);
    EXPECT_EQ(metrics.getCountByMimeType("application/pdf"), 2);
    EXPECT_EQ(metrics.getCountByMimeType("image/png"), 1);
}

TEST(ContentMetricsTest, RecordValidation) {
    ContentMetrics metrics;
    
    metrics.recordValidation(true);
    metrics.recordValidation(true);
    metrics.recordValidation(false);
    
    EXPECT_EQ(metrics.getTotalValidations(), 3);
    EXPECT_EQ(metrics.getSuccessfulValidations(), 2);
    EXPECT_EQ(metrics.getFailedValidations(), 1);
    EXPECT_NEAR(metrics.getValidationSuccessRate(), 66.67, 0.01);
}

TEST(ContentMetricsTest, RecordProcessing) {
    ContentMetrics metrics;
    
    metrics.recordProcessing("text/plain", true);
    metrics.recordProcessing("application/json", true);
    metrics.recordProcessing("text/html", false);
    
    EXPECT_EQ(metrics.getTotalProcessing(), 3);
    EXPECT_EQ(metrics.getSuccessfulProcessing(), 2);
    EXPECT_EQ(metrics.getFailedProcessing(), 1);
    EXPECT_NEAR(metrics.getProcessingSuccessRate(), 66.67, 0.01);
}

TEST(ContentMetricsTest, RecordExtraction) {
    ContentMetrics metrics;
    
    metrics.recordExtraction(true);
    metrics.recordExtraction(true);
    metrics.recordExtraction(true);
    metrics.recordExtraction(false);
    
    // Extraction metrics tracked internally
}

TEST(ContentMetricsTest, RecordChunking) {
    ContentMetrics metrics;
    
    metrics.recordChunking(10);
    metrics.recordChunking(5);
    metrics.recordChunking(3);
    
    // Chunking metrics tracked internally
}

TEST(ContentMetricsTest, RecordEmbedding) {
    ContentMetrics metrics;
    
    metrics.recordEmbedding(100);
    metrics.recordEmbedding(50);
    
    // Embedding metrics tracked internally
}

// ============================================================================
// Latency Metrics Tests
// ============================================================================

TEST(ContentMetricsTest, RecordLatency) {
    ContentMetrics metrics;
    
    metrics.recordLatency("validation", 10.0);
    metrics.recordLatency("validation", 20.0);
    metrics.recordLatency("validation", 30.0);
    metrics.recordLatency("validation", 40.0);
    metrics.recordLatency("validation", 50.0);
    
    auto percentiles = metrics.getLatencyPercentiles("validation");
    
    EXPECT_EQ(percentiles["count"], 5.0);
    EXPECT_DOUBLE_EQ(percentiles["min"], 10.0);
    EXPECT_DOUBLE_EQ(percentiles["max"], 50.0);
    EXPECT_DOUBLE_EQ(percentiles["avg"], 30.0);
    EXPECT_DOUBLE_EQ(percentiles["p50"], 30.0);  // Median of [10,20,30,40,50]
}

TEST(ContentMetricsTest, LatencyPercentiles) {
    ContentMetrics metrics;
    
    // Add 100 samples from 1.0 to 100.0
    for (int i = 1; i <= 100; i++) {
        metrics.recordLatency("extraction", static_cast<double>(i));
    }
    
    auto percentiles = metrics.getLatencyPercentiles("extraction");
    
    EXPECT_EQ(percentiles["count"], 100.0);
    EXPECT_DOUBLE_EQ(percentiles["min"], 1.0);
    EXPECT_DOUBLE_EQ(percentiles["max"], 100.0);
    EXPECT_DOUBLE_EQ(percentiles["avg"], 50.5);
    
    // p50 should be around 50.5
    EXPECT_NEAR(percentiles["p50"], 50.5, 1.0);
    // p95 should be around 95.5
    EXPECT_NEAR(percentiles["p95"], 95.5, 1.0);
    // p99 should be around 99.5
    EXPECT_NEAR(percentiles["p99"], 99.5, 1.0);
}

TEST(ContentMetricsTest, EmptyLatencyPercentiles) {
    ContentMetrics metrics;
    
    auto percentiles = metrics.getLatencyPercentiles("nonexistent");
    
    EXPECT_TRUE(percentiles.empty());
}

// ============================================================================
// Error Metrics Tests
// ============================================================================

TEST(ContentMetricsTest, RecordError) {
    ContentMetrics metrics;
    
    metrics.recordError(1001);  // CONTENT_SIZE_EXCEEDED
    metrics.recordError(1001);
    metrics.recordError(1200);  // CONTENT_MALWARE_DETECTED
    
    EXPECT_EQ(metrics.getTotalErrors(), 3);
}

TEST(ContentMetricsTest, RecordErrorCategory) {
    ContentMetrics metrics;
    
    metrics.recordErrorCategory("validation");
    metrics.recordErrorCategory("validation");
    metrics.recordErrorCategory("security");
    
    // Categories are tracked internally but not exposed in basic getters
}

TEST(ContentMetricsTest, RecordTimeout) {
    ContentMetrics metrics;
    
    metrics.recordTimeout("extraction");
    metrics.recordTimeout("chunking");
    metrics.recordTimeout("extraction");
    
    EXPECT_EQ(metrics.getTotalTimeouts(), 3);
}

// ============================================================================
// Validation Metrics Tests
// ============================================================================

TEST(ContentMetricsTest, RecordValidationViolation) {
    ContentMetrics metrics;
    
    metrics.recordValidationViolation("size");
    metrics.recordValidationViolation("format");
    metrics.recordValidationViolation("size");
    
    // Violations are tracked internally
}

// ============================================================================
// Cache Metrics Tests
// ============================================================================

TEST(ContentMetricsTest, CacheMetrics) {
    ContentMetrics metrics;
    
    metrics.recordCacheHit();
    metrics.recordCacheHit();
    metrics.recordCacheHit();
    metrics.recordCacheMiss();
    
    EXPECT_DOUBLE_EQ(metrics.getCacheHitRate(), 75.0);  // 3/4 = 75%
}

TEST(ContentMetricsTest, CacheHitRateZero) {
    ContentMetrics metrics;
    
    EXPECT_DOUBLE_EQ(metrics.getCacheHitRate(), 0.0);
}

TEST(ContentMetricsTest, CacheHitRateAllMisses) {
    ContentMetrics metrics;
    
    metrics.recordCacheMiss();
    metrics.recordCacheMiss();
    metrics.recordCacheMiss();
    
    EXPECT_DOUBLE_EQ(metrics.getCacheHitRate(), 0.0);
}

TEST(ContentMetricsTest, CacheHitRateAllHits) {
    ContentMetrics metrics;
    
    metrics.recordCacheHit();
    metrics.recordCacheHit();
    
    EXPECT_DOUBLE_EQ(metrics.getCacheHitRate(), 100.0);
}

// ============================================================================
// Format Distribution Tests
// ============================================================================

TEST(ContentMetricsTest, MimeTypeCounts) {
    ContentMetrics metrics;
    
    metrics.recordIngestion("application/pdf", 1000);
    metrics.recordIngestion("image/png", 500);
    metrics.recordIngestion("application/pdf", 2000);
    metrics.recordIngestion("text/plain", 300);
    
    auto counts = metrics.getMimeTypeCounts();
    
    EXPECT_EQ(counts.size(), 3);
    EXPECT_EQ(counts["application/pdf"], 2);
    EXPECT_EQ(counts["image/png"], 1);
    EXPECT_EQ(counts["text/plain"], 1);
}

// ============================================================================
// Success Rate Tests
// ============================================================================

TEST(ContentMetricsTest, ValidationSuccessRateZero) {
    ContentMetrics metrics;
    
    EXPECT_DOUBLE_EQ(metrics.getValidationSuccessRate(), 100.0);  // Default to 100% when no validations
}

TEST(ContentMetricsTest, ProcessingSuccessRateZero) {
    ContentMetrics metrics;
    
    EXPECT_DOUBLE_EQ(metrics.getProcessingSuccessRate(), 100.0);  // Default to 100% when no processing
}

TEST(ContentMetricsTest, ValidationSuccessRateAllSuccess) {
    ContentMetrics metrics;
    
    metrics.recordValidation(true);
    metrics.recordValidation(true);
    metrics.recordValidation(true);
    
    EXPECT_DOUBLE_EQ(metrics.getValidationSuccessRate(), 100.0);
}

TEST(ContentMetricsTest, ValidationSuccessRateAllFailure) {
    ContentMetrics metrics;
    
    metrics.recordValidation(false);
    metrics.recordValidation(false);
    
    EXPECT_DOUBLE_EQ(metrics.getValidationSuccessRate(), 0.0);
}

// ============================================================================
// JSON Export Tests
// ============================================================================

TEST(ContentMetricsTest, ToJson) {
    ContentMetrics metrics;
    
    metrics.recordIngestion("application/pdf", 1000);
    metrics.recordValidation(true);
    metrics.recordProcessing("application/pdf", true);
    metrics.recordLatency("validation", 15.5);
    metrics.recordError(1001);
    metrics.recordCacheHit();
    
    json j = metrics.toJson();
    
    EXPECT_TRUE(j.contains("throughput"));
    EXPECT_TRUE(j.contains("rates"));
    EXPECT_TRUE(j.contains("errors"));
    EXPECT_TRUE(j.contains("cache"));
    EXPECT_TRUE(j.contains("mime_types"));
    EXPECT_TRUE(j.contains("latency"));
    
    EXPECT_EQ(j["throughput"]["total_ingestions"], 1);
    EXPECT_EQ(j["throughput"]["total_bytes_processed"], 1000);
    EXPECT_EQ(j["cache"]["hits"], 1);
}

// ============================================================================
// Prometheus Export Tests
// ============================================================================

TEST(ContentMetricsTest, ToPrometheusFormat) {
    ContentMetrics metrics;
    
    metrics.recordIngestion("application/pdf", 1000);
    metrics.recordValidation(true);
    metrics.recordProcessing("application/pdf", true);
    metrics.recordCacheHit();
    metrics.recordError(1001);
    
    std::string prometheus = metrics.toPrometheusFormat();
    
    EXPECT_FALSE(prometheus.empty());
    EXPECT_NE(prometheus.find("content_ingestions_total"), std::string::npos);
    EXPECT_NE(prometheus.find("content_bytes_processed_total"), std::string::npos);
    EXPECT_NE(prometheus.find("content_validations_total"), std::string::npos);
    EXPECT_NE(prometheus.find("content_cache_requests_total"), std::string::npos);
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST(ContentMetricsTest, Reset) {
    ContentMetrics metrics;
    
    metrics.recordIngestion("application/pdf", 1000);
    metrics.recordValidation(true);
    metrics.recordProcessing("application/pdf", true);
    metrics.recordLatency("validation", 15.5);
    metrics.recordError(1001);
    
    EXPECT_GT(metrics.getTotalIngestions(), 0);
    
    metrics.reset();
    
    EXPECT_EQ(metrics.getTotalIngestions(), 0);
    EXPECT_EQ(metrics.getTotalBytesProcessed(), 0);
    EXPECT_EQ(metrics.getTotalValidations(), 0);
    EXPECT_EQ(metrics.getTotalProcessing(), 0);
    EXPECT_EQ(metrics.getTotalErrors(), 0);
    EXPECT_TRUE(metrics.getMimeTypeCounts().empty());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(ContentMetricsTest, ConcurrentIngestion) {
    ContentMetrics metrics;
    
    auto worker = [&metrics]() {
        for (int i = 0; i < 1000; i++) {
            metrics.recordIngestion("application/pdf", 100);
        }
    };
    
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    
    t1.join();
    t2.join();
    t3.join();
    
    EXPECT_EQ(metrics.getTotalIngestions(), 3000);
    EXPECT_EQ(metrics.getTotalBytesProcessed(), 300000);
}

TEST(ContentMetricsTest, ConcurrentLatencyRecording) {
    ContentMetrics metrics;
    
    auto worker = [&metrics]() {
        for (int i = 0; i < 100; i++) {
            metrics.recordLatency("test_op", static_cast<double>(i));
        }
    };
    
    std::thread t1(worker);
    std::thread t2(worker);
    
    t1.join();
    t2.join();
    
    auto percentiles = metrics.getLatencyPercentiles("test_op");
    EXPECT_EQ(percentiles["count"], 200.0);
}
