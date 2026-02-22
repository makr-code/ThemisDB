/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            module_loader.h                                    ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     661                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Module loader with DLL signature verification for modular ThemisDB
// This ensures all themis_* modules are verified before loading
// See docs/architecture/MODULARIZATION_PLAN.md for details

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include <functional>

namespace themis {
namespace modules {

// Forward declaration
class ModuleSecurityVerifier;

/**
 * @brief Module load stage for staged loading
 */
enum class LoadStage {
    UNLOADED,       // Module not loaded
    VERIFYING,      // Signature/hash verification in progress
    VERIFIED,       // Verification complete
    VALIDATING,     // ABI/metadata validation in progress
    VALIDATED,      // Validation complete
    STAGING,        // Pre-activation staging
    STAGED,         // Staged and ready for activation
    ACTIVATING,     // Health checks and activation in progress
    ACTIVE,         // Fully activated and operational
    FAILED,         // Load failed at some stage
    UNLOADING       // Unload in progress
};

/**
 * @brief Health check result for module activation
 */
struct HealthCheckResult {
    bool passed = false;
    std::string checkName;
    std::string message;
    uint64_t checkDurationMs = 0;
    
    static HealthCheckResult success(const std::string& name, const std::string& msg = "") {
        HealthCheckResult result;
        result.passed = true;
        result.checkName = name;
        result.message = msg.empty() ? "Health check passed" : msg;
        return result;
    }
    
    static HealthCheckResult failure(const std::string& name, const std::string& msg) {
        HealthCheckResult result;
        result.passed = false;
        result.checkName = name;
        result.message = msg;
        return result;
    }
};

/**
 * @brief Health check function type
 * 
 * Health check functions are called before module activation.
 * They can verify module initialization, resource availability, etc.
 * 
 * @param moduleHandle OS-specific module handle
 * @param moduleName Name of the module being checked
 * @return Health check result
 */
using HealthCheckFunction = std::function<HealthCheckResult(void* moduleHandle, const std::string& moduleName)>;

/**
 * @brief Module error codes for structured error handling
 */
enum class ModuleErrorCode {
    SUCCESS = 0,
    
    // File system errors (1xx)
    MODULE_NOT_FOUND = 100,
    MODULE_ALREADY_LOADED = 101,
    MODULE_DIRECTORY_NOT_FOUND = 102,
    MODULE_ACCESS_DENIED = 103,
    
    // Verification errors (2xx)
    VERIFICATION_FAILED = 200,
    SIGNATURE_INVALID = 201,
    HASH_MISMATCH = 202,
    CERTIFICATE_REVOKED = 203,
    CERTIFICATE_EXPIRED = 204,
    UNTRUSTED_SIGNER = 205,
    
    // Loading errors (3xx)
    LOAD_LIBRARY_FAILED = 300,
    SYMBOL_NOT_FOUND = 301,
    INITIALIZATION_FAILED = 302,
    HEALTH_CHECK_FAILED = 303,
    STAGING_FAILED = 304,
    ACTIVATION_FAILED = 305,
    
    // Version/ABI errors (4xx)
    VERSION_INCOMPATIBLE = 400,
    ABI_INCOMPATIBLE = 401,
    METADATA_MISSING = 402,
    METADATA_CORRUPTED = 403,
    
    // Policy errors (5xx)
    POLICY_VIOLATION = 500,
    BLACKLISTED = 501,
    QUARANTINED = 502,
    
    // Unknown/Internal errors (9xx)
    INTERNAL_ERROR = 900,
    UNKNOWN_ERROR = 999
};

/**
 * @brief Error category for error handling strategy
 */
enum class ErrorCategory {
    TRANSIENT,      // Temporary failure, retry may succeed
    PERMANENT,      // Persistent failure, retry unlikely to help
    RECOVERABLE,    // Can be fixed by user action
    FATAL           // Unrecoverable, requires system intervention
};

/**
 * @brief Module metadata (version, ABI, build info)
 */
struct ModuleMetadata {
    std::string version;        // Semantic version (e.g., "1.2.3")
    std::string abiVersion;     // ABI version for compatibility checking
    std::string buildId;        // Build ID / commit hash
    std::string buildDate;      // Build timestamp
    std::string compiler;       // Compiler version
    uint32_t themisMajor = 0;   // ThemisDB major version
    uint32_t themisMinor = 0;   // ThemisDB minor version
    uint32_t themisPatch = 0;   // ThemisDB patch version
    
