/**
 * @file tensor_workflow_observability.h
 * @brief Observability hooks for the tensor processing workflow.
 *
 * Declares span factories and metric helpers that instrument tensor
 * pipeline stages with OpenTelemetry traces and counters.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis::tensor {

struct TensorWorkflowSloConfig {
    double max_p95_persistence_ms = 10.0;
    double max_p95_training_ms = 250.0;
    double max_p95_compression_ms = 25.0;
    double max_p95_routing_ms = 10.0;
    double max_error_rate = 0.01;
    double max_error_budget_burn = 1.0;
};

struct TensorWorkflowHealthSummary {
    bool healthy = true;
    double error_rate = 0.0;
    double error_budget_burn = 0.0;
    std::map<std::string, double> p95_latency_ms;
    std::vector<std::string> violations;
};

/**
 * @brief Prometheus-oriented metrics and SLO tracker for tensor phase 5-8 workflows.
 */
class TensorWorkflowObservability {
public:
    enum class TrainingState {
        Queued,
        Running,
        Success,
        Failed,
        Retry,
        Cancelled,
    };

    void recordPersistenceOp(double latency_ms, bool success);
    void recordTrainingTransition(TrainingState state);
    void recordTrainingLatency(double latency_ms);
    void recordGpuDispatch(bool used_gpu, bool used_fallback, bool error);
    void recordCompressionLatency(double latency_ms);
    void recordRoutingLatency(double latency_ms);

    [[nodiscard]] std::string exportPrometheusText() const;
    [[nodiscard]] TensorWorkflowHealthSummary evaluateSlo(const TensorWorkflowSloConfig& cfg) const;

private:
    static double percentile95(std::vector<double> values);

    mutable std::mutex mutex_;

    std::size_t persistence_total_ = 0;
    std::size_t persistence_errors_ = 0;
    std::vector<double> persistence_latency_ms_;

    std::size_t training_queued_ = 0;
    std::size_t training_running_ = 0;
    std::size_t training_success_ = 0;
    std::size_t training_failed_ = 0;
    std::size_t training_retry_ = 0;
    std::size_t training_cancelled_ = 0;
    std::vector<double> training_latency_ms_;

    std::size_t gpu_used_ = 0;
    std::size_t gpu_fallback_ = 0;
    std::size_t gpu_errors_ = 0;

    std::vector<double> compression_latency_ms_;
    std::vector<double> routing_latency_ms_;
};

}  // namespace themis::tensor
