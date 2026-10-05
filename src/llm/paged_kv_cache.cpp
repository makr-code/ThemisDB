/**
 * @file paged_kv_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/paged_kv_cache.h"
#include <array>
#include <bit>
#include <cmath>
#include <algorithm>
#include <limits>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {
namespace {

constexpr std::array<int8_t, 16> kNvfp4Codebook = {
    0, 1, 2, 3, 4, 6, 8, 12,
    0, -1, -2, -3, -4, -6, -8, -12
};

inline uint8_t pickNearestNvfp4Code(float value, float scale) {
    if (scale == 0.0f || !std::isfinite(value)) {
        return 0;
    }

    uint8_t best_index = 0;
    float best_error = std::numeric_limits<float>::infinity();
    for (uint8_t idx = 0; idx < kNvfp4Codebook.size(); ++idx) {
        const float reconstructed = static_cast<float>(kNvfp4Codebook[idx]) * scale;
        const float error = std::abs(value - reconstructed);
        if (error < best_error) {
            best_error = error;
            best_index = idx;
        }
    }
    return best_index;
}

inline uint32_t readLittleEndian32(const std::vector<uint8_t>& data, size_t offset) {
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}

inline void writeLittleEndian32(std::vector<uint8_t>& data, size_t offset, uint32_t value) {
    data[offset + 0] = static_cast<uint8_t>(value & 0xFFu);
    data[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    data[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    data[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

} // namespace

PagedKVCache::PagedKVCache(const Config& config, std::shared_ptr<PagedBlockManager> block_manager)
    : config_(config)
    , block_manager_(block_manager) {
}

PagedKVCache::~PagedKVCache() {
    /**
     * @brief Clean up all sequences
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    block_tables_.clear();
    kv_storage_.clear();
    kv_storage_quantized_.clear();
    quantization_metadata_.clear();
}

/**
 * @brief Store.
 * @param[in] sequence_id Identifier of the sequence.
 * @param[in] layer_id Identifier of the layer.
 * @param[in] kv_data Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), calculateKVSize(), size(), getBlockMapping(), allocateBlocks(), evictLRU().
 */
bool PagedKVCache::store(uint64_t sequence_id, size_t layer_id, const std::vector<float>& kv_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get or create block table for this sequence
    auto it = block_tables_.find(sequence_id);
    if (it == block_tables_.end()) {
        BlockTable::Config bt_config;
        bt_config.block_size = config_.block_size;
        bt_config.enable_cow = config_.enable_prefix_caching;
        
        block_tables_[sequence_id] = std::make_shared<BlockTable>(
            block_manager_, sequence_id, bt_config);
        it = block_tables_.find(sequence_id);
    }
    
    auto block_table = it->second;
    
    // Calculate how many blocks we need
    size_t kv_size_per_token = calculateKVSize();
    size_t num_tokens = kv_data.size() / kv_size_per_token;
    size_t num_blocks_needed = (num_tokens + config_.block_size - 1) / config_.block_size;
    
    // Allocate blocks, retrying with LRU eviction up to 3 times
    auto current_blocks = block_table->getBlockMapping();
    if (current_blocks.size() < num_blocks_needed) {
        size_t blocks_to_allocate = num_blocks_needed - current_blocks.size() ;

        constexpr int kMaxEvictionRetries = 3;
        bool allocated = false;
        for (int attempt = 0; attempt <= kMaxEvictionRetries; ++attempt) {
            block_table->allocateBlocks(blocks_to_allocate);
            current_blocks = block_table->getBlockMapping();
            if (current_blocks.size() >= num_blocks_needed) {
                allocated = true;
                break;
            }
            // Still not enough — evict LRU and retry
            if (!evictLRU()) {
                break;  // Nothing left to evict
            }
            // Recalculate remaining need after eviction
            blocks_to_allocate = num_blocks_needed - current_blocks.size() ;
        }

        if (!allocated) {
            return false;
        }
    }

    // Touch LRU: move sequence_id to front (most-recently-used)
    auto lru_it = lru_map_.find(sequence_id);
    if (lru_it != lru_map_.end()) {
        lru_order_.erase(lru_it->second);
    }
    lru_order_.push_front(sequence_id);
    lru_map_[sequence_id] = lru_order_.begin();

    // Store KV data in blocks
    for (size_t i = 0; i < current_blocks.size(); ++i) {
        int block_id = current_blocks[i];
        
        // Calculate offset for this block
        size_t start_token = i * config_.block_size;
        size_t end_token = std::min(start_token + config_.block_size, num_tokens);
        size_t start_idx = start_token * kv_size_per_token;
        size_t end_idx = end_token * kv_size_per_token;
        
        // Copy KV data for this block
        if (end_idx <= kv_data.size()) {
            kv_storage_[block_id][layer_id] = std::vector<float>(
                kv_data.begin() + start_idx,
                kv_data.begin() + end_idx
            );
        }
    }

    return true;
}

