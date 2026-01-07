// Module loader implementation with DLL signature verification
// This prevents corrupted or malicious DLL loading in modular ThemisDB

#include "themis/base/module_loader.h"
#include "acceleration/plugin_security.h"
#include <filesystem>
#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace themis {
namespace modules {

using namespace themis::acceleration;

// ============================================================================
// ModuleSecurityVerifier::Impl - Bridge to existing plugin security
// ============================================================================

class ModuleSecurityVerifier::Impl {
public:
    Impl() : policy_(), verifier_(policy_) {
#ifdef NDEBUG
        // Production: Require signature verification
        policy_.requireSignature = true;
        policy_.allowUnsigned = false;
        policy_.verifyFileHash = true;
        policy_.checkRevocation = true;
        policy_.minTrustLevel = PluginTrustLevel::TRUSTED;
#else
        // Development: Allow unsigned modules for testing
        policy_.requireSignature = false;
        policy_.allowUnsigned = true;
        policy_.verifyFileHash = true;
        policy_.checkRevocation = false;
        policy_.minTrustLevel = PluginTrustLevel::UNTRUSTED;
#endif
        
        spdlog::info("ModuleSecurityVerifier initialized (Production mode: {})", 
                     policy_.requireSignature);
    }
    
    bool verifyModule(const std::string& modulePath, std::string& errorMessage) {
        spdlog::debug("Verifying module: {}", modulePath);
        
        // Use existing plugin security infrastructure
        bool result = verifier_.verifyPlugin(modulePath, errorMessage);
        
        if (result) {
            spdlog::info("Module verification PASSED: {}", modulePath);
        } else {
            spdlog::error("Module verification FAILED: {} - {}", modulePath, errorMessage);
        }
        
        return result;
    }
    
    std::string calculateFileHash(const std::string& modulePath) {
        return verifier_.calculateFileHash(modulePath);
    }
    
    void setRequireSignature(bool require) {
        policy_.requireSignature = require;
        verifier_.updatePolicy(policy_);
    }
    
    void setAllowUnsigned(bool allow) {
        policy_.allowUnsigned = allow;
        verifier_.updatePolicy(policy_);
    }
    
    void addWhitelistedHash(const std::string& hash) {
        policy_.whitelistedHashes.push_back(hash);
        verifier_.updatePolicy(policy_);
    }
    
