/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_loader.cpp                                  ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   79.0/100                                       ║
    • Total Lines:     1109                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Module loader implementation with DLL signature verification
// This prevents corrupted or malicious DLL loading in modular ThemisDB

#include "themis/base/module_loader.h"
#include "acceleration/plugin_security.h"
#include <filesystem>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
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
    
    // Step 1: Check quarantine and backoff
    if (checkQuarantine(modulePath, result)) {
        // Module is quarantined or in backoff - update metrics and return
        updateMetrics(false, 0, result.errorCode);
        return result;
    }
    
    // Step 2: Check if already loaded
    if (isModuleLoaded(moduleName)) {
        result.errorCode = ModuleErrorCode::MODULE_ALREADY_LOADED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage = getErrorMessage(result.errorCode) + ": " + moduleName;
        spdlog::warn("{}", result.errorMessage);
        result.success = false;
        updateMetrics(false, 0, result.errorCode);
        return result;
    }
    
    // Step 3: Verify module exists
    if (!std::filesystem::exists(modulePath)) {
        result.errorCode = ModuleErrorCode::MODULE_NOT_FOUND;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage = getErrorMessage(result.errorCode) + ": " + modulePath;
        spdlog::error("{}", result.errorMessage);
        result.success = false;
        recordFailure(modulePath, result.errorCode, result.errorMessage);
        updateMetrics(false, 0, result.errorCode);
        return result;
    }
    
    // Step 4: STAGED LOADING - Use cached metadata if available (optimization)
    // This eliminates the double-loading issue
    result.metadata = getCachedMetadata(modulePath);
    if (!result.metadata.isValid()) {
        spdlog::warn("Module metadata invalid or missing for: {}", modulePath);
        // Will extract after loading from handle
        result.metadata.version = "unversioned";
        result.metadata.abiVersion = "unknown";
    } else {
        spdlog::info("Module metadata: version={}, abi={}, themis={}.{}.{}", 
                     result.metadata.version, result.metadata.abiVersion,
                     result.metadata.themisMajor, result.metadata.themisMinor, result.metadata.themisPatch);
        
        // STAGED: Validation stage - Check ABI compatibility
        if (stagedLoadingEnabled_) {
            spdlog::debug("STAGE: VALIDATING - {}", moduleName);
        }
        
        if (!isABICompatible(result.metadata)) {
            result.errorCode = ModuleErrorCode::ABI_INCOMPATIBLE;
            result.errorCategory = categorizeError(result.errorCode);
            result.errorMessage = getErrorMessage(result.errorCode) + 
                                 ": module " + std::to_string(result.metadata.themisMajor) + "." +
                                 std::to_string(result.metadata.themisMinor) + 
                                 ", themis " + std::to_string(themisABIMajor_) + "." +
                                 std::to_string(themisABIMinor_);
            spdlog::error("{}", result.errorMessage);
            result.success = false;
            recordFailure(modulePath, result.errorCode, result.errorMessage);
            updateMetrics(false, 0, result.errorCode);
            return result;
        }
    }
    
    // STAGED: Verification stage
    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: VERIFYING - {}", moduleName);
    }
    
    // Step 5: SECURITY - Verify module signature and integrity
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
        
        // Record failure for quarantine tracking
        recordFailure(modulePath, result.errorCode, result.errorMessage);
        updateMetrics(false, 0, result.errorCode);
        return result;
    }
    
    // Verification succeeded - track it
    metrics_.verificationSuccesses++;
    
    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: VERIFIED - {}", moduleName);
    }
    
    // Step 6: Calculate and store file hash
    result.moduleHash = verifier_->calculateFileHash(modulePath);
    
    // STAGED: Staging phase - Load the module library
    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: STAGING - {}", moduleName);
    }
    
    // Step 7: Load the module library
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
        recordFailure(modulePath, result.errorCode, result.errorMessage);
        updateMetrics(false, 0, result.errorCode);
        return result;
    }
    
    // Step 7b: Extract metadata from loaded handle if not already valid
    // This optimizes by eliminating the double-load issue
    if (!result.metadata.isValid()) {
        result.metadata = extractMetadataFromHandle(handle);
        spdlog::info("Extracted metadata from handle: version={}", result.metadata.version);
        
        // Cache it for future use
        if (result.metadata.isValid()) {
            metadataCache_[modulePath] = result.metadata;
        }
    }
    
    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: STAGED - {}", moduleName);
    }
    
    // Step 8: Calculate load duration
    auto endTime = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Step 9: Create module info structure
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
    module.currentStage = stagedLoadingEnabled_ ? LoadStage::STAGING : LoadStage::ACTIVE;
    
    // STAGED: Activation stage - Run health checks if enabled
    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: ACTIVATING - {}", moduleName);
        
        if (!runHealthChecks(module, result)) {
            // Health check failed - unload and return error
            spdlog::error("Health checks failed for module: {}", moduleName);
            unloadLibrary(handle);
            recordFailure(modulePath, result.errorCode, result.errorMessage);
            updateMetrics(false, static_cast<uint64_t>(durationMs), result.errorCode);
            return result;
        }
        
        module.currentStage = LoadStage::ACTIVE;
        spdlog::info("STAGE: ACTIVE - {} (all health checks passed)", moduleName);
    }
    
    module.fullyActivated = true;
    
    loadedModules_.push_back(module);
    ModuleRegistry::instance().registerModule(module);
    
    // Success - clear failure history and update metrics
    clearFailureHistory(modulePath);
    updateMetrics(true, static_cast<uint64_t>(durationMs), ModuleErrorCode::SUCCESS);
    
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
        case ModuleErrorCode::HEALTH_CHECK_FAILED:
            return "Module health check failed";
        case ModuleErrorCode::STAGING_FAILED:
            return "Module staging failed";
        case ModuleErrorCode::ACTIVATION_FAILED:
            return "Module activation failed";
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

