/**
 * @file paged_kv_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/block_table.h"
#include "llm/paged_block_manager.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace themis {
namespace llm {

/**
 * PagedKVCache manages Key-Value cache storage using block-based memory allocation.
 * Integrates with llama.cpp KV cache format for efficient memory usage.
 */
class PagedKVCache {
public:
    /**
     * @brief KV-cache quantization mode metadata for page allocation planning.
     *
     * @note This enum declares intended storage mode. The current cache payload
     *       remains float-based until quantized storage paths are introduced.
     */
    enum class KVQuantizationType {
        FP16,   ///< Baseline half-precision KV cache mode.
        INT8,   ///< 8-bit KV quantization mode.
        NVFP4   ///< NVIDIA FP4 KV quantization mode.
    };

    struct Config {
        size_t block_size = 16;           // Tokens per block
        size_t num_blocks = 4096;         // Total blocks available
        size_t num_layers = 32;           // Number of transformer layers
        size_t head_dim = 128;            // Dimension per attention head
        size_t num_kv_heads = 8;          // Number of KV heads (GQA)
        KVQuantizationType kv_quantization = KVQuantizationType::FP16; // KV quantization target (FP16/INT8/NVFP4)
        int kv_quantization_bits = 16;    // Bit-width for quantized KV storage: 16=FP16, 8=INT8, 4=NVFP4
        bool enable_prefix_caching = true; // Enable prefix sharing
        bool enable_kv_quantization_runtime = false; // Enable runtime KV quantization (Phase 2+)
    };

    PagedKVCache(const Config& config, std::shared_ptr<PagedBlockManager> block_manager);
    
    ~PagedKVCache();

    // Store KV cache for a sequence
    void store(uint64_t sequence_id, size_t layer_id, const std::vector<float>& kv_data);
    
    // Retrieve KV cache for a sequence
    std::vector<float> retrieve(uint64_t sequence_id, size_t layer_id) const;
    
    // Share prefix between sequences (Copy-on-Write)
    void sharePrefix(uint64_t new_sequence_id, uint64_t parent_sequence_id, size_t prefix_length);
    
    // Get block table for sequence
    std::shared_ptr<BlockTable> getBlockTable(uint64_t sequence_id);
    
    // Remove sequence and free blocks
    void removeSequence(uint64_t sequence_id);
    
    // Get statistics
    struct Stats {
        size_t blocks_used = 0;
        size_t blocks_free = 0;
        size_t num_sequences = 0;
        double fragmentation_rate = 0.0;
        double prefix_sharing_ratio = 0.0;
    };
    Stats getStats() const;

    /**
     * @brief Quantize KV data to target precision format.
     *
     * Converts full-precision float KV cache to quantized storage (FP16, INT8, or NVFP4).
     * Returns quantized bytes; empty vector on error.
     * 
     * @param kv_data Input KV cache data (float32)
     * @param target_type Target quantization type (FP16, INT8, NVFP4)
     * @return Quantized KV data as vector of bytes
     */
    std::vector<uint8_t> quantizeKVData(
        const std::vector<float>& kv_data, 
        KVQuantizationType target_type) const;

    /**
     * @brief Dequantize KV data from quantized format back to float.
     *
     * Reconstructs full-precision KV cache from quantized storage.
     * 
     * @param quantized_data Quantized KV data
     * @param source_type Source quantization type
     * @return Dequantized KV data (float32)
     */
    std::vector<float> dequantizeKVData(
        const std::vector<uint8_t>& quantized_data,
        KVQuantizationType source_type) const;

    /**
     * @brief Get expected VRAM reduction factor for quantization type.
     *
     * @param type Quantization type
     * @return Reduction factor (0.0 to 1.0, where 0.5 = 50% reduction)
     */
    static float getCompressionFactor(KVQuantizationType type);

    /**
     * @brief Estimate accuracy retention for quantization type vs FP16 baseline.
     *
     * Returns expected accuracy as fraction (0.99 = 99% accuracy).
     *
     * @param type Quantization type
     * @return Expected accuracy retention (0.0 to 1.0)
     */
    static float getExpectedAccuracy(KVQuantizationType type);

    /**
     * @brief Get bit-width (storage bits per value) for quantization type.
     *
     * Used to calculate storage reduction and validate Config consistency.
     *
     * @param type Quantization type
     * @return Bits per value: 16 for FP16, 8 for INT8, 4 for NVFP4
     */
    static int getBitWidthForQuantizationType(KVQuantizationType type);

private:
    Config config_;
    std::shared_ptr<PagedBlockManager> block_manager_;
    
    // Map sequence_id -> BlockTable
    std::unordered_map<uint64_t, std::shared_ptr<BlockTable>> block_tables_;
    
    // KV cache storage: block_id -> layer_id -> kv_data (stored as quantized bytes)
    std::unordered_map<int, std::unordered_map<size_t, std::vector<uint8_t>>> kv_storage_quantized_;

    // Backwards-compatibility: legacy full-precision KV storage used by
    // existing implementation paths. New code should use `kv_storage_quantized_`.
    std::unordered_map<int, std::unordered_map<size_t, std::vector<float>>> kv_storage_;
    
    // Metadata for quantized storage: block_id -> layer_id -> quantization_type
    std::unordered_map<int, std::unordered_map<size_t, KVQuantizationType>> quantization_metadata_;
    
    mutable std::mutex mutex_;
    
    size_t calculateKVSize() const;

    /**
     * @brief Quantize float32 to NVFP4 (4-bit float: 1 sign, 2 exponent, 1 mantissa).
     *
     * NVFP4 format: [s1e2m1] — maximum range [-448, +448] with ~5% precision loss vs FP16.
     *
     * @param value Float32 input
     * @return 4-bit packed value (lower 4 bits used)
     */
    static uint8_t quantizeToNVFP4(float value);

    /**
     * @brief Dequantize NVFP4 (4-bit) back to float32.
     *
     * @param packed 4-bit packed value
     * @return Float32 output
     */
    static float dequantizeFromNVFP4(uint8_t packed);

    /**
     * @brief Quantize float32 to INT8 (per-channel quantization).
     *
     * @param values Input float32 values
     * @param scale Output scale factor
     * @param zero_point Output zero-point offset
     * @return INT8 quantized values
     */
    static std::vector<int8_t> quantizeToINT8(
        const std::vector<float>& values,
        float& scale,
        int8_t& zero_point);

    /**
     * @brief Dequantize INT8 values back to float32.
     *
     * @param quantized INT8 quantized values
     * @param scale Scale factor from quantization
     * @param zero_point Zero-point offset from quantization
     * @return Float32 dequantized values
     */
    static std::vector<float> dequantizeFromINT8(
        const std::vector<int8_t>& quantized,
        float scale,
        int8_t zero_point);
};

} // namespace llm
} // namespace themis
