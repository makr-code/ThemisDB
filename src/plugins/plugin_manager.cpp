#include "plugins/plugin_manager.h"
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include "acceleration/plugin_security.h"
#include "utils/logger.h"
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
        
        // Parse type
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
        
        // Parse binaries
        if (j.contains("binary")) {
            auto& bin = j["binary"];
            manifest.binary_windows = bin.value("windows", "");
            manifest.binary_linux = bin.value("linux", "");
            manifest.binary_macos = bin.value("macos", "");
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
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        THEMIS_WARN("Plugin directory does not exist: {}", directory);
        return Err<size_t>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                           fmt::format("Plugin directory does not exist: {}", directory));
    }
    
    size_t discovered = 0;
    
    // Recursively scan for plugin.json files
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        
        std::string filename = entry.path().filename().string();
        if (filename == "plugin.json") {
            auto manifest = loadManifest(entry.path().string());
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
                THEMIS_WARN("Plugin binary not found: {}", binary_path.string());
                continue;
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
    return Ok(discovered);
}

Result<IThemisPlugin*> PluginManager::loadPlugin(const std::string& name) {
    auto start = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        THEMIS_ERROR("Plugin not found: {}", name);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                    fmt::format("Plugin '{}' not found in registry", name));
    }
    
    auto& entry = it->second;
    
    // Already loaded?
    if (entry.loaded && entry.instance) {
        return Ok(entry.instance.get());
    }
    
    // Load dependencies (must release lock to avoid deadlock)
    if (!deps_to_load.empty()) {
        lock.unlock();  // RAII-based unlock
        
        for (const auto& dep : deps_to_load) {
            THEMIS_INFO("Auto-loading dependency {} for plugin {}", dep, name);
            auto* dep_plugin = loadPlugin(dep);
            if (!dep_plugin) {
                THEMIS_ERROR("Failed to load dependency {} for plugin {}", dep, name);
                // Re-acquire lock before returning
                lock.lock();
                metrics_.recordError(name);
                return nullptr;
            }
        }
        
        // Re-acquire lock and verify entry is still valid
        lock.lock();  // RAII-based lock
        
        // Re-find entry as map may have been modified
        it = plugins_.find(name);
        if (it == plugins_.end()) {
            THEMIS_ERROR("Plugin {} disappeared during dependency loading", name);
            metrics_.recordError(name);
            return nullptr;
        }
        
        // If plugin was loaded while we were loading dependencies, return it
        if (it->second.loaded && it->second.instance) {
            return it->second.instance.get();
        }
    }
    
    // At this point we have the lock and all dependencies are loaded
    auto& current_entry = it->second;
    
    // Verify plugin
    std::string error_message;
    if (!verifyPlugin(current_entry.path, error_message)) {
        THEMIS_ERROR("Plugin verification failed for {}: {}", name, error_message);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
                                    fmt::format("Plugin verification failed: {}", error_message));
    }
    
    // Load library
    void* handle = loadLibrary(current_entry.path);
    if (!handle) {
        THEMIS_ERROR("Failed to load plugin library: {}", current_entry.path);
#ifndef _WIN32
        THEMIS_ERROR("Error: {}", dlerror());
#endif
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to load plugin library from '{}'", entry.path));
    }
    
    // Get factory function
    auto createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "createPlugin"));
    if (!createFunc) {
        THEMIS_ERROR("Plugin does not export createPlugin: {}", current_entry.path);
        unloadLibrary(handle);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    "Plugin does not export createPlugin function");
    }
    
    // Create instance
    IThemisPlugin* plugin = createFunc();
    if (!plugin) {
        THEMIS_ERROR("Failed to create plugin instance: {}", name);
        unloadLibrary(handle);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to create plugin instance for '{}'", name));
    }
    
    // Initialize with empty config (can be configured later)
    if (!plugin->initialize("{}")) {
        THEMIS_ERROR("Failed to initialize plugin: {}", name);
        
        // Get destroy function and cleanup
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(getSymbol(handle, "destroyPlugin"));
        if (destroyFunc) {
            destroyFunc(plugin);
        }
        unloadLibrary(handle);
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to initialize plugin '{}'", name));
    }
    
    // Store
    current_entry.library_handle = handle;
    current_entry.instance.reset(plugin);
    current_entry.loaded = true;
    current_entry.file_hash = calculateFileHash(current_entry.path);
    
    // Record load metrics
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
    
    return OkVoid();
}

