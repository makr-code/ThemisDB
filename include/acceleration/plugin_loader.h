/*
 * ThemisDB | File: plugin_loader.h | Version: 0.0.47 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 99
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "acceleration/compute_backend.h"
#include <string>
#include <memory>
#include <functional>

namespace themis {
namespace acceleration {

// Plugin loading interface
class BackendPlugin {
public:
    virtual ~BackendPlugin() = default;
    
    // Plugin metadata
    virtual const char* pluginName() const noexcept = 0;
    virtual const char* pluginVersion() const noexcept = 0;
    virtual BackendType backendType() const noexcept = 0;
    
    // Factory methods to create backend instances
    virtual std::unique_ptr<IVectorBackend> createVectorBackend() = 0;
    virtual std::unique_ptr<IGraphBackend> createGraphBackend() = 0;
    virtual std::unique_ptr<IGeoBackend> createGeoBackend() = 0;
};

// Plugin loader - dynamically loads shared libraries
class PluginLoader {
public:
    PluginLoader() = default;
    ~PluginLoader();
    
    // Load a plugin from a shared library (.dll, .so, .dylib)
    // Returns true if successful, false otherwise
    bool loadPlugin(const std::string& libraryPath);
    
    // Load all plugins from a directory
    size_t loadPluginsFromDirectory(const std::string& directoryPath);
    
    // Unload a specific plugin
    void unloadPlugin(const std::string& pluginName);
    
    // Unload all plugins
    void unloadAllPlugins();
    
    // Get loaded plugin by name
    BackendPlugin* getPlugin(const std::string& pluginName) const;
    
    // Get all loaded plugins
    std::vector<BackendPlugin*> getLoadedPlugins() const;
    
private:
    struct PluginHandle {
        void* libraryHandle = nullptr;  // OS-specific handle (HMODULE on Windows, void* on Unix)
        std::unique_ptr<BackendPlugin> plugin;
        std::string name;
        std::string path;
        std::string fileHash;  // SHA-256 hash for security verification
    };
    
    std::vector<PluginHandle> plugins_;
    
    // Platform-specific loading
    void* loadLibrary(const std::string& path);
    void* getSymbol(void* handle, const std::string& symbolName);
    void unloadLibrary(void* handle);
};

// Standard plugin entry point signature
// Each plugin DLL must export this function:
// extern "C" EXPORT BackendPlugin* CreateBackendPlugin();
using CreatePluginFunc = BackendPlugin* (*)();

// Helper macro for plugin exports
#ifndef THEMIS_PLUGIN_EXPORT
#ifdef _WIN32
    #define THEMIS_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define THEMIS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#endif

// Macro to define a plugin entry point
#define THEMIS_DEFINE_PLUGIN(PluginClass) \
    extern "C" THEMIS_PLUGIN_EXPORT themis::acceleration::BackendPlugin* CreateBackendPlugin() { \
        return new PluginClass(); \
    }

} // namespace acceleration
} // namespace themis
