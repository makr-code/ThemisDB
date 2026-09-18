/**
 * @file tensor_training_coordinator.h
 * @brief Coordinator for distributed tensor training runs.
 *
 * Manages the lifecycle of a distributed training job: worker registration,
 * gradient aggregation, fault tolerance, and checkpoint synchronization.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::distributed_tensor {

enum class TrainingJobState {
    QUEUED,
    RUNNING,
    SUCCESS,
    FAILED,
    RETRY,
    CANCELLED,
};

struct TensorShardWorkItem {
    std::string shard_id;
    std::vector<float> tensor_values;
};

struct TensorTrainingJobSpec {
    std::string job_id;
    std::string tenant_id;
    std::string domain;
    std::vector<TensorShardWorkItem> shard_work;
    std::size_t max_retries = 1;
    std::size_t max_iterations = 2;
    double convergence_epsilon = 1e-5;
};

struct TensorTrainingJobResult {
    std::string job_id;
    TrainingJobState state = TrainingJobState::QUEUED;
    std::size_t retries = 0;
    std::size_t iterations = 0;
    std::vector<float> aggregated_tensor;
    std::string error_message;
};

/** @brief I tensor training worker component. */
class ITensorTrainingWorker {
public:
    /**
     * @brief TBD: Describe ~ITensorTrainingWorker.
     * @return Return value.
     */
    virtual ~ITensorTrainingWorker() = default;

    /**
     * @brief TBD: Describe processShard.
     * @param[in] job_id Input parameter.
     * @param[in] node_id Input parameter.
     * @param[in] shard Input parameter.
     * @param[in,out] error_message Input/output parameter.
     * @return Return value.
     */
    virtual std::optional<std::vector<float>> processShard(
        const std::string& job_id,
        const std::string& node_id,
        const TensorShardWorkItem& shard,
        std::string& error_message) = 0;
};

/**
 * @brief Multi-node coordinator for distributed tensor decomposition/training jobs.
 */
class TensorTrainingCoordinator {
public:
    /**
     * @brief TBD: Describe registerWorker.
     * @param[in] node_id Input parameter.
     * @param[in] worker Input parameter.
     */
    void registerWorker(const std::string& node_id, std::shared_ptr<ITensorTrainingWorker> worker);

    [[nodiscard]] bool submitJob(const TensorTrainingJobSpec& spec);
    [[nodiscard]] bool cancelJob(const std::string& job_id);
    [[nodiscard]] bool runNextJob();

    [[nodiscard]] std::optional<TensorTrainingJobResult> result(const std::string& job_id) const;

private:
    /**
     * @brief TBD: Describe runShardWithRetry.
     * @param[in] spec Input parameter.
     * @param[in] shard Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return Return value.
     */
    std::optional<std::vector<float>> runShardWithRetry(const TensorTrainingJobSpec& spec,
                                                        const TensorShardWorkItem& shard,
                                                        TensorTrainingJobResult& result);

    /**
     * @brief TBD: Describe aggregateShardResults.
     * @param[in] shard_results Input parameter.
     * @return Return value.
     */
    static std::vector<float> aggregateShardResults(const std::vector<std::vector<float>>& shard_results);
    /**
     * @brief TBD: Describe hasConverged.
     * @param[in] prev Input parameter.
     * @param[in] next Input parameter.
     * @param[in] epsilon Input parameter.
     * @return True on success.
     */
    static bool hasConverged(const std::vector<float>& prev,
                             const std::vector<float>& next,
                             double epsilon);

    std::deque<std::string> job_queue_;
    std::unordered_map<std::string, TensorTrainingJobSpec> jobs_;
    std::unordered_map<std::string, TensorTrainingJobResult> results_;
    std::unordered_map<std::string, std::shared_ptr<ITensorTrainingWorker>> workers_;
};

}  // namespace themis::distributed_tensor
