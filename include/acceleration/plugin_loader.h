/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_loader.h                                    ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:19:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     116                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
