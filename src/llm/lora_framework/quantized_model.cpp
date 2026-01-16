#include "llm/lora_framework/quantized_model.h"

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
namespace spdlog {
    template<typename... Args>
    inline void debug(const char*, Args&&...) {}
    template<typename... Args>
    inline void info(const char*, Args&&...) {}
}
#endif

#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

// ===== QuantizedLayerWeights Implementation =====

QuantizedLayerWeights::QuantizedLayerWeights(const Tensor& weights,
                                             const QuantizedModelConfig& config)
    : original_shape_(weights.shape()) {
    
    // Flatten tensor data for quantization
    const auto& data = weights.data();
    
    // Create quantized tensor
    quantized_ = QuantizedTensor(config.quantization_type, original_shape_, config.block_size);
    
    // Quantize based on type
    if (config.quantization_type == QuantizationType::NF4) {
        quantization::quantize_nf4(data, quantized_, config.block_size);
    } else if (config.quantization_type == QuantizationType::INT8) {
        quantization::quantize_int8(data, quantized_, config.block_size);
    } else {
        throw std::invalid_argument("Unsupported quantization type");
    }
    
    spdlog::debug("Quantized layer weights: {} -> {} bytes ({:.1f}% reduction)",
                  data.size() * sizeof(float),
                  quantized_.memory_bytes(),
                  (1.0f - static_cast<float>(quantized_.memory_bytes()) / 
                   (data.size() * sizeof(float))) * 100.0f);
}

Tensor QuantizedLayerWeights::dequantize() const {
    std::vector<float> data;
    quantization::dequantize(quantized_, data);
    
    Tensor result(original_shape_);
    result.data() = std::move(data);
    return result;
}

size_t QuantizedLayerWeights::memory_bytes() const {
    return quantized_.memory_bytes();
}

// ===== QuantizedModel Implementation =====

QuantizedModel::QuantizedModel(const QuantizedModelConfig& config)
    : config_(config) {
}

void QuantizedModel::add_layer(const std::string& layer_name, const Tensor& weights) {
    spdlog::debug("Adding layer '{}' to quantized model", layer_name);
    layers_.emplace(layer_name, QuantizedLayerWeights(weights, config_));
}

const QuantizedLayerWeights* QuantizedModel::get_layer(const std::string& layer_name) const {
    auto it = layers_.find(layer_name);
    if (it == layers_.end()) {
        return nullptr;
    }
    return &it->second;
}

Tensor QuantizedModel::dequantize_layer(const std::string& layer_name) const {
    auto layer = get_layer(layer_name);
    if (!layer) {
        throw std::invalid_argument("Layer not found: " + layer_name);
    }
    return layer->dequantize();
}

std::vector<std::string> QuantizedModel::layer_names() const {
    std::vector<std::string> names;
    names.reserve(layers_.size());
    for (const auto& pair : layers_) {
        names.push_back(pair.first);
    }
    return names;
}

size_t QuantizedModel::memory_bytes() const {
    size_t total = 0;
    for (const auto& pair : layers_) {
        total += pair.second.memory_bytes();
    }
    return total;
}

// ===== QLoRALayer Implementation =====

QLoRALayer::QLoRALayer(size_t in_dim, size_t out_dim, size_t rank,
                       std::shared_ptr<QuantizedLayerWeights> base_weights,
                       float scaling)
    : in_dim_(in_dim), out_dim_(out_dim), rank_(rank), scaling_(scaling),
      base_weights_(base_weights) {
    
    // Initialize LoRA matrices (same as regular LoRA)
    // B: Kaiming initialization, A: zeros
    B_ = std::make_unique<Tensor>(tensor_utils::kaiming_uniform({in_dim, rank}));
    A_ = std::make_unique<Tensor>(tensor_utils::zeros({rank, out_dim}));
    
    B_->requires_grad = true;
    A_->requires_grad = true;
    
    spdlog::debug("Created QLoRALayer: {}x{} (rank={}), base_weights={}",
                  in_dim, out_dim, rank, base_weights ? "yes" : "no");
}

Tensor QLoRALayer::forward(const Tensor& input) {
    cached_input_ = input.clone();
    
    // Compute LoRA part: BA = B @ A
    cached_BA_ = B_->matmul(*A_);
    
    // Apply LoRA to input
    Tensor lora_output = input.matmul(cached_BA_) * scaling_;
    
    // If we have quantized base weights, add base model output
    if (base_weights_) {
        // Dequantize base weights on-the-fly
        Tensor base_W = base_weights_->dequantize();
        
        // Compute base output: input @ W_base
        cached_base_output_ = input.matmul(base_W);
        
        // Combine: output = base_output + lora_output
        return cached_base_output_ + lora_output;
    } else {
        // No base weights, just return LoRA output
        return lora_output;
    }
}

