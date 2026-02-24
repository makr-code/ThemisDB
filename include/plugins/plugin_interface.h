/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_interface.h                                 ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-24 13:03:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     420                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>
#include <cstdio>

/**
 * @brief Unified Plugin Interface for ThemisDB
 * 
 * This interface unifies existing plugin loaders:
 * - acceleration/plugin_loader.h (Compute backends)
 * - security/hsm_provider_pkcs11.cpp (PKCS#11 dynamic loading)
 * - acceleration/zluda_backend.cpp (ZLUDA dynamic loading)
 * 
 * Benefits:
 * - Single plugin architecture for all components
 * - Consistent security verification
 * - Unified plugin discovery and lifecycle
 * - Shared code for DLL loading (Windows/Linux/macOS)
 */

// Platform-specific export macros
#ifndef THEMIS_PLUGIN_EXPORT
#ifdef _WIN32
    #ifdef THEMIS_PLUGIN_EXPORTS
        #define THEMIS_PLUGIN_EXPORT __declspec(dllexport)
    #else
        #define THEMIS_PLUGIN_EXPORT __declspec(dllimport)
    #endif
#else
    #define THEMIS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
#endif

namespace themis {
namespace plugins {

/**
 * @brief Plugin Type Categories
 * 
 * Maps to existing plugin systems:
 * - COMPUTE_BACKEND -> acceleration::BackendPlugin
 * - BLOB_STORAGE -> New blob storage backends
 * - IMPORTER -> New data importers
 * - HSM_PROVIDER -> security::HSMProvider (PKCS#11)
 * - LLM_BACKEND -> llm::ILLMPlugin (v1.5.0+)
 */
enum class PluginType {
    COMPUTE_BACKEND,   // Vector/Graph/Geo acceleration (existing)
    BLOB_STORAGE,      // Storage backends (Filesystem, S3, Azure, WebDAV)
    IMPORTER,          // Data importers (PostgreSQL, MySQL, CSV)
    EXPORTER,          // Data exporters
    HSM_PROVIDER,      // Hardware Security Modules (PKCS#11)
    EMBEDDING,         // Embedding providers (Sentence-BERT, OpenAI)
    LLM_BACKEND,       // LLM backends (llama.cpp, vLLM, etc.) - v1.5.0+
    CUSTOM             // Custom plugins
};

/**
 * @brief Plugin Capabilities
 */
struct PluginCapabilities {
    bool supports_streaming = false;
    bool supports_batching = false;
    bool supports_transactions = false;
    bool thread_safe = false;
    bool gpu_accelerated = false;
};

/**
 * @brief Inclusive version range constraint for plugin capability negotiation.
 *
 * Both bounds are optional: an empty string means "no constraint" on that bound.
 * Example: {min_version="1.0.0", max_version="2.0.0"} matches any version
 * from 1.0.0 to 2.0.0 inclusive.
 */
struct PluginVersionRange {
    std::string min_version;  ///< Inclusive lower bound; "" = no lower bound
    std::string max_version;  ///< Inclusive upper bound; "" = no upper bound

    bool isUnconstrained() const {
        return min_version.empty() && max_version.empty();
    }
};

/**
 * @brief A single capability requirement with an optional plugin version range.
 *
 * The capability_name maps to the boolean flags in PluginCapabilities:
 *   "streaming", "batching", "transactions", "thread_safe", "gpu_accelerated"
 *
 * The version_range constrains the plugin version (IThemisPlugin::getVersion()),
 * enabling callers to express requirements such as "I need streaming support
 * and the plugin must be at least v1.2.0".
 */
struct PluginCapabilityRequirement {
    std::string capability_name;      ///< Named capability (see PluginCapabilities)
    PluginVersionRange version_range; ///< Required plugin version range (optional)
};

/**
 * @brief Result of a plugin capability negotiation pass.
 */
struct PluginNegotiationResult {
    bool success = false;
    std::vector<std::string> satisfied;    ///< Requirements that were met
    std::vector<std::string> unsatisfied;  ///< Requirements that were NOT met
    std::string error_message;
};

/**
 * @brief Base Plugin Interface
 * 
 * All plugins must implement this interface.
 * Type-specific plugins should also implement their domain interface
 * (e.g., IBlobStorageBackend, IImporter, etc.)
 */
class IThemisPlugin {
public:
    virtual ~IThemisPlugin() = default;
    
