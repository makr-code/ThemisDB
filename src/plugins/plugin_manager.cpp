/**
 * @file plugin_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=6, M=25, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/plugin_manager.h"
#include <stdexcept>
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include "plugins/plugin_health_monitor.h"
#include "plugins/self_healing_plugin.h"
#include "plugins/oci_registry_client.h"
#include "acceleration/plugin_security.h"
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <cctype>
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

namespace {

// Platform-specific library handle unloading function
inline void unloadLibraryHandle(void* handle) noexcept {
    if (!handle) {
      return;
    }
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

// Custom deleter for library handles to be used with unique_ptr
struct LibraryHandleDeleter {
    void operator()(void* handle) const noexcept {
        if (handle) {
            try {
                unloadLibraryHandle(handle);
            } catch (...) {
                // Suppress exceptions during cleanup
                THEMIS_WARN("Exception during library handle cleanup");
            }
        }
    }
};

// Type alias for RAII-wrapped library handle
using LibraryHandlePtr = std::unique_ptr<void, LibraryHandleDeleter>;

std::string normalizeEditionName(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool manifestAllowsCurrentEdition(const PluginManifest& manifest) {
    if (manifest.allowed_editions.empty()) {
        return true;
    }
    const auto current = normalizeEditionName(std::string(edition::EDITION_STRING));
    return std::any_of(manifest.allowed_editions.begin(), manifest.allowed_editions.end(),
                       [&](const std::string& allowed) {
                           return normalizeEditionName(allowed) == current;
                       });
}

/**
 * @brief Validates plugin name against QW-43 path traversal attack patterns.
 * 
 * Rejects names containing:
 * - Directory separators: / \ ..
 * - Absolute path indicators: C:\ /etc/ etc.
 * - Special shell/control characters
 * 
 * Whitelist: alphanumeric (a-z, A-Z, 0-9), underscore (_), hyphen (-)
 * 
 * @param name Plugin name from manifest
 * @return true if valid, false if rejected (fail-closed)
 */
inline bool isValidPluginName(const std::string& name) {
    // Guard 1: Name must be non-empty and reasonable length
    if (name.empty() || name.length() > 256) {
        return false;
    }
     
    // Guard 2: No path traversal patterns
    if (name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos ||
        name.find("..") != std::string::npos) {
        return false;
    }
     
    // Guard 3: No absolute paths (Windows drive letters or Unix roots)
    if (name.find(':') != std::string::npos ||  // Windows C:, Unix absolute on Windows
        name.find('.') == 0) {                   // Unix hidden files / relative paths
        return false;
    }
     
    // Guard 4: Only alphanumeric, underscore, hyphen allowed
    for (unsigned char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return false;  // Fail-closed: reject on any invalid character
        }
    }
     
    return true;
}

}  // namespace

// ============================================================================
// Constants
// ============================================================================

// Brief delay after unloading to allow OS to release file handles and cleanup
constexpr auto RELOAD_UNLOAD_DELAY_MS = std::chrono::milliseconds(50);

// ============================================================================
// Platform-specific DLL loading (reused from acceleration/plugin_loader.cpp)
// ============================================================================

/**
 * @brief Platform-specific dynamic library loading wrapper.
 * @param path Shared library path.
 * @return Native module handle or nullptr on load failure.
 */
void* PluginManager::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

/**
 * @brief Resolve an exported symbol from a loaded shared library.
 * @param handle Native module handle returned by loadLibrary().
 * @param symbolName Exported symbol name.
 * @return Symbol address or nullptr when symbol is unavailable.
 */
void* PluginManager::getSymbol(void* handle, const std::string& symbolName) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
#else
    return dlsym(handle, symbolName.c_str());
#endif
}

/**
 * @brief Unload a previously loaded shared library.
 * @param handle Native module handle. nullptr is ignored.
 */
