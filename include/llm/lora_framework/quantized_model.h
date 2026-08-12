/**
 * @file quantized_model.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/quantization.h"
#include "llm/lora_framework/lora_layers.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Configuration for quantized model
 */
struct QuantizedModelConfig {
    QuantizationType quantization_type = QuantizationType::NF4;
    size_t block_size = 64;  // Elements per quantization block
    bool use_double_quantization = false;  // Quantize the quantization constants
    bool layer_by_layer = true;  // Quantize/dequantize layer by layer to save memory
    
    QuantizedModelConfig() = default;
};

/**
 * @brief Quantized layer weights
 * 
 * Stores a single layer's weights in quantized format.
 */
class QuantizedLayerWeights {
public:
    QuantizedLayerWeights() = default;
    
    /**
     * @brief Construct from full precision tensor
     * @param weights Full precision weights
     * @param config Quantization configuration
     */
    QuantizedLayerWeights(const Tensor& weights, const QuantizedModelConfig& config);
    
    /**
     * @brief Construct from pre-quantized tensor (e.g., from GGUF)
     * @param quantized Pre-quantized tensor
     * @param original_shape Original tensor shape
     */
    QuantizedLayerWeights(QuantizedTensor&& quantized, const std::vector<size_t>& original_shape);
    
    /**
     * @brief Dequantize weights back to full precision
     * @return Full precision tensor
     */
    Tensor dequantize() const;
    
    /**
     * @brief Get quantized tensor
     */
    const QuantizedTensor& quantized() const { return quantized_; }
    
    /**
     * @brief Memory usage in bytes
     */
    size_t memory_bytes() const;
    
    /**
     * @brief Quantization type
     */
    QuantizationType type() const { return quantized_.type(); }
    
private:
    QuantizedTensor quantized_;
    std::vector<size_t> original_shape_;
};

/**
 * @brief Quantized base model
 * 
 * Stores model weights in quantized format for memory-efficient fine-tuning.
 * Supports layer-by-layer dequantization for minimal memory footprint.
 */
class QuantizedModel {
public:
    QuantizedModel() = default;
    explicit QuantizedModel(const QuantizedModelConfig& config);
    
    /**
     * @brief Add a layer's weights to the model
     * @param layer_name Name/identifier for the layer
     * @param weights Full precision weights (will be quantized)
     */
    void add_layer(const std::string& layer_name, const Tensor& weights);
    
    /**
     * @brief Add a layer with pre-quantized weights (e.g., from GGUF)
     * @param layer_name Name/identifier for the layer
     * @param quantized_weights Pre-quantized layer weights
     */
    void add_quantized_layer(const std::string& layer_name, QuantizedLayerWeights&& quantized_weights);
    
    /**
     * @brief Get quantized weights for a layer
     * @param layer_name Layer identifier
     * @return Quantized layer weights
     */
    const QuantizedLayerWeights* get_layer(const std::string& layer_name) const;
    
    /**
     * @brief Dequantize a specific layer
     * @param layer_name Layer identifier
     * @return Full precision tensor
     */
    Tensor dequantize_layer(const std::string& layer_name) const;
    
    /**
     * @brief Get number of layers
     */
    size_t num_layers() const { return layers_.size(); }
    
    /**
     * @brief Get all layer names
     */
    std::vector<std::string> layer_names() const;
    
    /**
     * @brief Total memory usage in bytes
     */
    size_t memory_bytes() const;
    
    /**
     * @brief Configuration
     */
    const QuantizedModelConfig& config() const { return config_; }
    
    /**
     * @brief Quantization type
     */
    QuantizationType quantization_type() const { return config_.quantization_type; }
    
private:
    QuantizedModelConfig config_;
    std::unordered_map<std::string, QuantizedLayerWeights> layers_;
    
public:
    // Model metadata extracted from GGUF
    uint32_t embedding_dim = 768;          // Standard dimension, updated from GGUF metadata
    uint32_t metadata_num_layers = 32;     // Number of transformer layers (metadata)
    std::string model_type = "";          // Model architecture (llama, mistral, etc.)
};

