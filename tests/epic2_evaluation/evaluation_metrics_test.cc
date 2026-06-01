/**
 * @file evaluation_metrics_test.cc
 * @brief Contract tests for IEvaluationMetrics (sub-issue #5439).
 *
 * Validates factory construction, observation recording, report generation,
 * ablation variant registration, listing, and reset behavior.
 * Production statistics aggregation is tracked in sub-issue #5439.
 */

#include "evaluation/include/evaluation_metrics.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace themis::evaluation;

namespace {

MetricObservation makeObs(MetricDimension dim, double value,
                           const std::string& exp = "") {
    MetricObservation o;
    o.dimension     = dim;
    o.value         = value;
    o.experiment_id = exp;
    o.sample_index  = 0;
    return o;
}

} // namespace

class EvaluationMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        metrics_ = makeEvaluationMetrics();
        ASSERT_NE(metrics_, nullptr);
    }

    std::unique_ptr<IEvaluationMetrics> metrics_;
};

TEST_F(EvaluationMetricsTest, FactoryReturnsNonNull) {
    EXPECT_NE(metrics_, nullptr);
}

TEST_F(EvaluationMetricsTest, RecordObservationDoesNotThrow) {
    EXPECT_NO_THROW(metrics_->record(
        makeObs(MetricDimension::RecallAtK, 0.95)));
}

TEST_F(EvaluationMetricsTest, ReportAfterRecordHasNGreaterThanZero) {
    metrics_->record(makeObs(MetricDimension::RecallAtK, 0.95));
    MetricReport r = metrics_->report(MetricDimension::RecallAtK);
    EXPECT_GE(r.n, 1u);
}

TEST_F(EvaluationMetricsTest, ReportMeanMatchesSingleObservation) {
    metrics_->record(makeObs(MetricDimension::LatencyP50Ms, 12.5));
    MetricReport r = metrics_->report(MetricDimension::LatencyP50Ms);
    if (r.n > 0) {
        EXPECT_NEAR(r.mean, 12.5, 1e-6);
    }
}

TEST_F(EvaluationMetricsTest, ReportAllReturnsNonEmptyAfterRecord) {
    metrics_->record(makeObs(MetricDimension::RecallAtK, 0.9));
    metrics_->record(makeObs(MetricDimension::LatencyP99Ms, 30.0));
    auto reports = metrics_->reportAll();
    EXPECT_FALSE(reports.empty());
}

TEST_F(EvaluationMetricsTest, RegisterVariantDoesNotThrow) {
    AblationVariant v;
    v.id          = "variant-fp16";
    v.description = "FP16 compression ablation";
    v.params["compression"] = "fp16";
    EXPECT_NO_THROW(metrics_->registerVariant(v));
}

TEST_F(EvaluationMetricsTest, ListVariantsIncludesRegistered) {
    AblationVariant v;
    v.id = "variant-int8";
    metrics_->registerVariant(v);
    auto ids = metrics_->listVariants();
    bool found = false;
    for (const auto& id : ids) {
        if (id == "variant-int8") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(EvaluationMetricsTest, ResetClearsObservations) {
    metrics_->record(makeObs(MetricDimension::RecallAtK, 0.88));
    metrics_->reset();
    MetricReport r = metrics_->report(MetricDimension::RecallAtK);
    EXPECT_EQ(r.n, 0u);
}

TEST_F(EvaluationMetricsTest, ExperimentScopedReportDoesNotThrow) {
    metrics_->record(makeObs(MetricDimension::RecallAtK, 0.92, "exp-a"));
    EXPECT_NO_THROW(metrics_->report(MetricDimension::RecallAtK, "exp-a"));
    EXPECT_NO_THROW(metrics_->reportAll("exp-a"));
}