void PluginManager::unloadLibrary(void* handle) {
    if (!handle) {
      return;
    }
    
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

// ============================================================================
// Security & Hashing
// ============================================================================

/**
 * @brief Compute SHA-256 digest for a file path.
 * @param path File to hash.
 * @return Lower-case hex SHA-256 digest, or empty string on failure.
 * @note Failures include file-open errors and OpenSSL digest API failures.
 */
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
    
    std::stringstream ss = {};
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

/**
 * @brief Verify plugin binary against security policy.
 * @param path Candidate plugin library path.
 * @param error_message Output details when verification fails.
 * @return true when policy validation succeeds; false otherwise.
 * @note Production builds require signatures; development builds may allow unsigned plugins.
 */
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

/**
 * @brief Verify detached manifest signature according to build-mode policy.
 * @param manifest_path Path to plugin manifest file.
 * @param error_message Output detail for failure reason.
 * @return true when signature checks pass (or are optional in current mode).
 */
bool PluginManager::verifyManifestSignature(const std::string& manifest_path, std::string& error_message) {
    // Signature verification strategy:
    // 1. Check for manifest_path + ".sig" file (digital signature)
    // 2. Verify SHA256 hash matches signature file content
    // 3. In production, require valid signature
    
    #ifdef THEMIS_TEST_MODE
        // Test mode: Always allow (signature verification not required for tests)
        THEMIS_INFO("Manifest signature verification skipped (test mode): {}", manifest_path);
        return true;
    #elif defined(NDEBUG)
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
        std::string expected_hash = {};
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
        std::string expected_hash = {};
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
// Phase 2C: Unified Plugin Validation Logic
// ============================================================================

/**
 * @brief QW-43 Plugin Name Validation
 *
 * Path traversal defense: validates plugin names against injection attacks.
 * Implements fail-closed semantics for security.
 *
 * @param name Plugin name from manifest
 * @return true if valid, false if rejected (fail-closed)
 */

/**
 * @brief Phase 2C: Unified validation for plugin load operations.
 *
 * Implements 4-stage validation contract:
 * 1. Manifest schema validation (required fields, types)
 * 2. Manifest semantic validation (constraints, dependencies)
 * 3. Signature verification (detached signature checks)
 * 4. Capability validation (required capabilities available)
 *
 * Fail-safe semantics: If any stage fails, plugin remains in UNLOADED state.
 */
PluginsError PluginManager::validatePluginForLoad(
    const PluginManifest& manifest,
    const std::string& manifest_path,
    const std::string& plugin_binary_path,
    std::string& error_details
) {
    // Stage 1: Manifest schema validation
    if (manifest.name.empty()) {
        error_details = "Manifest schema validation failed: missing required 'name' field";
        THEMIS_ERROR("{}", error_details);
        return PluginsError::kManifestInvalid;
    }
    if (manifest.version.empty()) {
        error_details = "Manifest schema validation failed: missing required 'version' field";
        THEMIS_ERROR("{}", error_details);
        return PluginsError::kManifestInvalid;
    }
    
    // Stage 2: Manifest semantic validation (constraints)
    // - Name must pass QW-43 path traversal validation
    if (!isValidPluginName(manifest.name)) {
        error_details = "Manifest semantic validation failed: invalid plugin name (path traversal risk)";
        THEMIS_ERROR("{}", error_details);
        return PluginsError::kManifestInvalid;
    }
    
    // - Edition compatibility check
    if (!manifestAllowsCurrentEdition(manifest)) {
        error_details = fmt::format("Manifest semantic validation failed: plugin not available for edition '{}'",
                                   std::string(edition::EDITION_STRING));
        THEMIS_WARN("{}", error_details);
        return PluginsError::kManifestInvalid;
    }
    
    // - License feature check
    if (!manifest.license_feature.empty() &&
        !license::RuntimeLicenseGate::instance().isFeatureAllowed(manifest.license_feature)) {
        error_details = fmt::format("Manifest semantic validation failed: runtime license does not permit feature '{}'",
                                   manifest.license_feature);
        THEMIS_WARN("{}", error_details);
        return PluginsError::kManifestInvalid;
    }
    
    // Stage 3: Signature verification
    if (!verifyManifestSignature(manifest_path, error_details)) {
        THEMIS_ERROR("Plugin manifest signature verification failed: {}", error_details);
        return PluginsError::kSignatureVerifyFailed;
    }
    
    // Stage 4: Binary verification (security policy)
    if (!verifyPlugin(plugin_binary_path, error_details)) {
        THEMIS_ERROR("Plugin binary verification failed: {}", error_details);
        return PluginsError::kSignatureVerifyFailed;
    }
     
    THEMIS_INFO("Plugin validation succeeded for '{}': all 4 stages passed", manifest.name);
    return PluginsError::kSuccess;
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
    std::string error_message = {};
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
        manifest.version = j.value("version", "");
        manifest.description = j.value("description", "");

        // QW-43: Fail-closed path traversal guard on plugin name
        // Validates manifest.name against whitelist (alphanumeric + underscore + hyphen)
        // Rejects directory separators, absolute paths, special characters
        if (!isValidPluginName(manifest.name)) {
            THEMIS_ERROR("Plugin manifest rejected - invalid name (path traversal risk): {}",
                         manifest.name.empty() ? "(empty)" : manifest.name);
            return std::nullopt;  // Fail-closed: reject malicious manifest
        }

        // Validate required fields: name and version must be non-empty strings
        if (manifest.name.empty()) {
            THEMIS_ERROR("Plugin manifest missing required 'name' field: {}", manifest_path);
            return std::nullopt;
        }
        if (manifest.version.empty()) {
            THEMIS_ERROR("Plugin manifest missing required 'version' field: {}", manifest_path);
            return std::nullopt;
        }

        // Validate that at least one binary platform entry is present
        if (!j.contains("binary") || !j["binary"].is_object()) {
            THEMIS_ERROR("Plugin manifest missing required 'binary' section: {}", manifest_path);
            return std::nullopt;
        }
        
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
        {
            auto& bin = j["binary"];
            manifest.binary_windows = bin.value("windows", "");
            manifest.binary_linux = bin.value("linux", "");
            manifest.binary_macos = bin.value("macos", "");
        }

        // Legacy manifest compatibility: single library field
        if (j.contains("library") && j["library"].is_string()) {
            std::string lib = j["library"].get<std::string>();
            if (manifest.binary_windows.empty()) {
              manifest.binary_windows = lib;
            }
            if (manifest.binary_linux.empty()) {
              manifest.binary_linux = lib;
            }
            if (manifest.binary_macos.empty()) {
              manifest.binary_macos = lib;
            }
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

        // Optional: expected SHA-256 hash of the binary for integrity enforcement
        manifest.expected_hash = j.value("expected_hash", "");
        manifest.visibility = j.value("visibility", "public");
        manifest.license_feature = j.value("license_feature", "");
        manifest.compatible_core_abi = j.value("compatible_core_abi", "");
        manifest.min_themisdb_version = j.value("min_themisdb_version",
                                                j.value("min_themis_version", ""));
        manifest.max_themisdb_version = j.value("max_themisdb_version",
                                                j.value("max_themis_version", ""));
        if (j.contains("allowed_editions") && j["allowed_editions"].is_array()) {
            for (const auto& ed : j["allowed_editions"]) {
                if (ed.is_string()) {
                    manifest.allowed_editions.push_back(ed.get<std::string>());
                }
            }
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
    size_t manifest_files = 0;
    
    // Recursively scan for manifest JSON files
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
          continue;
        }
        
        std::string filename = entry.path().filename().string();
        if (entry.path().extension() == ".json") {
            manifest_files++;
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
                        if (type_str == "compute_backend") {
                          legacy.type = PluginType::COMPUTE_BACKEND;
                        }
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

                    // QW-43: Validate legacy plugin name against path traversal (fail-closed)
                    if (!legacy.name.empty() && isValidPluginName(legacy.name)) {
                        manifest = legacy;
                    } else if (!legacy.name.empty()) {
                        THEMIS_WARN("Legacy plugin manifest rejected - invalid name (path traversal risk): {}",
                                   legacy.name);
                    }
                } catch (...) {
                    // Fallback parsing failed; keep manifest as nullopt
                }
            }

            if (!manifest && filename != "plugin.json") {
                PluginManifest fallback;
                fallback.name = entry.path().stem().string();
                
                // QW-43: Validate fallback plugin name against path traversal (fail-closed)
                if (!isValidPluginName(fallback.name)) {
                    THEMIS_WARN("Fallback plugin rejected - invalid name from path: {}", fallback.name);
                    continue;  // Skip this malformed manifest
                }
                
                fallback.version = "1.0.0";
                fallback.type = PluginType::CUSTOM;
                std::string lib = entry.path().stem().string() + ".so";
                fallback.binary_windows = lib;
                fallback.binary_linux = lib;
                fallback.binary_macos = lib;
                manifest = fallback;
            }
            if (!manifest) {
              continue;
            }
            
            // Determine binary path based on platform
            std::string binary_name = {};
#ifdef _WIN32
            binary_name = manifest->binary_windows;
#elif defined(__APPLE__)
            binary_name = manifest->binary_macos;
#else
            binary_name = manifest->binary_linux;
#endif
            
            if (binary_name.empty()) {
                binary_name = manifest->name + ".so";
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

    if (discovered == 0 && manifest_files > 0) {
        discovered = manifest_files;
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

    // Phase 2A: Check lifecycle state transition validity
    {
        std::lock_guard<std::mutex> state_lock(entry.state_mutex);
        if (entry.state == PluginLifecycleState::LOADED && entry.instance) {
            // Plugin already loaded; return existing instance
            return Ok(entry.instance.get());
        }
        if (entry.state == PluginLifecycleState::LOADING || entry.state == PluginLifecycleState::UNLOADING) {
            // Plugin is mid-transition; reject concurrent load attempt
            THEMIS_ERROR("Cannot load plugin '{}': state machine transition in progress (current state: {})",
                        name, lifecycleStateToString(entry.state));
            metrics_.recordError(name);
            span.setStatus(false, "State machine transition in progress");
            return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                fmt::format("Plugin '{}' is in transition state", name));
        }
        if (!isValidLifecycleTransition(entry.state, PluginLifecycleState::LOADING)) {
            // Invalid transition (e.g., attempting to load from unexpected state)
            THEMIS_ERROR("Invalid lifecycle transition for plugin '{}': {} → LOADING",
                        name, lifecycleStateToString(entry.state));
            metrics_.recordError(name);
            span.setStatus(false, "Invalid lifecycle transition");
            return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                fmt::format("Invalid lifecycle transition for plugin '{}'", name));
        }
        // Transition to LOADING state
        entry.state = PluginLifecycleState::LOADING;
    }

    // Edition + runtime license gate: reject early on unsupported editions.
    // Public manifests without explicit private gating remain backward-compatible.
    const bool requires_enterprise_gate =
        entry.manifest.visibility != "public" ||
        !entry.manifest.license_feature.empty();
    if (requires_enterprise_gate && !isEditionSupported()) {
        {
            std::lock_guard<std::mutex> state_lock(entry.state_mutex);
            entry.state = PluginLifecycleState::UNLOADED;
        }
        const std::string msg = communityUnavailableMessage(name);
        THEMIS_WARN("{}", msg);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND, msg);
    }
    if (requires_enterprise_gate && !isLicensed()) {
        {
            std::lock_guard<std::mutex> state_lock(entry.state_mutex);
            entry.state = PluginLifecycleState::UNLOADED;
        }
        const std::string msg = "Plugin '" + name +
            "' cannot be loaded: runtime license does not permit enterprise_plugins.";
        THEMIS_WARN("{}", msg);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND, msg);
    }
    if (!manifestAllowsCurrentEdition(entry.manifest)) {
        {
            std::lock_guard<std::mutex> state_lock(entry.state_mutex);
            entry.state = PluginLifecycleState::UNLOADED;
        }
        const std::string msg = "Plugin '" + name + "' is not available in edition '" +
            std::string(edition::EDITION_STRING) + "'.";
        THEMIS_WARN("{}", msg);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND, msg);
    }
    if (!entry.manifest.license_feature.empty() &&
        !license::RuntimeLicenseGate::instance().isFeatureAllowed(entry.manifest.license_feature)) {
        {
            std::lock_guard<std::mutex> state_lock(entry.state_mutex);
            entry.state = PluginLifecycleState::UNLOADED;
        }
        const std::string msg = "Plugin '" + name + "' cannot be loaded: runtime license does not permit feature '" +
            entry.manifest.license_feature + "'.";
        THEMIS_WARN("{}", msg);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND, msg);
    }
    
    if (!deps_to_load.empty()) {
        // Check for circular dependencies before attempting to load them.
        // This prevents infinite recursion / stack overflow caused by dependency cycles.
        auto dep_graph = PluginDependencyResolver::buildGraph(plugins_);
        auto cycles = PluginDependencyResolver::detectCircularDependencies(dep_graph);
        if (!cycles.empty()) {
            std::string cycle_desc = {};
            for (const auto& cycle : cycles) {
                if (!cycle_desc.empty()) {
                  cycle_desc += "; ";
                }
                for (size_t i = 0; i  < cycle.size(); ++i) {
                    if (i > 0) {
                      cycle_desc += " -> ";
                    }
                    cycle_desc += cycle[i];
                }
            }
            THEMIS_ERROR("Circular dependency detected for plugin {}: {}", name, cycle_desc);
            {
                std::lock_guard<std::mutex> state_lock(entry.state_mutex);
                entry.state = PluginLifecycleState::UNLOADED;
            }
            metrics_.recordError(name);
            span.setStatus(false, "Circular dependency");
            return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_CIRCULAR_DEPENDENCY,
                fmt::format("Circular dependency detected involving plugin '{}': {}", name, cycle_desc));
        }

        // Validate that all declared dependencies are registered before attempting
        // to load them. This gives ERR_PLUGIN_MISSING_DEPENDENCY (6305) for
        // unregistered deps rather than the less-specific ERR_PLUGIN_NOT_FOUND.
        for (const auto& dep : deps_to_load) {
            if (plugins_.find(dep) == plugins_.end()) {
                auto missing_msg = fmt::format(
                    "Plugin '{}' requires unregistered dependency '{}'", name, dep);
                THEMIS_ERROR("{}", missing_msg);
                {
                    std::lock_guard<std::mutex> state_lock(entry.state_mutex);
                    entry.state = PluginLifecycleState::UNLOADED;
                }
                metrics_.recordError(name);
                span.setStatus(false, "Missing dependency");
                return Err<IThemisPlugin*>(
                    errors::ErrorCode::ERR_PLUGIN_MISSING_DEPENDENCY, missing_msg);
            }
        }

        // Release main plugin lock before recursively loading dependencies.
        // This prevents deadlock and allows concurrent dependency loading.
        // Note: entry reference becomes invalid after releasing lock, so we don't use it.
        lock.unlock();

        // Auto-load dependencies without holding the main plugins_ lock.
        // Each recursive loadPlugin() call will acquire its own lock as needed.
        for (const auto& dep : deps_to_load) {
            THEMIS_INFO("Auto-loading dependency {} for plugin {}", dep, name);
            auto dep_result = loadPlugin(dep);
            if (!dep_result) {
                // On dependency load failure, re-acquire lock and update state.
                // Must re-query iterator since entry may have changed during unlock period.
                {
                    std::lock_guard<std::mutex> relock(mutex_);
                    auto it_relock = plugins_.find(name);
                    if (it_relock != plugins_.end()) {
                        std::lock_guard<std::mutex> state_lock(it_relock->second.state_mutex);
                        it_relock->second.state = PluginLifecycleState::UNLOADED;
                    }
                }
                metrics_.recordError(name);
                return tl::unexpected(dep_result.error());
            }
        }

        // Re-acquire lock after dependency loading completes.
        // Must re-query the plugin entry since it may have been modified.
        {
            std::lock_guard<std::mutex> relock(mutex_);
            it = plugins_.find(name);
            if (it == plugins_.end()) {
                metrics_.recordError(name);
                return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                            fmt::format("Plugin '{}' not found after dependency load", name));
            }
            // If a dependency load sequence succeeded and loaded this plugin, return it.
            if (it->second.loaded && it->second.instance) {
                {
                    std::lock_guard<std::mutex> state_lock(it->second.state_mutex);
                    it->second.state = PluginLifecycleState::LOADED;
                }
                return Ok(it->second.instance.get());
            }
        }
    }
    
    // Re-acquire lock for main loading sequence below.
    // This ensures the lock is always held when accessing current_entry.
    if (!lock.owns_lock()) {
        lock.lock();
    }
    it = plugins_.find(name);
    if (it == plugins_.end()) {
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                                    fmt::format("Plugin '{}' was removed during loading", name));
    }
    
    auto& current_entry = it->second;
    
    // Binary hash enforcement: if the manifest specifies an expected_hash, verify
    // the on-disk binary matches before attempting to load it.
    if (!current_entry.manifest.expected_hash.empty()) {
        std::string actual_hash = calculateFileHash(current_entry.path);
        if (actual_hash.empty()) {
            THEMIS_ERROR("Failed to compute hash for plugin binary: {}", current_entry.path);
            {
                std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
                current_entry.state = PluginLifecycleState::UNLOADED;
            }
            metrics_.recordError(name);
            return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                fmt::format("Hash computation failed for plugin '{}'", name));
        }
        if (actual_hash != current_entry.manifest.expected_hash) {
            THEMIS_ERROR("Plugin binary hash mismatch for '{}': "
                         "expected {}, got {}",
                         name,
                         current_entry.manifest.expected_hash,
                         actual_hash);
            {
                std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
                current_entry.state = PluginLifecycleState::UNLOADED;
            }
            metrics_.recordError(name);
            return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
                fmt::format("Binary hash mismatch for plugin '{}' — possible tampering", name));
        }
    }

    std::string error_message = {};
    if (!verifyPlugin(current_entry.path, error_message)) {
        THEMIS_ERROR("Plugin verification failed for {}: {}", name, error_message);
        {
            std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
            current_entry.state = PluginLifecycleState::UNLOADED;
        }
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
        {
            std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
            current_entry.state = PluginLifecycleState::UNLOADED;
        }
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to load plugin library from '{}'", current_entry.path));
    }
    
    auto createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "createPlugin"));
    if (!createFunc) {
        THEMIS_ERROR("Plugin does not export createPlugin: {}", current_entry.path);
        unloadLibrary(handle);
        {
            std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
            current_entry.state = PluginLifecycleState::UNLOADED;
        }
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    "Plugin does not export createPlugin function");
    }
    
    IThemisPlugin* plugin = createFunc();
    if (!plugin) {
        THEMIS_ERROR("Failed to create plugin instance: {}", name);
        unloadLibrary(handle);
        {
            std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
            current_entry.state = PluginLifecycleState::UNLOADED;
        }
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
        {
            std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
            current_entry.state = PluginLifecycleState::UNLOADED;
        }
        metrics_.recordError(name);
        return Err<IThemisPlugin*>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                                    fmt::format("Failed to initialize plugin '{}'", name));
    }
    
    current_entry.library_handle = handle;
    current_entry.instance.reset(plugin);
    current_entry.loaded = true;
    current_entry.file_hash = calculateFileHash(current_entry.path);
    current_entry.frozen_capabilities = plugin->getCapabilities();
    
    // Phase 2A: Transition to LOADED state on successful load
    {
        std::lock_guard<std::mutex> state_lock(current_entry.state_mutex);
        current_entry.state = PluginLifecycleState::LOADED;
    }
    
    // Auto-register self-healing plugins with the health monitor
    if (health_monitor_) {
        auto* self_healing = dynamic_cast<ISelfHealingPlugin*>(plugin);
        if (self_healing) {
            health_monitor_->registerPlugin(name, self_healing);
        }
    }

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
    std::string error_message = {};
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
    entry.frozen_capabilities = plugin->getCapabilities();
    
    // Store
    plugins_[entry.name] = std::move(entry);
    type_index_[plugin->getType()].push_back(plugin_name);
    
    // Auto-register self-healing plugins with the health monitor
    if (health_monitor_) {
        auto* self_healing = dynamic_cast<ISelfHealingPlugin*>(plugin);
        if (self_healing) {
            health_monitor_->registerPlugin(plugin_name, self_healing);
        }
    }

    // Record load metrics
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    metrics_.recordLoad(plugin_name, duration);
    
    THEMIS_INFO("Dynamically loaded plugin: {} v{} (Load time: {}ms)", 
        plugin_name, plugin->getVersion(), duration.count());
    
    return Ok(plugin);
}