std::vector<float> PagedKVCache::retrieve(uint64_t sequence_id, size_t layer_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = block_tables_.find(sequence_id);
    if (it == block_tables_.end()) {
        return {};
    }

    // Touch LRU: move to front (most-recently-used)
    auto lru_it = lru_map_.find(sequence_id);
    if (lru_it != lru_map_.end()) {
        lru_order_.erase(lru_it->second);
    }
    lru_order_.push_front(sequence_id);
    lru_map_[sequence_id] = lru_order_.begin();
    
    auto block_table = it->second;
    auto block_ids = block_table->getBlockMapping();
    
    std::vector<float> result;
    
    // Retrieve KV data from all blocks
    for (int block_id : block_ids) {
        auto block_it = kv_storage_.find(block_id);
        if (block_it != kv_storage_.end()) {
            auto layer_it = block_it->second.find(layer_id);
            if (layer_it != block_it->second.end()) {
                const auto& block_kv = layer_it->second;
                result.insert(result.end(), block_kv.begin(), block_kv.end());
            }
        }
    }
    
    return result;
}

/**
 * @brief Share Prefix.
 * @param[in] new_sequence_id Identifier of the new sequence.
 * @param[in] parent_sequence_id Identifier of the parent sequence.
 * @param[in] prefix_length Input parameter.
 * @details Calls: lock(), find(), end().
 */
void PagedKVCache::sharePrefix(uint64_t new_sequence_id, uint64_t parent_sequence_id, size_t prefix_length) {
    if (!config_.enable_prefix_caching) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto parent_it = block_tables_.find(parent_sequence_id);
    if (parent_it == block_tables_.end()) {
        return;
    }
    
    // Create new block table
    BlockTable::Config bt_config;
    bt_config.block_size = config_.block_size;
    bt_config.enable_cow = true;
    
    auto new_block_table = std::make_shared<BlockTable>(
        block_manager_, new_sequence_id, bt_config);
    
    // Share prefix blocks
    new_block_table->sharePrefix(parent_sequence_id, prefix_length);
    
    block_tables_[new_sequence_id] = new_block_table;
}

/**
 * @brief Get Block Table.
 * @param[in] sequence_id Identifier of the sequence.
 * @return Return value.
 * @details Calls: lock(), find(), end().
 */
std::shared_ptr<BlockTable> PagedKVCache::getBlockTable(uint64_t sequence_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = block_tables_.find(sequence_id);
    if (it != block_tables_.end()) {
        return it->second;
    }
    
    return nullptr;
}

/**
 * @brief Remove Sequence.
 * @param[in] sequence_id Identifier of the sequence.
 * @details Calls: lock(), find(), end(), erase().
 */
void PagedKVCache::removeSequence(uint64_t sequence_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = block_tables_.find(sequence_id);
    if (it != block_tables_.end()) {
        // Blocks will be released by BlockTable destructor
        block_tables_.erase(it);
    }

    // Clean up LRU structures
    auto lru_it = lru_map_.find(sequence_id);
    if (lru_it != lru_map_.end()) {
        lru_order_.erase(lru_it->second);
        lru_map_.erase(lru_it);
    }
}

/**
 * @brief Evict LRU.
 * @return True when the operation succeeds.
 * @details Calls: empty(), back(), pop_back(), erase(), find(), end(), getBlockMapping(), fetch_add().
 */
bool PagedKVCache::evictLRU() {
    // Must be called while holding mutex_
    if (lru_order_.empty()) {
        return false;
    }

    uint64_t victim_id = lru_order_.back();
    lru_order_.pop_back();
    lru_map_.erase(victim_id);

    auto victim_it = block_tables_.find(victim_id);
    if (victim_it == block_tables_.end()) {
        return false;
    }
    const auto victim_blocks = victim_it->second->getBlockMapping();

    // Release block table (BlockTable destructor returns blocks to free list).
    block_tables_.erase(victim_it);

    // Clear KV payloads for all freed block IDs so reused blocks cannot expose
    // stale per-layer entries from the evicted sequence.
    for (int block_id : victim_blocks) {
        kv_storage_.erase(block_id);
        kv_storage_quantized_.erase(block_id);
        quantization_metadata_.erase(block_id);
    }

    uint64_t total = eviction_count_.fetch_add(1, std::memory_order_relaxed) + 1;
    spdlog::info("[KVCACHE] LRU evicted seq={}, evictions_total={}", victim_id, total);

    return true;
}

