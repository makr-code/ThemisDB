#include "plugins/plugin_manager.h"
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include "acceleration/plugin_security.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace themis {
namespace plugins {

namespace fs = std::filesystem;

// ============================================================================
// Constants
// ============================================================================

// Brief delay after unloading to allow OS to release file handles and cleanup
constexpr auto RELOAD_UNLOAD_DELAY_MS = std::chrono::milliseconds(50);

// ============================================================================
// Platform-specific DLL loading (reused from acceleration/plugin_loader.cpp)
// ============================================================================

void* PluginManager::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

void* PluginManager::getSymbol(void* handle, const std::string& symbolName) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
#else
    return dlsym(handle, symbolName.c_str());
#endif
}

void PluginManager::unloadLibrary(void* handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

// ============================================================================
// Security & Hashing
// ============================================================================

std::string PluginManager::calculateFileHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return "";
    }
    
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    EVP_MD_CTX_free(mdctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

bool PluginManager::verifyPlugin(const std::string& path, std::string& error_message) {
    using namespace themis::acceleration;
    
    PluginSecurityPolicy policy;
    
#ifdef NDEBUG
    // Production: Require signature
    policy.requireSignature = true;
    policy.allowUnsigned = false;
#else
    // Development: Allow unsigned
    policy.requireSignature = false;
    policy.allowUnsigned = true;
#endif
    
    PluginSecurityVerifier verifier(policy);
    return verifier.verifyPlugin(path, error_message);
}

// ============================================================================
// Manifest Signature Verification
// ============================================================================

bool PluginManager::verifyManifestSignature(const std::string& manifest_path, std::string& error_message) {
    // Signature verification strategy:
    // 1. Check for manifest_path + ".sig" file (digital signature)
    // 2. Verify SHA256 hash matches signature file content
    // 3. In production, require valid signature
    
#ifdef NDEBUG
    // Production mode: Require signature
    std::string sig_path = manifest_path + ".sig";
    
    if (!fs::exists(sig_path)) {
        error_message = "Manifest signature file not found: " + sig_path;
        THEMIS_ERROR("{}", error_message);
        return false;
    }
    
    try {
        // Read signature file (contains expected SHA256 hash)
        std::ifstream sig_file(sig_path);
        std::string expected_hash;
        std::getline(sig_file, expected_hash);
        
        // Trim whitespace
        expected_hash.erase(0, expected_hash.find_first_not_of(" \t\n\r"));
        expected_hash.erase(expected_hash.find_last_not_of(" \t\n\r") + 1);
        
        // Compute actual hash of manifest
        std::string actual_hash = calculateFileHash(manifest_path);
        
        if (actual_hash.empty()) {
            error_message = "Failed to compute hash for manifest: " + manifest_path;
            THEMIS_ERROR("{}", error_message);
            return false;
        }
        
        // Compare hashes
        if (expected_hash != actual_hash) {
            error_message = "Manifest signature verification failed: hash mismatch\n"
                          "  Expected: " + expected_hash + "\n"
                          "  Actual:   " + actual_hash;
            THEMIS_ERROR("{}", error_message);
            return false;
        }
        
        THEMIS_INFO("Manifest signature verified: {}", manifest_path);
        return true;
        
    } catch (const std::exception& e) {
        error_message = std::string("Signature verification error: ") + e.what();
        THEMIS_ERROR("{}", error_message);
        return false;
    }
#else
    // Development mode: Signature optional, just warn if missing
    std::string sig_path = manifest_path + ".sig";
    
    if (!fs::exists(sig_path)) {
        THEMIS_WARN("Manifest signature file not found (development mode): {}", sig_path);
        return true;  // Allow in development
    }
    
    try {
        // Verify if signature exists
        std::ifstream sig_file(sig_path);
        std::string expected_hash;
        std::getline(sig_file, expected_hash);
        
        expected_hash.erase(0, expected_hash.find_first_not_of(" \t\n\r"));
        expected_hash.erase(expected_hash.find_last_not_of(" \t\n\r") + 1);
        
        std::string actual_hash = calculateFileHash(manifest_path);
        
        if (!actual_hash.empty() && expected_hash != actual_hash) {
            THEMIS_WARN("Manifest signature mismatch (development mode, allowing): {}", manifest_path);
            THEMIS_WARN("  Expected: {}", expected_hash);
            THEMIS_WARN("  Actual:   {}", actual_hash);
        } else {
            THEMIS_INFO("Manifest signature verified: {}", manifest_path);
        }
        
    } catch (const std::exception& e) {
        THEMIS_WARN("Signature verification warning (development mode): {}", e.what());
    }
    
    return true;  // Always allow in development
#endif
}

// ============================================================================
// Manifest Loading
// ============================================================================

std::optional<PluginManifest> PluginManager::loadManifest(const std::string& manifest_path) {
    if (!fs::exists(manifest_path)) {
        THEMIS_WARN("Plugin manifest not found: {}", manifest_path);
        return std::nullopt;
    }
    
    // Verify manifest signature before loading
    std::string error_message;
    if (!verifyManifestSignature(manifest_path, error_message)) {
        THEMIS_ERROR("Manifest signature verification failed for {}: {}", 
            manifest_path, error_message);
        return std::nullopt;
    }
    
    try {
        std::ifstream file(manifest_path);
        json j;
        file >> j;
        
        PluginManifest manifest;
        manifest.name = j.value("name", "");
        manifest.version = j.value("version", "1.0.0");
        manifest.description = j.value("description", "");
        
        // Parse type (string form and legacy integer form)
        if (j.contains("type") && j["type"].is_number_integer()) {
            manifest.type = static_cast<PluginType>(j["type"].get<int>());
        } else {
            std::string type_str = j.value("type", "custom");
            if (type_str == "compute_backend") {
                manifest.type = PluginType::COMPUTE_BACKEND;
            } else if (type_str == "blob_storage") {
                manifest.type = PluginType::BLOB_STORAGE;
            } else if (type_str == "importer") {
                manifest.type = PluginType::IMPORTER;
            } else if (type_str == "exporter") {
                manifest.type = PluginType::EXPORTER;
            } else if (type_str == "hsm_provider") {
                manifest.type = PluginType::HSM_PROVIDER;
            } else if (type_str == "embedding") {
                manifest.type = PluginType::EMBEDDING;
            } else {
                manifest.type = PluginType::CUSTOM;
            }
        }
        
        // Parse binaries
        if (j.contains("binary")) {
            auto& bin = j["binary"];
            manifest.binary_windows = bin.value("windows", "");
            manifest.binary_linux = bin.value("linux", "");
            manifest.binary_macos = bin.value("macos", "");
        }

        // Legacy manifest compatibility: single library field
        if (j.contains("library") && j["library"].is_string()) {
            std::string lib = j["library"].get<std::string>();
            if (manifest.binary_windows.empty()) manifest.binary_windows = lib;
            if (manifest.binary_linux.empty()) manifest.binary_linux = lib;
            if (manifest.binary_macos.empty()) manifest.binary_macos = lib;
        }
        
        // Parse dependencies
        if (j.contains("dependencies") && j["dependencies"].is_array()) {
            for (const auto& dep : j["dependencies"]) {
                manifest.dependencies.push_back(dep.get<std::string>());
            }
        }
        
        // Parse capabilities
        if (j.contains("capabilities")) {
            auto& caps = j["capabilities"];
            manifest.capabilities.supports_streaming = caps.value("streaming", false);
            manifest.capabilities.supports_batching = caps.value("batching", false);
            manifest.capabilities.supports_transactions = caps.value("transactions", false);
            manifest.capabilities.thread_safe = caps.value("thread_safe", false);
            manifest.capabilities.gpu_accelerated = caps.value("gpu_accelerated", false);
        }
        
        manifest.auto_load = j.value("auto_load", false);
        manifest.load_priority = j.value("load_priority", 100);
        
        if (j.contains("config_schema")) {
            manifest.config_schema = j["config_schema"].dump();
        }
        
        return manifest;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse plugin manifest {}: {}", manifest_path, e.what());
        return std::nullopt;
    }
}

// ============================================================================
// Plugin Discovery & Loading
// ============================================================================

Result<size_t> PluginManager::scanPluginDirectory(const std::string& directory) {
    TracedSpan span("PluginManager.scanPluginDirectory");
    span.setAttribute("plugin.directory", directory);
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        THEMIS_WARN("Plugin directory does not exist: {}", directory);
        span.setStatus(false, "Directory does not exist");
        return Err<size_t>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                           fmt::format("Plugin directory does not exist: {}", directory));
    }
    
    size_t discovered = 0;
    
    // Recursively scan for manifest JSON files
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        
        std::string filename = entry.path().filename().string();
        if (entry.path().extension() == ".json") {
            auto manifest = loadManifest(entry.path().string());
            if (!manifest && filename != "plugin.json") {
                try {
                    std::ifstream file(entry.path());
                    json j;
                    file >> j;

                    PluginManifest legacy;
                    legacy.name = j.value("name", "");
                    legacy.version = j.value("version", "1.0.0");
                    legacy.description = j.value("description", "");

                    if (j.contains("type") && j["type"].is_number_integer()) {
                        legacy.type = static_cast<PluginType>(j["type"].get<int>());
                    } else {
                        std::string type_str = j.value("type", "custom");
                        if (type_str == "compute_backend") legacy.type = PluginType::COMPUTE_BACKEND;
                        else if (type_str == "blob_storage") legacy.type = PluginType::BLOB_STORAGE;
                        else if (type_str == "importer") legacy.type = PluginType::IMPORTER;
                        else if (type_str == "exporter") legacy.type = PluginType::EXPORTER;
                        else if (type_str == "hsm_provider") legacy.type = PluginType::HSM_PROVIDER;
                        else if (type_str == "embedding") legacy.type = PluginType::EMBEDDING;
                        else legacy.type = PluginType::CUSTOM;
                    }

                    std::string lib = j.value("library", "");
                    legacy.binary_windows = lib;
                    legacy.binary_linux = lib;
                    legacy.binary_macos = lib;

                    if (!legacy.name.empty()) {
                        manifest = legacy;
                    }
                } catch (...) {
                    // Fallback parsing failed; keep manifest as nullopt
                }
            }
            if (!manifest) continue;
            
            // Determine binary path based on platform
            std::string binary_name;
#ifdef _WIN32
            binary_name = manifest->binary_windows;
#elif defined(__APPLE__)
            binary_name = manifest->binary_macos;
#else
            binary_name = manifest->binary_linux;
#endif
            
            if (binary_name.empty()) {
                THEMIS_WARN("No binary specified for current platform in manifest: {}", 
                    manifest->name);
                continue;
            }
            
            // Binary is in same directory as manifest
            fs::path binary_path = entry.path().parent_path() / binary_name;
            
            if (!fs::exists(binary_path)) {
                THEMIS_WARN("Plugin binary not found (registering manifest only): {}", binary_path.string());
            }
            
            // Register plugin
            PluginEntry plugin_entry;
            plugin_entry.name = manifest->name;
            plugin_entry.type = manifest->type;
            plugin_entry.path = binary_path.string();
            plugin_entry.manifest = *manifest;
            plugin_entry.loaded = false;
            
            plugins_[manifest->name] = std::move(plugin_entry);
            type_index_[manifest->type].push_back(manifest->name);
            
            discovered++;
            
            THEMIS_INFO("Discovered plugin: {} v{} ({})", 
                manifest->name, manifest->version, binary_path.string());
        }
    }
    
    THEMIS_INFO("Discovered {} plugins in {}", discovered, directory);
    span.setAttribute("plugin.discovered_count", static_cast<int64_t>(discovered));
    span.setStatus(true);
    return Ok(discovered);
}

