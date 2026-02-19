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
    auto startTime = std::chrono::steady_clock::now();
    
    ModuleVerificationResult result;
    result.modulePath = modulePath;
    result.verificationTimestamp = static_cast<uint64_t>(std::time(nullptr));
    
    spdlog::info("Loading module: {} from {}", moduleName, modulePath);
    
    // Step 1: Check if already loaded
    if (isModuleLoaded(moduleName)) {
        result.errorCode = ModuleErrorCode::MODULE_ALREADY_LOADED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage = getErrorMessage(result.errorCode) + ": " + moduleName;
        spdlog::warn("{}", result.errorMessage);
        result.success = false;
        return result;
    }
    
    // Step 2: Verify module exists
    if (!std::filesystem::exists(modulePath)) {
        result.errorCode = ModuleErrorCode::MODULE_NOT_FOUND;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage = getErrorMessage(result.errorCode) + ": " + modulePath;
        spdlog::error("{}", result.errorMessage);
        result.success = false;
        return result;
    }
    
    // Step 3: Extract metadata (before security verification)
    // TODO: Optimize to avoid double-loading: Currently loads module twice (once here for
    // metadata, once later for actual use). Consider: 1) Extract metadata after main load,
    // 2) Cache metadata, or 3) Use platform-specific binary inspection without loading
    result.metadata = extractModuleMetadata(modulePath);
    if (!result.metadata.isValid()) {
        spdlog::warn("Module metadata invalid or missing for: {}", modulePath);
        // Set safe fallback values for compatibility
        // Note: Modules without metadata will show version "unversioned"
        // This allows loading but signals missing version info to operators
        result.metadata.version = "unversioned";
        result.metadata.abiVersion = "unknown";
    } else {
        spdlog::info("Module metadata: version={}, abi={}, themis={}.{}.{}", 
                     result.metadata.version, result.metadata.abiVersion,
                     result.metadata.themisMajor, result.metadata.themisMinor, result.metadata.themisPatch);
    }
    
    // Step 4: SECURITY - Verify module signature and integrity
    std::string errorMessage;
    if (!verifier_->verifyModule(modulePath, errorMessage)) {
        result.errorCode = ModuleErrorCode::VERIFICATION_FAILED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage = getErrorMessage(result.errorCode) + ": " + errorMessage;
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
    
    // Step 6: Load the module library
    void* handle = loadLibrary(modulePath);
    if (!handle) {
        result.errorCode = ModuleErrorCode::LOAD_LIBRARY_FAILED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage = getErrorMessage(result.errorCode) + ": " + modulePath;
#ifndef _WIN32
        result.errorMessage += " - " + std::string(dlerror());
#endif
        spdlog::error("{}", result.errorMessage);
        result.success = false;
        return result;
    }
    
    // Step 7: Calculate load duration
    auto endTime = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Step 8: Store module info
    LoadedModule module;
    module.name = moduleName;
    module.path = modulePath;
    module.fileHash = result.moduleHash;
    module.handle = handle;
    module.verified = true;
    module.loadTime = result.verificationTimestamp;
    module.loadDurationMs = static_cast<uint64_t>(durationMs);
    module.metadata = result.metadata;
    module.version = result.metadata.version;
    
    loadedModules_.push_back(module);
    ModuleRegistry::instance().registerModule(module);
    
    spdlog::info("Module loaded successfully: {} (version: {}, hash: {}, duration: {}ms)", 
                 moduleName, module.version, result.moduleHash, durationMs);
    
    result.success = true;
    result.errorCode = ModuleErrorCode::SUCCESS;
    result.errorCategory = ErrorCategory::PERMANENT;
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

// ============================================================================
// Helper methods for error handling and metadata extraction
// ============================================================================

std::string ModuleLoader::getErrorMessage(ModuleErrorCode code) const {
    switch (code) {
        case ModuleErrorCode::SUCCESS:
            return "Success";
        case ModuleErrorCode::MODULE_NOT_FOUND:
            return "Module file not found";
        case ModuleErrorCode::MODULE_ALREADY_LOADED:
            return "Module is already loaded";
        case ModuleErrorCode::MODULE_DIRECTORY_NOT_FOUND:
            return "Module directory not found";
        case ModuleErrorCode::MODULE_ACCESS_DENIED:
            return "Access denied to module file";
        case ModuleErrorCode::VERIFICATION_FAILED:
            return "Module verification failed";
        case ModuleErrorCode::SIGNATURE_INVALID:
            return "Invalid or missing signature";
        case ModuleErrorCode::HASH_MISMATCH:
            return "Hash verification failed";
        case ModuleErrorCode::CERTIFICATE_REVOKED:
            return "Certificate has been revoked";
        case ModuleErrorCode::CERTIFICATE_EXPIRED:
            return "Certificate has expired";
        case ModuleErrorCode::UNTRUSTED_SIGNER:
            return "Untrusted certificate signer";
        case ModuleErrorCode::LOAD_LIBRARY_FAILED:
            return "Failed to load module library";
        case ModuleErrorCode::SYMBOL_NOT_FOUND:
            return "Required symbol not found in module";
        case ModuleErrorCode::INITIALIZATION_FAILED:
            return "Module initialization failed";
        case ModuleErrorCode::VERSION_INCOMPATIBLE:
            return "Module version incompatible with ThemisDB";
        case ModuleErrorCode::ABI_INCOMPATIBLE:
            return "Module ABI incompatible with ThemisDB";
        case ModuleErrorCode::METADATA_MISSING:
            return "Module metadata is missing";
        case ModuleErrorCode::METADATA_CORRUPTED:
            return "Module metadata is corrupted";
        case ModuleErrorCode::POLICY_VIOLATION:
            return "Module violates security policy";
        case ModuleErrorCode::BLACKLISTED:
            return "Module is blacklisted";
        case ModuleErrorCode::QUARANTINED:
            return "Module is quarantined due to repeated failures";
        case ModuleErrorCode::INTERNAL_ERROR:
            return "Internal module loader error";
        case ModuleErrorCode::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

ErrorCategory ModuleLoader::categorizeError(ModuleErrorCode code) const {
    switch (code) {
        // Transient errors - may succeed on retry
        case ModuleErrorCode::MODULE_ACCESS_DENIED:
        case ModuleErrorCode::LOAD_LIBRARY_FAILED:
            return ErrorCategory::TRANSIENT;
            
        // Recoverable errors - user can fix
        case ModuleErrorCode::MODULE_NOT_FOUND:
        case ModuleErrorCode::MODULE_DIRECTORY_NOT_FOUND:
        case ModuleErrorCode::VERSION_INCOMPATIBLE:
        case ModuleErrorCode::ABI_INCOMPATIBLE:
        case ModuleErrorCode::METADATA_MISSING:
            return ErrorCategory::RECOVERABLE;
            
        // Fatal errors - require system intervention
        case ModuleErrorCode::VERIFICATION_FAILED:
        case ModuleErrorCode::SIGNATURE_INVALID:
        case ModuleErrorCode::HASH_MISMATCH:
        case ModuleErrorCode::CERTIFICATE_REVOKED:
        case ModuleErrorCode::CERTIFICATE_EXPIRED:
        case ModuleErrorCode::UNTRUSTED_SIGNER:
        case ModuleErrorCode::BLACKLISTED:
        case ModuleErrorCode::QUARANTINED:
        case ModuleErrorCode::POLICY_VIOLATION:
            return ErrorCategory::FATAL;
            
        // Permanent errors - retry won't help
        case ModuleErrorCode::MODULE_ALREADY_LOADED:
        case ModuleErrorCode::SYMBOL_NOT_FOUND:
        case ModuleErrorCode::METADATA_CORRUPTED:
        case ModuleErrorCode::INITIALIZATION_FAILED:
        default:
            return ErrorCategory::PERMANENT;
    }
}

ModuleMetadata ModuleLoader::extractModuleMetadata(const std::string& modulePath) {
    ModuleMetadata metadata;
    
    // Try to load metadata from module itself
    // For now, check if module exports standard version symbols
    void* tempHandle = loadLibrary(modulePath);
    if (!tempHandle) {
        spdlog::warn("Could not load module temporarily for metadata extraction: {}", modulePath);
        // Return empty metadata - caller should check isValid()
        return metadata;
    }
    
    // Try to get version information via exported symbols
    // Common pattern: themis_module_version, themis_module_abi, etc.
    typedef const char* (*GetVersionFunc)();
    typedef uint32_t (*GetVersionIntFunc)();
    
    auto getVersionStr = reinterpret_cast<GetVersionFunc>(getSymbol(tempHandle, "themis_module_version"));
    auto getAbiVersion = reinterpret_cast<GetVersionFunc>(getSymbol(tempHandle, "themis_module_abi_version"));
    auto getBuildId = reinterpret_cast<GetVersionFunc>(getSymbol(tempHandle, "themis_module_build_id"));
    auto getMajor = reinterpret_cast<GetVersionIntFunc>(getSymbol(tempHandle, "themis_api_version_major"));
    auto getMinor = reinterpret_cast<GetVersionIntFunc>(getSymbol(tempHandle, "themis_api_version_minor"));
    auto getPatch = reinterpret_cast<GetVersionIntFunc>(getSymbol(tempHandle, "themis_api_version_patch"));
    
    if (getVersionStr) {
        metadata.version = getVersionStr();
    }
    if (getAbiVersion) {
        metadata.abiVersion = getAbiVersion();
    }
    if (getBuildId) {
        metadata.buildId = getBuildId();
    }
    if (getMajor) {
        metadata.themisMajor = getMajor();
    }
    if (getMinor) {
        metadata.themisMinor = getMinor();
    }
    if (getPatch) {
        metadata.themisPatch = getPatch();
    }
    
    unloadLibrary(tempHandle);
    
    // Note: If no version symbols found, metadata will be invalid (isValid() == false)
    // Caller should handle missing metadata gracefully
    if (metadata.version.empty()) {
        spdlog::debug("No version symbol found in module: {}", modulePath);
    } else {
        spdlog::debug("Extracted metadata from {}: version={}, abi={}, buildId={}", 
                      modulePath, metadata.version, metadata.abiVersion, metadata.buildId);
    }
    
    return metadata;
}

void ModuleRegistry::clear() {
    modules_.clear();
}

} // namespace modules
} // namespace themis
