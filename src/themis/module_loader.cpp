/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_loader.cpp                                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-12                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1425                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Platform-independent ModuleLoader implementation for ThemisDB.
//
// Platform-specific methods live alongside this file:
//   module_loader_win32.cpp  – Windows: Zone.Identifier, Authenticode
//   module_loader_linux.cpp  – Linux: GPG, xattr, ELF metadata
//
// Security verifier implementation is in:
//   module_security.cpp      – ModuleSecurityVerifier / Impl
//
// Migrated from src/base/module_loader.cpp to src/themis/ as part of the
// v1.7.0 modular build architecture.

#include "themis/base/module_loader.h"
#include "acceleration/plugin_security.h"
#include <filesystem>
#include <fstream>
#include <chrono>
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
#else
    #include <dlfcn.h>
#endif

namespace themis {
namespace modules {

using namespace themis::acceleration;

// ============================================================================
// ModuleLoader – constructor / destructor
// ============================================================================

ModuleLoader::ModuleLoader()
    : verifier_(std::make_unique<ModuleSecurityVerifier>()) {
    spdlog::info("ModuleLoader initialized");
}

ModuleLoader::~ModuleLoader() {
    stopWatchdog();
    unloadAllModules();
}

// ============================================================================
// Platform-independent OS loader primitives
// ============================================================================

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
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str()));
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
    return (filename.rfind("themis_", 0) == 0 ||
            filename.rfind("libthemis_", 0) == 0);
}

// ============================================================================
// Core module loading
// ============================================================================

