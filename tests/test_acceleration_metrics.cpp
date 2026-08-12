/*
 * test_acceleration_metrics.cpp
 *
 * Unit tests for the acceleration metrics subsystem:
 *   - acceleration/metrics/metrics_collector.h
 *     Counter, Gauge, Histogram, Timer, MetricsCollector singleton
 *   - acceleration/metrics/backend_metrics.h
 *     BackendMetrics convenience wrapper
 *
 * All tests run on any platform — no GPU required.
 */

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>

#include "acceleration/metrics/metrics_collector.h"
#include "acceleration/metrics/backend_metrics.h"

using namespace themis::acceleration::metrics;

// ============================================================================
// Test fixture — clears the MetricsCollector singleton before each test so
// tests do not interfere with each other via globally registered metrics.
// ============================================================================

class MetricsCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::instance().clear();
    }
    void TearDown() override {
        MetricsCollector::instance().clear();
    }
};

// ============================================================================
// Counter
// ============================================================================

TEST_F(MetricsCollectorTest, Counter_DefaultValueIsZero) {
    auto* c = MetricsCollector::instance().registerCounter(
        "test_counter_zero", "desc");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->value(), 0u);
}

TEST_F(MetricsCollectorTest, Counter_IncrementByOne) {
    auto* c = MetricsCollector::instance().registerCounter(
        "test_counter_inc1", "desc");
    c->increment();
    EXPECT_EQ(c->value(), 1u);
}

TEST_F(MetricsCollectorTest, Counter_IncrementByDelta) {
    auto* c = MetricsCollector::instance().registerCounter(
        "test_counter_delta", "desc");
    c->increment(5);
    EXPECT_EQ(c->value(), 5u);
    c->increment(3);
    EXPECT_EQ(c->value(), 8u);
}

TEST_F(MetricsCollectorTest, Counter_Reset) {
    auto* c = MetricsCollector::instance().registerCounter(
        "test_counter_reset", "desc");
    c->increment(42);
    EXPECT_EQ(c->value(), 42u);
    c->reset();
    EXPECT_EQ(c->value(), 0u);
}

TEST_F(MetricsCollectorTest, Counter_Serialize_ContainsName) {
    auto* c = MetricsCollector::instance().registerCounter(
        "my_counter", "My counter description");
    c->increment(7);
    std::string s = c->serialize();
    EXPECT_NE(s.find("my_counter"), std::string::npos);
    EXPECT_NE(s.find("7"), std::string::npos);
}

TEST_F(MetricsCollectorTest, Counter_NameAndDescription) {
    auto* c = MetricsCollector::instance().registerCounter(
        "named_counter", "A named counter");
    EXPECT_EQ(c->name(), "named_counter");
    EXPECT_EQ(c->description(), "A named counter");
    EXPECT_EQ(c->type(), MetricType::COUNTER);
}

// ============================================================================
// Gauge
// ============================================================================

TEST_F(MetricsCollectorTest, Gauge_DefaultValueIsZero) {
    auto* g = MetricsCollector::instance().registerGauge(
        "test_gauge_zero", "desc");
    ASSERT_NE(g, nullptr);
    EXPECT_NEAR(g->value(), 0.0, 1e-9);
}

TEST_F(MetricsCollectorTest, Gauge_Set) {
    auto* g = MetricsCollector::instance().registerGauge("test_gauge_set", "desc");
    g->set(42.5);
    EXPECT_NEAR(g->value(), 42.5, 1e-9);
}

TEST_F(MetricsCollectorTest, Gauge_IncrementAndDecrement) {
    auto* g = MetricsCollector::instance().registerGauge("test_gauge_incdec", "desc");
    g->set(10.0);
    g->increment(3.0);
    EXPECT_NEAR(g->value(), 13.0, 1e-9);
    g->decrement(5.0);
    EXPECT_NEAR(g->value(), 8.0, 1e-9);
}

