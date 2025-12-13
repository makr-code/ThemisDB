// Copyright (c) 2025 ThemisDB Contributors
// Licensed under the MIT License

#include "enterprise/plugin_loader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ranges.h>
#include <nlohmann/json.hpp>

// Platform-specific headers
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

using json = nlohmann::json;

namespace themis {
namespace enterprise {

EnterprisePluginLoader::EnterprisePluginLoader() {
    license_info_.is_valid = false;
}

EnterprisePluginLoader::~EnterprisePluginLoader() {
    unloadAllPlugins();
}

bool EnterprisePluginLoader::loadLicense(const std::filesystem::path& license_path) {
    auto license_opt = validateLicenseFile(license_path);
    if (!license_opt) {
        spdlog::error("Failed to validate license file: {}", license_path.string());
        return false;
    }
    
    license_info_ = std::move(*license_opt);
    spdlog::info("License loaded successfully for organization: {}", license_info_.organization);
    
    // Format expiry date in human-readable format
    auto expiry_time = std::chrono::system_clock::to_time_t(license_info_.expiry_date);
    std::tm expiry_tm = {};
#ifdef _WIN32
    localtime_s(&expiry_tm, &expiry_time);
#else
    localtime_r(&expiry_time, &expiry_tm);
#endif
    std::ostringstream expiry_str;
    expiry_str << std::put_time(&expiry_tm, "%Y-%m-%d %H:%M:%S");
    
    spdlog::info("License expires: {}", expiry_str.str());
    spdlog::info("Enabled modules: {}", fmt::join(license_info_.enabled_modules, ", "));
    
    return true;
}

bool EnterprisePluginLoader::loadPlugin(const std::filesystem::path& dll_path) {
    if (!license_info_.is_valid) {
        spdlog::error("Cannot load plugin: no valid license loaded");
        return false;
    }
    
    spdlog::info("Loading enterprise plugin: {}", dll_path.string());
    
    // Load DLL
    auto dll_handle = loadDLL(dll_path);
    if (!dll_handle) {
        spdlog::error("Failed to load DLL: {}", dll_path.string());
        return false;
    }
    
    // Get API version
    using GetApiVersionFn = uint32_t (*)();
    auto get_api_version = (GetApiVersionFn)getSymbol(dll_handle, "getPluginApiVersion");
    if (get_api_version) {
        uint32_t api_version = get_api_version();
        if (api_version != THEMIS_PLUGIN_API_VERSION) {
            spdlog::error("Plugin API version mismatch: expected {}, got {}", 
                THEMIS_PLUGIN_API_VERSION, api_version);
            unloadDLL(dll_handle);
            return false;
        }
    }
    
    // Get factory function
    using CreatePluginFn = IEnterprisePlugin* (*)();
    auto create_plugin = (CreatePluginFn)getSymbol(dll_handle, "createPlugin");
    if (!create_plugin) {
        spdlog::error("Failed to find createPlugin symbol in {}", dll_path.string());
        unloadDLL(dll_handle);
        return false;
    }
    
    // Create plugin instance
    auto plugin = create_plugin();
    if (!plugin) {
        spdlog::error("createPlugin() returned nullptr for {}", dll_path.string());
        unloadDLL(dll_handle);
        return false;
    }
    
    // Validate license for this module
    if (!plugin->validateLicense(license_info_.license_key.c_str())) {
        spdlog::error("License validation failed for module: {}", plugin->getModuleName());
        delete plugin;
        unloadDLL(dll_handle);
        return false;
    }
    
    // Check if module is licensed
    std::string module_name = plugin->getModuleName();
    if (!isModuleLicensed(module_name)) {
        spdlog::error("Module '{}' is not included in license", module_name);
        delete plugin;
        unloadDLL(dll_handle);
        return false;
    }
    
    // Initialize plugin
    PluginConfig config;
    config.config_json = config_json_.c_str();
    config.license_key = license_info_.license_key.c_str();
    config.data_path = data_path_.c_str();
    config.user_data = nullptr;
    
    auto result = plugin->initialize(config);
    if (!result.success) {
        spdlog::error("Plugin initialization failed: {} (error code: {})", 
            result.error_message ? result.error_message : "unknown", 
            result.error_code);
        delete plugin;
        unloadDLL(dll_handle);
        return false;
    }
    
    // Store plugin
    FeatureModule module_type = plugin->getModuleType();
    PluginEntry entry;
    entry.dll_handle = dll_handle;
    entry.plugin = std::unique_ptr<IEnterprisePlugin>(plugin);
    entry.dll_path = dll_path;
    
    plugins_[module_type] = std::move(entry);
    
    spdlog::info("Successfully loaded enterprise plugin: {} v{}", 
        plugin->getModuleName(), plugin->getVersion());
    
    return true;
}

size_t EnterprisePluginLoader::loadAllPlugins(const std::filesystem::path& plugin_dir) {
    if (!std::filesystem::exists(plugin_dir)) {
        spdlog::warn("Enterprise plugin directory does not exist: {}", plugin_dir.string());
        return 0;
    }
    
    size_t loaded_count = 0;
    
#ifdef _WIN32
    const std::string dll_extension = ".dll";
#elif defined(__APPLE__)
    const std::string dll_extension = ".dylib";
#else
    const std::string dll_extension = ".so";
#endif
    
    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (!entry.is_regular_file()) continue;
        
        auto path = entry.path();
        if (path.extension() == dll_extension) {
            // Look for enterprise plugins (themis_enterprise_*.dll/so)
            std::string filename = path.filename().string();
            if (filename.find("themis_enterprise_") == 0) {
                if (loadPlugin(path)) {
                    loaded_count++;
                }
            }
        }
    }
    
