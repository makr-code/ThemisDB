/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_loader.cpp                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:23:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     322                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • ce91302f75  2026-02-24  feat: erweitere die ModularBuild-Konfiguration und implem... ║
    • 3f47ce19e8  2026-02-23  feat(acceleration): security hardening pass for plugin/dr... ║
    • 40c623acf8  2026-02-23  Implement security audit for backend plugin loading and r... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/plugin_loader.h"
#include "acceleration/plugin_security.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <sys/stat.h>
#endif

namespace themis {
namespace acceleration {

PluginLoader::~PluginLoader() {
    unloadAllPlugins();
}

void* PluginLoader::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void* PluginLoader::getSymbol(void* handle, const std::string& symbolName) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
#else
    return dlsym(handle, symbolName.c_str());
#endif
}

void PluginLoader::unloadLibrary(void* handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

bool PluginLoader::loadPlugin(const std::string& libraryPath) {
    // SECURITY: Validate path to prevent path traversal attacks
    std::string pathError;
    if (!PluginSecurityVerifier::validatePluginPath(libraryPath, pathError)) {
        std::cerr << "SECURITY: Plugin path validation failed: " << libraryPath << std::endl;
        std::cerr << "  Reason: " << pathError << std::endl;
        auto& auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({
            PluginSecurityEvent::EventType::POLICY_VIOLATION,
            libraryPath, "", pathError,
            static_cast<uint64_t>(std::time(nullptr)),
            "ERROR"
        });
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
                auto& auditor = PluginSecurityAuditor::instance();
                auditor.logEvent({
                    PluginSecurityEvent::EventType::POLICY_VIOLATION,
                    libraryPath, "", reason,
                    static_cast<uint64_t>(std::time(nullptr)),
                    "ERROR"
                });
                return false;
            }
            constexpr off_t kMaxPluginBytes = 128LL * 1024 * 1024;  // 128 MB
            if (st.st_size > kMaxPluginBytes) {
                std::string reason = "Plugin file exceeds maximum allowed size (128 MB)";
                std::cerr << "SECURITY: " << reason << ": " << libraryPath << std::endl;
                auto& auditor = PluginSecurityAuditor::instance();
                auditor.logEvent({
                    PluginSecurityEvent::EventType::POLICY_VIOLATION,
                    libraryPath, "", reason,
                    static_cast<uint64_t>(std::time(nullptr)),
                    "ERROR"
                });
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
    policy.allowUnsigned = false;
#else
    // Development: Allow unsigned
    policy.requireSignature = false;
    policy.allowUnsigned = true;
#endif
    
    PluginSecurityVerifier verifier(policy);
    std::string errorMessage;
    
    if (!verifier.verifyPlugin(libraryPath, errorMessage)) {
        std::cerr << "SECURITY: Plugin verification failed: " << libraryPath << std::endl;
        std::cerr << "  Reason: " << errorMessage << std::endl;
        
        // Log security event
        auto& auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
            libraryPath,
            verifier.calculateFileHash(libraryPath),
            errorMessage,
            static_cast<uint64_t>(std::time(nullptr)),
            "ERROR"
        });
        
        return false;
    }
    
    std::cout << "SECURITY: Plugin verification passed: " << libraryPath << std::endl;
    
    // Load the shared library
    void* handle = loadLibrary(libraryPath);
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
    BackendPlugin* plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance: " << libraryPath << std::endl;
        unloadLibrary(handle);
        return false;
    }
    
    // Store the plugin
    PluginHandle pluginHandle;
    pluginHandle.libraryHandle = handle;
    pluginHandle.plugin.reset(plugin);
    pluginHandle.name = plugin->pluginName();
    pluginHandle.path = libraryPath;
    pluginHandle.fileHash = verifier.calculateFileHash(libraryPath);
    
    plugins_.push_back(std::move(pluginHandle));
    
    std::cout << "Loaded plugin: " << plugin->pluginName() 
              << " v" << plugin->pluginVersion() 
              << " (Hash: " << pluginHandle.fileHash.substr(0, 16) << "...)" << std::endl;
    
    return true;
}

size_t PluginLoader::loadPluginsFromDirectory(const std::string& directoryPath) {
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
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        // SECURITY: Skip symlinks to prevent escaping the plugin directory
        if (entry.is_symlink()) {
            // Resolve symlink target and verify it stays within the directory
            // by comparing path components, not string prefixes, to avoid
            // false positives (e.g. /plugins_evil matching /plugins).
            fs::path resolvedTarget = fs::canonical(entry.path(), ec);
            bool escaped = (ec.value() != 0);
            if (!escaped) {
                auto [dirIt, tgtIt] = std::mismatch(
                    canonicalDir.begin(), canonicalDir.end(),
                    resolvedTarget.begin(), resolvedTarget.end()
                );
                escaped = (dirIt != canonicalDir.end());
            }
            if (escaped) {
                std::cerr << "SECURITY: Skipping symlink that escapes plugin directory: "
                          << entry.path() << std::endl;
                auto& auditor = PluginSecurityAuditor::instance();
                auditor.logEvent({
                    PluginSecurityEvent::EventType::POLICY_VIOLATION,
                    entry.path().string(), "",
                    "Symlink escapes plugin directory",
                    static_cast<uint64_t>(std::time(nullptr)),
                    "WARNING"
                });
                continue;
            }
        }

        if (!entry.is_regular_file() && !entry.is_symlink()) continue;
        
        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();
        
        // Check if it's a plugin library (starts with "themis_accel_" and has correct extension)
        if (filename.find("themis_accel_") == 0 && 
            filename.find(extension) != std::string::npos) {
            
            if (loadPlugin(path)) {
                loadedCount++;
            }
        }
    }
    
    std::cout << "Loaded " << loadedCount << " acceleration plugins from " << directoryPath << std::endl;
    
    return loadedCount;
}

void PluginLoader::unloadPlugin(const std::string& pluginName) {
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
    for (auto& plugin : plugins_) {
        std::cout << "Unloading plugin: " << plugin.name << std::endl;
        unloadLibrary(plugin.libraryHandle);
    }
    plugins_.clear();
}

BackendPlugin* PluginLoader::getPlugin(const std::string& pluginName) const {
    for (const auto& plugin : plugins_) {
        if (plugin.name == pluginName) {
            return plugin.plugin.get();
        }
    }
    return nullptr;
}

std::vector<BackendPlugin*> PluginLoader::getLoadedPlugins() const {
    std::vector<BackendPlugin*> result;
    result.reserve(plugins_.size());
    
    for (const auto& plugin : plugins_) {
        result.push_back(plugin.plugin.get());
    }
    
    return result;
}

} // namespace acceleration
} // namespace themis