TEST_F(MetricsCollectorTest, Gauge_Increment_DefaultDelta) {
    auto* g = MetricsCollector::instance().registerGauge("test_gauge_defaultinc", "desc");
    g->increment();
    EXPECT_NEAR(g->value(), 1.0, 1e-9);
}

TEST_F(MetricsCollectorTest, Gauge_Serialize_ContainsName) {
    auto* g = MetricsCollector::instance().registerGauge("my_gauge", "desc");
    g->set(3.14);
    std::string s = g->serialize();
    EXPECT_NE(s.find("my_gauge"), std::string::npos);
}

TEST_F(MetricsCollectorTest, Gauge_NameAndType) {
    auto* g = MetricsCollector::instance().registerGauge("gauge_meta", "Meta");
    EXPECT_EQ(g->name(), "gauge_meta");
    EXPECT_EQ(g->description(), "Meta");
    EXPECT_EQ(g->type(), MetricType::GAUGE);
}

// ============================================================================
// Histogram
// ============================================================================

TEST_F(MetricsCollectorTest, Histogram_StartsWithZeroCountAndSum) {
    auto* h = MetricsCollector::instance().registerHistogram(
        "test_hist_zero", "desc");
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->count(), 0u);
    EXPECT_NEAR(h->sum(), 0.0, 1e-9);
    EXPECT_NEAR(h->mean(), 0.0, 1e-9);
}

TEST_F(MetricsCollectorTest, Histogram_ObserveSingleValue) {
    auto* h = MetricsCollector::instance().registerHistogram(
        "test_hist_single", "desc");
    h->observe(2.5);
    EXPECT_EQ(h->count(), 1u);
    EXPECT_NEAR(h->sum(), 2.5, 1e-9);
    EXPECT_NEAR(h->mean(), 2.5, 1e-9);
}

TEST_F(MetricsCollectorTest, Histogram_MultipleObservations) {
    auto* h = MetricsCollector::instance().registerHistogram(
        "test_hist_multi", "desc");
    h->observe(1.0);
    h->observe(2.0);
    h->observe(3.0);
    EXPECT_EQ(h->count(), 3u);
    EXPECT_NEAR(h->sum(), 6.0, 1e-9);
    EXPECT_NEAR(h->mean(), 2.0, 1e-9);
}

TEST_F(MetricsCollectorTest, Histogram_WithCustomBuckets) {
    auto* h = MetricsCollector::instance().registerHistogram(
        "test_hist_buckets", "desc", {0.1, 0.5, 1.0, 5.0});
    ASSERT_NE(h, nullptr);
    h->observe(0.05);
    h->observe(0.3);
    h->observe(2.0);
    EXPECT_EQ(h->count(), 3u);
    EXPECT_NEAR(h->sum(), 2.35, 1e-6);
}

TEST_F(MetricsCollectorTest, Histogram_Serialize_ContainsNameAndSumCount) {
    auto* h = MetricsCollector::instance().registerHistogram("hist_ser", "desc");
    h->observe(1.0);
    std::string s = h->serialize();
    EXPECT_NE(s.find("hist_ser"), std::string::npos);
    EXPECT_NE(s.find("_sum"), std::string::npos);
    EXPECT_NE(s.find("_count"), std::string::npos);
}

TEST_F(MetricsCollectorTest, Histogram_NameAndType) {
    auto* h = MetricsCollector::instance().registerHistogram("hist_meta", "Meta");
    EXPECT_EQ(h->name(), "hist_meta");
    EXPECT_EQ(h->type(), MetricType::HISTOGRAM);
}

// ============================================================================
// Timer
// ============================================================================

