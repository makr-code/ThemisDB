#include "acceleration/metrics/metrics_collector.h"
#include "acceleration/metrics/backend_metrics.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace themis::acceleration::metrics;

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear metrics before each test
        MetricsCollector::instance().clear();
    }
    
    void TearDown() override {
        // Clean up after each test
        MetricsCollector::instance().clear();
    }
};

// ============================================================================
// Counter Tests
// ============================================================================

TEST_F(MetricsTest, CounterBasicOperations) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_counter", "Test counter metric");
    
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->value(), 0);
    
    counter->increment();
    EXPECT_EQ(counter->value(), 1);
    
    counter->increment(5);
    EXPECT_EQ(counter->value(), 6);
    
    counter->reset();
    EXPECT_EQ(counter->value(), 0);
}

TEST_F(MetricsTest, CounterThreadSafety) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_counter_threaded", "Threaded counter test");
    
    constexpr int num_threads = 10;
    constexpr int increments_per_thread = 1000;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([counter]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter->increment();
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(counter->value(), num_threads * increments_per_thread);
}

TEST_F(MetricsTest, CounterSerialization) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_counter_serialize", "Serialization test");
    
    counter->increment(42);
    
    std::string serialized = counter->serialize();
    EXPECT_NE(serialized.find("test_counter_serialize"), std::string::npos);
    EXPECT_NE(serialized.find("42"), std::string::npos);
}

// ============================================================================
// Gauge Tests
// ============================================================================

TEST_F(MetricsTest, GaugeBasicOperations) {
    auto* gauge = MetricsCollector::instance().registerGauge(
        "test_gauge", "Test gauge metric");
    
    ASSERT_NE(gauge, nullptr);
    EXPECT_DOUBLE_EQ(gauge->value(), 0.0);
    
    gauge->set(42.5);
    EXPECT_DOUBLE_EQ(gauge->value(), 42.5);
    
    gauge->increment(10.0);
    EXPECT_DOUBLE_EQ(gauge->value(), 52.5);
    
    gauge->decrement(2.5);
    EXPECT_DOUBLE_EQ(gauge->value(), 50.0);
}

