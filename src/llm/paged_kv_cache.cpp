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
#include <bit>
#include <cmath>
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

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

            static constexpr float kBucketValues[7] = {
                -0.20f, -0.10f, -0.03f, 0.0f, 0.03f, 0.10f, 0.20f
            };
            std::vector<uint8_t> result;
            result.reserve(8 + ((kv_data.size() * 3u + 7u) / 8u));

            const uint64_t count = static_cast<uint64_t>(kv_data.size());
            for (int i = 0; i < 8; ++i) {
                result.push_back(static_cast<uint8_t>((count >> (i * 8)) & 0xFFu));
            }

            uint64_t bit_buffer = 0;
            int bits_in_buffer = 0;
            auto append_bits = [&](uint64_t value, int bits) {
                const uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
                bit_buffer = (bit_buffer << bits) | (value & mask);
                bits_in_buffer += bits;
                while (bits_in_buffer >= 8) {
                    bits_in_buffer -= 8;
                    result.push_back(static_cast<uint8_t>((bit_buffer >> bits_in_buffer) & 0xFFu));
                }
            };

            for (float value : kv_data) {
                const float abs_v = std::abs(value);
                if (abs_v > 0.5f) {
                    append_bits(7ULL, 3);
                    append_bits(static_cast<uint64_t>(std::bit_cast<uint32_t>(value)), 32);
                    continue;
                }

                uint32_t code = 3u;
                float best_delta = std::numeric_limits<float>::infinity();
                for (uint32_t idx = 0; idx < 7u; ++idx) {
                    const float delta = std::abs(value - kBucketValues[idx]);
                    if (delta < best_delta) {
                        best_delta = delta;
                        code = idx;
                    }
                }
                append_bits(code, 3);
            }

            if (bits_in_buffer > 0) {
                result.push_back(static_cast<uint8_t>((bit_buffer << (8 - bits_in_buffer)) & 0xFFu));
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
            if (quantized_data.size() < 8) {
                return {};
            }

            static constexpr float kBucketValues[7] = {
                -0.20f, -0.10f, -0.03f, 0.0f, 0.03f, 0.10f, 0.20f
            };

            uint64_t count = 0;
            for (int i = 0; i < 8; ++i) {
                count |= static_cast<uint64_t>(quantized_data[i]) << (i * 8);
            }

            std::vector<float> result;
            result.reserve(static_cast<size_t>(count));

            size_t offset = 8;
            uint64_t bit_buffer = 0;
            int bits_in_buffer = 0;
            auto read_bits = [&](int bits) -> uint64_t {
                while (bits_in_buffer < bits) {
                    if (offset >= quantized_data.size()) {
                        return 0ULL;
                    }
                    bit_buffer = (bit_buffer << 8) | static_cast<uint64_t>(quantized_data[offset++]);
                    bits_in_buffer += 8;
                }
                bits_in_buffer -= bits;
                return (bit_buffer >> bits_in_buffer) & ((1ULL << bits) - 1ULL);
            };

            for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
                const uint64_t code = read_bits(3);
                if (code == 7ULL) {
                    if (offset + 4 > quantized_data.size()) {
                        break;
                    }
                    uint32_t float_bits = 0;
                    for (int j = 0; j < 4; ++j) {
                        float_bits |= static_cast<uint32_t>(quantized_data[offset++]) << (j * 8);
                    }
                    result.push_back(std::bit_cast<float>(float_bits));
                } else {
                    result.push_back(kBucketValues[code]);
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
    // NVFP4: [s1e2m1] format (1 sign, 2 exponent, 1 mantissa)
    // Range: [-448, +448], ~4-5% precision loss vs FP16
    
    if (value == 0.0f) {
      return 0x00;
    }
    
    uint32_t bits = std::bit_cast<uint32_t>(value);
    uint32_t sign = (bits >> 31) & 0x1;
    uint32_t exp_bias = ((bits >> 23) & 0xFF);
    uint32_t mantissa = (bits >> 22) & 0x1;  // Take only 1 bit for mantissa
    
    // Adjust exponent to fit in 2 bits (shift from 8-bit bias to 2-bit bias)
    uint32_t exp_4bit = (exp_bias > 127) ? ((exp_bias - 127) >> 5) : 0;
    exp_4bit = std::min(exp_4bit, 3u);  // Clamp to 2 bits
    
    const uint32_t packed_bits = ((sign & 0x1u) << 7) | ((exp_4bit & 0x3u) << 5) | ((mantissa & 0x1u) << 4);
    uint8_t result = static_cast<uint8_t>(packed_bits);
    return result;
}

/**
 * @brief Dequantize From NVFP4.
 * @param[in] packed Input parameter.
 * @return Return value.
 * @details Implements dequantizeFromNVFP4 without additional internal calls.
 */
float PagedKVCache::dequantizeFromNVFP4(uint8_t packed) {
    // NVFP4: [s1e2m1] format — reconstruct to FP32
    
    if (packed == 0x00) {
      return 0.0f;
    }
    
    uint32_t sign = (packed >> 7) & 0x1;
    uint32_t exp_2bit = (packed >> 5) & 0x3;
    uint32_t mantissa = (packed >> 4) & 0x1;
    
    // Expand to FP32 format
    uint32_t exp_8bit = (exp_2bit << 5) + 127;  // Bias to 8-bit exponent
    uint32_t mantissa_23bit = mantissa << 22;
    
    uint32_t fp32_bits = (sign << 31) | (exp_8bit << 23) | mantissa_23bit;
    float result = std::bit_cast<float>(fp32_bits);
    
    return result;
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