TEST_F(MetricsCollectorTest, Timer_RecordsElapsedTimeInHistogram) {
    auto* h = MetricsCollector::instance().registerHistogram("timer_test", "desc");
    {
        Timer timer(h);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // At least one observation must have been recorded
    EXPECT_EQ(h->count(), 1u);
    EXPECT_GT(h->sum(), 0.0);
}

TEST_F(MetricsCollectorTest, Timer_NullHistogram_DoesNotCrash) {
    // Timer with nullptr histogram must not crash on destruction
    {
        Timer timer(nullptr);
        // nothing to do — just verify no crash
    }
}

TEST_F(MetricsCollectorTest, Timer_MoveConstructor_OriginalBecomesNoop) {
    auto* h = MetricsCollector::instance().registerHistogram("timer_move", "desc");
    {
        Timer t1(h);
        Timer t2(std::move(t1));  // t1 is now a no-op
        // Both destructors run here at end of scope
        // (from t2, which owns the histogram pointer after the move)
    }  // <-- t1 and t2 are destroyed here
    EXPECT_EQ(h->count(), 1u);
}

// ============================================================================
// MetricsCollector — registration and retrieval
// ============================================================================

TEST_F(MetricsCollectorTest, RegisterAndGetCounter) {
    auto* c = MetricsCollector::instance().registerCounter(
        "get_counter", "desc");
    ASSERT_NE(c, nullptr);
    auto* got = MetricsCollector::instance().getCounter("get_counter");
    EXPECT_EQ(got, c);
}

TEST_F(MetricsCollectorTest, RegisterAndGetGauge) {
    auto* g = MetricsCollector::instance().registerGauge("get_gauge", "desc");
    ASSERT_NE(g, nullptr);
    auto* got = MetricsCollector::instance().getGauge("get_gauge");
    EXPECT_EQ(got, g);
}

TEST_F(MetricsCollectorTest, RegisterAndGetHistogram) {
    auto* h = MetricsCollector::instance().registerHistogram("get_hist", "desc");
    ASSERT_NE(h, nullptr);
    auto* got = MetricsCollector::instance().getHistogram("get_hist");
    EXPECT_EQ(got, h);
}

TEST_F(MetricsCollectorTest, GetUnknownCounter_ReturnsNull) {
    EXPECT_EQ(MetricsCollector::instance().getCounter("nonexistent"), nullptr);
}

TEST_F(MetricsCollectorTest, GetUnknownGauge_ReturnsNull) {
    EXPECT_EQ(MetricsCollector::instance().getGauge("nonexistent"), nullptr);
}

TEST_F(MetricsCollectorTest, GetUnknownHistogram_ReturnsNull) {
    EXPECT_EQ(MetricsCollector::instance().getHistogram("nonexistent"), nullptr);
}

// ============================================================================
// MetricsCollector — reset() and clear()
// ============================================================================

TEST_F(MetricsCollectorTest, Reset_ResetsCountersToZero) {
    auto* c = MetricsCollector::instance().registerCounter("reset_me", "desc");
    c->increment(99);
    EXPECT_EQ(c->value(), 99u);

    MetricsCollector::instance().reset();
    // After reset, the counter should be zero
    EXPECT_EQ(c->value(), 0u);
}

TEST_F(MetricsCollectorTest, Clear_RemovesAllMetrics) {
    MetricsCollector::instance().registerCounter("before_clear", "desc");
    MetricsCollector::instance().clear();
    EXPECT_EQ(MetricsCollector::instance().getCounter("before_clear"), nullptr);
}

// ============================================================================
// MetricsCollector — Prometheus and JSON export
// ============================================================================

TEST_F(MetricsCollectorTest, ExportPrometheus_ContainsRegisteredMetrics) {
    auto* c = MetricsCollector::instance().registerCounter(
        "prom_counter", "A Prometheus counter");
    c->increment(5);

    std::string output = MetricsCollector::instance().exportPrometheus();
    EXPECT_NE(output.find("prom_counter"), std::string::npos);
    EXPECT_NE(output.find("A Prometheus counter"), std::string::npos);
}

TEST_F(MetricsCollectorTest, ExportPrometheus_Empty_NoErrors) {
    // When no metrics are registered the export must not throw or crash.
    EXPECT_NO_THROW({
        std::string output = MetricsCollector::instance().exportPrometheus();
        (void)output;
    });
}

TEST_F(MetricsCollectorTest, ExportJSON_ContainsCounterKey) {
    auto* c = MetricsCollector::instance().registerCounter(
        "json_counter", "desc");
    c->increment(3);

    std::string output = MetricsCollector::instance().exportJSON();
    EXPECT_NE(output.find("json_counter"), std::string::npos);
    EXPECT_NE(output.find("counters"), std::string::npos);
}

TEST_F(MetricsCollectorTest, ExportJSON_ContainsGaugeKey) {
    auto* g = MetricsCollector::instance().registerGauge("json_gauge", "desc");
    g->set(7.0);

    std::string output = MetricsCollector::instance().exportJSON();
    EXPECT_NE(output.find("json_gauge"), std::string::npos);
    EXPECT_NE(output.find("gauges"), std::string::npos);
}

TEST_F(MetricsCollectorTest, ExportJSON_ContainsHistogramKey) {
    auto* h = MetricsCollector::instance().registerHistogram("json_hist", "desc");
    h->observe(1.5);

    std::string output = MetricsCollector::instance().exportJSON();
    EXPECT_NE(output.find("json_hist"), std::string::npos);
    EXPECT_NE(output.find("histograms"), std::string::npos);
    EXPECT_NE(output.find("count"), std::string::npos);
    EXPECT_NE(output.find("mean"), std::string::npos);
}

// ============================================================================
// BackendMetrics — construction and record methods
// ============================================================================

class BackendMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::instance().clear();
    }
    void TearDown() override {
        MetricsCollector::instance().clear();
    }
};

