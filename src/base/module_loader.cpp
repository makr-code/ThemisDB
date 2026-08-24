/**
 * @file module_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ============================================================
// DEPRECATED: This file (src/base/module_loader.cpp) is a legacy copy that
// was superseded by src/themis/module_loader.cpp in v1.7.0 as part of the
// modular build architecture migration.
//
// DO NOT add new code here.  All changes must be made in:
//   src/themis/module_loader.cpp  (canonical, v0.0.5+)
//
// This file is retained only to avoid breaking build targets that include it
// directly.  It will be removed in a future release once all CMake targets have
// been migrated to the src/themis/ build tree.
// ============================================================

// Module loader implementation with DLL signature verification
// This prevents corrupted or malicious DLL loading in modular ThemisDB

#include "themis/base/module_loader.h"
#include <stdexcept>
#include "acceleration/plugin_security.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <spdlog/spdlog.h>

#ifdef _WIN32
    #include <windows.h>
    #include <wintrust.h>
    #include <softpub.h>
    #include <wincrypt.h>
    #pragma comment(lib, "wintrust.lib")
    #pragma comment(lib, "crypt32.lib")
#else
    #include <dlfcn.h>
#endif

#ifdef __linux__
    #include <sys/xattr.h>
    #include <elf.h>
    #include <unistd.h>
    #include <sys/wait.h>
    #include <cerrno>
    #include <cstring>
#endif

namespace themis {
namespace modules {

using namespace themis::acceleration;

// ============================================================================
// ModuleSecurityVerifier::Impl - Bridge to existing plugin security
// ============================================================================

/** @brief ModuleSecurityVerifier::Impl - Bridge to existing plugin security. */
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
    stopWatchdog();
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
    
    // Step 3b: Zone.Identifier quarantine check (Windows only)
    // Reject modules marked with Zone ID >= 3 (Internet zone or Restricted Sites)
    // by the Windows Mark of the Web (MoTW) mechanism.  Such modules were
    // downloaded from an untrusted origin and must be explicitly unblocked
    // before they can be loaded.
#ifdef _WIN32
    {
        int zoneId = getZoneIdentifier(modulePath);
        result.zoneId = zoneId;
        if (zoneId >= 3) {
            const char* zoneName = (zoneId == 3) ? "Internet" : "Restricted Sites";
            result.errorCode = ModuleErrorCode::ZONE_ID_BLOCKED;
            result.errorCategory = categorizeError(result.errorCode);
            result.errorMessage = getErrorMessage(result.errorCode) +
                                  ": zone " + std::to_string(zoneId) +
                                  " (" + zoneName + ") for " + modulePath;
            spdlog::critical("SECURITY: {}", result.errorMessage);
            result.success = false;
            recordFailure(modulePath, result.errorCode, result.errorMessage);
            updateMetrics(false, 0, result.errorCode);
            return result;
        }
        if (zoneId >= 0) {
            const char* zoneName = (zoneId == 0) ? "Local Computer"
                                 : (zoneId == 1) ? "Local Intranet"
                                 :                 "Trusted Sites";
            spdlog::info("Zone.Identifier present for '{}': zone {} ({}) - permitted",
                         modulePath, zoneId, zoneName);
        }
    }
