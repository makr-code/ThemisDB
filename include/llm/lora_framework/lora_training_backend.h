/**
 * @file lora_training_backend.h
 * @brief Contract for differentiable LoRA/AdaLoRA fine-tuning backends.
 *
 * ThemisDB serves adapters through llama.cpp (GGUF), but llama.cpp b7974 does
 * not provide a production LoRA/AdaLoRA training loop. Real causal-LM
 * fine-tuning (next-token cross-entropy on DB-derived samples) is therefore
 * delegated to a backend implementing this interface. The reference backend
 * (PeftSubprocessTrainingBackend) runs Hugging Face PEFT, whose AdaLoraConfig
 * is the reference implementation of Zhang et al., "AdaLoRA" (ICLR 2023,
 * arXiv:2303.10512), and converts the result to a llama.cpp GGUF LoRA adapter.
 *
 * Ownership: requests and results are value types; backends own no caller
 * state beyond the duration of train().
 */
#pragma once

#include "llm/lora_framework/lora_training_service.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace themis {
namespace llm {
namespace lora {

/// @brief Adapter parameterisation used for a training job.
enum class AdapterTrainingMethod {
    LoRA,     ///< Fixed-rank LoRA (Hu et al. 2021); baseline.
    AdaLoRA   ///< SVD-parameterised LoRA with sensitivity-based rank budget.
};

/**
 * @brief AdaLoRA budget-schedule parameters (paper notation in brackets).
 *
 * The total rank budget starts at init_rank per module [b(0)] and is reduced
 * with a cubic schedule after warmup_steps [t_i] until it reaches target_rank
 * [b(T)]; the final final_steps [t_f] keep the budget fixed.
 */
struct AdaLoRAScheduleParams {
    int init_rank = 12;                 ///< Initial per-module rank (≈1.5× target).
    int target_rank = 8;                ///< Average target rank per module.
    int warmup_steps = 200;             ///< t_i: steps before pruning starts.
    int final_steps = 500;              ///< t_f: steps with fixed final budget.
    int delta_t = 10;                   ///< Steps between budget re-allocations.
    double beta1 = 0.85;                ///< EMA factor for sensitivity.
    double beta2 = 0.85;                ///< EMA factor for uncertainty.
    double orth_reg_weight = 0.5;       ///< γ: orthogonality regulariser weight.
};

/**
 * @brief Fully specified training job.
 *
 * @note samples must contain at least two entries so that a non-empty holdout
 *       split can be produced; backends reject smaller datasets.
 */
struct TrainingJobRequest {
    std::string job_id;                       ///< Orchestrator job identifier.
    std::string adapter_id;                   ///< Target adapter id ([A-Za-z0-9_.-]+).
    TrainingData data;                        ///< DB-derived samples (input/output, optional metadata.context).
    LoRAHyperparameters hyperparameters;      ///< rank/alpha/dropout/lr/epochs/seq-len/batch.
    AdapterTrainingMethod method = AdapterTrainingMethod::AdaLoRA;
    AdaLoRAScheduleParams adalora;            ///< Used only when method == AdaLoRA.
    std::string base_model;                   ///< HF model id or local HF directory for training.
    std::string base_model_gguf;              ///< GGUF base model the adapter must match at serving time.
    std::string output_dir;                   ///< Job-private output directory (created if missing).
    double holdout_fraction = 0.1;            ///< Fraction of samples reserved for evaluation (0,0.5].
    std::uint64_t seed = 42;                  ///< Deterministic split/initialisation seed.
    bool require_improvement = true;          ///< Fail job if adapter eval loss >= base-model eval loss.
};

/// @brief Progress event emitted by a backend during train().
struct TrainingProgress {
    float progress = 0.0f;        ///< 0..1
    int step = 0;
    int total_steps = 0;
    double train_loss = 0.0;      ///< Mean next-token cross-entropy of the last logging window.
    double eval_loss = -1.0;      ///< Holdout loss if evaluated at this step, otherwise < 0.
    std::string phase;            ///< "prepare" | "train" | "evaluate" | "export" | "done"
};

/// @brief Outcome of a training job. All metrics are measured, never estimated.
struct TrainingJobResult {
    bool success = false;
    std::string error;                  ///< Non-empty iff !success.
    std::string gguf_adapter_path;      ///< llama.cpp-loadable LoRA adapter (GGUF).
    std::string peft_adapter_dir;       ///< Raw PEFT checkpoint directory.
    double train_loss = 0.0;            ///< Final mean training cross-entropy.
    double eval_loss = 0.0;             ///< Holdout cross-entropy with adapter.
    double eval_perplexity = 0.0;       ///< exp(eval_loss).
    double baseline_eval_loss = 0.0;    ///< Holdout cross-entropy of the base model (adapter disabled).
    std::size_t train_samples = 0;
    std::size_t eval_samples = 0;
    nlohmann::json rank_pattern;        ///< AdaLoRA final per-module ranks (empty for LoRA).
    nlohmann::json raw_report;          ///< Backend-specific full report.
};

/**
 * @brief Interface of a differentiable LoRA/AdaLoRA training backend.
 *
 * @note Implementations must be safe to call concurrently for distinct
 *       request.output_dir values.
 */
class ILoRATrainingBackend {
public:
    using ProgressCallback = std::function<void(const TrainingProgress&)>;

    virtual ~ILoRATrainingBackend() = default;

    /**
     * @brief Run a blocking fine-tuning job.
     * @param request Job specification; validated by the backend.
     * @param on_progress Optional progress sink (may be empty); invoked from the calling thread.
     * @param cancel Cooperative cancellation flag; when it becomes true the backend
     *        terminates the job as soon as possible and returns success=false with
     *        error "cancelled".
     * @return Measured result. Never throws for job-level failures; these are
     *         reported via success=false and error.
     */
    [[nodiscard]] virtual TrainingJobResult train(const TrainingJobRequest& request,
                                                  const ProgressCallback& on_progress,
                                                  const std::atomic<bool>& cancel) = 0;

    /// @brief Human-readable backend identifier (e.g. "peft-subprocess").
    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
