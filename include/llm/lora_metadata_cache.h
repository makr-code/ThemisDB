/**
 * @file lora_metadata_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/concurrent_cache.h"
#include <string>
#include <chrono>
#include <optional>

namespace themis {
namespace llm {

struct LoRAMetadata {
    /**
     * @brief Lo RAMetadata.
     * @return Return value.
     */
    virtual ~LoRAMetadata() = default;
    std::string lora_id;
    std::string path;
    std::string base_model_id;
    size_t size_bytes = 0;
    float scale = 1.0f;  // LoRA scaling factor
    std::chrono::system_clock::time_point loaded_timestamp;
    std::chrono::system_clock::time_point last_accessed;
    uint64_t access_count = 0;
    bool is_loaded = false;
    int slot_id = -1;  // Current slot (-1 if not loaded)
    
    // LoRA-specific metadata
    int rank = 0;               // LoRA rank (e.g., 8, 16, 32)
    float alpha = 0.0f;          // LoRA alpha parameter
    std::vector<std::string> target_modules;  // Which layers are adapted
};

class LoRAMetadataCache {
public:
    using CacheType = ConcurrentCache<std::string, LoRAMetadata>;
    
    LoRAMetadataCache() = default;
    ~LoRAMetadataCache() = default;
    
    // Disable copy, allow move
    LoRAMetadataCache(const LoRAMetadataCache&) = delete;
    LoRAMetadataCache& operator=(const LoRAMetadataCache&) = delete;
    LoRAMetadataCache(LoRAMetadataCache&&) noexcept = default;
    LoRAMetadataCache& operator=(LoRAMetadataCache&&) noexcept = default;
    
    /**
     * @brief Put.
     * @param[in] lora_id Identifier of the lora.
     * @param[in] metadata Input parameter.
     */
    void put(const std::string& lora_id, const LoRAMetadata& metadata);
    
    /**
     * @brief Get.
     * @param[in] lora_id Identifier of the lora.
     * @return Return value.
     */
    std::optional<LoRAMetadata> get(const std::string& lora_id) const;
    
    /**
     * @brief Touch.
     * @param[in] lora_id Identifier of the lora.
     */
    void touch(const std::string& lora_id);
    
    /**
     * @brief Mark Loaded.
     * @param[in] lora_id Identifier of the lora.
     * @param[in] slot_id Identifier of the slot.
     */
    void markLoaded(const std::string& lora_id, int slot_id);
    
    /**
     * @brief Mark Unloaded.
     * @param[in] lora_id Identifier of the lora.
     */
    void markUnloaded(const std::string& lora_id);
    
    /**
     * @brief Contains.
     * @param[in] lora_id Identifier of the lora.
     * @return True when the operation succeeds.
     */
    bool contains(const std::string& lora_id) const;
    
    /**
     * @brief Remove.
     * @param[in] lora_id Identifier of the lora.
     * @return True when the operation succeeds.
     */
    bool remove(const std::string& lora_id);
    
    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;
    
    /**
     * @brief Clear.
     */
    void clear();
    
    struct Stats {
        size_t total_entries = 0;
        size_t loaded_entries = 0;
        size_t total_size_bytes = 0;
        uint64_t total_accesses = 0;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;
    
    /**
     * @brief Cache.
     * @return Return value.
     * @details Implements cache without additional internal calls.
     */
    CacheType& cache() { return cache_; }
    const CacheType& cache() const { return cache_; }
    
private:
    // REUSE: ThemisDB's ConcurrentCache
    CacheType cache_;
};

} // namespace llm
} // namespace themis
