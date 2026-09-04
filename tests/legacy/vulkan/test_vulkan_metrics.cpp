// Test: Vulkan Backend Metrics Integration Tests
// Validates that VulkanVectorBackend correctly records metrics for
// initialization, distance operations, and error paths.

#include <gtest/gtest.h>
#include "acceleration/vulkan_backend.h"
#include "acceleration/metrics/metrics_collector.h"
#include <vector>
#include <string>

using namespace themis::acceleration;
using namespace themis::acceleration::metrics;

// ============================================================================
// Fixture: clears the metrics collector before each test so counters
// don't accumulate across test cases.
// ============================================================================
class VulkanMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::instance().clear();
    }

    void TearDown() override {
        MetricsCollector::instance().clear();
    }
};

// ============================================================================
// Helper: retrieve a Counter value by name (returns 0 if not registered)
// ============================================================================
static uint64_t counterValue(const std::string& name) {
    auto* c = MetricsCollector::instance().getCounter(name);
    return c ? c->value() : 0u;
}

// ============================================================================
// Test: metric keys are registered as soon as the backend is constructed
// ============================================================================
TEST_F(VulkanMetricsTest, MetricsRegisteredOnConstruction) {
    VulkanBackend backend;

    // Counters must exist after construction (BackendMetrics registers them)
    EXPECT_NE(MetricsCollector::instance().getCounter(
        "themis_acceleration_vulkan_init_success_total"), nullptr);
    EXPECT_NE(MetricsCollector::instance().getCounter(
        "themis_acceleration_vulkan_init_failures_total"), nullptr);
    EXPECT_NE(MetricsCollector::instance().getCounter(
        "themis_acceleration_vulkan_l2_distance_operations_total"), nullptr);
    EXPECT_NE(MetricsCollector::instance().getCounter(
        "themis_acceleration_vulkan_cosine_operations_total"), nullptr);
    EXPECT_NE(MetricsCollector::instance().getCounter(
        "themis_acceleration_vulkan_errors_total"), nullptr);
}

// ============================================================================
// Test: Prometheus export contains Vulkan metric names
// ============================================================================
TEST_F(VulkanMetricsTest, PrometheusExportContainsVulkanMetrics) {
    VulkanBackend backend;
    std::string prom = MetricsCollector::instance().exportPrometheus();

    EXPECT_NE(prom.find("themis_acceleration_vulkan_"), std::string::npos)
        << "Prometheus output should contain vulkan metric names";
}

// ============================================================================
// Test: failed initialize() increments init_failures counter
// ============================================================================
TEST_F(VulkanMetricsTest, InitFailureIncrementedWhenVulkanUnavailable) {
    if (VulkanBackend().isAvailable()) {
        GTEST_SKIP() << "capability:vulkan_failure_path_applicable=false;reason=vulkan_icd_present";
    }

    VulkanBackend backend;
    uint64_t before = counterValue("themis_acceleration_vulkan_init_failures_total");
    backend.initialize();
    uint64_t after = counterValue("themis_acceleration_vulkan_init_failures_total");

    EXPECT_GT(after, before) << "init_failures should increment after failed initialize()";
}

// ============================================================================
// Test: successful initialize() increments init_success counter
// ============================================================================
TEST_F(VulkanMetricsTest, InitSuccessIncrementedOnSuccess) {
    VulkanBackend backend = {};

    if (!backend.isAvailable()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }
    if (!backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_backend_initialized=false;reason=vulkan_initialization_failed_no_gpu_or_shaders";
    }

    EXPECT_EQ(counterValue("themis_acceleration_vulkan_init_success_total"), 1u);
    EXPECT_EQ(counterValue("themis_acceleration_vulkan_init_failures_total"), 0u);

    backend.shutdown();
}

