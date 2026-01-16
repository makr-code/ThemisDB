#pragma once

#include "lora_config.h"
#include "lora_layers.h"
#include "llm/gguf_loader.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Layer information from base model
 */
struct BaseLayerInfo {
    std::string name;              // Layer name (e.g., "layers.0.attention.wq")
    std::vector<size_t> shape;     // Tensor shape
    size_t in_features;            // Input dimension
    size_t out_features;           // Output dimension
    std::string layer_type;        // "attention.wq", "attention.wk", "feed_forward.w1", etc.
    int layer_idx;                 // Layer index in model
};

/**
 * @brief Model architecture information
 */
struct ModelArchitecture {
    std::string architecture;      // "llama", "mistral", "gpt-neox"
    int num_layers;                // Number of transformer layers
    int hidden_size;               // Hidden dimension
    int num_attention_heads;       // Number of attention heads
    int intermediate_size;         // MLP intermediate size
    std::string rope_scaling_type; // RoPE scaling type
    float rope_freq_base;          // RoPE frequency base
};

/**
 * @brief Base Model Adapter - Loads and manages frozen base models for LoRA training
 * 
 * This class:
 * - Loads GGUF base models via GGUFLoader
 * - Extracts model architecture and layer information
 * - Identifies layers suitable for LoRA adaptation
 * - Provides frozen base model weights
 * - Maps model-specific layer names to standardized names
 */
class BaseModelAdapter {
public:
    BaseModelAdapter();
    ~BaseModelAdapter();
    
    /**
     * @brief Load base model from GGUF file
     * @param model_path Path to GGUF model file
     * @return true if loaded successfully
     */
    bool loadModel(const std::string& model_path);
    
    /**
     * @brief Get model architecture information
     * @return Model architecture details
     */
    const ModelArchitecture& getArchitecture() const { return architecture_; }
    
    /**
     * @brief Get all available layers for LoRA adaptation
     * @return Vector of layer information
     */
    std::vector<BaseLayerInfo> getAdaptableLayers() const;
    
    /**
     * @brief Get layers matching target module names
     * @param target_modules List of target module patterns (e.g., "attention.wq", "attention.wv")
     * @return Vector of matching layer information
     */
    std::vector<BaseLayerInfo> getLayersByTargetModules(
        const std::vector<std::string>& target_modules) const;
    
    /**
     * @brief Get base model layer weights (frozen, read-only)
     * @param layer_name Name of the layer
     * @return Optional tensor data (empty if layer not found)
     */
    std::optional<Tensor> getLayerWeights(const std::string& layer_name) const;
    
    /**
     * @brief Check if model is loaded
     * @return true if model is loaded
     */
    bool isLoaded() const { return model_loaded_; }
    
    /**
     * @brief Get model name
     * @return Model name
     */
    std::string getModelName() const { return model_name_; }
    
    /**
     * @brief Get model path
     * @return Model file path
     */
    std::string getModelPath() const { return model_path_; }
    
    /**
     * @brief Get total number of parameters in base model
     * @return Parameter count
     */
    size_t getTotalParameters() const;
    
    /**
     * @brief Unload model and free resources
     */
    void unload();
    
private:
    std::unique_ptr<GGUFLoader> gguf_loader_;
    std::string model_path_;
    std::string model_name_;
    bool model_loaded_;
    
    ModelArchitecture architecture_;
    std::vector<BaseLayerInfo> adaptable_layers_;
    
    // Cached layer information for fast lookup
    std::unordered_map<std::string, BaseLayerInfo> layer_map_;
    
    // Helper methods
    bool parseArchitecture();
    bool identifyAdaptableLayers();
    bool parseLayerInfo(const TensorMetadata& tensor, BaseLayerInfo& layer_info);
    std::string standardizeLayerName(const std::string& model_layer_name) const;
    bool matchesTargetModule(const std::string& layer_name, 
                            const std::string& target_pattern) const;
};

/**
 * @brief LoRA-Enhanced Model - Combines base model with LoRA adapters
 * 
 * This class:
 * - Manages both frozen base model and trainable LoRA layers
 * - Coordinates forward pass: base + LoRA
 * - Manages backward pass: only through LoRA
 * - Handles layer injection and composition
 */
class LoRAEnhancedModel {
public:
    /**
     * @brief Configuration for LoRA-enhanced model
     */
    struct Config {
        std::string base_model_path;
        LoRAHyperparameters lora_config;
        std::vector<std::string> target_modules;
        bool freeze_base_model = true;    // Should always be true for LoRA
        bool use_gradient_checkpointing = false;
    };
    
    explicit LoRAEnhancedModel(const Config& config);
    ~LoRAEnhancedModel();
    
    /**
     * @brief Initialize model (load base + create LoRA adapters)
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Forward pass through base model + LoRA adapters
     * @param input Input tensor
     * @param layer_idx Layer index to process
     * @return Output tensor
     */
    Tensor forward(const Tensor& input, int layer_idx);
    
    /**
     * @brief Backward pass (only through LoRA adapters)
     * @param grad_output Gradient from next layer
     * @param layer_idx Layer index
     * @return Gradient w.r.t. input
     */
    Tensor backward(const Tensor& grad_output, int layer_idx);
    
    /**
     * @brief Get all trainable parameters (LoRA only)
     * @return Vector of parameter pointers
     */
    std::vector<Tensor*> getTrainableParameters();
    
    /**
     * @brief Get LoRA parameter count
     * @return Number of trainable parameters
     */
    size_t getLoRAParameterCount() const;
    
    /**
     * @brief Get base model parameter count
     * @return Number of frozen parameters
     */
    size_t getBaseModelParameterCount() const;
    
    /**
     * @brief Export LoRA adapter weights
     * @return Map of layer name to (B, A) matrices
     */
    std::unordered_map<std::string, std::pair<Tensor, Tensor>> exportLoRAWeights() const;
    
    /**
     * @brief Import LoRA adapter weights (for resuming training)
     * @param weights Map of layer name to (B, A) matrices
     * @return true if successful
     */
    bool importLoRAWeights(
        const std::unordered_map<std::string, std::pair<Tensor, Tensor>>& weights);
    
    /**
     * @brief Check if initialized
     * @return true if ready for training
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get configuration
     * @return Current configuration
     */
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;
    bool initialized_;
    
    std::unique_ptr<BaseModelAdapter> base_model_;
    std::unordered_map<std::string, std::unique_ptr<LoRALayer>> lora_layers_;
    
    // Layer information for efficient lookup
    std::vector<BaseLayerInfo> active_layers_;
    
    // Helper methods
    bool createLoRAAdapters();
    Tensor computeBaseOutput(const Tensor& input, const std::string& layer_name);
    Tensor computeLoRAOutput(const Tensor& input, const std::string& layer_name);
};

} // namespace lora
} // namespace llm
} // namespace themis