PagedKVCache::Stats PagedKVCache::getStats() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.num_sequences = block_tables_.size();
    
    size_t total_blocks = 0;
    size_t shared_blocks = 0;
    
    for (const auto& [seq_id, block_table] : block_tables_) {
        auto bt_stats = block_table->getStats();
        total_blocks += bt_stats.num_blocks;
        shared_blocks += bt_stats.num_shared_blocks;
    }
    
    stats.blocks_used = total_blocks;
    stats.blocks_free = config_.num_blocks > total_blocks ? 
                       config_.num_blocks - total_blocks : 0;
    stats.fragmentation_rate = 0.0;  // Would calculate based on allocation pattern
    stats.prefix_sharing_ratio = total_blocks > 0 ? 
                                static_cast<double>(shared_blocks) / total_blocks : 0.0;
    
    return stats;
}

size_t PagedKVCache::calculateKVSize() const {
    // KV cache size per token: 2 (K and V) * num_kv_heads * head_dim
    return 2 * config_.num_kv_heads * config_.head_dim;
}

std::vector<uint8_t> PagedKVCache::quantizeKVData(
    const std::vector<float>& kv_data, 
    KVQuantizationType target_type) const {
    
    if (kv_data.empty()) {
        return {};
    }
    
    switch (target_type) {
        case KVQuantizationType::FP16: {
            // FP16 quantization (2 bytes per float)
            std::vector<uint8_t> result = {};

            result.reserve(kv_data.size() * 2);
            
            for (float value : kv_data) {
                // Simple FP32 -> FP16 conversion (actual implementation would use CUDA/specialized code)
                // For now, use bit-level approximation
                uint32_t bits = std::bit_cast<uint32_t>(value);
                uint16_t half = static_cast<uint16_t>((bits >> 16) & 0xFFFF);
                result.push_back(static_cast<uint8_t>(half & 0xFF));
                result.push_back(static_cast<uint8_t>((half >> 8) & 0xFF));
            }
            return result;
        }
        
        case KVQuantizationType::INT8: {
            if (kv_data.empty()) return {};

            float max_abs = 0.0f;
            for (float v : kv_data) {
                max_abs = std::max(max_abs, std::abs(v));
            }

            const float scale = max_abs > 1e-6f ? (max_abs / 127.0f) : 1.0f;
            std::vector<uint8_t> result;
            result.reserve(8 + kv_data.size());

            uint32_t scale_bits = std::bit_cast<uint32_t>(scale);
            uint32_t count_bits = static_cast<uint32_t>(kv_data.size());
            for (int i = 0; i < 4; ++i) {
                result.push_back(static_cast<uint8_t>((scale_bits >> (i * 8)) & 0xFFu));
            }
            for (int i = 0; i < 4; ++i) {
                result.push_back(static_cast<uint8_t>((count_bits >> (i * 8)) & 0xFFu));
            }

            for (float v : kv_data) {
                const int q = std::clamp(static_cast<int>(std::lround(v / scale)), -127, 127);
                result.push_back(static_cast<uint8_t>(static_cast<int8_t>(q)));
            }
            return result;
        }
        
        case KVQuantizationType::NVFP4: {
            if (kv_data.empty()) {
                return {};
            }

            constexpr size_t kBlockSize = 64;
            constexpr size_t kSubBlockSize = 16;
            constexpr size_t kSubBlocksPerBlock = kBlockSize / kSubBlockSize;

            std::vector<uint8_t> result;
            result.reserve((kv_data.size() / kBlockSize + 1) * (kSubBlocksPerBlock * 4 + kBlockSize / 2));

            const size_t num_blocks = (kv_data.size() + kBlockSize - 1) / kBlockSize;
            for (size_t block_index = 0; block_index < num_blocks; ++block_index) {
                const size_t block_start = block_index * kBlockSize;
                const size_t block_end = std::min(block_start + kBlockSize, kv_data.size());

                for (size_t sub_index = 0; sub_index < kSubBlocksPerBlock; ++sub_index) {
                    const size_t sub_start = block_start + sub_index * kSubBlockSize;
                    const size_t sub_end = std::min(sub_start + kSubBlockSize, block_end);
                    if (sub_start >= sub_end) {
                        continue;
                    }

                    float amax = 0.0f;
                    for (size_t i = sub_start; i < sub_end; ++i) {
                        amax = std::max(amax, std::abs(kv_data[i]));
                    }

                    const float scale = amax > 0.0f ? (amax / 6.0f) : 0.0f;
                    const uint32_t scale_bits = std::bit_cast<uint32_t>(scale);
                    const size_t scale_offset = result.size();
                    result.resize(result.size() + 4);
                    writeLittleEndian32(result, scale_offset, scale_bits);

                    for (size_t j = sub_start; j < sub_end; j += 2) {
                        const float value0 = (j < kv_data.size()) ? kv_data[j] : 0.0f;
                        const float value1 = ((j + 1) < kv_data.size()) ? kv_data[j + 1] : 0.0f;

                        const uint8_t code0 = pickNearestNvfp4Code(value0, scale);
                        const uint8_t code1 = pickNearestNvfp4Code(value1, scale);
                        result.push_back(static_cast<uint8_t>(code0 | (code1 << 4)));
                    }
                }
            }

            return result;
        }
        default:
            break;
    }
    
    return {};
}

