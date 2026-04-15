/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_plugin_manager.h                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
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
#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>

/**
 * @file llm_plugin_manager.h
 * @brief Manager for LLM plugins in ThemisDB
 * 
 * Coordinates multiple LLM backends and provides a unified interface
 * for the rest of ThemisDB to interact with LLM functionality.
 * 
 * Features:
 * - Multiple LLM backend support (llama.cpp, vLLM, etc.)
 * - Plugin discovery and loading
 * - Fallback and load balancing
 * - Integration with PluginManager
 */

namespace themis {
namespace llm {

/**
 * @brief LLM Plugin Manager
 * 
 * Central manager for all LLM plugins in ThemisDB.
 * Provides a simplified interface for LLM operations while
 * managing multiple backend plugins underneath.
 */
class LLMPluginManager {
public:
    LLMPluginManager() = default;
    ~LLMPluginManager() = default;
    
    // Prevent copying
    LLMPluginManager(const LLMPluginManager&) = delete;
    LLMPluginManager& operator=(const LLMPluginManager&) = delete;
    
    /**
     * @brief Register an LLM plugin
     * @param name Plugin identifier
     * @param plugin Plugin instance
     */
    void registerPlugin(
        const std::string& name,
        std::unique_ptr<ILLMPlugin> plugin
    );
    
    /**
     * @brief Unregister a plugin
     * @param name Plugin identifier
     */
    void unregisterPlugin(const std::string& name);
    
    /**
     * @brief Get a specific plugin
     * @param name Plugin identifier
     * @return Plugin instance or nullptr if not found
     */
    ILLMPlugin* getPlugin(const std::string& name) const;
    
    /**
     * @brief Get the default/primary plugin
     * @return Primary plugin or nullptr if none registered
     */
    ILLMPlugin* getDefaultPlugin() const;
    
    /**
     * @brief Set the default plugin
     * @param name Plugin identifier
     */
    void setDefaultPlugin(const std::string& name);
    
    /**
     * @brief List all registered plugins
     * @return Vector of plugin names
     */
    std::vector<std::string> listPlugins() const;
    
    /**
     * @brief Check if a plugin is registered
     * @param name Plugin identifier
     */
    bool hasPlugin(const std::string& name) const;
    
    /**
     * @brief Get aggregated capabilities from all plugins
     */
    json getAggregatedCapabilities() const;
    
    /**
     * @brief Get aggregated statistics from all plugins
     */
    json getAggregatedStats() const;
    
    /**
     * @brief Singleton instance
     */
    static LLMPluginManager& instance();
    
    // ═══════════════════════════════════════════════════════════
    // Convenience methods (delegate to default plugin)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate text using default plugin
     */
    InferenceResponse generate(const InferenceRequest& request);
    
    /**
     * @brief RAG generation using default plugin
     */
    InferenceResponse generateRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    );
    
    /**
     * @brief Embed text using default plugin
     */
    std::vector<float> embed(const std::string& text);

    // Convenience wrappers for model management
    bool loadModel(const std::string& model_id, const std::string& path);
    void unloadModel(const std::string& model_id);
    std::vector<std::string> listModels() const;

    // Convenience wrappers for LoRA management
    bool loadLoRA(const std::string& lora_id, const std::string& path, const std::string& base_model);
    bool unloadLoRA(const std::string& lora_id);
    std::vector<LoRAInfo> listLoRAs() const;

    // Streaming and ingestion helpers
    std::vector<std::string> generateStream(const InferenceRequest& request);
    bool ingestModel(const std::string& model_id, const std::string& data);
    std::optional<ModelInfo> getModelInfo(const std::string& model_id) const;

    struct PluginStatistics {
        int models_loaded = 0;
        int loras_loaded = 0;
        uint64_t total_requests = 0;
        double throughput = 0.0;
        double average_latency_ms = 0.0;
        double cache_hit_rate = 0.0;
        int active_workers = 0;
        int queue_depth = 0;
    };

    struct CacheStatistics {
        size_t response_cache_hits = 0;
        size_t response_cache_misses = 0;
        size_t response_cache_entries = 0;
        double response_cache_hit_rate = 0.0;
        size_t prefix_cache_hits = 0;
        size_t prefix_cache_misses = 0;
        size_t prefix_cache_entries = 0;
        double prefix_cache_hit_rate = 0.0;
    };

    struct HealthStatus {
        bool is_healthy = true;
        std::string plugin_manager_status = "ok";
        std::string async_engine_status = "ok";
        int models_loaded = 0;
        int loras_loaded = 0;
    };

    PluginStatistics getStatistics() const;
    CacheStatistics getCacheStatistics() const;
    HealthStatus getHealthStatus() const;
    void clearAllCaches();
    
private:
    struct PluginEntry {
        std::string name;
        std::unique_ptr<ILLMPlugin> plugin;
    };
    
    std::unordered_map<std::string, PluginEntry> plugins_;
    std::string default_plugin_name_;
    mutable std::mutex mutex_;
    
    ILLMPlugin* getDefaultPluginLocked() const;
};

/**
 * @brief Helper function to create and register a llama.cpp plugin
 * 
 * This is a convenience function for the most common use case.
 * 
 * @param name Plugin name (default: "llamacpp")
 * @param model_path Path to GGUF model file
 * @param config Plugin configuration
 * @return true if created and registered successfully
 */
bool createLlamaWrapper(
    const std::string& name = "llamacpp",
    const std::string& model_path = "",
    const json& config = {}
);

} // namespace llm
} // namespace themis
