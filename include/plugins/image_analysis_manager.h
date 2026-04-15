/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            image_analysis_manager.h                           ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:08:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     400                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file image_analysis_manager.h
 * @brief Image Analysis Plugin Manager for ThemisDB
 * 
 * Manages loading, initialization, and lifecycle of image analysis plugins.
 * Supports multiple backends running in parallel with the LLM system.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "plugins/image_analysis_interface.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <functional>
#include <future>

namespace themis {
namespace plugins {
namespace image {

/**
 * @brief Plugin Handle
 * 
 * Internal structure for managing loaded plugins.
 */
struct PluginHandle {
    std::string name;
    std::string path;
    void* library_handle = nullptr;           // OS-specific handle
    std::unique_ptr<IImageAnalysisBackend> instance;
    PluginInfo info;
    bool loaded = false;
    std::string file_hash;                    // SHA-256 for security
    int64_t load_time_ms = 0;
};

/**
 * @brief Image Analysis Plugin Manager
 * 
 * Singleton class that manages all image analysis plugins.
 * Thread-safe for concurrent access.
 * 
 * Features:
 * - Dynamic plugin loading from directory
 * - Multiple backend support (ONNX, llama.cpp, OpenCV, etc.)
 * - Automatic backend selection based on capabilities
 * - Plugin hot-reload support
 * - Security verification (file hash, signature)
 * - Statistics and monitoring
 */
class ImageAnalysisManager {
public:
    /**
     * @brief Get singleton instance
     */
    static ImageAnalysisManager& instance();
    
    /**
     * @brief Scan directory for image analysis plugins
     * 
     * Looks for DLLs/SOs with pattern: themis_image_*.dll/so
     * 
     * @param directory Path to plugin directory
     * @return Number of plugins discovered
     */
    size_t scanPluginDirectory(const std::string& directory);
    
    /**
     * @brief Load a plugin by name
     * 
     * @param name Plugin name (without extension)
     * @param config Configuration for plugin
     * @param backend Preferred backend (CPU, CUDA, etc.)
     * @return true if loaded successfully
     */
    bool loadPlugin(
        const std::string& name,
        const PluginConfig& config = PluginConfig(),
        BackendType backend = BackendType::AUTO
    );
    
    /**
     * @brief Load plugin from explicit path
     * 
     * @param path Full path to plugin DLL/SO
     * @param config Configuration for plugin
     * @param backend Preferred backend
     * @return true if loaded successfully
     */
    bool loadPluginFromPath(
        const std::string& path,
        const PluginConfig& config = PluginConfig(),
        BackendType backend = BackendType::AUTO
    );
    
    /**
     * @brief Unload a plugin
     * 
     * @param name Plugin name
     */
    void unloadPlugin(const std::string& name);
    
    /**
     * @brief Unload all plugins
     */
    void unloadAllPlugins();
    
    /**
     * @brief Get plugin by name
     * 
     * @param name Plugin name
     * @return Plugin instance or nullptr if not loaded
     */
    IImageAnalysisBackend* getPlugin(const std::string& name) const;
    
    /**
     * @brief Get best plugin for a capability
     * 
     * Selects the best available plugin based on:
     * - Capability support
     * - Backend type (GPU preferred)
     * - Performance characteristics
     * 
     * @param capability What capability is needed
     * @return Plugin instance or nullptr if none available
     */
    IImageAnalysisBackend* getBestPluginForCapability(const std::string& capability) const;
    
    /**
     * @brief Get all plugins supporting a capability
     * 
     * @param capability Capability name (embedding, captioning, etc.)
     * @return Vector of plugin instances
     */
    std::vector<IImageAnalysisBackend*> getPluginsWithCapability(const std::string& capability) const;
    
    /**
     * @brief List all loaded plugins
     * 
     * @return Vector of plugin names
     */
    std::vector<std::string> listLoadedPlugins() const;
    
    /**
     * @brief Get plugin information
     * 
     * @param name Plugin name
     * @return Plugin info or nullopt if not found
     */
    std::optional<PluginInfo> getPluginInfo(const std::string& name) const;
    
    /**
     * @brief Check if plugin is loaded
     * 
     * @param name Plugin name
     * @return true if loaded
     */
    bool isPluginLoaded(const std::string& name) const;
    
    /**
     * @brief Reload a plugin (hot-reload)
     * 
     * Useful for development or updating models without restart.
     * 
     * @param name Plugin name
     * @return true if successful
     */
    bool reloadPlugin(const std::string& name);
    
    /**
     * @brief Set default plugin for operations
     * 
     * Used when no explicit plugin is specified.
     * 
     * @param name Plugin name
     */
    void setDefaultPlugin(const std::string& name);
    
    /**
     * @brief Get default plugin
     */
    std::string getDefaultPlugin() const;
    
    // ========================================================================
    // High-Level Operations (Convenience API)
    // ========================================================================
    
