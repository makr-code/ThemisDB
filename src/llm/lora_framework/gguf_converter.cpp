#include "llm/lora_framework/gguf_converter.h"
#include <cstring>
#include <stdexcept>
#include <cmath>

namespace themis {
namespace llm {
namespace lora {

// FP16 to FP32 conversion helper
float GGUFConverter::fp16_to_fp32(uint16_t h) {
    // Extract sign, exponent, and mantissa
    uint32_t sign = (h & 0x8000) << 16;
    uint32_t exponent = (h & 0x7C00) >> 10;
    uint32_t mantissa = (h & 0x03FF);
    
    uint32_t result;
    
    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            result = sign;
        } else {
            // Denormalized number
            exponent = 1;
            while ((mantissa & 0x0400) == 0) {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x03FF;
            result = sign | ((exponent + 112) << 23) | (mantissa << 13);
        }
    } else if (exponent == 31) {
        // Infinity or NaN
        result = sign | 0x7F800000 | (mantissa << 13);
    } else {
        // Normalized number
        result = sign | ((exponent + 112) << 23) | (mantissa << 13);
    }
    
    float f;
    std::memcpy(&f, &result, sizeof(float));
    return f;
}

size_t GGUFConverter::calculateElements(const std::vector<int64_t>& shape) {
    size_t total = 1;
    for (auto dim : shape) {
        total *= static_cast<size_t>(dim);
    }
    return total;
}

bool GGUFConverter::isSupported(GGMLType type) {
    switch (type) {
        case GGMLType::F32:
        case GGMLType::F16:
        case GGMLType::Q4_K:  // Q4_K_M
        case GGMLType::Q8_0:
            return true;
        default:
            return false;
    }
}

QuantizationType GGUFConverter::getInternalType(GGMLType type) {
    switch (type) {
        case GGMLType::Q4_K: return QuantizationType::NF4;
        case GGMLType::Q8_0: return QuantizationType::INT8;
        case GGMLType::F32:
        case GGMLType::F16:
        default:
            return QuantizationType::NONE;
    }
}

std::vector<float> GGUFConverter::dequantizeQ4KM(const void* data, size_t num_elements) {
    std::vector<float> output(num_elements);
    const uint8_t* src = static_cast<const uint8_t*>(data);
    
    constexpr size_t BLOCK_SIZE = 256;
    size_t num_blocks = (num_elements + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    for (size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
        const gguf_blocks::Q4KBlock* block = 
            reinterpret_cast<const gguf_blocks::Q4KBlock*>(src + block_idx * sizeof(gguf_blocks::Q4KBlock));
        
        // Convert FP16 scales to FP32
        float d = fp16_to_fp32(block->d);
        float dmin = fp16_to_fp32(block->dmin);
        
        // Dequantize each value in the block
        size_t block_start = block_idx * BLOCK_SIZE;
        size_t block_elements = std::min(BLOCK_SIZE, num_elements - block_start);
        
        for (size_t i = 0; i < block_elements; ++i) {
            // Extract 4-bit value
            size_t byte_idx = i / 2;
            uint8_t byte = block->qs[byte_idx];
            uint8_t nibble = (i % 2 == 0) ? (byte & 0x0F) : (byte >> 4);
            
            // Extract scale for this sub-block (8 sub-blocks of 32 values each)
            size_t scale_idx = i / 32;
            uint8_t scale_byte = block->scales[scale_idx / 2];
            uint8_t scale = (scale_idx % 2 == 0) ? (scale_byte & 0x0F) : (scale_byte >> 4);
            
            // Dequantize: value = (q - 8) * scale * d + dmin
            float q = static_cast<float>(nibble);
            float s = static_cast<float>(scale);
            output[block_start + i] = (q - 8.0f) * s * d + dmin;
        }
    }
    
    return output;
}

std::vector<float> GGUFConverter::dequantizeQ8_0(const void* data, size_t num_elements) {
    std::vector<float> output(num_elements);
    const uint8_t* src = static_cast<const uint8_t*>(data);
    
    constexpr size_t BLOCK_SIZE = 32;
    size_t num_blocks = (num_elements + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    for (size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
        const gguf_blocks::Q8_0Block* block = 
            reinterpret_cast<const gguf_blocks::Q8_0Block*>(src + block_idx * sizeof(gguf_blocks::Q8_0Block));
        
        // Convert FP16 scale to FP32
        float d = fp16_to_fp32(block->d);
        
        // Dequantize each value in the block
        size_t block_start = block_idx * BLOCK_SIZE;
        size_t block_elements = std::min(BLOCK_SIZE, num_elements - block_start);
        
        for (size_t i = 0; i < block_elements; ++i) {
            // Dequantize: value = q * scale
            float q = static_cast<float>(block->qs[i]);
            output[block_start + i] = q * d;
        }
    }
    
    return output;
}

QuantizedTensor GGUFConverter::convertQ4KM(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    if (tensor_info.type != GGMLType::Q4_K) {
        throw std::invalid_argument("Tensor is not Q4_K_M type");
    }
    
    // Calculate number of elements
    size_t num_elements = calculateElements(tensor_info.shape);
    
    // Dequantize GGUF Q4_K_M to FP32
    std::vector<float> fp32_data = dequantizeQ4KM(gguf_data, num_elements);
    
    // Convert shape to size_t vector
    std::vector<size_t> shape;
    for (auto dim : tensor_info.shape) {
        shape.push_back(static_cast<size_t>(dim));
    }
    
    // Quantize to internal NF4 format
    QuantizedTensor result(QuantizationType::NF4, shape, 64);
    quantization::quantize_nf4(fp32_data, result);
    
    return result;
}

QuantizedTensor GGUFConverter::convertQ8_0(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    if (tensor_info.type != GGMLType::Q8_0) {
        throw std::invalid_argument("Tensor is not Q8_0 type");
    }
    
    // Calculate number of elements
    size_t num_elements = calculateElements(tensor_info.shape);
    
    // Dequantize GGUF Q8_0 to FP32
    std::vector<float> fp32_data = dequantizeQ8_0(gguf_data, num_elements);
    
    // Convert shape to size_t vector
    std::vector<size_t> shape;
    for (auto dim : tensor_info.shape) {
        shape.push_back(static_cast<size_t>(dim));
    }
    
    // Quantize to internal INT8 format
    QuantizedTensor result(QuantizationType::INT8, shape, 64);
    quantization::quantize_int8(fp32_data, result);
    
    return result;
}

std::vector<float> GGUFConverter::convertF16(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    if (tensor_info.type != GGMLType::F16) {
        throw std::invalid_argument("Tensor is not F16 type");
    }
    
    size_t num_elements = calculateElements(tensor_info.shape);
    std::vector<float> output(num_elements);
    
    const uint16_t* src = static_cast<const uint16_t*>(gguf_data);
    for (size_t i = 0; i < num_elements; ++i) {
        output[i] = fp16_to_fp32(src[i]);
    }
    
    return output;
}

std::vector<float> GGUFConverter::convertF32(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    if (tensor_info.type != GGMLType::F32) {
        throw std::invalid_argument("Tensor is not F32 type");
    }
    
    size_t num_elements = calculateElements(tensor_info.shape);
    std::vector<float> output(num_elements);
    
    const float* src = static_cast<const float*>(gguf_data);
    std::memcpy(output.data(), src, num_elements * sizeof(float));
    
    return output;
}

} // namespace lora
} // namespace llm
} // namespace themis