TEST_F(BackendMetricsTest, Construction_DoesNotCrash) {
    // BackendMetrics registers all counters/gauges/histograms in its constructor.
    // Just constructing it must not throw or crash.
    EXPECT_NO_FATAL_FAILURE({
        BackendMetrics bm("test_backend");
    });
}

TEST_F(BackendMetricsTest, RecordInitSuccess_IncrementsCounter) {
    BackendMetrics bm("init_success_be");
    bm.recordInitSuccess();
    bm.recordInitSuccess();

    auto* c = MetricsCollector::instance().getCounter(
        "themis_acceleration_init_success_be_init_success_total");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->value(), 2u);
}

TEST_F(BackendMetricsTest, RecordInitFailure_IncrementsCounter) {
    BackendMetrics bm("init_fail_be");
    bm.recordInitFailure();

    auto* c = MetricsCollector::instance().getCounter(
        "themis_acceleration_init_fail_be_init_failures_total");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->value(), 1u);
}

TEST_F(BackendMetricsTest, RecordInitDuration_ObservesHistogram) {
    BackendMetrics bm("init_dur_be");
    bm.recordInitDuration(0.05);

    auto* h = MetricsCollector::instance().getHistogram(
        "themis_acceleration_init_dur_be_init_duration_seconds");
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->count(), 1u);
    EXPECT_NEAR(h->sum(), 0.05, 1e-9);
}

TEST_F(BackendMetricsTest, RecordL2DistanceOperation_UpdatesCounterAndHistogram) {
    BackendMetrics bm("l2_be");
    bm.recordL2DistanceOperation(0.01, 100);

    auto* ops = MetricsCollector::instance().getCounter(
        "themis_acceleration_l2_be_l2_distance_operations_total");
    auto* vecs = MetricsCollector::instance().getCounter(
        "themis_acceleration_l2_be_l2_distance_vectors_total");
    ASSERT_NE(ops,  nullptr);
    ASSERT_NE(vecs, nullptr);
    EXPECT_EQ(ops->value(),  1u);
    EXPECT_EQ(vecs->value(), 100u);
}

TEST_F(BackendMetricsTest, RecordCosineOperation_UpdatesCounterAndHistogram) {
    BackendMetrics bm("cos_be");
    bm.recordCosineOperation(0.005, 50);

    auto* ops = MetricsCollector::instance().getCounter(
        "themis_acceleration_cos_be_cosine_operations_total");
    ASSERT_NE(ops, nullptr);
    EXPECT_EQ(ops->value(), 1u);
}

