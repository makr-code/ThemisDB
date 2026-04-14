/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            incremental_lora_trainer.h                         ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:44:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     389                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ac63c2ec8d  2026-04-12  [WIP] Update developer documentation for module training ... ║
    • e25b25ef58  2026-03-24  Changes before error encountered        ║
    • 334ca1434e  2026-03-11  fix: selectAdapterForRequest traffic routing; DocsAssista... ║
    • 495594752a  2026-03-11  feat(training): add quantization, multi-GPU, metrics trac... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "training/adapter_serving.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <limits>

namespace themis {
namespace training {

/**
 * @brief Training mode for incremental trainer
 */
enum class TrainingMode {
    INITIAL,      ///< Initial training from scratch
    INCREMENTAL,  ///< Incremental training on new data
    FINETUNE      ///< Fine-tune existing adapter
};

// ============================================================================
// Quantization support
// ============================================================================

/**
 * @brief Quantization type for LoRA weight compression during training.
 *
 * Mirrors llm::lora::QuantizationType but is defined here to avoid a
 * compile-time dependency on the LLM layer when THEMIS_ENABLE_LLM is off.
 */
enum class TrainingQuantizationType {
    NONE,   ///< Full-precision (fp32) – no quantization
    FP16,   ///< 16-bit floating point
    INT8,   ///< 8-bit integer (symmetric per-block)
    NF4     ///< 4-bit NormalFloat (QLoRA)
};

/**
 * @brief Quantization settings applied during LoRA training.
 *
 * When @c type is not NONE the base-model weights are quantized before the
 * LoRA adapters are applied.  This reduces GPU memory requirements and enables
 * QLoRA-style training on consumer hardware.
 */
struct QuantizationConfig {
    TrainingQuantizationType type = TrainingQuantizationType::NONE;
    int block_size = 64;  ///< Quantization block size (weights per quantization group)

    QuantizationConfig() = default;
    explicit QuantizationConfig(TrainingQuantizationType t, int bs = 64)
        : type(t), block_size(bs) {}
};

// ============================================================================
// Per-step / per-epoch training metrics
// ============================================================================

/**
 * @brief Aggregated metrics for a single training epoch.
 */
struct EpochMetrics {
    size_t epoch          = 0;
    size_t steps          = 0;
    double train_loss     = 0.0;   ///< Mean training loss over all steps in this epoch
    double val_loss       = 0.0;   ///< Validation loss at epoch end (0 if not evaluated)
    double accuracy       = 0.0;   ///< Accuracy at epoch end (0 if not evaluated)
    double learning_rate  = 0.0;   ///< Effective learning rate used in this epoch
    double elapsed_seconds = 0.0;  ///< Wall-clock time for this epoch
    std::chrono::system_clock::time_point timestamp;

    EpochMetrics() = default;
};

/**
 * @brief Full set of training metrics accumulated across all epochs.
 *
 * Obtained via IncrementalLoRATrainer::getMetrics() after train() completes.
 */
struct TrainingMetrics {
    std::vector<EpochMetrics>  epoch_metrics;     ///< One entry per epoch
    std::vector<double>        step_losses;       ///< Loss at every training step (all epochs)
    size_t total_steps    = 0;
    size_t total_epochs   = 0;
    double best_train_loss = std::numeric_limits<double>::max();
    double best_val_loss   = std::numeric_limits<double>::max();
    double total_elapsed_seconds = 0.0;

    TrainingMetrics() = default;

    /** @brief Reset all accumulated metrics. */
    void reset() {
        epoch_metrics.clear();
        step_losses.clear();
        total_steps   = 0;
        total_epochs  = 0;
        best_train_loss = std::numeric_limits<double>::max();
        best_val_loss   = std::numeric_limits<double>::max();
        total_elapsed_seconds = 0.0;
    }
};

/**
 * @brief Training result
 */
struct TrainingResult {
    bool success = false;
    std::string version;              ///< Version identifier (e.g., "legal_v1.1")
    std::string adapter_id;           ///< Adapter ID in storage
    double training_loss = 0.0;
    double validation_loss = 0.0;
    double accuracy = 0.0;
    size_t samples_trained = 0;
    double training_time_seconds = 0.0;
    std::string error_message;
    int gpus_used = 1;                ///< Number of GPUs used during training
    
