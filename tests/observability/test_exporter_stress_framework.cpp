/**
 * @file test_exporter_stress_framework.cpp
 * @brief Focused tests for the actual ExporterStressFramework API.
 */

#include "gtest/gtest.h"
#include "observability/exporter_stress_framework.h"

#include <vector>

namespace themis {
namespace observability {

class ExporterStressFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        framework = createExporterStressFramework();
    }

    std::unique_ptr<ExporterStressFramework> framework;
};

TEST_F(ExporterStressFrameworkTest, RunStressTestReturnsMetricsAndStatus) {
    ExporterStressTestConfig config;
    config.metrics_per_second = 1000;
    config.spans_per_second = 500;
    config.metric_cardinality = 10;
    config.duration_seconds = 1;
    config.failure_mode = FailureMode::NONE;

    auto result = framework->runStressTest(config);

    EXPECT_EQ(result.status, StressTestStatus::PASSED);
    EXPECT_GT(result.metrics.total_observations, 0u);
    EXPECT_GT(result.metrics.successful_observations, 0u);
    EXPECT_EQ(result.metrics.lost_observations, 0u);
    EXPECT_GE(result.metrics.p95_latency_ms, 0);
    EXPECT_GE(result.metrics.p99_latency_ms, 0);
}

TEST_F(ExporterStressFrameworkTest, BaselineGateWarmup) {
    ExporterStressTestConfig config;
    config.metrics_per_second = 2000;
    config.spans_per_second = 1000;
    config.metric_cardinality = 20;
    config.duration_seconds = 1;
    config.failure_mode = FailureMode::NONE;

    auto result = framework->runStressTest(config);

    EXPECT_EQ(result.status, StressTestStatus::PASSED);
    EXPECT_GT(result.metrics.successful_observations, 0u);
    EXPECT_EQ(result.failed_checks.size(), 0u);
}

TEST_F(ExporterStressFrameworkTest, RunGateBenchmarksReturnsSixResults) {
    const auto results = framework->runGateBenchmarks();

    EXPECT_EQ(results.size(), 6u);
    EXPECT_EQ(framework->getRegisteredGateCount(), 6u);
    EXPECT_FALSE(framework->getGateName(0).empty());
    EXPECT_FALSE(framework->getGateName(5).empty());
    EXPECT_TRUE(framework->getGateName(99).empty());
}

TEST_F(ExporterStressFrameworkTest, RunMultipleTestsExecutesSequentially) {
    std::vector<ExporterStressTestConfig> configs(3);
    for (auto& cfg : configs) {
        cfg.metrics_per_second = 500;
        cfg.spans_per_second = 250;
        cfg.metric_cardinality = 5;
        cfg.duration_seconds = 1;
        cfg.failure_mode = FailureMode::NONE;
    }

    const auto results = framework->runMultipleTests(configs);

    EXPECT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].status, StressTestStatus::PASSED);
    EXPECT_EQ(results[1].status, StressTestStatus::PASSED);
    EXPECT_EQ(results[2].status, StressTestStatus::PASSED);
}

TEST_F(ExporterStressFrameworkTest, ProgressCallbackInvoked) {
    ExporterStressTestConfig config;
    config.metrics_per_second = 300;
    config.spans_per_second = 150;
    config.metric_cardinality = 4;
    config.duration_seconds = 1;
    config.failure_mode = FailureMode::NONE;

    int progress_calls = 0;
    framework->setProgressCallback([&](std::uint32_t) {
        ++progress_calls;
    });

    const auto result = framework->runStressTest(config);

    EXPECT_EQ(result.status, StressTestStatus::PASSED);
    EXPECT_GT(progress_calls, 0);
}

TEST_F(ExporterStressFrameworkTest, CancelTestReturnsTrueAndResets) {
    EXPECT_TRUE(framework->cancelTest());
    EXPECT_FALSE(framework->cancelTest());
}

TEST_F(ExporterStressFrameworkTest, GenerateComparisonReportHasExpectedSections) {
    ExporterStressTestConfig config_a;
    config_a.metrics_per_second = 500;
    config_a.spans_per_second = 200;
    config_a.metric_cardinality = 5;
    config_a.duration_seconds = 1;
    config_a.failure_mode = FailureMode::NONE;

    ExporterStressTestConfig config_b = config_a;
    config_b.metrics_per_second = 750;
    config_b.spans_per_second = 300;

    const auto baseline = framework->runStressTest(config_a);
    const auto candidate = framework->runStressTest(config_b);
    const auto report = framework->generateComparisonReport(baseline, candidate);

    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("Baseline Results"), std::string::npos);
    EXPECT_NE(report.find("Candidate Results"), std::string::npos);
}

TEST_F(ExporterStressFrameworkTest, PartialPacketLossRemainsWithinBound) {
    ExporterStressTestConfig config;
    config.metrics_per_second = 2000;
    config.spans_per_second = 1000;
    config.metric_cardinality = 10;
    config.duration_seconds = 1;
    config.failure_mode = FailureMode::PARTIAL_PACKET_LOSS;
    config.acceptable_metric_loss_percent = 10.0;
    config.acceptable_span_loss_percent = 10.0;

    const auto result = framework->runStressTest(config);

    EXPECT_LE(result.metrics.loss_percent, 10.0);
    EXPECT_EQ(result.status, StressTestStatus::PASSED);
}

} // namespace observability
} // namespace themis
