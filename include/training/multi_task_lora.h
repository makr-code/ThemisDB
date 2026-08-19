/**
 * @file multi_task_lora.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace training {

// ============================================================================
// Task configuration
// ============================================================================

/**
 * @brief Per-task configuration for Multi-Task LoRA training.
 * 
 * QW-41: Configuration hardening with fail-closed guards:
 *   - id: must be non-empty (unique task identifier)
 *   - task_rank: must be >= 1 (LoRA rank cannot be zero)
 *   - learning_rate: must be > 0 (strictly positive)
 *   - loss_weight: must be >= 0 (non-negative for joint loss)
 * All guards are enforced in MultiTaskLoRATrainer::addTask() by throwing
 * std::invalid_argument on violation.
 */
struct TaskConfig {
    std::string id;                ///< Unique task identifier (QW-41: non-empty guard)
    float       loss_weight = 1.0f; ///< Relative loss weight in joint loss (QW-41: >= 0 guard)
    size_t      task_rank   = 4;    ///< Task-specific head LoRA rank (QW-41: >= 1 guard)
    float       learning_rate = 1e-3f; ///< Per-task learning rate override (QW-41: > 0 guard)
};

// ============================================================================
// Training data
// ============================================================================

/**
 * @brief A single multi-task training sample.
 */
struct MTLSample {
    std::string task_id;             ///< Which task this sample belongs to
    std::vector<float> input;        ///< Input feature vector
    std::vector<float> target;       ///< Target output vector (or one-hot label)
    float              weight = 1.0f; ///< Per-sample importance weight
};

// ============================================================================
// Domain gating
// ============================================================================

/**
 * @brief Result of the domain-gating classification step.
 */
struct DomainGatingResult {
    std::string task_id;         ///< Predicted task identifier
    float       confidence = 0.0f; ///< Gating confidence in [0, 1]
    std::vector<std::pair<std::string, float>> scores; ///< (task_id, score) for all tasks
};

// ============================================================================
// Training result
// ============================================================================

/**
 * @brief Per-task training metrics reported after each epoch.
 */
struct TaskMetrics {
    std::string task_id;
    double      train_loss  = 0.0;
    double      accuracy    = 0.0;
    size_t      num_samples = 0;
};

/**
 * @brief Wave B Acceptance Gate Metrics for Multi-Task LoRA
 * 
 * Tracks performance gates required for Wave B (Q1-Q2 2027):
 *  - avg_perf_gain: Average task performance gain vs single-task baseline (target: ≥ +8%)
 *  - training_time_overhead: Training time increase vs aggregated per-task single-task baselines (target: ≤ 15%)
 *  - task_routing_latency_ms: Average task routing latency (target: ≤ 10ms)
 *  - convergence_stable: Whether training converged stably across epochs
 */
struct AcceptanceGateMetrics {
    double avg_perf_gain         = 0.0;   ///< Avg performance gain ≥ +8% (Wave B gate)
    double training_time_overhead = 0.0;  ///< Training time overhead vs aggregated per-task baselines ≤ 15%
    double task_routing_latency_ms = 0.0; ///< Task routing latency ≤ 10ms (Wave B gate)
    bool   convergence_stable    = false; ///< Convergence validated across epochs
    size_t convergence_epochs    = 0;     ///< Epochs to reach stable loss
};

/**
 * @brief Aggregated training result for a Multi-Task LoRA run.
 */
struct MTLTrainResult {
    bool                      success         = false;
    double                    joint_loss      = 0.0; ///< Weighted joint loss
    size_t                    epochs_run      = 0;
    std::vector<TaskMetrics>  per_task;
    /// Improvement over single-task baseline (positive = better).
    double                    avg_improvement = 0.0;
    /// Wave B Acceptance gate metrics (Phase 5)
    AcceptanceGateMetrics     acceptance_gates;
};

// ============================================================================
// Multi-Task LoRA configuration
// ============================================================================

/**
 * @brief Top-level configuration for MultiTaskLoRATrainer.
 */
struct MultiTaskLoRAConfig {
    size_t shared_rank    = 8;    ///< Rank of the shared LoRA base adapter
    size_t epochs         = 20;   ///< Training epochs
    size_t batch_size     = 32;   ///< Samples per gradient step
    float  learning_rate  = 1e-3f; ///< Global base learning rate
    float  warmup_frac    = 0.1f;  ///< Fraction of steps used for LR warmup

    /// Input feature dimension (inferred from first sample if 0).
    size_t input_dim      = 0;

    /// Gating confidence threshold below which the trainer falls back to the
    /// highest-weight task rather than the gating prediction.
    float  gating_fallback_threshold = 0.2f;
};

// ============================================================================
// MultiTaskLoRATrainer
// ============================================================================

