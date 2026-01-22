#pragma once

#include "lora_config.h"
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <optional>

// Forward declaration for llama.cpp types
struct llama_context;

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief DEPRECATED: Use MultiLoRAManager instead
 * 
 * This class is maintained for backward compatibility only.
 * All new code should use MultiLoRAManager which provides:
 * - Better type safety (llama_context* instead of void*)
 * - Multi-LoRA support (vLLM-style)
 * - Improved performance and caching
 * 
 * This class will be removed in v2.0.
 * 
 * @deprecated Use themis::llm::MultiLoRAManager instead
 */
class [[deprecated("Use MultiLoRAManager instead")]] LoRAAdapterManager {
public:
    /**
     * @brief Configuration for adapter manager
     */
    struct Config {
        size_t max_cache_size = 5;           // Maximum cached adapters
        std::chrono::seconds cache_ttl{3600}; // Cache time-to-live
        bool enable_auto_unload = true;       // Automatically unload unused adapters
        size_t max_memory_mb = 2048;          // Maximum memory for adapters (MB)
    };
    
    explicit LoRAAdapterManager(const Config& config = Config{});
    ~LoRAAdapterManager();
    
    // Disable copy
    LoRAAdapterManager(const LoRAAdapterManager&) = delete;
    LoRAAdapterManager& operator=(const LoRAAdapterManager&) = delete;
    
    /**
     * @brief Load a LoRA adapter
     * @param adapter_id Unique identifier for the adapter
     * @param adapter_path Path to adapter weights (optional if already in storage)
     * @param base_model Base model name
     * @param scaling Scaling factor for adapter
     * @return true if loaded successfully
     */
    bool loadAdapter(
        const std::string& adapter_id,
        const std::string& adapter_path = "",
        const std::string& base_model = "",
        float scaling = 1.0f
    );
    
    /**
     * @brief Unload a LoRA adapter
     * @param adapter_id Adapter identifier
     * @param force Force unload even if pinned
     * @return true if unloaded successfully
     */
    bool unloadAdapter(const std::string& adapter_id, bool force = false);
    
    /**
     * @brief Switch from one adapter to another
     * @param from_id Current adapter
     * @param to_id Target adapter
     * @return true if switched successfully
     */
    bool switchAdapter(const std::string& from_id, const std::string& to_id);
    
    /**
     * @brief Apply loaded adapter to model context for inference
     * 
     * Implements weight fusion: output = base_weight @ input + alpha * adapter_weight @ input
     * This is the CRITICAL MISSING FEATURE - adapters are loaded but not applied!
     * 
     * @param adapter_id Adapter to apply
     * @param context Model context (llama.cpp context handle)
     * @param alpha Adapter scaling factor (default: use adapter's configured scaling)
     * @return true if applied successfully
     */
    bool applyAdapter(
        const std::string& adapter_id,
        llama_context* context,
        float alpha = -1.0f  // -1.0 means use adapter's configured scaling
    );
    
    /**
     * @brief Deactivate currently applied adapter (restore base model weights)
     * 
     * @param context Model context to restore
     * @return true if deactivated successfully
     */
    bool deactivateAdapter(llama_context* context);
    
    /**
     * @brief Switch between adapters with weight fusion
     * 
     * Efficiently switches from one adapter to another by:
     * 1. Removing old adapter weights
     * 2. Applying new adapter weights
     * 
     * @param from_adapter_id Current adapter (if empty, just apply to_adapter_id)
     * @param to_adapter_id Target adapter
     * @param context Model context
     * @param alpha Scaling for new adapter (default: use adapter's configured scaling)
     * @return true if switched successfully
     */
    bool switchAdapterWithFusion(
        const std::string& from_adapter_id,
        const std::string& to_adapter_id,
        llama_context* context,
        float alpha = -1.0f
    );
    
    /**
     * @brief Get list of loaded adapters
     * @return Vector of adapter IDs
     */
    std::vector<std::string> listAdapters() const;
    
    /**
     * @brief Get information about a specific adapter
     * @param adapter_id Adapter identifier
     * @return Optional adapter info
     */
    std::optional<AdapterInfo> getAdapterInfo(const std::string& adapter_id) const;
    
    /**
     * @brief Check if adapter is loaded
     * @param adapter_id Adapter identifier
     * @return true if loaded
     */
    bool isLoaded(const std::string& adapter_id) const;
    
    /**
     * @brief Pin adapter to prevent auto-unload
     * @param adapter_id Adapter identifier
     * @return true if pinned successfully
     */
    bool pinAdapter(const std::string& adapter_id);
    
    /**
     * @brief Unpin adapter
     * @param adapter_id Adapter identifier
     * @return true if unpinned successfully
     */
    bool unpinAdapter(const std::string& adapter_id);
    
    /**
     * @brief Enable or disable adapter cache
     * @param enable Enable cache
     */
    void enableAdapterCache(bool enable);
    
    /**
     * @brief Get cache statistics
     * @return Cache statistics
     */
    CacheStats getCacheStats() const;
    
    /**
     * @brief Clear cache (unload unpinned adapters)
     */
    void clearCache();
    
    /**
     * @brief Get current memory usage in bytes
     * @return Memory usage
     */
    size_t getMemoryUsage() const;
    
private:
    struct AdapterEntry {
        std::string adapter_id;
        std::string adapter_path;
        std::string base_model;
        float scaling;
        void* adapter_handle = nullptr;      // Opaque handle to actual adapter
        size_t memory_bytes = 0;
        bool is_pinned = false;
        bool is_applied = false;             // Track if adapter is currently applied to model
        std::chrono::system_clock::time_point last_used;
        AdapterMetadata metadata;
    };
    
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<AdapterEntry>> adapters_;
    bool cache_enabled_ = true;
    std::string currently_applied_adapter_;  // Track which adapter is currently applied
    
    // Cache statistics
    mutable CacheStats cache_stats_;
    
    /**
     * @brief Evict least recently used adapter if needed
     */
    void evictLRUIfNeeded();
    
    /**
     * @brief Load adapter from storage
     */
    bool loadAdapterFromStorage(const std::string& adapter_id, AdapterEntry& entry);
    
    /**
     * @brief Update last used time for cache
     */
    void touchAdapter(const std::string& adapter_id);
};

} // namespace lora
} // namespace llm
} // namespace themis