    bool isValid() const {
        return !version.empty() && themisMajor > 0;
    }
};

/**
 * @brief Module failure history for quarantine and backoff
 */
struct ModuleFailureHistory {
    std::string modulePath;
    std::vector<uint64_t> failureTimestamps;  // Unix timestamps of failures
    uint32_t consecutiveFailures = 0;
    uint64_t lastFailureTime = 0;
    uint64_t quarantineTime = 0;              // When quarantined (0 = not quarantined)
    uint64_t nextRetryTime = 0;               // Exponential backoff time
    ModuleErrorCode lastErrorCode = ModuleErrorCode::SUCCESS;
    std::string lastErrorMessage;
    
    bool isQuarantined() const {
        return quarantineTime > 0;
    }
    
    bool canRetry(uint64_t currentTime) const {
        return currentTime >= nextRetryTime;
    }
};

/**
 * @brief Module metrics for observability
 */
struct ModuleMetrics {
    // Load statistics
    uint64_t totalLoadAttempts = 0;
    uint64_t successfulLoads = 0;
    uint64_t failedLoads = 0;
    uint64_t totalUnloads = 0;
    
    // Duration statistics (milliseconds)
    uint64_t totalLoadDurationMs = 0;
    uint64_t minLoadDurationMs = UINT64_MAX;
    uint64_t maxLoadDurationMs = 0;
    
    // Verification statistics
    uint64_t verificationSuccesses = 0;
    uint64_t verificationFailures = 0;
    
    // Quarantine statistics
    uint64_t quarantineEvents = 0;
    uint64_t quarantineReleases = 0;
    uint32_t currentlyQuarantined = 0;
    
    // Error breakdown
    std::map<ModuleErrorCode, uint64_t> errorCounts;
    
    double getSuccessRate() const {
        return totalLoadAttempts > 0 
            ? (double)successfulLoads / totalLoadAttempts 
            : 0.0;
    }
    
    double getAverageLoadDurationMs() const {
        return successfulLoads > 0 
            ? (double)totalLoadDurationMs / successfulLoads 
            : 0.0;
    }
};

/**
 * @brief Module verification result
 */
struct ModuleVerificationResult {
    bool success = false;
    ModuleErrorCode errorCode = ModuleErrorCode::SUCCESS;
    ErrorCategory errorCategory = ErrorCategory::PERMANENT;
    std::string errorMessage;
    std::string moduleHash;
    std::string modulePath;
    uint64_t verificationTimestamp = 0;
    ModuleMetadata metadata;
    
    // Authenticode information (Windows only)
    std::string authenticodeSigner;  // Certificate subject (e.g., "CN=ThemisDB GmbH")
    bool hasAuthenticode = false;
    int zoneId = -1;  // -1 = no Zone.Identifier, 0-4 = zone
};

/**
 * @brief Loaded module information
 */
struct LoadedModule {
    std::string name;           // e.g., "themis_storage"
    std::string path;           // Full path to DLL/SO
    std::string version;        // Module version (from metadata)
    std::string fileHash;       // SHA-256 hash
    void* handle = nullptr;     // OS-specific handle
    bool verified = false;      // Signature verification status
    uint64_t loadTime = 0;      // Unix timestamp
    uint64_t loadDurationMs = 0; // Load duration in milliseconds
    ModuleMetadata metadata;    // Full metadata
    LoadStage currentStage = LoadStage::UNLOADED;  // Current load stage
    std::vector<HealthCheckResult> healthChecks;   // Health check results
    bool fullyActivated = false; // True if passed all stages
};

/**
 * @brief Module loader with security verification
 * 
 * This class is responsible for loading ThemisDB modular libraries
 * with mandatory signature verification to prevent corrupted or
 * malicious DLL injection.
 * 
 * Key features:
 * - SHA-256 hash verification
 * - Digital signature verification (X.509)
 * - Blacklist/whitelist support
 * - Certificate chain validation
 * - Audit logging of all load attempts
 */
class ModuleLoader {
public:
    ModuleLoader();
    ~ModuleLoader();
    
