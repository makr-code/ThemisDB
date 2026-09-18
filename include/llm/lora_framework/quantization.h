/**
 * @file quantization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

enum class QuantizationType {
    NONE,      // No quantization (full precision)
    NF4,       // 4-bit NormalFloat (QLoRA paper)
    INT8,      // 8-bit integer quantization
    Q4_K_M,    // GGUF 4-bit with K-means (future)
    Q8_0       // GGUF 8-bit (future)
};

namespace nf4_constants {
    // NF4 quantization bins (16 values for 4-bit)
    // Values are optimized for normally distributed weights
    constexpr float NF4_VALUES[16] = {
        -1.0f,      // bin 0000
        -0.6962f,   // bin 0001
        -0.5251f,   // bin 0010
        -0.3949f,   // bin 0011
        -0.2844f,   // bin 0100
        -0.1848f,   // bin 0101
        -0.0911f,   // bin 0110
        0.0f,       // bin 0111
        0.0796f,    // bin 1000
        0.1609f,    // bin 1001
        0.2461f,    // bin 1010
        0.3379f,    // bin 1011
        0.4407f,    // bin 1100
        0.5626f,    // bin 1101
        0.7230f,    // bin 1110
        1.0f        // bin 1111
    };
    
    constexpr size_t NUM_BINS = 16;
    constexpr size_t BITS_PER_VALUE = 4;
} // namespace nf4_constants

struct QuantizationBlock {
    /**
     * @brief Quantization Block.
     * @return Return value.
     */
    virtual ~QuantizationBlock() = default;
    float scale = 0.0f;      // Scaling factor for dequantization
    float zero_point = 0.0f; // Zero point offset
    size_t size = 0;      // Number of elements in this block
    
    QuantizationBlock() : scale(1.0f), zero_point(0.0f), size(0) {}
    QuantizationBlock(float s, float z, size_t sz) : scale(s), zero_point(z), size(sz) {}
};

class QuantizedTensor {
public:
    /**
     * @brief Quantized Tensor.
     * @return Return value.
     */
    virtual ~QuantizedTensor() = default;
    QuantizedTensor() = default;
    
    QuantizedTensor(QuantizationType type, 
                    const std::vector<size_t>& shape,
                    size_t block_size = 64);
    
    // Getters
    QuantizationType type() const { return type_; }
    const std::vector<size_t>& shape() const { return shape_; }
    size_t block_size() const { return block_size_; }
    size_t num_blocks() const { return blocks_.size(); }
    /**
     * @brief Total elements.
     * @return Return value.
     */
    size_t total_elements() const;
    
    // Quantized data access
    const std::vector<uint8_t>& data() const { return quantized_data_; }
    /**
     * @brief Data.
     * @return Return value.
     * @details Implements data without additional internal calls.
     */
    std::vector<uint8_t>& data() { return quantized_data_; }
    
    // Block parameters access
    const std::vector<QuantizationBlock>& blocks() const { return blocks_; }
    /**
     * @brief Blocks.
     * @return Return value.
     * @details Implements blocks without additional internal calls.
     */
    std::vector<QuantizationBlock>& blocks() { return blocks_; }
    
    // Memory usage
    /**
     * @brief Memory bytes.
     * @return Return value.
     */
    size_t memory_bytes() const;
    
private:
    QuantizationType type_;
    std::vector<size_t> shape_;
    size_t block_size_ = 0;
    
    // Quantized data storage
    // For NF4: 2 values packed per byte (4 bits each)
    // For INT8: 1 value per byte
    std::vector<uint8_t> quantized_data_;
    
    // Block-wise quantization parameters
    std::vector<QuantizationBlock> blocks_;
};

namespace quantization {

    using DebugLogFn = std::function<void(const std::string&)>;

    /**
     * @brief Set Debug Log Fn.
     * @param[in] fn Input parameter.
     */
    void setDebugLogFn(DebugLogFn fn);

    void quantize_nf4(const std::vector<float>& input,
                      QuantizedTensor& output,
                      size_t block_size = 64);
    
    void quantize_int8(const std::vector<float>& input,
                       QuantizedTensor& output,
                       size_t block_size = 64);
    
    /**
     * @brief Dequantize.
     * @param[in] input Input parameter.
     * @param[in,out] output Input/output parameter.
     */
    void dequantize(const QuantizedTensor& input,
                    std::vector<float>& output);
    
    /**
     * @brief Quantization error.
     * @param[in] original Input parameter.
     * @param[in] quantized Input parameter.
     * @return Return value.
     */
    float quantization_error(const std::vector<float>& original,
                             const QuantizedTensor& quantized);
    
    /**
     * @brief Find nf4 bin.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    uint8_t find_nf4_bin(float value);
    
} // namespace quantization

namespace double_quantization {
    
    /**
     * @brief Quantize block params.
     * @param[in] blocks Input parameter.
     * @param[in,out] quantized_scales Input/output parameter.
     * @param[in,out] quantized_zeros Input/output parameter.
     * @param[in,out] global_scale Input/output parameter.
     * @param[in,out] global_zero Input/output parameter.
     */
    void quantize_block_params(const std::vector<QuantizationBlock>& blocks,
                               std::vector<uint8_t>& quantized_scales,
                               std::vector<uint8_t>& quantized_zeros,
                               float& global_scale,
                               float& global_zero);
    
    /**
     * @brief Dequantize block params.
     * @param[in] quantized_scales Input parameter.
     * @param[in] quantized_zeros Input parameter.
     * @param[in] global_scale Input parameter.
     * @param[in] global_zero Input parameter.
     * @param[in,out] blocks Input/output parameter.
     */
    void dequantize_block_params(const std::vector<uint8_t>& quantized_scales,
                                 const std::vector<uint8_t>& quantized_zeros,
                                 float global_scale,
                                 float global_zero,
                                 std::vector<QuantizationBlock>& blocks);
    
} // namespace double_quantization

} // namespace lora
} // namespace llm
} // namespace themis
