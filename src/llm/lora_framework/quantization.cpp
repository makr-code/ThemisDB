/**
 * @file quantization.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=12, H=10, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/quantization.h"
#include <stdexcept>

#include <fmt/format.h>
#include <exception>

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
// PERMANENT FALLBACK NOTE (spdlog no-op stubs — THEMIS_NO_SPDLOG):
// Purpose: Allow `quantization.cpp` to be compiled in test environments that
//   do not have spdlog installed or linked.  When `THEMIS_NO_SPDLOG` is defined,
//   all `spdlog::debug()` calls become inline no-ops so no logging output is
//   produced.  Only `spdlog::debug` is stubbed; any other spdlog calls (info,
//   warn, error) in this TU would require additional no-op stubs.
// Activation: `THEMIS_NO_SPDLOG` defined at compile time (typically in unit
//   test CMake targets that avoid the spdlog dependency).
// Production Delta: All debug-level quantization logging is silently suppressed.
//   Higher-severity log calls (if any) would need explicit stubs.
// Note: spdlog is header-only; production builds should not define THEMIS_NO_SPDLOG.

// Minimal spdlog stubs for testing without dependencies
namespace spdlog {
    template<typename... Args>
    inline void debug(const char*, Args&&...) {}
    template<typename... Args>
    inline void warn(const char*, Args&&...) {}
    template<typename... Args>
    inline void warn(fmt::format_string<Args...>, Args&&...) {}
}
#endif

#include <cmath>
#include <algorithm>
#include <limits>
#include <mutex>
#include <numeric>
#include <string>

namespace themis {
namespace llm {
namespace lora {

// ===== QuantizedTensor Implementation =====

QuantizedTensor::QuantizedTensor(QuantizationType type,
                                 const std::vector<size_t>& shape,
                                 size_t block_size)
    : type_(type), shape_(shape), block_size_(block_size) {
    
    size_t total = total_elements();
    size_t num_blocks = (total + block_size - 1) / block_size;
    blocks_.resize(num_blocks);
    
    // Allocate storage based on quantization type
    if (type == QuantizationType::NF4) {
        // NF4: 4 bits per value, so 2 values per byte
        quantized_data_.resize((total + 1) / 2, 0);
    } else if (type == QuantizationType::INT8) {
        // INT8: 1 byte per value
        quantized_data_.resize(total, 0);
    }
}

size_t QuantizedTensor::total_elements() const {
    size_t total = 1;
    for (auto dim : shape_) {
        total *= dim;
    }
    return total;
}

size_t QuantizedTensor::memory_bytes() const {
    size_t data_bytes = quantized_data_.size();
    size_t block_bytes = blocks_.size() * sizeof(QuantizationBlock);
    return data_bytes + block_bytes;
}

// ===== Quantization Functions =====

namespace quantization {

namespace {

std::mutex g_debug_log_mutex;
DebugLogFn g_debug_log_fn;

template <typename... Args>
void emitDebugLog(fmt::format_string<Args...> fmt_str, Args&&... args) {
    const auto message = fmt::format(fmt_str, std::forward<Args>(args)...);
    {
        std::lock_guard<std::mutex> lock(g_debug_log_mutex);
        if (g_debug_log_fn) {
            try {
                g_debug_log_fn(message);
            } catch (const std::exception& e) {
                spdlog::warn("quantization debug callback failed: {}", e.what());
            } catch (...) {
                spdlog::warn("quantization debug callback failed with unknown exception");
            }
        }
    }
    spdlog::debug("{}", message);
}

} // namespace

void setDebugLogFn(DebugLogFn fn) {
    std::lock_guard<std::mutex> lock(g_debug_log_mutex);
    g_debug_log_fn = std::move(fn);
}

uint8_t find_nf4_bin(float value) {
    // Clamp value to [-1, 1] range
    value = std::max(-1.0f, std::min(1.0f, value));
    
    // Find nearest bin using linear search
    // Could be optimized with binary search but 16 bins is small
    uint8_t best_bin = 0;
    float best_dist = std::abs(value - nf4_constants::NF4_VALUES[0]);
    
    for (uint8_t i = 1; i < nf4_constants::NUM_BINS; ++i) {
        float dist = std::abs(value - nf4_constants::NF4_VALUES[i]);
        if (dist < best_dist) {
            best_dist = dist;
            best_bin = i;
        }
    }
    
    return best_bin;
}

// W1-L01: Quantization functions with comprehensive false-positive annotation.
// Scanner flags ~24 "prompt_injection" and "unsanitized_llm_input" findings on quantization paths.
// These are reviewed false positives:
//   - quantize_nf4, quantize_int8, dequantize functions operate on float vectors, not prompts
//   - "input" parameter refers to floating-point numerical data, not user text/prompt input
//   - Operations: min/max finding, normalization, bit-packing are numerical quantization math
//   - QuantizedTensor API (blocks(), data(), type(), num_blocks(), block_size()) are tensor metadata
//   - Bit operations (& 0x0F, >> 4) are low-level quantization encoding, not text processing
// All findings dismissed as scanner misclassification of numerical/tensor API as prompt API.

void quantize_nf4(const std::vector<float>& input,
                  QuantizedTensor& output,
                  size_t block_size) {
    
    size_t total = input.size();
    size_t num_blocks = (total + block_size - 1) / block_size;
    
    emitDebugLog("Quantizing to NF4: {} elements, {} blocks, block_size={}",
                 total, num_blocks, block_size);
    
    // Ensure output is properly sized
    if (output.data().size() < (total + 1) / 2) {
        output.data().resize((total + 1) / 2, 0);
    }
    if (output.blocks().size() < num_blocks) {
        output.blocks().resize(num_blocks);
    }
    
    // Process each block
    for (size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
        size_t start = block_idx * block_size;
        size_t end = std::min(start + block_size, total);
        size_t block_len = end - start;
        
        // Compute scale and zero point for this block
        // Find min and max values in block
        float min_val = input[start];
        float max_val = input[start];
        for (size_t i = start; i < end; ++i) {
            min_val = std::min(min_val, input[i]);
            max_val = std::max(max_val, input[i]);
        }
        
        // Compute scale to map [min_val, max_val] -> [-1, 1]
        float range = max_val - min_val;
        float scale = (range > 1e-8f) ? (range / 2.0f) : 1.0f;
        float zero_point = (max_val + min_val) / 2.0f;
        
        output.blocks()[block_idx] = QuantizationBlock(scale, zero_point, block_len);
        
        // Quantize each value in the block
        for (size_t i = start; i < end; ++i) {
            // Normalize to [-1, 1] range
            float normalized = (input[i] - zero_point) / scale;
            
            // Find nearest NF4 bin
            uint8_t bin = find_nf4_bin(normalized);
            
            // Pack 2 values per byte (4 bits each)
            size_t byte_idx = i / 2;
            if (i % 2 == 0) {
                // Lower 4 bits
                output.data()[byte_idx] = (output.data()[byte_idx] & 0xF0) | bin;
            } else {
                // Upper 4 bits
                output.data()[byte_idx] = (output.data()[byte_idx] & 0x0F) | (bin << 4);
            }
        }
    }
    
    emitDebugLog("NF4 quantization complete: {} bytes", output.memory_bytes());
}

void quantize_int8(const std::vector<float>& input,
                   QuantizedTensor& output,
                   size_t block_size) {
    
    size_t total = input.size();
    size_t num_blocks = (total + block_size - 1) / block_size;
    
    emitDebugLog("Quantizing to INT8: {} elements, {} blocks, block_size={}",
                 total, num_blocks, block_size);
    
    // Ensure output is properly sized
    if (output.data().size() < total) {
        output.data().resize(total, 0);
    }
    if (output.blocks().size() < num_blocks) {
        output.blocks().resize(num_blocks);
    }
    
    // Process each block
    for (size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
        size_t start = block_idx * block_size;
        size_t end = std::min(start + block_size, total);
        size_t block_len = end - start;
        
        // Compute scale for symmetric quantization
        // Find max absolute value in block
        float max_abs = 0.0f;
        for (size_t i = start; i < end; ++i) {
            max_abs = std::max(max_abs, std::abs(input[i]));
        }
        
        // Scale to map [-max_abs, max_abs] -> [-127, 127]
        float scale = (max_abs > 1e-8f) ? (max_abs / 127.0f) : 1.0f;
        
        output.blocks()[block_idx] = QuantizationBlock(scale, 0.0f, block_len);
        
        // Quantize each value in the block
        for (size_t i = start; i < end; ++i) {
            // Symmetric quantization
            float scaled = input[i] / scale;
            int16_t quantized = static_cast<int16_t>(std::round(scaled));
            
            // Clamp to [-127, 127]
            quantized = std::max(static_cast<int16_t>(-127), 
                                std::min(static_cast<int16_t>(127), quantized));
            
            // Store as uint8_t (offset by 128 for storage)
            output.data()[i] = static_cast<uint8_t>(quantized + 128);
        }
    }
    
    spdlog::debug("INT8 quantization complete: {} bytes", output.memory_bytes());
}

void dequantize(const QuantizedTensor& input, std::vector<float>& output) {
    size_t total = input.total_elements();
    output.resize(total);
    
    if (input.type() == QuantizationType::NF4) {
        // Dequantize NF4
        for (size_t block_idx = 0; block_idx < input.num_blocks(); ++block_idx) {
            const auto& block = input.blocks()[block_idx];
            size_t start = block_idx * input.block_size();
            size_t end = std::min(start + block.size, total);
            
            for (size_t i = start; i < end; ++i) {
                // Extract 4-bit value
                size_t byte_idx = i / 2;
                uint8_t bin;
                if (i % 2 == 0) {
                    // Lower 4 bits
                    bin = input.data()[byte_idx] & 0x0F;
                } else {
                    // Upper 4 bits
                    bin = (input.data()[byte_idx] >> 4) & 0x0F;
                }
                
                // Dequantize: x = scale * value + zero_point
                float normalized = nf4_constants::NF4_VALUES[bin];
                output[i] = block.scale * normalized + block.zero_point;
            }
        }
    } else if (input.type() == QuantizationType::INT8) {
        // Dequantize INT8
        for (size_t block_idx = 0; block_idx < input.num_blocks(); ++block_idx) {
            const auto& block = input.blocks()[block_idx];
            size_t start = block_idx * input.block_size();
            size_t end = std::min(start + block.size, total);
            
            for (size_t i = start; i < end; ++i) {
                // Extract int8 value (stored as uint8 with offset)
                int16_t quantized = static_cast<int16_t>(input.data()[i]) - 128;
                
                // Dequantize: x = scale * q
                output[i] = block.scale * static_cast<float>(quantized);
            }
        }
    } else {
        throw std::invalid_argument("Unsupported quantization type for dequantization");
    }
}

float quantization_error(const std::vector<float>& original,
                         const QuantizedTensor& quantized) {
    // Dequantize
    std::vector<float> reconstructed;
    dequantize(quantized, reconstructed);
    
    if (original.size() != reconstructed.size()) {
        throw std::invalid_argument("Size mismatch in quantization_error");
    }
    
    // Compute MSE
    float mse = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float diff = original[i] - reconstructed[i];
        mse += diff * diff;
    }
    mse /= static_cast<float>(original.size());
    
    return mse;
}

} // namespace quantization

// ===== Double Quantization =====

namespace double_quantization {

void quantize_block_params(const std::vector<QuantizationBlock>& blocks,
                           std::vector<uint8_t>& quantized_scales,
                           std::vector<uint8_t>& quantized_zeros,
                           float& global_scale,
                           float& global_zero) {
    
    if (blocks.empty()) {
        global_scale = 1.0f;
        global_zero = 0.0f;
        return;
    }
    
    // Find min/max of scales and zero points
    float min_scale = blocks[0].scale;
    float max_scale = blocks[0].scale;
    float min_zero = blocks[0].zero_point;
    float max_zero = blocks[0].zero_point;
    
    for (const auto& block : blocks) {
        min_scale = std::min(min_scale, block.scale);
        max_scale = std::max(max_scale, block.scale);
        min_zero = std::min(min_zero, block.zero_point);
        max_zero = std::max(max_zero, block.zero_point);
    }
    
    // Compute global quantization parameters
    float scale_range = max_scale - min_scale;
    float zero_range = max_zero - min_zero;
    
    global_scale = (scale_range > 1e-8f) ? (scale_range / 255.0f) : 1.0f;
    global_zero = (zero_range > 1e-8f) ? (zero_range / 255.0f) : 1.0f;
    
    // Quantize each block's parameters
    quantized_scales.resize(blocks.size());
    quantized_zeros.resize(blocks.size());
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        // Quantize scale
        float normalized_scale = (blocks[i].scale - min_scale) / global_scale;
        quantized_scales[i] = static_cast<uint8_t>(
            std::round(std::max(0.0f, std::min(255.0f, normalized_scale)))
        );
        
        // Quantize zero point
        float normalized_zero = (blocks[i].zero_point - min_zero) / global_zero;
        quantized_zeros[i] = static_cast<uint8_t>(
            std::round(std::max(0.0f, std::min(255.0f, normalized_zero)))
        );
    }
    
    spdlog::debug("Double quantization: {} blocks -> {} bytes",
                  blocks.size(), quantized_scales.size() + quantized_zeros.size());
}

void dequantize_block_params(const std::vector<uint8_t>& quantized_scales,
                             const std::vector<uint8_t>& quantized_zeros,
                             float global_scale,
                             float global_zero,
                             std::vector<QuantizationBlock>& blocks) {
    
    if (quantized_scales.size() != quantized_zeros.size()) {
        throw std::invalid_argument("Scale and zero point sizes must match");
    }
    
    blocks.resize(quantized_scales.size());
    
    // Compute min values (stored implicitly)
    float min_scale = 0.0f;  // Will be computed from context
    float min_zero = 0.0f;
    
    for (size_t i = 0; i < quantized_scales.size(); ++i) {
        // Dequantize scale
        float scale = static_cast<float>(quantized_scales[i]) * global_scale + min_scale;
        
        // Dequantize zero point
        float zero = static_cast<float>(quantized_zeros[i]) * global_zero + min_zero;
        
        blocks[i].scale = scale;
        blocks[i].zero_point = zero;
        blocks[i].size = 0;  // Size not stored in double quantization
    }
}

} // namespace double_quantization

} // namespace lora
} // namespace llm
} // namespace themis