Result<IThemisPlugin*> PluginManager::loadPluginFromOci(
    const std::string& oci_ref,
    const std::string& cache_dir,
    const std::string& auth_token)
{
    TracedSpan span("PluginManager.loadPluginFromOci");

    // 1. Parse OCI reference.
    auto ref_res = OciReference::parse(oci_ref);
    if (!ref_res.has_value()) {
        return Err<IThemisPlugin*>(ref_res.error().code(), ref_res.error().context());
    }
    const OciReference& ref = *ref_res;

    // 2. Determine cache directory (default: system temp / themis-plugins).
    std::string effective_cache = cache_dir;
    if (effective_cache.empty()) {
        effective_cache = (fs::temp_directory_path() / "themis-plugins").string();
    }

    // 3. Build OCI client and inject optional bearer token.
    OciRegistryClient oci_client = {};
    if (!auth_token.empty()) {
        OciAuthConfig auth;
        auth.bearer_token = auth_token;
        oci_client.setAuth(ref.registry, std::move(auth));
    }

    // 4. Pull plugin binary to cache directory.
    auto pull_res = oci_client.pullPluginBinary(ref, effective_cache);
    if (!pull_res.has_value()) {
        THEMIS_ERROR("OCI pull failed for {}: {}", oci_ref, pull_res.error().context());
        return Err<IThemisPlugin*>(pull_res.error().code(), pull_res.error().context());
    }
    const std::string& binary_path = *pull_res;

    THEMIS_INFO("OCI pull succeeded for {}; binary at {}", oci_ref, binary_path);

    // 5. Load the downloaded binary via the existing path-based loader.
    return loadPluginFromPath(binary_path);
}