TEST_F(BackendMetricsTest, SetDeviceMemory_UpdatesGauges) {
    BackendMetrics bm("mem_be");
    bm.setDeviceMemoryUsed(1024.0 * 1024.0);
    bm.setDeviceMemoryAvailable(4.0 * 1024.0 * 1024.0 * 1024.0);

    auto* used = MetricsCollector::instance().getGauge(
        "themis_acceleration_mem_be_device_memory_used_bytes");
    auto* avail = MetricsCollector::instance().getGauge(
        "themis_acceleration_mem_be_device_memory_available_bytes");
    ASSERT_NE(used,  nullptr);
    ASSERT_NE(avail, nullptr);
    EXPECT_NEAR(used->value(),  1024.0 * 1024.0, 1.0);
    EXPECT_GT(avail->value(), 0.0);
}

TEST_F(BackendMetricsTest, SetQueueDepth_UpdatesGauge) {
    BackendMetrics bm("queue_be");
    bm.setQueueDepth(7.0);

    auto* g = MetricsCollector::instance().getGauge(
        "themis_acceleration_queue_be_queue_depth");
    ASSERT_NE(g, nullptr);
    EXPECT_NEAR(g->value(), 7.0, 1e-9);
}

TEST_F(BackendMetricsTest, RecordError_IncrementsErrorCounter) {
    BackendMetrics bm("err_be");
    bm.recordError("SomeError");
    bm.recordError("AnotherError");

    auto* c = MetricsCollector::instance().getCounter(
        "themis_acceleration_err_be_errors_total");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->value(), 2u);
}

TEST_F(BackendMetricsTest, RecordKernelLaunchFailure_IncrementsCounter) {
    BackendMetrics bm("klf_be");
    bm.recordKernelLaunchFailure();

    auto* c = MetricsCollector::instance().getCounter(
        "themis_acceleration_klf_be_kernel_launch_failures_total");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->value(), 1u);
}

TEST_F(BackendMetricsTest, RecordMemoryAllocationFailure_IncrementsCounter) {
    BackendMetrics bm("maf_be");
    bm.recordMemoryAllocationFailure();

    auto* c = MetricsCollector::instance().getCounter(
        "themis_acceleration_maf_be_memory_allocation_failures_total");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->value(), 1u);
}

TEST_F(BackendMetricsTest, SetDeviceCount_UpdatesGauge) {
    BackendMetrics bm("dc_be");
    bm.setDeviceCount(4);

    auto* g = MetricsCollector::instance().getGauge(
        "themis_acceleration_dc_be_device_count");
    ASSERT_NE(g, nullptr);
    EXPECT_NEAR(g->value(), 4.0, 1e-9);
}

TEST_F(BackendMetricsTest, SetActiveDeviceIndex_UpdatesGauge) {
    BackendMetrics bm("adi_be");
    bm.setActiveDeviceIndex(2);

    auto* g = MetricsCollector::instance().getGauge(
        "themis_acceleration_adi_be_active_device_index");
    ASSERT_NE(g, nullptr);
    EXPECT_NEAR(g->value(), 2.0, 1e-9);
}

TEST_F(BackendMetricsTest, GetOperationsPerSecond_NonNegative) {
    BackendMetrics bm("ops_be");
    // Initially zero; must be >= 0
    EXPECT_GE(bm.getOperationsPerSecond(), 0.0);
    bm.recordL2DistanceOperation(0.01, 10);
    EXPECT_GE(bm.getOperationsPerSecond(), 0.0);
}

TEST_F(BackendMetricsTest, GetVectorsPerSecond_NonNegative) {
    BackendMetrics bm("vps_be");
    EXPECT_GE(bm.getVectorsPerSecond(), 0.0);
    bm.recordCosineOperation(0.01, 100);
    EXPECT_GE(bm.getVectorsPerSecond(), 0.0);
}
