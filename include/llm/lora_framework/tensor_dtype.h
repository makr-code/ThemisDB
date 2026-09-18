/**
 * @file tensor_dtype.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class DType {
    FLOAT32,  // Full precision (32-bit float) - default
    FLOAT16,  // Half precision (16-bit float) - IEEE 754 half
    BFLOAT16  // Brain float16 (16-bit) - Same exponent range as FP32
};

/**
 * @brief Dtype size.
 * @param[in] dtype Input parameter.
 * @return Return value.
 * @details Implements dtype_size without additional internal calls.
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
 * @brief Dtype name.
 * @param[in] dtype Input parameter.
 * @return Return value.
 * @details Implements dtype_name without additional internal calls.
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
 * @brief Is mixed precision.
 * @param[in] dtype Input parameter.
 * @return True when the operation succeeds.
 * @details Implements is_mixed_precision without additional internal calls.
 */
inline bool is_mixed_precision(DType dtype) {
    return dtype == DType::FLOAT16 || dtype == DType::BFLOAT16;
}

/**
 * @brief Fp32 to fp16 bits.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: std::memcpy().
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
 * @brief Fp16 bits to fp32.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: std::memcpy().
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
 * @brief Fp32 to bf16 bits.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: std::memcpy().
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
 * @brief Bf16 bits to fp32.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: std::memcpy().
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
