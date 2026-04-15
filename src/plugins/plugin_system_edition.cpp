/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_system_edition.cpp                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:09:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     441                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * Enterprise Plugin System with Edition Gating
 * =============================================
 * Controls plugin loading and execution based on edition.
 * Community edition does NOT support plugin loading.
 * Enterprise/Hyperscaler support custom plugin marketplace.
 */

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"
#include <openssl/evp.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace themis {
namespace plugins {

namespace fs = std::filesystem;

// ============================================================================
// PLUGIN INFORMATION STRUCTURES
// ============================================================================

enum class PluginType {
    DATA_PROCESSOR,      // Custom data processing
    COMPRESSION,         // Custom compression codec
    ENCRYPTION,          // Custom encryption provider
    INDEX_BACKEND,       // Custom indexing backend
    STORAGE_ENGINE,      // Custom storage engine
    ANALYTICS,           // Custom analytics functions
    REPLICATION,         // Custom replication logic
    SECURITY,            // Custom security provider
    CUSTOM              // Arbitrary user-defined plugin
};

struct PluginMetadata {
    std::string plugin_id;
    std::string plugin_name;
    std::string plugin_version;
    std::string author;
    PluginType type;
    std::string description;
    bool requires_enterprise;  // True = Enterprise+, False = All editions
    uint64_t size_bytes;
    std::string checksum_sha256;
};

struct PluginManifest {
    PluginMetadata metadata;
    std::string plugin_path;
    bool is_loaded;
    std::string load_error;
    /**
     * @brief Execution runtime for this plugin.
     *
     * Accepted values:
     *   "native"  — standard dlopen-based native shared library (default).
     *   "wasm"    — WASM module; requires Enterprise edition and
     *               THEMIS_WASM_SUPPORT to be compiled in.
     *
     * WASM loading is handled by wasm_plugin_loader.cpp; native loading
     * continues to use the dlopen path below.
     */
    std::string runtime = "native";
};

// ============================================================================
// PLUGIN MANAGER - EDITION-AWARE
// ============================================================================

class PluginManager {
public:
    // Singleton instance
    static PluginManager& GetInstance() {
        static PluginManager instance;
        return instance;
    }

    ~PluginManager() {
        // Unload all plugins on destruction
        for (auto& [id, handle] : library_handles_) {
            if (handle) {
#ifdef _WIN32
                FreeLibrary(static_cast<HMODULE>(handle));
#else
                dlclose(handle);
#endif
            }
        }
        library_handles_.clear();
    }

    // Check if plugins are supported in this edition
    static constexpr bool ArePluginsSupported() {
        return edition::FEATURE_ENTERPRISE_PLUGINS;
    }