    void addBlacklistedHash(const std::string& hash) {
        policy_.blacklistedHashes.push_back(hash);
        verifier_.updatePolicy(policy_);
    }
    
private:
    PluginSecurityPolicy policy_;
    PluginSecurityVerifier verifier_;
};

// ============================================================================
// ModuleSecurityVerifier Implementation
// ============================================================================

ModuleSecurityVerifier::ModuleSecurityVerifier()
    : impl_(std::make_unique<Impl>()) {
}

ModuleSecurityVerifier::~ModuleSecurityVerifier() = default;

bool ModuleSecurityVerifier::verifyModule(const std::string& modulePath, std::string& errorMessage) {
    return impl_->verifyModule(modulePath, errorMessage);
}

std::string ModuleSecurityVerifier::calculateFileHash(const std::string& modulePath) {
    return impl_->calculateFileHash(modulePath);
}

void ModuleSecurityVerifier::setRequireSignature(bool require) {
    impl_->setRequireSignature(require);
}

void ModuleSecurityVerifier::setAllowUnsigned(bool allow) {
    impl_->setAllowUnsigned(allow);
}

void ModuleSecurityVerifier::addWhitelistedHash(const std::string& hash) {
    impl_->addWhitelistedHash(hash);
}

void ModuleSecurityVerifier::addBlacklistedHash(const std::string& hash) {
    impl_->addBlacklistedHash(hash);
}

// ============================================================================
// ModuleLoader Implementation
// ============================================================================

ModuleLoader::ModuleLoader()
    : verifier_(std::make_unique<ModuleSecurityVerifier>()) {
    spdlog::info("ModuleLoader initialized");
}

ModuleLoader::~ModuleLoader() {
    unloadAllModules();
}

void* ModuleLoader::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

void ModuleLoader::unloadLibrary(void* handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* ModuleLoader::getSymbol(void* handle, const std::string& symbolName) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
#else
    return dlsym(handle, symbolName.c_str());
#endif
}

std::string ModuleLoader::getModuleNameFromPath(const std::string& path) {
    std::filesystem::path p(path);
    std::string filename = p.stem().string();
    
    // Remove "lib" prefix on Unix systems
#ifndef _WIN32
    if (filename.rfind("lib", 0) == 0) {
        filename = filename.substr(3);
    }
#endif
    
    return filename;
}

bool ModuleLoader::isThemisModule(const std::string& filename) {
    // Check if filename starts with "themis_" or "libthemis_"
    return (filename.rfind("themis_", 0) == 0 || 
            filename.rfind("libthemis_", 0) == 0);
}

ModuleVerificationResult ModuleLoader::loadModule(const std::string& modulePath, 
                                                 const std::string& moduleName) {
    ModuleVerificationResult result;
    result.modulePath = modulePath;
    result.verificationTimestamp = static_cast<uint64_t>(std::time(nullptr));
    
    spdlog::info("Loading module: {} from {}", moduleName, modulePath);
    
    // Step 1: Check if already loaded
    if (isModuleLoaded(moduleName)) {
        result.errorMessage = "Module already loaded: " + moduleName;
        spdlog::warn("{}", result.errorMessage);
        result.success = false;
        return result;
    }
    
    // Step 2: Verify module exists
    if (!std::filesystem::exists(modulePath)) {
        result.errorMessage = "Module file not found: " + modulePath;
        spdlog::error("{}", result.errorMessage);
        result.success = false;
        return result;
    }
    
    // Step 3: SECURITY - Verify module signature and integrity
    std::string errorMessage;
    if (!verifier_->verifyModule(modulePath, errorMessage)) {
        result.errorMessage = "Module verification failed: " + errorMessage;
        spdlog::critical("SECURITY: {}", result.errorMessage);
        result.success = false;
        
        // Log to security auditor
        auto& auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
            modulePath,
            verifier_->calculateFileHash(modulePath),
            errorMessage,
            result.verificationTimestamp,
            "CRITICAL"
        });
        
        return result;
    }
    
    // Step 4: Calculate and store file hash
    result.moduleHash = verifier_->calculateFileHash(modulePath);
    
    // Step 5: Load the module library
    void* handle = loadLibrary(modulePath);
    if (!handle) {
        result.errorMessage = "Failed to load module library: " + modulePath;
#ifndef _WIN32
        result.errorMessage += " - " + std::string(dlerror());
#endif
        spdlog::error("{}", result.errorMessage);
        result.success = false;
        return result;
    }
    
    // Step 6: Store module info
    LoadedModule module;
    module.name = moduleName;
    module.path = modulePath;
    module.fileHash = result.moduleHash;
    module.handle = handle;
    module.verified = true;
    module.loadTime = result.verificationTimestamp;
    
    // TODO: Extract version from module metadata
    module.version = "unknown";
    
    loadedModules_.push_back(module);
    ModuleRegistry::instance().registerModule(module);
    
    spdlog::info("Module loaded successfully: {} (hash: {})", moduleName, result.moduleHash);
    
    result.success = true;
    return result;
}