    TrainingResult() = default;
};

/**
 * @brief Training progress callback
 */
using TrainingCallback = std::function<void(size_t epoch,
                                            size_t step,
                                            double loss,
                                            const std::string& status)>;

/**
 * @brief Configuration for incremental LoRA training
 */
struct IncrementalTrainingConfig {
    std::string training_data_collection; ///< Collection with training samples
    std::string base_model_path;          ///< Path to base model (GGUF)
    std::string adapter_version;          ///< Starting adapter version (empty = new)
    
    // LoRA hyperparameters
    int rank = 8;                         ///< LoRA rank
    float alpha = 16.0f;                  ///< LoRA alpha
    float dropout = 0.1f;                 ///< Dropout rate
    float learning_rate = 0.0003f;        ///< Learning rate
    
    // Training parameters
    size_t batch_size = 4;
    size_t num_epochs = 3;
    size_t max_seq_length = 512;
    float validation_split = 0.1f;
    
    // Incremental training
    bool use_existing_adapter = false;    ///< Start from existing adapter
    bool freeze_existing_layers = false;  ///< Freeze already-trained layers
    size_t incremental_steps = 1000;      ///< Steps for incremental update
    
    // Device
    std::string device = "cuda";          ///< Device: cuda, cpu, mps, hip
    int device_id = 0;

    // Multi-GPU distributed training
    int num_gpus = 1;                     ///< Number of GPUs to use (1 = single-GPU)
    std::vector<int> gpu_ids;             ///< Explicit GPU device IDs (empty = first N GPUs)
    int sync_steps = 1;                   ///< Gradient-sync interval (steps between all-reduce)

    // Quantization (QLoRA)
    QuantizationConfig quantization;      ///< Quantization settings (default: NONE = full precision)

    // Checkpoint directory (non-empty enables LoRACheckpointManager integration)
    std::string checkpoint_dir;           ///< Directory for checkpoint files

    // LoRA+ asymmetric learning rate (Hayou et al., 2024)
    // When > 1.0, B matrices use lr * lora_plus_lambda and A matrices use lr.
    // This yields faster convergence and better downstream task performance.
    // Set to 1.0 (default) to use standard LoRA (single learning rate for A and B).
    float lora_plus_lambda = 1.0f;        ///< LoRA+ λ ratio (B lr = lr * λ, A lr = lr)

    IncrementalTrainingConfig() = default;
};

/**
 * @brief Incremental LoRA trainer
 * 
 * Supports continuous learning with incremental updates to LoRA adapters.
 * Enables training new data without full retraining from scratch.
 * 
 * Features:
 * - Initial training: Creates new LoRA adapter from base model
 * - Incremental training: Updates existing adapter with new data
 * - Version management: Tracks adapter versions (v1 → v1.1 → v1.2)
 * - A/B testing support: Deploy new versions gradually
 * 
 * Example usage:
 * @code
 * IncrementalTrainingConfig config;
 * config.training_data_collection = "legal_training_samples";
 * config.base_model_path = "models/llama-2-7b-chat.gguf";
 * config.adapter_version = "";  // Start fresh
 * config.rank = 8;
 * config.num_epochs = 3;
 * 
 * IncrementalLoRATrainer trainer(config, db);
 * 
 * // Initial training
 * auto result = trainer.train(TrainingMode::INITIAL);
 * std::cout << "Trained adapter: " << result.version << "\n";
 * 
 * // Later: Incremental update with new data
 * config.adapter_version = result.version;
 * config.use_existing_adapter = true;
 * auto result2 = trainer.train(TrainingMode::INCREMENTAL);
 * std::cout << "Updated to: " << result2.version << "\n";
 * @endcode
 */
class IncrementalLoRATrainer {
public:
    /**
     * @brief Construct incremental trainer
     * @param config Training configuration
     * @param db_connection Database connection string
     */
    explicit IncrementalLoRATrainer(const IncrementalTrainingConfig& config,
                                    const std::string& db_connection);
    
    ~IncrementalLoRATrainer();
    
    // Delete copy
    IncrementalLoRATrainer(const IncrementalLoRATrainer&) = delete;
    IncrementalLoRATrainer& operator=(const IncrementalLoRATrainer&) = delete;
    