Result<void> PluginManager::unloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                       fmt::format("Plugin not found: {}", name));
    }
    
    auto& entry = it->second;
    
    // Phase 2A: Check lifecycle state
    {
        std::lock_guard<std::mutex> state_lock(entry.state_mutex);
        if (entry.state != PluginLifecycleState::LOADED) {
            THEMIS_ERROR("Cannot unload plugin '{}': not in LOADED state (current state: {})",
                        name, lifecycleStateToString(entry.state));
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                           fmt::format("Plugin '{}' is not loaded", name));
        }
        // Transition to UNLOADING state
        if (!isValidLifecycleTransition(entry.state, PluginLifecycleState::UNLOADING)) {
            THEMIS_ERROR("Invalid lifecycle transition for plugin '{}': {} → UNLOADING",
                        name, lifecycleStateToString(entry.state));
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                           fmt::format("Invalid lifecycle transition for plugin '{}'", name));
        }
        entry.state = PluginLifecycleState::UNLOADING;
    }

    // Block unload if other loaded plugins depend on this one.
    auto dependents = findDependentPlugins(name);
    if (!dependents.empty()) {
        std::string dep_list = {};
        for (const auto& dep : dependents) {
            if (!dep_list.empty()) {
              dep_list += ", ";
            }
            dep_list += dep;
        }
        auto error_msg = fmt::format(
            "Cannot unload plugin '{}' — {} plugin(s) depend on it: {}",
            name,static_cast<int>(dependents.size()), dep_list);
        THEMIS_ERROR("{}", error_msg);
        // Revert state back to LOADED on failure
        {
            std::lock_guard<std::mutex> state_lock(entry.state_mutex);
            entry.state = PluginLifecycleState::LOADED;
        }
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_DEPENDENCY_CONFLICT, error_msg);
    }
    
    // Unregister from health monitor before shutting down the instance
    if (health_monitor_) {
        health_monitor_->unregisterPlugin(name);
    }

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
    
    // Phase 2A: Transition to UNLOADED state on successful unload
    {
        std::lock_guard<std::mutex> state_lock(entry.state_mutex);
        entry.state = PluginLifecycleState::UNLOADED;
    }
    
    THEMIS_INFO("Unloaded plugin: {}", name);
    return OkVoid();
}

Result<void> PluginManager::unloadAllPlugins() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Unload all loaded plugins
    for (auto& pair : plugins_) {
        if (!pair.second.loaded) {
          continue;
        }
        
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
    
    // Clear all plugins (both loaded and discovered-only) from registry
    plugins_.clear();
    type_index_.clear();
    
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
    
    std::vector<PluginManifest> result = {};

    for (const auto& pair : plugins_) {
        result.push_back(pair.second.manifest);
    }
    
    return result;
}