#endif

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
    
    // Step 6b: SHA-256 manifest check (Issue #2471)
    // If a hash manifest was loaded via setHashManifest(), verify the module's
    // computed hash matches the expected value.  Modules not listed in the
    // manifest pass through — the manifest is an integrity verification list
    // for known modules, not a global block/allowlist.
    if (hashVerifier_.manifestSize() > 0) {
        const auto expected = hashVerifier_.getExpectedHash(moduleName);
        if (expected.has_value()) {
            if (result.moduleHash != *expected) {
                result.errorCode = ModuleErrorCode::HASH_MISMATCH;
                result.errorCategory = categorizeError(result.errorCode);
                result.errorMessage =
                    "Integrity violation for module '" + moduleName +
                    "': expected SHA-256=" + *expected +
                    " got=" + result.moduleHash;
                spdlog::critical("SECURITY: {}", result.errorMessage);
                result.success = false;
                recordFailure(modulePath, result.errorCode, result.errorMessage);
                updateMetrics(false, 0, result.errorCode);
                return result;
            }
            spdlog::info("Hash manifest check PASSED for '{}' ({})",
                         moduleName, result.moduleHash);
        }
    }

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

    // Gap: resource_leaked_in_exception — RAII guard ensures the library handle
    // is released if any exception propagates before the handle is transferred
    // to loadedModules_.  Call guard.release() just before insert_or_assign so
    // the stored module owns the handle from that point onward.
    struct LibraryHandleGuard {
        void *h;
        ModuleLoader *loader;
        explicit LibraryHandleGuard(void *handle, ModuleLoader *l) : h(handle), loader(l) {}
        ~LibraryHandleGuard() { if (h) { loader->unloadLibrary(h); } }
        void release() { h = nullptr; }
    } handleGuard(handle, this);
    
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
            // Health check failed; handleGuard will call unloadLibrary(handle)
            // automatically when it goes out of scope — no explicit call needed.
            spdlog::error("Health checks failed for module: {}", moduleName);
            // Log health check / activation failure to per-plugin audit trail
            auto& auditor = PluginSecurityAuditor::instance();
            auditor.logEvent({
                PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
                modulePath,
                result.moduleHash,
                result.errorMessage,
                result.verificationTimestamp,
                "ERROR"
            });
            recordFailure(modulePath, result.errorCode, result.errorMessage);
            updateMetrics(false, static_cast<uint64_t>(durationMs), result.errorCode);
            return result;
        }
        
        module.currentStage = LoadStage::ACTIVE;
        spdlog::info("STAGE: ACTIVE - {} (all health checks passed)", moduleName);
    }
    
    module.fullyActivated = true;
    
    {
        std::unique_lock<std::shared_mutex> lk(modulesMutex_);
        // Transfer ownership of the handle to the module map; release the
        // RAII guard so it no longer calls unloadLibrary on destruction.
        handleGuard.release();
        loadedModules_.insert_or_assign(module.name, module);
    }
    ModuleRegistry::instance().registerModule(module);
    
    // Log successful load to per-plugin audit trail
    {
        auto& auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOADED,
            modulePath,
            result.moduleHash,
            "Module activated: " + moduleName +
                " (version=" + module.version + ")",
            result.verificationTimestamp,
            "INFO"
        });
    }

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
    std::unique_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);
    
    if (it != loadedModules_.end()) {
        spdlog::info("Unloading module: {}", moduleName);
        uint64_t now = static_cast<uint64_t>(std::time(nullptr));
        // Log unload event to per-plugin audit trail before releasing the handle
        auto& auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_UNLOADED,
            it->second.path,
            it->second.fileHash,
            "Module unloaded: " + moduleName,
            now,
            "INFO"
        });
        unloadLibrary(it->second.handle);
        loadedModules_.erase(it);
        ModuleRegistry::instance().unregisterModule(moduleName);
        metrics_.totalUnloads++;
    }
}

void ModuleLoader::unloadAllModules() {
    std::unique_lock<std::shared_mutex> lk(modulesMutex_);
    spdlog::info("Unloading all modules ({} loaded)", loadedModules_.size());
    
    auto& auditor = PluginSecurityAuditor::instance();
    uint64_t now = static_cast<uint64_t>(std::time(nullptr));
    for (auto& [name, module] : loadedModules_) {
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_UNLOADED,
            module.path,
            module.fileHash,
            "Module unloaded: " + module.name,
            now,
            "INFO"
        });
        unloadLibrary(module.handle);
        ModuleRegistry::instance().unregisterModule(module.name);
        metrics_.totalUnloads++;
    }
    
    loadedModules_.clear();
}

bool ModuleLoader::isModuleLoaded(const std::string& moduleName) const {
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    return loadedModules_.count(moduleName) > 0;
}

