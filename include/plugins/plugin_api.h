/**
 * @file plugin_api.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugin_registry.h"
#include <memory>
#include <stdexcept>
#include <vector>

namespace themis {
namespace plugins {

/**
 * @brief Type-Safe Plugin API
 * 
 * Provides convenient template-based access to plugins.
 * Eliminates manual void* casts.
 * 
 * Usage:
 * ```cpp
 * // Single plugin
 * auto s3 = PluginAPI::get<IBlobStorageBackend>("s3_plugin");
 * if (s3) { s3->write(...); }
 * 
 * // All plugins of a type
 * auto all_importers = PluginAPI::getAll<IImporter>();
 * for (auto& importer : all_importers) {
 *     importer->import_data(...);
 * }
 * ```
 */
class PluginAPI {
public:
    /**
     * @brief Get single plugin by name
     * 
     * @tparam PluginInterface  Desired interface type
     * @param plugin_name       Plugin identifier
     * @return Loaded plugin instance or nullptr
     * 
     * @note Returns nullptr instead of throwing on error for graceful fallback
     */
    template<typename PluginInterface>
    static std::unique_ptr<PluginInterface> get(const std::string& plugin_name) {
        try {
            return PluginRegistry::create<PluginInterface>(plugin_name);
        } catch (const std::exception&) {
            // Log error, return nullptr instead of throwing
            // This allows graceful fallback
            return nullptr;
        }
    }

    /**
     * @brief Get all plugins implementing an interface
     * 
     * @tparam PluginInterface  Desired interface type
     * @return Vector of plugin instances
     */
    template<typename PluginInterface>
    static std::vector<std::unique_ptr<PluginInterface>> getAll() {
        std::vector<std::unique_ptr<PluginInterface>> result;
        auto names = PluginRegistry::listPlugins<PluginInterface>();
        
        for (const auto& name : names) {
            auto plugin = get<PluginInterface>(name);
            if (plugin) {
                result.push_back(std::move(plugin));
            }
        }
        
        return result;
    }

    /**
     * @brief Check if plugin is available
     * 
     * @tparam PluginInterface  Interface type
     * @param plugin_name       Plugin identifier
     * @return true if plugin exists and is correct type
     */
    template<typename PluginInterface>
    static bool has(const std::string& plugin_name) {
        return PluginRegistry::hasPlugin<PluginInterface>(plugin_name);
    }

    /**
     * @brief Get plugin with fallback
     * 
     * Returns first available plugin if primary fails.
     * Useful for finding alternative implementations.
     * 
     * @tparam PluginInterface  Interface type
     * @return First available plugin or nullptr
     */
    template<typename PluginInterface>
    static std::unique_ptr<PluginInterface> getWithFallback() {
        auto plugins = getAll<PluginInterface>();
        if (!plugins.empty()) {
            auto result = std::move(plugins[0]);
            return result;
        }
        return nullptr;
    }
};

} // namespace plugins
} // namespace themis
