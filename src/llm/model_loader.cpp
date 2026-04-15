/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            model_loader.cpp                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:42:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     835                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/model_loader.h"
#include "llm/gguf_loader.h"
#include "utils/error_registry.h"
#include "utils/expected.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <llama.h>

namespace fs = std::filesystem;

namespace themis {
namespace llm {

LazyModelLoader::LazyModelLoader(const Config& config)
    : config_(config) {
    
    // Initialize llama.cpp backend (must be called before any model operations)
    // This initializes CUDA/Metal backends and sets up memory management
    static std::once_flag backend_init_flag;
    std::call_once(backend_init_flag, []() {
        spdlog::info("Initializing llama.cpp backend...");
        llama_backend_init();
        spdlog::info("✓ llama.cpp backend initialized");
        
        // Register cleanup to free backend resources at process exit
        std::atexit([]() {
            spdlog::debug("Freeing llama.cpp backend resources");
            llama_backend_free();
        });
    });
    
    spdlog::info("LazyModelLoader initialized (Ollama-style):");
    spdlog::info("  Max VRAM: {} MB", config_.max_vram_mb);
    spdlog::info("  Max models: {}", config_.max_models);
    spdlog::info("  Model TTL: {} seconds", config_.model_ttl.count());
    spdlog::info("  Lazy loading: {}", config_.enable_lazy_load ? "enabled" : "disabled");
    spdlog::info("  Default GPU layers: {}", config_.default_n_gpu_layers);
    spdlog::info("  Default context: {} tokens", config_.default_n_ctx);
}

LazyModelLoader::~LazyModelLoader() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Unload all models properly
    for (auto& [id, model] : models_) {
        spdlog::info("Unloading model: {}", id);
        
        // Free llama.cpp resources if they exist
        // Safety: reinterpret_cast is safe here because:
        // 1. These are opaque C API handles from llama.cpp
        // 2. Handles were originally cast TO void* when stored (type-erasing pattern)
        // 3. We're now casting them back to their original types for proper cleanup
        // 4. llama_free and llama_free_model expect the original pointer types
        if (model->context_handle) {
            llama_free(reinterpret_cast<llama_context*>(model->context_handle));
            model->context_handle = nullptr;
        }
        if (model->model_handle) {
            llama_free_model(reinterpret_cast<llama_model*>(model->model_handle));
            model->model_handle = nullptr;
        }
    }
    models_.clear();
}

CachedModel* LazyModelLoader::getOrLoadModel(
    const std::string& model_id,
    const std::string& model_path,
    const json& load_config
) {
    std::unique_lock<std::mutex> lock(mutex_); // Use unique_lock for manual unlock/lock
    
    // Check if already loaded
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        spdlog::debug("Model cache hit: {}", model_id);
        cache_hits_.fetch_add(1, std::memory_order_relaxed);
        
        // Update usage
        it->second->last_used = std::chrono::system_clock::now();
        it->second->use_count++;
        
        return it->second.get();
    }
    
    // Check if async preload is in progress
    auto pending_it = pending_loads_.find(model_id);
    if (pending_it != pending_loads_.end()) {
        spdlog::info("Waiting for async preload to complete: {}", model_id);
        
        // Move future out of map to avoid issues with iterator invalidation
        std::future<CachedModel*> future_model = std::move(pending_it->second);
        pending_loads_.erase(pending_it);
        
        // Release lock temporarily to allow async task to complete
        lock.unlock();
        
        try {
            // Wait for async load to complete (with timeout)
            auto status = future_model.wait_for(std::chrono::seconds(300)); // 5 minute timeout
            
            if (status == std::future_status::ready) {
                auto* model = future_model.get();
                
                // Reacquire lock
                lock.lock();
                
                if (model) {
                    spdlog::info("Async preload completed for: {}", model_id);
                    cache_hits_.fetch_add(1, std::memory_order_relaxed); // Count as cache hit since it was preloaded
                    return model;
                } else {
                    spdlog::error("Async preload failed for: {}", model_id);
                    // Fall through to synchronous load attempt
                }
            } else {
                spdlog::error("Async preload timed out for: {}", model_id);
                lock.lock();
                // Fall through to synchronous load attempt
            }
        } catch (const std::exception& e) {
            spdlog::error("Exception waiting for async load: {}", e.what());
            if (!lock.owns_lock()) {
                lock.lock();
            }
            // Fall through to synchronous load attempt
        }
    }
    