// ============================================================================
// Test: computeDistances (L2) increments l2_distance_operations counter
// ============================================================================
TEST_F(VulkanMetricsTest, L2DistanceOperationRecorded) {
    VulkanBackend backend = {};

    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }

    const size_t nq = 2, nv = 4, dim = 8;
    std::vector<float> queries(nq * dim, 0.5f);
    std::vector<float> vectors(nv * dim, 0.1f);

    backend.computeDistances(queries.data(), nq, dim, vectors.data(), nv, /*useL2=*/true);

    EXPECT_EQ(counterValue("themis_acceleration_vulkan_l2_distance_operations_total"), 1u);
    EXPECT_EQ(counterValue("themis_acceleration_vulkan_cosine_operations_total"), 0u);
    EXPECT_GE(counterValue("themis_acceleration_vulkan_l2_distance_vectors_total"),
              static_cast<uint64_t>(nq * nv));

    backend.shutdown();
}

// ============================================================================
// Test: computeDistances (cosine) increments cosine_operations counter
// ============================================================================
TEST_F(VulkanMetricsTest, CosineDistanceOperationRecorded) {
    VulkanBackend backend = {};

    if (!backend.isAvailable() || !backend.initialize()) {
        GTEST_SKIP() << "capability:vulkan_runtime_available=false;reason=vulkan_not_available";
    }

    const size_t nq = 2, nv = 4, dim = 8;
    std::vector<float> queries(nq * dim, 0.5f);
    std::vector<float> vectors(nv * dim, 0.1f);

    backend.computeDistances(queries.data(), nq, dim, vectors.data(), nv, /*useL2=*/false);

    EXPECT_EQ(counterValue("themis_acceleration_vulkan_cosine_operations_total"), 1u);
    EXPECT_EQ(counterValue("themis_acceleration_vulkan_l2_distance_operations_total"), 0u);

    backend.shutdown();
}

// ============================================================================
// Test: calling computeDistances when not initialized records an error
// ============================================================================
TEST_F(VulkanMetricsTest, ErrorRecordedWhenNotInitialized) {
    VulkanBackend backend;  // never call initialize()

    const size_t nq = 1, nv = 1, dim = 4;
    std::vector<float> q(nq * dim, 1.0f);
    std::vector<float> v(nv * dim, 0.0f);

    auto result = backend.computeDistances(q.data(), nq, dim, v.data(), nv, true);
    EXPECT_TRUE(result.empty()) << "Should return empty result when not initialized";

#ifdef THEMIS_ENABLE_VULKAN
    EXPECT_GT(counterValue("themis_acceleration_vulkan_errors_total"), 0u)
        << "errors_total should be incremented when not initialized";
#endif
}

// ============================================================================
// Test: JSON export includes numeric values for vulkan metrics
// ============================================================================
TEST_F(VulkanMetricsTest, JsonExportContainsVulkanKeys) {
    VulkanBackend backend;
    std::string json = MetricsCollector::instance().exportJSON();

    EXPECT_NE(json.find("themis_acceleration_vulkan_"), std::string::npos)
        << "JSON output should contain vulkan metric keys";
}

#ifndef THEMIS_ENABLE_VULKAN
// When Vulkan is not compiled in, all operations should be no-ops and
// metrics stay at zero (nothing incremented, nothing crashed).
TEST_F(VulkanMetricsTest, NoMetricsWhenVulkanNotCompiled) {
    VulkanBackend backend;
    backend.initialize();

    const size_t nq = 1, nv = 1, dim = 4;
    std::vector<float> q(nq * dim, 1.0f);
    std::vector<float> v(nv * dim, 0.0f);
    backend.computeDistances(q.data(), nq, dim, v.data(), nv, true);
    backend.batchKnnSearch(q.data(), nq, dim, v.data(), nv, 1, true);

    // In the non-Vulkan build the implementation returns immediately
    // without touching any metrics; error counter stays 0.
    EXPECT_EQ(counterValue("themis_acceleration_vulkan_l2_distance_operations_total"), 0u);
    EXPECT_EQ(counterValue("themis_acceleration_vulkan_cosine_operations_total"), 0u);
    EXPECT_EQ(counterValue("themis_acceleration_vulkan_init_success_total"), 0u);
}
#endif