Result<IThemisPlugin*> PluginManager::loadPlugin(const std::string& name) {
    TracedSpan span("PluginManager.loadPlugin");
    span.setAttribute("plugin.name", name);
    
    auto start = std::chrono::steady_clock::now();
    
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        THEMIS_ERROR("Plugin not found: {}", name);
        metrics_.recordError(name);
        span.setStatus(false, "Plugin not found");
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                    fmt::format("Plugin '{}' not found in registry", name));
    }
    
    auto& entry = it->second;
    const auto deps_to_load = entry.manifest.dependencies;
    
    if (entry.loaded && entry.instance) {
        return Ok(entry.instance.get());
    }
    
    if (!deps_to_load.empty()) {
        lock.unlock();
        for (const auto& dep : deps_to_load) {
            THEMIS_INFO("Auto-loading dependency {} for plugin {}", dep, name);
            auto dep_result = loadPlugin(dep);
            if (!dep_result) {
                lock.lock();
                metrics_.recordError(name);
                return tl::unexpected(dep_result.error());
            }
        }
        lock.lock();
        it = plugins_.find(name);
        if (it == plugins_.end()) {
            metrics_.recordError(name);
            return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                        fmt::format("Plugin '{}' not found after dependency load", name));
        }
        if (it->second.loaded && it->second.instance) {
            return Ok(it->second.instance.get());
        }
    }
    
    auto& current_entry = it->second;
    
    std::string error_message;
    if (!verifyPlugin(current_entry.path, error_message)) {
        THEMIS_ERROR("Plugin verification failed for {}: {}", name, error_message);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
                                    fmt::format("Plugin verification failed: {}", error_message));
    }
    
    void* handle = loadLibrary(current_entry.path);
    if (!handle) {
        THEMIS_ERROR("Failed to load plugin library: {}", current_entry.path);
#ifndef _WIN32
        THEMIS_ERROR("Error: {}", dlerror());
#endif
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to load plugin library from '{}'", current_entry.path));
    }
    
    auto createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "createPlugin"));
    if (!createFunc) {
        THEMIS_ERROR("Plugin does not export createPlugin: {}", current_entry.path);
        unloadLibrary(handle);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    "Plugin does not export createPlugin function");
    }
    
    IThemisPlugin* plugin = createFunc();
    if (!plugin) {
        THEMIS_ERROR("Failed to create plugin instance: {}", name);
        unloadLibrary(handle);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to create plugin instance for '{}'", name));
    }
    
    if (!plugin->initialize("{}")) {
        THEMIS_ERROR("Failed to initialize plugin: {}", name);
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(getSymbol(handle, "destroyPlugin"));
        if (destroyFunc) {
            destroyFunc(plugin);
        }
        unloadLibrary(handle);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to initialize plugin '{}'", name));
    }
    
    current_entry.library_handle = handle;
    current_entry.instance.reset(plugin);
    current_entry.loaded = true;
    current_entry.file_hash = calculateFileHash(current_entry.path);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    metrics_.recordLoad(name, duration);
    
    THEMIS_INFO("Loaded plugin: {} v{} (Hash: {}..., Load time: {}ms)",
        name, plugin->getVersion(), current_entry.file_hash.substr(0, 16), duration.count());
    
    return Ok(plugin);
}

