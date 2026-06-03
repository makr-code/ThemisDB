/*
 * ThemisDB | File: multi_task_lora.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=2; TODO=0, Stub=2, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Wave B — issue #5039)
 */

/**
 * @file training/multi_task_lora.h
 * @brief Multi-Task LoRA Fine-Tuning (Wave B B3).
 *
 * ## Overview
 *
 * Multi-Task Learning with LoRA (MTL-LoRA) trains a shared low-rank adapter
 * base that captures cross-task knowledge, alongside per-task projection heads
 * that specialise for each downstream task.  A lightweight domain-gating
 * mechanism routes inputs to the correct task head at inference time.
 *
 * Architecture:
 *
 * ```
 *  input
 *    │
 *    ▼
 *  DomainGating ──────────────────────────► task_id (one-of-N)
 *    │
 *    ▼
 *  SharedLoRABase  (rank r, shared across all tasks)
 *    │
 *    ├── TaskHead["task_0"]  (task-specific projection, rank r_task)
 *    ├── TaskHead["task_1"]
 *    └── TaskHead["task_N"]
 *    │
 *    ▼
 *  per-task logits / outputs
 *    │
 *    ▼
 *  JointLoss = Σ  weight[i] * loss[i]
 * ```
 *
 * ## Integration
 *
 * ```cpp
 * MultiTaskLoRAConfig cfg;
 * cfg.shared_rank = 8;
 *
 * MultiTaskLoRATrainer trainer(cfg);
 * trainer.addTask({"qa",       0.4f, 8, 0.001f});
 * trainer.addTask({"summary",  0.3f, 4, 0.001f});
 * trainer.addTask({"classify", 0.3f, 4, 0.001f});
 *
 * auto result = trainer.train(samples);
 * auto best   = trainer.exportSharedWeights();
 * ```
 *
 * ## Acceptance Criteria (issue #5039 B3)
 *
 * - Average task performance ≥ +8% vs. single-task baselines.
 * - Training time increase ≤ 15% vs. single-adapter training.
 * - Robust across task configurations (ablation: shared vs. separate adapters).
 *
 * ## References
 * - Ruder et al. (2015). An overview of multi-task learning in deep neural
 *   networks. arXiv:1506.02246.
 * - Hu et al. (2022). LoRA: Low-Rank Adaptation of Large Language Models.
 *   ICLR 2022. arXiv:2106.09685.
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
 * @brief Aggregated training result for a Multi-Task LoRA run.
 */
struct MTLTrainResult {
    bool                      success         = false;
    double                    joint_loss      = 0.0; ///< Weighted joint loss
    size_t                    epochs_run      = 0;
    std::vector<TaskMetrics>  per_task;
    /// Improvement over single-task baseline (positive = better).
    double                    avg_improvement = 0.0;
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
