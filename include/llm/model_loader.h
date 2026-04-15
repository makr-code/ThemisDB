/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            model_loader.h                                     ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     325                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "utils/expected.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>
#include <future>
#include <thread>
#include <functional>
#include <atomic>

/**
 * @file model_loader.h
 * @brief Ollama-inspired lazy model loading for ThemisDB
 * 
 * This component implements lazy loading of LLM models similar to Ollama:
 * - Models are loaded on-demand when first requested
 * - Models can be unloaded automatically when not used (TTL-based eviction)
 * - Multiple models can be kept in memory simultaneously (with memory limits)
 * - Efficient model switching with minimal overhead
 * 
 * Key features from Ollama:
 * - Lazy loading: Models load only when needed
 * - Automatic unloading: Unused models are evicted based on LRU + TTL
 * - Multi-model support: Keep multiple models loaded
 * - Resource management: Respect VRAM/RAM limits
 */

namespace themis {
namespace llm {

/**
 * @brief Model loading phases for progress tracking
 */
enum class LoadPhase {
    PARSING,        // 0-20% - Parse GGUF file
    ALLOCATING,     // 20-70% - Allocate model weights
    INITIALIZING    // 70-100% - Initialize context
};

/**
 * @brief Progress information for async model loading
 */
struct LoadProgress {
    LoadPhase phase;
    double phase_progress;        // 0.0-1.0 within current phase
    double overall_percent;       // 0-100 overall progress
    std::string status_msg;
    std::chrono::steady_clock::time_point start_time;
    
    LoadProgress() 
        : phase(LoadPhase::PARSING), 
          phase_progress(0.0), 
          overall_percent(0.0),
          start_time(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Progress callback function type
 */
using ProgressCallback = std::function<void(const LoadProgress&)>;

/**
 * @brief Cancellation token for async operations
 */
class CancellationToken {
public:
    CancellationToken() : cancelled_(std::make_shared<std::atomic<bool>>(false)) {}
    
    void cancel() { cancelled_->store(true); }
    bool is_cancelled() const { return cancelled_->load(); }
    
private:
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

/**
 * @brief Model cache entry with metadata
 */
struct CachedModel {
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

/**
 * @brief Lazy Model Loader (Ollama-inspired)
 * 
 * Manages multiple models with lazy loading and automatic eviction.
 * Models are loaded on first use and can be kept in memory for fast switching.
 * 
 * Example workflow:
 * 1. Request model "mistral-7b" for inference
 * 2. If not loaded, load it lazily (first request is slower)
 * 3. Subsequent requests use the cached model (fast)
 * 4. After TTL expires without use, model is unloaded to free memory
 * 5. Next request loads it again (lazy)
 */
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
    };
    
    explicit LazyModelLoader(const Config& config);
    ~LazyModelLoader();
    
    /**
     * @brief Get or load a model (lazy loading)
     * 
     * If model is already loaded, returns immediately.
     * If not loaded, loads it on-demand (blocking).
     * 
     * Thread-safe.
     * 
     * @param model_id Unique model identifier
     * @param model_path Path to model file (if not loaded)
     * @param load_config Optional loading configuration
     * @return Model handle or nullptr on failure
     */
    CachedModel* getOrLoadModel(
        const std::string& model_id,
        const std::string& model_path,
        const json& load_config = {}
    );
    
    /**
     * @brief Preload a model (background loading)
     * 
     * Loads a model asynchronously in the background.
     * Useful for warming up the cache before actual requests.
     * 
     * @return true if preload started successfully
     */
    bool preloadModel(
        const std::string& model_id,
        const std::string& model_path,
        const json& load_config = {}
    );
    
    /**
     * @brief Load model asynchronously with progress callback
     * 
     * Non-blocking model load with progress reporting and cancellation support.
     * This prevents query threads from blocking during model initialization.
     * 
     * @param model_id Unique model identifier
     * @param model_path Path to model file
     * @param progress_cb Optional callback for progress updates
     * @param cancel_token Optional cancellation token
     * @param load_config Optional loading configuration
     * @return Future that resolves to model handle or nullptr on failure
     */
    std::future<CachedModel*> loadAsync(
        const std::string& model_id,
        const std::string& model_path,
        ProgressCallback progress_cb = nullptr,
        CancellationToken cancel_token = CancellationToken(),
        const json& load_config = {}
    );
    
    /**
     * @brief Unload a specific model
     * 
     * Immediately unloads a model from memory.
     * 
     * @param model_id Model to unload
     * @param force If true, unload even if pinned
     */
    bool unloadModel(const std::string& model_id, bool force = false);
    
    /**
     * @brief Pin a model in memory (prevent eviction)
     * 
     * Useful for frequently used models that should always be available.
     */
    void pinModel(const std::string& model_id);
    
    /**
     * @brief Unpin a model (allow eviction)
     */
    void unpinModel(const std::string& model_id);
    
    /**
     * @brief Check if model is loaded
     */
    bool isModelLoaded(const std::string& model_id) const;
    
    /**
     * @brief Get model info (if loaded)
     */
    std::optional<ModelInfo> getModelInfo(const std::string& model_id) const;
    
    /**
     * @brief List all loaded models
     */
    std::vector<std::string> listLoadedModels() const;
    
    /**
     * @brief Evict least recently used model(s) to free memory
     * 
     * Called automatically when memory limits are exceeded.
     * Can also be called manually.
     * 
     * @param target_vram_mb Target VRAM to free
     * @return Amount of VRAM freed (MB)
     */
    size_t evictLRU(size_t target_vram_mb = 0);
    
    /**
     * @brief Evict models that exceeded their TTL
     * 
     * @return Number of models evicted
     */
    size_t evictExpired();
    
    /**
     * @brief Get memory usage statistics
     */
    json getMemoryStats() const;
    
    /**
     * @brief Get cache statistics
     */
    json getCacheStats() const;

    // Compact typed statistics API for tests
    struct Stats {
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t evictions = 0;
        size_t models_loaded = 0;
    };

    Stats getStatistics() const;
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::unique_ptr<CachedModel>> models_;
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
    Result<CachedModel*> loadModelInternal(
        const std::string& model_id,
        const std::string& model_path,
        const json& config
    );
    
    bool hasCapacity(size_t vram_mb, size_t ram_mb) const;
    void updateMemoryUsage();
};

} // namespace llm
} // namespace themis