Result<IThemisPlugin*> PluginManager::loadPluginFromPath(
    const std::string& path,
    const std::string& config
) {
    auto start = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify plugin
    std::string error_message;
    if (!verifyPlugin(path, error_message)) {
        THEMIS_ERROR("Plugin verification failed for {}: {}", path, error_message);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
                                    fmt::format("Plugin verification failed: {}", error_message));
    }
    
    // Load library
    void* handle = loadLibrary(path);
    if (!handle) {
        THEMIS_ERROR("Failed to load plugin library: {}", path);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to load plugin library from '{}'", path));
    }
    
    // Get factory function
    auto createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "createPlugin"));
    if (!createFunc) {
        THEMIS_ERROR("Plugin does not export createPlugin: {}", path);
        unloadLibrary(handle);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    "Plugin does not export createPlugin function");
    }
    
    // Create instance
    IThemisPlugin* plugin = createFunc();
    if (!plugin) {
        THEMIS_ERROR("Failed to create plugin instance from: {}", path);
        unloadLibrary(handle);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to create plugin instance from '{}'", path));
    }
    
    // Initialize with provided config
    if (!plugin->initialize(config.c_str())) {
        THEMIS_ERROR("Failed to initialize plugin from: {}", path);
        
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(getSymbol(handle, "destroyPlugin"));
        if (destroyFunc) {
            destroyFunc(plugin);
        }
        unloadLibrary(handle);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to initialize plugin from '{}'", path));
    }
    
    std::string plugin_name = plugin->getName();
    
    // Create entry for dynamically loaded plugin
    PluginEntry entry;
    entry.name = plugin_name;
    entry.type = plugin->getType();
    entry.path = path;
    entry.library_handle = handle;
    entry.instance.reset(plugin);
    entry.loaded = true;
    entry.file_hash = calculateFileHash(path);
    
    // Store
    plugins_[entry.name] = std::move(entry);
    type_index_[plugin->getType()].push_back(plugin_name);
    
    // Record load metrics
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    metrics_.recordLoad(plugin_name, duration);
    
    THEMIS_INFO("Dynamically loaded plugin: {} v{} (Load time: {}ms)", 
        plugin_name, plugin->getVersion(), duration.count());
    
    return Ok(plugin);
}