    // Prevent copying
    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader& operator=(const ModuleLoader&) = delete;
    
    /**
     * @brief Load a ThemisDB module with security verification
     * @param modulePath Full path to the module DLL/SO
     * @param moduleName Expected module name (e.g., "themis_storage")
     * @return Verification result with success status
     */
    ModuleVerificationResult loadModule(const std::string& modulePath, 
                                       const std::string& moduleName);
    
    /**
     * @brief Load all required modules from a directory
     * @param moduleDirectory Directory containing themis_* modules
     * @return Number of successfully loaded modules
     */
    size_t loadAllModules(const std::string& moduleDirectory);
    
    /**
     * @brief Unload a specific module
     * @param moduleName Name of the module to unload
     */
    void unloadModule(const std::string& moduleName);
    
    /**
     * @brief Unload all modules
     */
    void unloadAllModules();
    
    /**
     * @brief Check if a module is loaded
     * @param moduleName Name of the module
     * @return true if loaded, false otherwise
     */
    bool isModuleLoaded(const std::string& moduleName) const;
    
    /**
     * @brief Get information about a loaded module
     * @param moduleName Name of the module
     * @return Optional module info (nullopt if not loaded)
     */
    std::optional<LoadedModule> getModuleInfo(const std::string& moduleName) const;
    
    /**
     * @brief Get all loaded modules
     * @return Vector of all loaded modules
     */
    std::vector<LoadedModule> getAllLoadedModules() const;
    
    /**
     * @brief Set whether to require signature verification
     * @param require If true, reject unsigned modules (production mode)
     */
    void setRequireSignature(bool require);
    
    /**
     * @brief Set whether to allow unsigned modules
     * @param allow If true, allow unsigned modules (development mode)
     */
    void setAllowUnsigned(bool allow);
    
    /**
     * @brief Add a hash to the module whitelist
     * @param hash SHA-256 hash to whitelist
     */
    void addWhitelistedHash(const std::string& hash);
    
    /**
     * @brief Add a hash to the module blacklist
     * @param hash SHA-256 hash to blacklist
     */
    void addBlacklistedHash(const std::string& hash);
    
    /**
     * @brief Get failure history for a module
     * @param modulePath Path to module
     * @return Optional failure history (nullopt if no failures)
     */
    std::optional<ModuleFailureHistory> getFailureHistory(const std::string& modulePath) const;
    
    /**
     * @brief Get all quarantined modules
     * @return Vector of quarantined module paths
     */
    std::vector<std::string> getQuarantinedModules() const;
    
    /**
     * @brief Release a module from quarantine
     * @param modulePath Path to module to release
     * @return true if released, false if not quarantined
     */
    bool releaseFromQuarantine(const std::string& modulePath);
    
    /**
     * @brief Clear failure history for a module
     * @param modulePath Path to module
     */
    void clearFailureHistory(const std::string& modulePath);
    
    /**
     * @brief Get current module metrics
     * @return Current metrics snapshot
     */
    ModuleMetrics getMetrics() const;
    
    /**
     * @brief Reset all metrics to zero
     */
    void resetMetrics();
    
    /**
     * @brief Check if ABI is compatible with ThemisDB
     * @param metadata Module metadata to check
     * @return true if compatible, false otherwise
     */
    bool isABICompatible(const ModuleMetadata& metadata) const;
    
    /**
     * @brief Set quarantine threshold (consecutive failures before quarantine)
     * @param threshold Number of failures (default: 3)
     */
    void setQuarantineThreshold(uint32_t threshold);
    
    /**
     * @brief Set maximum backoff time in seconds
     * @param maxSeconds Maximum backoff time (default: 300 = 5 minutes)
     */
    void setMaxBackoffSeconds(uint32_t maxSeconds);
    