    /**
     * @brief Generate embedding using best available plugin
     * 
     * @param image_data Raw image bytes
     * @param plugin_name Optional: specific plugin to use
     * @return Embedding result
     */
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const std::string& plugin_name = ""
    );
    
    /**
     * @brief Generate caption using best available plugin
     * 
     * @param image_data Raw image bytes
     * @param max_length Maximum caption length
     * @param plugin_name Optional: specific plugin to use
     * @return Caption result
     */
    CaptionResult generateCaption(
        const std::vector<uint8_t>& image_data,
        int max_length = 50,
        const std::string& plugin_name = ""
    );
    
    /**
     * @brief Detect objects using best available plugin
     * 
     * @param image_data Raw image bytes
     * @param confidence_threshold Minimum confidence
     * @param plugin_name Optional: specific plugin to use
     * @return Detection result
     */
    DetectionResult detectObjects(
        const std::vector<uint8_t>& image_data,
        float confidence_threshold = 0.5f,
        const std::string& plugin_name = ""
    );
    
    /**
     * @brief Generate image using best available plugin
     * 
     * @param params Generation parameters
     * @param plugin_name Optional: specific plugin to use
     * @return Generation result
     */
    GenerationResult generateImage(
        const GenerationParams& params,
        const std::string& plugin_name = ""
    );
    
    // ========================================================================
    // Parallel Execution with LLM
    // ========================================================================
    
    /**
     * @brief Execute image analysis in parallel with LLM operation
     * 
     * Runs image analysis on a separate thread while LLM processes text.
     * Useful for multimodal RAG where both image and text need processing.
     * 
     * @param image_data Raw image bytes
     * @param llm_task LLM task to run in parallel (returns when both complete)
     * @return Embedding result from image analysis
     */
    template<typename LLMTask>
    EmbeddingResult parallelExecuteWithLLM(
        const std::vector<uint8_t>& image_data,
        LLMTask llm_task
    ) {
        // Start image analysis on separate thread
        std::future<EmbeddingResult> image_future = std::async(
            std::launch::async,
            [this, &image_data]() { return generateEmbedding(image_data); }
        );
        
        // Execute LLM task on current thread
        llm_task();
        
        // Wait for image analysis to complete
        return image_future.get();
    }
    
    // ========================================================================
    // Statistics and Monitoring
    // ========================================================================
    
    /**
     * @brief Get aggregate statistics
     */
    struct Statistics {
        size_t total_plugins_loaded = 0;
        size_t total_inferences = 0;
        int64_t total_inference_time_ms = 0;
        double average_inference_time_ms = 0.0;
        
        std::unordered_map<std::string, size_t> inferences_per_plugin;
        std::unordered_map<std::string, int64_t> time_per_plugin_ms;
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief Health check all plugins
     * 
     * @return Map of plugin_name -> is_healthy
     */
    std::unordered_map<std::string, bool> healthCheckAll() const;
    
    /**
     * @brief Get memory usage estimate
     * 
     * @return Total estimated memory usage in MB
     */
    size_t getMemoryUsageMB() const;
    
private:
    ImageAnalysisManager() = default;
    ~ImageAnalysisManager();
    
    // Prevent copying
    ImageAnalysisManager(const ImageAnalysisManager&) = delete;
    ImageAnalysisManager& operator=(const ImageAnalysisManager&) = delete;
    
    // Plugin storage
    std::unordered_map<std::string, PluginHandle> plugins_;
    std::string default_plugin_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Statistics
    mutable Statistics stats_;
    
    // Platform-specific loading
    void* loadLibrary(const std::string& path);
    void* getSymbol(void* handle, const std::string& symbol_name);
    void unloadLibrary(void* handle);
    
    // Security
    bool verifyPlugin(const std::string& path, std::string& error_message);
    std::string calculateFileHash(const std::string& path);
    
    // Helper methods
    std::string getPluginNameFromPath(const std::string& path);
    bool isImagePlugin(const std::string& filename);
};

/**
 * @brief RAII Plugin Loader
 * 
 * Automatically loads plugin on construction and unloads on destruction.
 * Useful for scoped plugin usage.
 */
class ScopedPluginLoader {
public:
    ScopedPluginLoader(
        const std::string& name,
        const PluginConfig& config = PluginConfig(),
        BackendType backend = BackendType::AUTO
    ) : name_(name) {
        loaded_ = ImageAnalysisManager::instance().loadPlugin(name, config, backend);
    }
    
    ~ScopedPluginLoader() {
        if (loaded_) {
            ImageAnalysisManager::instance().unloadPlugin(name_);
        }
    }
    
    bool isLoaded() const { return loaded_; }
    IImageAnalysisBackend* getPlugin() const {
        return ImageAnalysisManager::instance().getPlugin(name_);
    }
    
private:
    std::string name_;
    bool loaded_ = false;
};

} // namespace image
} // namespace plugins
} // namespace themis