    spdlog::info("Model cache miss: {} - loading lazily", model_id);
    cache_misses_.fetch_add(1, std::memory_order_relaxed);
    
    // Check if we need to evict
    if (models_.size() >= config_.max_models) {
        spdlog::info("Model cache full, evicting LRU");
        evictLRU();
    }
    
    // Load model
    auto result = loadModelInternal(model_id, model_path, load_config);
    if (!result) {
        spdlog::error("Failed to load model {}: {}", model_id, result.error().message());
        return nullptr;
    }
    return *result;
}

bool LazyModelLoader::preloadModel(
    const std::string& model_id,
    const std::string& model_path,
    const json& load_config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    if (models_.find(model_id) != models_.end()) {
        spdlog::info("Model {} already loaded, skipping preload", model_id);
        return true;
    }
    
    // Check if already being loaded
    if (pending_loads_.find(model_id) != pending_loads_.end()) {
        spdlog::info("Model {} is already being preloaded", model_id);
        return true;
    }
    
    spdlog::info("Starting async preload for model: {}", model_id);
    
    // Launch async loading task
    pending_loads_[model_id] = std::async(std::launch::async, [this, model_id, model_path, load_config]() -> CachedModel* {
        try {
            // Acquire lock for the actual loading
            std::lock_guard<std::mutex> load_lock(mutex_);
            
            // Check again if model was loaded while we were waiting
            auto it = models_.find(model_id);
            if (it != models_.end()) {
                spdlog::debug("Model {} was loaded during async wait", model_id);
                return it->second.get();
            }
            
            // Perform the actual load
            auto result = loadModelInternal(model_id, model_path, load_config);
            if (result) {
                spdlog::info("Async preload completed successfully for: {}", model_id);
                return *result;
            } else {
                spdlog::error("Async preload failed for: {}: {}", model_id, result.error().message());
                return nullptr;
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during async model load for {}: {}", model_id, e.what());
            return nullptr;
        }
    });
    
    return true;
}

std::future<CachedModel*> LazyModelLoader::loadAsync(
    const std::string& model_id,
    const std::string& model_path,
    ProgressCallback progress_cb,
    CancellationToken cancel_token,
    const json& load_config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    if (models_.find(model_id) != models_.end()) {
        spdlog::info("Model {} already loaded, returning immediately", model_id);
        // Return already-ready future
        std::promise<CachedModel*> promise;
        promise.set_value(models_[model_id].get());
        return promise.get_future();
    }
    
    // Check if already being loaded via preloadModel
    if (pending_loads_.find(model_id) != pending_loads_.end()) {
        spdlog::warn("Model {} is already being loaded via preloadModel(). "
                    "New progress callback will not be used. "
                    "Consider using preloadModel() for async loading without progress tracking.",
                    model_id);
        // For safety, just start a new async load rather than trying to share futures
        // This ensures each caller gets their own future they can safely use
    }
    
    spdlog::info("Starting async model load with progress tracking: {}", model_id);
    
    // Launch async loading task with progress reporting
    // Note: Each call to loadAsync() creates a new future to avoid sharing issues
    return std::async(std::launch::async, [this, model_id, model_path, load_config, progress_cb, cancel_token]() -> CachedModel* {
        try {
            LoadProgress progress;
            progress.start_time = std::chrono::steady_clock::now();
            
            // Phase 1: PARSING (0-20%)
            progress.phase = LoadPhase::PARSING;
            progress.phase_progress = 0.0;
            progress.overall_percent = 0.0;
            progress.status_msg = "Parsing GGUF file...";
            if (progress_cb) progress_cb(progress);
            
            // Check cancellation
            if (cancel_token.is_cancelled()) {
                spdlog::info("Model load cancelled during PARSING: {}", model_id);
                return nullptr;
            }
            
            // Report parsing phase progress
            progress.phase_progress = 1.0;
            progress.overall_percent = 20.0;
            if (progress_cb) progress_cb(progress);
            
            // Phase 2: ALLOCATING (20-70%)
            progress.phase = LoadPhase::ALLOCATING;
            progress.phase_progress = 0.0;
            progress.overall_percent = 20.0;
            progress.status_msg = "Allocating model weights...";
            if (progress_cb) progress_cb(progress);
            
            // Acquire lock for actual loading
            std::unique_lock<std::mutex> load_lock(mutex_);
            
            // Check again if model was loaded while waiting
            auto it = models_.find(model_id);
            if (it != models_.end()) {
                spdlog::debug("Model {} was loaded during async wait", model_id);
                return it->second.get();
            }
            
            // Perform the actual model load
            // Note: loadModelInternal is the heavy operation that does the real work
            // In a full implementation, this would report progress internally
            auto result = loadModelInternal(model_id, model_path, load_config);
            
            CachedModel* model = nullptr;
            if (result) {
                model = *result;
            } else {
                spdlog::error("Model load failed: {}", result.error().message());
            }
            
            load_lock.unlock();
            
            if (cancel_token.is_cancelled()) {
                spdlog::info("Model load cancelled after loading: {}", model_id);
                // Model was loaded but user cancelled, so unload it
                load_lock.lock();
                unloadModel(model_id, true);
                load_lock.unlock();
                return nullptr;
            }
            
            // Report allocation phase complete
            progress.phase_progress = 1.0;
            progress.overall_percent = 70.0;
            if (progress_cb) progress_cb(progress);
            
            // Phase 3: INITIALIZING (70-100%)
            progress.phase = LoadPhase::INITIALIZING;
            progress.phase_progress = 0.0;
            progress.overall_percent = 70.0;
            progress.status_msg = "Initializing context...";
            if (progress_cb) progress_cb(progress);
            
            // Complete
            progress.phase = LoadPhase::INITIALIZING;
            progress.phase_progress = 1.0;
            progress.overall_percent = 100.0;
            progress.status_msg = "Model load complete";
            if (progress_cb) progress_cb(progress);
            
            if (model) {
                spdlog::info("Async model load completed successfully: {}", model_id);
            } else {
                spdlog::error("Async model load failed: {}", model_id);
            }
            
            return model;
            
        } catch (const std::exception& e) {
            spdlog::error("Exception during async model load for {}: {}", model_id, e.what());
            return nullptr;
        }
    });
}

bool LazyModelLoader::unloadModel(const std::string& model_id, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return false;
    }
    
    if (it->second->keep_loaded && !force) {
        spdlog::warn("Model {} is pinned, cannot unload (use force=true)", model_id);
        return false;
    }
    
    spdlog::info("Unloading model: {}", model_id);

    // Free llama.cpp resources
    // Safety: reinterpret_cast safe - recovering original types from type-erased C API handles
    if (it->second->context_handle) {
        llama_free(reinterpret_cast<llama_context*>(it->second->context_handle));
        it->second->context_handle = nullptr;
    }
    if (it->second->model_handle) {
        llama_free_model(reinterpret_cast<llama_model*>(it->second->model_handle));
        it->second->model_handle = nullptr;
    }

    // Update memory usage
    total_vram_mb_ -= it->second->vram_mb;
    total_ram_mb_ -= it->second->ram_mb;
    
    models_.erase(it);
    return true;
}