/**
 * @brief Trains a shared LoRA base with task-specific projection heads.
 *
 * Thread-safety: `MultiTaskLoRATrainer` instances are **not** thread-safe.
 * Each training thread should own its own instance.
 */
class MultiTaskLoRATrainer {
public:
    explicit MultiTaskLoRATrainer(MultiTaskLoRAConfig cfg = {});
    ~MultiTaskLoRATrainer();

    // ------------------------------------------------------------------
    // Task registration
    // ------------------------------------------------------------------

    /**
     * @brief Register a task.
     *
     * Must be called before `train()`.  Duplicate task ids are silently
     * ignored (first registration wins).
     * 
     * @param task Task configuration.
     * @throws std::invalid_argument if any configuration guard fails (QW-41):
     *   - if task.id is empty
     *   - if task.task_rank is 0 (must be >= 1)
     *   - if task.learning_rate <= 0 (must be > 0)
     *   - if task.loss_weight < 0 (must be >= 0)
     * @note Fail-closed: On guard failure, exception is thrown and task is not added.
     *       Calling code must handle the exception; task registration fails atomically.
     */
    void addTask(const TaskConfig& task);

    /// Return the number of registered tasks.
    size_t taskCount() const;

    // ------------------------------------------------------------------
    // Training
    // ------------------------------------------------------------------

    /**
     * @brief Run multi-task LoRA training on the supplied samples.
     *
     * @param samples  Training samples (from one or more tasks).
     * @return         Training result with per-task metrics.
     * @throws std::runtime_error if no tasks registered or no samples supplied.
     */
    MTLTrainResult train(const std::vector<MTLSample>& samples);

    // ------------------------------------------------------------------
    // Inference
    // ------------------------------------------------------------------

    /**
     * @brief Run domain-gating to predict which task an input belongs to.
     *
     * Uses a learned (or heuristic) gate trained during `train()`.
     *
     * @param input Input feature vector.
     * @return Gating result with predicted task and confidence.
     * @throws std::runtime_error if model has not been trained.
     */
    DomainGatingResult inferTask(const std::vector<float>& input) const;

    /**
     * @brief Run the shared base + predicted task head on an input.
     *
     * @param input Input feature vector.
     * @return Output from the gated task head.
     */
    std::vector<float> forward(const std::vector<float>& input) const;

    // ------------------------------------------------------------------
    // Export
    // ------------------------------------------------------------------

    /**
     * @brief Export the shared LoRA base weight matrix (B × A product).
     *
     * @return Flat row-major weight matrix of size (input_dim × shared_rank).
     *         Returns empty vector if not trained.
     */
    std::vector<float> exportSharedWeights() const;

    /**
     * @brief Export the task-specific head weights for a given task.
     *
     * @param task_id  Task identifier.
     * @return Flat row-major weight matrix of size (shared_rank × output_dim).
     *         Returns empty vector if not trained or unknown task.
     */
    std::vector<float> exportTaskWeights(const std::string& task_id) const;

    // ------------------------------------------------------------------
    // Wave B Acceptance Gates (Phase 5)
    // ------------------------------------------------------------------

    /**
     * @brief Validate acceptance gates for Wave B deployment.
     *
     * Runs acceptance gate validation:
     *  - Average task performance gain ≥ +8% vs single-task baseline
     *  - Training-time increase ≤ 15% across benchmarked task sets
     *  - Task routing latency ≤ 10ms
     *  - Convergence stability across configured task-weight schedules
     *
     * @return AcceptanceGateMetrics with measured values and validation status.
     * @throws std::runtime_error if model has not been trained.
     */
    AcceptanceGateMetrics validateAcceptanceGates() const;

    /**
     * @brief Run a three-task transfer evaluation benchmark for Wave B.
     *
     * Implements the Wave B benchmark harness:
     *  - Create three synthetic tasks (semantic similarity task, sentiment, QA)
     *  - Measure baseline single-task performance
     *  - Measure multi-task performance with shared base
     *  - Calculate task interference and gating effectiveness
     *
     * @param num_samples Number of samples per task for evaluation.
     * @return MTLTrainResult with comprehensive metrics and gates.
     */
    MTLTrainResult benchmarkThreeTaskTransfer(size_t num_samples = 100);

    /**
     * @brief Run ablation study comparing shared multi-task training vs per-task single-task baselines.
     *
     * Compares two training configurations:
     *  - Shared base with per-task heads (current implementation)
     *  - One independently trained single-task model per task, aggregated as the baseline
     *
     * @param samples Training samples for evaluation.
     * @return Pair of (shared_result, baseline_result) for comparison.
     */
    std::pair<MTLTrainResult, MTLTrainResult> runAblationStudy(
       const std::vector<MTLSample>& samples);

    // ------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------

    const MultiTaskLoRAConfig& config() const noexcept { return cfg_; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    MultiTaskLoRAConfig   cfg_;
};

} // namespace training
} // namespace themis
