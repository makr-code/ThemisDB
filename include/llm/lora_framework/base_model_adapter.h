/**
 * @file base_model_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct BaseLayerInfo {
    /**
     * @brief Base Layer Info.
     * @return Return value.
     */
    virtual ~BaseLayerInfo() = default;
    std::string name;              // Layer name (e.g., "layers.0.attention.wq")
    std::vector<size_t> shape;     // Tensor shape
    size_t in_features = 0;            // Input dimension
    size_t out_features = 0;           // Output dimension
    std::string layer_type;        // "attention.wq", "attention.wk", "feed_forward.w1", etc.
    int layer_idx = 0;                 // Layer index in model
};

struct ModelArchitectureInfo {
    /**
     * @brief Model Architecture Info.
     * @return Return value.
     */
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

class BaseModelAdapter {
public:
    BaseModelAdapter();
    ~BaseModelAdapter();
    
    /**
     * @brief Load Model.
     * @param[in] model_path Path to the model.
     * @return True when the operation succeeds.
     */
    bool loadModel(const std::string& model_path);
    
    const ModelArchitectureInfo& getArchitecture() const { return architecture_; }
    
    /**
     * @brief Get Adaptable Layers.
     * @return Return value.
     */
    std::vector<BaseLayerInfo> getAdaptableLayers() const;
    
    /**
     * @brief Get Layers By Target Modules.
     * @param[in] target_modules Input parameter.
     * @return Return value.
     */
    std::vector<BaseLayerInfo> getLayersByTargetModules(
        const std::vector<std::string>& target_modules) const;
    
    /**
     * @brief Get Layer Weights.
     * @param[in] layer_name Name of the layer.
     * @return Return value.
     */
    std::optional<Tensor> getLayerWeights(const std::string& layer_name) const;
    
    bool isLoaded() const { return model_loaded_; }
    
    const std::string& getModelName() const { return model_name_; }
    
    const std::string& getModelPath() const { return model_path_; }
    
    /**
     * @brief Get Total Parameters.
     * @return Return value.
     */
    size_t getTotalParameters() const;
    
    /**
     * @brief Unload.
     */
    void unload();
    
    /**
     * @brief Get Token Embedding.
     * @param[in] token_id Identifier of the token.
     * @return Return value.
     */
    std::vector<float> getTokenEmbedding(int token_id) const;
    
    /**
     * @brief Get Token Embeddings.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    std::vector<float> getTokenEmbeddings(const std::vector<int>& token_ids) const;
    
    /**
     * @brief Get Embedding Matrix.
     * @return Pointer to the result.
     */
    const float* getEmbeddingMatrix() const;
    
    /**
     * @brief Log Cache Stats.
     */
    void logCacheStats() const;
    
    int getVocabSize() const { return architecture_.vocab_size; }
    
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
    /**
     * @brief Parse Architecture.
     * @return True when the operation succeeds.
     */
    bool parseArchitecture();
    /**
     * @brief Identify Adaptable Layers.
     * @return True when the operation succeeds.
     */
    bool identifyAdaptableLayers();
    /**
     * @brief Parse Layer Info.
     * @param[in] tensor Input parameter.
     * @param[in,out] layer_info Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseLayerInfo(const TensorMetadata& tensor, BaseLayerInfo& layer_info);
    /**
     * @brief Standardize Layer Name.
     * @param[in] model_layer_name Name of the model layer.
     * @return Return value.
     */
    std::string standardizeLayerName(const std::string& model_layer_name) const;
    /**
     * @brief Matches Target Module.
     * @param[in] layer_name Name of the layer.
     * @param[in] target_pattern Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchesTargetModule(const std::string& layer_name, 
                            const std::string& target_pattern) const;
    
    // Embedding extraction helpers
    /**
     * @brief Find Embedding Tensor Name.
     * @return Return value.
     */
    std::string findEmbeddingTensorName() const;
    /**
     * @brief Extract Embedding From GGUF.
     * @param[in] token_id Identifier of the token.
     * @return Return value.
     */
    std::vector<float> extractEmbeddingFromGGUF(int token_id) const;
};

class LoRAEnhancedModel {
public:
    struct Config {
        std::string base_model_path;
        LoRAHyperparameters lora_config;
        std::vector<std::string> target_modules;
        bool freeze_base_model = true;    // Should always be true for LoRA
        bool use_gradient_checkpointing = false;
    };
    
    /**
     * @brief Lo RAEnhanced Model.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRAEnhancedModel(const Config& config);
    ~LoRAEnhancedModel();
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    /**
     * @brief Forward.
     * @param[in] input Input parameter.
     * @param[in] layer_idx Input parameter.
     * @return Return value.
     */
    Tensor forward(const Tensor& input, int layer_idx);
    
    /**
     * @brief Backward.
     * @param[in] grad_output Input parameter.
     * @param[in] layer_idx Input parameter.
     * @return Return value.
     */
    Tensor backward(const Tensor& grad_output, int layer_idx);
    
    /**
     * @brief Get Trainable Parameters.
     * @return Return value.
     */
    std::vector<Tensor*> getTrainableParameters();
    
    /**
     * @brief Get Lo RAParameter Count.
     * @return Return value.
     */
    size_t getLoRAParameterCount() const;
    
    /**
     * @brief Get Base Model Parameter Count.
     * @return Return value.
     */
    size_t getBaseModelParameterCount() const;
    
    const BaseModelAdapter* getBaseModel() const { return base_model_.get(); }
    
    std::unordered_map<std::string, std::pair<Tensor, Tensor>> exportLoRAWeights() const;
    
    bool importLoRAWeights(
        const std::unordered_map<std::string, std::pair<Tensor, Tensor>>& weights);
    
    bool isInitialized() const { return initialized_; }
    
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;
    bool initialized_ = false;
    
    std::unique_ptr<BaseModelAdapter> base_model_;
    std::unordered_map<std::string, std::unique_ptr<LoRALayer>> lora_layers_;
    
    // Layer information for efficient lookup
    std::vector<BaseLayerInfo> active_layers_;
    
    // Helper methods
    /**
     * @brief Create Lo RAAdapters.
     * @return True when the operation succeeds.
     */
    bool createLoRAAdapters();
    /**
     * @brief Compute Base Output.
     * @param[in] input Input parameter.
     * @param[in] layer_name Name of the layer.
     * @return Return value.
     */
    Tensor computeBaseOutput(const Tensor& input, const std::string& layer_name);
    /**
     * @brief Compute Lo RAOutput.
     * @param[in] input Input parameter.
     * @param[in] layer_name Name of the layer.
     * @return Return value.
     */
    Tensor computeLoRAOutput(const Tensor& input, const std::string& layer_name);
};

} // namespace lora
} // namespace llm
} // namespace themis

