/**
 * @file inline_training_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2025 ThemisDB
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>
#include "adapter_registry.h"
#include "training_data_iterator.h"
#include "gguf_st_adapter.h"

// Forward-declare governance types so callers don't need to include
// the full governance header unless they actually call setGovernancePolicy().
namespace themis::governance {
    class ModelGovernancePolicy;
}

// Forward-declare RocksDB type so callers don't need the full header.
namespace rocksdb {
    class DB;
}

namespace themis::llm {

// Forward declarations
class BatchGenerator;
class MultiModelTrainingData;
class LLamaCppBackend;

/**
 * @brief Optimizer types supported by the training engine
 */
enum class OptimizerType {
    ADAM_W,      // AdamW optimizer (recommended for LoRA)
    SGD,         // Stochastic Gradient Descent
    ADAM,        // Adam optimizer
    ADAGRAD,     // Adagrad optimizer
    RMSPROP      // RMSprop optimizer
};

/**
 * @brief Learning rate scheduler types
 */
enum class SchedulerType {
    CONSTANT,           // Constant learning rate
    LINEAR,             // Linear decay
    COSINE,             // Cosine annealing
    COSINE_WITH_WARMUP, // Cosine with warmup
    POLYNOMIAL          // Polynomial decay
};

/**
 * @brief Optimizer configuration
 */
struct OptimizerConfig {
    OptimizerType type = OptimizerType::ADAM_W;
    float learning_rate = 1e-4f;
    float beta1 = 0.9f;           // For Adam/AdamW
    float beta2 = 0.999f;         // For Adam/AdamW
    float epsilon = 1e-8f;
    float weight_decay = 0.01f;   // For AdamW
    float momentum = 0.9f;        // For SGD
    bool nesterov = false;        // For SGD

    // Gradient clipping
    bool use_gradient_clipping = true;
    float max_grad_norm = 1.0f;

    nlohmann::json toJSON() const;
    static OptimizerConfig fromJSON(const nlohmann::json& j);
};

/**
 * @brief Learning rate scheduler configuration
 */
struct SchedulerConfig {
    SchedulerType type = SchedulerType::COSINE_WITH_WARMUP;
    int warmup_steps = 100;
    float min_lr = 1e-6f;
    float max_lr = 1e-4f;
    int total_steps = 1000;
    float power = 1.0f;  // For polynomial decay

    nlohmann::json toJSON() const;
    static SchedulerConfig fromJSON(const nlohmann::json& j);
};

/**
 * @brief Training metrics and statistics
 */
struct TrainingMetrics {
    virtual ~TrainingMetrics() = default;
    int epoch = 0;
    int step = 0;
    float loss = 0.0f;
    float gradient_norm = 0.0f;
    float learning_rate = 0.0f;

    // Additional metrics
    std::optional<float> perplexity;
    std::optional<float> accuracy;

    // Timing
    double elapsed_seconds = 0.0;
    double samples_per_second = 0.0;

    nlohmann::json toJSON() const;
};

/**
 * @brief Training progress callback
 */
using ProgressCallback = std::function<void(const TrainingMetrics&)>;

/**
 * @brief Checkpoint callback (called when checkpoint is saved)
 */
using CheckpointCallback = std::function<void(const std::string& checkpoint_path)>;

/**
 * @brief Training state for checkpointing
 */
struct TrainingState {
    virtual ~TrainingState() = default;
    int current_epoch = 0;
    int current_step = 0;
    float best_loss = std::numeric_limits<float>::max();
    std::vector<float> loss_history;

    // Optimizer state (will be serialized by optimizer)
    std::vector<uint8_t> optimizer_state;

    nlohmann::json toJSON() const;
    static TrainingState fromJSON(const nlohmann::json& j);
};

/**
 * @brief Configuration for inline training
 */
struct InlineTrainingConfig {
    // Training parameters
    int epochs = 3;
    int batch_size = 4;
    int gradient_accumulation_steps = 1;
    int max_steps = -1;  // -1 means train for full epochs