/**
 * @brief QLoRA layer - LoRA layer that works with quantized base model
 * 
 * Combines quantized base model weights with full-precision LoRA adapters.
 * Forward pass: dequantize base weights, apply LoRA, then forward.
 * Backward pass: only compute gradients for LoRA adapters (not base).
 */
class QLoRALayer : public ITrainableLayer {
public:
    /**
     * @brief Construct QLoRA layer
     * @param in_dim Input dimension
     * @param out_dim Output dimension
     * @param rank LoRA rank
     * @param base_weights Quantized base model weights (optional)
     * @param scaling LoRA scaling factor
     */
    QLoRALayer(size_t in_dim, size_t out_dim, size_t rank,
               std::shared_ptr<QuantizedLayerWeights> base_weights = nullptr,
               float scaling = 1.0f);
    
    ~QLoRALayer() override = default;
    
    // ITrainableLayer interface
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return "QLoRALayer"; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;
    
    /**
     * @brief Set quantized base weights
     */
    void set_base_weights(std::shared_ptr<QuantizedLayerWeights> base_weights);
    
    /**
     * @brief Get LoRA weights (for checkpointing)
     */
    std::pair<Tensor, Tensor> get_lora_weights() const;
    void set_lora_weights(const Tensor& B, const Tensor& A);
    
private:
    size_t in_dim_ = 0;
    size_t out_dim_ = 0;
    size_t rank_ = 0;
    float scaling_ = 1.0f;
    
    // Quantized base model weights (frozen, not trainable)
    std::shared_ptr<QuantizedLayerWeights> base_weights_;
    
    // LoRA trainable parameters (full precision)
    std::unique_ptr<Tensor> B_;  // (in_dim, rank)
    std::unique_ptr<Tensor> A_;  // (rank, out_dim)
    
    // Cached for backward pass
    Tensor cached_input_;
    Tensor cached_BA_;
    Tensor cached_base_output_;  // Output from base model (if exists)
};

/**
 * @brief Utility functions for quantized models
 */
namespace quantized_model_utils {
    
    /**
     * @brief Estimate memory usage for quantization
     * 
     * @param num_parameters Number of parameters in model
     * @param quant_type Quantization type
     * @param block_size Block size for quantization
     * @param use_double_quant Whether to use double quantization
     * @return Estimated memory usage in bytes
     */
    size_t estimate_memory_usage(size_t num_parameters,
                                  QuantizationType quant_type,
                                  size_t block_size = 64,
                                  bool use_double_quant = false);
    
    /**
     * @brief Calculate expected memory reduction
     * 
     * @param original_bytes Original model size in bytes (FP32)
     * @param quant_type Quantization type
     * @return Reduction factor (0.0 - 1.0)
     */
    float calculate_memory_reduction(size_t original_bytes,
                                      QuantizationType quant_type);
    
    /**
     * @brief Convert full precision model to quantized
     * 
     * @param model_weights Map of layer names to full precision tensors
     * @param config Quantization configuration
     * @return Quantized model
     */
    QuantizedModel convert_to_quantized(
        const std::unordered_map<std::string, Tensor>& model_weights,
        const QuantizedModelConfig& config);
    
    /**
     * @brief Load quantized model from GGUF file
     * 
     * Loads a GGUF format file and converts quantized tensors to internal format.
     * Supports Q4_K_M (→ NF4) and Q8_0 (→ INT8) quantization types.
     * 
     * @param gguf_path Path to GGUF file
     * @param config Quantization configuration (optional, inferred from GGUF)
     * @return Quantized model
     * @throws std::runtime_error if file cannot be loaded or format is unsupported
     */
    QuantizedModel load_from_gguf(
        const std::string& gguf_path,
        const QuantizedModelConfig* config = nullptr);
    
} // namespace quantized_model_utils

} // namespace lora
} // namespace llm
} // namespace themis