    /**
     * @brief Train LoRA adapter
     * @param mode Training mode (INITIAL/INCREMENTAL/FINETUNE)
     * @param callback Optional progress callback
     * @return Training result with version info
     */
    TrainingResult train(TrainingMode mode = TrainingMode::INITIAL,
                        TrainingCallback callback = nullptr);
    
    /**
     * @brief Resume training from checkpoint
     * @param checkpoint_path Path to checkpoint file
     * @param callback Optional progress callback
     * @return Training result
     */
    TrainingResult resumeFromCheckpoint(const std::string& checkpoint_path,
                                       TrainingCallback callback = nullptr);
    
    /**
     * @brief Evaluate adapter on validation set
     * @param adapter_version Adapter version to evaluate
     * @return Evaluation metrics
     */
    TrainingResult evaluate(const std::string& adapter_version);
    
    /**
     * @brief Deploy adapter version to production
     * @param adapter_version Version to deploy
     * @param traffic_split Traffic split for A/B testing (0.0-1.0)
     * @return true if deployment successful (including integrity verification)
     */
    bool deployVersion(const std::string& adapter_version, float traffic_split = 1.0f);

    /**
     * @brief Deploy adapter version with full result details.
     *
     * Equivalent to deployVersion() but returns a DeployResult that includes
     * the active version, applied split fraction, and a human-readable error
     * message on failure.  When an ILLMRouter has been injected via
     * setLLMRouter(), the router's setAdapterWeight() is called atomically
     * after the local version registry is updated.
     *
     * Error codes in DeployResult::error:
     *  - "version_not_found"  – version is unknown and not in checkpoint dir
     *  - "integrity_failure"  – checkpoint checksum validation failed
     *  - "router_unavailable" – router is not reachable
     *  - "invalid_split"      – traffic_split outside [0, 1]
     *
     * @param adapter_version Version identifier to deploy.
     * @param traffic_split   Fraction of traffic routed to this version [0,1].
     * @return DeployResult with success flag, active_version, split_applied, error.
     */
    DeployResult deployVersionEx(const std::string& adapter_version,
                                 float traffic_split = 1.0f);

    /**
     * @brief Rollback to previous adapter version
     * @param target_version Version to roll back to
     * @return true if rollback successful
     */
    bool rollbackVersion(const std::string& target_version);

    /**
     * @brief Roll back to a target version with full result details.
     *
     * Equivalent to rollbackVersion() but returns a DeployResult.  When an
     * ILLMRouter has been injected, the router weight is updated atomically.
     *
     * @param target_version Version to roll back to.
     * @return DeployResult describing the outcome.
     */
    DeployResult rollbackVersionEx(const std::string& target_version);
    
    /**
     * @brief Get list of available adapter versions
     * @return Vector of version identifiers
     */
    std::vector<std::string> listVersions() const;

    /**
     * @brief Select an adapter version for the next request using weighted-random traffic routing.
     *
     * Returns the adapter version name chosen according to the traffic_split weights set via
     * deployVersion().  When multiple versions are active, each call independently samples
     * from the configured distribution, enabling A/B-test-style canary rollouts.
     *
     * @return Selected adapter version name, or empty string if no version is active.
     */
    std::string selectAdapterForRequest() const;
    
    /**
     * @brief Set training hyperparameters
     * @param rank LoRA rank
     * @param alpha LoRA alpha
     * @param learning_rate Learning rate
     */
    void setHyperparameters(int rank, float alpha, float learning_rate);
    
    /**
     * @brief Enable/disable checkpointing
     * @param enabled Whether to save checkpoints
     * @param checkpoint_steps Steps between checkpoints
     */
    void setCheckpointing(bool enabled, size_t checkpoint_steps = 100);

    /**
     * @brief Inject an LLM router for adapter serving integration.
     *
     * When a non-null router is set, deployVersionEx() and rollbackVersionEx()
     * call router->setAdapterWeight() after updating the local version registry
     * to propagate the traffic split to the live inference layer.
     *
     * The trainer does NOT take ownership; the router must remain valid for the
     * lifetime of this trainer.
     *
     * @param router Pointer to ILLMRouter implementation, or nullptr to detach.
     */
    void setLLMRouter(ILLMRouter* router);

    /**
     * @brief Get training metrics accumulated during the last train() call.
     *
     * Returns per-epoch and per-step losses, best loss values, and timing.
     * The metrics are reset at the start of each train() call.
     */
    TrainingMetrics getMetrics() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
