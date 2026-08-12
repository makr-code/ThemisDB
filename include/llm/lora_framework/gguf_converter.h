/**
 * @file gguf_converter.h
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
#include "llm/gguf_loader.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief GGUF format converter for QLoRA integration
 * 
 * Converts GGUF quantized formats (Q4_K_M, Q8_0) to internal
 * quantization formats (NF4, INT8) for QLoRA training.
 */
class GGUFConverter {
public:
    GGUFConverter() = default;
    
    /**
     * @brief Convert Q4_K_M tensor to internal NF4 format
     * 
     * Q4_K_M uses 4-bit quantization with K-means clustering.
     * Block structure: 256 values per block
     * - 32 bytes: quantized data (4 bits per value)
     * - 2 bytes: FP16 scale per super-block
     * - 2 bytes: FP16 min values
     * 
     * @param gguf_data Raw GGUF tensor data
     * @param tensor_info GGUF tensor metadata
     * @return QuantizedTensor in NF4 format
     */
    static QuantizedTensor convertQ4KM(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert Q8_0 tensor to internal INT8 format
     * 
     * Q8_0 uses 8-bit symmetric quantization.
     * Block structure: 32 values per block
     * - 32 bytes: INT8 quantized data
     * - 2 bytes: FP16 scale
     * 
     * @param gguf_data Raw GGUF tensor data
     * @param tensor_info GGUF tensor metadata
     * @return QuantizedTensor in INT8 format
     */
    static QuantizedTensor convertQ8_0(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert F16 tensor to internal format
     * 
     * @param gguf_data Raw GGUF tensor data
     * @param tensor_info GGUF tensor metadata
     * @return Full precision tensor as std::vector<float>
     */
    static std::vector<float> convertF16(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert F32 tensor to internal format
     * 
     * @param gguf_data Raw GGUF tensor data
     * @param tensor_info GGUF tensor metadata
     * @return Full precision tensor as std::vector<float>
     */
    static std::vector<float> convertF32(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Check if GGUF type is supported
     * 
     * @param type GGML quantization type
     * @return true if conversion is supported
     */
    static bool isSupported(GGMLType type);
    
    /**
     * @brief Get equivalent internal quantization type
     * 
     * @param type GGML quantization type
     * @return Corresponding QuantizationType
     */
    static QuantizationType getInternalType(GGMLType type);
    
    /**
     * @brief Convert Q4_K_M directly to internal NF4 format (no FP32 intermediate)
     * 
     * This is the preferred method for quantized loading as it avoids
     * precision loss from dequantize/requantize cycle.
     * 
     * @param gguf_data Raw GGUF tensor data
     * @param tensor_info GGUF tensor metadata
     * @return QuantizedTensor in NF4 format
     */
    static QuantizedTensor convertQ4KM_direct(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert Q8_0 directly to internal INT8 format (no FP32 intermediate)
     * 
     * This is the preferred method for quantized loading as it avoids
     * precision loss from dequantize/requantize cycle.
     * 
     * @param gguf_data Raw GGUF tensor data
     * @param tensor_info GGUF tensor metadata
     * @return QuantizedTensor in INT8 format
     */
    static QuantizedTensor convertQ8_0_direct(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Dequantize Q4_K_M data to FP32
     * 
     * @param data Raw Q4_K_M data
     * @param num_elements Number of elements to dequantize
     * @return FP32 vector
     */
    static std::vector<float> dequantizeQ4KM(
        const void* data,
        size_t num_elements
    );
    
    /**
     * @brief Dequantize Q8_0 data to FP32
     * 
     * @param data Raw Q8_0 data
     * @param num_elements Number of elements to dequantize
     * @return FP32 vector
     */
    static std::vector<float> dequantizeQ8_0(
        const void* data,
        size_t num_elements
    );
    
    /**
     * @brief Calculate total elements from shape
     * 
     * @param shape Tensor shape
     * @return Total number of elements
     */
    static size_t calculateElements(const std::vector<int64_t>& shape);
    
    /**
     * @brief FP16 to FP32 conversion helper (public for testing)
     * 
     * @param h FP16 value as uint16_t
     * @return FP32 value
     */
    static float fp16_to_fp32(uint16_t h);
};

// Default block size for internal quantization after GGUF conversion
constexpr size_t GGUF_CONVERSION_BLOCK_SIZE = 64;

/**
 * @brief GGUF block structures for reference
 */
namespace gguf_blocks {
    
    // Q4_K_M block (256 values)
    struct Q4KBlock {
        uint8_t qs[128];        // Quantized values (4 bits each, packed)
        uint8_t scales[12];     // Scales and mins (mixed)
        uint16_t d = 0;             // Delta (FP16)
        uint16_t dmin = 0;          // Min (FP16)
    };
    static_assert(sizeof(Q4KBlock) == 144, "Q4KBlock size mismatch");
    
    // Q8_0 block (32 values)
    struct Q8_0Block {
        uint16_t d = 0;             // Scale (FP16)
        int8_t qs[32];          // Quantized values (INT8)
    };
    static_assert(sizeof(Q8_0Block) == 34, "Q8_0Block size mismatch");
    
} // namespace gguf_blocks

} // namespace lora
} // namespace llm
} // namespace themis
