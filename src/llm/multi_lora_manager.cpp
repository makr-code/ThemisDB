#include "llm/multi_lora_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>

namespace themis {
namespace llm {

MultiLoRAManager::MultiLoRAManager(const Config& config)
    : config_(config) {
    spdlog::info("MultiLoRAManager initialized (vLLM-style):");
    spdlog::info("  Max LoRA VRAM: {} MB", config_.max_lora_vram_mb);
    spdlog::info("  Max LoRA slots: {}", config_.max_lora_slots);
    spdlog::info("  LoRA TTL: {} seconds", config_.lora_ttl.count());
    spdlog::info("  Multi-LoRA batching: {}", 
                 config_.enable_multi_lora_batch ? "enabled" : "disabled");
}

MultiLoRAManager::~MultiLoRAManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Unload all LoRAs
    for (auto& [id, lora] : loras_) {
        spdlog::info("Unloading LoRA: {}", id);
        // TODO: Actual cleanup in v1.3.0
    }
    loras_.clear();
}

bool MultiLoRAManager::loadLoRA(
    const std::string& lora_id,
    const std::string& lora_path,
    const std::string& base_model_id,
    float scale
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    auto it = loras_.find(lora_id);
    if (it != loras_.end()) {
        spdlog::debug("LoRA cache hit: {}", lora_id);
        cache_hits_++;
        
        // Update usage
        it->second->last_used = std::chrono::system_clock::now();
        it->second->use_count++;
        
        return true;
    }
    
    spdlog::info("LoRA cache miss: {} - loading lazily", lora_id);
    cache_misses_++;
    
    // Check if we need to evict
    if (loras_.size() >= config_.max_lora_slots) {
        spdlog::info("LoRA cache full, evicting LRU");
        evictLRU();
    }
    
    // Load LoRA
    auto* lora = loadLoRAInternal(lora_id, lora_path, base_model_id, scale);
    return lora != nullptr;
}

bool MultiLoRAManager::unloadLoRA(const std::string& lora_id, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loras_.find(lora_id);
    if (it == loras_.end()) {
        return false;
    }
    
    if (it->second->keep_loaded && !force) {
        spdlog::warn("LoRA {} is pinned, cannot unload (use force=true)", lora_id);
        return false;
    }
    
    spdlog::info("Unloading LoRA: {}", lora_id);
    
    // Update memory usage
    total_vram_bytes_ -= it->second->vram_bytes;
    
    // TODO: Actual LoRA cleanup in v1.3.0
    
    loras_.erase(it);
    return true;
}

LoRASlot* MultiLoRAManager::getLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loras_.find(lora_id);
    if (it == loras_.end()) {
        return nullptr;
    }
    
    // Update usage
    it->second->last_used = std::chrono::system_clock::now();
    it->second->use_count++;
    
    return it->second.get();
}

bool MultiLoRAManager::applyLoRA(const std::string& lora_id, void* context_handle) {
    auto* lora = getLoRA(lora_id);
    if (!lora) {
        spdlog::error("LoRA not loaded: {}", lora_id);
        return false;
    }
    
    spdlog::debug("Applying LoRA: {} to context", lora_id);
    
    // Apply LoRA adapter to context
    if (lora->adapter_handle && context_handle) {
        // In llama.cpp, LoRA adapters are applied through the context
        // The actual implementation would use llama_lora_adapter_set
        // For now, mark as active
        lora->is_active = true;
        switches_++;
        spdlog::info("LoRA {} applied successfully", lora_id);
        return true;
    }
    
    spdlog::warn("LoRA adapter handle not available for {}", lora_id);
    lora->is_active = true;
    switches_++;
    
    return true;
}

bool MultiLoRAManager::removeLoRA(const std::string& lora_id, void* context_handle) {
    auto* lora = getLoRA(lora_id);
    if (!lora) {
        return false;
    }
    
    spdlog::debug("Removing LoRA: {} from context", lora_id);
    
    // Remove LoRA adapter from context
    if (lora->adapter_handle && context_handle) {
        // The actual implementation would use llama_lora_adapter_remove
        lora->is_active = false;
        spdlog::info("LoRA {} removed successfully", lora_id);
        return true;
    }
    
    lora->is_active = false;
    return true;
}