ModuleVerificationResult ModuleLoader::loadModule(const std::string& modulePath,
                                                  const std::string& moduleName) {
    auto startTime = std::chrono::steady_clock::now();

    ModuleVerificationResult result;
    result.modulePath            = modulePath;
    result.verificationTimestamp = static_cast<uint64_t>(std::time(nullptr));

    spdlog::info("Loading module: {} from {}", moduleName, modulePath);

    // Step 1: Check quarantine and backoff
    if (checkQuarantine(modulePath, result)) {
        updateMetrics(false, 0, result.errorCode);
        return result;
    }

    // Step 2: Check if already loaded
    if (isModuleLoaded(moduleName)) {
        result.errorCode     = ModuleErrorCode::MODULE_ALREADY_LOADED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage  = getErrorMessage(result.errorCode) + ": " + moduleName;
        spdlog::warn("{}", result.errorMessage);
        result.success = false;
        updateMetrics(false, 0, result.errorCode);
        return result;
    }

    // Step 3: Verify module exists
    if (!std::filesystem::exists(modulePath)) {
        result.errorCode     = ModuleErrorCode::MODULE_NOT_FOUND;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage  = getErrorMessage(result.errorCode) + ": " + modulePath;
        spdlog::error("{}", result.errorMessage);
        result.success = false;
        recordFailure(modulePath, result.errorCode, result.errorMessage);
        updateMetrics(false, 0, result.errorCode);
        return result;
    }

    // Step 3b: Zone.Identifier quarantine check (Windows only)
    // Reject modules marked with Zone ID >= 3 (Internet zone or Restricted Sites)
#ifdef _WIN32
    {
        int zoneId       = getZoneIdentifier(modulePath);
        result.zoneId    = zoneId;
        if (zoneId >= 3) {
            const char* zoneName = (zoneId == 3) ? "Internet" : "Restricted Sites";
            result.errorCode     = ModuleErrorCode::ZONE_ID_BLOCKED;
            result.errorCategory = categorizeError(result.errorCode);
            result.errorMessage  = getErrorMessage(result.errorCode) +
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

    // Step 4: Use cached metadata if available
    result.metadata = getCachedMetadata(modulePath);
    if (!result.metadata.isValid()) {
        spdlog::warn("Module metadata invalid or missing for: {}", modulePath);
        result.metadata.version    = "unversioned";
        result.metadata.abiVersion = "unknown";
    } else {
        spdlog::info("Module metadata: version={}, abi={}, themis={}.{}.{}",
                     result.metadata.version, result.metadata.abiVersion,
                     result.metadata.themisMajor, result.metadata.themisMinor,
                     result.metadata.themisPatch);

        if (stagedLoadingEnabled_) {
            spdlog::debug("STAGE: VALIDATING - {}", moduleName);
        }

        if (!isABICompatible(result.metadata)) {
            result.errorCode     = ModuleErrorCode::ABI_INCOMPATIBLE;
            result.errorCategory = categorizeError(result.errorCode);
            result.errorMessage  = getErrorMessage(result.errorCode) +
                                  ": module " +
                                  std::to_string(result.metadata.themisMajor) + "." +
                                  std::to_string(result.metadata.themisMinor) +
                                  ", themis " +
                                  std::to_string(themisABIMajor_) + "." +
                                  std::to_string(themisABIMinor_);
            spdlog::error("{}", result.errorMessage);
            result.success = false;
            recordFailure(modulePath, result.errorCode, result.errorMessage);
            updateMetrics(false, 0, result.errorCode);
            return result;
        }
    }

    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: VERIFYING - {}", moduleName);
    }

    // Step 5: Verify module signature and integrity
    std::string errorMessage;
    if (!verifier_->verifyModule(modulePath, errorMessage)) {
        result.errorCode     = ModuleErrorCode::VERIFICATION_FAILED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage  = getErrorMessage(result.errorCode) + ": " + errorMessage;
        spdlog::critical("SECURITY: {}", result.errorMessage);
        result.success = false;

        auto& auditor = PluginSecurityAuditor::instance();
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
            modulePath,
            verifier_->calculateFileHash(modulePath),
            errorMessage,
            result.verificationTimestamp,
            "CRITICAL"
        });

        recordFailure(modulePath, result.errorCode, result.errorMessage);
        updateMetrics(false, 0, result.errorCode);
        return result;
    }

    metrics_.verificationSuccesses++;

    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: VERIFIED - {}", moduleName);
    }

    // Step 6: Calculate and store file hash
    result.moduleHash = verifier_->calculateFileHash(modulePath);

    // Step 6b: SHA-256 manifest check (Issue #2471)
    if (hashVerifier_.manifestSize() > 0) {
        const auto expected = hashVerifier_.getExpectedHash(moduleName);
        if (expected.has_value()) {
            if (result.moduleHash != *expected) {
                result.errorCode     = ModuleErrorCode::HASH_MISMATCH;
                result.errorCategory = categorizeError(result.errorCode);
                result.errorMessage  =
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

    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: STAGING - {}", moduleName);
    }

    // Step 7: Load the module library
    void* handle = loadLibrary(modulePath);
    if (!handle) {
        result.errorCode     = ModuleErrorCode::LOAD_LIBRARY_FAILED;
        result.errorCategory = categorizeError(result.errorCode);
        result.errorMessage  = getErrorMessage(result.errorCode) + ": " + modulePath;
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
    if (!result.metadata.isValid()) {
        result.metadata = extractMetadataFromHandle(handle);
        spdlog::info("Extracted metadata from handle: version={}",
                     result.metadata.version);

        if (result.metadata.isValid()) {
            metadataCache_[modulePath] = result.metadata;
        }
    }

    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: STAGED - {}", moduleName);
    }

    // Step 8: Calculate load duration
    auto endTime    = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          endTime - startTime).count();

    // Step 9: Create module info structure
    LoadedModule module;
    module.name            = moduleName;
    module.path            = modulePath;
    module.fileHash        = result.moduleHash;
    module.handle          = handle;
    module.verified        = true;
    module.loadTime        = result.verificationTimestamp;
    module.loadDurationMs  = static_cast<uint64_t>(durationMs);
    module.metadata        = result.metadata;
    module.version         = result.metadata.version;
    module.currentStage    = stagedLoadingEnabled_
                             ? LoadStage::STAGING : LoadStage::ACTIVE;

    // Staged activation: run health checks
    if (stagedLoadingEnabled_) {
        spdlog::debug("STAGE: ACTIVATING - {}", moduleName);

        if (!runHealthChecks(module, result)) {
            spdlog::error("Health checks failed for module: {}", moduleName);
            unloadLibrary(handle);

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
        loadedModules_[module.name] = module;
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

    clearFailureHistory(modulePath);
    updateMetrics(true, static_cast<uint64_t>(durationMs), ModuleErrorCode::SUCCESS);

    spdlog::info("Module loaded successfully: {} (version: {}, hash: {}, duration: {}ms)",
                 moduleName, module.version, result.moduleHash, durationMs);

    result.success       = true;
    result.errorCode     = ModuleErrorCode::SUCCESS;
    result.errorCategory = categorizeError(ModuleErrorCode::SUCCESS);
    return result;
}

size_t ModuleLoader::loadAllModules(const std::string& moduleDirectory) {
    spdlog::info("Loading all modules from: {}", moduleDirectory);

    if (!std::filesystem::exists(moduleDirectory)) {
        spdlog::error("Module directory not found: {}", moduleDirectory);
        return 0;
    }

    size_t loadedCount = 0;

    for (const auto& entry :
         std::filesystem::directory_iterator(moduleDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string filename = entry.path().filename().string();

#ifdef _WIN32
        if (filename.rfind("themis_", 0) != 0 ||
            entry.path().extension() != ".dll") {
            continue;
        }
#else
        if (filename.rfind("libthemis_", 0) != 0 ||
            entry.path().extension() != ".so") {
            continue;
        }
#endif

        std::string moduleName =
            getModuleNameFromPath(entry.path().string());
        auto result = loadModule(entry.path().string(), moduleName);

        if (result.success) {
            loadedCount++;
        }
    }

    spdlog::info(
        "Loaded {}/{} modules from {}",
        loadedCount,
        std::distance(
            std::filesystem::directory_iterator(moduleDirectory),
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

    auto&    auditor = PluginSecurityAuditor::instance();
    uint64_t now     = static_cast<uint64_t>(std::time(nullptr));

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

std::optional<LoadedModule>
ModuleLoader::getModuleInfo(const std::string& moduleName) const {
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
    return result;
}

// ============================================================================
// Security policy forwarding
// ============================================================================

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

// ============================================================================
// Audit log access
// ============================================================================

bool ModuleLoader::exportAuditLog(const std::string& outputPath) const {
    auto& auditor = PluginSecurityAuditor::instance();
    return auditor.exportEvents(outputPath);
}

std::vector<PluginSecurityEvent>
ModuleLoader::getPluginAuditTrail(const std::string& modulePath) const {
    auto& auditor = PluginSecurityAuditor::instance();
    return auditor.getEventsForPlugin(modulePath);
}

// ============================================================================
// ModuleRegistry
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
    auto it = std::find_if(
        modules_.begin(), modules_.end(),
        [&moduleName](const LoadedModule& m) { return m.name == moduleName; });

    if (it != modules_.end()) {
        modules_.erase(it);
        spdlog::debug("Module unregistered: {}", moduleName);
    }
}

bool ModuleRegistry::isRegistered(const std::string& moduleName) const {
    return std::find_if(
               modules_.begin(), modules_.end(),
               [&moduleName](const LoadedModule& m) {
                   return m.name == moduleName;
               }) != modules_.end();
}

std::vector<LoadedModule> ModuleRegistry::getAllModules() const {
    return modules_;
}

void ModuleRegistry::clear() {
    modules_.clear();
}

// ============================================================================
// Error handling helpers
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
            return "Module blocked: Windows Zone.Identifier marks it as "
                   "downloaded from Internet or Restricted Sites";
        case ModuleErrorCode::INTERNAL_ERROR:
            return "Internal module loader error";
        case ModuleErrorCode::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

ErrorCategory ModuleLoader::categorizeError(ModuleErrorCode code) const {
    switch (code) {
        case ModuleErrorCode::SUCCESS:
            return ErrorCategory::NONE;

        case ModuleErrorCode::MODULE_ACCESS_DENIED:
        case ModuleErrorCode::LOAD_LIBRARY_FAILED:
            return ErrorCategory::TRANSIENT;

        case ModuleErrorCode::MODULE_NOT_FOUND:
        case ModuleErrorCode::MODULE_DIRECTORY_NOT_FOUND:
        case ModuleErrorCode::VERSION_INCOMPATIBLE:
        case ModuleErrorCode::ABI_INCOMPATIBLE:
        case ModuleErrorCode::METADATA_MISSING:
            return ErrorCategory::RECOVERABLE;

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

        case ModuleErrorCode::MODULE_ALREADY_LOADED:
        case ModuleErrorCode::SYMBOL_NOT_FOUND:
        case ModuleErrorCode::METADATA_CORRUPTED:
        case ModuleErrorCode::INITIALIZATION_FAILED:
        default:
            return ErrorCategory::PERMANENT;
    }
}

// ============================================================================
// Metadata extraction
// ============================================================================

ModuleMetadata
ModuleLoader::extractModuleMetadata(const std::string& modulePath) {
    ModuleMetadata metadata;

    void* tempHandle = loadLibrary(modulePath);
    if (!tempHandle) {
        spdlog::warn("Could not load module temporarily for metadata "
                     "extraction: {}",
                     modulePath);
        return metadata;
    }

    typedef const char* (*GetVersionFunc)();
    typedef uint32_t    (*GetVersionIntFunc)();

    auto getVersionStr =
        reinterpret_cast<GetVersionFunc>(getSymbol(tempHandle, "themis_module_version"));
    auto getAbiVersion =
        reinterpret_cast<GetVersionFunc>(getSymbol(tempHandle, "themis_module_abi_version"));
    auto getBuildId =
        reinterpret_cast<GetVersionFunc>(getSymbol(tempHandle, "themis_module_build_id"));
    auto getMajor =
        reinterpret_cast<GetVersionIntFunc>(getSymbol(tempHandle, "themis_api_version_major"));
    auto getMinor =
        reinterpret_cast<GetVersionIntFunc>(getSymbol(tempHandle, "themis_api_version_minor"));
    auto getPatch =
        reinterpret_cast<GetVersionIntFunc>(getSymbol(tempHandle, "themis_api_version_patch"));

    if (getVersionStr) metadata.version    = getVersionStr();
    if (getAbiVersion) metadata.abiVersion = getAbiVersion();
    if (getBuildId)    metadata.buildId    = getBuildId();
    if (getMajor)      metadata.themisMajor = getMajor();
    if (getMinor)      metadata.themisMinor = getMinor();
    if (getPatch)      metadata.themisPatch = getPatch();

    unloadLibrary(tempHandle);

    if (metadata.version.empty()) {
        spdlog::debug("No version symbol found in module: {}", modulePath);
    } else {
        spdlog::debug("Extracted metadata from {}: version={}, abi={}, buildId={}",
                      modulePath, metadata.version,
                      metadata.abiVersion, metadata.buildId);
    }

    return metadata;
}

ModuleMetadata ModuleLoader::extractMetadataFromHandle(void* handle) {
    ModuleMetadata metadata;

    if (!handle) {
        return metadata;
    }

    typedef const char* (*GetVersionFunc)();
    typedef uint32_t    (*GetVersionIntFunc)();

    auto getVersionStr =
        reinterpret_cast<GetVersionFunc>(getSymbol(handle, "themis_module_version"));
    auto getAbiVersion =
        reinterpret_cast<GetVersionFunc>(getSymbol(handle, "themis_module_abi_version"));
    auto getBuildId =
        reinterpret_cast<GetVersionFunc>(getSymbol(handle, "themis_module_build_id"));
    auto getMajor =
        reinterpret_cast<GetVersionIntFunc>(getSymbol(handle, "themis_api_version_major"));
    auto getMinor =
        reinterpret_cast<GetVersionIntFunc>(getSymbol(handle, "themis_api_version_minor"));
    auto getPatch =
        reinterpret_cast<GetVersionIntFunc>(getSymbol(handle, "themis_api_version_patch"));

    if (getVersionStr) metadata.version     = getVersionStr();
    if (getAbiVersion) metadata.abiVersion  = getAbiVersion();
    if (getBuildId)    metadata.buildId     = getBuildId();
    if (getMajor)      metadata.themisMajor = getMajor();
    if (getMinor)      metadata.themisMinor = getMinor();
    if (getPatch)      metadata.themisPatch = getPatch();

    return metadata;
}

ModuleMetadata ModuleLoader::getCachedMetadata(const std::string& modulePath) {
    auto it = metadataCache_.find(modulePath);
    if (it != metadataCache_.end()) {
        spdlog::debug("Using cached metadata for: {}", modulePath);
        return it->second;
    }

    auto metadata = extractModuleMetadata(modulePath);
    if (metadata.isValid()) {
        metadataCache_[modulePath] = metadata;
        spdlog::debug("Cached metadata for: {}", modulePath);
    }

    return metadata;
}

// ============================================================================
// Quarantine and backoff
// ============================================================================

void ModuleLoader::recordFailure(const std::string& modulePath,
                                 ModuleErrorCode    errorCode,
                                 const std::string& errorMessage) {
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));

    auto& history               = failureHistory_[modulePath];
    history.modulePath          = modulePath;
    history.failureTimestamps.push_back(currentTime);
    history.consecutiveFailures++;
    history.lastFailureTime     = currentTime;
    history.lastErrorCode       = errorCode;
    history.lastErrorMessage    = errorMessage;
    history.nextRetryTime =
        currentTime + calculateBackoffTime(history.consecutiveFailures);

    spdlog::warn(
        "Module failure recorded: {} (failures: {}, next retry: {}s)",
        modulePath, history.consecutiveFailures,
        calculateBackoffTime(history.consecutiveFailures));

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

    uint64_t currentTime        = static_cast<uint64_t>(std::time(nullptr));
    it->second.quarantineTime   = currentTime;

    metrics_.quarantineEvents++;
    metrics_.currentlyQuarantined++;

    spdlog::critical(
        "QUARANTINE: Module quarantined after {} consecutive failures: {} "
        "(last error: {})",
        it->second.consecutiveFailures, modulePath,
        it->second.lastErrorMessage);
}

uint64_t ModuleLoader::calculateBackoffTime(uint32_t consecutiveFailures) const {
    if (consecutiveFailures == 0) {
        return 0;
    }
    if (consecutiveFailures > 32) {
        return maxBackoffSeconds_;
    }
    uint64_t backoff = 1ULL << (consecutiveFailures - 1);
    return std::min(backoff, static_cast<uint64_t>(maxBackoffSeconds_));
}

bool ModuleLoader::checkQuarantine(const std::string& modulePath,
                                   ModuleVerificationResult& result) {
    auto it = failureHistory_.find(modulePath);
    if (it == failureHistory_.end()) {
        return false;
    }

    auto& history = it->second;

    if (history.isQuarantined()) {
        result.success       = false;
        result.errorCode     = ModuleErrorCode::QUARANTINED;
        result.errorCategory = ErrorCategory::FATAL;
        result.errorMessage  = getErrorMessage(ModuleErrorCode::QUARANTINED) +
                              ": " + modulePath +
                              " (failures: " +
                              std::to_string(history.consecutiveFailures) + ")";
        spdlog::error("{}", result.errorMessage);
        return true;
    }

    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    if (!history.canRetry(currentTime)) {
        result.success       = false;
        result.errorCode     = ModuleErrorCode::POLICY_VIOLATION;
        result.errorCategory = ErrorCategory::TRANSIENT;
        uint64_t waitTime    = history.nextRetryTime - currentTime;
        result.errorMessage  = "Module in backoff period, retry in " +
                              std::to_string(waitTime) + " seconds: " +
                              modulePath;
        spdlog::warn("{}", result.errorMessage);
        return true;
    }

    return false;
}

std::optional<ModuleFailureHistory>
ModuleLoader::getFailureHistory(const std::string& modulePath) const {
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

    it->second.quarantineTime       = 0;
    it->second.consecutiveFailures  = 0;
    it->second.nextRetryTime        = 0;

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
// ABI compatibility
// ============================================================================

bool ModuleLoader::isABICompatible(const ModuleMetadata& metadata) const {
    if (!metadata.isValid()) {
        spdlog::warn("Cannot check ABI compatibility: metadata invalid");
        return false;
    }

    if (metadata.themisMajor != themisABIMajor_) {
        spdlog::error(
            "ABI incompatible: major version mismatch (module: {}, themis: {})",
            metadata.themisMajor, themisABIMajor_);
        return false;
    }

    if (metadata.themisMinor > themisABIMinor_) {
        spdlog::error(
            "ABI incompatible: module minor version too new (module: {}, "
            "themis: {})",
            metadata.themisMinor, themisABIMinor_);
        return false;
    }

    spdlog::debug("ABI compatible: module {}.{}.{} with themis {}.{}",
                  metadata.themisMajor, metadata.themisMinor,
                  metadata.themisPatch,
                  themisABIMajor_, themisABIMinor_);

    return true;
}

// ============================================================================
// Metrics
// ============================================================================

void ModuleLoader::updateMetrics(bool success, uint64_t durationMs,
                                 ModuleErrorCode errorCode) {
    metrics_.totalLoadAttempts++;

    if (success) {
        metrics_.successfulLoads++;
        metrics_.totalLoadDurationMs += durationMs;
        metrics_.minLoadDurationMs =
            std::min(metrics_.minLoadDurationMs, durationMs);
        metrics_.maxLoadDurationMs =
            std::max(metrics_.maxLoadDurationMs, durationMs);
    } else {
        metrics_.failedLoads++;
        metrics_.errorCounts[errorCode]++;

        if (errorCode == ModuleErrorCode::VERIFICATION_FAILED ||
            errorCode == ModuleErrorCode::SIGNATURE_INVALID   ||
            errorCode == ModuleErrorCode::HASH_MISMATCH       ||
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
// Staged loading
// ============================================================================

void ModuleLoader::registerHealthCheck(const std::string& checkName,
                                       HealthCheckFunction checkFunc) {
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

std::optional<LoadStage>
ModuleLoader::queryModuleStage(const std::string& moduleName) const {
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);

    if (it == loadedModules_.end()) {
        return std::nullopt;
    }

    return it->second.currentStage;
}

std::vector<HealthCheckResult>
ModuleLoader::getHealthCheckResults(const std::string& moduleName) const {
    std::shared_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);

    if (it == loadedModules_.end()) {
        return {};
    }

    return it->second.healthChecks;
}

bool ModuleLoader::updateModuleStage(const std::string& moduleName,
                                     LoadStage newStage) {
    std::unique_lock<std::shared_mutex> lk(modulesMutex_);
    auto it = loadedModules_.find(moduleName);

    if (it == loadedModules_.end()) {
        return false;
    }

    it->second.currentStage = newStage;
    spdlog::debug("Module {} stage updated to {}",
                  moduleName, static_cast<int>(newStage));
    return true;
}

bool ModuleLoader::runHealthChecks(LoadedModule& module,
                                   ModuleVerificationResult& result) {
    if (healthChecks_.empty()) {
        spdlog::debug("No health checks registered for module: {}",
                      module.name);
        return true;
    }

    spdlog::info("Running {} health checks for module: {}",
                 healthChecks_.size(), module.name);

    for (const auto& [checkName, checkFunc] : healthChecks_) {
        auto startTime = std::chrono::steady_clock::now();

        try {
            auto healthResult = checkFunc(module.handle, module.name);

            auto endTime = std::chrono::steady_clock::now();
            healthResult.checkDurationMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    endTime - startTime).count();

            module.healthChecks.push_back(healthResult);

            if (!healthResult.passed) {
                result.errorCode     = ModuleErrorCode::HEALTH_CHECK_FAILED;
                result.errorCategory = ErrorCategory::RECOVERABLE;
                result.errorMessage  = "Health check failed: " + checkName +
                                      " - " + healthResult.message;
                spdlog::error("{}", result.errorMessage);
                return false;
            }

            spdlog::info("Health check passed: {} ({}ms)",
                         checkName, healthResult.checkDurationMs);
        } catch (const std::exception& e) {
            auto healthResult = HealthCheckResult::failure(
                checkName,
                "Exception during health check: " + std::string(e.what()));
            module.healthChecks.push_back(healthResult);

            result.errorCode     = ModuleErrorCode::HEALTH_CHECK_FAILED;
            result.errorCategory = ErrorCategory::FATAL;
            result.errorMessage  = "Health check exception: " + checkName +
                                  " - " + std::string(e.what());
            spdlog::critical("{}", result.errorMessage);
            return false;
        }
    }

    spdlog::info("All health checks passed for module: {}", module.name);
    return true;
}

// ============================================================================
// Plugin Watchdog (Issue #2373)
// ============================================================================

uint64_t ModuleLoader::nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

void ModuleLoader::configureWatchdog(const WatchdogConfig& config) {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    watchdogConfig_ = config;
    spdlog::info(
        "Watchdog configured: interval={}ms, max_restarts={}, "
        "initial_backoff={}ms",
        config.check_interval_ms, config.max_restart_attempts,
        config.initial_backoff_ms);
}

void ModuleLoader::startWatchdog() {
    if (watchdogRunning_.exchange(true)) {
        return;
    }
    watchdogThread_ = std::thread([this]() { watchdogLoop(); });
    spdlog::info("Plugin watchdog started");
}

void ModuleLoader::stopWatchdog() {
    if (!watchdogRunning_.exchange(false)) {
        return;
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

std::optional<WatchdogModuleStats>
ModuleLoader::getWatchdogStats(const std::string& moduleName) const {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    auto it = watchdogStats_.find(moduleName);
    if (it == watchdogStats_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::map<std::string, WatchdogModuleStats>
ModuleLoader::getAllWatchdogStats() const {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    return watchdogStats_;
}

void ModuleLoader::resetWatchdogStats() {
    std::lock_guard<std::mutex> lk(watchdogMutex_);
    watchdogStats_.clear();
    spdlog::info("Watchdog stats reset");
}

void ModuleLoader::watchdogLoop() {
    spdlog::debug("Watchdog loop started");

    while (watchdogRunning_.load()) {
        {
            std::unique_lock<std::mutex> lk(watchdogMutex_);
            uint64_t interval_ms = watchdogConfig_.check_interval_ms;
            watchdogCv_.wait_for(
                lk,
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
    std::vector<std::pair<std::string, std::string>> snapshot;
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
                continue;
            }
            modCopy = it->second;
        }

        if (healthChecks_.empty()) {
            continue;
        }

        std::string errorMsg;
        bool        healthy = watchdogRunHealthChecks(modCopy, errorMsg);

        uint64_t now = nowMs();

        bool               shouldRestart = false;
        WatchdogModuleStats statsCopy;
        {
            std::lock_guard<std::mutex> lk(watchdogMutex_);
            auto& stats            = watchdogStats_[name];
            stats.moduleName       = name;
            stats.modulePath       = path;
            stats.last_health_check_ms = now;

            if (healthy) {
                if (stats.consecutive_failures > 0) {
                    spdlog::info(
                        "Watchdog: module '{}' recovered (was {} consecutive "
                        "failures)",
                        name, stats.consecutive_failures);
                }
                stats.consecutive_failures = 0;
                stats.last_error.clear();
            } else {
                stats.consecutive_failures++;
                stats.last_failure_ms = now;
                stats.last_error      = errorMsg;

                spdlog::warn(
                    "Watchdog: health check FAILED for '{}' "
                    "(consecutive: {}): {}",
                    name, stats.consecutive_failures, errorMsg);

                if (stats.permanently_failed) {
                    spdlog::warn(
                        "Watchdog: module '{}' is permanently failed; "
                        "skipping restart",
                        name);
                } else {
                    uint32_t maxAttempts = watchdogConfig_.max_restart_attempts;
                    if (maxAttempts > 0 &&
                        stats.restart_count >= maxAttempts) {
                        stats.permanently_failed = true;
                        spdlog::error(
                            "Watchdog: module '{}' exceeded "
                            "max_restart_attempts ({}); marking as "
                            "permanently failed",
                            name, maxAttempts);
                    } else if (now < stats.next_retry_ms) {
                        uint64_t wait = stats.next_retry_ms - now;
                        spdlog::debug(
                            "Watchdog: module '{}' in backoff; {}ms remaining",
                            name, wait);
                    } else {
                        shouldRestart = true;
                        statsCopy     = stats;
                    }
                }
            }
        }

        if (shouldRestart) {
            watchdogRestartModule(statsCopy, path);

            std::lock_guard<std::mutex> lk(watchdogMutex_);
            auto it2 = watchdogStats_.find(name);
            if (it2 != watchdogStats_.end()) {
                it2->second = statsCopy;
            }
        }
    }
}

bool ModuleLoader::watchdogRunHealthChecks(LoadedModule& module,
                                           std::string& errorMessage) {
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
            errorMessage =
                checkName + " threw exception: " + e.what();
            return false;
        }
    }
    return true;
}

bool ModuleLoader::watchdogRestartModule(WatchdogModuleStats& stats,
                                         const std::string&   modulePath) {
    const std::string name = stats.moduleName;
    spdlog::info(
        "Watchdog: attempting restart of module '{}' (attempt #{}) from '{}'",
        name, stats.restart_count + 1, modulePath);

    unloadModule(name);

    auto loadResult = loadModule(modulePath, name);

    uint64_t now = nowMs();
    if (loadResult.success) {
        stats.restart_count++;
        stats.consecutive_failures = 0;
        stats.last_restart_ms      = now;
        stats.next_retry_ms        = 0;
        spdlog::info(
            "Watchdog: module '{}' restarted successfully "
            "(total restarts: {})",
            name, stats.restart_count);
        return true;
    }

    stats.consecutive_failures++;
    uint64_t backoff      = watchdogCalculateBackoff(stats.consecutive_failures);
    stats.next_retry_ms   = now + backoff;
    spdlog::error(
        "Watchdog: restart of '{}' failed ({}); next retry in {}ms",
        name, loadResult.errorMessage, backoff);
    return false;
}

uint64_t ModuleLoader::watchdogCalculateBackoff(
    uint32_t consecutiveFailures) const {
    if (consecutiveFailures == 0) {
        return 0;
    }
    if (consecutiveFailures > 60) {
        return watchdogConfig_.max_backoff_ms;
    }
    double backoff =
        watchdogConfig_.initial_backoff_ms *
        std::pow(watchdogConfig_.backoff_multiplier,
                 static_cast<double>(consecutiveFailures - 1));
    return static_cast<uint64_t>(
        std::min(backoff,
                 static_cast<double>(watchdogConfig_.max_backoff_ms)));
}

} // namespace modules
} // namespace themis