// ============================================================================
// Quarantine and Backoff Implementation
// ============================================================================

void ModuleLoader::recordFailure(const std::string& modulePath, ModuleErrorCode errorCode, const std::string& errorMessage) {
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    
    auto& history = failureHistory_[modulePath];
    history.modulePath = modulePath;
    history.failureTimestamps.push_back(currentTime);
    history.consecutiveFailures++;
    history.lastFailureTime = currentTime;
    history.lastErrorCode = errorCode;
    history.lastErrorMessage = errorMessage;
    
    // Calculate exponential backoff: 2^n seconds (1s, 2s, 4s, 8s, ...)
    history.nextRetryTime = currentTime + calculateBackoffTime(history.consecutiveFailures);
    
    spdlog::warn("Module failure recorded: {} (failures: {}, next retry: {}s)", 
                 modulePath, history.consecutiveFailures, 
                 calculateBackoffTime(history.consecutiveFailures));
    
    // Check if should be quarantined
    if (shouldQuarantine(modulePath)) {
        quarantineModule(modulePath);
    }
}

bool ModuleLoader::shouldQuarantine(const std::string& modulePath) const {
    auto it = failureHistory_.find(modulePath);
    if (it == failureHistory_.end()) {
        return false;
    }
    
    return it->second.consecutiveFailures >= quarantineThreshold_;
}

void ModuleLoader::quarantineModule(const std::string& modulePath) {
    auto it = failureHistory_.find(modulePath);
    if (it == failureHistory_.end()) {
        return;
    }
    
    if (it->second.quarantineTime > 0) {
        return;  // Already quarantined
    }
    
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    it->second.quarantineTime = currentTime;
    
    metrics_.quarantineEvents++;
    metrics_.currentlyQuarantined++;
    
    spdlog::critical("QUARANTINE: Module quarantined after {} consecutive failures: {} (last error: {})",
                     it->second.consecutiveFailures, modulePath, it->second.lastErrorMessage);
}

