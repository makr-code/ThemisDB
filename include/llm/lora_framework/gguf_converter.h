/**
 * @file gguf_converter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class GGUFConverter {
public:
    GGUFConverter() = default;
    
    /**
     * @brief Convert Q4 KM.
     * @param[in] gguf_data Input parameter.
     * @param[in] tensor_info Input parameter.
     * @return Return value.
     */
    static QuantizedTensor convertQ4KM(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert Q8 0.
     * @param[in] gguf_data Input parameter.
     * @param[in] tensor_info Input parameter.
     * @return Return value.
     */
    static QuantizedTensor convertQ8_0(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert F16.
     * @param[in] gguf_data Input parameter.
     * @param[in] tensor_info Input parameter.
     * @return Return value.
     */
    static std::vector<float> convertF16(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert F32.
     * @param[in] gguf_data Input parameter.
     * @param[in] tensor_info Input parameter.
     * @return Return value.
     */
    static std::vector<float> convertF32(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Is Supported.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isSupported(GGMLType type);
    
    /**
     * @brief Get Internal Type.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static QuantizationType getInternalType(GGMLType type);
    
    /**
     * @brief Convert Q4 KM direct.
     * @param[in] gguf_data Input parameter.
     * @param[in] tensor_info Input parameter.
     * @return Return value.
     */
    static QuantizedTensor convertQ4KM_direct(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Convert Q8 0 direct.
     * @param[in] gguf_data Input parameter.
     * @param[in] tensor_info Input parameter.
     * @return Return value.
     */
    static QuantizedTensor convertQ8_0_direct(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    /**
     * @brief Dequantize Q4 KM.
     * @param[in] data Input parameter.
     * @param[in] num_elements Input parameter.
     * @return Return value.
     */
    static std::vector<float> dequantizeQ4KM(
        const void* data,
        size_t num_elements
    );
    
    /**
     * @brief Dequantize Q8 0.
     * @param[in] data Input parameter.
     * @param[in] num_elements Input parameter.
     * @return Return value.
     */
    static std::vector<float> dequantizeQ8_0(
        const void* data,
        size_t num_elements
    );
    
    /**
     * @brief Calculate Elements.
     * @param[in] shape Input parameter.
     * @return Return value.
     */
    static size_t calculateElements(const std::vector<int64_t>& shape);
    
    /**
     * @brief Fp16 to fp32.
     * @param[in] h Input parameter.
     * @return Return value.
     */
    static float fp16_to_fp32(uint16_t h);
};

// Default block size for internal quantization after GGUF conversion
constexpr size_t GGUF_CONVERSION_BLOCK_SIZE = 64;

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
