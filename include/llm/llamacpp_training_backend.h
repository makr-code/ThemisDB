/**
 * @file llamacpp_training_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include "adapter_registry.h"

namespace themisdb {
namespace llm {

// llama.cpp configuration for model loading
struct LlamaCppConfig {
    virtual ~LlamaCppConfig() = default;
    std::string model_path;              // Path to base model GGUF file
    int n_ctx = 2048;                    // Context length
    int n_batch = 512;                   // Batch size for prompt processing
    int n_gpu_layers = 0;                // Number of layers to offload to GPU (0=CPU only)
    bool use_mmap = true;                // Use memory-mapped file loading
    bool use_mlock = false;              // Lock model in RAM (prevents swapping)
    int n_threads = 4;                   // Number of CPU threads
    float rope_freq_base = 10000.0f;     // RoPE frequency base
    float rope_freq_scale = 1.0f;        // RoPE frequency scaling
    
    nlohmann::json toJSON() const {
        return {
            {"model_path", model_path},
            {"n_ctx", n_ctx},
            {"n_batch", n_batch},
            {"n_gpu_layers", n_gpu_layers},
            {"use_mmap", use_mmap},
            {"use_mlock", use_mlock},
            {"n_threads", n_threads},
            {"rope_freq_base", rope_freq_base},
            {"rope_freq_scale", rope_freq_scale}
        };
    }
    
    static LlamaCppConfig fromJSON(const nlohmann::json& j) {
        LlamaCppConfig config;
        config.model_path = j.value("model_path", "");
        config.n_ctx = j.value("n_ctx", 2048);
        config.n_batch = j.value("n_batch", 512);
        config.n_gpu_layers = j.value("n_gpu_layers", 0);
        config.use_mmap = j.value("use_mmap", true);
        config.use_mlock = j.value("use_mlock", false);
        config.n_threads = j.value("n_threads", 4);
        config.rope_freq_base = j.value("rope_freq_base", 10000.0f);
        config.rope_freq_scale = j.value("rope_freq_scale", 1.0f);
        return config;
    }
};

// LoRA layer configuration
struct LoRALayerConfig {
    int rank = 8;                        // LoRA rank (typically 4, 8, 16, 32, 64)
    float alpha = 16.0f;                 // LoRA alpha (scaling factor, usually 2*rank)
    float dropout = 0.0f;                // LoRA dropout probability
    std::vector<std::string> target_modules;  // Modules to apply LoRA (e.g., "q_proj", "v_proj")
    
    nlohmann::json toJSON() const {
        return {
            {"rank", rank},
            {"alpha", alpha},
            {"dropout", dropout},
            {"target_modules", target_modules}
        };
    }
    
    static LoRALayerConfig fromJSON(const nlohmann::json& j) {
        LoRALayerConfig config;
        config.rank = j.value("rank", 8);
        config.alpha = j.value("alpha", 16.0f);
        config.dropout = j.value("dropout", 0.0f);
        config.target_modules = j.value("target_modules", std::vector<std::string>{"q_proj", "v_proj"});
        return config;
    }
};

// Common target module configurations
namespace TargetModules {
    inline std::vector<std::string> QV_ONLY() { return {"q_proj", "v_proj"}; }
    inline std::vector<std::string> QKV() { return {"q_proj", "k_proj", "v_proj"}; }
    inline std::vector<std::string> QKVO() { return {"q_proj", "k_proj", "v_proj", "o_proj"}; }
    inline std::vector<std::string> ALL_LINEAR() { 
        return {"q_proj", "k_proj", "v_proj", "o_proj", 
                "gate_proj", "up_proj", "down_proj"}; 
    }
}

// Training step result
struct TrainingStepResult {
    virtual ~TrainingStepResult() = default;
    float loss = 0.0f;                          // Training loss
    float grad_norm = 0.0f;                     // Gradient norm (for monitoring)
    int num_tokens = 0;                      // Number of tokens processed
    float learning_rate = 0.0f;                 // Current learning rate
    bool success = false;                        // Whether step succeeded
    std::string error_message;           // Error details if failed
    
    nlohmann::json toJSON() const {
        return {
            {"loss", loss},
            {"grad_norm", grad_norm},
            {"num_tokens", num_tokens},
            {"learning_rate", learning_rate},
            {"success", success},
            {"error_message", error_message}
        };
    }
};

// Evaluation result
struct EvaluationResult {
    virtual ~EvaluationResult() = default;
    float loss = 0.0f;                          // Evaluation loss
    float perplexity = 0.0f;                    // Perplexity
    int num_tokens = 0;                      // Number of tokens evaluated
    bool success = false;                        // Whether evaluation succeeded
    std::string error_message;           // Error details if failed
    
    nlohmann::json toJSON() const {
        return {
            {"loss", loss},
            {"perplexity", perplexity},
            {"num_tokens", num_tokens},
            {"success", success},
            {"error_message", error_message}
        };
    }
};

// Checkpoint data structure
struct CheckpointData {
    virtual ~CheckpointData() = default;
    int epoch = 0;                           // Current epoch
    int global_step = 0;                     // Global training step
    std::map<std::string, std::vector<float>> lora_weights;  // LoRA weight matrices
    std::map<std::string, std::vector<float>> optimizer_state;  // Optimizer state (momentum, etc.)
    TrainingMetrics metrics;             // Training metrics at checkpoint
    std::string timestamp;               // Checkpoint creation time
    std::string base_model_hash;         // SHA-256 hash of base model (verify compatibility)
    LoRALayerConfig lora_config;         // LoRA configuration
    
    nlohmann::json toJSON() const {
        nlohmann::json j;
        j["epoch"] = epoch;
        j["global_step"] = global_step;
        j["metrics"] = metrics.toJSON();
        j["timestamp"] = timestamp;
        j["base_model_hash"] = base_model_hash;
        j["lora_config"] = lora_config.toJSON();
        // Note: weights and optimizer_state are binary, saved separately
        return j;
    }
    
    static CheckpointData fromJSON(const nlohmann::json& j) {
        CheckpointData data;
        data.epoch = j.value("epoch", 0);
        data.global_step = j.value("global_step", 0);
        data.metrics = TrainingMetrics::fromJSON(j.value("metrics", nlohmann::json{}));
        data.timestamp = j.value("timestamp", "");
        data.base_model_hash = j.value("base_model_hash", "");
        data.lora_config = LoRALayerConfig::fromJSON(j.value("lora_config", nlohmann::json{}));
        return data;
    }
};

// Forward declaration of llama.cpp types (to avoid including llama.h in header)
struct llama_context;
struct llama_model;

// llama.cpp training backend
class LlamaCppTrainingBackend {
public:
    LlamaCppTrainingBackend();
    ~LlamaCppTrainingBackend();
    
    // Load base model from GGUF file
    bool loadModel(const LlamaCppConfig& config);
    
    // Initialize LoRA layers on top of base model
    bool initLoRA(const LoRALayerConfig& lora_config);
    
    // Single training step (forward + backward + optimizer step)
    TrainingStepResult trainingStep(
        const std::vector<int>& input_ids,
        const std::vector<int>& labels,
        float learning_rate,
        const std::map<std::string, std::vector<float>>& optimizer_state
    );
    
    // Evaluation step (no gradient computation)
    EvaluationResult evaluate(
        const std::vector<int>& input_ids,
        const std::vector<int>& labels
    );
    
    // Save LoRA weights to file (GGUF format compatible with llama.cpp inference)
    bool saveLoRAWeights(const std::string& output_path);
    
    // Load LoRA weights from checkpoint (for resuming training)
    bool loadLoRAWeights(const std::string& checkpoint_path);
    
    // Save complete checkpoint (weights + optimizer state + metadata)
    bool saveCheckpoint(const std::string& checkpoint_path, const CheckpointData& data);
    
    // Load complete checkpoint
    std::optional<CheckpointData> loadCheckpoint(const std::string& checkpoint_path);
    
    // Get base model hash (for checkpoint verification)
    std::string getModelHash() const;
    
    // Get LoRA parameter count
    size_t getLoRAParameterCount() const;
    
    // Get base model parameter count
    size_t getBaseModelParameterCount() const;
    
    // Set gradient clipping threshold
    void setGradientClipping(float max_norm) { gradient_clip_max_norm_ = max_norm; }
    
    // Enable/disable mixed precision training
    void setMixedPrecision(bool enable) { use_mixed_precision_ = enable; }
    
    // Get current configuration
    const LlamaCppConfig& getModelConfig() const { return model_config_; }
    const LoRALayerConfig& getLoRAConfig() const { return lora_config_; }
    
private:
    // llama.cpp context and model (opaque pointers)
    llama_context* ctx_ = nullptr;
    llama_model* model_ = nullptr;
    
    // Configuration
    LlamaCppConfig model_config_;
    LoRALayerConfig lora_config_;
    
    // LoRA weights (stored separately from frozen base model)
    std::map<std::string, std::vector<float>> lora_A_;  // Down-projection matrices
    std::map<std::string, std::vector<float>> lora_B_;  // Up-projection matrices
    
    // Training state
    bool model_loaded_ = false;
    bool lora_initialized_ = false;
    float gradient_clip_max_norm_ = 1.0f;
    bool use_mixed_precision_ = false;
    
    // Helper methods
    void initializeLoRAMatrices(const std::string& module_name, int in_features, int out_features);
    void computeGradients(const std::vector<float>& loss_grad);
    void clipGradients();
    void applyOptimizerStep(float learning_rate, 
                           const std::map<std::string, std::vector<float>>& optimizer_state);
    std::string computeModelHash() const;
};

// Factory for creating backends
class LlamaCppBackendFactory {
public:
    static std::unique_ptr<LlamaCppTrainingBackend> create(const LlamaCppConfig& config) {
        auto backend = std::make_unique<LlamaCppTrainingBackend>();
        if (!backend->loadModel(config)) {
            return nullptr;
        }
        return backend;
    }
    
    static std::unique_ptr<LlamaCppTrainingBackend> createWithLoRA(
        const LlamaCppConfig& model_config,
        const LoRALayerConfig& lora_config
    ) {
        auto backend = create(model_config);
        if (backend && !backend->initLoRA(lora_config)) {
            return nullptr;
        }
        return backend;
    }
};

} // namespace llm
} // namespace themisdb