    /**
     * @brief Get plugin name
     */
    virtual const char* getName() const = 0;
    
    /**
     * @brief Get plugin version (semantic versioning)
     */
    virtual const char* getVersion() const = 0;
    
    /**
     * @brief Get plugin type
     */
    virtual PluginType getType() const = 0;
    
    /**
     * @brief Get plugin capabilities
     */
    virtual PluginCapabilities getCapabilities() const = 0;
    
    /**
     * @brief Initialize plugin with configuration JSON
     * @param config_json Configuration as JSON string
     * @return true if initialized successfully
     */
    virtual bool initialize(const char* config_json) = 0;
    
    /**
     * @brief Shutdown plugin and release resources
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Get plugin instance (type-specific)
     * @return Pointer to plugin implementation (must be cast to specific type)
     * 
     * For COMPUTE_BACKEND: Cast to acceleration::BackendPlugin*
     * For BLOB_STORAGE: Cast to storage::IBlobStorageBackend*
     * For IMPORTER: Cast to importers::IImporter*
     */
    virtual void* getInstance() = 0;
};

/**
 * @brief Stateful Plugin Interface
 * 
 * Optional interface for plugins that need state preservation during hot-reload.
 * Plugins implementing this interface can save their state before unload
 * and restore it after reload, enabling zero-downtime updates.
 * 
 * Thread-Safety: Implementations must be thread-safe
 */
class IStatefulPlugin {
public:
    virtual ~IStatefulPlugin() = default;
    
    /**
     * @brief Save plugin state before reload
     * 
     * Called by PluginManager before unloading during hot-reload.
     * The returned state will be passed back to restoreState() after reload.
     * 
     * @return Serialized state as JSON string, or empty string if no state
     * @throws std::exception on serialization error (will be logged, not fatal)
     */
    virtual std::string saveState() = 0;
    
    /**
     * @brief Restore plugin state after reload
     * 
     * Called by PluginManager after successful reload.
     * The plugin should restore its internal state from the provided data.
     * 
     * @param state Previously saved state from saveState()
     * @return true if state restored successfully, false otherwise
     * @note If restoration fails, plugin remains loaded with default state
     */
    virtual bool restoreState(const std::string& state) = 0;
};

/**
 * @brief Runtime plugin capability negotiator.
 *
 * Checks whether a plugin satisfies a set of PluginCapabilityRequirements,
 * including version-range constraints on the plugin version.
 *
 * Thread-Safety: All methods are stateless and thread-safe.
 *
 * Example usage:
 * @code
 *   PluginCapabilityNegotiator::negotiate(plugin, {
 *       {"streaming",     {"1.0.0", ""}},
 *       {"gpu_accelerated", {"", ""}},
 *   });
 * @endcode
 */
class PluginCapabilityNegotiator {
public:
    /**
     * @brief Negotiate capabilities between a plugin and a list of requirements.
     *
     * For each requirement:
     *   1. Checks that the named capability flag is enabled in the plugin.
     *   2. Checks that the plugin version falls within the required version range.
     *
     * @param plugin       Plugin to inspect (via getVersion() / getCapabilities()).
     * @param requirements List of capability requirements to satisfy.
     * @return PluginNegotiationResult with success flag and per-requirement details.
     */
    static PluginNegotiationResult negotiate(
        const IThemisPlugin& plugin,
        const std::vector<PluginCapabilityRequirement>& requirements)
    {
        PluginNegotiationResult result;
        const char* raw_version = plugin.getVersion();
        const std::string version = raw_version ? raw_version : "";
        const PluginCapabilities caps = plugin.getCapabilities();

        for (const auto& req : requirements) {
            bool cap_ok = checkCapability(req.capability_name, caps);
            bool ver_ok = isVersionInRange(version, req.version_range);

            if (cap_ok && ver_ok) {
                result.satisfied.push_back(req.capability_name);
            } else {
                result.unsatisfied.push_back(req.capability_name);
                if (!result.error_message.empty()) {
                    result.error_message += "; ";
                }
                if (!cap_ok) {
                    result.error_message += "capability '" + req.capability_name
                        + "' not supported";
                } else {
                    result.error_message += "plugin version '" + version
                        + "' out of range [" + req.version_range.min_version
                        + ", " + req.version_range.max_version
                        + "] for '" + req.capability_name + "'";
                }
            }
        }

        result.success = result.unsatisfied.empty();
        return result;
    }

