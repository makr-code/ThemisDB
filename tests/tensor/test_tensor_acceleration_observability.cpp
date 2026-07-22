#include <gtest/gtest.h>

#include "tensor/tensor_compression_routing_accelerator.h"
#include "tensor/tensor_workflow_observability.h"

namespace themis::tensor::test {

TEST(TensorAccelerationTest, CompressionAndRoutingMatchCpuReference) {
    TensorCompressionRoutingAccelerator accelerator;

    const std::vector<float> input = {0.1f, -0.3f, 1.2f, -2.4f};
    const float scale = 0.1f;

    const auto cpu_quantized = accelerator.compressToInt8(input, scale, true);
    const auto dispatch_quantized = accelerator.compressToInt8(input, scale, false);
    EXPECT_EQ(dispatch_quantized, cpu_quantized);

    const std::vector<std::vector<float>> routes = {
        {1.0f, 0.0f, 0.5f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.5f},
    };

    const auto cpu_scores = accelerator.computeRoutingScores(input, routes, true);
    const auto dispatch_scores = accelerator.computeRoutingScores(input, routes, false);
    ASSERT_EQ(dispatch_scores.size(), cpu_scores.size());
    for (std::size_t i = 0; i < dispatch_scores.size(); ++i) {
        EXPECT_NEAR(dispatch_scores[i], cpu_scores[i], 1e-5f);
    }
}

TEST(TensorObservabilityTest, EmitsPrometheusAndDetectsSloViolation) {
    TensorWorkflowObservability obs;

    obs.recordPersistenceOp(4.0, true);
    obs.recordPersistenceOp(8.0, false);
    obs.recordTrainingTransition(TensorWorkflowObservability::TrainingState::Queued);
    obs.recordTrainingTransition(TensorWorkflowObservability::TrainingState::Running);
    obs.recordTrainingTransition(TensorWorkflowObservability::TrainingState::Retry);
    obs.recordTrainingTransition(TensorWorkflowObservability::TrainingState::Failed);
    obs.recordTrainingLatency(120.0);
    obs.recordTrainingLatency(360.0);
    obs.recordGpuDispatch(true, true, true);
    obs.recordCompressionLatency(5.0);
    obs.recordCompressionLatency(45.0);
    obs.recordRoutingLatency(3.0);
    obs.recordRoutingLatency(30.0);

    const auto text = obs.exportPrometheusText();
    EXPECT_NE(text.find("tensor_persistence_ops_total 2"), std::string::npos);
    EXPECT_NE(text.find("tensor_gpu_fallback_total 1"), std::string::npos);
    EXPECT_NE(text.find("tensor_training_latency_p95_ms"), std::string::npos);

    TensorWorkflowSloConfig cfg;
    cfg.max_p95_training_ms = 200.0;
    cfg.max_p95_compression_ms = 10.0;
    cfg.max_p95_routing_ms = 10.0;
    cfg.max_error_rate = 0.05;

    const auto summary = obs.evaluateSlo(cfg);
    EXPECT_FALSE(summary.healthy);
    EXPECT_GT(summary.p95_latency_ms.at("training"), cfg.max_p95_training_ms);
    EXPECT_FALSE(summary.violations.empty());
}

}  // namespace themis::tensor::test