    /**
     * @brief Register a health check function
     * @param checkName Unique name for this health check
     * @param checkFunc Health check function
     */
    void registerHealthCheck(const std::string& checkName, HealthCheckFunction checkFunc);
    
    /**
     * @brief Clear all registered health checks
     */
    void clearHealthChecks();
    
    /**
     * @brief Query current load stage of a module
     * @param moduleName Name of the module
     * @return Optional load stage (nullopt if not found)
     */
    std::optional<LoadStage> queryModuleStage(const std::string& moduleName) const;
    
    /**
     * @brief Get health check results for a module
     * @param moduleName Name of the module
     * @return Vector of health check results
     */
    std::vector<HealthCheckResult> getHealthCheckResults(const std::string& moduleName) const;
    
    /**
     * @brief Enable or disable staged loading
     * @param enable If true, use staged loading; if false, load directly
     */
    void setStagedLoadingEnabled(bool enable);
    
    /**
     * @brief Export security audit log
     * @param outputPath Path to export JSON audit log
     * @return true if successful, false otherwise
     */
    bool exportAuditLog(const std::string& outputPath) const;
    
#ifdef _WIN32
    /**
     * @brief Check if module has Zone.Identifier (downloaded from internet)
     * @param modulePath Path to module DLL
     * @return Zone ID (0-4), or -1 if no Zone.Identifier
     * 
     * Zone IDs:
     * - 0 = Local Computer
     * - 1 = Local Intranet  
     * - 2 = Trusted Sites
     * - 3 = Internet (triggers warnings)
     * - 4 = Restricted Sites
     */
    int getZoneIdentifier(const std::string& modulePath) const;
    
    /**
     * @brief Remove Zone.Identifier (unblock file)
     * @param modulePath Path to module DLL
     * @return true if successful
     */
    bool removeZoneIdentifier(const std::string& modulePath);
    
    /**
     * @brief Verify Authenticode signature (Windows PE signature)
     * @param modulePath Path to module DLL
     * @param signerInfo Output: certificate subject if signed
     * @return true if Authenticode signature valid
     * 
     * This verifies the embedded PE signature that Windows checks.
     * Shows up in file properties "Digital Signatures" tab.
     */
    bool verifyAuthenticodeSignature(const std::string& modulePath, 
                                     std::string& signerInfo) const;
#endif

#ifdef __linux__
    /**
     * @brief Verify GPG signature of module (Linux)
     * @param modulePath Path to .so file
     * @param signaturePath Path to .sig/.asc file (optional, auto-detected)
     * @return true if GPG signature valid
     * 
     * Checks for detached GPG signatures (.asc or .sig files).
     * Equivalent to Windows Authenticode but uses separate signature files.
     */
    bool verifyGPGSignature(const std::string& modulePath,
                           const std::string& signaturePath = "") const;
    
    /**
     * @brief Check extended attributes (download marker)
     * @param modulePath Path to .so file
     * @return Map of attribute name → value
     * 
     * Reads extended attributes (xattr), similar to Zone.Identifier on Windows.
     * Attributes like user.download.source indicate internet downloads.
     */
    std::map<std::string, std::string> getExtendedAttributes(const std::string& modulePath) const;
    
    /**
     * @brief Read ELF metadata (Build ID, version info)
     * @param modulePath Path to .so file
     * @return Metadata string (Build ID, .comment section)
     * 
     * Extracts metadata from ELF notes and sections.
     * Similar to PE version resources on Windows.
     */
    std::string readELFMetadata(const std::string& modulePath) const;
#endif
    
private:
    std::vector<LoadedModule> loadedModules_;
    std::unique_ptr<ModuleSecurityVerifier> verifier_;
    
    // Quarantine and backoff tracking
    std::map<std::string, ModuleFailureHistory> failureHistory_;
    uint32_t quarantineThreshold_ = 3;     // Failures before quarantine
    uint32_t maxBackoffSeconds_ = 300;      // 5 minutes max backoff
    
    // Metrics tracking
    ModuleMetrics metrics_;
    