Result<void> PluginManager::unloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                       fmt::format("Plugin not found: {}", name));
    }
    
    if (!it->second.loaded) {
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                       fmt::format("Plugin not loaded: {}", name));
    }
    
    auto& entry = it->second;
    
    // Shutdown plugin
    if (entry.instance) {
        entry.instance->shutdown();
        
        // Get destroy function
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(entry.library_handle, "destroyPlugin")
        );
        
        if (destroyFunc) {
            destroyFunc(entry.instance.release());
        } else {
            entry.instance.reset();
        }
    }
    
    // Unload library
    unloadLibrary(entry.library_handle);
    
    entry.library_handle = nullptr;
    entry.loaded = false;
    
    THEMIS_INFO("Unloaded plugin: {}", name);
    return OkVoid();
}

Result<void> PluginManager::unloadAllPlugins() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : plugins_) {
        if (!pair.second.loaded) continue;
        
        auto& entry = pair.second;
        
        if (entry.instance) {
            entry.instance->shutdown();
            
            auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
                getSymbol(entry.library_handle, "destroyPlugin")
            );
            
            if (destroyFunc) {
                destroyFunc(entry.instance.release());
            } else {
                entry.instance.reset();
            }
        }
        
        unloadLibrary(entry.library_handle);
        entry.library_handle = nullptr;
        entry.loaded = false;
    }
    
    THEMIS_INFO("Unloaded all plugins");
    return OkVoid();
}

