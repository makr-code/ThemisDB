/**
 * @file tensor_dtype.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Data type for tensor elements
 * 
 * Supports full precision (FP32) and mixed precision training (FP16, BF16).
 */
enum class DType {
    FLOAT32,  // Full precision (32-bit float) - default
    FLOAT16,  // Half precision (16-bit float) - IEEE 754 half
    BFLOAT16  // Brain float16 (16-bit) - Same exponent range as FP32
};

/**
 * @brief Get size in bytes for a given data type
 * @param dtype Data type
 * @return Size in bytes
 */
inline size_t dtype_size(DType dtype) {
    switch (dtype) {
        case DType::FLOAT32:
            return 4;
        case DType::FLOAT16:
        case DType::BFLOAT16:
            return 2;
        default:
            return 4;
    }
}

/**
 * @brief Get string name for data type
 * @param dtype Data type
 * @return String representation
 */
inline std::string dtype_name(DType dtype) {
    switch (dtype) {
        case DType::FLOAT32:
            return "float32";
        case DType::FLOAT16:
            return "float16";
        case DType::BFLOAT16:
            return "bfloat16";
        default:
            return "unknown";
    }
}

/**
 * @brief Check if dtype is mixed precision (FP16 or BF16)
 * @param dtype Data type
 * @return true if FP16 or BF16
 */
inline bool is_mixed_precision(DType dtype) {
    return dtype == DType::FLOAT16 || dtype == DType::BFLOAT16;
}

/**
 * @brief Convert FP32 to FP16 (CPU simulation)
 * 
 * This is a simplified conversion for CPU. Real GPU implementation
 * uses native __half type from cuda_fp16.h
 * 
 * @param value FP32 value
 * @return FP16 value (stored in uint16_t)
 */
inline uint16_t fp32_to_fp16_bits(float value) {
    // IEEE 754 half precision conversion (simplified)
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    
    uint32_t sign = (bits >> 16) & 0x8000;
    int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xFF) - 127 + 15;  // Use signed int
    uint32_t mantissa = (bits >> 13) & 0x3FF;
    
    // Clamp exponent
    if (exponent <= 0) {
        // Underflow to zero
        return static_cast<uint16_t>(sign);
    } else if (exponent >= 31) {
        // Overflow to infinity
        return static_cast<uint16_t>(sign | 0x7C00);
    }
    
    return static_cast<uint16_t>(sign | (exponent << 10) | mantissa);
}

/**
 * @brief Convert FP16 to FP32 (CPU simulation)
 * 
 * @param value FP16 value (stored in uint16_t)
 * @return FP32 value
 */
inline float fp16_bits_to_fp32(uint16_t value) {
    uint32_t sign = (value & 0x8000) << 16;
    uint32_t exponent = (value >> 10) & 0x1F;
    uint32_t mantissa = value & 0x3FF;
    
    if (exponent == 0) {
        // Zero or denormal
        if (mantissa == 0) {
            uint32_t bits = sign;
            float result = 0.0f;
            std::memcpy(&result, &bits, sizeof(float));
            return result;
        }
        // Denormal (not fully implemented)
        exponent = 1;
    } else if (exponent == 31) {
        // Infinity or NaN
        uint32_t bits = sign | 0x7F800000 | (mantissa << 13);
        float result = 0.0f;
        std::memcpy(&result, &bits, sizeof(float));
        return result;
    }
    
    uint32_t bits = sign | ((exponent - 15 + 127) << 23) | (mantissa << 13);
    float result = 0.0f;
    std::memcpy(&result, &bits, sizeof(float));
    return result;
}

/**
 * @brief Convert FP32 to BF16 (CPU simulation)
 * 
 * BF16 has same exponent range as FP32, just truncated mantissa
 * 
 * @param value FP32 value
 * @return BF16 value (stored in uint16_t)
 */
inline uint16_t fp32_to_bf16_bits(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    
    // BF16 is just upper 16 bits of FP32
    // Add rounding
    bits += 0x7FFF + ((bits >> 16) & 1);
    
    return static_cast<uint16_t>(bits >> 16);
}

/**
 * @brief Convert BF16 to FP32 (CPU simulation)
 * 
 * @param value BF16 value (stored in uint16_t)
 * @return FP32 value
 */
inline float bf16_bits_to_fp32(uint16_t value) {
    uint32_t bits = static_cast<uint32_t>(value) << 16;
    float result = 0.0f;
    std::memcpy(&result, &bits, sizeof(float));
    return result;
}

} // namespace lora
} // namespace llm
} // namespace themis
