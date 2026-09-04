/**
 * @file plugin_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Public interface
#include "acceleration/plugin_loader.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iostream>

#include "acceleration/plugin_security.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/stat.h>
#endif

/*
 * Acceleration module — Dynamic Backend Plugin Loader
 * ====================================================
 * Loads external GPU backend shared libraries (.so / .dll) into the
 * acceleration registry after verifying their digital signatures and
 * enforcing file-permission security checks.
 *
 * Dispatch chain position
 * -----------------------
 *   BackendRegistry::initializeRuntime()
 *       └─► PluginLoader::loadPluginsFromDirectory()   ← this file
 *               ├─► PluginSecurityVerifier::validatePluginPath()   (path traversal guard)
 *               ├─► PluginSecurityVerifier::checkFilePermissions() (group/world-writable reject)
 *               ├─► PluginSecurityVerifier::verifySignature()      (GPG / macOS codesign / PE cert)
 *               └─► dlopen(RTLD_NOW | RTLD_LOCAL)                  (fail-fast symbol binding)
 *                       └─► plugin exports create_backend() → IComputeBackend*
 *                               └─► BackendRegistry::registerBackend()
 *
 * Security boundaries
 * --------------------
 *   - File must exist, not be group/world-writable, be under 128 MB, and pass
 *     digital signature verification before any dynamic linking occurs.
 *   - RTLD_NOW detects unresolved symbols at load time (not at first call).
 *   - TLS public-key pinning for remote plugin registries via RegistryConfig::pinned_public_key.
 *   - All security events are logged to PluginSecurityAuditor::instance().
 *
 * Key interfaces implemented / exposed
 * -------------------------------------
 *   PluginLoader::loadPlugin(path)              — load and register a single plugin
 *   PluginLoader::loadPluginsFromDirectory(dir) — scan directory and load all valid plugins
 *   PluginLoader::unloadAllPlugins()            — release all loaded plugin handles
 *
 * Related files
 * -------------
 *   include/acceleration/plugin_loader.h    — PluginLoader interface declaration
 *   src/acceleration/plugin_security.cpp    — signature verification and audit logging
 *   src/acceleration/backend_registry.cpp   — consumes loadPluginsFromDirectory() at startup
 *   src/acceleration/SECURITY.md            — threat model and security controls
 */

namespace themis {
namespace acceleration {

PluginLoader::~PluginLoader() {
    unloadAllPlugins();
}

void *PluginLoader::loadLibrary(const std::string &path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void *PluginLoader::getSymbol(void *handle, const std::string &symbolName) {
#ifdef _WIN32
    return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
#else
    return dlsym(handle, symbolName.c_str());
#endif
}

void PluginLoader::unloadLibrary(void *handle) {
    if (!handle) {
        return;
    }

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

bool PluginLoader::loadPlugin(const std::string &libraryPath) {
    // SECURITY: Validate path to prevent path traversal attacks
    std::string pathError;
    if (!PluginSecurityVerifier::validatePluginPath(libraryPath, pathError)) {
        std::cerr << "SECURITY: Plugin path validation failed: " << libraryPath << std::endl;
        std::cerr << "  Reason: " << pathError << std::endl;
        auto &auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({PluginSecurityEvent::EventType::POLICY_VIOLATION, libraryPath, "", pathError,
                          static_cast<uint64_t>(std::time(nullptr)), "ERROR"});
        return false;
    }

#ifndef _WIN32
    // SECURITY: Reject plugins with insecure file permissions (group/world writable)
    // and enforce a maximum file size to guard against resource exhaustion.
    {
        struct stat st;
        if (stat(libraryPath.c_str(), &st) == 0) {
            if (st.st_mode & (S_IWGRP | S_IWOTH)) {
                std::string reason = "Plugin file has insecure permissions (group/world writable)";
                std::cerr << "SECURITY: " << reason << ": " << libraryPath << std::endl;
                auto &auditor = PluginSecurityAuditor::instance();
                auditor.logEvent({PluginSecurityEvent::EventType::POLICY_VIOLATION, libraryPath, "", reason,
                                  static_cast<uint64_t>(std::time(nullptr)), "ERROR"});
                return false;
            }
            constexpr off_t kMaxPluginBytes = 128LL * 1024 * 1024; // 128 MB
            if (st.st_size > kMaxPluginBytes) {
                std::string reason = "Plugin file exceeds maximum allowed size (128 MB)";
                std::cerr << "SECURITY: " << reason << ": " << libraryPath << std::endl;
                auto &auditor = PluginSecurityAuditor::instance();
                auditor.logEvent({PluginSecurityEvent::EventType::POLICY_VIOLATION, libraryPath, "", reason,
                                  static_cast<uint64_t>(std::time(nullptr)), "ERROR"});
                return false;
            }
        }
    }
#endif

    // SECURITY: Verify plugin before loading
    PluginSecurityPolicy policy;
    // Load policy from config if available
    // For now, use default policy (requires signature in production)

#ifdef NDEBUG
    // Production: Require signature
    policy.requireSignature = true;
    policy.allowUnsigned    = false;
#else
    // Development: Allow unsigned
    policy.requireSignature = false;
    policy.allowUnsigned    = true;
#endif

    PluginSecurityVerifier verifier(policy);
    std::string errorMessage;

    if (!verifier.verifyPlugin(libraryPath, errorMessage)) {
        std::cerr << "SECURITY: Plugin verification failed: " << libraryPath << std::endl;
        std::cerr << "  Reason: " << errorMessage << std::endl;

        // Log security event
        auto &auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED, libraryPath,
                          verifier.calculateFileHash(libraryPath), errorMessage,
                          static_cast<uint64_t>(std::time(nullptr)), "ERROR"});

        return false;
    }

    std::cout << "SECURITY: Plugin verification passed: " << libraryPath << std::endl;

    // Load the shared library
    void *handle = loadLibrary(libraryPath);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << libraryPath << std::endl;
#ifndef _WIN32
        std::cerr << "Error: " << dlerror() << std::endl;
#endif
        return false;
    }

