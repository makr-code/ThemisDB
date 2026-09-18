/**
 * @file quantized_model.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct QuantizedModelConfig {
    QuantizationType quantization_type = QuantizationType::NF4;
    size_t block_size = 64;  // Elements per quantization block
    bool use_double_quantization = false;  // Quantize the quantization constants
    bool layer_by_layer = true;  // Quantize/dequantize layer by layer to save memory
    
    QuantizedModelConfig() = default;
};

class QuantizedLayerWeights {
public:
    QuantizedLayerWeights() = default;
    
    QuantizedLayerWeights(const Tensor& weights, const QuantizedModelConfig& config);
    
    QuantizedLayerWeights(QuantizedTensor&& quantized, const std::vector<size_t>& original_shape);
    
    /**
     * @brief Dequantize.
     * @return Return value.
     */
    Tensor dequantize() const;
    
    const QuantizedTensor& quantized() const { return quantized_; }
    
    /**
     * @brief Memory bytes.
     * @return Return value.
     */
    size_t memory_bytes() const;
    
    QuantizationType type() const { return quantized_.type(); }
    
private:
    QuantizedTensor quantized_;
    std::vector<size_t> original_shape_;
};

class QuantizedModel {
public:
    QuantizedModel() = default;
    /**
     * @brief Quantized Model.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit QuantizedModel(const QuantizedModelConfig& config);
    
    /**
     * @brief Add layer.
     * @param[in] layer_name Name of the layer.
     * @param[in] weights Input parameter.
     */
    void add_layer(const std::string& layer_name, const Tensor& weights);
    
    /**
     * @brief Add quantized layer.
     * @param[in] layer_name Name of the layer.
     * @param[in] quantized_weights Input parameter.
     */
    void add_quantized_layer(const std::string& layer_name, QuantizedLayerWeights&& quantized_weights);
    
    /**
     * @brief Get layer.
     * @param[in] layer_name Name of the layer.
     * @return Pointer to the result.
     */
    const QuantizedLayerWeights* get_layer(const std::string& layer_name) const;
    
    /**
     * @brief Dequantize layer.
     * @param[in] layer_name Name of the layer.
     * @return Return value.
     */
    Tensor dequantize_layer(const std::string& layer_name) const;
    
    size_t num_layers() const { return layers_.size(); }
    
    /**
     * @brief Layer names.
     * @return Return value.
     */
    std::vector<std::string> layer_names() const;
    
    /**
     * @brief Memory bytes.
     * @return Return value.
     */
    size_t memory_bytes() const;
    
    const QuantizedModelConfig& config() const { return config_; }
    
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

class QLoRALayer : public ITrainableLayer {
public:
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
     * @brief Set base weights.
     * @param[in] base_weights Input parameter.
     */
    void set_base_weights(std::shared_ptr<QuantizedLayerWeights> base_weights);
    
    std::pair<Tensor, Tensor> get_lora_weights() const;
    /**
     * @brief Set lora weights.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     */
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

namespace quantized_model_utils {
    
    size_t estimate_memory_usage(size_t num_parameters,
                                  QuantizationType quant_type,
                                  size_t block_size = 64,
                                  bool use_double_quant = false);
    
    /**
     * @brief Calculate memory reduction.
     * @param[in] original_bytes Input parameter.
     * @param[in] quant_type Input parameter.
     * @return Return value.
     */
    float calculate_memory_reduction(size_t original_bytes,
                                      QuantizationType quant_type);
    
    QuantizedModel convert_to_quantized(
        const std::unordered_map<std::string, Tensor>& model_weights,
        const QuantizedModelConfig& config);
    
    QuantizedModel load_from_gguf(
        const std::string& gguf_path,
        const QuantizedModelConfig* config = nullptr);
    
} // namespace quantized_model_utils

} // namespace lora
} // namespace llm
} // namespace themis
