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

private:
    // Q4_K_M dequantization helpers
    static std::vector<float> dequantizeQ4KM(
        const void* data,
        size_t num_elements
    );
    
    // Q8_0 dequantization helpers
    static std::vector<float> dequantizeQ8_0(
        const void* data,
        size_t num_elements
    );
    
    // FP16 to FP32 conversion
    static float fp16_to_fp32(uint16_t h);
    
    // Calculate total elements from shape
    static size_t calculateElements(const std::vector<int64_t>& shape);
};

/**
 * @brief GGUF block structures for reference
 */
namespace gguf_blocks {
    
    // Q4_K_M block (256 values)
    struct Q4KBlock {
        uint8_t qs[128];        // Quantized values (4 bits each, packed)
        uint8_t scales[12];     // Scales and mins (mixed)
        uint16_t d;             // Delta (FP16)
        uint16_t dmin;          // Min (FP16)
    };
    static_assert(sizeof(Q4KBlock) == 144, "Q4KBlock size mismatch");
    
    // Q8_0 block (32 values)
    struct Q8_0Block {
        uint16_t d;             // Scale (FP16)
        int8_t qs[32];          // Quantized values (INT8)
    };
    static_assert(sizeof(Q8_0Block) == 34, "Q8_0Block size mismatch");
    
} // namespace gguf_blocks

} // namespace lora
} // namespace llm
} // namespace themis