    /**
     * @brief Check whether a plugin version satisfies a version range.
     *
     * Uses semantic versioning (major.minor.patch).  Non-parseable or empty
     * version strings are treated as (0, 0, 0) and satisfy only unconstrained
     * ranges.
     *
     * @param version Plugin version string (e.g. "1.2.3").
     * @param range   Required version range.
     * @return true if version satisfies range.
     */
    static bool isVersionInRange(const std::string& version,
                                 const PluginVersionRange& range)
    {
        if (range.isUnconstrained()) {
            return true;
        }
        // An unversioned plugin satisfies only unconstrained ranges.
        if (version.empty()) {
            return false;
        }
        auto ver = parseVersion(version);
        if (!range.min_version.empty() && ver < parseVersion(range.min_version)) {
            return false;
        }
        if (!range.max_version.empty() && ver > parseVersion(range.max_version)) {
            return false;
        }
        return true;
    }

    /**
     * @brief Check whether a named capability is enabled.
     *
     * Recognised names: "streaming", "batching", "transactions",
     *                   "thread_safe", "gpu_accelerated".
     * Unknown names always return false.
     *
     * @param name Capability name.
     * @param caps Plugin capabilities struct.
     * @return true if capability is enabled.
     */
    static bool checkCapability(const std::string& name,
                                const PluginCapabilities& caps)
    {
        if (name == "streaming")       return caps.supports_streaming;
        if (name == "batching")        return caps.supports_batching;
        if (name == "transactions")    return caps.supports_transactions;
        if (name == "thread_safe")     return caps.thread_safe;
        if (name == "gpu_accelerated") return caps.gpu_accelerated;
        return false;
    }

private:
    using Version3 = std::tuple<int, int, int>;

    static Version3 parseVersion(const std::string& v) {
        int major = 0, minor = 0, patch = 0;
        if (!v.empty()) {
            // Pre-initialised to 0; partial parses (e.g. "1.2.x") degrade
            // gracefully, matching the behaviour in module_dependency_resolver.cpp.
            (void)std::sscanf(v.c_str(), "%d.%d.%d", &major, &minor, &patch);
        }
        return {major, minor, patch};
    }
};

/**
 * @brief Plugin Entry Points
 * 
 * Every plugin DLL must export these two functions:
 */
typedef IThemisPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IThemisPlugin*);

/**
 * @brief Plugin Manifest (parsed from plugin.json)
 */
struct PluginManifest {
    std::string name;
    std::string version;
    std::string description;
    PluginType type;
    
    // Platform-specific binaries
    std::string binary_windows;  // .dll
    std::string binary_linux;    // .so
    std::string binary_macos;    // .dylib
    
    // Dependencies
    std::vector<std::string> dependencies;
    
    // Capabilities
    PluginCapabilities capabilities;
    
    // Auto-load on startup?
    bool auto_load = false;
    
    // Load priority (lower = higher priority)
    int load_priority = 100;
    
    // Config schema (JSON Schema)
    std::string config_schema;
    
    // Expected SHA-256 hash of the binary (hex-encoded).
    // When set, the plugin manager verifies the on-disk binary hash before loading.
    // Leave empty to skip hash enforcement (development/unsigned builds).
    std::string expected_hash;
};

} // namespace plugins
} // namespace themis

/**
 * @brief Convenience macro for plugin implementation
 * 
 * Usage:
 * ```cpp
 * class MyPlugin : public IThemisPlugin { ... };
 * 
 * THEMIS_PLUGIN_IMPL(MyPlugin)
 * ```
 */
#define THEMIS_PLUGIN_IMPL(PluginClass) \
    extern "C" { \
        THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() { \
            return new PluginClass(); \
        } \
        THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) { \
            delete plugin; \
        } \
    }