uint64_t ModuleLoader::calculateBackoffTime(uint32_t consecutiveFailures) const {
    // Exponential backoff: 2^(n-1) seconds, capped at maxBackoffSeconds_
    // Use bit shifting for efficiency instead of std::pow
    if (consecutiveFailures == 0) {
        return 0;
    }
    
    // Prevent overflow for very large failure counts
    if (consecutiveFailures > 32) {
        return maxBackoffSeconds_;
    }
    
    uint64_t backoff = 1ULL << (consecutiveFailures - 1);
    return std::min(backoff, static_cast<uint64_t>(maxBackoffSeconds_));
}

bool ModuleLoader::checkQuarantine(const std::string& modulePath, ModuleVerificationResult& result) {
    auto it = failureHistory_.find(modulePath);
    if (it == failureHistory_.end()) {
        return false;  // No failure history
    }
    
    auto& history = it->second;
    
    // Check if quarantined
    if (history.isQuarantined()) {
        result.success = false;
        result.errorCode = ModuleErrorCode::QUARANTINED;
        result.errorCategory = ErrorCategory::FATAL;
        result.errorMessage = getErrorMessage(ModuleErrorCode::QUARANTINED) + 
                             ": " + modulePath + 
                             " (failures: " + std::to_string(history.consecutiveFailures) + ")";
        spdlog::error("{}", result.errorMessage);
        return true;  // Module is quarantined
    }
    
    // Check backoff
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    if (!history.canRetry(currentTime)) {
        result.success = false;
        result.errorCode = ModuleErrorCode::POLICY_VIOLATION;
        result.errorCategory = ErrorCategory::TRANSIENT;
        uint64_t waitTime = history.nextRetryTime - currentTime;
        result.errorMessage = "Module in backoff period, retry in " + 
                             std::to_string(waitTime) + " seconds: " + modulePath;
        spdlog::warn("{}", result.errorMessage);
        return true;  // In backoff period
    }
    
    return false;  // Can proceed with load
}

