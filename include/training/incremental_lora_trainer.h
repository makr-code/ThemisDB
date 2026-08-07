/**
 * @file incremental_lora_trainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: incremental_lora_trainer.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "training/adapter_serving.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

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
     *  - "router_update_failed" – router rejected the weight update and local state was reverted
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

    // ── IMPL-A3: Federation bridges ──────────────────────────────────────────

    /**
     * @brief Set the shard identifier used in exported gradients.
     *
     * The shard_id is embedded in every `EncryptedGradient` produced by
     * `exportGradient()`.  Defaults to "default_shard" when not set.
     *
     * @param shard_id  Cluster-unique shard identifier.
     */
    void setShardId(const std::string& shard_id);

    /**
     * @brief Set the learning rate applied when incorporating a global delta.
     *
     * Controls the weight update in `applyGlobalDelta()`:
     *   `local_weight[layer] += federated_lr * global_delta[layer]`
     *
     * Defaults to 0.01.  Must be positive.
     *
     * @param lr  Federated learning rate (> 0).
     */
    void setFederatedLearningRate(double lr);

    /**
     * @brief Export the accumulated gradient delta as a federated contribution.
     *
     * Reads the gradient accumulator that has been filled during `train()` calls
     * since the last `exportGradient()` invocation (or since construction).
     * The gradient is normalised: `data[layer] = Σ(deltas) / update_count`.
     *
     * After a successful export the accumulator is reset to zero so that the
     * next export only reflects new training steps.
     *
     * @param federation_round  Current federated round number (embedded in the result).
     * @return `EncryptedGradient` with non-empty `data` map.
     * @throws std::runtime_error  When no training has occurred since the last export.
     */
    themis::distributed_knowledge::EncryptedGradient
    exportGradient(uint64_t federation_round);

    /**
     * @brief Incorporate an aggregated global delta into the local adapter weights.
     *
     * Applies: `local_weight[layer] += federated_lr * delta.delta[layer]`
     * for every layer name present in `delta.delta`.  Unknown layer names in the
     * delta are silently ignored (forward-compatible with larger global models).
     *
     * @param delta  Aggregated weight delta produced by `LoRAFederationCoordinator`.
     */
    void applyGlobalDelta(
        const themis::distributed_knowledge::GlobalAdapterDelta& delta);

    /**
     * @brief Return the current local weight for a named layer.
     *
     * Used by tests and observability tooling to verify that
     * `applyGlobalDelta()` has modified the local weight map.
     *
     * @param layer_name  Layer identifier (e.g. "lora_A_layer_0").
     * @return Current weight value, or 0.0 if the layer is not yet tracked.
     */
    double getLocalWeight(const std::string& layer_name) const;

    // =========================================================================
    // Phase 2: Runtime Stabilization and Diagnostics
    // =========================================================================

    /**
     * @brief Phase 2: Validate training state machine for correctness.
     *
     * Returns an error message if the trainer is in an invalid state:
     * - Training cannot start if already in progress
     * - Checkpointing cannot save intermediate states incorrectly
     * - Deployment cannot occur during active training
     *
     * @return Empty string if state is valid; error message otherwise.
     */
    std::string validateTrainingState() const;

    /**
     * @brief Phase 2: Get detailed training diagnostics and recovery info.
     *
     * Returns human-readable diagnostics including:
     * - Current training state (IDLE/TRAINING/SAVING/RECOVERING)
     * - Last error encountered (if any)
     * - Checkpoint recovery status
     * - Performance metrics summary
     *
     * @return Formatted diagnostics string.
     */
    std::string getTrainingDiagnostics() const;

    /**
     * @brief Phase 2: Get training interruption status and recovery options.
     *
     * Returns information about any previous training interruption:
     * - Whether a checkpoint can be resumed from
     * - Recommended recovery action
     * - Time since interruption
     *
     * @return Recovery status string (empty if no interruption).
     */
    std::string getRecoveryStatus() const;

    /**
     * @brief Phase 2: Enable intermediate checkpoint saving during training.
     *
     * When enabled, saves adapter state to checkpoint every N steps.
     * This allows resuming from the most recent checkpoint on interruption.
     *
     * @param enabled Whether to enable intermediate checkpoints
     * @param save_interval Steps between intermediate saves (default: 100)
     */
    void enableIntermediateCheckpointing(bool enabled, size_t save_interval = 100);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