Result<size_t> PluginManager::autoLoadPlugins() {
    std::vector<std::pair<int, std::string>> to_load;
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        THEMIS_ERROR("Plugin not found: {}", name);
        return false;
    }
    
    if (!it->second.loaded) {
        THEMIS_WARN("Plugin {} is not loaded, cannot reload", name);
        return false;
    }
    
    // 1. Check for dependent plugins
    std::vector<std::string> dependents = findDependentPlugins(name);
    if (!dependents.empty()) {
        THEMIS_ERROR("Cannot reload plugin {} - {} plugins depend on it: {}", 
            name, dependents.size(), 
            [&]() {
                std::string list;
                for (size_t i = 0; i < dependents.size(); ++i) {
                    if (i > 0) list += ", ";
                    list += dependents[i];
                    if (i >= 5) {  // Limit to first 5
                        list += "...";
                        break;
                    }
                }
                return list;
            }());
        return false;
    }
    
    auto& entry = it->second;
    
    // 2. Notify listeners: BEFORE_UNLOAD
    lock.unlock();
    notifyPluginReload(name, PluginReloadPhase::BEFORE_UNLOAD);
    lock.lock();
    
    // 3. Save plugin state (if supported)
    std::string saved_state;
    if (entry.instance) {
        try {
            // Try to cast to IStatefulPlugin
            auto* stateful = dynamic_cast<IStatefulPlugin*>(entry.instance.get());
            if (stateful) {
                saved_state = stateful->saveState();
                if (!saved_state.empty()) {
                    THEMIS_INFO("Saved state for plugin {} ({} bytes)", name, saved_state.size());
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to save plugin state for {}: {}", name, e.what());
            // Continue with reload even if state save fails
        }
    }
    
    // 4. Store old plugin information for rollback
    std::string old_hash = entry.file_hash;
    std::string old_path = entry.path;
    void* old_handle = entry.library_handle;
    std::unique_ptr<IThemisPlugin> old_instance = std::move(entry.instance);
    
    // 5. Unload old plugin
    if (old_instance) {
        try {
            old_instance->shutdown();
        } catch (const std::exception& e) {
            THEMIS_WARN("Exception during plugin shutdown for {}: {}", name, e.what());
        }
        
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(old_handle, "destroyPlugin")
        );
        
        if (destroyFunc) {
            try {
                destroyFunc(old_instance.release());
            } catch (const std::exception& e) {
                THEMIS_ERROR("Exception destroying plugin {}: {}", name, e.what());
            }
        } else {
            old_instance.reset();
        }
    }
    
    unloadLibrary(old_handle);
    entry.library_handle = nullptr;
    entry.instance = nullptr;
    entry.loaded = false;
    
    // 6. Notify listeners: AFTER_UNLOAD
    lock.unlock();
    notifyPluginReload(name, PluginReloadPhase::AFTER_UNLOAD);
    
    // 7. Wait briefly for OS to release file handles and complete cleanup
    std::this_thread::sleep_for(RELOAD_UNLOAD_DELAY_MS);
    lock.lock();
    
    // 8. Verify new plugin binary
    std::string error_message;
    if (!verifyPlugin(entry.path, error_message)) {
        THEMIS_ERROR("Plugin verification failed after reload for {}: {}", name, error_message);
        
        // Attempt rollback
        THEMIS_WARN("Attempting to rollback plugin {} to previous version", name);
        entry.library_handle = old_handle;
        entry.instance = std::move(old_instance);
        entry.loaded = true;
        entry.file_hash = old_hash;
        
        metrics_.recordError(name);
        return false;
    }
    
    // 9. Load new plugin binary
    void* handle = loadLibrary(entry.path);
    if (!handle) {
        THEMIS_ERROR("Failed to reload plugin library: {}", entry.path);
        
        // Attempt rollback
        THEMIS_WARN("Attempting to rollback plugin {} to previous version", name);
        entry.library_handle = old_handle;
        entry.instance = std::move(old_instance);
        entry.loaded = true;
        entry.file_hash = old_hash;
        
        metrics_.recordError(name);
        return false;
    }
    
    auto createFunc = reinterpret_cast<CreatePluginFunc>(
        getSymbol(handle, "createPlugin")
    );
    if (!createFunc) {
        THEMIS_ERROR("Plugin does not export createPlugin: {}", entry.path);
        unloadLibrary(handle);
        
        // Attempt rollback
        THEMIS_WARN("Attempting to rollback plugin {} to previous version", name);
        entry.library_handle = old_handle;
        entry.instance = std::move(old_instance);
        entry.loaded = true;
        entry.file_hash = old_hash;
        
        metrics_.recordError(name);
        return false;
    }
    
    IThemisPlugin* plugin = nullptr;
    try {
        plugin = createFunc();
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception creating plugin instance for {}: {}", name, e.what());
        unloadLibrary(handle);
        
        // Attempt rollback
        THEMIS_WARN("Attempting to rollback plugin {} to previous version", name);
        entry.library_handle = old_handle;
        entry.instance = std::move(old_instance);
        entry.loaded = true;
        entry.file_hash = old_hash;
        
        metrics_.recordError(name);
        return false;
    }
    
    if (!plugin) {
        THEMIS_ERROR("Failed to create plugin instance: {}", name);
        unloadLibrary(handle);
        
        // Attempt rollback
        THEMIS_WARN("Attempting to rollback plugin {} to previous version", name);
        entry.library_handle = old_handle;
        entry.instance = std::move(old_instance);
        entry.loaded = true;
        entry.file_hash = old_hash;
        
        metrics_.recordError(name);
        return false;
    }
    
    // 10. Initialize with configuration and state
    std::string init_config = "{}";
    if (!saved_state.empty()) {
        try {
            nlohmann::json config;
            config["restored_state"] = saved_state;
            init_config = config.dump();
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to create init config with state for {}: {}", name, e.what());
            // Continue with empty config
        }
    }
    
    bool init_success = false;
    try {
        init_success = plugin->initialize(init_config.c_str());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception initializing reloaded plugin {}: {}", name, e.what());
    }
    
    if (!init_success) {
        THEMIS_ERROR("Failed to initialize reloaded plugin: {}", name);
        
        auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(handle, "destroyPlugin")
        );
        if (destroyFunc) {
            try {
                destroyFunc(plugin);
            } catch (...) {}
        }
        unloadLibrary(handle);
        
        // Attempt rollback
        THEMIS_WARN("Attempting to rollback plugin {} to previous version", name);
        entry.library_handle = old_handle;
        entry.instance = std::move(old_instance);
        entry.loaded = true;
        entry.file_hash = old_hash;
        
        metrics_.recordError(name);
        return false;
    }
    
    // 11. Restore state if plugin supports it
    if (!saved_state.empty()) {
        try {
            auto* stateful = dynamic_cast<IStatefulPlugin*>(plugin);
            if (stateful) {
                if (stateful->restoreState(saved_state)) {
                    THEMIS_INFO("Successfully restored state for plugin {}", name);
                } else {
                    THEMIS_WARN("Failed to restore state for plugin {} - using default state", name);
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Exception restoring plugin state for {}: {}", name, e.what());
            // Continue with default state
        }
    }
    
    // 12. Update entry with new plugin
    entry.library_handle = handle;
    entry.instance.reset(plugin);
    entry.loaded = true;
    std::string new_hash = calculateFileHash(entry.path);
    entry.file_hash = new_hash;
    
    // 13. Record metrics
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    metrics_.recordReload(name, duration);
    
    // 14. Notify listeners: AFTER_LOAD
    lock.unlock();
    notifyPluginReload(name, PluginReloadPhase::AFTER_LOAD);
    
    THEMIS_INFO("Successfully reloaded plugin: {} (old hash: {}..., new hash: {}..., duration: {}ms)",
        name, 
        old_hash.substr(0, 16), 
        new_hash.substr(0, 16),
        duration.count());
    
    return true;
}

size_t PluginManager::autoLoadPlugins() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 1. Build dependency graph
    auto graph = PluginDependencyResolver::buildGraph(plugins_);
    
    // 2. Check for circular dependencies
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    if (!cycles.empty()) {
        THEMIS_ERROR("Circular dependencies detected in plugin system:");
        for (const auto& cycle : cycles) {
            std::string cycle_str;
            for (size_t i = 0; i < cycle.size(); ++i) {
                cycle_str += cycle[i];
                if (i < cycle.size() - 1) {
                    cycle_str += " -> ";
                }
            }
            THEMIS_ERROR("  Cycle: {}", cycle_str);
        }
        return 0;
    }
    
    // 3. Compute load order using topological sort
    std::vector<std::string> load_order;
    try {
        load_order = PluginDependencyResolver::computeLoadOrder(graph);
    } catch (const std::runtime_error& e) {
        THEMIS_ERROR("Failed to compute plugin load order: {}", e.what());
        return 0;
    }
    
    // 4. Load plugins in dependency order
    size_t loaded = 0;
    for (const auto& [priority, name] : to_load) {
        auto result = loadPlugin(name);
        if (result.has_value()) {
            loaded++;
        }
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

void PluginRegistry::registerFactory(
    const std::string& name,
    PluginType type,
    PluginFactory factory
) {
    auto& registry = instance();
    std::lock_guard<std::mutex> lock(registry.mutex_);
    
    registry.factories_[name] = {type, factory};
    THEMIS_INFO("Registered plugin factory: {}", name);
}

std::unique_ptr<IThemisPlugin> PluginRegistry::createPlugin(const std::string& name) {
    auto& registry = instance();
    std::lock_guard<std::mutex> lock(registry.mutex_);
    
    auto it = registry.factories_.find(name);
    if (it != registry.factories_.end()) {
        return it->second.second();
    }
    
    return nullptr;
}

PluginRegistry& PluginRegistry::instance() {
    static PluginRegistry instance;
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