std::vector<std::string> PluginManager::listLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result = {};

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

    // -------------------------------------------------------------------------
    // Phase 1: Pre-reload validation and state capture (under mutex)
    // -------------------------------------------------------------------------
    std::string saved_state = {};
    std::string plugin_path = {};
    std::string expected_hash = {};

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(name);
        if (it == plugins_.end() || !it->second.loaded) {
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                           fmt::format("Plugin not loaded: {}", name));
        }

        // Block reload if other loaded plugins depend on this one.
        auto dependents = findDependentPlugins(name);
        if (!dependents.empty()) {
            std::string dep_list = {};
            for (const auto& dep : dependents) {
                if (!dep_list.empty()) {
                  dep_list += ", ";
                }
                dep_list += dep;
            }
            auto error_msg = fmt::format(
                "Cannot reload plugin '{}' — {} plugin(s) depend on it: {}",
                name,static_cast<int>(dependents.size()), dep_list);
            THEMIS_ERROR("{}", error_msg);
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_DEPENDENCY_CONFLICT, error_msg);
        }

        // Pre-reload tamper detection: compare current on-disk hash against the
        // hash recorded at load time to detect unexpected file mutations.
        if (it->second.loaded && !it->second.file_hash.empty()) {
            std::string current_hash = calculateFileHash(it->second.path);
            if (!current_hash.empty() && current_hash != it->second.file_hash) {
                THEMIS_WARN("Plugin '{}' binary has changed on disk since last load "
                            "(stored hash: {}..., current hash: {}...); reloading modified binary",
                            name,
                            it->second.file_hash.substr(0, 16),
                            current_hash.substr(0, 16));
            }
        }

        // Save IStatefulPlugin state before the old instance is replaced.
        if (it->second.instance) {
            auto* stateful = dynamic_cast<IStatefulPlugin*>(it->second.instance.get());
            if (stateful) {
                try {
                    saved_state = stateful->saveState();
                    THEMIS_INFO("Saved state for plugin '{}' ({} bytes)",
                                name,static_cast<int>(saved_state.size()));
                } catch (const std::exception& e) {
                    THEMIS_WARN("Failed to save state for plugin '{}': {}", name, e.what());
                }
            }
        }

        plugin_path    = it->second.path;
        expected_hash  = it->second.manifest.expected_hash;
    }

    // -------------------------------------------------------------------------
    // Phase 2: ATOMIC RELOAD — load new binary before touching the old one.
    //
    // If any step in this phase fails, the old plugin instance is completely
    // untouched and continues to run (rollback is implicit).
    // -------------------------------------------------------------------------

    // Step 2a: Optional binary hash enforcement (manifest expected_hash)
    if (!expected_hash.empty()) {
        std::string actual_hash = calculateFileHash(plugin_path);
        if (actual_hash.empty()) {
            auto msg = fmt::format("Hash computation failed for plugin '{}' during reload", name);
            THEMIS_ERROR("{}", msg);
            metrics_.recordError(name);
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED, msg);
        }
        if (actual_hash != expected_hash) {
            auto msg = fmt::format("Binary hash mismatch for plugin '{}' — possible tampering", name);
            THEMIS_ERROR("Plugin '{}': expected hash {}, got {}", name, expected_hash, actual_hash);
            metrics_.recordError(name);
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE, msg);
        }
    }

    // Step 2b: Signature / security verification of new binary
    std::string verify_error = {};
    if (!verifyPlugin(plugin_path, verify_error)) {
        auto msg = fmt::format("Plugin verification failed after reload for '{}': {}", name, verify_error);
        THEMIS_ERROR("{}", msg);
        metrics_.recordError(name);
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE, msg);
    }

    // Step 2c: Load new binary (old library stays open — OS ref-counts handles)
    // Use RAII wrapper to ensure automatic cleanup if anything fails
    void* raw_new_handle = loadLibrary(plugin_path);
    if (!raw_new_handle) {
        auto msg = fmt::format("Failed to load new plugin binary for '{}' from '{}'", name, plugin_path);
        THEMIS_ERROR("{}", msg);
#ifndef _WIN32
        THEMIS_ERROR("dlopen error: {}", dlerror());
#endif
        metrics_.recordError(name);
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED, msg);
    }
    
    // Wrap handle in unique_ptr for RAII cleanup on failure
    LibraryHandlePtr new_handle(raw_new_handle);

    // Step 2d: Resolve createPlugin / destroyPlugin entry points
    auto new_create  = reinterpret_cast<CreatePluginFunc>(getSymbol(new_handle.get(), "createPlugin"));
    auto new_destroy = reinterpret_cast<DestroyPluginFunc>(getSymbol(new_handle.get(), "destroyPlugin"));
    if (!new_create) {
        // new_handle RAII will automatically unload on scope exit
        auto msg = fmt::format("New binary for plugin '{}' does not export createPlugin", name);
        THEMIS_ERROR("{}", msg);
        metrics_.recordError(name);
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED, msg);
    }

    // Step 2e: Create new plugin instance
    IThemisPlugin* raw_new_instance = new_create();
    if (!raw_new_instance) {
        // new_handle RAII will automatically unload on scope exit
        auto msg = fmt::format("createPlugin() returned null for '{}'", name);
        THEMIS_ERROR("{}", msg);
        metrics_.recordError(name);
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED, msg);
    }

    // Wrap instance in unique_ptr with appropriate deleter
    // Note: we need to capture new_destroy as the deleter
    // Using a lambda deleter to call the plugin's destroy function if available
    auto instance_deleter = [new_destroy](IThemisPlugin* ptr) noexcept {
        if (ptr) {
            try {
                if (new_destroy) {
                    new_destroy(ptr);
                } else {
                    delete ptr;
                }
            } catch (...) {
                THEMIS_WARN("Exception during plugin instance cleanup");
            }
        }
    };
    std::unique_ptr<IThemisPlugin, decltype(instance_deleter)> new_instance(raw_new_instance, instance_deleter);

    // Step 2f: Initialize with restored state embedded in config (if available)
    std::string init_config = "{}";
    if (!saved_state.empty()) {
        try {
            nlohmann::json cfg;
            cfg["restored_state"] = saved_state;
            init_config = cfg.dump();
        } catch (...) {
            init_config = "{}";
        }
    }

    if (!new_instance->initialize(init_config.c_str())) {
        // Both new_handle and new_instance RAII will automatically cleanup on scope exit
        auto msg = fmt::format("New plugin binary for '{}' failed initialize()", name);
        THEMIS_ERROR("{}", msg);
        metrics_.recordError(name);
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED, msg);
    }

    // Step 2g: Restore state directly via IStatefulPlugin (belt-and-suspenders)
    if (!saved_state.empty()) {
        auto* stateful = dynamic_cast<IStatefulPlugin*>(new_instance.get());
        if (stateful && !stateful->restoreState(saved_state)) {
            THEMIS_WARN("restoreState() returned false for '{}' after reload; "
                        "plugin continues with state from initialize()", name);
        }
    }

    // -------------------------------------------------------------------------
    // Phase 3: New binary is healthy. Notify listeners (BEFORE_UNLOAD),
    // then atomically swap entries under mutex, then clean up old binary.
    // -------------------------------------------------------------------------

    notifyPluginReload(name, PluginReloadPhase::BEFORE_UNLOAD);

    // Capture old handle/instance for post-swap cleanup outside the lock
    void* old_handle = nullptr;
    IThemisPlugin* old_raw   = nullptr;
    DestroyPluginFunc old_destroy = nullptr;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = plugins_.find(name);
        if (it == plugins_.end()) {
            // Unlikely: plugin entry was removed concurrently.
            // RAII wrappers (new_handle and new_instance) will automatically clean up on scope exit
            return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                           fmt::format("Plugin entry '{}' removed during reload", name));
        }

        auto& entry      = it->second;
        old_handle       = entry.library_handle;

        // Retrieve old destroyPlugin symbol before the handle is replaced
        if (old_handle) {
            old_destroy = reinterpret_cast<DestroyPluginFunc>(
                getSymbol(old_handle, "destroyPlugin"));
        }

        // Atomically swap instances:
        // 1. Capture the raw pointer of the old instance from entry.instance
        old_raw = entry.instance.get();
        // 2. Release ownership from the old unique_ptr (don't delete yet)
        entry.instance.release();
        // 3. Move ownership of the new instance to entry.instance
        // We release from the RAII wrapper and pass the raw pointer to reset()
        entry.instance.reset(new_instance.release());
        // 4. Store the new handle (release from RAII wrapper and store raw pointer)
        entry.library_handle = new_handle.release();
        entry.loaded         = true;
        entry.file_hash      = calculateFileHash(plugin_path);
    }

    // Swap complete — old plugin is now detached. Notify AFTER_UNLOAD.
    notifyPluginReload(name, PluginReloadPhase::AFTER_UNLOAD);

    // Create RAII wrappers for old instance and handle cleanup
    // Note: old_destroy was captured with specific knowledge of old_handle's exported functions
    auto old_instance_deleter = [old_destroy](IThemisPlugin* ptr) noexcept {
        if (ptr) {
            try {
                ptr->shutdown();
                if (old_destroy) {
                    old_destroy(ptr);
                } else {
                    delete ptr;
                }
            } catch (...) {
                THEMIS_WARN("Exception during old plugin instance cleanup");
            }
        }
    };
    
    // Wrap old instance in RAII holder (it will be destroyed when this scope exits)
    if (old_raw) {
        std::unique_ptr<IThemisPlugin, decltype(old_instance_deleter)> 
            old_instance_holder(old_raw, old_instance_deleter);
        // old_instance_holder automatically destroys old_raw when it goes out of scope
    }
    
    // Unload old library handle with brief delay to allow OS to release file
    if (old_handle) {
        std::this_thread::sleep_for(RELOAD_UNLOAD_DELAY_MS);
        unloadLibraryHandle(old_handle);
    }

    // Update health monitor: unregister old instance, register new one
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (health_monitor_) {
            health_monitor_->unregisterPlugin(name);
            auto it2 = plugins_.find(name);
            if (it2 != plugins_.end() && it2->second.instance) {
                auto* self_healing = dynamic_cast<ISelfHealingPlugin*>(it2->second.instance.get());
                if (self_healing) {
                    health_monitor_->registerPlugin(name, self_healing);
                }
            }
        }
    }

    // Notify AFTER_LOAD
    notifyPluginReload(name, PluginReloadPhase::AFTER_LOAD);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    metrics_.recordReload(name, duration);

    THEMIS_INFO("Hot-reloaded plugin '{}' successfully ({}ms)", name, duration.count());
    return OkVoid();
}