std::optional<LoadedModule> ModuleLoader::getModuleInfo(const std::string& moduleName) const {
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);
    
    if (it != loadedModules_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<LoadedModule> ModuleLoader::getAllLoadedModules() const {
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    std::vector<LoadedModule> result;
    result.reserve(loadedModules_.size());
    for (const auto& [name, module] : loadedModules_) {
        result.push_back(module);
    }
    std::sort(result.begin(), result.end(),
              [](const LoadedModule& a, const LoadedModule& b) {
                  return a.name < b.name;
              });
    return result;
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

bool ModuleLoader::setHashManifest(const std::string& manifestPath) {
    const bool ok = hashVerifier_.loadManifest(manifestPath);
    if (ok) {
        spdlog::info("ModuleLoader: loaded hash manifest from '{}' ({} entries)",
                     manifestPath, hashVerifier_.manifestSize());
    } else {
        spdlog::error("ModuleLoader: failed to load hash manifest from '{}'",
                      manifestPath);
    }
    return ok;
}

bool ModuleLoader::exportAuditLog(const std::string& outputPath) const {
    auto& auditor = PluginSecurityAuditor::instance();
    return auditor.exportEvents(outputPath);
}

std::vector<PluginSecurityEvent> ModuleLoader::getPluginAuditTrail(const std::string& modulePath) const {
    auto& auditor = PluginSecurityAuditor::instance();
    return auditor.getEventsForPlugin(modulePath);
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
        case ModuleErrorCode::ZONE_ID_BLOCKED:
            return "Module blocked: Windows Zone.Identifier marks it as downloaded from Internet or Restricted Sites";
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
        case ModuleErrorCode::ZONE_ID_BLOCKED:
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
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);
    
    if (it == loadedModules_.end()) {
        return std::nullopt;
    }
    
    return it->second.currentStage;
}

std::vector<HealthCheckResult> ModuleLoader::getHealthCheckResults(const std::string& moduleName) const {
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);
    
    if (it == loadedModules_.end()) {
        return std::vector<HealthCheckResult>{};
    }
    
    return it->second.healthChecks;
}

bool ModuleLoader::updateModuleStage(const std::string& moduleName, LoadStage newStage) {
    std::unique_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);
    
    if (it == loadedModules_.end()) {
        return false;
    }
    
    it->second.currentStage = newStage;
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

// ============================================================================
// Platform-Specific Signature Verification (Phase 4)
// ============================================================================

#ifdef _WIN32

static constexpr DWORD kZoneIdBufferSize  = 256;
static constexpr DWORD kCertNameBufferSize = 256;

int ModuleLoader::getZoneIdentifier(const std::string& modulePath) const {
    // Read NTFS Zone.Identifier alternate data stream
    std::string adsPath = modulePath + ":Zone.Identifier";
    HANDLE hFile = CreateFileA(adsPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        spdlog::debug("No Zone.Identifier ADS for: {}", modulePath);
        return -1;
    }

    char buffer[kZoneIdBufferSize] = {};
    DWORD bytesRead = 0;
    ReadFile(hFile, buffer, kZoneIdBufferSize - 1, &bytesRead, nullptr);
    CloseHandle(hFile);

    std::string content(buffer, bytesRead);
    // Zone.Identifier format: "[ZoneTransfer]\r\nZoneId=<N>"
    const std::string zoneIdKey = "ZoneId=";
    auto pos = content.find(zoneIdKey);
    if (pos == std::string::npos) {
        return -1;
    }
    try {
        return std::stoi(content.substr(pos + zoneIdKey.size()));
    } catch (const std::invalid_argument &) {
        return -1;
    } catch (const std::out_of_range &) {
        return -1;
    }
}

bool ModuleLoader::removeZoneIdentifier(const std::string& modulePath) {
    std::string adsPath = modulePath + ":Zone.Identifier";
    if (DeleteFileA(adsPath.c_str())) {
        spdlog::info("Removed Zone.Identifier ADS from: {}", modulePath);
        return true;
    }
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND) {
        // Already absent - treat as success
        return true;
    }
    spdlog::warn("Failed to remove Zone.Identifier from {}: error {}", modulePath, err);
    return false;
}

