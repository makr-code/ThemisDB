/**
 * @file plugin_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// @brief Interface for dynamically-loaded acceleration backend plugins.
///
/// Plugins implement this interface to provide factory methods for creating
/// backend instances (vector, graph, geo) and expose metadata about the plugin
/// (name, version, backend type).
///
/// @note Plugins are typically created by the THEMIS_DEFINE_PLUGIN() macro, which
///       generates the required CreateBackendPlugin() entry point.
class BackendPlugin {
public:
    virtual ~BackendPlugin() = default;
    
    /// @brief Get the human-readable name of the plugin.
    /// @return A null-terminated C string (e.g., "CUDA Backend Plugin v1.0").
    virtual const char* pluginName() const noexcept = 0;
    
    /// @brief Get the semantic version string of the plugin.
    /// @return A null-terminated C string in semver format (e.g., "1.0.0").
    virtual const char* pluginVersion() const noexcept = 0;
    
    /// @brief Get the backend type (CUDA, HIP, OpenCL, Vulkan, etc.).
    /// @return The BackendType enum value this plugin implements.
    virtual BackendType backendType() const noexcept = 0;
    
    /// @brief Create a vector compute backend instance.
    /// @return A unique_ptr to a newly-allocated IVectorBackend instance.
    /// @throws Any exception derived from std::exception if creation fails.
    /// @note The caller owns the returned instance; responsibility is transferred.
    virtual std::unique_ptr<IVectorBackend> createVectorBackend() = 0;
    
    /// @brief Create a graph compute backend instance.
    /// @return A unique_ptr to a newly-allocated IGraphBackend instance.
    /// @throws Any exception derived from std::exception if creation fails.
    /// @note The caller owns the returned instance; responsibility is transferred.
    virtual std::unique_ptr<IGraphBackend> createGraphBackend() = 0;
    
    /// @brief Create a geospatial compute backend instance.
    /// @return A unique_ptr to a newly-allocated IGeoBackend instance.
    /// @throws Any exception derived from std::exception if creation fails.
    /// @note The caller owns the returned instance; responsibility is transferred.
    virtual std::unique_ptr<IGeoBackend> createGeoBackend() = 0;
};

/// @brief Dynamic plugin loader for acceleration backend implementations.
///
/// Loads and manages backend plugins from shared libraries (.dll on Windows,
/// .so on Linux, .dylib on macOS). Each plugin must export a CreateBackendPlugin()
/// function and implement the BackendPlugin interface.
///
/// Features:
/// - Lazy loading: plugins are loaded on demand.
/// - Platform-aware: handles Windows/Unix differences automatically.
/// - Security: SHA-256 hash verification of plugin files (if enabled).
/// - Non-copyable but movable RAII resource management.
///
/// @note Thread-safety of public methods is not guaranteed; callers must
///       synchronize access if PluginLoader is shared across threads.
class PluginLoader {
public:
    /// @brief Construct an empty plugin loader.
    PluginLoader() = default;
    
    /// @brief Destructor; unloads all loaded plugins.
    ~PluginLoader();
    
    /// @brief Load a plugin from a shared library.
    /// @param libraryPath Full or relative path to the library (.so/.dll/.dylib).
    /// @return true if the plugin was loaded successfully; false otherwise.
    /// @throws std::runtime_error on library loading errors or if the plugin
    ///         entry point is not found.
    /// @note On failure, an error message is typically logged. Call getPlugin()
    ///       after loading to verify the plugin was registered.
    bool loadPlugin(const std::string& libraryPath);
    
    /// @brief Load all plugins from a directory.
    /// @param directoryPath Path to scan for plugin libraries.
    /// @return The number of successfully loaded plugins.
    /// @throws std::runtime_error if the directory cannot be read.
    /// @note Non-plugin files are silently skipped.
    size_t loadPluginsFromDirectory(const std::string& directoryPath);
    
    /// @brief Unload a specific plugin by name.
    /// @param pluginName The name of the plugin to unload (from BackendPlugin::pluginName()).
    /// @note If the plugin is not loaded, this is a no-op.
    void unloadPlugin(const std::string& pluginName);
    
    /// @brief Unload all currently loaded plugins.
    void unloadAllPlugins();
    
    /// @brief Retrieve a loaded plugin by name.
    /// @param pluginName The plugin name to search for.
    /// @return A pointer to the BackendPlugin, or nullptr if not found.
    /// @note The returned pointer is valid until unloadPlugin() or
    ///       unloadAllPlugins() is called.
    BackendPlugin* getPlugin(const std::string& pluginName) const;
    
    /// @brief Get all currently loaded plugins.
    /// @return A vector of BackendPlugin pointers.
    /// @note Pointers remain valid until unloadPlugin() or unloadAllPlugins().
    std::vector<BackendPlugin*> getLoadedPlugins() const;

private:
    /// @brief OS-specific library handle with metadata and plugin instance.
    struct PluginHandle {
        void* libraryHandle = nullptr;  /// OS-specific handle (HMODULE on Windows, void* on Unix)
        std::unique_ptr<BackendPlugin> plugin;  /// Plugin instance
        std::string name;                       /// Plugin name from metadata
        std::string path;                       /// File path of the loaded library
        std::string fileHash;                   /// SHA-256 hash for security verification
    };
    
    std::vector<PluginHandle> plugins_;
    
    /// @brief Platform-specific: load a shared library.
    /// @param path Path to the library file.
    /// @return OS-specific handle (HMODULE on Windows, void* on Unix).
    /// @throws std::runtime_error on failure.
    void* loadLibrary(const std::string& path);
    
    /// @brief Platform-specific: retrieve a symbol from a loaded library.
    /// @param handle The library handle from loadLibrary().
    /// @param symbolName The name of the exported symbol.
    /// @return Pointer to the symbol, or nullptr if not found.
    void* getSymbol(void* handle, const std::string& symbolName);
    
    /// @brief Platform-specific: unload a shared library.
    /// @param handle The library handle from loadLibrary().
    void unloadLibrary(void* handle);
};

/// @brief Plugin factory function signature.
///
/// Every backend plugin must export a function with this signature:
/// ```c
/// extern "C" BackendPlugin* CreateBackendPlugin();
/// ```
///
/// The PluginLoader calls this function to instantiate the plugin.
/// The caller (PluginLoader) takes ownership of the returned pointer.
///
/// @return A newly-allocated BackendPlugin instance, or nullptr on failure.
using CreatePluginFunc = BackendPlugin* (*)();

/// @brief Platform-specific macro for exporting plugin symbols.
///
/// On Windows, expands to `__declspec(dllexport)`.
/// On Unix-like systems, expands to `__attribute__((visibility("default")))`.
/// 
/// Use this to mark the CreateBackendPlugin() function as exported in a plugin DLL.
#ifndef THEMIS_PLUGIN_EXPORT
#ifdef _WIN32
    #define THEMIS_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define THEMIS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#endif

/// @brief Helper macro to define a plugin entry point.
///
/// Expands to the CreateBackendPlugin() function that PluginLoader expects.
/// 
/// Usage:
/// ```cpp
/// class MyBackendPlugin : public BackendPlugin { ... };
/// THEMIS_DEFINE_PLUGIN(MyBackendPlugin);
/// ```
///
/// @param PluginClass The class that implements BackendPlugin.
#define THEMIS_DEFINE_PLUGIN(PluginClass) \
    extern "C" THEMIS_PLUGIN_EXPORT themis::acceleration::BackendPlugin* CreateBackendPlugin() { \
        return new PluginClass(); \
    }

} // namespace acceleration
} // namespace themis