std::optional<ModuleFailureHistory> ModuleLoader::getFailureHistory(const std::string& modulePath) const {
    auto it = failureHistory_.find(modulePath);
    if (it == failureHistory_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> ModuleLoader::getQuarantinedModules() const {
    std::vector<std::string> quarantined;
    for (const auto& [path, history] : failureHistory_) {
        if (history.isQuarantined()) {
            quarantined.push_back(path);
        }
    }
    return quarantined;
}

bool ModuleLoader::releaseFromQuarantine(const std::string& modulePath) {
    auto it = failureHistory_.find(modulePath);
    if (it == failureHistory_.end() || !it->second.isQuarantined()) {
        return false;
    }
    
    it->second.quarantineTime = 0;
    it->second.consecutiveFailures = 0;
    it->second.nextRetryTime = 0;
    
    metrics_.quarantineReleases++;
    metrics_.currentlyQuarantined--;
    
    spdlog::info("Module released from quarantine: {}", modulePath);
    return true;
}

void ModuleLoader::clearFailureHistory(const std::string& modulePath) {
    auto it = failureHistory_.find(modulePath);
    if (it != failureHistory_.end()) {
        if (it->second.isQuarantined()) {
            metrics_.currentlyQuarantined--;
        }
        failureHistory_.erase(it);
        spdlog::info("Failure history cleared for module: {}", modulePath);
    }
}

void ModuleLoader::setQuarantineThreshold(uint32_t threshold) {
    quarantineThreshold_ = threshold;
    spdlog::info("Quarantine threshold set to: {}", threshold);
}

void ModuleLoader::setMaxBackoffSeconds(uint32_t maxSeconds) {
    maxBackoffSeconds_ = maxSeconds;
    spdlog::info("Max backoff time set to: {} seconds", maxSeconds);
}

// ============================================================================
// ABI Compatibility Implementation
// ============================================================================

bool ModuleLoader::isABICompatible(const ModuleMetadata& metadata) const {
    // Check if metadata is valid
    if (!metadata.isValid()) {
        spdlog::warn("Cannot check ABI compatibility: metadata invalid");
        return false;
    }
    
    // ABI compatibility rules:
    // 1. Major version must match exactly
    // 2. Module minor version must be <= ThemisDB minor version
    
    if (metadata.themisMajor != themisABIMajor_) {
        spdlog::error("ABI incompatible: major version mismatch (module: {}, themis: {})",
                      metadata.themisMajor, themisABIMajor_);
        return false;
    }
    
    if (metadata.themisMinor > themisABIMinor_) {
        spdlog::error("ABI incompatible: module minor version too new (module: {}, themis: {})",
                      metadata.themisMinor, themisABIMinor_);
        return false;
    }
    
    spdlog::debug("ABI compatible: module {}.{}.{} with themis {}.{}", 
                  metadata.themisMajor, metadata.themisMinor, metadata.themisPatch,
                  themisABIMajor_, themisABIMinor_);
    
    return true;
}

// ============================================================================
// Metrics Implementation
// ============================================================================

void ModuleLoader::updateMetrics(bool success, uint64_t durationMs, ModuleErrorCode errorCode) {
    metrics_.totalLoadAttempts++;
    
    if (success) {
        metrics_.successfulLoads++;
        metrics_.totalLoadDurationMs += durationMs;
        metrics_.minLoadDurationMs = std::min(metrics_.minLoadDurationMs, durationMs);
        metrics_.maxLoadDurationMs = std::max(metrics_.maxLoadDurationMs, durationMs);
        // Note: verificationSuccesses is tracked separately in loadModule when verification passes
    } else {
        metrics_.failedLoads++;
        metrics_.errorCounts[errorCode]++;
        
        // Only count verification failures, not all failures
        if (errorCode == ModuleErrorCode::VERIFICATION_FAILED ||
            errorCode == ModuleErrorCode::SIGNATURE_INVALID ||
            errorCode == ModuleErrorCode::HASH_MISMATCH ||
            errorCode == ModuleErrorCode::CERTIFICATE_REVOKED ||
            errorCode == ModuleErrorCode::CERTIFICATE_EXPIRED ||
            errorCode == ModuleErrorCode::UNTRUSTED_SIGNER) {
            metrics_.verificationFailures++;
        }
    }
}

ModuleMetrics ModuleLoader::getMetrics() const {
    return metrics_;
}

void ModuleLoader::resetMetrics() {
    metrics_ = ModuleMetrics();
    spdlog::info("Module loader metrics reset");
}

// ============================================================================
// Staged Loading Implementation (Phase 3)
// ============================================================================

void ModuleLoader::registerHealthCheck(const std::string& checkName, HealthCheckFunction checkFunc) {
    healthChecks_[checkName] = checkFunc;
    spdlog::info("Health check registered: {}", checkName);
}

void ModuleLoader::clearHealthChecks() {
    healthChecks_.clear();
    spdlog::info("All health checks cleared");
}

void ModuleLoader::setStagedLoadingEnabled(bool enable) {
    stagedLoadingEnabled_ = enable;
    spdlog::info("Staged loading {}", enable ? "enabled" : "disabled");
}

std::optional<LoadStage> ModuleLoader::queryModuleStage(const std::string& moduleName) const {
    auto it = std::find_if(loadedModules_.begin(), loadedModules_.end(),
                          [&moduleName](const LoadedModule& m) { return m.name == moduleName; });
    
    if (it == loadedModules_.end()) {
        return std::nullopt;
    }
    
    return it->currentStage;
}

std::vector<HealthCheckResult> ModuleLoader::getHealthCheckResults(const std::string& moduleName) const {
    auto it = std::find_if(loadedModules_.begin(), loadedModules_.end(),
                          [&moduleName](const LoadedModule& m) { return m.name == moduleName; });
    
    if (it == loadedModules_.end()) {
        return {};
    }
    
    return it->healthChecks;
}

bool ModuleLoader::updateModuleStage(const std::string& moduleName, LoadStage newStage) {
    auto it = std::find_if(loadedModules_.begin(), loadedModules_.end(),
                          [&moduleName](LoadedModule& m) { return m.name == moduleName; });
    
    if (it == loadedModules_.end()) {
        return false;
    }
    
    it->currentStage = newStage;
    spdlog::debug("Module {} stage updated to {}", moduleName, static_cast<int>(newStage));
    return true;
}

bool ModuleLoader::runHealthChecks(LoadedModule& module, ModuleVerificationResult& result) {
    if (healthChecks_.empty()) {
        spdlog::debug("No health checks registered for module: {}", module.name);
        return true;  // No health checks = pass
    }
    
    spdlog::info("Running {} health checks for module: {}", healthChecks_.size(), module.name);
    
    for (const auto& [checkName, checkFunc] : healthChecks_) {
        auto startTime = std::chrono::steady_clock::now();
        
        try {
            auto healthResult = checkFunc(module.handle, module.name);
            
            auto endTime = std::chrono::steady_clock::now();
            healthResult.checkDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - startTime).count();
            
            module.healthChecks.push_back(healthResult);
            
            if (!healthResult.passed) {
                result.errorCode = ModuleErrorCode::HEALTH_CHECK_FAILED;
                result.errorCategory = ErrorCategory::RECOVERABLE;
                result.errorMessage = "Health check failed: " + checkName + " - " + healthResult.message;
                spdlog::error("{}", result.errorMessage);
                return false;
            }
            
            spdlog::info("Health check passed: {} ({}ms)", checkName, healthResult.checkDurationMs);
        } catch (const std::exception& e) {
            auto healthResult = HealthCheckResult::failure(checkName, 
                "Exception during health check: " + std::string(e.what()));
            module.healthChecks.push_back(healthResult);
            
            result.errorCode = ModuleErrorCode::HEALTH_CHECK_FAILED;
            result.errorCategory = ErrorCategory::FATAL;
            result.errorMessage = "Health check exception: " + checkName + " - " + std::string(e.what());
            spdlog::critical("{}", result.errorMessage);
            return false;
        }
    }
    
    spdlog::info("All health checks passed for module: {}", module.name);
    return true;
}

ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
    ModuleMetadata metadata;
    
    if (!handle) {
        return metadata;
    }
    
    // Extract version information from already-loaded handle
    typedef const char* (*GetVersionFunc)();
    typedef uint32_t (*GetVersionIntFunc)();
    
    auto getVersionStr = reinterpret_cast<GetVersionFunc>(getSymbol(handle, "themis_module_version"));
    auto getAbiVersion = reinterpret_cast<GetVersionFunc>(getSymbol(handle, "themis_module_abi_version"));
    auto getBuildId = reinterpret_cast<GetVersionFunc>(getSymbol(handle, "themis_module_build_id"));
    auto getMajor = reinterpret_cast<GetVersionIntFunc>(getSymbol(handle, "themis_api_version_major"));
    auto getMinor = reinterpret_cast<GetVersionIntFunc>(getSymbol(handle, "themis_api_version_minor"));
    auto getPatch = reinterpret_cast<GetVersionIntFunc>(getSymbol(handle, "themis_api_version_patch"));
    
    if (getVersionStr) metadata.version = getVersionStr();
    if (getAbiVersion) metadata.abiVersion = getAbiVersion();
    if (getBuildId) metadata.buildId = getBuildId();
    if (getMajor) metadata.themisMajor = getMajor();
    if (getMinor) metadata.themisMinor = getMinor();
    if (getPatch) metadata.themisPatch = getPatch();
    
    return metadata;
}

ModuleMetadata ModuleLoader::getCachedMetadata(const std::string& modulePath) {
    // Check cache first
    auto it = metadataCache_.find(modulePath);
    if (it != metadataCache_.end()) {
        spdlog::debug("Using cached metadata for: {}", modulePath);
        return it->second;
    }
    
    // Not in cache - extract and cache it
    auto metadata = extractModuleMetadata(modulePath);
    if (metadata.isValid()) {
        metadataCache_[modulePath] = metadata;
        spdlog::debug("Cached metadata for: {}", modulePath);
    }
    
    return metadata;
}

void ModuleRegistry::clear() {
    modules_.clear();
}

} // namespace modules
} // namespace themis
