/**
 * @file kv_cache_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/attention/kv_cache_manager.h"
#include "utils/type_conversion.h"
#include <stdexcept>
#include <cstring>
#include <limits>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {
namespace attention {

KVCacheManager::KVCacheManager(const FlashAttentionConfig& config)
    : config_(config) {
    
    // IVB-KV-01: Guard zero kv_block_size before any block-size arithmetic
    if (config_.kv_block_size == 0) {
        throw std::invalid_argument("KVCacheManager: kv_block_size must be > 0");
    }

    // Initialize physical blocks
    size_t total_blocks = config_.num_kv_blocks;
    blocks_.resize(total_blocks);
    
    size_t block_size = calculateBlockSize();
    
    for (size_t i = 0; i < total_blocks; ++i) {
        blocks_[i].block_id = static_cast<int>(i);
        blocks_[i].is_free = true;
        blocks_[i].ref_count = 0;
        blocks_[i].data.resize(block_size, 0.0f);
        
        free_blocks_.push(static_cast<int>(i));
    }
}

KVCacheManager::~KVCacheManager() {
    // Cleanup all sequences
    std::lock_guard<std::mutex> lock(mutex_);
    sequences_.clear();
}

BlockTable KVCacheManager::allocateSequence(uint64_t seq_id, int expected_tokens) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if sequence already exists
    if (sequences_.find(seq_id) != sequences_.end()) {
        throw std::runtime_error("Sequence ID already exists");
    }
    
    // IVB-KV-02: Reject non-positive token counts
    if (expected_tokens <= 0) {
        throw std::invalid_argument("allocateSequence: expected_tokens must be > 0");
    }
    
    BlockTable table;
    table.sequence_id = seq_id;
    table.num_tokens = 0;
    
    // IVB-KV-03: Use size_t for block arithmetic to avoid signed/unsigned mismatch
    // and guard against overflow: (expected_tokens - 1) / kv_block_size + 1
    size_t uexpected = static_cast<size_t>(expected_tokens);
    size_t blocks_needed = (uexpected - 1) / config_.kv_block_size + 1;
    
    for (size_t i = 0; i < blocks_needed; ++i) {
        if (free_blocks_.empty()) {
            // Out of memory - free allocated blocks and fail
            for (int block_id : table.block_ids) {
                freeBlock(block_id);
            }
            throw std::runtime_error("Out of KV cache blocks");
        }
        
        int block_id = allocateBlock();
        table.block_ids.push_back(block_id);
    }
    
    sequences_[seq_id] = table;
    return table;
}

void KVCacheManager::freeSequence(uint64_t seq_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sequences_.find(seq_id);
    if (it == sequences_.end()) {
        return; // Sequence not found
    }
    
    // Free all blocks
    for (int block_id : it->second.block_ids) {
        freeBlock(block_id);
    }
    
    sequences_.erase(it);
}

void KVCacheManager::appendToken(uint64_t seq_id, const KVTensor& kv) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sequences_.find(seq_id);
    if (it == sequences_.end()) {
        throw std::runtime_error("Sequence not found");
    }
    
    BlockTable& table = it->second;
    
    // Check if we need to allocate a new block
    // IVB-KV-04: Use size_t division to avoid signed/unsigned mismatch; kv_block_size
    // is already validated > 0 in the constructor so no zero-division risk here.
    size_t token_block_idx = static_cast<size_t>(table.num_tokens) / config_.kv_block_size;
    if (token_block_idx >= table.block_ids.size()) {
        if (free_blocks_.empty()) {
            throw std::runtime_error("Out of KV cache blocks");
        }
        int new_block = allocateBlock();
        table.block_ids.push_back(new_block);
    }
    
    // Copy KV data to block
    int block_id = table.block_ids[token_block_idx];
    Block& block = blocks_[block_id];
    
    // IVB-KV-05: Compute offset using size_t arithmetic to avoid overflow;
    // kv_block_size > 0 is guaranteed by the constructor guard.
    size_t slot_in_block = static_cast<size_t>(table.num_tokens) % config_.kv_block_size;
    size_t offset = slot_in_block * kv.data.size();
    if (offset + kv.data.size() <= block.data.size()) {
        std::memcpy(block.data.data() + offset, kv.data.data(), 
                    kv.data.size() * sizeof(float));
    }
    
    table.num_tokens++;
}

void KVCacheManager::sharePrefix(uint64_t new_seq_id, uint64_t parent_seq_id, 
                                  int prefix_length) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // IVB-KV-07: Reject non-positive prefix_length to avoid wrap-around in size_t cast
    if (prefix_length <= 0) {
        throw std::invalid_argument("sharePrefix: prefix_length must be > 0");
    }

    auto parent_it = sequences_.find(parent_seq_id);
    if (parent_it == sequences_.end()) {
        throw std::runtime_error("Parent sequence not found");
    }
    
    if (sequences_.find(new_seq_id) != sequences_.end()) {
        throw std::runtime_error("New sequence ID already exists");
    }
    
    const BlockTable& parent_table = parent_it->second;
    
    // Create new sequence with shared prefix blocks
    BlockTable new_table;
    new_table.sequence_id = new_seq_id;
    new_table.has_shared_prefix = true;
    new_table.parent_sequence_id = parent_seq_id;
    new_table.shared_prefix_length = prefix_length;
    new_table.num_tokens = prefix_length;
    
    // Share prefix blocks (Copy-on-Write)
    // IVB-KV-06: Use size_t for prefix_blocks to avoid signed/unsigned narrowing;
    // kv_block_size > 0 is guaranteed by the constructor guard.
    size_t uprefix = static_cast<size_t>(prefix_length);
    size_t prefix_blocks = (uprefix - 1) / config_.kv_block_size + 1;
    size_t max_blocks = std::min(prefix_blocks, parent_table.block_ids.size());
    for (size_t i = 0; i < max_blocks; ++i) {
        int block_id = parent_table.block_ids[i];
        new_table.block_ids.push_back(block_id);
        blocks_[block_id].ref_count++;
    }
    
    sequences_[new_seq_id] = new_table;
}

const BlockTable* KVCacheManager::getBlockTable(uint64_t seq_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sequences_.find(seq_id);
    if (it == sequences_.end()) {
        return nullptr;
    }
    
    return &it->second;
}

const Block* KVCacheManager::getBlock(int block_id) const {
    if (block_id < 0 || block_id >= static_cast<int>(blocks_.size())) {
        return nullptr;
    }
    return &blocks_[block_id];
}

AttentionMemoryStats KVCacheManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AttentionMemoryStats stats;
    
    stats.blocks_used = blocks_.size() - free_blocks_.size();
    stats.blocks_free = free_blocks_.size();
    
    size_t block_size = calculateBlockSize();
    stats.kv_cache_bytes = blocks_.size() * block_size * sizeof(float);
    stats.total_memory_bytes = stats.kv_cache_bytes;
    
    if (blocks_.size() > 0) {
        stats.fragmentation_rate = static_cast<double>(free_blocks_.size()) / blocks_.size();
    }
    
    // Calculate prefix sharing ratio
    int shared_blocks = 0;
    for (const auto& block : blocks_) {
        if (block.ref_count > 1) {
            shared_blocks++;
        }
    }
    if (stats.blocks_used > 0) {
        stats.prefix_sharing_ratio = static_cast<double>(shared_blocks) / stats.blocks_used;
    }
    
    return stats;
}

size_t KVCacheManager::getFreeBlockCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return free_blocks_.size();
}

int KVCacheManager::allocateBlock() {
    if (free_blocks_.empty()) {
        return -1;
    }
    
    int block_id = free_blocks_.front();
    free_blocks_.pop();
    
    blocks_[block_id].is_free = false;
    blocks_[block_id].ref_count = 1;
    
    return block_id;
}

void KVCacheManager::freeBlock(int block_id) {
    if (block_id < 0 || block_id >= static_cast<int>(blocks_.size())) {
        return;
    }
    
    Block& block = blocks_[block_id];
    
    // Decrement ref count (for Copy-on-Write)
    block.ref_count--;
    
    if (block.ref_count <= 0) {
        block.is_free = true;
        block.ref_count = 0;
        free_blocks_.push(block_id);
    }
}

size_t KVCacheManager::calculateBlockSize() const {
    // IVB-KV-08: Guard against multiplication chain overflow.
    // Each factor is independently bounded; check product fits in size_t before multiplying.
    if (config_.num_layers <= 0 || config_.head_dim <= 0 || config_.num_kv_heads <= 0) {
        throw std::invalid_argument("calculateBlockSize: num_layers, head_dim, and num_kv_heads must be > 0");
    }
    size_t num_layers   = static_cast<size_t>(config_.num_layers);
    size_t head_dim     = static_cast<size_t>(config_.head_dim);
    size_t num_kv_heads = static_cast<size_t>(config_.num_kv_heads);
    constexpr size_t kMax = std::numeric_limits<size_t>::max();
    if (num_layers > kMax / config_.kv_block_size ||
        head_dim > kMax / (config_.kv_block_size * num_layers) ||
        num_kv_heads > kMax / (config_.kv_block_size * num_layers * head_dim) ||
        2 > kMax / (config_.kv_block_size * num_layers * head_dim * num_kv_heads)) {
        throw std::overflow_error("calculateBlockSize: block size computation overflows size_t");
    }
    return config_.kv_block_size * num_layers * head_dim * num_kv_heads * 2;
}

} // namespace attention
} // namespace llm
} // namespace themis