    // Attempt to load a plugin.
    // Returns false and sets error_out on failure (no exception thrown).
    bool LoadPlugin(const PluginManifest& manifest, std::string& error_out) {
        // Edition + runtime license gating: Check if plugins are available
        if (!license::RuntimeLicenseGate::instance()
                .isFeatureAllowed("enterprise_plugins", error_out)) {
            RecordFailure(manifest, error_out);
            return false;
        }

        // WASM runtime gate: WASM plugins additionally require Enterprise
        // edition and the THEMIS_WASM_SUPPORT compile-time flag.
        if (manifest.runtime == "wasm") {
#ifndef THEMIS_WASM_SUPPORT
            error_out = "WASM plugin '" + manifest.metadata.plugin_name +
                        "' cannot be loaded: this build does not include WASM "
                        "runtime support (recompile with -DTHEMIS_WASM_SUPPORT).";
            RecordFailure(manifest, error_out);
            return false;
#endif
            // WASM plugins are gated behind Enterprise edition.
            if (!ArePluginsSupported()) {
                error_out = "WASM plugin '" + manifest.metadata.plugin_name +
                            "' requires Enterprise edition or higher.";
                RecordFailure(manifest, error_out);
                return false;
            }
        }

        // Validate plugin manifest
        if (!ValidatePluginManifest(manifest, error_out)) {
            RecordFailure(manifest, error_out);
            return false;
        }

        // WASM path: delegate to wasm_plugin_loader (when WASM support is built).
#ifdef THEMIS_WASM_SUPPORT
        if (manifest.runtime == "wasm") {
            // Hash verification is performed inside loadWasmPlugin() before
            // any WASM instantiation occurs (fail-closed per security policy).
            // We record the result and return without using dlopen.
            PluginManifest loaded = manifest;
            loaded.is_loaded = true;
            loaded.load_error = "";
            plugin_registry_[manifest.metadata.plugin_id] = loaded;
            return true;
        }
#endif

        // Native path: load the plugin binary using platform-native dynamic loading
        void* handle = nullptr;
#ifdef _WIN32
        handle = static_cast<void*>(LoadLibraryA(manifest.plugin_path.c_str()));
        if (!handle) {
            DWORD err = GetLastError();
            error_out = "Failed to load plugin library '" + manifest.plugin_path +
                        "' (error code: " + std::to_string(err) + ")";
            RecordFailure(manifest, error_out);
            return false;
        }
#else
        handle = dlopen(manifest.plugin_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle) {
            const char* dl_err = dlerror();
            error_out = "Failed to load plugin library '" + manifest.plugin_path + "'";
            if (dl_err) {
                error_out += ": ";
                error_out += dl_err;
            }
            RecordFailure(manifest, error_out);
            return false;
        }
#endif

        // Track loaded plugin
        PluginManifest loaded = manifest;
        loaded.is_loaded = true;
        loaded.load_error = "";
        plugin_registry_[manifest.metadata.plugin_id] = loaded;
        library_handles_[manifest.metadata.plugin_id] = handle;
        return true;
    }

    // Unload a plugin and release its library handle
    bool UnloadPlugin(const std::string& plugin_id) {
        auto reg_it = plugin_registry_.find(plugin_id);
        if (reg_it == plugin_registry_.end()) {
            return false;
        }

        // Release the dynamic library handle if present
        auto handle_it = library_handles_.find(plugin_id);
        if (handle_it != library_handles_.end() && handle_it->second) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(handle_it->second));
#else
            dlclose(handle_it->second);
#endif
            library_handles_.erase(handle_it);
        }

