#pragma once

#include "llm/llm_plugin_interface.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

/**
 * @file multi_lora_manager.h
 * @brief vLLM-inspired multi-LoRA management for ThemisDB
 * 
 * This component implements efficient multi-LoRA adapter management similar to vLLM:
 * - Multiple LoRA adapters can be loaded simultaneously
 * - Efficient adapter switching during inference (minimal overhead)
 * - Batched inference with different LoRAs for different requests
 * - Memory-efficient adapter storage and composition
 * 
 * Key features from vLLM:
 * - Multi-LoRA inference: Different requests can use different adapters
 * - Adapter batching: Multiple LoRAs in a single batch
 * - Dynamic loading/unloading: Adapters load/unload on demand
 * - Memory pooling: Efficient VRAM usage for multiple adapters
 */

namespace themis {
namespace llm {

/**
 * @brief LoRA adapter slot
 * 
 * Represents a loaded LoRA adapter with its metadata and handle.
 */
struct LoRASlot {
    std::string lora_id;
    std::string path;
    std::string base_model_id;
    
    void* adapter_handle = nullptr;     // Opaque LoRA handle
    float scale = 1.0f;
    
    size_t vram_bytes = 0;
    size_t rank = 0;                    // LoRA rank (r)
    size_t alpha = 0;                   // LoRA alpha
    
    std::chrono::system_clock::time_point loaded_at;
    std::chrono::system_clock::time_point last_used;
    size_t use_count = 0;
    
    bool is_active = false;             // Currently applied to model
    bool keep_loaded = false;           // Pin in memory
};

/**
 * @brief Multi-LoRA Manager (vLLM-inspired)
 * 
 * Manages multiple LoRA adapters for a single base model.
 * Supports efficient switching between adapters and even batched
 * inference with different adapters per request.
 * 
 * Example workflow:
 * 1. Load base model (mistral-7b)
 * 2. Load multiple LoRAs: legal-qa, medical-diagnosis, code-assistant
 * 3. Request 1: Use legal-qa adapter
 * 4. Request 2: Use medical-diagnosis adapter
 * 5. Both can be in the same inference batch (if backend supports it)
 */
class MultiLoRAManager {
public:
    struct Config {
        // Memory limits
        size_t max_lora_vram_mb = 2048;  // 2 GB for all LoRAs
        size_t max_lora_slots = 16;      // Max concurrent LoRAs
        
        // Cache policy
        std::chrono::seconds lora_ttl{1800};  // 30 min TTL
        bool enable_lazy_load = true;
        
        // Batching (vLLM-style)
        bool enable_multi_lora_batch = false;  // Multiple LoRAs in one batch
        size_t max_loras_per_batch = 4;
        
        // Adapter fusion
        bool enable_adapter_fusion = false;    // Merge multiple LoRAs
    };
    
    explicit MultiLoRAManager(const Config& config);
    ~MultiLoRAManager();
    
    /**
     * @brief Load a LoRA adapter (lazy loading)
     * 
     * If adapter is already loaded, returns immediately.
     * Otherwise, loads it on-demand.
     * 
     * Thread-safe.
     * 
     * @param lora_id Unique LoRA identifier
     * @param lora_path Path to LoRA weights file
     * @param base_model_id Compatible base model
     * @param scale LoRA scaling factor (default: 1.0)
     * @return true if loaded successfully
     */
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        float scale = 1.0f
    );
    
    /**
     * @brief Unload a LoRA adapter
     * 
     * @param lora_id LoRA to unload
     * @param force If true, unload even if pinned
     */
    bool unloadLoRA(const std::string& lora_id, bool force = false);
    
    /**
     * @brief Get LoRA slot (if loaded)
     * 
     * Returns pointer to loaded LoRA slot, or nullptr if not loaded.
     * Updates last_used timestamp.
     */
    LoRASlot* getLoRA(const std::string& lora_id);
    
    /**
     * @brief Apply LoRA to model context
     * 
     * Activates a specific LoRA adapter for the next inference.
     * Multiple LoRAs can be active simultaneously if backend supports it.
     * 
     * @param lora_id LoRA to activate
     * @param context_handle Model context to apply to
     * @return true if applied successfully
     */
    bool applyLoRA(const std::string& lora_id, void* context_handle);
    