std::vector<float> PagedKVCache::dequantizeKVData(
    const std::vector<uint8_t>& quantized_data,
    KVQuantizationType source_type) const {
    
    if (quantized_data.empty()) {
        return {};
    }
    
    switch (source_type) {
        case KVQuantizationType::FP16: {
            // FP16 dequantization (2 bytes per float)
            std::vector<float> result = {};

            result.reserve(quantized_data.size() / 2);
            
            for (size_t i = 0; i + 1 < quantized_data.size(); i += 2) {
                uint16_t half = (static_cast<uint16_t>(quantized_data[i + 1]) << 8) | quantized_data[i];
                uint32_t bits = (static_cast<uint32_t>(half) << 16);
                result.push_back(std::bit_cast<float>(bits));
            }
            return result;
        }
        
        case KVQuantizationType::INT8: {
            if (quantized_data.size() < 8) return {};

            uint32_t scale_bits = 0;
            uint32_t count_bits = 0;
            for (int i = 0; i < 4; ++i) {
                scale_bits |= static_cast<uint32_t>(quantized_data[i]) << (i * 8);
                count_bits |= static_cast<uint32_t>(quantized_data[4 + i]) << (i * 8);
            }

            const float scale = std::bit_cast<float>(scale_bits);
            const size_t value_count = static_cast<size_t>(count_bits);
            if (value_count == 0) {
                return {};
            }

            std::vector<float> result;
            result.reserve(value_count);
            for (size_t i = 8; i < quantized_data.size() && i - 8 < value_count; ++i) {
                const int8_t quantized = static_cast<int8_t>(quantized_data[i]);
                result.push_back(static_cast<float>(quantized) * scale);
            }
            return result;
        }
        
        case KVQuantizationType::NVFP4: {
            if (quantized_data.empty()) {
                return {};
            }

            constexpr size_t kBlockSize = 64;
            constexpr size_t kSubBlockSize = 16;
            constexpr size_t kSubBlocksPerBlock = kBlockSize / kSubBlockSize;

            std::vector<float> result;
            result.reserve(quantized_data.size() * 2);

            size_t offset = 0;
            while (offset < quantized_data.size()) {
                // Each sub-block stores one float32 scale and 8 bytes of packed 4-bit values.
                for (size_t sub = 0; sub < kSubBlocksPerBlock; ++sub) {
                    if (offset + 4 > quantized_data.size()) {
                        return result;
                    }
                    const uint32_t scale_bits = readLittleEndian32(quantized_data, offset);
                    offset += 4;
                    const float scale = std::bit_cast<float>(scale_bits);

                    for (size_t pair = 0; pair < kSubBlockSize / 2; ++pair) {
                        if (offset >= quantized_data.size()) {
                            return result;
                        }
                        const uint8_t packed = quantized_data[offset++];
                        const uint8_t code0 = packed & 0x0Fu;
                        const uint8_t code1 = (packed >> 4) & 0x0Fu;
                        result.push_back(static_cast<float>(kNvfp4Codebook[code0]) * scale);
                        result.push_back(static_cast<float>(kNvfp4Codebook[code1]) * scale);
                    }
                }
            }

            return result;
        }
        default:
            break;
    }
    
    return {};
}

/**
 * @brief Get Compression Factor.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Implements getCompressionFactor without additional internal calls.
 */