    // Evaluation
    int eval_steps = 100;
    int save_steps = 500;
    bool eval_on_start = false;

    // Mixed precision
    bool use_fp16 = false;
    bool use_bf16 = false;

    // Checkpointing
    std::string checkpoint_dir = "./checkpoints";
    bool save_optimizer_state = true;
    int max_checkpoints_to_keep = 3;

    // Optimizer and scheduler
    OptimizerConfig optimizer;
    SchedulerConfig scheduler;

    // Callbacks
    ProgressCallback progress_callback;
    CheckpointCallback checkpoint_callback;

    // Seed for reproducibility
    std::optional<int> seed;

    // Governance: when true, train() fails immediately if no governance
    // policy is set (or if the policy returns DENY).  When false (default),
    // a missing policy only emits a WARN and allows the job to proceed.
    //
    // Deployment guidance: this field defaults to false so that existing
    // callers that do not inject a policy remain unaffected.  Production
    // environments MUST set this to true via their configuration layer
    // (e.g. an environment-specific JSON/YAML config, or a build-time
    // constant that is asserted in integration tests) to prevent training
    // from running without an active governance check.
    bool require_policy_gate = false;

    nlohmann::json toJSON() const;
    static InlineTrainingConfig fromJSON(const nlohmann::json& j);
};

/**
 * @brief Result of training operation
 */
struct TrainingResult {
    bool success = false;
    std::string message;
    std::string adapter_path;
    TrainingMetrics final_metrics;
    std::vector<TrainingMetrics> history;

    nlohmann::json toJSON() const;
};

/**
 * @brief Gradient computer callback type.
 *
 * When set via `setGradientComputer()` this function is called by
 * `computeGradients()` instead of the built-in synthetic proxy signal.
 * The function receives the current mini-batch and fills the pre-sized
 * `gradients` vector with the real loss-gradient values.
 *
 * @param batch     Mini-batch of training samples.
 * @param gradients Output gradient vector (resized by the callback to
 *                  match the actual LoRA parameter count).
 */
using GradientComputerFn =
    std::function<void(const std::vector<TrainingDataIterator::TrainingSample>&,
                       std::vector<float>&)>;

/**
 * @brief Inline training engine for LoRA/QLoRA adapters
 *
 * This engine performs training directly on data from RocksDB without
 * requiring JSONL export. It supports:
 * - Multi-model enrichment (Graph + Vector + Relational)
 * - Multiple optimizers (AdamW, SGD, etc.)
 * - Learning rate scheduling
 * - Gradient accumulation
 * - Mixed precision training
 * - Checkpointing and resumption
 * - Progress tracking
 */
class InlineTrainingEngine {
public:
    /**
     * @brief Constructor
     * @param registry Adapter registry for metadata management
     * @param data_iterator Training data source
     * @param config Training configuration
     */
    InlineTrainingEngine(
        std::shared_ptr<AdapterRegistry> registry,
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const InlineTrainingConfig& config
    );

    ~InlineTrainingEngine() noexcept;

    /**
     * @brief Inject a real gradient computer for production LoRA training.
     *
     * When set, `computeGradients()` delegates entirely to @p fn instead of
     * the built-in synthetic proxy signal.  The callback is expected to:
     *   1. Perform a forward + backward pass over @p batch against the loaded
     *      llama.cpp model and return the per-parameter gradient vector.
     *   2. Resize `gradients` to the actual LoRA parameter count.
     *
     * Pass `nullptr` to clear the callback and revert to the synthetic proxy
     * (useful for unit-testing the optimizer/LR-scheduling machinery without
     * a live model backend).
     *
     * Thread-safe: the stored function is replaced atomically inside the
     * engine's internal mutex before the next training step reads it.
     *
     * Roadmap ref: src/llm/FUTURE_ENHANCEMENTS.md §InlineTrainingEngine
     *              production gradient (v1.8.0)
     */
    void setGradientComputer(GradientComputerFn fn);

