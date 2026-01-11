#include "llm/lora_framework/lora_adapter_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

LoRAAdapterManager::LoRAAdapterManager(const Config& config)
    : config_(config), cache_enabled_(true) {
    spdlog::info("LoRAAdapterManager initialized:");
    spdlog::info("  Max cache size: {}", config_.max_cache_size);
    spdlog::info("  Cache TTL: {} seconds", config_.cache_ttl.count());
    spdlog::info("  Max memory: {} MB", config_.max_memory_mb);
    spdlog::info("  Auto-unload: {}", config_.enable_auto_unload);
}

LoRAAdapterManager::~LoRAAdapterManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("Unloading {} LoRA adapters", adapters_.size());
    
    for (auto& [id, entry] : adapters_) {
        if (entry->adapter_handle) {
            // In production with llama.cpp, this would free the adapter
            // llama_lora_adapter_free(entry->adapter_handle);
            entry->adapter_handle = nullptr;
        }
    }
    
    adapters_.clear();
    spdlog::info("LoRAAdapterManager destroyed");
}

bool LoRAAdapterManager::loadAdapter(
    const std::string& adapter_id,
    const std::string& adapter_path,
    const std::string& base_model,
    float scaling
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    if (adapters_.find(adapter_id) != adapters_.end()) {
        spdlog::debug("Adapter {} already loaded", adapter_id);
        touchAdapter(adapter_id);
        cache_stats_.cache_hits++;
        return true;
    }
    
    cache_stats_.cache_misses++;
    cache_stats_.total_loads++;
    
    // Evict if needed
    evictLRUIfNeeded();
    
    // Create new entry
    auto entry = std::make_shared<AdapterEntry>();
    entry->adapter_id = adapter_id;
    entry->adapter_path = adapter_path.empty() ? "storage://" + adapter_id : adapter_path;
    entry->base_model = base_model;
    entry->scaling = scaling;
    entry->last_used = std::chrono::system_clock::now();
    
    // Try to load from storage if no path provided
    if (adapter_path.empty()) {
        if (!loadAdapterFromStorage(adapter_id, *entry)) {
            spdlog::error("Failed to load adapter {} from storage", adapter_id);
            return false;
        }
    } else {
        // Simulate loading adapter (in production, this would load actual weights)
        // For now, we just estimate memory usage
        entry->memory_bytes = 32 * 1024 * 1024; // 32 MB estimate for rank-8 LoRA
        entry->adapter_handle = reinterpret_cast<void*>(0x1); // Placeholder
        
        // Set metadata
        entry->metadata.adapter_id = adapter_id;
        entry->metadata.base_model = base_model;
        entry->metadata.created_at = std::chrono::system_clock::now();
        entry->metadata.updated_at = entry->metadata.created_at;
    }
    
    adapters_[adapter_id] = entry;
    cache_stats_.current_size = adapters_.size();
    
    spdlog::info("Loaded LoRA adapter: {} (memory: {} MB)", 
                 adapter_id, entry->memory_bytes / (1024 * 1024));
    
    return true;
}

bool LoRAAdapterManager::unloadAdapter(const std::string& adapter_id, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        spdlog::warn("Adapter {} not found", adapter_id);
        return false;
    }
    
    // Check if pinned
    if (it->second->is_pinned && !force) {
        spdlog::warn("Cannot unload pinned adapter {} (use force=true)", adapter_id);
        return false;
    }
    
    // Free adapter handle
    if (it->second->adapter_handle) {
        // In production: llama_lora_adapter_free(it->second->adapter_handle);
        it->second->adapter_handle = nullptr;
    }
    
    spdlog::info("Unloaded LoRA adapter: {}", adapter_id);
    adapters_.erase(it);
    cache_stats_.current_size = adapters_.size();
    
    return true;
}

bool LoRAAdapterManager::switchAdapter(const std::string& from_id, const std::string& to_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if target exists
    if (adapters_.find(to_id) == adapters_.end()) {
        spdlog::error("Target adapter {} not loaded", to_id);
        return false;
    }
    
    touchAdapter(to_id);
    
    spdlog::info("Switched from adapter {} to {}", from_id, to_id);
    return true;
}

