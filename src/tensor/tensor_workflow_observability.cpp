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
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream out;
    out << "tensor_persistence_ops_total " << persistence_total_ << "\n";
    out << "tensor_persistence_errors_total " << persistence_errors_ << "\n";
    out << "tensor_training_jobs_queued_total " << training_queued_ << "\n";
    out << "tensor_training_jobs_running_total " << training_running_ << "\n";
    out << "tensor_training_jobs_success_total " << training_success_ << "\n";
    out << "tensor_training_jobs_failed_total " << training_failed_ << "\n";
    out << "tensor_training_jobs_retry_total " << training_retry_ << "\n";
    out << "tensor_training_jobs_cancelled_total " << training_cancelled_ << "\n";
    out << "tensor_gpu_dispatch_total " << gpu_used_ << "\n";
    out << "tensor_gpu_fallback_total " << gpu_fallback_ << "\n";
    out << "tensor_gpu_errors_total " << gpu_errors_ << "\n";

    out << "tensor_compression_latency_p95_ms " << percentile95(compression_latency_ms_) << "\n";
    out << "tensor_routing_latency_p95_ms " << percentile95(routing_latency_ms_) << "\n";
    out << "tensor_persistence_latency_p95_ms " << percentile95(persistence_latency_ms_) << "\n";
    return out.str();
}

TensorWorkflowHealthSummary
TensorWorkflowObservability::evaluateSlo(const TensorWorkflowSloConfig& cfg) const {
    std::lock_guard<std::mutex> lock(mutex_);

    TensorWorkflowHealthSummary summary;
    summary.p95_latency_ms["persistence"] = percentile95(persistence_latency_ms_);
    summary.p95_latency_ms["training"] = 0.0;
    summary.p95_latency_ms["compression"] = percentile95(compression_latency_ms_);
    summary.p95_latency_ms["routing"] = percentile95(routing_latency_ms_);

    const std::size_t total_ops = persistence_total_ + training_success_ + training_failed_ + gpu_used_ + gpu_fallback_;
    const std::size_t total_errors = persistence_errors_ + training_failed_ + gpu_errors_;
    summary.error_rate = total_ops == 0 ? 0.0 : static_cast<double>(total_errors) / static_cast<double>(total_ops);
    summary.error_budget_burn = cfg.max_error_rate > 0.0 ? summary.error_rate / cfg.max_error_rate : 0.0;

    auto add_latency_violation = [&](const std::string& metric, double value, double limit) {
        if (value > limit) {
            summary.healthy = false;
            summary.violations.push_back(metric + " p95 exceeded");
        }
    };

    add_latency_violation("persistence", summary.p95_latency_ms["persistence"], cfg.max_p95_persistence_ms);
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
    const std::size_t idx = static_cast<std::size_t>(std::floor(0.95 * static_cast<double>(values.size() - 1)));
    return values[idx];
}

}  // namespace themis::tensor