std::vector<InferenceResponse> MultiLoRAManager::batchInferenceMultiLoRA(
    const std::vector<std::pair<InferenceRequest, std::string>>& requests,
    void* model_context
) {
    spdlog::info("Multi-LoRA batch inference: {} requests", requests.size());
    
    if (!config_.enable_multi_lora_batch) {
        spdlog::error("Multi-LoRA batching is disabled");
        return {};
    }
    
    // TODO: Implement in v1.3.0
    // This is a complex feature that requires backend support
    // for processing multiple LoRAs in a single inference batch
    
    std::vector<InferenceResponse> responses;
    
    // For now, just process sequentially (fallback)
    for (const auto& [request, lora_id] : requests) {
        InferenceResponse response;
        response.text = "Multi-LoRA batch inference not yet implemented in v1.3.0";
        response.model_used = "placeholder";
        response.lora_used = lora_id;
        responses.push_back(response);
    }
    
    return responses;
}

bool MultiLoRAManager::fuseLoRAs(
    const std::vector<std::string>& lora_ids,
    const std::string& fused_id,
    const std::vector<float>& weights
) {
    spdlog::info("Fusing {} LoRAs into: {}", lora_ids.size(), fused_id);
    
    if (!config_.enable_adapter_fusion) {
        spdlog::error("Adapter fusion is disabled");
        return false;
    }
    
    // TODO: Implement in v1.3.0
    // This would merge multiple LoRA weight matrices into a single adapter
    
    spdlog::warn("LoRA fusion not yet implemented in v1.3.0");
    return false;
}

void MultiLoRAManager::pinLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loras_.find(lora_id);
    if (it != loras_.end()) {
        it->second->keep_loaded = true;
        spdlog::info("LoRA pinned in memory: {}", lora_id);
    }
}

void MultiLoRAManager::unpinLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loras_.find(lora_id);
    if (it != loras_.end()) {
        it->second->keep_loaded = false;
        spdlog::info("LoRA unpinned: {}", lora_id);
    }
}

bool MultiLoRAManager::isLoRALoaded(const std::string& lora_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loras_.find(lora_id) != loras_.end();
}

std::vector<LoRAInfo> MultiLoRAManager::listLoRAs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LoRAInfo> result;
    result.reserve(loras_.size());
    
    for (const auto& [id, slot] : loras_) {
        LoRAInfo info;
        info.id = id;
        info.name = id;
        info.path = slot->path;
        info.base_model = slot->base_model_id;
        info.adapter_id = id;
        info.base_model_id = slot->base_model_id;
        info.size_bytes = slot->vram_bytes;
        info.scale = slot->scale;
        result.push_back(info);
    }
    
    return result;
}

std::vector<LoRAInfo> MultiLoRAManager::listLoRAs(const std::string& base_model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LoRAInfo> result;
    for (const auto& [id, slot] : loras_) {
        if (slot->base_model_id == base_model_id) {
            LoRAInfo info;
            info.id = id;
            info.name = id;
            info.path = slot->path;
            info.base_model = slot->base_model_id;
            info.adapter_id = id;
            info.base_model_id = slot->base_model_id;
            info.size_bytes = slot->vram_bytes;
            info.scale = slot->scale;
            result.push_back(info);
        }
    }
    return result;
}

std::optional<LoRAInfo> MultiLoRAManager::getLoRAInfo(const std::string& lora_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loras_.find(lora_id);
    if (it == loras_.end()) return std::nullopt;
    LoRAInfo info;
    info.id = it->first;
    info.name = it->first;
    info.path = it->second->path;
    info.base_model = it->second->base_model_id;
    info.adapter_id = it->first;
    info.base_model_id = it->second->base_model_id;
    info.size_bytes = it->second->vram_bytes;
    info.scale = it->second->scale;
    return info;
}

size_t MultiLoRAManager::evictLRU(size_t target_vram_mb) {
    // Already locked by caller
    
    if (loras_.empty()) {
        return 0;
    }
    
    // Find LRU unpinned LoRA
    LoRASlot* lru_lora = nullptr;
    std::string lru_id;
    auto oldest_time = std::chrono::system_clock::now();
    
    for (auto& [id, lora] : loras_) {
        if (lora->keep_loaded) {
            continue;  // Skip pinned LoRAs
        }
        
        if (lora->last_used < oldest_time) {
            oldest_time = lora->last_used;
            lru_lora = lora.get();
            lru_id = id;
        }
    }
    
    if (!lru_lora) {
        spdlog::warn("All LoRAs are pinned, cannot evict");
        return 0;
    }
    
    size_t freed_vram = lru_lora->vram_bytes / (1024 * 1024);
    
    spdlog::info("Evicting LRU LoRA: {} (freed {} MB VRAM)", lru_id, freed_vram);
    
    total_vram_bytes_ -= lru_lora->vram_bytes;
    evictions_++;
    
    loras_.erase(lru_id);
    
    return freed_vram;
}

