/**
 * @file quantized_model.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=4, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/quantized_model.h"
#include "llm/gguf_loader.h"
#include "llm/lora_framework/gguf_converter.h"

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
namespace spdlog {
    template<typename... Args>
    inline void debug(const char*, Args&&...) {}
    template<typename... Args>
    inline void info(const char*, Args&&...) {}
    template<typename... Args>
    inline void warn(const char*, Args&&...) {}
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

QuantizedLayerWeights::QuantizedLayerWeights(QuantizedTensor&& quantized, 
                                             const std::vector<size_t>& original_shape)
    : quantized_(std::move(quantized)), original_shape_(original_shape) {
    
    spdlog::debug("Created QuantizedLayerWeights from pre-quantized tensor: {} bytes",
                  quantized_.memory_bytes());
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

void QuantizedModel::add_quantized_layer(const std::string& layer_name, 
                                         QuantizedLayerWeights&& quantized_weights) {
    spdlog::debug("Adding pre-quantized layer '{}' to quantized model", layer_name);
    layers_.emplace(layer_name, std::move(quantized_weights));
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
    
    // Gradient w.r.t. B: input.T @ scaled_grad @ A.T
    // Shapes: (in_dim,batch) @ (batch,out_dim) @ (out_dim,rank) -> (in_dim,rank)
    Tensor A_T = A_->transpose();
    Tensor grad_B = input_T.matmul(scaled_grad).matmul(A_T);
    
    // Store gradients
    A_->grad = std::make_unique<Tensor>(std::move(grad_A));
    B_->grad = std::make_unique<Tensor>(std::move(grad_B));
    
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

float calculate_memory_reduction(size_t /*original_bytes*/, QuantizationType quant_type) {
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

QuantizedModel load_from_gguf(
    const std::string& gguf_path,
    const QuantizedModelConfig* config) {
    
    spdlog::info("Loading GGUF model from: {}", gguf_path);
    
    // Parse GGUF file
    llm::GGUFLoader loader;
    if (!loader.parseFile(gguf_path)) {
        throw std::runtime_error("Failed to parse GGUF file: " + gguf_path);
    }
    
    const auto& metadata = loader.getMetadata();
    spdlog::info("GGUF version: {}, architecture: {}, tensors: {}",
                 metadata.version, metadata.architecture, metadata.tensors.size());
    
    // Determine quantization config
    QuantizedModelConfig model_config;
    if (config) {
        model_config = *config;
    } else {
        // Infer from first tensor type
        if (!metadata.tensors.empty()) {
            auto internal_type = GGUFConverter::getInternalType(metadata.tensors[0].type);
            model_config.quantization_type = internal_type;
        }
    }
    
    // Create quantized model
    QuantizedModel model(model_config);
    
    // Load and convert each tensor
    size_t converted = 0;
    size_t skipped = 0;
    
    for (const auto& tensor_info : metadata.tensors) {
        // Check if conversion is supported
        if (!GGUFConverter::isSupported(tensor_info.type)) {
            spdlog::debug("Skipping unsupported tensor: {} (type: {})",
                         tensor_info.name, tensor_info.type_string());
            skipped++;
            continue;
        }
        
        // Get tensor data from mmap
        void* tensor_data = loader.mmapTensor(tensor_info.name);
        if (!tensor_data) {
            spdlog::warn("Failed to mmap tensor: {}", tensor_info.name);
            skipped++;
            continue;
        }
        
        try {
            // Convert based on type
            if (tensor_info.type == llm::GGMLType::F16 || 
                tensor_info.type == llm::GGMLType::F32) {
                // Full precision - convert to FP32 and quantize via normal path
                std::vector<float> fp32_data;
                if (tensor_info.type == llm::GGMLType::F16) {
                    fp32_data = GGUFConverter::convertF16(tensor_data, tensor_info);
                } else {
                    fp32_data = GGUFConverter::convertF32(tensor_data, tensor_info);
                }
                
                // Create Tensor and add to model
                std::vector<size_t> shape;
                for (auto dim : tensor_info.shape) {
                    shape.push_back(static_cast<size_t>(dim));
                }
                Tensor tensor(shape);
                tensor.data() = std::move(fp32_data);
                
                model.add_layer(tensor_info.name, tensor);
                spdlog::debug("Loaded FP tensor: {} -> quantized", tensor_info.name);
                converted++;
                
            } else if (tensor_info.type == llm::GGMLType::Q4_K || 
                      tensor_info.type == llm::GGMLType::Q8_0) {
                // Pre-quantized formats - directly convert to QuantizedTensor
                // Uses DIRECT conversion (Q4_K → NF4 or Q8_0 → INT8) without FP32 intermediate
                // This preserves original quantization quality and avoids precision loss
                QuantizedTensor quantized_tensor;
                std::vector<size_t> shape;
                
                for (auto dim : tensor_info.shape) {
                    shape.push_back(static_cast<size_t>(dim));
                }
                
                if (tensor_info.type == llm::GGMLType::Q4_K) {
                    quantized_tensor = GGUFConverter::convertQ4KM(tensor_data, tensor_info);
                } else {
                    quantized_tensor = GGUFConverter::convertQ8_0(tensor_data, tensor_info);
                }
                
                // Create QuantizedLayerWeights and add directly to model
                QuantizedLayerWeights layer_weights(std::move(quantized_tensor), shape);
                model.add_quantized_layer(tensor_info.name, std::move(layer_weights));
                
                spdlog::debug("Loaded pre-quantized tensor: {} (using real quantized weights)",
                             tensor_info.name);
                converted++;
            }
            
        } catch (const std::exception& e) {
            spdlog::warn("Failed to convert tensor {}: {}", tensor_info.name, e.what());
            skipped++;
        }
    }
    
    spdlog::info("GGUF load complete: {} tensors converted, {} skipped",
                 converted, skipped);
    
    if (converted == 0) {
        throw std::runtime_error("No tensors could be converted from GGUF file");
    }
    
    return model;
}

} // namespace quantized_model_utils

} // namespace lora
} // namespace llm
} // namespace themis
