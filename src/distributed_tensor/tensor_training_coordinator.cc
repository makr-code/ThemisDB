#include "distributed_tensor/tensor_training_coordinator.h"

#include <algorithm>
#include <cmath>

namespace themis::distributed_tensor {

namespace {

bool hasConsistentShardDimensions(const std::vector<std::vector<float>>& shard_outputs) {
    if (shard_outputs.empty()) {
        return true;
    }
    const std::size_t dim = shard_outputs.front().size();
    for (const auto& shard : shard_outputs) {
        if (shard.size() != dim) {
            return false;
        }
    }
    return true;
}

}  // namespace

void TensorTrainingCoordinator::registerWorker(const std::string& node_id,
                                               std::shared_ptr<ITensorTrainingWorker> worker) {
    if (node_id.empty() || !worker) {
        return;
    }
    workers_[node_id] = std::move(worker);
}

bool TensorTrainingCoordinator::submitJob(const TensorTrainingJobSpec& spec) {
    if (spec.job_id.empty() || spec.shard_work.empty() || jobs_.count(spec.job_id) != 0) {
        return false;
    }

    jobs_[spec.job_id] = spec;
    job_queue_.push_back(spec.job_id);

    TensorTrainingJobResult result;
    result.job_id = spec.job_id;
    result.state = TrainingJobState::QUEUED;
    results_[spec.job_id] = result;
    return true;
}

bool TensorTrainingCoordinator::cancelJob(const std::string& job_id) {
    auto it = results_.find(job_id);
    if (it == results_.end()) {
        return false;
    }

    it->second.state = TrainingJobState::CANCELLED;
    return true;
}

bool TensorTrainingCoordinator::runNextJob() {
    if (job_queue_.empty() || workers_.empty()) {
        return false;
    }

    const std::string job_id = job_queue_.front();
    job_queue_.pop_front();

    auto result_it = results_.find(job_id);
    auto job_it = jobs_.find(job_id);
    if (result_it == results_.end() || job_it == jobs_.end()) {
        return false;
    }

    auto& result = result_it->second;
    const auto& spec = job_it->second;

    if (result.state == TrainingJobState::CANCELLED) {
        return true;
    }

    result.state = TrainingJobState::RUNNING;

    std::vector<float> previous_aggregate = {};

    for (std::size_t iter = 0; iter < spec.max_iterations; ++iter) {
        std::vector<std::vector<float>> shard_outputs;
        shard_outputs.reserve(spec.shard_work.size());

        for (const auto& shard : spec.shard_work) {
            auto shard_result = runShardWithRetry(spec, shard, result);
            if (!shard_result) {
                result.state = TrainingJobState::FAILED;
                if (result.error_message.empty()) {
                    result.error_message = "worker execution failed";
                }
                return true;
            }
            shard_outputs.push_back(std::move(*shard_result));
        }

        if (!hasConsistentShardDimensions(shard_outputs)) {
            result.state = TrainingJobState::FAILED;
            result.error_message = "shard output dimension mismatch";
            return true;
        }
        auto aggregated = aggregateShardResults(shard_outputs);
        result.iterations = iter + 1;
        if (!previous_aggregate.empty() && hasConverged(previous_aggregate, aggregated, spec.convergence_epsilon)) {
            result.aggregated_tensor = std::move(aggregated);
            result.state = TrainingJobState::SUCCESS;
            return true;
        }

        previous_aggregate = std::move(aggregated);
    }

    result.aggregated_tensor = std::move(previous_aggregate);
    result.state = TrainingJobState::SUCCESS;
    return true;
}

std::optional<TensorTrainingJobResult>
TensorTrainingCoordinator::result(const std::string& job_id) const {
    auto it = results_.find(job_id);
    if (it == results_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::vector<float>>
TensorTrainingCoordinator::runShardWithRetry(const TensorTrainingJobSpec& spec,
                                             const TensorShardWorkItem& shard,
                                             TensorTrainingJobResult& result) {
    if (workers_.empty()) {
        result.error_message = "no workers registered";
        return std::nullopt;
    }

    std::size_t attempt = 0;
    const std::size_t node_count = workers_.size();
    auto worker_it = workers_.begin();

    while (attempt <= spec.max_retries) {
        if (worker_it == workers_.end()) {
            worker_it = workers_.begin();
        }

        std::string err = {};
        auto output = worker_it->second->processShard(spec.job_id, worker_it->first, shard, err);
        if (output) {
            return output;
        }

        result.state = TrainingJobState::RETRY;
        result.retries++;
        result.error_message = err;
        ++attempt;
        if (node_count > 1) {
            ++worker_it;
        }
    }

    return std::nullopt;
}

std::vector<float> TensorTrainingCoordinator::aggregateShardResults(
    const std::vector<std::vector<float>>& shard_results) {
    if (shard_results.empty()) {
        return {};
    }

    const std::size_t dim = shard_results.front().size();
    std::vector<float> aggregate(dim, 0.0f);

    for (const auto& shard : shard_results) {
        if (shard.size() != dim) {
            return {};
        }
        for (std::size_t i = 0; i < dim; ++i) {
            aggregate[i] += shard[i];
        }
    }

    for (float& v : aggregate) {
        v /= static_cast<float>(shard_results.size());
    }

    return aggregate;
}

bool TensorTrainingCoordinator::hasConverged(const std::vector<float>& prev,
                                             const std::vector<float>& next,
                                             double epsilon) {
    if (prev.size() != next.size()) {
        return false;
    }

    double max_delta = 0.0;
    for (std::size_t i = 0; i < prev.size(); ++i) {
        max_delta = std::max(max_delta, std::fabs(static_cast<double>(prev[i]) - static_cast<double>(next[i])));
    }
    return max_delta <= epsilon;
}

}  // namespace themis::distributed_tensor
