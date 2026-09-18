/**
 * @file plugin_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include <string>
#include <memory>
#include <functional>

namespace themis {
namespace acceleration {

class BackendPlugin {
public:
    /**
     * @brief Backend Plugin.
     * @return Return value.
     */
    virtual ~BackendPlugin() = default;
    
    /**
     * @brief Plugin Name.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    virtual const char* pluginName() const noexcept = 0;
    
    /**
     * @brief Plugin Version.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    virtual const char* pluginVersion() const noexcept = 0;
    
    /**
     * @brief Backend Type.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    virtual BackendType backendType() const noexcept = 0;
    
    /**
     * @brief Create Vector Backend.
     * @return Return value.
     */
    virtual std::unique_ptr<IVectorBackend> createVectorBackend() = 0;
    
    /**
     * @brief Create Graph Backend.
     * @return Return value.
     */
    virtual std::unique_ptr<IGraphBackend> createGraphBackend() = 0;
    
    /**
     * @brief Create Geo Backend.
     * @return Return value.
     */
    virtual std::unique_ptr<IGeoBackend> createGeoBackend() = 0;
};

class PluginLoader {
public:
    PluginLoader() = default;
    
    ~PluginLoader();
    
    /**
     * @brief Load Plugin.
     * @param[in] libraryPath Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadPlugin(const std::string& libraryPath);
    
    /**
     * @brief Load Plugins From Directory.
     * @param[in] directoryPath Input parameter.
     * @return Return value.
     */
    size_t loadPluginsFromDirectory(const std::string& directoryPath);
    
    /**
     * @brief Unload Plugin.
     * @param[in] pluginName Input parameter.
     */
    void unloadPlugin(const std::string& pluginName);
    
    /**
     * @brief Unload All Plugins.
     */
    void unloadAllPlugins();
    
    /**
     * @brief Get Plugin.
     * @param[in] pluginName Input parameter.
     * @return Pointer to the result.
     */
    BackendPlugin* getPlugin(const std::string& pluginName) const;
    
    /**
     * @brief Get Loaded Plugins.
     * @return Return value.
     */
    std::vector<BackendPlugin*> getLoadedPlugins() const;

private:
    struct PluginHandle {
        void* libraryHandle = nullptr;  /// OS-specific handle (HMODULE on Windows, void* on Unix)
        std::unique_ptr<BackendPlugin> plugin;  /// Plugin instance
        std::string name;                       /// Plugin name from metadata
        std::string path;                       /// File path of the loaded library
        std::string fileHash;                   /// SHA-256 hash for security verification
    };
    
    std::vector<PluginHandle> plugins_;
    
    /**
     * @brief Load Library.
     * @param[in] path Input parameter.
     * @return Pointer to the result.
     */
    void* loadLibrary(const std::string& path);
    
    /**
     * @brief Get Symbol.
     * @param[in,out] handle Input/output parameter.
     * @param[in] symbolName Input parameter.
     * @return Pointer to the result.
     */
    void* getSymbol(void* handle, const std::string& symbolName);
    
    /**
     * @brief Unload Library.
     * @param[in,out] handle Input/output parameter.
     */
    void unloadLibrary(void* handle);
};

using CreatePluginFunc = BackendPlugin* (*)();

#ifndef THEMIS_PLUGIN_EXPORT
#ifdef _WIN32
    #define THEMIS_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define THEMIS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#endif

#define THEMIS_DEFINE_PLUGIN(PluginClass) \
    extern "C" THEMIS_PLUGIN_EXPORT themis::acceleration::BackendPlugin* CreateBackendPlugin() { \
        return new PluginClass(); \
    }

} // namespace acceleration
} // namespace themis