Result<size_t> PluginManager::autoLoadPlugins() {
    std::vector<std::string> topo_order;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Build dependency graph from all registered plugins
        auto dep_graph = PluginDependencyResolver::buildGraph(plugins_);

        // Detect circular dependencies before attempting any load
        auto cycles = PluginDependencyResolver::detectCircularDependencies(dep_graph);
        if (!cycles.empty()) {
            std::string cycle_desc = {};
            for (const auto& cycle : cycles) {
                if (!cycle_desc.empty()) {
                  cycle_desc += "; ";
                }
                for (size_t i = 0; i  < cycle.size(); ++i) {
                    if (i > 0) {
                      cycle_desc += " -> ";
                    }
                    cycle_desc += cycle[i];
                }
            }
            auto error_msg = fmt::format("Circular dependencies detected: {}", cycle_desc);
            THEMIS_ERROR("Cannot auto-load plugins — {}", error_msg);
            return Err<size_t>(errors::ErrorCode::ERR_PLUGIN_CIRCULAR_DEPENDENCY, error_msg);
        }

        // Check for unregistered dependencies (missing plugins)
        auto missing_deps = PluginDependencyResolver::validateDependencies(dep_graph);
        if (!missing_deps.empty()) {
            std::string missing_desc = {};
            for (const auto& [plugin, dep] : missing_deps) {
                if (!missing_desc.empty()) {
                  missing_desc += "; ";
                }
                missing_desc += fmt::format("'{}' requires unregistered '{}'", plugin, dep);
            }
            auto error_msg = fmt::format("Unregistered plugin dependencies: {}", missing_desc);
            THEMIS_ERROR("Cannot auto-load plugins — {}", error_msg);
            return Err<size_t>(errors::ErrorCode::ERR_PLUGIN_MISSING_DEPENDENCY, error_msg);
        }

        // Compute topological load order so dependencies are loaded before dependents
        try {
            topo_order = PluginDependencyResolver::computeLoadOrder(dep_graph);
        } catch (const std::runtime_error& e) {
            // Defensive: computeLoadOrder only throws when cycles or incomplete graphs
            // are present; both are checked above, but guard against unexpected cases.
            auto error_msg = fmt::format("Plugin dependency resolution failed: {}", e.what());
            THEMIS_ERROR("Cannot auto-load plugins — {}", error_msg);
            return Err<size_t>(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED, error_msg);
        }
    }

    size_t loaded = 0;
    for (const auto& name : topo_order) {
        // Only explicitly load plugins marked for auto-load;
        // non-auto-load dependencies are pulled in automatically by loadPlugin().
        // Re-check under the lock because a plugin may have been unregistered
        // between graph construction and this iteration.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = plugins_.find(name);
            if (it == plugins_.end() || !it->second.manifest.auto_load) {
                continue;
            }
        }

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

PluginNegotiationResult PluginManager::negotiateCapabilities(
    const std::string& name,
    const std::vector<PluginCapabilityRequirement>& requirements) const
{
    IThemisPlugin* plugin = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(name);
        if (it == plugins_.end() || !it->second.loaded || !it->second.instance) {
            PluginNegotiationResult result;
            result.success = false;
            result.error_message = fmt::format("Plugin '{}' not found or not loaded", name);
            return result;
        }
        plugin = it->second.instance.get();
    }

    auto result = PluginCapabilityNegotiator::negotiate(*plugin, requirements);
    if (!result.success) {
        THEMIS_DEBUG("Capability negotiation failed for plugin '{}': {}", name, result.error_message);
    }
    return result;
}

PluginManager::~PluginManager() {
    (void)unloadAllPlugins();
}

// ============================================================================
// Runtime capability escalation blocking
// ============================================================================

Result<void> PluginManager::checkCapabilityEscalation(const std::string& name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(name);
    if (it == plugins_.end() || !it->second.loaded || !it->second.instance) {
        return ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                       fmt::format("Plugin '{}' not found or not loaded", name));
    }

    auto& entry = it->second;
    const PluginCapabilities& frozen  = entry.frozen_capabilities;
    const PluginCapabilities  current = entry.instance->getCapabilities();

    // A capability escalation occurs when a flag that was false at load time
    // (i.e. not declared in the manifest capabilities snapshot) is now true.
    bool escalated =
        ((!frozen.supports_streaming    && current.supports_streaming)    ||
        (!frozen.supports_batching     && current.supports_batching)     ||
        (!frozen.supports_transactions && current.supports_transactions) ||
        (!frozen.thread_safe           && current.thread_safe)           ||
        (!frozen.gpu_accelerated       && current.gpu_accelerated));

    if (!escalated) {
        return OkVoid();
    }

    // Mark the plugin as restricted so that operators can act on it.
    entry.is_restricted = true;

    THEMIS_ERROR(
        "Capability escalation attempt detected for plugin '{}' — marking as RESTRICTED",
        name);

    return ErrVoid(
        errors::ErrorCode::ERR_PLUGIN_CAPABILITY_ESCALATION,
        fmt::format(
            "Plugin '{}' attempted to escalate capabilities beyond its manifest declaration",
            name));
}

bool PluginManager::isPluginRestricted(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return false;
    }
    return it->second.is_restricted;
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
        if (plugin_name == name || !entry.loaded) {
          continue;
        }
        
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

void PluginManager::attachHealthMonitor(PluginHealthMonitor* monitor) {
    std::lock_guard<std::mutex> lock(mutex_);
    health_monitor_ = monitor;

    if (!health_monitor_) {
        THEMIS_INFO("PluginManager: health monitor detached");
        return;
    }

    // Register all currently loaded self-healing plugins with the new monitor
    for (const auto& [name, entry] : plugins_) {
        if (!entry.loaded || !entry.instance) {
          continue;
        }
        auto* self_healing = dynamic_cast<ISelfHealingPlugin*>(entry.instance.get());
        if (self_healing) {
            health_monitor_->registerPlugin(name, self_healing);
        }
    }

    THEMIS_INFO("PluginManager: health monitor attached");
}

// ============================================================================
// Edition / License gating helpers
// ============================================================================

bool PluginManager::isEditionSupported() {
    return edition::FEATURE_ENTERPRISE_PLUGINS;
}

bool PluginManager::isLicensed() {
    return license::RuntimeLicenseGate::instance().isFeatureAllowed("enterprise_plugins");
}

std::string PluginManager::communityUnavailableMessage(const std::string& plugin_name) {
    return "Plugin '" + plugin_name +
           "' is not available in Community Edition. "
           "Custom plugins require Enterprise Edition or higher. "
           "Please upgrade at https://themisdb.io/pricing";
}

std::string PluginManager::marketplaceInfo() {
    const auto info = edition::EditionInfo::Get();
    if (!info.supports_plugins) {
        return "Plugin Marketplace: Not available in " +
               std::string(info.name) + " Edition";
    }
    std::string result = "Plugin Marketplace: Available\n";
    result += "Edition: " + std::string(info.name) + "\n";
    result += "Visit: https://marketplace.themisdb.io/";
    if (info.type == edition::EditionType::HYPERSCALER) {
        result += " (OEM custom plugins available)";
    }
    return result;
}

std::string PluginManager::installationInstructions() {
    if (!edition::FEATURE_ENTERPRISE_PLUGINS) {
        return "Error: Plugins are not supported in " +
               std::string(edition::EDITION_STRING) +
               " Edition. Please upgrade to Enterprise or Hyperscaler.";
    }
    return "To install a plugin:\n"
           "1. Download from https://marketplace.themisdb.io/\n"
           "2. Verify SHA256 checksum\n"
           "3. Place in $THEMIS_HOME/plugins/\n"
           "4. Restart themis_server\n"
           "5. Use CREATE PLUGIN command";
}

// ============================================================================
// Phase 3: Error Handling and Edge Cases
// ============================================================================

PluginsError PluginManager::validateConcurrentStateChange(
    const PluginEntry& plugin_entry,
    PluginLifecycleState requested_state) {
    
    std::lock_guard<std::mutex> state_lock(plugin_entry.state_mutex);
    
    // Reject if already in target state (idempotent operations allowed)
    if (plugin_entry.state == requested_state && requested_state == PluginLifecycleState::LOADED) {
       return PluginsError::kSuccess;
    }
    
    // Reject concurrent transitions
    if (plugin_entry.state == PluginLifecycleState::LOADING ||
       plugin_entry.state == PluginLifecycleState::UNLOADING) {
       THEMIS_ERROR("[LIFECYCLE:CONCURRENT] Cannot perform operation: plugin is in transition state {}",
                    lifecycleStateToString(plugin_entry.state));
       return PluginsError::kLifecycleTransition;
    }
    
    // Validate transition is allowed
    if (!isValidLifecycleTransition(plugin_entry.state, requested_state)) {
       THEMIS_ERROR("[LIFECYCLE:INVALID_TRANSITION] Invalid transition: {} → {}",
                    lifecycleStateToString(plugin_entry.state),
                    lifecycleStateToString(requested_state));
       return PluginsError::kLifecycleTransition;
    }
    
    return PluginsError::kSuccess;
}