float PagedKVCache::getCompressionFactor(KVQuantizationType type) {
    switch (type) {
        case KVQuantizationType::FP16:
            return 0.5f;  // 50% compression (4 bytes -> 2 bytes)
        case KVQuantizationType::INT8:
            return 0.75f; // 75% compression (4 bytes -> 1 byte, plus small metadata overhead per block)
        case KVQuantizationType::NVFP4:
            return 0.25f; // Stable compact representation for the KV stream.
        default: break;
    }
    return 1.0f;
}

/**
 * @brief Get Expected Accuracy.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Implements getExpectedAccuracy without additional internal calls.
 */
float PagedKVCache::getExpectedAccuracy(KVQuantizationType type) {
    switch (type) {
        case KVQuantizationType::FP16:
            return 0.999f; // ~99.9% accuracy vs FP32 baseline
        case KVQuantizationType::INT8:
            return 0.98f;  // ~98% accuracy
        case KVQuantizationType::NVFP4:
            return 0.99f;  // ~99% accuracy (target per requirements)
        default: break;
    }
    return 1.0f;
}

/**
 * @brief Get Bit Width For Quantization Type.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Implements getBitWidthForQuantizationType without additional internal calls.
 */
int PagedKVCache::getBitWidthForQuantizationType(KVQuantizationType type) {
    switch (type) {
        case KVQuantizationType::FP16:
            return 16;  // Half-precision floating point
        case KVQuantizationType::INT8:
            return 8;   // 8-bit signed integer
        case KVQuantizationType::NVFP4:
            return 4;   // 4-bit NVIDIA floating point
        default: break;
    }
    return 32;  // Default to FP32 (no quantization)
}

/**
 * @brief Quantize To NVFP4.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: std::min().
 */
uint8_t PagedKVCache::quantizeToNVFP4(float value) {
    if (value == 0.0f) {
        return 0x00;
    }

    const float magnitude = std::abs(value);
    const float scale = std::max(magnitude / 6.0f, 1.0e-6f);
    const uint8_t code = pickNearestNvfp4Code(value, scale);
    return static_cast<uint8_t>(code);
}

/**
 * @brief Dequantize From NVFP4.
 * @param[in] packed Input parameter.
 * @return Return value.
 * @details Implements dequantizeFromNVFP4 without additional internal calls.
 */
float PagedKVCache::dequantizeFromNVFP4(uint8_t packed) {
    const uint8_t code = packed & 0x0Fu;
    return static_cast<float>(kNvfp4Codebook[code]);
}

/**
 * @brief Quantize To INT8.
 * @param[in] values Input parameter.
 * @param[in,out] scale Input/output parameter.
 * @param[in,out] zero_point Input/output parameter.
 * @return Return value.
 * @details Calls: empty(), std::min(), std::max(), std::round(), reserve(), size(), int8_t(), push_back().
 */
std::vector<int8_t> PagedKVCache::quantizeToINT8(
    const std::vector<float>& values,
    float& scale,
    int8_t& zero_point) {
    
    if (values.empty()) {
        scale = 1.0f;
        zero_point = 0;
        return {};
    }
    
    float min_val = values[0], max_val = values[0];
    for (float v : values) {
        min_val = std::min(min_val, v);
        max_val = std::max(max_val, v);
    }
    
    scale = (max_val - min_val) / 255.0f;
    if (scale < 1e-6f) {
      scale = 1.0f;
    }
    
    zero_point = static_cast<int8_t>(std::round(-min_val / scale));
    
    std::vector<int8_t> result = {};

    result.reserve(values.size());
    
    for (float v : values) {
        int8_t quantized = static_cast<int8_t>(std::round(v / scale + zero_point));
        quantized = std::max(int8_t(-128), std::min(int8_t(127), quantized));
        result.push_back(quantized);
    }
    
    return result;
}

/**
 * @brief Dequantize From INT8.
 * @param[in] quantized Input parameter.
 * @param[in] scale Input parameter.
 * @param[in] zero_point Input parameter.
 * @return Return value.
 * @details Calls: reserve(), size(), push_back().
 */
std::vector<float> PagedKVCache::dequantizeFromINT8(
    const std::vector<int8_t>& quantized,
    float scale,
    int8_t zero_point) {
    
    std::vector<float> result = {};

    result.reserve(quantized.size());
    
    for (int8_t q : quantized) {
        result.push_back((static_cast<float>(q) - zero_point) * scale);
    }
    
    return result;
}

} // namespace llm
} // namespace themis
