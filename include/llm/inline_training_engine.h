/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inline_training_engine.h                           ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2f644f2edb  2026-04-13  feat(llm): implement InlineTrainingEngine for on-the-fly ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    
    ~InlineTrainingEngine();
    
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
