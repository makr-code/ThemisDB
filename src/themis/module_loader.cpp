/**
 * @file module_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <random>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#ifdef THEMIS_HAVE_LIBZIP
#  include <zip.h>
#endif
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>

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
    std::sort(result.begin(), result.end(),
              [](const LoadedModule& a, const LoadedModule& b) {
                  return a.name < b.name;
              });
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
        [[fallthrough]];\n        default:
            return "Unknown error";
    }
}

ErrorCategory ModuleLoader::categorizeError(ModuleErrorCode code) const {
    switch (code) {
        case ModuleErrorCode::SUCCESS:
            return ErrorCategory::NONE;

        case ModuleErrorCode::MODULE_ACCESS_DENIED:
        [[fallthrough]];\n        case ModuleErrorCode::LOAD_LIBRARY_FAILED:
            return ErrorCategory::TRANSIENT;

        case ModuleErrorCode::MODULE_NOT_FOUND:
        [[fallthrough]];\n        case ModuleErrorCode::MODULE_DIRECTORY_NOT_FOUND:
        [[fallthrough]];\n        case ModuleErrorCode::VERSION_INCOMPATIBLE:
        [[fallthrough]];\n        case ModuleErrorCode::ABI_INCOMPATIBLE:
        [[fallthrough]];\n        case ModuleErrorCode::METADATA_MISSING:
            return ErrorCategory::RECOVERABLE;

        case ModuleErrorCode::VERIFICATION_FAILED:
        [[fallthrough]];\n        case ModuleErrorCode::SIGNATURE_INVALID:
        [[fallthrough]];\n        case ModuleErrorCode::HASH_MISMATCH:
        [[fallthrough]];\n        case ModuleErrorCode::CERTIFICATE_REVOKED:
        [[fallthrough]];\n        case ModuleErrorCode::CERTIFICATE_EXPIRED:
        [[fallthrough]];\n        case ModuleErrorCode::UNTRUSTED_SIGNER:
        [[fallthrough]];\n        case ModuleErrorCode::BLACKLISTED:
        [[fallthrough]];\n        case ModuleErrorCode::QUARANTINED:
        [[fallthrough]];\n        case ModuleErrorCode::ZONE_ID_BLOCKED:
        [[fallthrough]];\n        case ModuleErrorCode::POLICY_VIOLATION:
            return ErrorCategory::FATAL;

        case ModuleErrorCode::MODULE_ALREADY_LOADED:
        [[fallthrough]];\n        case ModuleErrorCode::SYMBOL_NOT_FOUND:
        [[fallthrough]];\n        case ModuleErrorCode::METADATA_CORRUPTED:
        [[fallthrough]];\n        case ModuleErrorCode::INITIALIZATION_FAILED:
        [[fallthrough]];\n        default:
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

// ============================================================================
// PluginBundleLoader — cross-platform bundle loading (v1.4.0)
// ============================================================================

namespace themis {
namespace modules {

namespace {

/// Return the OpenSSL error string for the most recent error.
std::string opensslLastError() {
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    return buf;
}

/// RAII guard that removes a directory tree on scope exit unless disarmed.
struct TempDirGuard {
    explicit TempDirGuard(std::string path) : path_(std::move(path)) {}
    ~TempDirGuard() {
        if (!path_.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(path_, ec);
        }
    }
    /// Prevent cleanup (caller takes ownership of the directory).
    void disarm() { path_.clear(); }
    std::string path_;
};

#ifdef THEMIS_HAVE_LIBZIP

/// Generate a unique temporary directory path with a random hex suffix.
std::string makeTempDirPath() {
    namespace fs = std::filesystem;
    auto base = fs::temp_directory_path() / "themis_bundle";

    // Combine a monotonic timestamp with a random component to guarantee
    // uniqueness under concurrent calls.
    auto ns = std::chrono::steady_clock::now().time_since_epoch().count();
    std::random_device rd;
    std::uniform_int_distribution<uint32_t> dist;
    std::ostringstream oss;
    oss << std::hex << ns << "_" << dist(rd);
    return (base / oss.str()).string();
}

/// Reject ZipSlip: verify that resolvedPath is inside tempDir.
/// Returns true when safe; false when the path escapes the temp dir.
///
/// Precondition: resolvedPath was produced by lexically_normal() on
/// (tempDir / non_empty_name), so resolvedStr.size() > tempStr.size() is
/// guaranteed for valid in-directory entries, making the separator access safe.
bool isSafeEntryPath(const std::filesystem::path& tempDir,
                     const std::filesystem::path& resolvedPath) {
    auto tempStr     = tempDir.string();
    auto resolvedStr = resolvedPath.string();
    // Require the resolved path to be strictly longer (at least one component).
    if (resolvedStr.size() <= tempStr.size()) return false;
    // Require the temp dir to be a proper prefix followed by a separator.
    if (resolvedStr.substr(0, tempStr.size()) != tempStr) return false;
    // At this point resolvedStr.size() > tempStr.size() ensures safe access.
    char sep = resolvedStr[tempStr.size()];
    return sep == '/' || sep == '\\';
}

#endif // THEMIS_HAVE_LIBZIP

} // anonymous namespace

// ----------------------------------------------------------------------------
// PluginBundleLoader — construction / destruction
// ----------------------------------------------------------------------------

PluginBundleLoader::PluginBundleLoader() = default;
PluginBundleLoader::~PluginBundleLoader() = default;

// ----------------------------------------------------------------------------
// setPublicKey
// ----------------------------------------------------------------------------

void PluginBundleLoader::setPublicKey(const std::string& publicKeyPem) {
    publicKeyPem_ = publicKeyPem;
}

void PluginBundleLoader::setAllowUnsignedBundles(bool allow) {
    allowUnsignedBundles_ = allow;
}

// ----------------------------------------------------------------------------
// currentPlatform
// ----------------------------------------------------------------------------

std::string PluginBundleLoader::currentPlatform() {
#if defined(_WIN32) || defined(_WIN64)
    const std::string os = "windows";
#elif defined(__APPLE__)
    const std::string os = "macos";
#elif defined(__linux__)
    const std::string os = "linux";
#else
    const std::string os = "unknown";
#endif

#if defined(__x86_64__) || defined(_M_X64)
    const std::string arch = "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    const std::string arch = "arm64";
#elif defined(__arm__) || defined(_M_ARM)
    const std::string arch = "arm";
#elif defined(__i386__) || defined(_M_IX86)
    const std::string arch = "x86";
#else
    const std::string arch = "unknown";
#endif

    return os + "-" + arch;
}

// ----------------------------------------------------------------------------
// parseManifest
// ----------------------------------------------------------------------------

bool PluginBundleLoader::parseManifest(const std::string& jsonText,
                                       PluginBundleManifest& manifest,
                                       std::string& error) {
    try {
        auto j = nlohmann::json::parse(jsonText);

        // Mandatory fields
        if (!j.contains("name") || !j["name"].is_string()) {
            error = "manifest.json missing required string field 'name'";
            return false;
        }
        if (!j.contains("version") || !j["version"].is_string()) {
            error = "manifest.json missing required string field 'version'";
            return false;
        }

        manifest.name    = j["name"].get<std::string>();
        manifest.version = j["version"].get<std::string>();

        if (manifest.name.empty()) {
            error = "manifest.json 'name' must not be empty";
            return false;
        }
        if (manifest.version.empty()) {
            error = "manifest.json 'version' must not be empty";
            return false;
        }

        // Optional fields
        if (j.contains("description") && j["description"].is_string()) {
            manifest.description = j["description"].get<std::string>();
        }
        if (j.contains("author") && j["author"].is_string()) {
            manifest.author = j["author"].get<std::string>();
        }
        if (j.contains("wasmFallback") && j["wasmFallback"].is_string()) {
            manifest.wasmFallback = j["wasmFallback"].get<std::string>();
        }
        if (j.contains("signatureFile") && j["signatureFile"].is_string()) {
            manifest.signatureFile = j["signatureFile"].get<std::string>();
        }

        // nativeLibraries: map<string, string>
        if (j.contains("nativeLibraries") && j["nativeLibraries"].is_object()) {
            for (auto& [platform, libPath] : j["nativeLibraries"].items()) {
                if (libPath.is_string()) {
                    manifest.nativeLibraries[platform] = libPath.get<std::string>();
                }
            }
        }

        return true;
    } catch (const nlohmann::json::exception& ex) {
        error = std::string("JSON parse error in manifest.json: ") + ex.what();
        return false;
    }
}

// ----------------------------------------------------------------------------
// verifyEd25519Signature
// ----------------------------------------------------------------------------

bool PluginBundleLoader::verifyEd25519Signature(const uint8_t* message,
                                                  size_t messageLen,
                                                  const std::vector<uint8_t>& signatureBytes,
                                                  const std::string& publicKeyPem,
                                                  std::string& error) {
    if (signatureBytes.size() != 64) {
        error = "Ed25519 signature must be exactly 64 bytes, got " +
                std::to_string(signatureBytes.size());
        return false;
    }

    BIO* bio = BIO_new_mem_buf(publicKeyPem.data(),
                               static_cast<int>(publicKeyPem.size()));
    if (!bio) {
        error = "BIO_new_mem_buf failed";
        return false;
    }

    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
        error = "Failed to parse Ed25519 public key: " + opensslLastError();
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        error = "EVP_MD_CTX_new failed";
        return false;
    }

    // For Ed25519, the digest parameter must be NULL.
    if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        error = "EVP_DigestVerifyInit failed: " + opensslLastError();
        return false;
    }

    int rc = EVP_DigestVerify(ctx,
                               signatureBytes.data(), signatureBytes.size(),
                               message, messageLen);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    if (rc == 1) {
        return true;
    }
    error = (rc == 0)
        ? "Ed25519 signature verification failed: signature does not match"
        : ("EVP_DigestVerify error: " + opensslLastError());
    return false;
}

// ----------------------------------------------------------------------------
// extractToTempDir
// ----------------------------------------------------------------------------

std::string PluginBundleLoader::extractToTempDir([[maybe_unused]] const std::string& bundlePath,
                                                  std::string& error) {
#ifndef THEMIS_HAVE_LIBZIP
    error = "PluginBundleLoader requires libzip, which was not found at build time";
    return {};
#else
    namespace fs = std::filesystem;

    int zipErr = 0;
    zip_t* archive = zip_open(bundlePath.c_str(), ZIP_RDONLY, &zipErr);
    if (!archive) {
        zip_error_t ze;
        zip_error_init_with_code(&ze, zipErr);
        error = std::string("Failed to open bundle archive '") + bundlePath +
                "': " + zip_error_strerror(&ze);
        zip_error_fini(&ze);
        return {};
    }

    // Create a unique temporary directory.  create_directories() returns false
    // both on failure and when the dir already exists, so we must inspect the
    // error code to distinguish the two cases.
    std::string tempDir = makeTempDirPath();
    std::error_code fsErr;
    fs::create_directories(tempDir, fsErr);
    if (fsErr && !fs::exists(tempDir)) {
        zip_close(archive);
        error = "Failed to create temp directory '" + tempDir + "': " + fsErr.message();
        return {};
    }

    // Canonicalise the temp dir for ZipSlip checking.
    std::error_code canonErr;
    fs::path tempDirCanon = fs::canonical(tempDir, canonErr);
    if (canonErr) {
        zip_close(archive);
        error = "Failed to canonicalise temp directory: " + canonErr.message();
        return {};
    }

    zip_int64_t entryCount = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < entryCount; ++i) {
        const char* entryName = zip_get_name(archive, i, 0);
        // Skip null or empty entries without error.
        if (!entryName || !*entryName) continue;

        std::string nameStr(entryName);

        // ── ZipSlip guard ────────────────────────────────────────────────
        // Reject absolute paths and paths that contain the parent-directory
        // component ("..") before we construct any filesystem path.
        if (nameStr[0] == '/' || nameStr[0] == '\\') {
            zip_close(archive);
            error = "Bundle contains absolute-path entry '" + nameStr + "' (ZipSlip rejected)";
            return {};
        }
#ifdef _WIN32
        // Reject Windows rooted drive-letter paths (e.g., "C:\..." or "C:/...").
        // A path of the form "C:filename" is a relative path on Windows and is
        // also blocked conservatively: any entry starting with "<letter>:" is
        // rejected regardless of whether a separator follows.
        if (nameStr.size() >= 2 && std::isalpha(static_cast<unsigned char>(nameStr[0])) &&
            nameStr[1] == ':') {
            zip_close(archive);
            error = "Bundle contains drive-letter entry '" + nameStr + "' (ZipSlip rejected)";
            return {};
        }
#endif
        // Normalise the entry path and check it stays inside tempDirCanon.
        // isSafeEntryPath requires resolvedStr.size() > tempStr.size(), which is
        // guaranteed here because entryName is non-empty (checked above).
        fs::path entryPath = (tempDirCanon / nameStr).lexically_normal();
        if (!isSafeEntryPath(tempDirCanon, entryPath)) {
            zip_close(archive);
            error = "Bundle entry '" + nameStr + "' would escape temp dir (ZipSlip rejected)";
            return {};
        }

        // Directory entries end with '/'
        if (nameStr.back() == '/') {
            fs::create_directories(entryPath, fsErr);
            continue;
        }

        fs::create_directories(entryPath.parent_path(), fsErr);

        zip_file_t* zf = zip_fopen_index(archive, i, 0);
        if (!zf) {
            zip_close(archive);
            error = std::string("Failed to open archive entry '") + entryName + "'";
            return {};
        }

        std::ofstream outFile(entryPath, std::ios::binary);
        if (!outFile.is_open()) {
            zip_fclose(zf);
            zip_close(archive);
            error = "Failed to create output file '" + entryPath.string() + "'";
            return {};
        }

        static constexpr zip_uint64_t kBufSize = 65536;
        std::vector<char> buf(kBufSize);
        zip_int64_t bytesRead = 0;
        while ((bytesRead = zip_fread(zf, buf.data(), kBufSize)) > 0) {
            outFile.write(buf.data(), bytesRead);
        }

        zip_fclose(zf);
        outFile.close();

        if (bytesRead < 0) {
            zip_close(archive);
            error = std::string("Read error while extracting '") + entryName + "'";
            return {};
        }
    }

    zip_close(archive);
    return tempDir;
#endif // THEMIS_HAVE_LIBZIP
}

// ----------------------------------------------------------------------------
// loadBundle
// ----------------------------------------------------------------------------

PluginBundleLoadResult PluginBundleLoader::loadBundle(const std::string& bundlePath,
                                                       ModuleLoader& loader) {
    PluginBundleLoadResult result;

    // ── Step 1: Extract archive to temp dir ───────────────────────────────
    std::string extractError;
    std::string tempDir = extractToTempDir(bundlePath, extractError);
    if (tempDir.empty()) {
        result.errorMessage = "Bundle extraction failed: " + extractError;
        spdlog::error("PluginBundleLoader: {}", result.errorMessage);
        return result;
    }
    result.tempDirectory = tempDir;
    spdlog::debug("PluginBundleLoader: extracted '{}' → '{}'", bundlePath, tempDir);

    // RAII guard: remove the temp directory on any failure path.
    // disarm() is called before the successful return at the end.
    TempDirGuard tempGuard(tempDir);

    // ── Step 2: Parse manifest.json ────────────────────────────────────────
    namespace fs = std::filesystem;
    fs::path manifestPath = fs::path(tempDir) / "manifest.json";

    std::ifstream manifestFile(manifestPath);
    if (!manifestFile.is_open()) {
        result.errorMessage = "manifest.json not found in bundle '" + bundlePath + "'";
        spdlog::error("PluginBundleLoader: {}", result.errorMessage);
        return result;
    }

    std::string manifestJson((std::istreambuf_iterator<char>(manifestFile)),
                              std::istreambuf_iterator<char>());
    manifestFile.close();

    std::string parseError;
    if (!parseManifest(manifestJson, result.manifest, parseError)) {
        result.errorMessage = "Failed to parse manifest: " + parseError;
        spdlog::error("PluginBundleLoader: {}", result.errorMessage);
        return result;
    }

    // ── Step 3: Verify Ed25519 signature of manifest.json ─────────────────
    if (!publicKeyPem_.empty()) {
        fs::path sigPath = fs::path(tempDir) / result.manifest.signatureFile;
        std::ifstream sigFile(sigPath, std::ios::binary);
        if (!sigFile.is_open()) {
            result.errorMessage = "Signature file '" +
                                   result.manifest.signatureFile +
                                   "' not found in bundle";
            spdlog::error("PluginBundleLoader: {}", result.errorMessage);
            return result;
        }

        std::vector<uint8_t> sigBytes((std::istreambuf_iterator<char>(sigFile)),
                                       std::istreambuf_iterator<char>());
        sigFile.close();

        std::string sigError;
        if (!verifyEd25519Signature(
                reinterpret_cast<const uint8_t*>(manifestJson.data()),
                manifestJson.size(),
                sigBytes,
                publicKeyPem_,
                sigError)) {
            result.errorMessage = "Bundle signature verification failed: " + sigError;
            spdlog::error("PluginBundleLoader: {}", result.errorMessage);
            return result;
        }

        spdlog::info("PluginBundleLoader: Ed25519 signature of '{}' verified OK",
                     bundlePath);
    } else if (allowUnsignedBundles_) {
        // Explicit opt-in: caller has acknowledged unsigned bundles are acceptable.
        spdlog::warn("PluginBundleLoader: unsigned bundle explicitly allowed for '{}'",
                     bundlePath);
    } else {
        // Fail-closed: no public key and no explicit opt-in — refuse to load.
        result.errorMessage =
            "Bundle '" + bundlePath + "' cannot be loaded: no public key is configured "
            "and unsigned bundles are not explicitly allowed (call setAllowUnsignedBundles(true) "
            "to opt in for development/testing only)";
        spdlog::error("PluginBundleLoader: {}", result.errorMessage);
        return result;
    }

    // ── Step 4: Select native library or fall back to WASM ────────────────
    const std::string platform = currentPlatform();
    auto it = result.manifest.nativeLibraries.find(platform);

    std::string selectedRelPath;
    if (it != result.manifest.nativeLibraries.end()) {
        selectedRelPath = it->second;
        result.usedWasmFallback = false;
        spdlog::debug("PluginBundleLoader: selected native library '{}' for platform '{}'",
                      selectedRelPath, platform);
    } else if (!result.manifest.wasmFallback.empty()) {
        selectedRelPath = result.manifest.wasmFallback;
        result.usedWasmFallback = true;
        spdlog::info("PluginBundleLoader: no native library for platform '{}' — "
                     "using WASM fallback '{}'", platform, selectedRelPath);
    } else {
        result.errorMessage = "Bundle '" + bundlePath +
                               "' has no native library for platform '" + platform +
                               "' and no WASM fallback";
        spdlog::error("PluginBundleLoader: {}", result.errorMessage);
        return result;
    }

    result.resolvedBinaryPath = (fs::path(tempDir) / selectedRelPath).string();

    if (!fs::exists(result.resolvedBinaryPath)) {
        result.errorMessage = "Resolved binary '" + result.resolvedBinaryPath +
                               "' not found after extraction";
        spdlog::error("PluginBundleLoader: {}", result.errorMessage);
        return result;
    }

    // ── Step 5: Delegate to ModuleLoader (native) or flag for WASM runtime ─
    if (!result.usedWasmFallback) {
        auto loadResult = loader.loadModule(result.resolvedBinaryPath,
                                            result.manifest.name);
        if (!loadResult.success) {
            result.errorMessage = "ModuleLoader::loadModule failed for '" +
                                   result.resolvedBinaryPath + "': " +
                                   loadResult.errorMessage;
            spdlog::error("PluginBundleLoader: {}", result.errorMessage);
            return result;
        }
        spdlog::info("PluginBundleLoader: successfully loaded bundle '{}' "
                     "(plugin '{}' v{})",
                     bundlePath, result.manifest.name, result.manifest.version);
    } else {
        // WASM-only path: the resolved path points to the .wasm binary.
        // Actual WASM execution is handled by WasmPluginSandbox; we report
        // success so the caller knows the binary path to hand off.
        spdlog::info("PluginBundleLoader: WASM-only bundle '{}' (plugin '{}' v{}) "
                     "extracted to '{}'; hand off to WASM runtime",
                     bundlePath, result.manifest.name, result.manifest.version,
                     result.resolvedBinaryPath);
    }

    // Success: disarm the RAII guard so the temp dir is preserved for the
    // duration of the loaded plugin's lifetime.
    tempGuard.disarm();
    result.success = true;
    return result;
}

} // namespace modules
} // namespace themis