    spdlog::info("Loaded {} enterprise plugins from {}", loaded_count, plugin_dir.string());
    return loaded_count;
}

void EnterprisePluginLoader::unloadPlugin(FeatureModule module) {
    auto it = plugins_.find(module);
    if (it == plugins_.end()) {
        return;
    }
    
    auto& entry = it->second;
    
    // Shutdown plugin
    if (entry.plugin) {
        entry.plugin->shutdown();
        entry.plugin.reset();
    }
    
    // Unload DLL
    if (entry.dll_handle) {
        unloadDLL(entry.dll_handle);
    }
    
    plugins_.erase(it);
}

void EnterprisePluginLoader::unloadAllPlugins() {
    for (auto& [module, entry] : plugins_) {
        if (entry.plugin) {
            entry.plugin->shutdown();
            entry.plugin.reset();
        }
        if (entry.dll_handle) {
            unloadDLL(entry.dll_handle);
        }
    }
    plugins_.clear();
}

bool EnterprisePluginLoader::isModuleLoaded(FeatureModule module) const {
    return plugins_.find(module) != plugins_.end();
}

IEnterprisePlugin* EnterprisePluginLoader::getPlugin(FeatureModule module) const {
    auto it = plugins_.find(module);
    return (it != plugins_.end()) ? it->second.plugin.get() : nullptr;
}

bool EnterprisePluginLoader::isModuleLicensed(const std::string& module_name) const {
    if (!license_info_.is_valid) {
        return false;
    }
    
    // Convert to lowercase for comparison
    std::string lower_name = module_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    for (const auto& licensed_module : license_info_.enabled_modules) {
        std::string lower_licensed = licensed_module;
        std::transform(lower_licensed.begin(), lower_licensed.end(), 
                      lower_licensed.begin(), ::tolower);
        if (lower_licensed == lower_name) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> EnterprisePluginLoader::getLoadedModules() const {
    std::vector<std::string> modules;
    modules.reserve(plugins_.size());
    
    for (const auto& [module, entry] : plugins_) {
        if (entry.plugin) {
            modules.push_back(entry.plugin->getModuleName());
        }
    }
    
    return modules;
}

// Platform-specific DLL loading
DLLHandle EnterprisePluginLoader::loadDLL(const std::filesystem::path& path) {
#ifdef _WIN32
    return LoadLibraryA(path.string().c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY);
#endif
}

void* EnterprisePluginLoader::getSymbol(DLLHandle handle, const char* symbol_name) {
#ifdef _WIN32
    return (void*)GetProcAddress(handle, symbol_name);
#else
    return dlsym(handle, symbol_name);
#endif
}

void EnterprisePluginLoader::unloadDLL(DLLHandle handle) {
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

std::optional<LicenseInfo> EnterprisePluginLoader::validateLicenseFile(
    const std::filesystem::path& license_path) {
    
    try {
        // Read license file
        std::ifstream file(license_path);
        if (!file.is_open()) {
            spdlog::error("Failed to open license file: {}", license_path.string());
            return std::nullopt;
        }
        
        json license_json;
        file >> license_json;
        
        LicenseInfo info;
        
        // Parse basic fields
        info.license_key = license_json.value("license_key", "");
        info.organization = license_json.value("organization", "");
        info.edition = license_json.value("edition", "");
        
        // Parse dates (ISO 8601 format: YYYY-MM-DD)
        std::string issued_str = license_json.value("issued_date", "");
        std::string expiry_str = license_json.value("expiry_date", "");
        
        std::tm issued_tm = {};
        std::tm expiry_tm = {};
        std::istringstream issued_ss(issued_str);
        std::istringstream expiry_ss(expiry_str);
        
        issued_ss >> std::get_time(&issued_tm, "%Y-%m-%d");
        if (issued_ss.fail()) {
            spdlog::error("Failed to parse issued_date: {}", issued_str);
            return std::nullopt;
        }
        
        expiry_ss >> std::get_time(&expiry_tm, "%Y-%m-%d");
        if (expiry_ss.fail()) {
            spdlog::error("Failed to parse expiry_date: {}", expiry_str);
            return std::nullopt;
        }
        
        info.issued_date = std::chrono::system_clock::from_time_t(std::mktime(&issued_tm));
        info.expiry_date = std::chrono::system_clock::from_time_t(std::mktime(&expiry_tm));
        
        // Check expiry
        auto now = std::chrono::system_clock::now();
        if (now > info.expiry_date) {
            spdlog::error("License has expired");
            return std::nullopt;
        }
        
        // Parse modules
        if (license_json.contains("modules") && license_json["modules"].is_array()) {
            for (const auto& module : license_json["modules"]) {
                info.enabled_modules.push_back(module.get<std::string>());
            }
        }
        
        // Parse limits
        if (license_json.contains("limits") && license_json["limits"].is_object()) {
            for (auto it = license_json["limits"].begin(); 
                 it != license_json["limits"].end(); ++it) {
                info.limits[it.key()] = it.value().get<int64_t>();
            }
        }
        
        // TODO: Verify RSA signature
        // SECURITY WARNING: This is a stub implementation.
        // Production use requires RSA signature verification.
        // Example implementation:
        // 1. Load public key from embedded certificate
        // 2. Hash license fields (organization, dates, modules, limits)
        // 3. Verify signature using RSA-SHA256
        // 4. Return nullopt if signature verification fails
        //
        // For now, we just check if required fields are present
        if (info.license_key.empty() || info.organization.empty() || 
            info.enabled_modules.empty()) {
            spdlog::error("Invalid license file: missing required fields");
            return std::nullopt;
        }
        
        // Check for signature field (should be present even if not verified yet)
        if (!license_json.contains("signature")) {
            spdlog::warn("License file missing signature field - using development mode");
        }
        
        info.is_valid = true;
        return info;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception while validating license: {}", e.what());
        return std::nullopt;
    }
}

FeatureModule EnterprisePluginLoader::moduleNameToEnum(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "sharding") return FeatureModule::SHARDING;
    if (lower == "gpu") return FeatureModule::GPU;
    if (lower == "analytics") return FeatureModule::ANALYTICS;
    if (lower == "replication") return FeatureModule::REPLICATION;
    if (lower == "security") return FeatureModule::SECURITY;
    if (lower == "management") return FeatureModule::MANAGEMENT;
    if (lower == "content") return FeatureModule::CONTENT;
    
    throw std::runtime_error("Unknown module name: " + name);
}

std::string EnterprisePluginLoader::moduleEnumToName(FeatureModule module) {
    switch (module) {
        case FeatureModule::SHARDING: return "Sharding";
        case FeatureModule::GPU: return "GPU";
        case FeatureModule::ANALYTICS: return "Analytics";
        case FeatureModule::REPLICATION: return "Replication";
        case FeatureModule::SECURITY: return "Security";
        case FeatureModule::MANAGEMENT: return "Management";
        case FeatureModule::CONTENT: return "Content";
        default: return "Unknown";
    }
}

} // namespace enterprise
} // namespace themis
