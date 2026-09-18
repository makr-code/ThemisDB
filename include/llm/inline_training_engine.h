/**
 * @file inline_training_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class OptimizerType {
    ADAM_W,      // AdamW optimizer (recommended for LoRA)
    SGD,         // Stochastic Gradient Descent
    ADAM,        // Adam optimizer
    ADAGRAD,     // Adagrad optimizer
    RMSPROP      // RMSprop optimizer
};

enum class SchedulerType {
    CONSTANT,           // Constant learning rate
    LINEAR,             // Linear decay
    COSINE,             // Cosine annealing
    COSINE_WITH_WARMUP, // Cosine with warmup
    POLYNOMIAL          // Polynomial decay
};

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

    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static OptimizerConfig fromJSON(const nlohmann::json& j);
};

struct SchedulerConfig {
    SchedulerType type = SchedulerType::COSINE_WITH_WARMUP;
    int warmup_steps = 100;
    float min_lr = 1e-6f;
    float max_lr = 1e-4f;
    int total_steps = 1000;
    float power = 1.0f;  // For polynomial decay

    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static SchedulerConfig fromJSON(const nlohmann::json& j);
};

struct TrainingMetrics {
    /**
     * @brief Training Metrics.
     * @return Return value.
     */
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

    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

using ProgressCallback = std::function<void(const TrainingMetrics&)>;

using CheckpointCallback = std::function<void(const std::string& checkpoint_path)>;

struct TrainingState {
    /**
     * @brief Training State.
     * @return Return value.
     */
    virtual ~TrainingState() = default;
    int current_epoch = 0;
    int current_step = 0;
    float best_loss = std::numeric_limits<float>::max();
    std::vector<float> loss_history;

    // Optimizer state (will be serialized by optimizer)
    std::vector<uint8_t> optimizer_state;

    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static TrainingState fromJSON(const nlohmann::json& j);
};

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

    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static InlineTrainingConfig fromJSON(const nlohmann::json& j);
};

struct TrainingResult {
    bool success = false;
    std::string message;
    std::string adapter_path;
    TrainingMetrics final_metrics;
    std::vector<TrainingMetrics> history;

    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

using GradientComputerFn =
    std::function<void(const std::vector<TrainingDataIterator::TrainingSample>&,
                       std::vector<float>&)>;

class InlineTrainingEngine {
public:
    InlineTrainingEngine(
        std::shared_ptr<AdapterRegistry> registry,
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const InlineTrainingConfig& config
    );

    ~InlineTrainingEngine() noexcept;

    /**
     * @brief Set Gradient Computer.
     * @param[in] fn Input parameter.
     */
    void setGradientComputer(GradientComputerFn fn);

    /**
     * @brief Set Governance Policy.
     * @param[in] policy Input parameter.
     */
    void setGovernancePolicy(
        std::shared_ptr<governance::ModelGovernancePolicy> policy);

    /**
     * @brief Set Checkpoint Db.
     * @param[in] db Input parameter.
     */
    void setCheckpointDb(std::shared_ptr<rocksdb::DB> db);

    /**
     * @brief Train.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] base_model_path Path to the base model.
     * @param[in] training_config Input parameter.
     * @return Return value.
     */
    TrainingResult train(
        const std::string& adapter_id,
        const std::string& base_model_path,
        const TrainingConfig& training_config
    );

    /**
     * @brief Resume From Checkpoint.
     * @param[in] checkpoint_path Path to the checkpoint.
     * @return Return value.
     */
    TrainingResult resumeFromCheckpoint(const std::string& checkpoint_path);

    /**
     * @brief Evaluate.
     * @param[in] adapter_path Path to the adapter.
     * @param[in] base_model_path Path to the base model.
     * @return Return value.
     */
    TrainingMetrics evaluate(
        const std::string& adapter_path,
        const std::string& base_model_path
    );

    /**
     * @brief Stop Training.
     */
    void stopTraining();

    /**
     * @brief Is Training.
     * @return True when the operation succeeds.
     */
    bool isTraining() const;

    /**
     * @brief Get Current State.
     * @return Return value.
     */
    std::optional<TrainingState> getCurrentState() const;

private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Optional RocksDB handle for checkpoint persistence (dual-write)
    std::shared_ptr<rocksdb::DB> checkpoint_db_;

    // Training loop implementation
    /**
     * @brief Train Loop.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] base_model_path Path to the base model.
     * @param[in] training_config Input parameter.
     * @return Return value.
     */
    TrainingResult trainLoop(
        const std::string& adapter_id,
        const std::string& base_model_path,
        const TrainingConfig& training_config
    );

    // Gradient computation
    /**
     * @brief Compute Gradients.
     * @param[in] batch Input parameter.
     * @param[in,out] gradients Input/output parameter.
     */
    void computeGradients(
        const std::vector<TrainingDataIterator::TrainingSample>& batch,
        std::vector<float>& gradients
    );

    // Optimizer step
    /**
     * @brief Optimizer Step.
     * @param[in,out] parameters Input/output parameter.
     * @param[in] gradients Input parameter.
     * @param[in] step Input parameter.
     */
    void optimizerStep(
        std::vector<float>& parameters,
        const std::vector<float>& gradients,
        int step
    );

    // Learning rate scheduling
    /**
     * @brief Get Learning Rate.
     * @param[in] step Input parameter.
     * @return Return value.
     */
    float getLearningRate(int step) const;

    // Checkpointing
    /**
     * @brief Save Checkpoint.
     * @param[in] path Input parameter.
     * @param[in] state Input parameter.
     */
    void saveCheckpoint(const std::string& path, const TrainingState& state);
    /**
     * @brief Load Checkpoint.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    TrainingState loadCheckpoint(const std::string& path);

    // Validation
    /**
     * @brief Run Validation.
     * @return Return value.
     */
    TrainingMetrics runValidation();
};

class TrainingEngineFactory {
public:
    /**
     * @brief Create.
     * @param[in] registry Input parameter.
     * @param[in] data_iterator Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<InlineTrainingEngine> create(
        std::shared_ptr<AdapterRegistry> registry,
        std::shared_ptr<TrainingDataIterator> data_iterator
    );

    /**
     * @brief Create.
     * @param[in] registry Input parameter.
     * @param[in] data_iterator Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<InlineTrainingEngine> create(
        std::shared_ptr<AdapterRegistry> registry,
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const InlineTrainingConfig& config
    );
};

} // namespace themis::llm

