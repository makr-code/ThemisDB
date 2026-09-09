/**
 * @file tensor_workflow_observability.cpp
 * @brief Tensor workflow observability implementation.
 *
 * Implements span creation, attribute attachment, and metric
 * recording for tensor pipeline observability.
 */

#include "tensor/tensor_workflow_observability.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace themis::tensor {

void TensorWorkflowObservability::recordPersistenceOp(double latency_ms, bool success) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++persistence_total_;
    if (!success) {
        ++persistence_errors_;
    }
    persistence_latency_ms_.push_back(latency_ms);
}

void TensorWorkflowObservability::recordTrainingTransition(TrainingState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    switch (state) {
        case TrainingState::Queued:
            ++training_queued_;
            break;
        case TrainingState::Running:
            ++training_running_;
            break;
        case TrainingState::Success:
            ++training_success_;
            break;
        case TrainingState::Failed:
            ++training_failed_;
            break;
        case TrainingState::Retry:
            ++training_retry_;
            break;
        case TrainingState::Cancelled:
            ++training_cancelled_;
            break;
    }
}

void TensorWorkflowObservability::recordTrainingLatency(double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    training_latency_ms_.push_back(latency_ms);
}

void TensorWorkflowObservability::recordGpuDispatch(bool used_gpu, bool used_fallback, bool error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (used_gpu) {
        ++gpu_used_;
    }
    if (used_fallback) {
        ++gpu_fallback_;
    }
    if (error) {
        ++gpu_errors_;
    }
}

void TensorWorkflowObservability::recordCompressionLatency(double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    compression_latency_ms_.push_back(latency_ms);
}

void TensorWorkflowObservability::recordRoutingLatency(double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    routing_latency_ms_.push_back(latency_ms);
}

std::string TensorWorkflowObservability::exportPrometheusText() const {
    std::size_t persistence_total = 0;
    std::size_t persistence_errors = 0;
    std::size_t training_queued = 0;
    std::size_t training_running = 0;
    std::size_t training_success = 0;
    std::size_t training_failed = 0;
    std::size_t training_retry = 0;
    std::size_t training_cancelled = 0;
    std::size_t gpu_used = 0;
    std::size_t gpu_fallback = 0;
    std::size_t gpu_errors = 0;
    std::vector<double> persistence_latency_ms;
    std::vector<double> training_latency_ms;
    std::vector<double> compression_latency_ms;
    std::vector<double> routing_latency_ms;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        persistence_total = persistence_total_;
        persistence_errors = persistence_errors_;
        training_queued = training_queued_;
        training_running = training_running_;
        training_success = training_success_;
        training_failed = training_failed_;
        training_retry = training_retry_;
        training_cancelled = training_cancelled_;
        gpu_used = gpu_used_;
        gpu_fallback = gpu_fallback_;
        gpu_errors = gpu_errors_;
        persistence_latency_ms = persistence_latency_ms_;
        training_latency_ms = training_latency_ms_;
        compression_latency_ms = compression_latency_ms_;
        routing_latency_ms = routing_latency_ms_;
    }

    std::ostringstream out;
    out << "tensor_persistence_ops_total " << persistence_total << "\n";
    out << "tensor_persistence_errors_total " << persistence_errors << "\n";
    out << "tensor_training_jobs_queued_total " << training_queued << "\n";
    out << "tensor_training_jobs_running_total " << training_running << "\n";
    out << "tensor_training_jobs_success_total " << training_success << "\n";
    out << "tensor_training_jobs_failed_total " << training_failed << "\n";
    out << "tensor_training_jobs_retry_total " << training_retry << "\n";
    out << "tensor_training_jobs_cancelled_total " << training_cancelled << "\n";
    out << "tensor_gpu_dispatch_total " << gpu_used << "\n";
    out << "tensor_gpu_fallback_total " << gpu_fallback << "\n";
    out << "tensor_gpu_errors_total " << gpu_errors << "\n";

    out << "tensor_persistence_latency_p95_ms " << percentile95(std::move(persistence_latency_ms)) << "\n";
    out << "tensor_training_latency_p95_ms " << percentile95(std::move(training_latency_ms)) << "\n";
    out << "tensor_compression_latency_p95_ms " << percentile95(std::move(compression_latency_ms)) << "\n";
    out << "tensor_routing_latency_p95_ms " << percentile95(std::move(routing_latency_ms)) << "\n";
    return out.str();
}

TensorWorkflowHealthSummary
TensorWorkflowObservability::evaluateSlo(const TensorWorkflowSloConfig& cfg) const {
    std::size_t persistence_total = 0;
    std::size_t training_success = 0;
    std::size_t training_failed = 0;
    std::size_t gpu_used = 0;
    std::size_t gpu_fallback = 0;
    std::size_t persistence_errors = 0;
    std::size_t gpu_errors = 0;
    std::vector<double> persistence_latency_ms;
    std::vector<double> training_latency_ms;
    std::vector<double> compression_latency_ms;
    std::vector<double> routing_latency_ms;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        persistence_total = persistence_total_;
        training_success = training_success_;
        training_failed = training_failed_;
        gpu_used = gpu_used_;
        gpu_fallback = gpu_fallback_;
        persistence_errors = persistence_errors_;
        gpu_errors = gpu_errors_;
        persistence_latency_ms = persistence_latency_ms_;
        training_latency_ms = training_latency_ms_;
        compression_latency_ms = compression_latency_ms_;
        routing_latency_ms = routing_latency_ms_;
    }

    TensorWorkflowHealthSummary summary;
    summary.p95_latency_ms["persistence"] = percentile95(std::move(persistence_latency_ms));
    summary.p95_latency_ms["training"] = percentile95(std::move(training_latency_ms));
    summary.p95_latency_ms["compression"] = percentile95(std::move(compression_latency_ms));
    summary.p95_latency_ms["routing"] = percentile95(std::move(routing_latency_ms));

    const std::size_t total_ops = persistence_total + training_success + training_failed + gpu_used + gpu_fallback;
    const std::size_t total_errors = persistence_errors + training_failed + gpu_errors;
    summary.error_rate = total_ops == 0 ? 0.0 : static_cast<double>(total_errors) / static_cast<double>(total_ops);
    summary.error_budget_burn = cfg.max_error_rate > 0.0 ? summary.error_rate / cfg.max_error_rate : 0.0;

    auto add_latency_violation = [&](const std::string& metric, double value, double limit) {
        if (value > limit) {
            summary.healthy = false;
            summary.violations.push_back(metric + " p95 exceeded");
        }
    };

    add_latency_violation("persistence", summary.p95_latency_ms["persistence"], cfg.max_p95_persistence_ms);
    add_latency_violation("training", summary.p95_latency_ms["training"], cfg.max_p95_training_ms);
    add_latency_violation("compression", summary.p95_latency_ms["compression"], cfg.max_p95_compression_ms);
    add_latency_violation("routing", summary.p95_latency_ms["routing"], cfg.max_p95_routing_ms);

    if (summary.error_rate > cfg.max_error_rate) {
        summary.healthy = false;
        summary.violations.push_back("error rate exceeded");
    }
    if (summary.error_budget_burn > cfg.max_error_budget_burn) {
        summary.healthy = false;
        summary.violations.push_back("error budget burn exceeded");
    }

    return summary;
}

double TensorWorkflowObservability::percentile95(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const std::size_t idx = static_cast<std::size_t>(std::floor(0.95 * static_cast<double>(static_cast<int>(values.size()) - 1)));
    return values[idx];
}

}  // namespace themis::tensor