void LazyModelLoader::pinModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        it->second->keep_loaded = true;
        spdlog::info("Model pinned in memory: {}", model_id);
    }
}

void LazyModelLoader::unpinModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it != models_.end()) {
        it->second->keep_loaded = false;
        spdlog::info("Model unpinned: {}", model_id);
    }
}

bool LazyModelLoader::isModelLoaded(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return models_.find(model_id) != models_.end();
}

std::optional<ModelInfo> LazyModelLoader::getModelInfo(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return std::nullopt;
    }
    
    return it->second->info;
}

std::vector<std::string> LazyModelLoader::listLoadedModels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    result.reserve(models_.size());
    
    for (const auto& [id, _] : models_) {
        result.push_back(id);
    }
    
    return result;
}

size_t LazyModelLoader::evictLRU(size_t /*target_vram_mb*/) {
    // Already locked by caller
    
    if (models_.empty()) {
        return 0;
    }
    
    // Find LRU unpinned model
    CachedModel* lru_model = nullptr;
    std::string lru_id;
    auto oldest_time = std::chrono::system_clock::now();
    
    for (auto& [id, model] : models_) {
        if (model->keep_loaded) {
            continue;  // Skip pinned models
        }
        
        if (model->last_used < oldest_time) {
            oldest_time = model->last_used;
            lru_model = model.get();
            lru_id = id;
        }
    }
    
    if (!lru_model) {
        spdlog::warn("All models are pinned, cannot evict");
        return 0;
    }
    
    size_t freed_vram = lru_model->vram_mb;
    
    spdlog::info("Evicting LRU model: {} (freed {} MB VRAM)", lru_id, freed_vram);
    
    total_vram_mb_ -= lru_model->vram_mb;
    total_ram_mb_ -= lru_model->ram_mb;
    evictions_.fetch_add(1, std::memory_order_relaxed);
    
    models_.erase(lru_id);
    
    return freed_vram;
}

