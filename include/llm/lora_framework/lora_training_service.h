/**
 * @file lora_training_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_config.h"
#include "mixed_precision.h"
#include "lr_scheduler.h"
#include "gradient_utils.h"
#include "llm/lora_training_error_codes.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <cmath>

// Forward declarations for shard infrastructure
namespace themis {
namespace sharding {
    class ShardRouter;
    class ShardTopology;
}
}

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Training data sample
 */
struct TrainingDataSample {
    std::string input;          // Input text or prompt
    std::string output;         // Expected output or completion
    json metadata;              // Optional metadata
    
    json toJSON() const {
        return json{
            {"input", input},
            {"output", output},
            {"metadata", metadata}
        };
    }
    
    static TrainingDataSample fromJSON(const json& j) {
        TrainingDataSample sample;
        if (j.contains("input")) sample.input = j["input"];
        if (j.contains("output")) sample.output = j["output"];
        if (j.contains("metadata")) sample.metadata = j["metadata"];
        return sample;
    }
};

/**
 * @brief Training dataset
 */
struct TrainingData {
    std::vector<TrainingDataSample> samples;
    std::string dataset_name;
    json metadata;
    
    size_t size() const { return samples.size(); }
    
    json toJSON() const {
        json j;
        j["dataset_name"] = dataset_name;
        j["metadata"] = metadata;
        j["samples"] = json::array();
        for (const auto& sample : samples) {
            j["samples"].push_back(sample.toJSON());
        }
        return j;
    }
    
    static TrainingData fromJSON(const json& j) {
        TrainingData data;
        if (j.contains("dataset_name")) data.dataset_name = j["dataset_name"];
        if (j.contains("metadata")) data.metadata = j["metadata"];
        if (j.contains("samples")) {
            for (const auto& sample_json : j["samples"]) {
                data.samples.push_back(TrainingDataSample::fromJSON(sample_json));
            }
        }
        return data;
    }
};

/**
 * @brief Training result
 */
struct TrainingResult {
    virtual ~TrainingResult() = default;
    bool success = false;
    std::string adapter_id;
    std::string version;
    float final_loss = 0.0f;
    float validation_accuracy = 0.0f;
    int epochs_completed = 0;
    std::chrono::seconds training_time{0};
    std::string error_message;
    json metrics;
    
    json toJSON() const {
        return json{
            {"success", success},
            {"adapter_id", adapter_id},
            {"version", version},
            {"final_loss", final_loss},
            {"validation_accuracy", validation_accuracy},
            {"epochs_completed", epochs_completed},
            {"training_time_seconds", training_time.count()},
            {"error_message", error_message},
            {"metrics", metrics}
        };
    }
};

/**
 * @brief Training metrics
 */
struct TrainingMetrics {
    virtual ~TrainingMetrics() = default;
    int current_epoch = 0;
    int total_epochs = 0;
    int current_step = 0;
    int total_steps = 0;
    float current_loss = 0.0f;
    float learning_rate = 0.0f;
    float progress = 0.0f;  // 0.0 to 1.0
    std::string status = "idle"; // "idle", "training", "validating", "completed", "failed"
    
    json toJSON() const {
        return json{
            {"current_epoch", current_epoch},
            {"total_epochs", total_epochs},
            {"current_step", current_step},
            {"total_steps", total_steps},
            {"current_loss", current_loss},
            {"learning_rate", learning_rate},
            {"progress", progress},
            {"status", status}
        };
    }
};

/**
 * @brief Callback for training progress
 */
using TrainingCallback = std::function<void(const TrainingMetrics&)>;

/**
 * @brief Manages LoRA adapter training
 * 
 * Features:
 * - On-the-fly training
 * - Batch training
 * - Configuration management
 * - Progress monitoring
 */
class LoRATrainingService {
public:
    /**
     * @brief Configuration for training service
     */
    struct Config {
        LoRAHyperparameters default_hyperparameters;
        std::string base_model_path = "models/default.gguf";
        int max_concurrent_training = 1;     // Maximum concurrent training jobs
        bool enable_checkpointing = true;
        int checkpoint_interval_steps = 100;
        std::string checkpoint_dir = "data/lora_checkpoints";
        
        // Phase 2: Base model integration settings
        std::vector<std::string> target_modules = {"attention.wq", "attention.wv"};  // Layers to adapt
        bool use_base_model = false;         // Enable base model integration (Phase 2b)

        /**
         * @brief Optional model-path resolver.
         *
         * When set, this function is called with a model identifier (e.g. the
         * base_model_path stem) to return the actual filesystem path of the GGUF
         * file.  Implement with LLMModelStorage::resolveGGUFPath() or any other
         * model registry.  When nullptr the raw @p base_model_path string is used
         * directly, preserving backward-compatible behaviour.
         *
         * @param model_id  Model identifier string.
         * @return Absolute or relative path to the GGUF file.
         */
        using ModelPathProviderFn = std::function<std::string(const std::string& model_id)>;
        ModelPathProviderFn model_path_provider;
        
        // QLoRA configuration
        QLoRAConfig qlora;
        