bool ModuleLoader::verifyAuthenticodeSignature(const std::string& modulePath,
                                               std::string& signerInfo) const {
    // Convert UTF-8 path to wide string for Windows API
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, modulePath.c_str(),
                                      static_cast<int>(modulePath.size()), nullptr, 0);
    if (wideLen == 0) {
        spdlog::error("verifyAuthenticodeSignature: path conversion failed for: {}", modulePath);
        return false;
    }
    std::wstring widePath(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, modulePath.c_str(),
                        static_cast<int>(modulePath.size()), &widePath[0], wideLen);

    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = widePath.c_str();

    GUID actionId = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA trustData = {};
    trustData.cbStruct          = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice        = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice     = WTD_CHOICE_FILE;
    trustData.pFile             = &fileInfo;
    trustData.dwStateAction     = WTD_STATEACTION_VERIFY;
    trustData.dwProvFlags       = WTD_SAFER_FLAG;

    LONG status = WinVerifyTrust(nullptr, &actionId, &trustData);

    // Retrieve signer subject CN when verification succeeded
    if (status == ERROR_SUCCESS) {
        CRYPT_PROVIDER_DATA* provData = WTHelperProvDataFromStateData(trustData.hWVTStateData);
        if (provData) {
            CRYPT_PROVIDER_SGNR* signer = WTHelperGetProvSignerFromChain(provData, 0, FALSE, 0);
            if (signer && signer->pChainContext &&
                signer->pChainContext->rgpChain &&
                signer->pChainContext->rgpChain[0]->rgpElement &&
                signer->pChainContext->rgpChain[0]->cElement > 0) {
                PCCERT_CONTEXT cert =
                    signer->pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;
                if (cert) {
                    char nameBuffer[kCertNameBufferSize] = {};
                    CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                                       nullptr, nameBuffer, kCertNameBufferSize);
                    signerInfo = nameBuffer;
                }
            }
        }
    }

    // Always close the state handle
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &actionId, &trustData);

    if (status == ERROR_SUCCESS) {
        spdlog::info("Authenticode verification PASSED for: {} (signer: {})",
                     modulePath, signerInfo);
        return true;
    }

    switch (status) {
        case TRUST_E_NOSIGNATURE:
            spdlog::warn("Authenticode: no signature on: {}", modulePath);
            break;
        case TRUST_E_EXPLICIT_DISTRUST:
            spdlog::error("Authenticode: signature explicitly distrusted: {}", modulePath);
            break;
        case TRUST_E_SUBJECT_NOT_TRUSTED:
            spdlog::error("Authenticode: subject not trusted: {}", modulePath);
            break;
        default:
            spdlog::error("Authenticode verification failed (code {}) for: {}", status, modulePath);
            break;
    }
    return false;
}
#endif // _WIN32

#ifdef __linux__

static constexpr size_t kGpgOutputBufferSize = 256;
static constexpr uint64_t kMaxCommentSectionSize = 4096;

