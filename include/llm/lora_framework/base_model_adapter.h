/**
 * @file base_model_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
    virtual ~BaseLayerInfo() = default;
    std::string name;              // Layer name (e.g., "layers.0.attention.wq")
    std::vector<size_t> shape;     // Tensor shape
    size_t in_features = 0;            // Input dimension
    size_t out_features = 0;           // Output dimension
    std::string layer_type;        // "attention.wq", "attention.wk", "feed_forward.w1", etc.
    int layer_idx = 0;                 // Layer index in model
};

/**
 * @brief Model architecture information
 */
struct ModelArchitectureInfo {
    virtual ~ModelArchitectureInfo() = default;
    std::string architecture;      // "llama", "mistral", "gpt-neox"
    int num_layers = 0;                // Number of transformer layers
    int hidden_size = 0;               // Hidden dimension
    int num_attention_heads = 0;       // Number of attention heads
    int intermediate_size = 0;         // MLP intermediate size
    int vocab_size = 0;                // Vocabulary size
    std::string rope_scaling_type; // RoPE scaling type
    float rope_freq_base = 0.0f;          // RoPE frequency base
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
    const ModelArchitectureInfo& getArchitecture() const { return architecture_; }
    
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
    const std::string& getModelName() const { return model_name_; }
    
    /**
     * @brief Get model path
     * @return Model file path
     */
    const std::string& getModelPath() const { return model_path_; }
    
    /**
     * @brief Get total number of parameters in base model
     * @return Parameter count
     */
    size_t getTotalParameters() const;
    
    /**
     * @brief Unload model and free resources
     */
    void unload();
    
    /**
     * @brief Extract raw token embedding vector from embedding matrix
     * 
     * This extracts the embedding from the model's embedding layer (first layer),
     * NOT the contextualized sequence embeddings from llama_get_embeddings().
     * 
     * For LoRA training, we need these raw token embeddings as inputs to individual
     * layers, not the final model output.
     * 
     * @param token_id Token ID to extract embedding for
     * @return Embedding vector (size = model's hidden_dim), empty if failed
     */
    std::vector<float> getTokenEmbedding(int token_id) const;
    
    /**
     * @brief Extract raw token embeddings for multiple tokens (batched)
     * 
     * Batch version of getTokenEmbedding() for efficiency.
     * Returns raw embedding layer weights, not contextualized embeddings.
     * 
     * @param token_ids Vector of token IDs
     * @return Flattened embedding matrix [num_tokens * hidden_dim]
     */
    std::vector<float> getTokenEmbeddings(const std::vector<int>& token_ids) const;
    
    /**
     * @brief Get pointer to full embedding matrix (read-only)
     * 
     * Direct access to the embedding layer weight matrix.
     * Use this for efficient access when processing many tokens.
     * 
     * @return Pointer to embedding matrix or nullptr if not available
     * @note Matrix is [vocab_size * hidden_dim], row-major layout
     * @note This is the raw embedding matrix, not contextualized embeddings
     */
    const float* getEmbeddingMatrix() const;
    
    /**
     * @brief Get embedding cache statistics
     */
    void logCacheStats() const;
    
    /**
     * @brief Get vocabulary size from model architecture
     * @return Vocabulary size
     */
    int getVocabSize() const { return architecture_.vocab_size; }
    
    /**
     * @brief Get hidden dimension from model architecture
     * @return Hidden dimension
     */
    int getHiddenSize() const { return architecture_.hidden_size; }
    
private:
    std::unique_ptr<GGUFLoader> gguf_loader_;
    std::string model_path_;
    std::string model_name_;
    bool model_loaded_ = false;
    
    ModelArchitectureInfo architecture_;
    std::vector<BaseLayerInfo> adaptable_layers_;
    
    // Cached layer information for fast lookup
    std::unordered_map<std::string, BaseLayerInfo> layer_map_;
    
    // Embedding cache for performance
    mutable std::unordered_map<int, std::vector<float>> embedding_cache_;
    mutable size_t cache_hits_ = 0;
    mutable size_t cache_misses_ = 0;
    static constexpr size_t MAX_CACHE_SIZE = 10000;  // Cache top 10k tokens
    
    // Cached embedding matrix pointer (mmap'd or loaded)
    mutable const float* embedding_matrix_ = nullptr;
    mutable std::string embedding_tensor_name_;
    
    // Helper methods
    bool parseArchitecture();
    bool identifyAdaptableLayers();
    bool parseLayerInfo(const TensorMetadata& tensor, BaseLayerInfo& layer_info);
    std::string standardizeLayerName(const std::string& model_layer_name) const;
    bool matchesTargetModule(const std::string& layer_name, 
                            const std::string& target_pattern) const;
    
    // Embedding extraction helpers
    std::string findEmbeddingTensorName() const;
    std::vector<float> extractEmbeddingFromGGUF(int token_id) const;
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
     * @brief Get base model adapter (for accessing embeddings, etc.)
     * @return Pointer to base model adapter, nullptr if not initialized
     */
    const BaseModelAdapter* getBaseModel() const { return base_model_.get(); }
    
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
    bool initialized_ = false;
    
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