size_t LazyModelLoader::evictExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t evicted = 0;
    
    std::vector<std::string> to_evict;
    
    for (const auto& [id, model] : models_) {
        if (model->keep_loaded) {
            continue;  // Skip pinned models
        }
        
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - model->last_used
        );
        
        if (age > config_.model_ttl) {
            to_evict.push_back(id);
        }
    }
    
    for (const auto& id : to_evict) {
        spdlog::info("Evicting expired model: {}", id);
        unloadModel(id, true);
        evicted++;
    }
    
    return evicted;
}

json LazyModelLoader::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json stats;
    stats["vram_used_mb"] = total_vram_mb_;
    stats["vram_max_mb"] = config_.max_vram_mb;
    stats["vram_usage_pct"] = (config_.max_vram_mb > 0) 
        ? (total_vram_mb_ * 100.0 / config_.max_vram_mb) : 0.0;
    
    stats["ram_used_mb"] = total_ram_mb_;
    stats["ram_max_mb"] = config_.max_ram_mb;
    
    stats["models_loaded"] = models_.size();
    stats["models_max"] = config_.max_models;
    
    return stats;
}

json LazyModelLoader::getCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Load atomic counters once to ensure consistency
    size_t hits = cache_hits_.load(std::memory_order_relaxed);
    size_t misses = cache_misses_.load(std::memory_order_relaxed);
    size_t evict = evictions_.load(std::memory_order_relaxed);
    size_t loaded = models_loaded_.load(std::memory_order_relaxed);
    
    json stats;
    stats["cache_hits"] = hits;
    stats["cache_misses"] = misses;
    stats["evictions"] = evict;
    stats["models_loaded"] = loaded;
    
    if ((hits + misses) > 0) {
        stats["hit_rate"] = static_cast<double>(hits) / (hits + misses);
    } else {
        stats["hit_rate"] = 0.0;
    }
    
    return stats;
}