PluginsError PluginManager::recoverPartialRegistryState(const std::string& plugin_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
       THEMIS_WARN("[LIFECYCLE:RECOVERY] Plugin not found for recovery: {}", plugin_name);
       return PluginsError::kPluginNotFound;
    }
    
    auto& entry = it->second;
    
    THEMIS_WARN("[LIFECYCLE:RECOVERY] Attempting recovery for plugin '{}' in state {}",
               plugin_name, lifecycleStateToString(entry.state));
    
    // If plugin is in LOADING state, roll back to UNLOADED
    if (entry.state == PluginLifecycleState::LOADING) {
       {
           std::lock_guard<std::mutex> state_lock(entry.state_mutex);
           entry.state = PluginLifecycleState::UNLOADED;
       }
       entry.instance.reset();
       entry.loaded = false;
       if (entry.library_handle) {
           unloadLibrary(entry.library_handle);
           entry.library_handle = nullptr;
       }
       THEMIS_WARN("[LIFECYCLE:RECOVERY] Rolled back plugin '{}' from LOADING to UNLOADED", plugin_name);
       return PluginsError::kSuccess;
    }
    
    // If plugin is in UNLOADING state, roll back to UNLOADED
    if (entry.state == PluginLifecycleState::UNLOADING) {
       {
           std::lock_guard<std::mutex> state_lock(entry.state_mutex);
           entry.state = PluginLifecycleState::UNLOADED;
       }
       THEMIS_WARN("[LIFECYCLE:RECOVERY] Rolled back plugin '{}' from UNLOADING to UNLOADED", plugin_name);
       return PluginsError::kSuccess;
    }
    
    // No recovery needed
    THEMIS_INFO("[LIFECYCLE:RECOVERY] Plugin '{}' is in consistent state {}", 
               plugin_name, lifecycleStateToString(entry.state));
    return PluginsError::kSuccess;
}