    /**
     * @brief Inject a governance policy used to gate training jobs.
     *
     * When set, train() calls ModelGovernancePolicy::checkExportPermission()
     * before starting the training loop.  A DENY decision causes train() to
     * return immediately with success=false and a human-readable denial reason.
     *
     * Passing nullptr clears the policy.  If InlineTrainingConfig::require_policy_gate
     * is true and policy is nullptr, train() will also return failure to prevent
     * ungoverned training in strict environments.
     *
     * Thread-safe: the stored shared_ptr is replaced atomically via the
     * engine's internal mutex before the next train() call reads it.
     */
    void setGovernancePolicy(
        std::shared_ptr<governance::ModelGovernancePolicy> policy);

    /**
     * @brief Inject a RocksDB handle for checkpoint persistence.
     *
     * When set, saveCheckpoint() additionally writes the serialised
     * TrainingState JSON into RocksDB under the given path key, and
     * loadCheckpoint() reads from RocksDB first, falling back to the
     * filesystem JSON if the key is absent.
     *
     * Pass nullptr to clear the handle and revert to filesystem-only mode.
     */
    void setCheckpointDb(std::shared_ptr<rocksdb::DB> db);

    /**
     * @brief Train a new LoRA adapter
     * @param adapter_id Unique identifier for the adapter
     * @param base_model_path Path to base model (GGUF format)
     * @param training_config LoRA-specific training configuration
     * @return Training result with metrics and adapter path
     */
    TrainingResult train(
        const std::string& adapter_id,
        const std::string& base_model_path,
        const TrainingConfig& training_config
    );

    /**
     * @brief Resume training from a checkpoint
     * @param checkpoint_path Path to checkpoint directory
     * @return Training result
     */
    TrainingResult resumeFromCheckpoint(const std::string& checkpoint_path);

    /**
     * @brief Evaluate adapter on validation data
     * @param adapter_path Path to adapter file
     * @param base_model_path Path to base model
     * @return Evaluation metrics
     */
    TrainingMetrics evaluate(
        const std::string& adapter_path,
        const std::string& base_model_path
    );

    /**
     * @brief Stop training (can be called from another thread)
     */
    void stopTraining();

    /**
     * @brief Check if training is currently running
     */
    bool isTraining() const;

    /**
     * @brief Get current training state (for monitoring)
     */
    std::optional<TrainingState> getCurrentState() const;

private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Optional RocksDB handle for checkpoint persistence (dual-write)
    std::shared_ptr<rocksdb::DB> checkpoint_db_;

    // Training loop implementation
    TrainingResult trainLoop(
        const std::string& adapter_id,
        const std::string& base_model_path,
        const TrainingConfig& training_config
    );

    // Gradient computation
    void computeGradients(
        const std::vector<TrainingDataIterator::TrainingSample>& batch,
        std::vector<float>& gradients
    );

    // Optimizer step
    void optimizerStep(
        std::vector<float>& parameters,
        const std::vector<float>& gradients,
        int step
    );

    // Learning rate scheduling
    float getLearningRate(int step) const;

    // Checkpointing
    void saveCheckpoint(const std::string& path, const TrainingState& state);
    TrainingState loadCheckpoint(const std::string& path);

    // Validation
    TrainingMetrics runValidation();
};

/**
 * @brief Factory for creating training engines
 */
class TrainingEngineFactory {
public:
    /**
     * @brief Create a training engine with default configuration
     */
    static std::unique_ptr<InlineTrainingEngine> create(
        std::shared_ptr<AdapterRegistry> registry,
        std::shared_ptr<TrainingDataIterator> data_iterator
    );

    /**
     * @brief Create a training engine with custom configuration
     */
    static std::unique_ptr<InlineTrainingEngine> create(
        std::shared_ptr<AdapterRegistry> registry,
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const InlineTrainingConfig& config
    );
};

} // namespace themis::llm