    // Get the plugin factory function
    auto createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "CreateBackendPlugin"));
    if (!createFunc) {
        std::cerr << "Plugin library does not export CreateBackendPlugin: " << libraryPath << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Create the plugin instance
    BackendPlugin *plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance: " << libraryPath << std::endl;
        unloadLibrary(handle);
        return false;
    }

    // Store the plugin
    PluginHandle pluginHandle;
    pluginHandle.libraryHandle = handle;
    pluginHandle.plugin.reset(plugin);
    pluginHandle.name     = plugin->pluginName();
    pluginHandle.path     = libraryPath;
    pluginHandle.fileHash = verifier.calculateFileHash(libraryPath);

    plugins_.push_back(std::move(pluginHandle));

    std::cout << "Loaded plugin: " << plugin->pluginName() << " v" << plugin->pluginVersion()
              << " (Hash: " << pluginHandle.fileHash.substr(0, 16) << "...)" << std::endl;

    return true;
}

size_t PluginLoader::loadPluginsFromDirectory(const std::string &directoryPath) {
    namespace fs = std::filesystem;

    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cerr << "Plugin directory does not exist: " << directoryPath << std::endl;
        return 0;
    }

    // SECURITY: Resolve the canonical directory path once to detect symlink escapes
    std::error_code ec;
    fs::path canonicalDir = fs::canonical(directoryPath, ec);
    if (ec) {
        std::cerr << "SECURITY: Cannot resolve canonical directory path: " << directoryPath << std::endl;
        return 0;
    }

    size_t loadedCount = 0;

    // Determine the platform-specific library extension
#ifdef _WIN32
    const std::string extension = ".dll";
#elif defined(__APPLE__)
    const std::string extension = ".dylib";
#else
    const std::string extension = ".so";
#endif

    // Scan directory for plugin libraries
    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        // SECURITY: Skip symlinks to prevent escaping the plugin directory
        if (entry.is_symlink()) {
            // Resolve symlink target and verify it stays within the directory
            // by comparing path components, not string prefixes, to avoid
            // false positives (e.g. /plugins_evil matching /plugins).
            fs::path resolvedTarget = fs::canonical(entry.path(), ec);
            bool escaped            = (ec.value() != 0);
            if (!escaped) {
                const auto mismatchPair = std::mismatch(canonicalDir.begin(), canonicalDir.end(),
                                                        resolvedTarget.begin(), resolvedTarget.end());
                escaped                 = (mismatchPair.first != canonicalDir.end());
            }
            if (escaped) {
                std::cerr << "SECURITY: Skipping symlink that escapes plugin directory: " << entry.path() << std::endl;
                auto &auditor = PluginSecurityAuditor::instance();
                auditor.logEvent({PluginSecurityEvent::EventType::POLICY_VIOLATION, entry.path().string(), "",
                                  "Symlink escapes plugin directory", static_cast<uint64_t>(std::time(nullptr)),
                                  "WARNING"});
                continue;
            }
        }

        if (!entry.is_regular_file() && !entry.is_symlink()) {
            continue;
        }

        std::string path     = entry.path().string();
        std::string filename = entry.path().filename().string();

        // Check if it's a plugin library (starts with "themis_accel_" and has correct extension)
        if (filename.find("themis_accel_") == 0 && filename.find(extension) != std::string::npos) {
            if (loadPlugin(path)) {
                loadedCount++;
            }
        }
    }

    std::cout << "Loaded " << loadedCount << " acceleration plugins from " << directoryPath << std::endl;

    return loadedCount;
}

void PluginLoader::unloadPlugin(const std::string &pluginName) {
    for (auto it = plugins_.begin(); it != plugins_.end(); ++it) {
        if (it->name == pluginName) {
            std::cout << "Unloading plugin: " << pluginName << std::endl;
            unloadLibrary(it->libraryHandle);
            plugins_.erase(it);
            return;
        }
    }
}

void PluginLoader::unloadAllPlugins() {
    for (auto &plugin : plugins_) {
        std::cout << "Unloading plugin: " << plugin.name << std::endl;
        unloadLibrary(plugin.libraryHandle);
    }
    plugins_.clear();
}

BackendPlugin *PluginLoader::getPlugin(const std::string &pluginName) const {
    for (const auto &plugin : plugins_) {
        if (plugin.name == pluginName) {
            return plugin.plugin.get();
        }
    }
    return nullptr;
}

std::vector<BackendPlugin *> PluginLoader::getLoadedPlugins() const {
    std::vector<BackendPlugin *> result = {};

    result.reserve(plugins_.size());

    for (const auto &plugin : plugins_) {
        result.push_back(plugin.plugin.get());
    }

    return result;
}

} // namespace acceleration
} // namespace themis