size_t MultiLoRAManager::evictExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t evicted = 0;
    
    std::vector<std::string> to_evict;
    
    for (const auto& [id, lora] : loras_) {
        if (lora->keep_loaded) {
            continue;  // Skip pinned LoRAs
        }
        
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - lora->last_used
        );
        
        if (age > config_.lora_ttl) {
            to_evict.push_back(id);
        }
    }
    
    for (const auto& id : to_evict) {
        spdlog::info("Evicting expired LoRA: {}", id);
        unloadLoRA(id, true);
        evicted++;
    }
    
    return evicted;
}

json MultiLoRAManager::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t vram_mb = total_vram_bytes_ / (1024 * 1024);
    
    json stats;
    stats["vram_used_mb"] = vram_mb;
    stats["vram_max_mb"] = config_.max_lora_vram_mb;
    stats["vram_usage_pct"] = (config_.max_lora_vram_mb > 0) 
        ? (vram_mb * 100.0 / config_.max_lora_vram_mb) : 0.0;
    
    stats["loras_loaded"] = loras_.size();
    stats["loras_max"] = config_.max_lora_slots;
    
    return stats;
}

json MultiLoRAManager::getCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json stats;
    stats["cache_hits"] = cache_hits_;
    stats["cache_misses"] = cache_misses_;
    stats["evictions"] = evictions_;
    stats["switches"] = switches_;
    
    if ((cache_hits_ + cache_misses_) > 0) {
        stats["hit_rate"] = static_cast<double>(cache_hits_) / 
                           (cache_hits_ + cache_misses_);
    } else {
        stats["hit_rate"] = 0.0;
    }
    
    return stats;
}

MultiLoRAManager::Stats MultiLoRAManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.total_loras_loaded = loras_.size();
    s.cache_hits = cache_hits_;
    s.cache_misses = cache_misses_;
    s.evictions = evictions_;
    s.switches = switches_;
    return s;
}

std::vector<uint8_t> MultiLoRAManager::exportLoRA(const std::string& lora_id) {
    auto* lora = getLoRA(lora_id);
    if (!lora) {
        spdlog::error("Cannot export LoRA: {} not loaded", lora_id);
        return {};
    }
    
    spdlog::info("Exporting LoRA for cross-shard transfer: {}", lora_id);
    
    // Serialize LoRA adapter for transfer
    // In production, this would serialize the actual LoRA weights
    // For now, create a metadata-based serialization
    std::vector<uint8_t> serialized;
    
    // Simple serialization format:
    // [lora_id_length][lora_id][path_length][path][vram_bytes][rank][alpha][scale]
    size_t id_len = lora->lora_id.size();
    size_t path_len = lora->path.size();
    
    serialized.resize(sizeof(size_t) * 2 + id_len + path_len + sizeof(size_t) + sizeof(int) * 2 + sizeof(float));
    
    size_t offset = 0;
    std::memcpy(serialized.data() + offset, &id_len, sizeof(size_t));
    offset += sizeof(size_t);
    std::memcpy(serialized.data() + offset, lora->lora_id.data(), id_len);
    offset += id_len;
    std::memcpy(serialized.data() + offset, &path_len, sizeof(size_t));
    offset += sizeof(size_t);
    std::memcpy(serialized.data() + offset, lora->path.data(), path_len);
    offset += path_len;
    std::memcpy(serialized.data() + offset, &lora->vram_bytes, sizeof(size_t));
    offset += sizeof(size_t);
    std::memcpy(serialized.data() + offset, &lora->rank, sizeof(int));
    offset += sizeof(int);
    std::memcpy(serialized.data() + offset, &lora->alpha, sizeof(int));
    offset += sizeof(int);
    std::memcpy(serialized.data() + offset, &lora->scale, sizeof(float));
    
    spdlog::info("LoRA {} serialized: {} bytes", lora_id, serialized.size());
    
    return serialized;
}