std::vector<std::string> LoRAAdapterManager::listAdapters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(adapters_.size());
    
    for (const auto& [id, _] : adapters_) {
        ids.push_back(id);
    }
    
    return ids;
}

std::optional<AdapterInfo> LoRAAdapterManager::getAdapterInfo(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        return std::nullopt;
    }
    
    AdapterInfo info;
    info.adapter_id = it->second->adapter_id;
    info.base_model = it->second->base_model;
    info.memory_bytes = it->second->memory_bytes;
    info.is_loaded = (it->second->adapter_handle != nullptr);
    info.is_pinned = it->second->is_pinned;
    info.metadata = it->second->metadata;
    
    return info;
}

bool LoRAAdapterManager::isLoaded(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adapters_.find(adapter_id) != adapters_.end();
}

bool LoRAAdapterManager::pinAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        spdlog::warn("Adapter {} not found", adapter_id);
        return false;
    }
    
    it->second->is_pinned = true;
    spdlog::info("Pinned adapter: {}", adapter_id);
    return true;
}

bool LoRAAdapterManager::unpinAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = adapters_.find(adapter_id);
    if (it == adapters_.end()) {
        spdlog::warn("Adapter {} not found", adapter_id);
        return false;
    }
    
    it->second->is_pinned = false;
    spdlog::info("Unpinned adapter: {}", adapter_id);
    return true;
}

void LoRAAdapterManager::enableAdapterCache(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_enabled_ = enable;
    spdlog::info("Adapter cache {}", enable ? "enabled" : "disabled");
}

CacheStats LoRAAdapterManager::getCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_stats_.max_size = config_.max_cache_size;
    return cache_stats_;
}

void LoRAAdapterManager::clearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> to_remove;
    for (const auto& [id, entry] : adapters_) {
        if (!entry->is_pinned) {
            to_remove.push_back(id);
        }
    }
    
    for (const auto& id : to_remove) {
        unloadAdapter(id, false);
    }
    
    spdlog::info("Cleared cache, removed {} unpinned adapters", to_remove.size());
}

size_t LoRAAdapterManager::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total = 0;
    for (const auto& [_, entry] : adapters_) {
        total += entry->memory_bytes;
    }
    
    return total;
}

void LoRAAdapterManager::evictLRUIfNeeded() {
    // Check cache size
    if (adapters_.size() < config_.max_cache_size) {
        return;
    }
    
    // Check memory usage
    size_t total_memory = 0;
    for (const auto& [_, entry] : adapters_) {
        total_memory += entry->memory_bytes;
    }
    
    size_t max_memory_bytes = config_.max_memory_mb * 1024 * 1024;
    if (total_memory < max_memory_bytes && adapters_.size() < config_.max_cache_size) {
        return;
    }
    
    // Find LRU unpinned adapter
    std::string lru_id;
    auto oldest_time = std::chrono::system_clock::now();
    
    for (const auto& [id, entry] : adapters_) {
        if (!entry->is_pinned && entry->last_used < oldest_time) {
            oldest_time = entry->last_used;
            lru_id = id;
        }
    }
    
    if (!lru_id.empty()) {
        spdlog::info("Evicting LRU adapter: {}", lru_id);
        unloadAdapter(lru_id, false);
        cache_stats_.evictions++;
    }
}

bool LoRAAdapterManager::loadAdapterFromStorage(const std::string& adapter_id, AdapterEntry& entry) {
    // This would load from LoRAStorageService in production
    // For now, simulate loading
    entry.memory_bytes = 32 * 1024 * 1024; // 32 MB estimate
    entry.adapter_handle = reinterpret_cast<void*>(0x1); // Placeholder
    
    entry.metadata.adapter_id = adapter_id;
    entry.metadata.created_at = std::chrono::system_clock::now();
    entry.metadata.updated_at = entry.metadata.created_at;
    
    return true;
}

void LoRAAdapterManager::touchAdapter(const std::string& adapter_id) {
    auto it = adapters_.find(adapter_id);
    if (it != adapters_.end()) {
        it->second->last_used = std::chrono::system_clock::now();
    }
}

} // namespace lora
} // namespace llm
} // namespace themis
