#pragma once

/**
 * @file model_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/llm_plugin_interface.h"
#include "utils/expected.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {

enum class LoadPhase {
    PARSING,        // 0-20% - Parse GGUF file
    ALLOCATING,     // 20-70% - Allocate model weights
    INITIALIZING    // 70-100% - Initialize context
};

struct LoadProgress {
    LoadPhase phase;
    double phase_progress = 0.0;        // 0.0-1.0 within current phase
    double overall_percent = 0.0;       // 0-100 overall progress
    std::string status_msg;
    std::chrono::steady_clock::time_point start_time;
    
    LoadProgress() 
        : phase(LoadPhase::PARSING), 
          phase_progress(0.0), 
          overall_percent(0.0),
          start_time(std::chrono::steady_clock::now()) {}
};

using ProgressCallback = std::function<void(const LoadProgress&)>;

class CancellationToken {
public:
    CancellationToken() : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}
    
    /**
     * @brief Cancel.
     * @details Calls: store().
     */
    void cancel() { cancelled_->store(true); }
    bool is_cancelled() const { return cancelled_->load(); }
    
private:
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

struct CachedModel {
    /**
     * @brief Cached Model.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    virtual ~CachedModel() noexcept;
    std::string model_id;
    std::string model_path;
    ModelInfo info;
    
    void* model_handle = nullptr;      // Opaque model handle
    void* context_handle = nullptr;    // Opaque context handle
    
    std::chrono::system_clock::time_point last_used;
    std::chrono::system_clock::time_point loaded_at;
    size_t use_count = 0;
    
    size_t vram_mb = 0;
    size_t ram_mb = 0;
    
    bool is_loading = false;            // Prevent concurrent loads
    bool keep_loaded = false;           // Pin in memory (don't evict)
};

class LazyModelLoader {
public:
    struct Config {
        // Memory limits
        size_t max_vram_mb = 24576;      // 24 GB total VRAM budget
        size_t max_ram_mb = 65536;       // 64 GB total RAM budget
        
        // Cache policy
        size_t max_models = 3;           // Max models in memory
        std::chrono::seconds model_ttl{3600};  // 1 hour TTL
        bool enable_lazy_load = true;
        
        // Loading behavior
        int default_n_gpu_layers = 32;
        int default_n_ctx = 4096;
        bool use_mmap = true;
        
        // GGUF Loader preference (security - embedded safetensor)
        bool prefer_custom_gguf_loader = true;  // Prefer custom GGUFLoader over native llama.cpp
        bool fallback_to_native = true;          // Fallback to llama_load_model_from_file() on error
        
        // Security: Model integrity verification
        bool require_model_integrity = false;   // Require SHA-256 checksum for model loading
    };
    
    /**
     * @brief Lazy Model Loader.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LazyModelLoader(const Config& config);
    ~LazyModelLoader() noexcept;
    
    CachedModel* getOrLoadModel(
        const std::string& model_id,
        const std::string& model_path,
        const json& load_config = {}
    );
    
    std::shared_ptr<CachedModel> getOrLoadModelShared(
        const std::string& model_id,
        const std::string& model_path,
        const json& load_config = {}
    );

    CachedModel* getModel(
        const std::string& model_id,
        const std::string& model_path,
        const json& load_config = {}
    ) {
        return getOrLoadModel(model_id, model_path, load_config);
    }
    
    bool preloadModel(
        const std::string& model_id,
        const std::string& model_path,
        const json& load_config = {}
    );
    
    std::future<CachedModel*> loadAsync(
        const std::string& model_id,
        const std::string& model_path,
        ProgressCallback progress_cb = nullptr,
        CancellationToken cancel_token = CancellationToken(),
        const json& load_config = {}
    );
    
    bool unloadModel(const std::string& model_id, bool force = false);
    
    /**
     * @brief Pin Model.
     * @param[in] model_id Identifier of the model.
     */
    void pinModel(const std::string& model_id);
    
    /**
     * @brief Unpin Model.
     * @param[in] model_id Identifier of the model.
     */
    void unpinModel(const std::string& model_id);
    
    /**
     * @brief Is Model Loaded.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool isModelLoaded(const std::string& model_id) const;
    
    /**
     * @brief Get Model Info.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<ModelInfo> getModelInfo(const std::string& model_id) const;
    
    /**
     * @brief List Loaded Models.
     * @return Return value.
     */
    std::vector<std::string> listLoadedModels() const;
    
    size_t evictLRU(size_t target_vram_mb = 0);
    
    /**
     * @brief Evict Expired.
     * @return Return value.
     */
    size_t evictExpired();
    
    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    json getMemoryStats() const;
    
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    json getCacheStats() const;

    // Compact typed statistics API for tests
    struct Stats {
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t evictions = 0;
        size_t models_loaded = 0;
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Stats getStatistics() const;
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::shared_ptr<CachedModel>> models_;
    mutable std::mutex mutex_;
    
    // Async loading tracking
    std::unordered_map<std::string, std::future<CachedModel*>> pending_loads_;
    
    // Statistics
    // Note: total_vram_mb_ and total_ram_mb_ are protected by mutex_ since they're 
    // updated together with models_ map modifications
    size_t total_vram_mb_ = 0;
    size_t total_ram_mb_ = 0;
    
    // Thread-safe counters using atomics (accessed outside critical sections)
    std::atomic<size_t> cache_hits_{0};
    std::atomic<size_t> cache_misses_{0};
    std::atomic<size_t> evictions_{0};
    std::atomic<size_t> models_loaded_{0};
    
    // Internal helpers
    /**
     * @brief Load Model Internal.
     * @param[in] model_id Identifier of the model.
     * @param[in] model_path Path to the model.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<CachedModel*> loadModelInternal(
        const std::string& model_id,
        const std::string& model_path,
        const json& config
    );
    [[nodiscard]] bool verifyModelChecksum(
        const std::string& model_id,
        const std::string& model_path,
        const json& config
    ) const;
    
    /**
     * @brief Has Capacity.
     * @param[in] vram_mb Input parameter.
     * @param[in] ram_mb Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCapacity(size_t vram_mb, size_t ram_mb) const;
    /**
     * @brief Update Memory Usage.
     */
    void updateMemoryUsage();
    /**
     * @brief Unload Model Unlocked.
     * @param[in] model_id Identifier of the model.
     * @param[in] force Input parameter.
     * @return True when the operation succeeds.
     */
    bool unloadModelUnlocked(const std::string& model_id, bool force);
    size_t evictLRUUnlocked(size_t target_vram_mb = 0);
};

} // namespace llm
} // namespace themis
