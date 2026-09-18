/**
 * @file lora_training_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: contains().
     */
    static TrainingDataSample fromJSON(const json& j) {
        TrainingDataSample sample = {};
        if (j.contains("input")) {
          sample.input = j["input"];
        }
        if (j.contains("output")) {
          sample.output = j["output"];
        }
        if (j.contains("metadata")) {
          sample.metadata = j["metadata"];
        }
        return sample;
    }
};

struct TrainingData {
    std::vector<TrainingDataSample> samples;
    std::string dataset_name = {};
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
    
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: contains(), push_back().
     */
    static TrainingData fromJSON(const json& j) {
        TrainingData data = {};
        if (j.contains("dataset_name")) {
          data.dataset_name = j["dataset_name"];
        }
        if (j.contains("metadata")) {
          data.metadata = j["metadata"];
        }
        if (j.contains("samples")) {
            for (const auto& sample_json : j["samples"]) {
                data.samples.push_back(TrainingDataSample::fromJSON(sample_json));
            }
        }
        return data;
    }
};

struct TrainingResult {
    /**
     * @brief Training Result.
     * @return Return value.
     */
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

struct TrainingMetrics {
    /**
     * @brief Training Metrics.
     * @return Return value.
     */
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

using TrainingCallback = std::function<void(const TrainingMetrics&)>;

class LoRATrainingService {
public:
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
    
    /**
     * @brief Lo RATraining Service.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRATrainingService(const Config& config);
    /**
     * @brief Lo RATraining Service.
     * @return Return value.
     */
    explicit LoRATrainingService();
    ~LoRATrainingService() noexcept;
    
    // Disable copy
    LoRATrainingService(const LoRATrainingService&) = delete;
    LoRATrainingService& operator=(const LoRATrainingService&) = delete;
    
    TrainingResult trainOnTheFly(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
    TrainingResult trainBatch(
        const std::string& adapter_id,
        const std::vector<TrainingData>& dataset,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
    /**
     * @brief Set Training Config.
     * @param[in] config Input parameter.
     */
    void setTrainingConfig(const Config& config);
    
    /**
     * @brief Get Training Config.
     * @return Return value.
     */
    Config getTrainingConfig() const;
    
    /**
     * @brief Set Hyperparameters.
     * @param[in] hyperparameters Input parameter.
     */
    void setHyperparameters(const LoRAHyperparameters& hyperparameters);
    
    /**
     * @brief Get Hyperparameters.
     * @return Return value.
     */
    LoRAHyperparameters getHyperparameters() const;
    
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    TrainingMetrics getMetrics() const;
    
    /**
     * @brief Register Callback.
     * @param[in] callback Input parameter.
     */
    void registerCallback(TrainingCallback callback);
    
    /**
     * @brief Is Training.
     * @return True when the operation succeeds.
     */
    bool isTraining() const;
    
    /**
     * @brief Stop Training.
     */
    void stopTraining();
    
    TrainingResult trainWithQuantization(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
    TrainingResult trainDistributed(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters = std::nullopt
    );
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Helper methods for QLoRA
     * @param[in] model Input parameter.
     * @param[in] rank Input parameter.
     * @return Return value.
     */
    std::vector<std::unique_ptr<class QLoRALayer>> createQLoRALayers(
        const class QuantizedModel& model,
        size_t rank
    );
    
    /**
     * @brief Load Quantized Base Model.
     * @param[in] model_path Path to the model.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    std::unique_ptr<class QuantizedModel> loadQuantizedBaseModel(
        const std::string& model_path,
        const QLoRAConfig& config
    );

    // ─── ModelPathProvider bridge (stub #289) ────────────────────────────────

    using ModelPathProviderFn = std::function<std::string(const std::string& model_name)>;

    /**
     * @brief Set Model Path Provider Fn.
     * @param[in] fn Input parameter.
     */
    static void setModelPathProviderFn(ModelPathProviderFn fn);

    /**
     * @brief Clear Model Path Provider Fn.
     */
    static void clearModelPathProviderFn();
    
    /**
     * @brief Estimate Memory Usage.
     * @param[in] model_path Path to the model.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    size_t estimateMemoryUsage(
        const std::string& model_path,
        const QLoRAConfig& config
    );
};

} // namespace lora
} // namespace llm
} // namespace themis