bool ModuleLoader::verifyGPGSignature(const std::string& modulePath,
                                      const std::string& signaturePath) const {
    // Locate the detached signature file first (no character checks needed for
    // the filesystem lookup itself — the paths are not passed to a shell).
    std::string sigFile = signaturePath;
    if (sigFile.empty()) {
        for (const auto& ext : {".asc", ".sig", ".gpg"}) {
            std::string candidate = modulePath + ext;
            if (std::filesystem::exists(candidate)) {
                sigFile = candidate;
                break;
            }
        }
    }

    if (sigFile.empty()) {
        spdlog::warn("verifyGPGSignature: no signature file found for: {}", modulePath);
        return false;
    }

    // GAP-014: Replace popen()/shell-string with fork()+execvp() so that gpg
    // arguments are passed as individual array elements — the shell is never
    // invoked, eliminating command injection even if the paths contain special
    // characters such as spaces, quotes, or semicolons.
    //
    // Pipe layout: child writes gpg stderr+stdout → parent reads from pipe[0].
    int pipe_fds[2];
    if (::pipe(pipe_fds) != 0) {
        spdlog::error("verifyGPGSignature: pipe() failed ({}): {}", errno, std::strerror(errno));
        return false;
    }

    const pid_t child = ::fork();
    if (child < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        spdlog::error("verifyGPGSignature: fork() failed ({}): {}", errno, std::strerror(errno));
        return false;
    }

    if (child == 0) {
        // ── child ──
        // Redirect stdout + stderr into the write-end of the pipe.
        ::close(pipe_fds[0]);
        ::dup2(pipe_fds[1], STDOUT_FILENO);
        ::dup2(pipe_fds[1], STDERR_FILENO);
        ::close(pipe_fds[1]);

        // execvp takes a non-const argv — cast is safe because execvp does not
        // modify the strings (POSIX guarantees this).
        const char* argv[] = {
            "gpg",
            "--verify",
            sigFile.c_str(),
            modulePath.c_str(),
            nullptr
        };
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        ::execvp("gpg", const_cast<char* const*>(argv));
        // execvp only returns on error.
        ::_exit(127);
    }

    // ── parent ──
    ::close(pipe_fds[1]);

    char buf[kGpgOutputBufferSize];
    std::string output;
    ssize_t n;
    while ((n = ::read(pipe_fds[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        output += buf;
    }
    ::close(pipe_fds[0]);

    int status = 0;
    ::waitpid(child, &status, 0);
    const int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if (exitCode == 0 && output.find("Good signature") != std::string::npos) {
        spdlog::info("GPG signature verification PASSED for: {}", modulePath);
        return true;
    }

    spdlog::warn("GPG signature verification FAILED for: {} (exit={}) - {}",
                 modulePath, exitCode, output);
    return false;
}

std::map<std::string, std::string>
ModuleLoader::getExtendedAttributes(const std::string& modulePath) const {
    std::map<std::string, std::string> result;

    // First, list all attribute names
    ssize_t listSize = listxattr(modulePath.c_str(), nullptr, 0);
    if (listSize <= 0) {
        return result;
    }

    std::string namesBuf(static_cast<size_t>(listSize), '\0');
    listSize = listxattr(modulePath.c_str(), &namesBuf[0], static_cast<size_t>(listSize));
    if (listSize <= 0) {
        return result;
    }

    // Parse null-separated attribute names and read each value
    size_t pos = 0;
    while (pos < static_cast<size_t>(listSize)) {
        std::string name = &namesBuf[pos];
        pos += name.size() + 1;

        ssize_t valueSize = getxattr(modulePath.c_str(), name.c_str(), nullptr, 0);
        if (valueSize < 0) {
            continue;
        }
        std::string value(static_cast<size_t>(valueSize), '\0');
        if (getxattr(modulePath.c_str(), name.c_str(), &value[0],
                     static_cast<size_t>(valueSize)) >= 0) {
            result[name] = value;
        }
    }

    return result;
}

std::string ModuleLoader::readELFMetadata(const std::string& modulePath) const {
    std::ifstream file(modulePath, std::ios::binary);
    if (!file) {
        spdlog::warn("readELFMetadata: cannot open: {}", modulePath);
        return std::string{};
    }

    // Verify ELF magic number
    unsigned char magic[4];
    file.read(reinterpret_cast<char*>(magic), 4);
    if (file.gcount() < 4 ||
        magic[0] != 0x7f || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') {
        return std::string{};
    }

    file.seekg(0, std::ios::beg);

    // Read ELF class (32 or 64-bit)
    unsigned char elfClass;
    file.seekg(4);
    file.read(reinterpret_cast<char*>(&elfClass), 1);
    file.seekg(0, std::ios::beg);

    std::string metadata;

    auto processNoteSection = [&](uint64_t offset, uint64_t size) {
        file.seekg(static_cast<std::streamoff>(offset));
        uint64_t remaining = size;
        while (remaining >= sizeof(Elf64_Nhdr)) {
            Elf64_Nhdr nhdr = {};
            file.read(reinterpret_cast<char*>(&nhdr), sizeof(nhdr));
            if (file.gcount() < static_cast<std::streamsize>(sizeof(nhdr))) break;
            remaining -= sizeof(nhdr);

            uint64_t nameSize  = (nhdr.n_namesz + 3) & ~3u;
            uint64_t descSize  = (nhdr.n_descsz + 3) & ~3u;

            if (nameSize > remaining) break;
            // n_namesz includes the null terminator; exclude it for the string data
            uint32_t nameDataLen = nhdr.n_namesz > 0 ? nhdr.n_namesz - 1 : 0;
            std::string name(nameDataLen, '\0');
            file.read(&name[0], nameDataLen);
            // skip null terminator and alignment padding
            file.seekg(static_cast<std::streamoff>(
                file.tellg()) + static_cast<std::streamoff>(nameSize - nameDataLen));
            remaining -= nameSize;

            if (descSize > remaining) break;
            if (nhdr.n_type == NT_GNU_BUILD_ID && name == "GNU") {
                // Build ID: hex-encode raw bytes
                std::vector<unsigned char> buildId(nhdr.n_descsz);
                file.read(reinterpret_cast<char*>(buildId.data()), nhdr.n_descsz);
                // Skip padding to reach next note entry
                uint64_t padding = descSize - nhdr.n_descsz;
                if (padding > 0) {
                    file.seekg(static_cast<std::streamoff>(file.tellg()) +
                               static_cast<std::streamoff>(padding));
                }
                std::string buildIdHex;
                static const char hex[] = "0123456789abcdef";
                for (unsigned char b : buildId) {
                    buildIdHex += hex[b >> 4];
                    buildIdHex += hex[b & 0xf];
                }
                if (!metadata.empty()) metadata += "; ";
                metadata += "BuildID=" + buildIdHex;
            } else {
                file.seekg(static_cast<std::streamoff>(file.tellg()) +
                           static_cast<std::streamoff>(descSize));
            }
            remaining -= descSize;
        }
    };

    if (elfClass == ELFCLASS64) {
        Elf64_Ehdr ehdr = {};
        file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

        uint64_t shOffset = ehdr.e_shoff;
        uint16_t shEntSize = ehdr.e_shentsize;
        uint16_t shNum = ehdr.e_shnum;
        uint16_t shStrIdx = ehdr.e_shstrndx;

        if (shOffset == 0 || shNum == 0) return metadata;

        // Load section name string table
        file.seekg(static_cast<std::streamoff>(shOffset +
                   static_cast<uint64_t>(shStrIdx) * shEntSize));
        Elf64_Shdr strShdr = {};
        file.read(reinterpret_cast<char*>(&strShdr), sizeof(strShdr));
        std::string strtab(strShdr.sh_size, '\0');
        file.seekg(static_cast<std::streamoff>(strShdr.sh_offset));
        file.read(&strtab[0], static_cast<std::streamsize>(strShdr.sh_size));

        // Iterate sections to find NOTE and .comment
        for (uint16_t i = 0; i < shNum; ++i) {
            file.seekg(static_cast<std::streamoff>(shOffset +
                       static_cast<uint64_t>(i) * shEntSize));
            Elf64_Shdr shdr = {};
            file.read(reinterpret_cast<char*>(&shdr), sizeof(shdr));

            std::string secName;
            if (shdr.sh_name < strtab.size()) {
                secName = &strtab[shdr.sh_name];
            }

            if (shdr.sh_type == SHT_NOTE) {
                processNoteSection(shdr.sh_offset, shdr.sh_size);
            } else if (secName == ".comment" && shdr.sh_size > 0 && shdr.sh_size < kMaxCommentSectionSize) {
                std::string comment(shdr.sh_size, '\0');
                file.seekg(static_cast<std::streamoff>(shdr.sh_offset));
                file.read(&comment[0], static_cast<std::streamsize>(shdr.sh_size));
                // Null bytes serve as separators; replace with spaces
                for (char& c : comment) {
                    if (c == '\0') c = ' ';
                }
                // Trim trailing spaces
                while (!comment.empty() && comment.back() == ' ') comment.pop_back();
                if (!comment.empty()) {
                    if (!metadata.empty()) metadata += "; ";
                    metadata += "Comment=" + comment;
                }
            }
        }
    } else if (elfClass == ELFCLASS32) {
        Elf32_Ehdr ehdr = {};
        file.seekg(0);
        file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

        uint32_t shOffset = ehdr.e_shoff;
        uint16_t shEntSize = ehdr.e_shentsize;
        uint16_t shNum = ehdr.e_shnum;
        uint16_t shStrIdx = ehdr.e_shstrndx;

        if (shOffset == 0 || shNum == 0) return metadata;

        file.seekg(static_cast<std::streamoff>(shOffset +
                   static_cast<uint32_t>(shStrIdx) * shEntSize));
        Elf32_Shdr strShdr = {};
        file.read(reinterpret_cast<char*>(&strShdr), sizeof(strShdr));
        std::string strtab(strShdr.sh_size, '\0');
        file.seekg(static_cast<std::streamoff>(strShdr.sh_offset));
        file.read(&strtab[0], static_cast<std::streamsize>(strShdr.sh_size));

        for (uint16_t i = 0; i < shNum; ++i) {
            file.seekg(static_cast<std::streamoff>(shOffset +
                       static_cast<uint32_t>(i) * shEntSize));
            Elf32_Shdr shdr = {};
            file.read(reinterpret_cast<char*>(&shdr), sizeof(shdr));

            std::string secName;
            if (shdr.sh_name < strtab.size()) {
                secName = &strtab[shdr.sh_name];
            }

            if (shdr.sh_type == SHT_NOTE) {
                processNoteSection(shdr.sh_offset, shdr.sh_size);
            } else if (secName == ".comment" && shdr.sh_size > 0 && shdr.sh_size < kMaxCommentSectionSize) {
                std::string comment(shdr.sh_size, '\0');
                file.seekg(static_cast<std::streamoff>(shdr.sh_offset));
                file.read(&comment[0], static_cast<std::streamsize>(shdr.sh_size));
                for (char& c : comment) {
                    if (c == '\0') c = ' ';
                }
                while (!comment.empty() && comment.back() == ' ') comment.pop_back();
                if (!comment.empty()) {
                    if (!metadata.empty()) metadata += "; ";
                    metadata += "Comment=" + comment;
                }
            }
        }
    }

    return metadata;
}
#endif // __linux__

// ============================================================================
// Plugin Watchdog Implementation (Issue #2373)
// ============================================================================

uint64_t ModuleLoader::nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

void ModuleLoader::configureWatchdog(const WatchdogConfig& config) {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    watchdogConfig_ = config;
    spdlog::info("Watchdog configured: interval={}ms, max_restarts={}, initial_backoff={}ms",
                 config.check_interval_ms, config.max_restart_attempts, config.initial_backoff_ms);
}

void ModuleLoader::startWatchdog() {
    if (watchdogRunning_.exchange(true)) {
        return;  // Already running
    }
    watchdogThread_ = std::thread([this]() { watchdogLoop(); });
    spdlog::info("Plugin watchdog started");
}

void ModuleLoader::stopWatchdog() {
    if (!watchdogRunning_.exchange(false)) {
        return;  // Not running
    }
    watchdogCv_.notify_all();
    if (watchdogThread_.joinable()) {
        watchdogThread_.join();
    }
    spdlog::info("Plugin watchdog stopped");
}

bool ModuleLoader::isWatchdogRunning() const {
    return watchdogRunning_.load();
}

std::optional<WatchdogModuleStats> ModuleLoader::getWatchdogStats(const std::string& moduleName) const {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    auto it = watchdogStats_.find(moduleName);
    if (it == watchdogStats_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::map<std::string, WatchdogModuleStats> ModuleLoader::getAllWatchdogStats() const {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    return watchdogStats_;
}

void ModuleLoader::resetWatchdogStats() {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    watchdogStats_.clear();
    spdlog::info("Watchdog stats reset");
}

// ---- private helpers -------------------------------------------------------

void ModuleLoader::watchdogLoop() {
    spdlog::debug("Watchdog loop started");

    while (watchdogRunning_.load()) {
        {
            std::unique_lock<std::mutex> lk(watchdogMutex_);
            uint64_t interval_ms = watchdogConfig_.check_interval_ms;
            watchdogCv_.wait_for(lk,
                std::chrono::milliseconds(interval_ms),
                [this]() { return !watchdogRunning_.load(); });
        }

        if (!watchdogRunning_.load()) {
            break;
        }

        {
            std::lock_guard<std::mutex> lk(watchdogMutex_);
            if (!watchdogConfig_.enabled) {
                continue;
            }
        }

        watchdogCheckAllModules();
    }

    spdlog::debug("Watchdog loop exited");
}

void ModuleLoader::watchdogCheckAllModules() {
    // Take a snapshot of currently loaded, fully-activated modules under a
    // shared_lock so that concurrent load/unload operations do not race with
    // this iteration.
    std::vector<std::pair<std::string, std::string>> snapshot;  // name → path
    {
        std::shared_lock<std::shared_mutex> lk(modulesMutex_);
        for (const auto& [name, mod] : loadedModules_) {
            if (mod.fullyActivated) {
                snapshot.emplace_back(mod.name, mod.path);
            }
        }
    }

    for (auto& [name, path] : snapshot) {
        if (!watchdogRunning_.load()) {
            break;
        }

        // Verify module is still loaded (may have been unloaded between snapshot and now)
        LoadedModule modCopy;
        {
            std::shared_lock<std::shared_mutex> lk(modulesMutex_);
            auto it = loadedModules_.find(name);
            if (it == loadedModules_.end()) {
                continue;  // Module was unloaded since snapshot
            }
            modCopy = it->second;
        }

        if (healthChecks_.empty()) {
            continue;  // Nothing to check
        }

        std::string errorMsg;
        bool healthy = watchdogRunHealthChecks(modCopy, errorMsg);

        uint64_t now = nowMs();

        // Determine whether a restart is needed, updating watchdog stats under
        // the watchdogMutex_ lock.  The actual unload/reload is deferred to
        // after the lock is released to avoid a potential deadlock with
        // loadModule()/unloadModule() (which may acquire their own resources).
        bool shouldRestart = false;
        WatchdogModuleStats statsCopy;
        {
            std::lock_guard<std::mutex> lk(watchdogMutex_);
            auto& stats = watchdogStats_[name];
            stats.moduleName = name;
            stats.modulePath = path;
            stats.last_health_check_ms = now;

            if (healthy) {
                if (stats.consecutive_failures > 0) {
                    spdlog::info("Watchdog: module '{}' recovered (was {} consecutive failures)",
                                 name, stats.consecutive_failures);
                }
                stats.consecutive_failures = 0;
                stats.last_error.clear();
                // healthy — no restart needed; move to next module
            } else {
                // Health check failed
                stats.consecutive_failures++;
                stats.last_failure_ms = now;
                stats.last_error = errorMsg;

                spdlog::warn("Watchdog: health check FAILED for '{}' (consecutive: {}): {}",
                             name, stats.consecutive_failures, errorMsg);

                if (stats.permanently_failed) {
                    spdlog::warn("Watchdog: module '{}' is permanently failed; skipping restart",
                                 name);
                } else {
                    uint32_t maxAttempts = watchdogConfig_.max_restart_attempts;
                    if (maxAttempts > 0 && stats.restart_count >= maxAttempts) {
                        stats.permanently_failed = true;
                        spdlog::error("Watchdog: module '{}' exceeded max_restart_attempts ({}); "
                                      "marking as permanently failed",
                                      name, maxAttempts);
                    } else if (now < stats.next_retry_ms) {
                        uint64_t wait = stats.next_retry_ms - now;
                        spdlog::debug("Watchdog: module '{}' in backoff; {}ms remaining",
                                      name, wait);
                    } else {
                        shouldRestart = true;
                        statsCopy = stats;  // take a safe copy while holding the lock
                    }
                }
            }
        } // lock released here

        if (shouldRestart) {
            // Perform unload/reload without holding watchdogMutex_ to avoid
            // locking ordering issues with the auditor and OS loader.
            // statsCopy is a local snapshot; the map entry is not touched until
            // we write the results back under a fresh lock below.
            watchdogRestartModule(statsCopy, path);

            // Write the updated stats back to the map (if it still exists — it
            // may have been removed by a concurrent resetWatchdogStats() call).
            std::lock_guard<std::mutex> lk(watchdogMutex_);
            auto it = watchdogStats_.find(name);
            if (it != watchdogStats_.end()) {
                it->second = statsCopy;
            }
        }
    }
}

bool ModuleLoader::watchdogRunHealthChecks(LoadedModule& module, std::string& errorMessage) {
    if (healthChecks_.empty()) {
        return true;
    }

    for (const auto& [checkName, checkFunc] : healthChecks_) {
        try {
            auto result = checkFunc(module.handle, module.name);
            if (!result.passed) {
                errorMessage = checkName + ": " + result.message;
                return false;
            }
        } catch (const std::exception& e) {
            errorMessage = checkName + " threw exception: " + e.what();
            return false;
        }
    }
    return true;
}

bool ModuleLoader::watchdogRestartModule(WatchdogModuleStats& stats,
                                         const std::string& modulePath) {
    // Called without watchdogMutex_ held so that unloadModule()/loadModule()
    // can proceed without a locking ordering issue.  Stats are updated after
    // the OS-level operations complete, which is safe because only the watchdog
    // thread modifies stats for a given module.
    const std::string name = stats.moduleName;  // copy name (stats ref may alias)
    spdlog::info("Watchdog: attempting restart of module '{}' (attempt #{}) from '{}'",
                 name, stats.restart_count + 1, modulePath);

    // Unload the failed module
    unloadModule(name);

    // Attempt to reload
    auto loadResult = loadModule(modulePath, name);

    uint64_t now = nowMs();
    if (loadResult.success) {
        stats.restart_count++;
        stats.consecutive_failures = 0;
        stats.last_restart_ms = now;
        stats.next_retry_ms = 0;
        spdlog::info("Watchdog: module '{}' restarted successfully (total restarts: {})",
                     name, stats.restart_count);
        return true;
    }

    // Reload failed — update backoff for next attempt
    stats.consecutive_failures++;
    uint64_t backoff = watchdogCalculateBackoff(stats.consecutive_failures);
    stats.next_retry_ms = now + backoff;
    spdlog::error("Watchdog: restart of '{}' failed ({}); next retry in {}ms",
                  name, loadResult.errorMessage, backoff);
    return false;
}

uint64_t ModuleLoader::watchdogCalculateBackoff(uint32_t consecutiveFailures) const {
    if (consecutiveFailures == 0) {
        return 0;
    }
    // Cap at 60 to prevent floating-point overflow in std::pow():
    // 2^60 ≈ 1.15e18 ms, far beyond any realistic max_backoff_ms.
    if (consecutiveFailures > 60) {
        return watchdogConfig_.max_backoff_ms;
    }
    double backoff = watchdogConfig_.initial_backoff_ms *
                     std::pow(watchdogConfig_.backoff_multiplier,
                              static_cast<double>(consecutiveFailures - 1));
    return static_cast<uint64_t>(
        std::min(backoff, static_cast<double>(watchdogConfig_.max_backoff_ms)));
}

} // namespace modules
} // namespace themis