TEST_F(MetricsTest, GaugeThreadSafety) {
    auto* gauge = MetricsCollector::instance().registerGauge(
        "test_gauge_threaded", "Threaded gauge test");
    
    gauge->set(1000.0);
    
    std::vector<std::thread> threads;
    // Half threads increment, half decrement
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([gauge, i]() {
            if (i % 2 == 0) {
                gauge->increment(1.0);
            } else {
                gauge->decrement(1.0);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should end up at 1000.0 (5 increments, 5 decrements)
    EXPECT_DOUBLE_EQ(gauge->value(), 1000.0);
}

// ============================================================================
// Histogram Tests
// ============================================================================

TEST_F(MetricsTest, HistogramBasicOperations) {
    std::vector<double> buckets = {1.0, 10.0, 100.0};
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_histogram", "Test histogram metric", buckets);
    
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->count(), 0);
    EXPECT_DOUBLE_EQ(histogram->sum(), 0.0);
    
    histogram->observe(5.0);
    histogram->observe(15.0);
    histogram->observe(50.0);
    
    EXPECT_EQ(histogram->count(), 3);
    EXPECT_DOUBLE_EQ(histogram->sum(), 70.0);
    EXPECT_NEAR(histogram->mean(), 23.333, 0.001);
}

TEST_F(MetricsTest, HistogramBuckets) {
    std::vector<double> buckets = {0.001, 0.01, 0.1, 1.0};
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_histogram_buckets", "Bucket test", buckets);
    
    // Observe values in different buckets
    histogram->observe(0.0005);  // < 0.001
    histogram->observe(0.005);   // < 0.01
    histogram->observe(0.05);    // < 0.1
    histogram->observe(0.5);     // < 1.0
    histogram->observe(5.0);     // > 1.0 (inf)
    
    EXPECT_EQ(histogram->count(), 5);
    
    std::string serialized = histogram->serialize();
    EXPECT_NE(serialized.find("_bucket"), std::string::npos);
    EXPECT_NE(serialized.find("+Inf"), std::string::npos);
}

TEST_F(MetricsTest, HistogramThreadSafety) {
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_histogram_threaded", "Threaded histogram test");
    
    constexpr int num_threads = 10;
    constexpr int observations_per_thread = 100;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([histogram]() {
            for (int j = 0; j < observations_per_thread; ++j) {
                histogram->observe(1.0);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(histogram->count(), num_threads * observations_per_thread);
    EXPECT_DOUBLE_EQ(histogram->sum(), num_threads * observations_per_thread);
}

// ============================================================================
// Timer Tests
// ============================================================================

TEST_F(MetricsTest, TimerBasicOperation) {
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_timer", "Timer test");
    
    {
        Timer timer(histogram);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }  // Timer destroyed, duration recorded
    
    EXPECT_EQ(histogram->count(), 1);
    EXPECT_GT(histogram->mean(), 0.008);  // At least 8ms
    EXPECT_LT(histogram->mean(), 0.1);    // Less than 100ms
}

TEST_F(MetricsTest, TimerMove) {
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_timer_move", "Timer move test");
    
    Timer timer1(histogram);
    Timer timer2(std::move(timer1));
    
    // timer1 should no longer record
    // Only timer2 will record when destroyed
    
    EXPECT_EQ(histogram->count(), 0);
}

// ============================================================================
// MetricsCollector Tests
// ============================================================================

TEST_F(MetricsTest, MetricsCollectorGetMetrics) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_get_counter", "Get counter test");
    
    auto* retrieved = MetricsCollector::instance().getCounter("test_get_counter");
    EXPECT_EQ(counter, retrieved);
    
    auto* not_found = MetricsCollector::instance().getCounter("nonexistent");
    EXPECT_EQ(not_found, nullptr);
}

TEST_F(MetricsTest, PrometheusExport) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_prom_counter", "Prometheus counter");
    auto* gauge = MetricsCollector::instance().registerGauge(
        "test_prom_gauge", "Prometheus gauge");
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_prom_histogram", "Prometheus histogram");
    
    counter->increment(10);
    gauge->set(42.0);
    histogram->observe(1.5);
    
    std::string prometheus = MetricsCollector::instance().exportPrometheus();
    
    // Verify format
    EXPECT_NE(prometheus.find("# HELP test_prom_counter"), std::string::npos);
    EXPECT_NE(prometheus.find("# TYPE test_prom_counter counter"), std::string::npos);
    EXPECT_NE(prometheus.find("test_prom_counter{} 10"), std::string::npos);
    
    EXPECT_NE(prometheus.find("# HELP test_prom_gauge"), std::string::npos);
    EXPECT_NE(prometheus.find("# TYPE test_prom_gauge gauge"), std::string::npos);
    
    EXPECT_NE(prometheus.find("# HELP test_prom_histogram"), std::string::npos);
    EXPECT_NE(prometheus.find("# TYPE test_prom_histogram histogram"), std::string::npos);
    EXPECT_NE(prometheus.find("test_prom_histogram_sum"), std::string::npos);
    EXPECT_NE(prometheus.find("test_prom_histogram_count"), std::string::npos);
    EXPECT_NE(prometheus.find("_bucket{le="), std::string::npos);
}

TEST_F(MetricsTest, JSONExport) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_json_counter", "JSON counter");
    auto* gauge = MetricsCollector::instance().registerGauge(
        "test_json_gauge", "JSON gauge");
    
    counter->increment(5);
    gauge->set(3.14);
    
    std::string json = MetricsCollector::instance().exportJSON();
    
    // Verify JSON structure
    EXPECT_NE(json.find("\"counters\":"), std::string::npos);
    EXPECT_NE(json.find("\"gauges\":"), std::string::npos);
    EXPECT_NE(json.find("\"histograms\":"), std::string::npos);
    EXPECT_NE(json.find("\"test_json_counter\": 5"), std::string::npos);
    EXPECT_NE(json.find("\"test_json_gauge\":"), std::string::npos);
}

// ============================================================================
// BackendMetrics Tests
// ============================================================================

TEST_F(MetricsTest, BackendMetricsInitialization) {
    BackendMetrics metrics("CUDA");
    
    metrics.recordInitSuccess();
    metrics.recordInitDuration(0.5);
    
    auto* counter = MetricsCollector::instance().getCounter(
        "themis_acceleration_CUDA_init_success_total");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->value(), 1);
    
    auto* histogram = MetricsCollector::instance().getHistogram(
        "themis_acceleration_CUDA_init_duration_seconds");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->count(), 1);
    EXPECT_DOUBLE_EQ(histogram->sum(), 0.5);
}

