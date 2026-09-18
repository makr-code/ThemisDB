/**
 * @file paged_kv_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/block_table.h"
#include "llm/paged_block_manager.h"
#include <vector>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include <atomic>

namespace themis {
namespace llm {

class PagedKVCache {
public:
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

    /**
     * @brief Store KV cache for a sequence.
     * @param[in] sequence_id Identifier of the sequence.
     * @param[in] layer_id Identifier of the layer.
     * @param[in] kv_data Input parameter.
     * @return True when the operation succeeds.
     * @details Returns true on success, false if blocks could not be allocated even after LRU eviction.
     */
    bool store(uint64_t sequence_id, size_t layer_id, const std::vector<float>& kv_data);
    
    /**
     * @brief Retrieve KV cache for a sequence
     * @param[in] sequence_id Identifier of the sequence.
     * @param[in] layer_id Identifier of the layer.
     * @return Return value.
     */
    std::vector<float> retrieve(uint64_t sequence_id, size_t layer_id) const;
    
    /**
     * @brief Share prefix between sequences (Copy-on-Write)
     * @param[in] new_sequence_id Identifier of the new sequence.
     * @param[in] parent_sequence_id Identifier of the parent sequence.
     * @param[in] prefix_length Input parameter.
     */
    void sharePrefix(uint64_t new_sequence_id, uint64_t parent_sequence_id, size_t prefix_length);
    
    /**
     * @brief Get block table for sequence
     * @param[in] sequence_id Identifier of the sequence.
     * @return Return value.
     */
    std::shared_ptr<BlockTable> getBlockTable(uint64_t sequence_id);
    
    /**
     * @brief Remove sequence and free blocks
     * @param[in] sequence_id Identifier of the sequence.
     */
    void removeSequence(uint64_t sequence_id);
    
    // Get statistics
    struct Stats {
        size_t blocks_used = 0;
        size_t blocks_free = 0;
        size_t num_sequences = 0;
        double fragmentation_rate = 0.0;
        double prefix_sharing_ratio = 0.0;
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    uint64_t evictionCount() const noexcept { return eviction_count_.load(std::memory_order_relaxed); }

    /**
     * @brief Quantize KVData.
     * @param[in] kv_data Input parameter.
     * @param[in] target_type Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> quantizeKVData(
        const std::vector<float>& kv_data, 
        KVQuantizationType target_type) const;

    /**
     * @brief Dequantize KVData.
     * @param[in] quantized_data Input parameter.
     * @param[in] source_type Input parameter.
     * @return Return value.
     */
    std::vector<float> dequantizeKVData(
        const std::vector<uint8_t>& quantized_data,
        KVQuantizationType source_type) const;

    /**
     * @brief Get Compression Factor.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static float getCompressionFactor(KVQuantizationType type);

    /**
     * @brief Get Expected Accuracy.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static float getExpectedAccuracy(KVQuantizationType type);

    /**
     * @brief Get Bit Width For Quantization Type.
     * @param[in] type Input parameter.
     * @return Return value.
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
    
    // LRU eviction structures (guarded by mutex_)
    // Front = most-recently-used, back = least-recently-used
    mutable std::list<uint64_t> lru_order_;
    mutable std::unordered_map<uint64_t, std::list<uint64_t>::iterator> lru_map_;

    // Total eviction counter (atomic for lock-free reads via evictionCount())
    std::atomic<uint64_t> eviction_count_{0};
    
    /**
     * @brief Calculate KVSize.
     * @return Return value.
     */
    size_t calculateKVSize() const;

    /**
     * @brief Evict LRU.
     * @return True when the operation succeeds.
     */
    bool evictLRU();

    /**
     * @brief Quantize To NVFP4.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static uint8_t quantizeToNVFP4(float value);

    /**
     * @brief Dequantize From NVFP4.
     * @param[in] packed Input parameter.
     * @return Return value.
     */
    static float dequantizeFromNVFP4(uint8_t packed);

    /**
     * @brief Quantize To INT8.
     * @param[in] values Input parameter.
     * @param[in,out] scale Input/output parameter.
     * @param[in,out] zero_point Input/output parameter.
     * @return Return value.
     */
    static std::vector<int8_t> quantizeToINT8(
        const std::vector<float>& values,
        float& scale,
        int8_t& zero_point);

    /**
     * @brief Dequantize From INT8.
     * @param[in] quantized Input parameter.
     * @param[in] scale Input parameter.
     * @param[in] zero_point Input parameter.
     * @return Return value.
     */
    static std::vector<float> dequantizeFromINT8(
        const std::vector<int8_t>& quantized,
        float scale,
        int8_t zero_point);
};

} // namespace llm
} // namespace themis