Tensor QLoRALayer::backward(const Tensor& grad_output) {
    // Backward pass only for LoRA parameters (base model is frozen)
    
    Tensor scaled_grad = grad_output * scaling_;
    
    // Gradient w.r.t. A: B.T @ (input.T @ scaled_grad)
    Tensor B_T = B_->transpose();
    Tensor input_T = cached_input_.transpose();
    Tensor grad_A = B_T.matmul(input_T.matmul(scaled_grad));
    
    // Gradient w.r.t. B: (scaled_grad @ A.T) @ input.T
    Tensor A_T = A_->transpose();
    Tensor grad_B = (scaled_grad.matmul(A_T)).matmul(input_T);
    
    // Store gradients
    A_->grad = grad_A;
    B_->grad = grad_B;
    
    // Gradient w.r.t. input
    Tensor BA_T = cached_BA_.transpose();
    Tensor grad_input = scaled_grad.matmul(BA_T);
    
    // If we have base weights, also propagate through base
    // (but we don't compute gradients for base weights - they're frozen)
    if (base_weights_) {
        Tensor base_W = base_weights_->dequantize();
        Tensor base_W_T = base_W.transpose();
        Tensor grad_input_base = grad_output.matmul(base_W_T);
        grad_input = grad_input + grad_input_base;
    }
    
    return grad_input;
}

std::vector<Tensor*> QLoRALayer::parameters() {
    return {B_.get(), A_.get()};
}

size_t QLoRALayer::parameter_count() const {
    return (in_dim_ * rank_) + (rank_ * out_dim_);
}

size_t QLoRALayer::memory_bytes() const {
    size_t lora_bytes = parameter_count() * sizeof(float);
    size_t base_bytes = base_weights_ ? base_weights_->memory_bytes() : 0;
    return lora_bytes + base_bytes;
}

void QLoRALayer::set_base_weights(std::shared_ptr<QuantizedLayerWeights> base_weights) {
    base_weights_ = base_weights;
}

std::pair<Tensor, Tensor> QLoRALayer::get_lora_weights() const {
    return {B_->clone(), A_->clone()};
}

void QLoRALayer::set_lora_weights(const Tensor& B, const Tensor& A) {
    *B_ = B.clone();
    *A_ = A.clone();
}

// ===== Utility Functions =====

namespace quantized_model_utils {

size_t estimate_memory_usage(size_t num_parameters,
                              QuantizationType quant_type,
                              size_t block_size,
                              bool use_double_quant) {
    
    size_t data_bytes = 0;
    size_t num_blocks = (num_parameters + block_size - 1) / block_size;
    
    // Data storage
    if (quant_type == QuantizationType::NF4) {
        data_bytes = (num_parameters + 1) / 2;  // 4 bits per value
    } else if (quant_type == QuantizationType::INT8) {
        data_bytes = num_parameters;  // 8 bits per value
    } else {
        data_bytes = num_parameters * sizeof(float);  // Fallback
    }
    
    // Block parameters
    size_t block_bytes;
    if (use_double_quant) {
        // 2 uint8 per block + 2 global floats
        block_bytes = num_blocks * 2 + 2 * sizeof(float);
    } else {
        // 2 floats per block (scale + zero_point)
        block_bytes = num_blocks * 2 * sizeof(float);
    }
    
    return data_bytes + block_bytes;
}

float calculate_memory_reduction(size_t original_bytes, QuantizationType quant_type) {
    // Rough estimates based on typical block sizes
    float reduction = 0.0f;
    
    switch (quant_type) {
        case QuantizationType::NF4:
            reduction = 0.80f;  // ~80% reduction (4 bits + overhead)
            break;
        case QuantizationType::INT8:
            reduction = 0.70f;  // ~70% reduction (8 bits + overhead)
            break;
        case QuantizationType::Q4_K_M:
            reduction = 0.75f;  // ~75% reduction
            break;
        case QuantizationType::Q8_0:
            reduction = 0.65f;  // ~65% reduction
            break;
        default:
            reduction = 0.0f;   // No reduction
            break;
    }
    
    return reduction;
}

QuantizedModel convert_to_quantized(
    const std::unordered_map<std::string, Tensor>& model_weights,
    const QuantizedModelConfig& config) {
    
    QuantizedModel model(config);
    
    spdlog::info("Converting model to quantized format: {} layers", model_weights.size());
    
    for (const auto& pair : model_weights) {
        model.add_layer(pair.first, pair.second);
    }
    
    spdlog::info("Quantization complete: {} bytes", model.memory_bytes());
    
    return model;
}

} // namespace quantized_model_utils

} // namespace lora
} // namespace llm
} // namespace themis
