/**
 * @file lora_adapter_manager_compat.cpp
 * @brief DEPRECATED: Compatibility wrapper for LoRAAdapterManager
 * 
 * This file implements LoRAAdapterManager by delegating to MultiLoRAManager.
 * Used ONLY for backwards compatibility during migration.
 * 
 * [[deprecated]] - Use MultiLoRAManager directly instead!
 * 
 * @author ThemisDB Team - Migration 2026
 */

#include "llm/lora_framework/lora_adapter_manager.h"
#include "llm/multi_lora_manager.h"
#include <spdlog/spdlog.h>
#include <memory>

namespace themis {
namespace llm {
namespace lora {

// Hidden static implementation - delegates to MultiLoRAManager
class LoRAAdapterManager::Impl {
public:
    std::unique_ptr<llm::MultiLoRAManager> multi_manager_;
    
    Impl(const Config& config) {
        llm::MultiLoRAManager::Config multi_config;
        multi_config.max_lora_slots = config.max_cache_size;
        multi_config.max_lora_vram_mb = config.max_memory_mb;
        multi_config.lora_ttl = config.cache_ttl;
        multi_config.enable_adapter_fusion = true;
        multi_config.multi_gpu.enabled = false;
        multi_config.quantization.enabled = false;
        
        multi_manager_ = std::make_unique<llm::MultiLoRAManager>(multi_config);
    }
};

// ════════════════════════════════════════════════════════════════════════════
// LoRAAdapterManager Constructor/Destructor
// ════════════════════════════════════════════════════════════════════════════

LoRAAdapterManager::LoRAAdapterManager(const Config& config)
    : config_(config), cache_enabled_(true), impl_(std::make_unique<Impl>(config)) {
    spdlog::warn("LoRAAdapterManager is DEPRECATED! Use MultiLoRAManager directly.");
    spdlog::info("LoRAAdapterManager initialized (compat wrapper):");
    spdlog::info("  Max cache size: {}", config_.max_cache_size);
    spdlog::info("  Cache TTL: {} seconds", config_.cache_ttl.count());
}

LoRAAdapterManager::~LoRAAdapterManager() {
    impl_.reset();
    spdlog::info("LoRAAdapterManager destroyed");
}

// ════════════════════════════════════════════════════════════════════════════
// Load/Unload Operations
// ════════════════════════════════════════════════════════════════════════════

bool LoRAAdapterManager::loadAdapter(
    const std::string& adapter_id,
    const std::string& adapter_path,
    const std::string& base_model,
    float scaling) {
    
    if (!impl_ || !impl_->multi_manager_) {
        spdlog::error("MultiLoRAManager not initialized");
        return false;
    }
    
    // Use MultiLoRAManager's loadLoRA (handles both storage and file paths)
    bool loaded = impl_->multi_manager_->loadLoRA(
        adapter_id,
        adapter_path,
        base_model,
        false,  // quantize
        GPUPlacement::SINGLE_GPU,
        scaling
    );
    
    if (loaded) {
        cache_stats_.cache_misses++;
    }
    
    return loaded;
}

bool LoRAAdapterManager::unloadAdapter(const std::string& adapter_id, bool force) {
    if (!impl_ || !impl_->multi_manager_) {
        spdlog::error("MultiLoRAManager not initialized");
        return false;
    }
    
    return impl_->multi_manager_->unloadLoRA(adapter_id, force);
}

// ════════════════════════════════════════════════════════════════════════════
// Query Operations
// ════════════════════════════════════════════════════════════════════════════

bool LoRAAdapterManager::isLoaded(const std::string& adapter_id) const {
    if (!impl_ || !impl_->multi_manager_) {
        return false;
    }
    
    return impl_->multi_manager_->isLoRALoaded(adapter_id);
}

std::optional<AdapterInfo> LoRAAdapterManager::getAdapterInfo(
    const std::string& adapter_id) const {
    
    if (!impl_ || !impl_->multi_manager_) {
        return std::nullopt;
    }
    
    auto multi_info = impl_->multi_manager_->getLoRAInfo(adapter_id);
    if (!multi_info) {
        return std::nullopt;
    }
    
    // Convert MultiLoRAManager::LoRAInfo to lora::AdapterInfo
    AdapterInfo info;
    info.adapter_id = adapter_id;
    info.is_loaded = true;
    info.memory_bytes = 0;  // Not tracked by MultiLoRAManager in simple mode
    
    return info;
}

std::vector<std::string> LoRAAdapterManager::listAdapters() const {
    if (!impl_ || !impl_->multi_manager_) {
        return {};
    }
    
    auto loras = impl_->multi_manager_->listLoRAs();
    std::vector<std::string> ids;
    for (const auto& lora : loras) {
        ids.push_back(lora.id);
    }
    return ids;
}

// ════════════════════════════════════════════════════════════════════════════
// Adapter Application (Weight Fusion)
// ════════════════════════════════════════════════════════════════════════════

bool LoRAAdapterManager::applyAdapter(
    const std::string& adapter_id,
    llama_context* context,
    float alpha) {
    
    if (!impl_ || !impl_->multi_manager_) {
        spdlog::error("MultiLoRAManager not initialized");
        return false;
    }
    
    // MultiLoRAManager::applyLoRA handles the weight fusion
    // (The old applyAdapter was completely MOCK - this fixes it!)
    return impl_->multi_manager_->applyLoRA(adapter_id, context);
}

// ════════════════════════════════════════════════════════════════════════════
// Hot-Swap Operations
// ════════════════════════════════════════════════════════════════════════════

bool LoRAAdapterManager::switchAdapter(
    const std::string& from_adapter_id,
    const std::string& to_adapter_id) {
    
    if (!impl_ || !impl_->multi_manager_) {
        spdlog::error("MultiLoRAManager not initialized");
        return false;
    }
    
    // Unload "from" adapter
    if (!unloadAdapter(from_adapter_id)) {
        spdlog::warn("Failed to unload adapter: {}", from_adapter_id);
        // Continue anyway - might still be in cache
    }
    
    // Check if "to" adapter exists
    if (!impl_->multi_manager_->isLoRALoaded(to_adapter_id)) {
        spdlog::error("Target adapter {} not loaded", to_adapter_id);
        return false;
    }
    
    spdlog::debug("Switched from {} to {}", from_adapter_id, to_adapter_id);
    return true;
}

bool LoRAAdapterManager::deactivateAdapter(llama_context* context) {
    if (!impl_ || !impl_->multi_manager_) {
        spdlog::error("MultiLoRAManager not initialized");
        return false;
    }
    
    // Remove all LoRAs from context
    auto loras = impl_->multi_manager_->listLoRAs();
    for (const auto& lora : loras) {
        impl_->multi_manager_->removeLoRA(lora.id, context);
    }
    
    return true;
}

bool LoRAAdapterManager::switchAdapterWithFusion(
    const std::string& from_adapter_id,
    const std::string& to_adapter_id,
    llama_context* context,
    float alpha) {
    
    if (!impl_ || !impl_->multi_manager_) {
        spdlog::error("MultiLoRAManager not initialized");
        return false;
    }
    
    // Remove old adapter if specified
    if (!from_adapter_id.empty() && context) {
        if (!impl_->multi_manager_->removeLoRA(from_adapter_id, context)) {
            spdlog::warn("Failed to remove adapter: {}", from_adapter_id);
        }
    }
    
    // Apply new adapter
    if (!impl_->multi_manager_->applyLoRA(to_adapter_id, context)) {
        spdlog::error("Failed to apply adapter: {}", to_adapter_id);
        return false;
    }
    
    spdlog::debug("Switched adapters with fusion: {} -> {}", from_adapter_id, to_adapter_id);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// Pinning Operations
// ════════════════════════════════════════════════════════════════════════════

bool LoRAAdapterManager::pinAdapter(const std::string& adapter_id) {
    if (!impl_ || !impl_->multi_manager_) {
        return false;
    }
    
    impl_->multi_manager_->pinLoRA(adapter_id);
    return true;
}

bool LoRAAdapterManager::unpinAdapter(const std::string& adapter_id) {
    if (!impl_ || !impl_->multi_manager_) {
        return false;
    }
    
    impl_->multi_manager_->unpinLoRA(adapter_id);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
// Cache Statistics
// ════════════════════════════════════════════════════════════════════════════

LoRAAdapterManager::CacheStats LoRAAdapterManager::getCacheStats() const {
    CacheStats stats = cache_stats_;
    
    if (impl_ && impl_->multi_manager_) {
        auto mem_stats = impl_->multi_manager_->getMemoryStats();
        stats.current_size = static_cast<size_t>(mem_stats["loras_loaded"]);
    }
    
    return stats;
}

void LoRAAdapterManager::enableAdapterCache(bool enable) {
    cache_enabled_ = enable;
    if (!enable) {
        clearCache();
    }
}

void LoRAAdapterManager::clearCache() {
    if (!impl_ || !impl_->multi_manager_) {
        return;
    }
    
    auto loras = impl_->multi_manager_->listLoRAs();
    for (const auto& lora : loras) {
        impl_->multi_manager_->unloadLoRA(lora.id, true);
    }
    
    cache_stats_.cache_hits = 0;
    cache_stats_.cache_misses = 0;
    cache_stats_.total_loads = 0;
}

size_t LoRAAdapterManager::getMemoryUsage() const {
    if (!impl_ || !impl_->multi_manager_) {
        return 0;
    }
    
    auto mem_stats = impl_->multi_manager_->getMemoryStats();
    // Convert MB to bytes
    return static_cast<size_t>(mem_stats["vram_used_mb"]) * 1024 * 1024;
}

// ════════════════════════════════════════════════════════════════════════════
// Private Stub Methods
// ════════════════════════════════════════════════════════════════════════════

bool LoRAAdapterManager::loadAdapterFromStorage(
    const std::string& adapter_id,
    AdapterEntry& entry) {
    // Stub - MultiLoRAManager handles storage internally
    spdlog::debug("loadAdapterFromStorage delegated to MultiLoRAManager");
    return true;
}

void LoRAAdapterManager::evictLRUIfNeeded() {
    // Stub - MultiLoRAManager handles eviction internally
}

void LoRAAdapterManager::touchAdapter(const std::string& adapter_id) {
    // Stub - MultiLoRAManager handles access tracking internally
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
