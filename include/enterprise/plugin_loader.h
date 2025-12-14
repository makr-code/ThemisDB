// Copyright (c) 2025 ThemisDB Contributors
// Licensed under the MIT License

#pragma once

#include "enterprise/enterprise_plugin.h"
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    using DLLHandle = HMODULE;
#else
    #include <dlfcn.h>
    using DLLHandle = void*;
#endif

namespace themis {
namespace enterprise {

/**
 * @brief License information structure
 */
struct LicenseInfo {
    std::string organization;
    std::string license_key;
    std::chrono::system_clock::time_point issued_date;
    std::chrono::system_clock::time_point expiry_date;
    std::string edition;
    std::vector<std::string> enabled_modules;
    std::unordered_map<std::string, int64_t> limits;
    bool is_valid;
};

/**
 * @brief Plugin loader and manager for enterprise features
 * 
 * Responsibilities:
 * - Load enterprise DLLs dynamically
 * - Validate licenses
 * - Manage plugin lifecycle
 * - Provide access to loaded plugins
 */
class EnterprisePluginLoader {
public:
    EnterprisePluginLoader();
    ~EnterprisePluginLoader();
    
    /**
     * @brief Load license from file
     * @param license_path Path to license JSON file
     * @return true if license loaded and validated successfully
     */
    bool loadLicense(const std::filesystem::path& license_path);
    
    /**
     * @brief Load a specific enterprise plugin
     * @param dll_path Path to plugin DLL/SO
     * @return true if plugin loaded and initialized successfully
     */
    bool loadPlugin(const std::filesystem::path& dll_path);
    
    /**
     * @brief Load all enterprise plugins from a directory
     * @param plugin_dir Directory containing enterprise DLLs
     * @return Number of plugins successfully loaded
     */
    size_t loadAllPlugins(const std::filesystem::path& plugin_dir);
    
    /**
     * @brief Unload a specific plugin
     * @param module Module type to unload
     */
    void unloadPlugin(FeatureModule module);
    
    /**
     * @brief Unload all plugins
     */
    void unloadAllPlugins();
    
    /**
     * @brief Check if a module is loaded
     */
    bool isModuleLoaded(FeatureModule module) const;
    
    /**
     * @brief Get a loaded plugin
     * @param module Module type to retrieve
     * @return Pointer to plugin, or nullptr if not loaded
     */
    IEnterprisePlugin* getPlugin(FeatureModule module) const;
    
    /**
     * @brief Get license information
     */
    const LicenseInfo& getLicenseInfo() const {
        return license_info_;
    }
    
    /**
     * @brief Check if a module is licensed
     */
    bool isModuleLicensed(const std::string& module_name) const;
    
    /**
     * @brief Get list of loaded module names
     */
    std::vector<std::string> getLoadedModules() const;
    
private:
    struct PluginEntry {
        DLLHandle dll_handle;
        std::unique_ptr<IEnterprisePlugin> plugin;
        std::filesystem::path dll_path;
    };
    
    // Helper functions for DLL loading
    DLLHandle loadDLL(const std::filesystem::path& path);
    void* getSymbol(DLLHandle handle, const char* symbol_name);
    void unloadDLL(DLLHandle handle);
    
    // License validation
    std::optional<LicenseInfo> validateLicenseFile(const std::filesystem::path& license_path);
    
    // Module name to FeatureModule mapping
    static FeatureModule moduleNameToEnum(const std::string& name);
    static std::string moduleEnumToName(FeatureModule module);
    
    // Loaded plugins
    std::unordered_map<FeatureModule, PluginEntry> plugins_;
    
    // License information
    LicenseInfo license_info_;
    
    // Configuration
    std::string config_json_;
    std::string data_path_;
};

} // namespace enterprise
} // namespace themis
