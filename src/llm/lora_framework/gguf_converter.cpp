/**
 * @file gguf_converter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gguf_converter.h"
#include <cstring>
#include <stdexcept>
#include <cmath>

namespace themis {
namespace llm {
namespace lora {

// FP16 to FP32 conversion helper
float GGUFConverter::fp16_to_fp32([[maybe_unused]] uint16_t h) {
    // Extract sign, exponent, and mantissa
    uint32_t sign = (h & 0x8000) << 16;
    uint32_t exponent = (h & 0x7C00) >> 10;
    uint32_t mantissa = (h & 0x03FF);
    
    uint32_t result = 0;
    
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
        [[fallthrough]];\n        case GGMLType::F16:
        [[fallthrough]];\n        case GGMLType::Q4_K:  // Q4_K_M
        [[fallthrough]];\n        case GGMLType::Q8_0:
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
        [[fallthrough]];\n        case GGMLType::F16:
        [[fallthrough]];\n        default:
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

QuantizedTensor GGUFConverter::convertQ4KM_direct(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    if (tensor_info.type != GGMLType::Q4_K) {
        throw std::invalid_argument("Tensor is not Q4_K_M type");
    }
    
    // Calculate number of elements
    size_t num_elements = calculateElements(tensor_info.shape);
    
    // Convert shape to size_t vector
    std::vector<size_t> shape;
    for (auto dim : tensor_info.shape) {
        shape.push_back(static_cast<size_t>(dim));
    }
    
    // Direct conversion: Copy GGUF Q4_K blocks to internal NF4 format
    // Q4_K_M uses 256 values per block, but we'll use our standard block size
    constexpr size_t GGUF_Q4K_BLOCK_SIZE = 256;
    size_t num_gguf_blocks = (num_elements + GGUF_Q4K_BLOCK_SIZE - 1) / GGUF_Q4K_BLOCK_SIZE;
    const uint8_t* src = static_cast<const uint8_t*>(gguf_data);
    
    // Create output tensor with NF4 quantization
    QuantizedTensor result(QuantizationType::NF4, shape, GGUF_CONVERSION_BLOCK_SIZE);
    
    // Initialize all block parameters to track scales correctly
    for (auto& block : result.blocks()) {
        block.scale = 0.0f;  // Will track maximum effective scale
        block.zero_point = 0.0f;
        block.size = 0;
    }
    
    // Process each GGUF block
    for (size_t gguf_block_idx = 0; gguf_block_idx < num_gguf_blocks; ++gguf_block_idx) {
        const gguf_blocks::Q4KBlock* gguf_block = 
            reinterpret_cast<const gguf_blocks::Q4KBlock*>(src + gguf_block_idx * sizeof(gguf_blocks::Q4KBlock));
        
        // Convert FP16 scales to FP32
        float d = fp16_to_fp32(gguf_block->d);
        float dmin = fp16_to_fp32(gguf_block->dmin);
        
        // Process values in this GGUF block (up to 256 values)
        size_t block_start = gguf_block_idx * GGUF_Q4K_BLOCK_SIZE;
        size_t block_elements = std::min(GGUF_Q4K_BLOCK_SIZE, num_elements - block_start);
        
        // Track maximum effective scale for this GGUF block
        float max_effective_scale = 0.0f;
        
        // Copy/convert each 4-bit value directly
        for (size_t i = 0; i < block_elements; ++i) {
            size_t global_idx = block_start + i;
            
            // Extract 4-bit value from GGUF format
            size_t byte_idx = i / 2;
            uint8_t byte = gguf_block->qs[byte_idx];
            uint8_t nibble = (i % 2 == 0) ? (byte & 0x0F) : (byte >> 4);
            
            // Extract scale for this sub-block
            size_t scale_idx = i / 32;
            uint8_t scale_byte = gguf_block->scales[scale_idx / 2];
            uint8_t scale_nibble = (scale_idx % 2 == 0) ? (scale_byte & 0x0F) : (scale_byte >> 4);
            
            // Store the 4-bit quantized value directly in NF4 format
            // NF4 uses similar 4-bit encoding, so we can map directly
            size_t out_byte_idx = global_idx / 2;
            if (global_idx % 2 == 0) {
                result.data()[out_byte_idx] = (result.data()[out_byte_idx] & 0xF0) | (nibble & 0x0F);
            } else {
                result.data()[out_byte_idx] = (result.data()[out_byte_idx] & 0x0F) | ((nibble & 0x0F) << 4);
            }
            
            // Track maximum effective scale for block parameter updates
            float effective_scale = d * static_cast<float>(scale_nibble);
            max_effective_scale = std::max(max_effective_scale, effective_scale);
        }
        
        // Update quantization block parameters (once per GGUF block)
        // Map GGUF blocks to our internal block structure
        size_t start_internal_block = block_start / GGUF_CONVERSION_BLOCK_SIZE;
        size_t end_internal_block = (block_start + block_elements - 1) / GGUF_CONVERSION_BLOCK_SIZE;
        
        for (size_t internal_block_idx = start_internal_block; internal_block_idx <= end_internal_block && internal_block_idx < result.blocks().size(); ++internal_block_idx) {
            // Store scale information (max effective scale)
            result.blocks()[internal_block_idx].scale = std::max(result.blocks()[internal_block_idx].scale, max_effective_scale);
            result.blocks()[internal_block_idx].zero_point = dmin;
            // Calculate block size
            size_t block_begin = internal_block_idx * GGUF_CONVERSION_BLOCK_SIZE;
            size_t block_end = std::min(block_begin + GGUF_CONVERSION_BLOCK_SIZE, num_elements);
            result.blocks()[internal_block_idx].size = block_end - block_begin;
        }
    }
    
    return result;
}

QuantizedTensor GGUFConverter::convertQ4KM(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    // Use direct conversion by default for better quality
    return convertQ4KM_direct(gguf_data, tensor_info);
}

QuantizedTensor GGUFConverter::convertQ8_0_direct(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    if (tensor_info.type != GGMLType::Q8_0) {
        throw std::invalid_argument("Tensor is not Q8_0 type");
    }
    
    // Calculate number of elements
    size_t num_elements = calculateElements(tensor_info.shape);
    
    // Convert shape to size_t vector
    std::vector<size_t> shape;
    for (auto dim : tensor_info.shape) {
        shape.push_back(static_cast<size_t>(dim));
    }
    
    // Direct conversion: Copy GGUF Q8_0 blocks to internal INT8 format
    // Q8_0 uses 32 values per block with FP16 scale
    constexpr size_t GGUF_Q8_0_BLOCK_SIZE = 32;
    size_t num_gguf_blocks = (num_elements + GGUF_Q8_0_BLOCK_SIZE - 1) / GGUF_Q8_0_BLOCK_SIZE;
    const uint8_t* src = static_cast<const uint8_t*>(gguf_data);
    
    // Create output tensor with INT8 quantization
    QuantizedTensor result(QuantizationType::INT8, shape, GGUF_CONVERSION_BLOCK_SIZE);
    
    // Initialize all block scales to 0 for proper max tracking
    for (auto& block : result.blocks()) {
        block.scale = 0.0f;
        block.zero_point = 128.0f;  // Zero point for signed-to-unsigned conversion
        block.size = 0;
    }
    
    // Process each GGUF block
    for (size_t gguf_block_idx = 0; gguf_block_idx < num_gguf_blocks; ++gguf_block_idx) {
        const gguf_blocks::Q8_0Block* gguf_block = 
            reinterpret_cast<const gguf_blocks::Q8_0Block*>(src + gguf_block_idx * sizeof(gguf_blocks::Q8_0Block));
        
        // Convert FP16 scale to FP32
        float d = fp16_to_fp32(gguf_block->d);
        
        // Process values in this GGUF block (up to 32 values)
        size_t block_start = gguf_block_idx * GGUF_Q8_0_BLOCK_SIZE;
        size_t block_elements = std::min(GGUF_Q8_0_BLOCK_SIZE, num_elements - block_start);
        
        // Copy INT8 values directly (they're already in the right format)
        for (size_t i = 0; i < block_elements; ++i) {
            size_t global_idx = block_start + i;
            
            // GGUF Q8_0 uses symmetric quantization: value = q * scale
            // Our internal INT8 also uses symmetric quantization with offset
            // Convert from signed int8 (-128..127) to unsigned uint8 (0..255)
            // Clamp to prevent overflow: q is in [-128, 127], so q+128 is in [0, 255]
            int8_t q = gguf_block->qs[i];
            int16_t offset_val = static_cast<int16_t>(q) + 128;
            result.data()[global_idx] = static_cast<uint8_t>(std::max(static_cast<int16_t>(0), std::min(static_cast<int16_t>(255), offset_val)));
        }
        
        // Update quantization block parameters (once per GGUF block)
        // Map GGUF blocks to our internal block structure
        size_t start_internal_block = block_start / GGUF_CONVERSION_BLOCK_SIZE;
        size_t end_internal_block = (block_start + block_elements - 1) / GGUF_CONVERSION_BLOCK_SIZE;
        
        for (size_t internal_block_idx = start_internal_block; internal_block_idx <= end_internal_block && internal_block_idx < result.blocks().size(); ++internal_block_idx) {
            // Store scale information (max scale in block)
            result.blocks()[internal_block_idx].scale = std::max(result.blocks()[internal_block_idx].scale, d);
            // Calculate block size
            size_t block_begin = internal_block_idx * GGUF_CONVERSION_BLOCK_SIZE;
            size_t block_end = std::min(block_begin + GGUF_CONVERSION_BLOCK_SIZE, num_elements);
            result.blocks()[internal_block_idx].size = block_end - block_begin;
        }
    }
    
    return result;
}

QuantizedTensor GGUFConverter::convertQ8_0(
    const void* gguf_data,
    const TensorMetadata& tensor_info
) {
    // Use direct conversion by default for better quality
    return convertQ8_0_direct(gguf_data, tensor_info);
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