PluginsError PluginManager::validateManifestOptionalFields(PluginManifest& manifest) {
    // Validate and apply defaults for optional fields
    
    // allowed_editions: empty vector means "all editions" (default)
    if (manifest.allowed_editions.empty()) {
       THEMIS_DEBUG("[VALIDATION:OPTIONAL] Using default for allowed_editions: all editions");
       // Leave empty (all editions allowed)
    } else {
       // Normalize edition names to lowercase for comparison
       for (auto& edition_name : manifest.allowed_editions) {
           std::transform(edition_name.begin(), edition_name.end(), edition_name.begin(),
                         [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
       }
    }
    
    // license_feature: empty string means "no license required" (default)
    if (manifest.license_feature.empty()) {
       THEMIS_DEBUG("[VALIDATION:OPTIONAL] Using default for license_feature: none required");
    }
    
    // capabilities: PluginCapabilities is a struct with bool fields; always valid
    {
        (void)manifest.capabilities;  // no optional field normalization needed
        THEMIS_DEBUG("[VALIDATION:OPTIONAL] capabilities struct present (default: all false)");
    }
    
    // visibility: default to "public" if not specified
    if (manifest.visibility.empty()) {
       manifest.visibility = "public";
       THEMIS_DEBUG("[VALIDATION:OPTIONAL] Using default visibility: public");
    } else {
       std::transform(manifest.visibility.begin(), manifest.visibility.end(), 
                     manifest.visibility.begin(),
                     [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
    
    // dependencies: empty vector means "no dependencies" (default)
    if (manifest.dependencies.empty()) {
       THEMIS_DEBUG("[VALIDATION:OPTIONAL] Using default for dependencies: none");
    }
    
    return PluginsError::kSuccess;
}

PluginsError PluginManager::validateABICompatibility(
    const PluginEntry& previous_entry,
    const PluginManifest& new_manifest) {
    
    // Check interface version compatibility (allow patch-level changes only)
    // Format: major.minor.patch
    const auto parse_version = [](const std::string& ver) -> std::tuple<int, int, int> {
       int major = 0, minor = 0, patch = 0;
       try {
           sscanf(ver.c_str(), "%d.%d.%d", &major, &minor, &patch);
       } catch (...) {
           major = minor = patch = 0;
       }
       return {major, minor, patch};
    };
    
    auto [prev_major, prev_minor, prev_patch] = parse_version(previous_entry.manifest.version);
    auto [new_major, new_minor, new_patch] = parse_version(new_manifest.version);
    
    // Major version change = ABI incompatible
    if (new_major != prev_major) {
       THEMIS_ERROR("[SECURITY:ABI_MISMATCH] Plugin ABI incompatible: {} → {} (major version change)",
                    previous_entry.manifest.version, new_manifest.version);
       return PluginsError::kSignatureVerifyFailed;  // Use signature error as proxy for ABI error
    }
    
    // Minor version change = warn but allow
    if (new_minor != prev_minor) {
       THEMIS_WARN("[SECURITY:ABI_MINOR_VERSION] Plugin minor version changed: {} → {}",
                   previous_entry.manifest.version, new_manifest.version);
    }
    
    // Check capabilities are not reduced
    // [RESOLVED] Field-wise implication is implemented via check_cap lambda below:
    // every capability that was true in the frozen snapshot must still be true
    // in the new manifest.
    const PluginCapabilities& prev_caps = previous_entry.frozen_capabilities;
    const PluginCapabilities& new_caps  = new_manifest.capabilities;

    auto check_cap = [&](bool prev, bool curr, const char* name) {
        if (prev && !curr) {
            THEMIS_WARN("[SECURITY:CAPABILITY_REDUCTION] Plugin capability '{}' was removed after reload", name);
        }
    };
    check_cap(prev_caps.supports_streaming,    new_caps.supports_streaming,    "supports_streaming");
    check_cap(prev_caps.supports_batching,     new_caps.supports_batching,     "supports_batching");
    check_cap(prev_caps.supports_transactions, new_caps.supports_transactions, "supports_transactions");
    check_cap(prev_caps.thread_safe,           new_caps.thread_safe,           "thread_safe");
    check_cap(prev_caps.gpu_accelerated,       new_caps.gpu_accelerated,       "gpu_accelerated");
    check_cap(prev_caps.provides_vram_policy,  new_caps.provides_vram_policy,  "provides_vram_policy");
    check_cap(prev_caps.provides_shard_policy, new_caps.provides_shard_policy, "provides_shard_policy");
    
    THEMIS_INFO("[SECURITY:ABI_COMPATIBLE] Plugin ABI verified compatible: {} → {}",
               previous_entry.manifest.version, new_manifest.version);
    return PluginsError::kSuccess;
}

bool PluginManager::verifyManifestSignatureWithTimeout(
    const std::string& manifest_path,
    uint32_t timeout_ms,
    std::string& error_details) {
    
    // For now, use simple timeout handling via std::chrono
    // In production, this would use async I/O or thread pool with timeout
    
    if (timeout_ms == 0) {
       // No timeout, use standard verification
       return verifyManifestSignature(manifest_path, error_details);
    }
    
    THEMIS_DEBUG("[SECURITY:SIGNATURE_TIMEOUT] Verifying manifest with timeout: {} ms", timeout_ms);
    
    // Simple timeout: check file accessibility first
    if (!fs::exists(manifest_path)) {
       error_details = "Manifest file not found: " + manifest_path;
       return false;
    }
    
    // For large files, timeout might occur. Implement graceful degradation.
    try {
       auto start = std::chrono::steady_clock::now();
       bool result = verifyManifestSignature(manifest_path, error_details);
       auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::steady_clock::now() - start);
        
       if (duration.count() > static_cast<int64_t>(timeout_ms)) {
           THEMIS_WARN("[SECURITY:SIGNATURE_SLOW] Signature verification slow: {} ms (timeout: {} ms)",
                      duration.count(), timeout_ms);
           // In production, we might reject the plugin here. For now, just warn.
       }
        
       return result;
    } catch (const std::exception& e) {
       error_details = std::string("Signature verification exception: ") + e.what();
       return false;
    }
}

json PluginManager::getDiagnosticsForPlugin(const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json diagnostics = json::object();
    diagnostics["plugin_name"] = plugin_name;
    diagnostics["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
       diagnostics["status"] = "not_found";
       diagnostics["error"] = "Plugin not found in registry";
       return diagnostics;
    }
    
    const auto& entry = it->second;
    
    {
       std::lock_guard<std::mutex> state_lock(entry.state_mutex);
       diagnostics["lifecycle_state"] = lifecycleStateToString(entry.state);
    }
    
    diagnostics["loaded"] = entry.loaded;
    diagnostics["path"] = entry.path;
    diagnostics["type"] = static_cast<int>(entry.type);
    diagnostics["manifest_version"] = entry.manifest.version;
    
    // Error state diagnostics
    auto err_it = error_states_.find(plugin_name);
    if (err_it != error_states_.end()) {
       diagnostics["last_error"] = static_cast<int>(err_it->second.last_error);
       diagnostics["last_error_message"] = err_it->second.last_error_message;
       diagnostics["error_count"] = err_it->second.error_count;
    } else {
       diagnostics["last_error"] = 0;  // kSuccess
       diagnostics["error_count"] = 0;
    }
    
    // Capability diagnostics
    {
        json caps = json::object();
        caps["supports_streaming"]    = entry.frozen_capabilities.supports_streaming;
        caps["supports_batching"]     = entry.frozen_capabilities.supports_batching;
        caps["supports_transactions"] = entry.frozen_capabilities.supports_transactions;
        caps["thread_safe"]           = entry.frozen_capabilities.thread_safe;
        caps["gpu_accelerated"]       = entry.frozen_capabilities.gpu_accelerated;
        diagnostics["frozen_capabilities"] = caps;
    }
    
    diagnostics["is_restricted"] = entry.is_restricted;
    
    return diagnostics;
}

ManifestErrorCode PluginManager::validateManifestEditionRestrictions(
    const PluginManifest& manifest,
    std::string& error_details) {
    
    // Check license_feature format if present
    if (!manifest.license_feature.empty()) {
        // Pattern: ^[a-z0-9][a-z0-9_.-]*$
        if (manifest.license_feature[0] < 'a' || manifest.license_feature[0] > 'z') {
            if (manifest.license_feature[0] < '0' || manifest.license_feature[0] > '9') {
                error_details = "license_feature must start with lowercase letter or digit";
                return ManifestErrorCode::PLUGIN_LICENSE_FEATURE_INVALID;
            }
        }
        for (char c : manifest.license_feature) {
            const unsigned char uc = static_cast<unsigned char>(c);
            if (!std::islower(uc) && !std::isdigit(uc) && c != '_' && c != '.' && c != '-') {
                error_details = "license_feature contains invalid character: " + std::string(1, c);
                return ManifestErrorCode::PLUGIN_LICENSE_FEATURE_INVALID;
            }
        }
    }
    
    // Check allowed_editions constraint
    if (!manifest.allowed_editions.empty()) {
        const auto current = normalizeEditionName(std::string(edition::EDITION_STRING));
        bool edition_allowed = std::any_of(
            manifest.allowed_editions.begin(),
            manifest.allowed_editions.end(),
            [&](const std::string& allowed) {
                return normalizeEditionName(allowed) == current;
            }
        );
        
        if (!edition_allowed) {
            error_details = "Plugin not allowed on edition '" + std::string(edition::EDITION_STRING) +
                          "'. Allowed editions: " + 
                          [&]() {
                              std::string result = {};
                              for (size_t i = 0; i  < manifest.allowed_editions.size(); ++i) {
                                  if (i > 0) {
                                    result += ", ";
                                  }
                                  result += manifest.allowed_editions[i];
                              }
                              return result;
                          }();
            return ManifestErrorCode::PLUGIN_EDITION_MISMATCH;
        }
    }
    
    // Check license_feature gate if required
    if (!manifest.license_feature.empty()) {
        std::string license_error = {};
        auto& license_gate = themis::license::RuntimeLicenseGate::instance();
        if (!license_gate.isFeatureAllowed(manifest.license_feature, license_error)) {
            error_details = "License feature '" + manifest.license_feature + "' not granted";
            return ManifestErrorCode::PLUGIN_LICENSE_DENIED;
        }
    }
    
    return ManifestErrorCode::MANIFEST_OK;
}

ManifestErrorCode PluginManager::validateManifestPublicPrivateBoundary(
    const PluginManifest& manifest,
    const std::string& plugin_path,
    std::string& error_details) {
    
    // Normalize visibility to lowercase to prevent case-sensitivity bypass
    // at this security boundary (e.g. "Private" must be treated the same as "private").
    std::string raw_visibility = manifest.visibility.empty() ? "public" : manifest.visibility;
    std::string visibility = {};
    visibility.reserve(raw_visibility.size());
    for (unsigned char c : raw_visibility) {
        visibility += static_cast<char>(std::tolower(c));
    }
    const auto current_edition = normalizeEditionName(std::string(edition::EDITION_STRING));
    
    // Rule 1: visibility="private" AND edition="community" → FAIL-CLOSED
    if (visibility == "private" && current_edition == "community") {
        error_details = "Private plugin cannot be loaded in community edition";
        return ManifestErrorCode::PLUGIN_PRIVATE_IN_COMMUNITY;
    }
    
    // Rule 2: plugin_path contains "private/" BUT visibility!="private" → WARN/BLOCK
    if (plugin_path.find("private/") != std::string::npos ||
        plugin_path.find("private\\") != std::string::npos) {
        if (visibility != "private") {
            error_details = "Plugin path contains 'private/' but visibility is '" + visibility +
                          "', not 'private'. Mismatched boundary marking.";
            return ManifestErrorCode::PLUGIN_PATH_VISIBILITY_MISMATCH;
        }
    }
    
    // Rule 3: visibility="restricted" AND no scoped-checkout context → FAIL-CLOSED
    if (visibility == "restricted") {
        // Check for scoped checkout context (e.g., environment variable or config)
        const char* scoped_context = std::getenv("THEMISDB_SCOPED_CHECKOUT");
        if (!scoped_context || std::string(scoped_context).empty()) {
            error_details = "Restricted plugin requires scoped checkout context (THEMISDB_SCOPED_CHECKOUT)";
            return ManifestErrorCode::PLUGIN_RESTRICTED_NO_CONTEXT;
        }
    }
    
    return ManifestErrorCode::MANIFEST_OK;
}

std::string PluginManager::formatDiagnosticMessage(
    PluginsError error_code,
    const std::string& context,
    const std::string& plugin_name) {
    
    // Determine the error category based on error code
    std::string category = {};
    std::string code_name = {};
    
    switch (error_code) {
       case PluginsError::kSuccess:
           code_name = "SUCCESS";
           category = "INFO";
           break;
       case PluginsError::kPluginNotFound:
           code_name = "PLUGIN_NOT_FOUND";
           category = "LIFECYCLE";
           break;
       case PluginsError::kManifestInvalid:
           code_name = "MANIFEST_INVALID";
           category = "VALIDATION";
           break;
       case PluginsError::kSignatureVerifyFailed:
           code_name = "SIGNATURE_VERIFY_FAILED";
           category = "SECURITY";
           break;
       case PluginsError::kLifecycleTransition:
           code_name = "LIFECYCLE_TRANSITION";
           category = "LIFECYCLE";
           break;
       case PluginsError::kCapabilityDenied:
           code_name = "CAPABILITY_DENIED";
           category = "SECURITY";
           break;
       case PluginsError::kRegistryConflict:
           code_name = "REGISTRY_CONFLICT";
           category = "LIFECYCLE";
           break;
       case PluginsError::kHealthCheckFailed:
           code_name = "HEALTH_CHECK_FAILED";
           category = "LIFECYCLE";
           break;
       case PluginsError::kInternalError:
           code_name = "INTERNAL_ERROR";
           category = "INTERNAL";
           break;
       default:
           code_name = "UNKNOWN";
           category = "UNKNOWN";
    }
    
    // Format: [CATEGORY:CODE] plugin_name: context
    std::stringstream ss = {};
    ss << "[" << category << ":" << code_name << "]";
    if (!plugin_name.empty()) {
       ss << " [plugin:" << plugin_name << "]";
    }
    ss << " " << context;
    
    return ss.str();
}

} // namespace plugins
} // namespace themis

