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
bool createLlamaCppPlugin(
    const std::string& name = "llamacpp",
    const std::string& model_path = "",
    const json& config = {}
);

} // namespace llm
} // namespace themis