        // Production training features
        MixedPrecisionConfig mixed_precision;
        LRSchedulerConfig lr_scheduler;
        GradientClippingConfig gradient_clipping;
        GradientAccumulationConfig gradient_accumulation;
        
        // Distributed training configuration
        bool enable_distributed_training = false;  // Enable distributed training across shards
        std::string coordinator_shard;             // Coordinator shard ID
        std::vector<std::string> participant_shards;  // Participant shard IDs
        
        // Shard infrastructure (optional - for dependency injection)
        std::shared_ptr<themis::sharding::ShardRouter> shard_router;
        std::shared_ptr<themis::sharding::ShardTopology> shard_topology;
        bool auto_discover_shards = true;          // Auto-discover shards from topology
    };
    
    explicit LoRATrainingService(const Config& config);
    explicit LoRATrainingService();
    ~LoRATrainingService();
    
    // Disable copy
    LoRATrainingService(const LoRATrainingService&) = delete;
    LoRATrainingService& operator=(const LoRATrainingService&) = delete;
    
    /**
     * @brief Train adapter on-the-fly with small dataset
     * @param adapter_id Adapter identifier
     * @param data Training data
     * @param hyperparameters LoRA hyperparameters (optional)
     * @return Training result
     */
    TrainingResult trainOnTheFly(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
    /**
     * @brief Train adapter with batch processing
     * @param adapter_id Adapter identifier
     * @param dataset Large training dataset
     * @param hyperparameters LoRA hyperparameters (optional)
     * @return Training result
     */
    TrainingResult trainBatch(
        const std::string& adapter_id,
        const std::vector<TrainingData>& dataset,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
    /**
     * @brief Set training configuration
     * @param config Training configuration
     */
    void setTrainingConfig(const Config& config);
    
    /**
     * @brief Get training configuration
     * @return Current configuration
     */
    Config getTrainingConfig() const;
    
    /**
     * @brief Set hyperparameters for training
     * @param hyperparameters LoRA hyperparameters
     */
    void setHyperparameters(const LoRAHyperparameters& hyperparameters);
    
    /**
     * @brief Get current hyperparameters
     * @return LoRA hyperparameters
     */
    LoRAHyperparameters getHyperparameters() const;
    
    /**
     * @brief Get current training metrics
     * @return Training metrics
     */
    TrainingMetrics getMetrics() const;
    
    /**
     * @brief Register callback for training progress
     * @param callback Callback function
     */
    void registerCallback(TrainingCallback callback);
    
    /**
     * @brief Check if training is in progress
     * @return true if training
     */
    bool isTraining() const;
    
    /**
     * @brief Stop current training
     */
    void stopTraining();
    
    /**
     * @brief Train adapter with QLoRA (quantized base model)
     * @param adapter_id Adapter identifier
     * @param data Training data
     * @param hyperparameters LoRA hyperparameters (optional)
     * @return Training result
     */
    TrainingResult trainWithQuantization(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
    /**
     * @brief Train adapter in distributed mode across multiple shards
     * 
     * Coordinates distributed training across shards with:
     * - Gradient synchronization and aggregation
     * - Fault tolerance (shard failures)
     * - Checkpointing and recovery
     * - Byzantine fault detection
     * 
     * @param adapter_id Adapter identifier
     * @param data Training data (distributed across shards)
     * @param hyperparameters LoRA hyperparameters (optional)
     * @return Training result with distributed statistics
     */
    TrainingResult trainDistributed(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods for QLoRA
    /**
     * @brief Create QLoRA layers for training
     * @param model Quantized base model
     * @param rank LoRA rank
     * @return Vector of QLoRA layers
     */
    std::vector<std::unique_ptr<class QLoRALayer>> createQLoRALayers(
        const class QuantizedModel& model,
        size_t rank
    );
    
    /**
     * @brief Load and optionally quantize base model
     * @param model_path Path to base model
     * @param config QLoRA configuration
     * @return Quantized model
     */
    std::unique_ptr<class QuantizedModel> loadQuantizedBaseModel(
        const std::string& model_path,
        const QLoRAConfig& config
    );

    // ─── ModelPathProvider bridge (stub #289) ────────────────────────────────

    /// @brief Type alias for model path lookup injection.
    using ModelPathProviderFn = std::function<std::string(const std::string& model_name)>;

    /**
     * @brief Install a model path lookup callback for loadQuantizedBaseModel().
     *
     * When set, loadQuantizedBaseModel() calls this function to resolve the GGUF
     * file path for a given model name instead of relying on the caller-provided
     * path.  Replaces the synthetic 3-layer fallback when the path is not found.
     * @param fn Callable receiving a model name → resolved absolute file path.
     */
    static void setModelPathProviderFn(ModelPathProviderFn fn);

    /**
     * @brief Remove the model path lookup bridge (reverts to caller-supplied path).
     */
    static void clearModelPathProviderFn();
    
    /**
     * @brief Estimate memory usage for QLoRA training
     * @param model_path Path to base model
     * @param config QLoRA configuration
     * @return Estimated memory in bytes
     */
    size_t estimateMemoryUsage(
        const std::string& model_path,
        const QLoRAConfig& config
    );
};

} // namespace lora
} // namespace llm
} // namespace themis