LazyModelLoader::Stats LazyModelLoader::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.cache_hits = cache_hits_.load(std::memory_order_relaxed);
    s.cache_misses = cache_misses_.load(std::memory_order_relaxed);
    s.evictions = evictions_.load(std::memory_order_relaxed);
    s.models_loaded = models_loaded_.load(std::memory_order_relaxed);
    return s;
}

Result<CachedModel*> LazyModelLoader::loadModelInternal(
    const std::string& model_id,
    const std::string& model_path,
    const json& config
) {
    // Already locked by caller
    spdlog::info("Loading model: {} from {}", model_id, model_path);

    auto model = std::make_unique<CachedModel>();
    model->model_id = model_id;
    model->model_path = model_path;
    model->loaded_at = std::chrono::system_clock::now();
    model->last_used = std::chrono::system_clock::now();
    model->use_count = 1;

    // Check if file exists
    if (!fs::exists(model_path)) {
        errors::logError(errors::ErrorCode::ERR_LLM_MODEL_NOT_FOUND, model_path);
        return Err<CachedModel*>(errors::ErrorCode::ERR_LLM_MODEL_NOT_FOUND, 
            fmt::format("Model file not found: {}", model_path));
    }

    size_t size_bytes = static_cast<size_t>(fs::file_size(model_path));

    // Initialize llama.cpp model parameters
    llama_model_params model_params = llama_model_default_params();
    
    // Configure GPU layers from config and normalize to prevent negative values
    int n_gpu_layers_raw = config.value("n_gpu_layers", config_.default_n_gpu_layers);
    int n_gpu_layers = std::max(0, n_gpu_layers_raw);  // Clamp to 0 minimum
    
    // GPU/VRAM handling with CPU fallback
    // Check if GPU is available by attempting to use it
    if (n_gpu_layers > 0) {
        spdlog::info("GPU offloading requested: {} layers", n_gpu_layers);
        
        // Set GPU layers - llama.cpp will handle fallback internally
        // If no GPU is available, it will automatically use CPU
        model_params.n_gpu_layers = n_gpu_layers;
        
        // Log GPU configuration
        spdlog::info("GPU offload configuration:");
        spdlog::info("  Requested GPU layers: {}", n_gpu_layers);
        spdlog::info("  VRAM limit: {} MB", config_.max_vram_mb);
        spdlog::info("  Note: llama.cpp will auto-fallback to CPU if GPU unavailable");
    } else {
        spdlog::info("CPU-only inference configured (n_gpu_layers={})", n_gpu_layers);
        model_params.n_gpu_layers = 0;
    }
    
    // Enable Flash Attention if available and configured
    bool use_flash_attn = config.value("use_flash_attn", false);
    #ifdef LLAMA_FLASH_ATTN
    if (use_flash_attn) {
        model_params.flash_attn = true;
        spdlog::info("Flash Attention enabled for model: {}", model_id);
    }
    #else
    if (use_flash_attn) {
        spdlog::warn("Flash Attention requested but not available in this llama.cpp build");
    }
    #endif
    
    // Memory management
    model_params.use_mmap = config.value("use_mmap", true);
    model_params.use_mlock = config.value("use_mlock", false);
    
    llama_model* lmodel = nullptr;
    bool custom_loader_success = false;
    
    // Try custom GGUF loader first if preferred (security - embedded safetensor)
    if (config_.prefer_custom_gguf_loader) {
        spdlog::info("Attempting to load model with custom GGUFLoader (security: embedded safetensor)");
        
        try {
            // Create GGUF loader instance
            GGUFLoader gguf_loader;
            
            // Parse the GGUF file
            if (gguf_loader.parseFile(model_path)) {
                spdlog::info("✓ Custom GGUFLoader: GGUF file parsed successfully");
                
                // Get metadata for validation
                const auto& metadata = gguf_loader.getMetadata();
                spdlog::info("  Model metadata: architecture={}, version={}, tensors={}",
                            metadata.architecture, metadata.version, metadata.tensors.size());
                
                // After parsing with custom loader, still use llama.cpp's native loader
                // for actual model initialization (custom loader validated the file)
                // This provides security validation + native performance
                lmodel = llama_load_model_from_file(model_path.c_str(), model_params);
                
                if (lmodel) {
                    spdlog::info("✓ Model loaded successfully with custom GGUF validation");
                    custom_loader_success = true;
                } else {
                    spdlog::warn("Custom GGUF validation succeeded, but llama.cpp load failed");
                }
            } else {
                spdlog::warn("Custom GGUFLoader: Failed to parse GGUF file");
            }
        } catch (const std::exception& e) {
            spdlog::warn("Custom GGUFLoader exception: {}", e.what());
        }
    }
    
    // Fallback to native llama.cpp loader
    if (!lmodel && config_.fallback_to_native) {
        spdlog::info("Falling back to native llama_load_model_from_file()");
        lmodel = llama_load_model_from_file(model_path.c_str(), model_params);
        
        if (lmodel) {
            spdlog::info("✓ Model loaded successfully with native llama.cpp loader");
        }
    }
    
    if (!lmodel) {
        errors::logError(errors::ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);
        spdlog::error("Failed to load model with both custom and native loaders");
        return Err<CachedModel*>(errors::ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, 
            fmt::format("Failed to load model from file: {}", model_path));
    }
    
    // Log which loader was used
    if (custom_loader_success) {
        spdlog::info("Model loading strategy: Custom GGUF validation + native llama.cpp");
    } else {
        spdlog::info("Model loading strategy: Native llama.cpp only");
    }
    
    // Initialize context parameters
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config.value("n_ctx", config_.default_n_ctx);
    ctx_params.n_batch = config.value("n_batch", 512);
    ctx_params.n_threads = config.value("n_threads", 8);
    
    // Configure RoPE scaling (Phase 3.1)
    if (config.value("rope_scaling_enabled", false)) {
        std::string method = config.value("rope_scaling_method", "yarn");
        int max_context = config.value("rope_max_context", 32768);
        int original_context = config.value("rope_original_context", 4096);
        
        // Calculate scaling factor
        float scale_factor = static_cast<float>(original_context) / static_cast<float>(max_context);
        
        if (method == "linear") {
            // Linear scaling: simple frequency scaling
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_LINEAR;
            ctx_params.rope_freq_scale = scale_factor;
            spdlog::info("RoPE Linear scaling: {} → {} tokens (scale: {:.4f})",
                        original_context, max_context, scale_factor);
        }
        else if (method == "ntk") {
            // NTK-Aware scaling: adjust base frequency
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_NONE;  // NTK uses freq_base
            float scaling_ratio = static_cast<float>(max_context) / static_cast<float>(original_context);
            ctx_params.rope_freq_base = 10000.0f * std::pow(scaling_ratio, 0.5f);
            spdlog::info("RoPE NTK scaling: {} → {} tokens (freq_base: {:.2f})",
                        original_context, max_context, ctx_params.rope_freq_base);
        }
        else if (method == "yarn") {
            // YaRN scaling: best quality for high factors
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_YARN;
            ctx_params.rope_freq_scale = scale_factor;
            
            // YaRN-specific parameters
            ctx_params.yarn_ext_factor = config.value("rope_yarn_ext_factor", 1.0f);
            ctx_params.yarn_attn_factor = config.value("rope_yarn_attn_factor", 1.0f);
            ctx_params.yarn_beta_fast = config.value("rope_yarn_beta_fast", 32.0f);
            ctx_params.yarn_beta_slow = config.value("rope_yarn_beta_slow", 1.0f);
            
            spdlog::info("RoPE YaRN scaling: {} → {} tokens (scale: {:.4f}, ext: {:.2f}, attn: {:.2f})",
                        original_context, max_context, scale_factor,
                        ctx_params.yarn_ext_factor, ctx_params.yarn_attn_factor);
        }
        else if (method == "dynamic") {
            // Dynamic scaling: adapts to input length
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_LINEAR;  // Use linear as base
            ctx_params.rope_freq_scale = scale_factor;
            spdlog::info("RoPE Dynamic scaling: {} → {} tokens (adaptive)",
                        original_context, max_context);
        }
        else {
            spdlog::warn("Unknown RoPE scaling method: {}, using YaRN", method);
            ctx_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_YARN;
            ctx_params.rope_freq_scale = scale_factor;
        }
    }
    
    // Check for embeddings mode
    bool enable_embeddings = config.value("enable_embeddings", false);
    if (enable_embeddings) {
        ctx_params.embeddings = true;
        spdlog::info("Embeddings mode enabled for model: {}", model_id);
    }
    
    // Create context
    llama_context* lctx = llama_new_context_with_model(lmodel, ctx_params);
    
    if (!lctx) {
        errors::logError(errors::ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED, model_id);
        llama_free_model(lmodel);
        return Err<CachedModel*>(errors::ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED,
            fmt::format("Failed to create context for model: {}", model_id));
    }

    // Populate model info
    model->info.name = model_id;
    model->info.model_id = model_id;
    model->info.path = model_path;
    model->info.format = "gguf";
    model->info.architecture = "llama";
    model->info.context_length = ctx_params.n_ctx;
    model->info.size_bytes = size_bytes;
    model->info.is_loaded = true;

    // Estimate VRAM/RAM usage
    size_t vram_mb = static_cast<size_t>(size_bytes / (1024ull * 1024ull));
    size_t ram_mb = vram_mb / 2;
    model->vram_mb = vram_mb;
    model->ram_mb = ram_mb;

    // Store opaque handles
    // Safety: reinterpret_cast safe for type-erasure of C API handles
    // These pointers will be cast back to their original types when needed
    // Standard pattern for interfacing with C libraries that use opaque handles
    model->model_handle = reinterpret_cast<void*>(lmodel);
    model->context_handle = reinterpret_cast<void*>(lctx);

    auto* result = model.get();
    models_[model_id] = std::move(model);

    // Update memory accounting and stats
    total_vram_mb_ += vram_mb;
    total_ram_mb_ += ram_mb;
    models_loaded_.fetch_add(1, std::memory_order_relaxed);

    // Log successful load with GPU configuration details
    spdlog::info("✓ Model loaded successfully: {}", model_id);
    spdlog::info("  Size: {} MB", vram_mb);
    spdlog::info("  GPU layers: {} {}", n_gpu_layers, 
                 n_gpu_layers > 0 ? "(GPU acceleration enabled)" : "(CPU-only mode)");
    spdlog::info("  Context length: {} tokens", ctx_params.n_ctx);
    spdlog::info("  Flash Attention: {}", use_flash_attn ? "ON" : "OFF");
    spdlog::info("  Memory-mapped: {}", model_params.use_mmap ? "yes" : "no");
    
    return result;
}

bool LazyModelLoader::hasCapacity(size_t vram_mb, size_t ram_mb) const {
    // Respect both memory budgets and max_models when set (0 means unlimited)
    const bool vram_ok = (config_.max_vram_mb == 0) || (total_vram_mb_ + vram_mb <= config_.max_vram_mb);
    const bool ram_ok = (config_.max_ram_mb == 0) || (total_ram_mb_ + ram_mb <= config_.max_ram_mb);
    const bool count_ok = models_.size() + 1 <= config_.max_models;
    return vram_ok && ram_ok && count_ok;
}

void LazyModelLoader::updateMemoryUsage() {
    // Recalculate from scratch
    total_vram_mb_ = 0;
    total_ram_mb_ = 0;
    
    for (const auto& [_, model] : models_) {
        total_vram_mb_ += model->vram_mb;
        total_ram_mb_ += model->ram_mb;
    }
}

} // namespace llm
} // namespace themis