    // ThemisDB version for ABI compatibility
    uint32_t themisABIMajor_ = 1;
    uint32_t themisABIMinor_ = 0;
    
    // Staged loading
    bool stagedLoadingEnabled_ = true;     // Default: use staged loading
    std::map<std::string, HealthCheckFunction> healthChecks_;
    std::map<std::string, ModuleMetadata> metadataCache_;  // Cache to avoid double-loading
    
    // Platform-specific loading functions
    void* loadLibrary(const std::string& path);
    void unloadLibrary(void* handle);
    void* getSymbol(void* handle, const std::string& symbolName);
    
    // Helper functions
    bool verifyModuleSignature(const std::string& modulePath, std::string& errorMessage);
    std::string calculateModuleHash(const std::string& modulePath);
    std::string getModuleNameFromPath(const std::string& path);
    bool isThemisModule(const std::string& filename);
    
    // Metadata extraction
    ModuleMetadata extractModuleMetadata(const std::string& modulePath);
    std::string getErrorMessage(ModuleErrorCode code) const;
    ErrorCategory categorizeError(ModuleErrorCode code) const;
    
    // Quarantine and backoff helpers
    void recordFailure(const std::string& modulePath, ModuleErrorCode errorCode, const std::string& errorMessage);
    bool shouldQuarantine(const std::string& modulePath) const;
    void quarantineModule(const std::string& modulePath);
    uint64_t calculateBackoffTime(uint32_t consecutiveFailures) const;
    bool checkQuarantine(const std::string& modulePath, ModuleVerificationResult& result);
    
    // Metrics helpers
    void updateMetrics(bool success, uint64_t durationMs, ModuleErrorCode errorCode);
    
    // Staged loading helpers
    bool updateModuleStage(const std::string& moduleName, LoadStage newStage);
    bool runHealthChecks(LoadedModule& module, ModuleVerificationResult& result);
    ModuleMetadata extractMetadataFromHandle(void* handle);
    ModuleMetadata getCachedMetadata(const std::string& modulePath);
};

/**
 * @brief Module security verifier (wrapper around PluginSecurityVerifier)
 * 
 * This class adapts the existing plugin security infrastructure
 * for use with ThemisDB modular libraries.
 */
class ModuleSecurityVerifier {
public:
    ModuleSecurityVerifier();
    ~ModuleSecurityVerifier();
    
    /**
     * @brief Verify a module before loading
     * @param modulePath Path to module DLL/SO
     * @param errorMessage Output parameter for error details
     * @return true if module is safe to load, false otherwise
     */
    bool verifyModule(const std::string& modulePath, std::string& errorMessage);
    
    /**
     * @brief Calculate SHA-256 hash of module file
     * @param modulePath Path to module file
     * @return Hex-encoded SHA-256 hash (empty string on error)
     */
    std::string calculateFileHash(const std::string& modulePath);
    
    /**
     * @brief Set security policy
     */
    void setRequireSignature(bool require);
    void setAllowUnsigned(bool allow);
    void addWhitelistedHash(const std::string& hash);
    void addBlacklistedHash(const std::string& hash);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Singleton module registry
 * 
 * Tracks all loaded modules globally to prevent duplicate loading
 * and provide system-wide module status.
 */
class ModuleRegistry {
public:
    static ModuleRegistry& instance();
    
    /**
     * @brief Register a loaded module
     */
    void registerModule(const LoadedModule& module);
    
    /**
     * @brief Unregister a module
     */
    void unregisterModule(const std::string& moduleName);
    
    /**
     * @brief Check if module is registered
     */
    bool isRegistered(const std::string& moduleName) const;
    
    /**
     * @brief Get all registered modules
     */
    std::vector<LoadedModule> getAllModules() const;
    
    /**
     * @brief Clear all registrations (for testing)
     */
    void clear();
    
private:
    ModuleRegistry() = default;
    ~ModuleRegistry() = default;
    ModuleRegistry(const ModuleRegistry&) = delete;
    ModuleRegistry& operator=(const ModuleRegistry&) = delete;
    
    std::vector<LoadedModule> modules_;
};

} // namespace modules
} // namespace themis
