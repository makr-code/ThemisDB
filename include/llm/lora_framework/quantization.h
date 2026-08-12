/**
 * @file quantization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Quantization types supported by QLoRA
 */
enum class QuantizationType {
    NONE,      // No quantization (full precision)
    NF4,       // 4-bit NormalFloat (QLoRA paper)
    INT8,      // 8-bit integer quantization
    Q4_K_M,    // GGUF 4-bit with K-means (future)
    Q8_0       // GGUF 8-bit (future)
};

/**
 * @brief NF4 (4-bit NormalFloat) quantization constants
 * 
 * Optimized for normally distributed weights in neural networks.
 * 16 bins with non-uniform spacing (denser near 0).
 * 
 * Reference: QLoRA paper (https://arxiv.org/abs/2305.14314)
 */
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

/**
 * @brief Quantization configuration for a block of values
 * 
 * Block-wise quantization uses separate scale and zero-point for each block
 * to improve quantization accuracy. Typical block size: 64-128 elements.
 */
struct QuantizationBlock {
    virtual ~QuantizationBlock() = default;
    float scale = 0.0f;      // Scaling factor for dequantization
    float zero_point = 0.0f; // Zero point offset
    size_t size = 0;      // Number of elements in this block
    
    QuantizationBlock() : scale(1.0f), zero_point(0.0f), size(0) {}
    QuantizationBlock(float s, float z, size_t sz) : scale(s), zero_point(z), size(sz) {}
};

/**
 * @brief Quantized tensor storage
 * 
 * Stores quantized weights with block-wise quantization parameters.
 * Supports NF4 (4-bit) and INT8 (8-bit) quantization.
 */
class QuantizedTensor {
public:
    virtual ~QuantizedTensor() = default;
    QuantizedTensor() = default;
    
    /**
     * @brief Construct quantized tensor
     * @param type Quantization type (NF4 or INT8)
     * @param shape Original tensor shape
     * @param block_size Number of elements per quantization block
     */
    QuantizedTensor(QuantizationType type, 
                    const std::vector<size_t>& shape,
                    size_t block_size = 64);
    
    // Getters
    QuantizationType type() const { return type_; }
    const std::vector<size_t>& shape() const { return shape_; }
    size_t block_size() const { return block_size_; }
    size_t num_blocks() const { return blocks_.size(); }
    size_t total_elements() const;
    
    // Quantized data access
    const std::vector<uint8_t>& data() const { return quantized_data_; }
    std::vector<uint8_t>& data() { return quantized_data_; }
    
    // Block parameters access
    const std::vector<QuantizationBlock>& blocks() const { return blocks_; }
    std::vector<QuantizationBlock>& blocks() { return blocks_; }
    
    // Memory usage
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

/**
 * @brief Quantization operations
 */
namespace quantization {

    using DebugLogFn = std::function<void(const std::string&)>;

    /**
     * @brief Inject an alternative debug sink for quantization traces.
     *
     * This is primarily used by `THEMIS_NO_SPDLOG` builds so debug-level
     * quantization diagnostics remain observable without linking spdlog.
     */
    void setDebugLogFn(DebugLogFn fn);

    /**
     * @brief Quantize a tensor to NF4 format
     * 
     * @param input Input tensor data (full precision)
     * @param output Output quantized tensor
     * @param block_size Number of elements per quantization block
     * 
     * Uses block-wise quantization with separate scale/zero-point per block.
     */
    void quantize_nf4(const std::vector<float>& input,
                      QuantizedTensor& output,
                      size_t block_size = 64);
    
    /**
     * @brief Quantize a tensor to INT8 format
     * 
     * @param input Input tensor data (full precision)
     * @param output Output quantized tensor
     * @param block_size Number of elements per quantization block
     * 
     * Uses symmetric quantization: q = round(x / scale)
     * Range: [-127, 127] (symmetric around 0)
     */
    void quantize_int8(const std::vector<float>& input,
                       QuantizedTensor& output,
                       size_t block_size = 64);
    
    /**
     * @brief Dequantize a tensor back to full precision
     * 
     * @param input Quantized tensor
     * @param output Output full precision data
     * 
     * Reconstructs original values using: x = scale * (q - zero_point)
     */
    void dequantize(const QuantizedTensor& input,
                    std::vector<float>& output);
    
    /**
     * @brief Compute quantization error (MSE)
     * 
     * @param original Original full precision data
     * @param quantized Quantized tensor
     * @return Mean squared error between original and reconstructed
     */
    float quantization_error(const std::vector<float>& original,
                             const QuantizedTensor& quantized);
    
    /**
     * @brief Find nearest NF4 bin for a value
     * 
     * @param value Normalized value (after scaling)
     * @return Bin index (0-15)
     */
    uint8_t find_nf4_bin(float value);
    
} // namespace quantization

/**
 * @brief Double quantization support
 * 
 * Quantizes the quantization constants (scale, zero_point) to save additional memory.
 * Typical savings: 0.37 bits per parameter.
 */
namespace double_quantization {
    
    /**
     * @brief Quantize block parameters (scales and zero points) to 8-bit
     * 
     * @param blocks Input block parameters
     * @param quantized_scales Output quantized scales
     * @param quantized_zeros Output quantized zero points
     * @param global_scale Global scale for dequantization
     * @param global_zero Global zero point for dequantization
     */
    void quantize_block_params(const std::vector<QuantizationBlock>& blocks,
                               std::vector<uint8_t>& quantized_scales,
                               std::vector<uint8_t>& quantized_zeros,
                               float& global_scale,
                               float& global_zero);
    
    /**
     * @brief Dequantize block parameters back to FP32
     * 
     * @param quantized_scales Quantized scales
     * @param quantized_zeros Quantized zero points
     * @param global_scale Global scale
     * @param global_zero Global zero point
     * @param blocks Output reconstructed blocks
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