TEST_F(MetricsTest, BackendMetricsOperations) {
    BackendMetrics metrics("OpenCL");
    
    metrics.recordL2DistanceOperation(0.001, 100);
    metrics.recordCosineOperation(0.002, 50);
    
    auto* l2_ops = MetricsCollector::instance().getCounter(
        "themis_acceleration_OpenCL_l2_distance_operations_total");
    ASSERT_NE(l2_ops, nullptr);
    EXPECT_EQ(l2_ops->value(), 1);
    
    auto* l2_vectors = MetricsCollector::instance().getCounter(
        "themis_acceleration_OpenCL_l2_distance_vectors_total");
    ASSERT_NE(l2_vectors, nullptr);
    EXPECT_EQ(l2_vectors->value(), 100);
    
    auto* cosine_ops = MetricsCollector::instance().getCounter(
        "themis_acceleration_OpenCL_cosine_operations_total");
    ASSERT_NE(cosine_ops, nullptr);
    EXPECT_EQ(cosine_ops->value(), 1);
}

TEST_F(MetricsTest, BackendMetricsResources) {
    BackendMetrics metrics("HIP");
    
    metrics.setDeviceMemoryUsed(1024 * 1024 * 512);  // 512 MB
    metrics.setDeviceMemoryAvailable(1024 * 1024 * 1024);  // 1 GB
    metrics.setQueueDepth(5);
    
    auto* mem_used = MetricsCollector::instance().getGauge(
        "themis_acceleration_HIP_device_memory_used_bytes");
    ASSERT_NE(mem_used, nullptr);
    EXPECT_DOUBLE_EQ(mem_used->value(), 1024 * 1024 * 512);
    
    auto* queue_depth = MetricsCollector::instance().getGauge(
        "themis_acceleration_HIP_queue_depth");
    ASSERT_NE(queue_depth, nullptr);
    EXPECT_DOUBLE_EQ(queue_depth->value(), 5.0);
}

TEST_F(MetricsTest, BackendMetricsErrors) {
    BackendMetrics metrics("Metal");
    
    metrics.recordError("101");
    metrics.recordKernelLaunchFailure();
    metrics.recordMemoryAllocationFailure();
    
    auto* errors = MetricsCollector::instance().getCounter(
        "themis_acceleration_Metal_errors_total");
    ASSERT_NE(errors, nullptr);
    EXPECT_EQ(errors->value(), 1);
    
    auto* kernel_failures = MetricsCollector::instance().getCounter(
        "themis_acceleration_Metal_kernel_launch_failures_total");
    ASSERT_NE(kernel_failures, nullptr);
    EXPECT_EQ(kernel_failures->value(), 1);
}

TEST_F(MetricsTest, BackendMetricsThroughput) {
    BackendMetrics metrics("CPU");
    
    metrics.recordL2DistanceOperation(0.01, 1000);
    metrics.recordCosineOperation(0.01, 500);
    
    EXPECT_EQ(metrics.getOperationsPerSecond(), 2.0);
    EXPECT_EQ(metrics.getVectorsPerSecond(), 1500.0);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(MetricsTest, PerformanceCounterIncrement) {
    auto* counter = MetricsCollector::instance().registerCounter(
        "test_perf_counter", "Performance test");
    
    auto start = std::chrono::steady_clock::now();
    
    constexpr int iterations = 1000000;
    for (int i = 0; i < iterations; ++i) {
        counter->increment();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    
    // Should be very fast (atomic increment)
    double ns_per_op = (duration / iterations) * 1e9;
    EXPECT_LT(ns_per_op, 100.0);  // Less than 100ns per operation
    
    std::cout << "Counter increment: " << ns_per_op << " ns/op" << std::endl;
}

TEST_F(MetricsTest, PerformanceHistogramObserve) {
    auto* histogram = MetricsCollector::instance().registerHistogram(
        "test_perf_histogram", "Performance test");
    
    auto start = std::chrono::steady_clock::now();
    
    constexpr int iterations = 100000;
    for (int i = 0; i < iterations; ++i) {
        histogram->observe(1.0);
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    
    double ns_per_op = (duration / iterations) * 1e9;
    EXPECT_LT(ns_per_op, 1000.0);  // Less than 1 microsecond per operation
    
    std::cout << "Histogram observe: " << ns_per_op << " ns/op" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
