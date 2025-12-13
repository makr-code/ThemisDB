// Copyright (c) 2025 ThemisDB Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Cross-platform DLL export/import macros
#ifdef _WIN32
    #ifdef THEMIS_ENTERPRISE_EXPORTS
        #define THEMIS_ENTERPRISE_API __declspec(dllexport)
    #else
        #define THEMIS_ENTERPRISE_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_ENTERPRISE_API __attribute__((visibility("default")))
#endif

namespace themis {
namespace enterprise {

/**
 * @brief Enterprise feature modules
 */
enum class FeatureModule {
    SHARDING,          // Horizontal scaling, consistent hashing, cross-shard joins
    GPU,               // GPU acceleration (CUDA, Vulkan, HIP, etc.)
    ANALYTICS,         // Advanced OLAP/CEP analytics
    REPLICATION,       // Leader-follower, multi-master replication
    SECURITY,          // RBAC, HSM, field encryption, audit logging
    MANAGEMENT,        // Multi-tenancy, rate limiting, load shedding
    CONTENT            // Content processors (PDF, video, geo, CAD)
};

/**
 * @brief Plugin API version for compatibility checking
 */
constexpr uint32_t THEMIS_PLUGIN_API_VERSION = 1;

/**
 * @brief Base configuration structure
 * Uses C-compatible types to avoid ABI issues across compilers
 */
struct PluginConfig {
    const char* config_json;     // JSON configuration string
    const char* license_key;     // License key for validation
    const char* data_path;       // Base data directory
    void* user_data;             // Opaque user data pointer
};

/**
 * @brief Plugin initialization result
 */
struct PluginResult {
    bool success;
    const char* error_message;   // NULL if success, error description otherwise
    uint32_t error_code;         // 0 if success, error code otherwise
};

/**
 * @brief Base interface for all enterprise plugins
 * 
 * All virtual methods use C++ ABI but factory functions use C linkage.
 * Plugins must be built with the same compiler toolchain as the core.
 */
class IEnterprisePlugin {
public:
    virtual ~IEnterprisePlugin() = default;
    
    /**
     * @brief Initialize the plugin with configuration
     * @param config Plugin configuration
     * @return Result of initialization
     */
    virtual PluginResult initialize(const PluginConfig& config) = 0;
    
    /**
     * @brief Shutdown the plugin and release resources
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Get the feature module type
     */
    virtual FeatureModule getModuleType() const = 0;
    
    /**
     * @brief Get the module name (e.g., "Sharding", "GPU")
     */
    virtual const char* getModuleName() const = 0;
    
    /**
     * @brief Get the plugin version (semantic versioning)
     */
    virtual const char* getVersion() const = 0;
    
    /**
     * @brief Get the API version this plugin was built against
     */
    virtual uint32_t getApiVersion() const = 0;
    
    /**
     * @brief Validate license for this module
     * @param license_key License key string
     * @return true if license is valid, false otherwise
     */
    virtual bool validateLicense(const char* license_key) = 0;
    
    /**
     * @brief Get plugin capabilities as JSON string
     * @return JSON string describing capabilities (caller must not free)
     */
    virtual const char* getCapabilities() const = 0;
};

/**
 * @brief Helper base class implementing common plugin functionality
 */
class EnterprisePluginBase : public IEnterprisePlugin {
public:
    EnterprisePluginBase(FeatureModule module, 
                        const char* name, 
                        const char* version)
        : module_(module)
        , name_(name)
        , version_(version)
        , initialized_(false) {
    }
    
    virtual ~EnterprisePluginBase() = default;
    
    FeatureModule getModuleType() const override {
        return module_;
    }
    
    const char* getModuleName() const override {
        return name_;
    }
    
    const char* getVersion() const override {
        return version_;
    }
    
    uint32_t getApiVersion() const override {
        return THEMIS_PLUGIN_API_VERSION;
    }
    
    bool isInitialized() const {
        return initialized_;
    }
    
protected:
    FeatureModule module_;
    const char* name_;
    const char* version_;
    bool initialized_;
};

} // namespace enterprise
} // namespace themis

// C-style factory functions for each plugin DLL
// These must be implemented by each plugin module
extern "C" {
    /**
     * @brief Create a plugin instance
     * @return Pointer to plugin instance, or NULL on failure
     */
    THEMIS_ENTERPRISE_API themis::enterprise::IEnterprisePlugin* createPlugin();
    
    /**
     * @brief Destroy a plugin instance
     * @param plugin Plugin instance to destroy
     */
    THEMIS_ENTERPRISE_API void destroyPlugin(themis::enterprise::IEnterprisePlugin* plugin);
    
    /**
     * @brief Get the plugin API version
     * @return API version number
     */
    THEMIS_ENTERPRISE_API uint32_t getPluginApiVersion();
}