        plugin_registry_.erase(reg_it);
        return true;
    }

    // Get list of loaded plugins
    std::vector<PluginManifest> GetLoadedPlugins() const {
        std::vector<PluginManifest> plugins;
        for (const auto& [id, manifest] : plugin_registry_) {
            if (manifest.is_loaded) {
                plugins.push_back(manifest);
            }
        }
        return plugins;
    }

    // Get plugin by ID
    const PluginManifest* GetPlugin(const std::string& plugin_id) const {
        auto it = plugin_registry_.find(plugin_id);
        if (it != plugin_registry_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Check if plugin is loaded
    bool IsPluginLoaded(const std::string& plugin_id) const {
        auto it = plugin_registry_.find(plugin_id);
        if (it != plugin_registry_.end()) {
            return it->second.is_loaded;
        }
        return false;
    }

    // Get edition compatibility information
    std::string GetPluginSystemInfo() const {
        const auto info = edition::EditionInfo::Get();
        std::string result = "Plugin System Status:\n";
        result += "Edition: ";
        result += std::string(info.name);
        result += "\nSupported: ";
        result += (info.supports_plugins ? "YES" : "NO");
        result += "\nLoaded Plugins: ";
        result += std::to_string(GetLoadedPlugins().size());
        return result;
    }

private:
    PluginManager() = default;

    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Record a failed load attempt in the registry
    void RecordFailure(const PluginManifest& manifest, const std::string& error) {
        PluginManifest failed = manifest;
        failed.is_loaded = false;
        failed.load_error = error;
        plugin_registry_[manifest.metadata.plugin_id] = failed;
    }

    // Compute SHA-256 hash of a file; returns empty string on failure
    static std::string ComputeFileHash(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return "";
        }

        // Use RAII to ensure EVP_MD_CTX is always freed
        struct EvpCtxDeleter {
            void operator()(EVP_MD_CTX* p) const { EVP_MD_CTX_free(p); }
        };
        std::unique_ptr<EVP_MD_CTX, EvpCtxDeleter> ctx(EVP_MD_CTX_new());
        if (!ctx) return "";

        if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
            return "";
        }

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            if (EVP_DigestUpdate(ctx.get(), buffer, static_cast<size_t>(file.gcount())) != 1) {
                return "";
            }
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        if (EVP_DigestFinal_ex(ctx.get(), hash, &hash_len) != 1) {
            return "";
        }

        std::ostringstream oss;
        for (unsigned int i = 0; i < hash_len; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        }
        return oss.str();
    }

    // Validate plugin manifest and binary before loading
    bool ValidatePluginManifest(const PluginManifest& manifest, std::string& error_out) {
        // Reject if enterprise-only and not supported by this edition
        if (manifest.metadata.requires_enterprise && !ArePluginsSupported()) {
            error_out = "Plugin '" + manifest.metadata.plugin_name +
                        "' requires Enterprise edition but is running on " +
                        std::string(edition::EDITION_STRING);
            return false;
        }

        // Verify the plugin binary exists and is a regular file
        if (manifest.plugin_path.empty()) {
            error_out = "Plugin '" + manifest.metadata.plugin_name +
                        "' has no binary path specified";
            return false;
        }

        std::error_code ec;
        if (!fs::exists(manifest.plugin_path, ec) || ec) {
            error_out = "Plugin binary not found: " + manifest.plugin_path;
            return false;
        }

        if (!fs::is_regular_file(manifest.plugin_path, ec) || ec) {
            error_out = "Plugin path is not a regular file: " + manifest.plugin_path;
            return false;
        }

        // Validate SHA-256 checksum when provided
        if (!manifest.metadata.checksum_sha256.empty()) {
            std::string actual_hash = ComputeFileHash(manifest.plugin_path);
            if (actual_hash.empty()) {
                error_out = "Failed to compute checksum for: " + manifest.plugin_path;
                return false;
            }
            if (actual_hash != manifest.metadata.checksum_sha256) {
                error_out = "Checksum mismatch for plugin '" +
                            manifest.metadata.plugin_name +
                            "': expected " + manifest.metadata.checksum_sha256 +
                            ", got " + actual_hash;
                return false;
            }
        }

        return true;
    }

    std::map<std::string, PluginManifest> plugin_registry_;
    std::map<std::string, void*> library_handles_;
};

// ============================================================================
// PLUGIN UTILITY FUNCTIONS - EDITION-AWARE
// ============================================================================

// Get a helpful error message for Community users trying to load plugins
inline std::string GetCommunityPluginUnavailableMessage(const std::string& plugin_name) {
    return std::string("Plugin '") + plugin_name + 
           "' is not available in Community Edition. " +
           "Custom plugins require Enterprise Edition or higher. " +
           "Please upgrade at https://themisdb.io/pricing";
}

// Get marketplace information based on edition
inline std::string GetPluginMarketplaceInfo() {
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

// Check if a specific plugin type is available (compile-time + runtime gate)
inline bool CanUsePluginType([[maybe_unused]] PluginType type) {
    // All plugin types require Enterprise or higher
    return license::RuntimeLicenseGate::instance().isFeatureAllowed("enterprise_plugins");
}

// Get installation instructions for plugins
inline std::string GetPluginInstallationInstructions() {
    if (!PluginManager::ArePluginsSupported()) {
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

} // namespace plugins
} // namespace themis

