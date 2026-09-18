/**
 * @file model_metadata_cache.h
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

struct ModelMetadata {
    /**
     * @brief Model Metadata.
     * @return Return value.
     */
    virtual ~ModelMetadata() = default;
    std::string model_id;
    std::string path;
    size_t size_bytes = 0;
    int n_layers = 0;
    int n_ctx = 0;
    std::chrono::system_clock::time_point loaded_timestamp;
    std::chrono::system_clock::time_point last_accessed;
    uint64_t access_count = 0;
    bool is_pinned = false;
    
    // GGUF-specific metadata
    std::string architecture;  // e.g., "llama", "mistral"
    std::string quantization;  // e.g., "Q4_K_M", "Q8_0"
};

class ModelMetadataCache {
public:
    using CacheType = ConcurrentCache<std::string, ModelMetadata>;
    
    ModelMetadataCache() = default;
    ~ModelMetadataCache() = default;
    
    // Disable copy, allow move
    ModelMetadataCache(const ModelMetadataCache&) = delete;
    ModelMetadataCache& operator=(const ModelMetadataCache&) = delete;
    ModelMetadataCache(ModelMetadataCache&&) noexcept = default;
    ModelMetadataCache& operator=(ModelMetadataCache&&) noexcept = default;
    
    /**
     * @brief Put.
     * @param[in] model_id Identifier of the model.
     * @param[in] metadata Input parameter.
     */
    void put(const std::string& model_id, const ModelMetadata& metadata);
    
    /**
     * @brief Get.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<ModelMetadata> get(const std::string& model_id) const;
    
    /**
     * @brief Touch.
     * @param[in] model_id Identifier of the model.
     */
    void touch(const std::string& model_id);
    
    /**
     * @brief Contains.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool contains(const std::string& model_id) const;
    
    /**
     * @brief Remove.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool remove(const std::string& model_id);
    
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
        size_t pinned_entries = 0;
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