Result<IThemisPlugin*> PluginManager::getPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it != plugins_.end() && it->second.loaded) {
        return Ok(it->second.instance.get());
    }
    
    return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                fmt::format("Plugin '{}' not found or not loaded", name));
}

std::vector<IThemisPlugin*> PluginManager::getPluginsByType(PluginType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<IThemisPlugin*> result;
    
    auto it = type_index_.find(type);
    if (it != type_index_.end()) {
        for (const auto& name : it->second) {
            auto plugin_it = plugins_.find(name);
            if (plugin_it != plugins_.end() && plugin_it->second.loaded) {
                result.push_back(plugin_it->second.instance.get());
            }
        }
    }
    
    return result;
}

std::vector<PluginManifest> PluginManager::listPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PluginManifest> result;
    for (const auto& pair : plugins_) {
        result.push_back(pair.second.manifest);
    }
    
    return result;
}

std::vector<std::string> PluginManager::listLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& pair : plugins_) {
        if (pair.second.loaded) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

bool PluginManager::isPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    return it != plugins_.end() && it->second.loaded;
}

Result<void> PluginManager::reloadPlugin(const std::string& name) {
    auto start = std::chrono::steady_clock::now();
    // Unload first
    auto unload_result = unloadPlugin(name);
    if (!unload_result) {
        return tl::unexpected(unload_result.error());
    }
    
    // Then reload
    auto result = loadPlugin(name);
    bool success = result.has_value();
    
    if (success) {
        // Record reload metrics (note: loadPlugin already recorded load time)
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        metrics_.recordReload(name, duration);
    } else {
        metrics_.recordError(name);
    }
    
    if (!result) {
        metrics_.recordError(name);
        return tl::unexpected(result.error());
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    metrics_.recordReload(name, duration);
    return OkVoid();
}

Result<size_t> PluginManager::autoLoadPlugins() {
    std::vector<std::pair<int, std::string>> to_load;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [name, entry] : plugins_) {
            if (entry.manifest.auto_load) {
                to_load.emplace_back(entry.manifest.load_priority, name);
            }
        }
    }
    
    std::sort(to_load.begin(), to_load.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    
    size_t loaded = 0;
    for (const auto& [priority, name] : to_load) {
        (void)priority;
        auto result = loadPlugin(name);
        if (!result) {
            metrics_.recordError(name);
            return tl::unexpected(result.error());
        }
        ++loaded;
    }
    
    THEMIS_INFO("Auto-loaded {} plugins", loaded);
    return Ok(loaded);
}