    /**
     * @brief Remove LoRA from model context
     * 
     * Deactivates a LoRA adapter.
     */
    bool removeLoRA(const std::string& lora_id, void* context_handle);
    
    /**
     * @brief Batch inference with multiple LoRAs (vLLM-style)
     * 
     * Processes multiple inference requests, each with its own LoRA adapter,
     * in a single batch for efficiency.
     * 
     * Requires backend support for multi-LoRA batching.
     * 
     * @param requests Vector of inference requests with LoRA IDs
     * @return Vector of responses (same order as requests)
     */
    std::vector<InferenceResponse> batchInferenceMultiLoRA(
        const std::vector<std::pair<InferenceRequest, std::string>>& requests,
        void* model_context
    );
    
    /**
     * @brief Fuse multiple LoRAs into a single adapter
     * 
     * Combines multiple LoRA adapters into one for efficiency.
     * Useful when always using the same combination of adapters.
     * 
     * @param lora_ids LoRAs to fuse
     * @param fused_id New identifier for fused adapter
     * @param weights Weights for each LoRA (default: equal)
     * @return true if fusion successful
     */
    bool fuseLoRAs(
        const std::vector<std::string>& lora_ids,
        const std::string& fused_id,
        const std::vector<float>& weights = {}
    );
    
    /**
     * @brief Pin a LoRA in memory (prevent eviction)
     */
    void pinLoRA(const std::string& lora_id);
    
    /**
     * @brief Unpin a LoRA (allow eviction)
     */
    void unpinLoRA(const std::string& lora_id);
    
    /**
     * @brief Check if LoRA is loaded
     */
    bool isLoRALoaded(const std::string& lora_id) const;
    
    /**
     * @brief List all loaded LoRAs
     */
    std::vector<LoRAInfo> listLoRAs() const;

    /**
     * @brief List loaded LoRAs filtered by base model id
     */
    std::vector<LoRAInfo> listLoRAs(const std::string& base_model_id) const;

    /**
     * @brief Get LoRA info by id
     */
    std::optional<LoRAInfo> getLoRAInfo(const std::string& lora_id) const;
    
    /**
     * @brief Evict least recently used LoRA(s)
     * 
     * @param target_vram_mb Target VRAM to free
     * @return Amount of VRAM freed (MB)
     */
    size_t evictLRU(size_t target_vram_mb = 0);
    
    /**
     * @brief Evict LoRAs that exceeded their TTL
     * 
     * @return Number of LoRAs evicted
     */
    size_t evictExpired();
    
    /**
     * @brief Get memory usage statistics
     */
    json getMemoryStats() const;
    
    /**
     * @brief Get LoRA cache statistics
     */
    json getCacheStats() const;

    // Compact typed statistics API for tests
    struct Stats {
        size_t total_loras_loaded = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t evictions = 0;
        size_t switches = 0;
    };

    Stats getStatistics() const;

    // Backward-compat: legacy tests expect getStats()
    Stats getStats() const { return getStatistics(); }
    
    /**
     * @brief Export LoRA for cross-shard transfer
     * 
     * Serializes a LoRA adapter for transfer to another shard.
     */
    std::vector<uint8_t> exportLoRA(const std::string& lora_id);
    
    /**
     * @brief Import LoRA from another shard
     * 
     * Deserializes and loads a LoRA adapter received from another shard.
     */
    bool importLoRA(
        const std::string& lora_id,
        const std::vector<uint8_t>& data,
        const std::string& base_model_id
    );
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::unique_ptr<LoRASlot>> loras_;
    mutable std::mutex mutex_;
    
    // Statistics
    size_t total_vram_bytes_ = 0;
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;
    size_t evictions_ = 0;
    size_t switches_ = 0;                // LoRA switch count
    
    // Internal helpers
    LoRASlot* loadLoRAInternal(
        const std::string& lora_id,
        const std::string& lora_path,
        const std::string& base_model_id,
        float scale
    );
    
    bool hasCapacity(size_t vram_bytes) const;
    void updateMemoryUsage();
};

} // namespace llm
} // namespace themis