size_t ModuleLoader::loadAllModules(const std::string& moduleDirectory) {
    spdlog::info("Loading all modules from: {}", moduleDirectory);
    
    if (!std::filesystem::exists(moduleDirectory)) {
        spdlog::error("Module directory not found: {}", moduleDirectory);
        return 0;
    }
    
    size_t loadedCount = 0;
    
    for (const auto& entry : std::filesystem::directory_iterator(moduleDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        
        std::string filename = entry.path().filename().string();
        
        // Check if it's a ThemisDB module
#ifdef _WIN32
        // Windows: themis_*.dll
        if (filename.rfind("themis_", 0) != 0 || 
            entry.path().extension() != ".dll") {
            continue;
        }
#else
        // Unix: libthemis_*.so
        if (filename.rfind("libthemis_", 0) != 0 || 
            entry.path().extension() != ".so") {
            continue;
        }
#endif
        
        std::string moduleName = getModuleNameFromPath(entry.path().string());
        auto result = loadModule(entry.path().string(), moduleName);
        
        if (result.success) {
            loadedCount++;
        }
    }
    
    spdlog::info("Loaded {}/{} modules from {}", loadedCount, 
                 std::distance(std::filesystem::directory_iterator(moduleDirectory),
                              std::filesystem::directory_iterator{}),
                 moduleDirectory);
    
    return loadedCount;
}

void ModuleLoader::unloadModule(const std::string& moduleName) {
    auto it = std::find_if(loadedModules_.begin(), loadedModules_.end(),
                          [&moduleName](const LoadedModule& m) { return m.name == moduleName; });
    
    if (it != loadedModules_.end()) {
        spdlog::info("Unloading module: {}", moduleName);
        unloadLibrary(it->handle);
        ModuleRegistry::instance().unregisterModule(moduleName);
        loadedModules_.erase(it);
    }
}

void ModuleLoader::unloadAllModules() {
    spdlog::info("Unloading all modules ({} loaded)", loadedModules_.size());
    
    for (auto& module : loadedModules_) {
        unloadLibrary(module.handle);
        ModuleRegistry::instance().unregisterModule(module.name);
    }
    
    loadedModules_.clear();
}

bool ModuleLoader::isModuleLoaded(const std::string& moduleName) const {
    return std::find_if(loadedModules_.begin(), loadedModules_.end(),
                       [&moduleName](const LoadedModule& m) { return m.name == moduleName; })
           != loadedModules_.end();
}

std::optional<LoadedModule> ModuleLoader::getModuleInfo(const std::string& moduleName) const {
    auto it = std::find_if(loadedModules_.begin(), loadedModules_.end(),
                          [&moduleName](const LoadedModule& m) { return m.name == moduleName; });
    
    if (it != loadedModules_.end()) {
        return *it;
    }
    
    return std::nullopt;
}

std::vector<LoadedModule> ModuleLoader::getAllLoadedModules() const {
    return loadedModules_;
}

void ModuleLoader::setRequireSignature(bool require) {
    verifier_->setRequireSignature(require);
}

void ModuleLoader::setAllowUnsigned(bool allow) {
    verifier_->setAllowUnsigned(allow);
}

void ModuleLoader::addWhitelistedHash(const std::string& hash) {
    verifier_->addWhitelistedHash(hash);
}

void ModuleLoader::addBlacklistedHash(const std::string& hash) {
    verifier_->addBlacklistedHash(hash);
}

bool ModuleLoader::exportAuditLog(const std::string& outputPath) const {
    auto& auditor = PluginSecurityAuditor::instance();
    return auditor.exportEvents(outputPath);
}

// ============================================================================
// ModuleRegistry Implementation
// ============================================================================

ModuleRegistry& ModuleRegistry::instance() {
    static ModuleRegistry registry;
    return registry;
}

void ModuleRegistry::registerModule(const LoadedModule& module) {
    modules_.push_back(module);
    spdlog::debug("Module registered: {}", module.name);
}

void ModuleRegistry::unregisterModule(const std::string& moduleName) {
    auto it = std::find_if(modules_.begin(), modules_.end(),
                          [&moduleName](const LoadedModule& m) { return m.name == moduleName; });
    
    if (it != modules_.end()) {
        modules_.erase(it);
        spdlog::debug("Module unregistered: {}", moduleName);
    }
}

bool ModuleRegistry::isRegistered(const std::string& moduleName) const {
    return std::find_if(modules_.begin(), modules_.end(),
                       [&moduleName](const LoadedModule& m) { return m.name == moduleName; })
           != modules_.end();
}

std::vector<LoadedModule> ModuleRegistry::getAllModules() const {
    return modules_;
}

void ModuleRegistry::clear() {
    modules_.clear();
}

} // namespace modules
} // namespace themis