Result<PluginManifest> PluginManager::getManifest(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return Ok(it->second.manifest);
    }
    
    return Err<PluginManifest>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                fmt::format("Plugin manifest not found: {}", name));
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
}

// Singleton
PluginManager& PluginManager::instance() {
    static PluginManager instance;
    return instance;
}

// ============================================================================
// Plugin Registry
// ============================================================================

void PluginManagerRegistry::registerFactory(
    const std::string& name,
    PluginType type,
    PluginFactory factory
) {
    auto& registry = instance();
    std::lock_guard<std::mutex> lock(registry.mutex_);
    
    registry.factories_[name] = {type, factory};
    THEMIS_INFO("Registered plugin factory: {}", name);
}

std::unique_ptr<IThemisPlugin> PluginManagerRegistry::createPlugin(const std::string& name) {
    auto& registry = instance();
    std::lock_guard<std::mutex> lock(registry.mutex_);
    
    auto it = registry.factories_.find(name);
    if (it != registry.factories_.end()) {
        return it->second.second();
    }
    
    return nullptr;
}

PluginManagerRegistry& PluginManagerRegistry::instance() {
    static PluginManagerRegistry instance;
    return instance;
}

// ============================================================================
// Hot-Plug Monitoring
// ============================================================================

bool PluginManager::enableHotPlug(const std::string& directory, const HotPlugConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (hot_plug_monitor_) {
        THEMIS_WARN("Hot-plug monitoring already enabled");
        return false;
    }
    
    hot_plug_monitor_ = std::make_unique<PluginHotPlugMonitor>(this, directory, config);
    bool started = hot_plug_monitor_->start();
    
    if (!started) {
        hot_plug_monitor_.reset();
        return false;
    }
    
    THEMIS_INFO("Hot-plug monitoring enabled for: {}", directory);
    return true;
}

void PluginManager::disableHotPlug() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!hot_plug_monitor_) {
        return;
    }
    
    hot_plug_monitor_->stop();
    hot_plug_monitor_.reset();
    
    THEMIS_INFO("Hot-plug monitoring disabled");
}

bool PluginManager::isHotPlugEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hot_plug_monitor_ != nullptr && hot_plug_monitor_->isRunning();
}

// ============================================================================
// Reload Event Listeners
// ============================================================================

void PluginManager::registerReloadListener(PluginReloadListener listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    reload_listeners_.push_back(std::move(listener));
}

void PluginManager::clearReloadListeners() {
    std::lock_guard<std::mutex> lock(mutex_);
    reload_listeners_.clear();
}

// ============================================================================
// Hot-Reload Helper Methods
// ============================================================================

std::vector<std::string> PluginManager::findDependentPlugins(const std::string& name) const {
    std::vector<std::string> dependents;
    
    for (const auto& [plugin_name, entry] : plugins_) {
        if (plugin_name == name || !entry.loaded) continue;
        
        for (const auto& dep : entry.manifest.dependencies) {
            if (dep == name) {
                dependents.push_back(plugin_name);
                break;
            }
        }
    }
    
    return dependents;
}

void PluginManager::notifyPluginReload(const std::string& name, PluginReloadPhase phase) {
    // Make a copy of listeners to avoid deadlock if listener calls back into PluginManager
    std::vector<PluginReloadListener> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_copy = reload_listeners_;
    }
    
    // Notify all listeners (outside of mutex lock)
    for (const auto& listener : listeners_copy) {
        try {
            listener(name, phase);
        } catch (const std::exception& e) {
            THEMIS_WARN("Reload listener threw exception: {}", e.what());
        }
    }
}

} // namespace plugins
} // namespace themis