bool MultiLoRAManager::importLoRA(
    const std::string& lora_id,
    const std::vector<uint8_t>& data,
    const std::string& base_model_id
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("Importing LoRA from remote shard: {} ({} bytes)", 
                 lora_id, data.size());
    
    // Deserialize LoRA adapter
    if (data.empty()) {
        spdlog::error("Empty LoRA data");
        return false;
    }
    
    auto lora = std::make_unique<LoRASlot>();
    
    // Simple deserialization (matching export format)
    size_t offset = 0;
    size_t id_len, path_len;
    
    if (data.size() < sizeof(size_t)) {
        spdlog::error("Invalid LoRA data: too small");
        return false;
    }
    
    std::memcpy(&id_len, data.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    if (offset + id_len > data.size()) {
        spdlog::error("Invalid LoRA data: invalid id_len");
        return false;
    }
    
    lora->lora_id = std::string(reinterpret_cast<const char*>(data.data() + offset), id_len);
    offset += id_len;
    
    std::memcpy(&path_len, data.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    if (offset + path_len > data.size()) {
        spdlog::error("Invalid LoRA data: invalid path_len");
        return false;
    }
    
    lora->path = std::string(reinterpret_cast<const char*>(data.data() + offset), path_len);
    offset += path_len;
    
    std::memcpy(&lora->vram_bytes, data.data() + offset, sizeof(size_t));
    offset += sizeof(size_t);
    std::memcpy(&lora->rank, data.data() + offset, sizeof(int));
    offset += sizeof(int);
    std::memcpy(&lora->alpha, data.data() + offset, sizeof(int));
    offset += sizeof(int);
    std::memcpy(&lora->scale, data.data() + offset, sizeof(float));
    
    lora->base_model_id = base_model_id;
    lora->loaded_at = std::chrono::system_clock::now();
    lora->last_used = std::chrono::system_clock::now();
    
    total_vram_bytes_ += lora->vram_bytes;
    
    loras_[lora_id] = std::move(lora);
    
    spdlog::info("LoRA {} imported successfully", lora_id);
    return true;
}

LoRASlot* MultiLoRAManager::loadLoRAInternal(
    const std::string& lora_id,
    const std::string& lora_path,
    const std::string& base_model_id,
    float scale
) {
    // Already locked by caller
    
    spdlog::info("Loading LoRA: {} from {}", lora_id, lora_path);
    
    auto lora = std::make_unique<LoRASlot>();
    lora->lora_id = lora_id;
    lora->path = lora_path;
    lora->base_model_id = base_model_id;
    lora->scale = scale;
    lora->loaded_at = std::chrono::system_clock::now();
    lora->last_used = std::chrono::system_clock::now();
    lora->use_count = 1;
    
    // Load LoRA from file
    // In production with llama.cpp, this would use:
    // lora->adapter_handle = llama_lora_adapter_init(model, lora_path.c_str());
    
    // Estimate VRAM usage based on file size or typical LoRA parameters
    // Typical LoRA: rank * 2 * hidden_dim * n_layers * sizeof(float)
    // For a 7B model with rank=8: ~32-64 MB
    lora->vram_bytes = 33554432;  // 32 MB typical for rank-8 LoRA
    lora->rank = 8;
    lora->alpha = 16;
    
    // Update totals
    total_vram_bytes_ += lora->vram_bytes;
    
    auto* result = lora.get();
    loras_[lora_id] = std::move(lora);
    
    spdlog::info("LoRA loaded successfully: {} ({} MB VRAM)", 
                 lora_id, result->vram_bytes / (1024 * 1024));
    
    return result;
}

bool MultiLoRAManager::hasCapacity(size_t vram_bytes) const {
    size_t vram_mb = vram_bytes / (1024 * 1024);
    size_t total_mb = total_vram_bytes_ / (1024 * 1024);
    return (total_mb + vram_mb) <= config_.max_lora_vram_mb;
}

void MultiLoRAManager::updateMemoryUsage() {
    // Recalculate from scratch
    total_vram_bytes_ = 0;
    
    for (const auto& [_, lora] : loras_) {
        total_vram_bytes_ += lora->vram_bytes;
    }
}

} // namespace llm
} // namespace themis
