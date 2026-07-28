#include <gtest/gtest.h>

#include "distributed_tensor/tensor_training_coordinator.h"

namespace themis { namespace distributed_tensor { namespace test { 

class DeterministicWorker final : public ITensorTrainingWorker {
public:
    explicit DeterministicWorker(std::string failing_shard = {})
        : failing_shard_(std::move(failing_shard)) {}

    std::optional<std::vector<float>> processShard(const std::string&,
                                                   const std::string& node_id,
                                                   const TensorShardWorkItem& shard,
                                                   std::string& error_message) override {
        if (!failing_shard_.empty() && shard.shard_id == failing_shard_ && !failed_once_) {
            failed_once_ = true;
            error_message = "transient failure on " + node_id;
            return std::nullopt;
        }
        return shard.tensor_values;
    }

private:
    std::string failing_shard_;
    bool failed_once_ = false;
};

TEST(TensorTrainingCoordinatorTest, LifecycleRetryAndAggregation) {
    TensorTrainingCoordinator coordinator;
    coordinator.registerWorker("node-a", std::make_shared<DeterministicWorker>("s1"));

    TensorTrainingJobSpec spec;
    spec.job_id = "job-1";
    spec.tenant_id = "tenant";
    spec.domain = "domain";
    spec.max_retries = 2;
    spec.max_iterations = 2;
    spec.convergence_epsilon = 1e-6;
    spec.shard_work = {
        {"s1", {1.0f, 3.0f}},
        {"s2", {3.0f, 5.0f}},
    };

    ASSERT_TRUE(coordinator.submitJob(spec));
    ASSERT_TRUE(coordinator.runNextJob());

    const auto result = coordinator.result("job-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->state, TrainingJobState::SUCCESS);
    EXPECT_GE(result->retries, 1u);
    ASSERT_EQ(result->aggregated_tensor.size(), 2u);
    EXPECT_NEAR(result->aggregated_tensor[0], 2.0f, 1e-6f);
    EXPECT_NEAR(result->aggregated_tensor[1], 4.0f, 1e-6f);
}

TEST(TensorTrainingCoordinatorTest, EndToEndMultiNodeSimulation) {
    TensorTrainingCoordinator coordinator;
    coordinator.registerWorker("node-0", std::make_shared<DeterministicWorker>());
    coordinator.registerWorker("node-1", std::make_shared<DeterministicWorker>());
    coordinator.registerWorker("node-2", std::make_shared<DeterministicWorker>());

    TensorTrainingJobSpec spec;
    spec.job_id = "job-e2e";
    spec.tenant_id = "tenant-x";
    spec.domain = "routing";
    spec.max_retries = 1;
    spec.max_iterations = 3;
    spec.convergence_epsilon = 0.0;
    spec.shard_work = {
        {"a", {2.0f, 2.0f, 2.0f}},
        {"b", {4.0f, 4.0f, 4.0f}},
        {"c", {6.0f, 6.0f, 6.0f}},
    };

    ASSERT_TRUE(coordinator.submitJob(spec));
    ASSERT_TRUE(coordinator.runNextJob());

    const auto result = coordinator.result("job-e2e");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->state, TrainingJobState::SUCCESS);
    ASSERT_EQ(result->aggregated_tensor.size(), 3u);
    EXPECT_NEAR(result->aggregated_tensor[0], 4.0f, 1e-6f);
    EXPECT_NEAR(result->aggregated_tensor[1], 4.0f, 1e-6f);
    EXPECT_NEAR(result->aggregated_tensor[2], 4.0f, 1e-6f);
}

TEST(TensorTrainingCoordinatorTest, FailsOnMismatchedShardDimensions) {
    TensorTrainingCoordinator coordinator;
    coordinator.registerWorker("node-0", std::make_shared<DeterministicWorker>());

    TensorTrainingJobSpec spec;
    spec.job_id = "job-dim-mismatch";
    spec.tenant_id = "tenant";
    spec.domain = "domain";
    spec.max_retries = 0;
    spec.max_iterations = 1;
    spec.shard_work = {
        {"a", {1.0f, 2.0f}},
        {"b", {3.0f}},
    };

    ASSERT_TRUE(coordinator.submitJob(spec));
    ASSERT_TRUE(coordinator.runNextJob());
    const auto result = coordinator.result(spec.job_id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->state, TrainingJobState::FAILED);
    EXPECT_EQ(result->error_message, "shard output dimension mismatch");
}
} } } // namespace themis::distributed_tensor::test
