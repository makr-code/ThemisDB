/**
 * @file module_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Module loader with DLL signature verification for modular ThemisDB
// This ensures all themis_* modules are verified before loading
// See docs/architecture/MODULARIZATION_PLAN.md for details

#pragma once

#include "themis/module_hash_verifier.h"
#include "acceleration/plugin_security.h"

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstdint>

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
    std::string checkName = {};
    std::string message = {};
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
    
    // Dependency errors (1xx continued)
    DEPENDENCY_NOT_FOUND = 110,        ///< Required dependency not registered or missing
    DEPENDENCY_CIRCULAR = 111,          ///< Circular dependency detected in module graph
    DEPENDENCY_VERSION_MISMATCH = 112,  ///< Dependency version constraint not satisfied
    
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
    ZONE_ID_BLOCKED = 503,  ///< Windows Zone.Identifier marks file as Internet/Restricted zone
    
    // Unknown/Internal errors (9xx)
    INTERNAL_ERROR = 900,
    UNKNOWN_ERROR = 999
};

/**
 * @brief Error category for error handling strategy
 */
enum class ErrorCategory {
    NONE,           // No error (operation succeeded)
    TRANSIENT,      // Temporary failure, retry may succeed
    PERMANENT,      // Persistent failure, retry unlikely to help
    RECOVERABLE,    // Can be fixed by user action
    FATAL           // Unrecoverable, requires system intervention
};

/**
 * @brief Dependency declaration for a module.
 *
 * Each entry describes one other module that must be present (or present
 * with a compatible version) before this module can be activated.
 */
struct ModuleDependency {
    std::string name;           ///< Module name (e.g., "themis_base")
    std::string minVersion;     ///< Minimum compatible version, "" = unconstrained
    std::string maxVersion;     ///< Maximum compatible version, "" = unconstrained
    bool required = true;       ///< If false, the dep is optional (load-order hint only)
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
    
    std::vector<ModuleDependency> dependencies; ///< Declared module dependencies
    
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
 * @brief Configuration for the plugin watchdog monitor.
 *
 * All time fields are in milliseconds for easy unit-testing.
 */
struct WatchdogConfig {
    /// Interval between successive health-check sweeps (default: 30 s).
    uint64_t check_interval_ms = 30'000;

    /// Maximum number of restart attempts before a module is declared permanently failed
    /// (0 = unlimited).
    uint32_t max_restart_attempts = 5;

    /// Initial backoff delay after the first restart attempt (default: 5 s).
    uint64_t initial_backoff_ms = 5'000;

    /// Multiplicative factor applied to backoff after each consecutive failure (default: 2×).
    double backoff_multiplier = 2.0;

    /// Upper bound on calculated backoff (default: 5 min).
    uint64_t max_backoff_ms = 300'000;

    /// When false the watchdog thread is started but performs no work until re-enabled.
    bool enabled = true;
};

/**
 * @brief Per-module runtime statistics maintained by the plugin watchdog.
 */
struct WatchdogModuleStats {
    std::string moduleName;
    std::string modulePath;

    uint32_t restart_count = 0;         ///< Total successful restart count
    uint32_t consecutive_failures = 0;  ///< Failures since last successful health-check
    uint64_t last_health_check_ms = 0;  ///< Wall-clock time of last check (epoch ms)
    uint64_t last_failure_ms = 0;       ///< Wall-clock time of last failure (epoch ms)
    uint64_t last_restart_ms = 0;       ///< Wall-clock time of last restart (epoch ms)
    uint64_t next_retry_ms = 0;         ///< Earliest time for next restart attempt (epoch ms)

    bool permanently_failed = false;    ///< True when max_restart_attempts exhausted

    /// Human-readable last error message from the failing health check.
    std::string last_error;
};

/**
 * @brief Module verification result
 */
struct ModuleVerificationResult {
    bool success = false;
    ModuleErrorCode errorCode = ModuleErrorCode::SUCCESS;
    ErrorCategory errorCategory = ErrorCategory::NONE;
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
     * @brief Load a SHA-256 hash manifest for module integrity verification.
     *
     * When a manifest is set, every call to loadModule() verifies the module's
     * SHA-256 hash against the manifest entry for that module name.  If the
     * module name is present in the manifest and the hashes do not match, the
     * load is rejected with HASH_MISMATCH.  Modules not listed in the manifest
     * are still loaded — the manifest acts as an integrity verification list
     * for known modules, not a global block/allowlist.
     *
     * @param manifestPath Path to JSON manifest file
     *                     (format: { "moduleName": "hex-sha256", … }).
     * @return true if manifest was loaded successfully, false on I/O or
     *         parse error.
     */
    bool setHashManifest(const std::string& manifestPath);

    /**
     * @brief Export security audit log
     * @param outputPath Path to export JSON audit log
     * @return true if successful, false otherwise
     */
    bool exportAuditLog(const std::string& outputPath) const;
    
    /**
     * @brief Get the per-plugin audit trail (load, unload, errors)
     * 
     * Returns all recorded security and lifecycle events for the given
     * module path in chronological order.  Events include PLUGIN_LOADED,
     * PLUGIN_UNLOADED, PLUGIN_LOAD_FAILED, and related security events.
     *
     * @param modulePath Full path to the module (as supplied to loadModule)
     * @return Vector of audit events for that specific plugin
     */
    std::vector<themis::acceleration::PluginSecurityEvent>
    getPluginAuditTrail(const std::string& modulePath) const;

    // =========================================================================
    // Plugin Watchdog — automatic health monitoring and restart (Issue #2373)
    // =========================================================================

    /**
     * @brief Configure the watchdog before starting it.
     *
     * May be called before or after startWatchdog(); if called while the
     * watchdog is running the new settings take effect on the next sweep.
     *
     * @param config Watchdog configuration parameters.
     */
    void configureWatchdog(const WatchdogConfig& config);

    /**
     * @brief Start the watchdog background thread.
     *
     * The thread performs periodic health checks on all loaded modules and
     * automatically restarts any module that fails its checks, applying
     * exponential backoff between successive restart attempts.
     *
     * Calling startWatchdog() while it is already running is a no-op.
     */
    void startWatchdog();

    /**
     * @brief Stop the watchdog background thread.
     *
     * Blocks until the background thread has exited.  Calling stopWatchdog()
     * when the watchdog is not running is a no-op.
     */
    void stopWatchdog();

    /**
     * @brief Return true if the watchdog background thread is running.
     */
    bool isWatchdogRunning() const;

    /**
     * @brief Get watchdog statistics for a specific module.
     *
     * @param moduleName Name of the module.
     * @return Optional stats (nullopt if module not tracked).
     */
    std::optional<WatchdogModuleStats> getWatchdogStats(const std::string& moduleName) const;

    /**
     * @brief Get watchdog statistics for all tracked modules.
     *
     * @return Map from module name to watchdog stats.
     */
    std::map<std::string, WatchdogModuleStats> getAllWatchdogStats() const;

    /**
     * @brief Reset watchdog statistics for all modules.
     *
     * Clears restart counts, failure counters, and permanently_failed flags.
     * Does not stop the watchdog if it is running.
     */
    void resetWatchdogStats();
    
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
    std::unordered_map<std::string, LoadedModule> loadedModules_;
    mutable std::shared_mutex modulesMutex_;
    std::unique_ptr<ModuleSecurityVerifier> verifier_;
    
    // SHA-256 hash manifest for integrity verification (Issue #2471)
    ModuleHashVerifier hashVerifier_;
    
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

    // -------------------------------------------------------------------------
    // Watchdog state (Issue #2373)
    // -------------------------------------------------------------------------
    WatchdogConfig watchdogConfig_;
    std::map<std::string, WatchdogModuleStats> watchdogStats_;  // keyed by module name
    std::thread watchdogThread_;
    mutable std::mutex watchdogMutex_;
    std::condition_variable watchdogCv_;
    std::atomic<bool> watchdogRunning_{false};
    
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

    // Watchdog helpers (Issue #2373)
    void watchdogLoop();
    void watchdogCheckAllModules();
    bool watchdogRunHealthChecks(LoadedModule& module, std::string& errorMessage);
    bool watchdogRestartModule(WatchdogModuleStats& stats, const std::string& modulePath);
    uint64_t watchdogCalculateBackoff(uint32_t consecutiveFailures) const;
    static uint64_t nowMs();
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

// ============================================================================
// MODULE DEPENDENCY RESOLUTION
// ============================================================================

/**
 * @brief Result of a dependency resolution pass.
 */
struct DependencyResolutionResult {
    bool success = false;
    std::vector<std::string> loadOrder;             ///< Resolved load order (deps first)
    std::vector<std::string> missingRequired;        ///< Required deps absent from registry
    std::vector<std::vector<std::string>> cycles;   ///< Detected dependency cycles
    /// Version-constraint violations: each entry is "module→dep (got X, need ≥Y ≤Z)"
    std::vector<std::string> versionMismatches;
    std::string errorMessage;
};

/**
 * @brief Resolves module dependencies and computes a safe load order.
 *
 * Modules register themselves together with their dependency declarations.
 * Calling resolve() returns a topological ordering that guarantees every
 * dependency is loaded before the module that depends on it.  Circular
 * dependencies and missing required dependencies are reported as errors.
 *
 * Usage:
 * @code
 *   ModuleDependencyResolver resolver;
 *   resolver.registerModule("themis_base",    {});
 *   resolver.registerModule("themis_storage", {{"themis_base"}});
 *   resolver.registerModule("themis_query",   {{"themis_storage"}, {"themis_base"}});
 *
 *   auto result = resolver.resolve();
 *   if (result.success) {
 *       for (const auto& mod : result.loadOrder) { ... }
 *   }
 * @endcode
 */
class ModuleDependencyResolver {
public:
    /**
     * @brief Register a module together with its dependency declarations.
     *
     * Registering a module a second time replaces the previous registration.
     *
     * @param name     Unique module name (e.g., "themis_base").
     * @param version  Semantic version of this module (e.g., "1.2.3").  May be "".
     * @param deps     Dependency list; may be empty.
     */
    void registerModule(const std::string& name,
                        const std::string& version,
                        const std::vector<ModuleDependency>& deps);

    /**
     * @brief Convenience overload — registers a module with no version string.
     *
     * Version constraints declared by other modules against this module will
     * only be satisfied if they are unconstrained (both minVersion and
     * maxVersion are empty).
     *
     * @param name  Unique module name.
     * @param deps  Dependency list; may be empty.
     */
    void registerModule(const std::string& name,
                        const std::vector<ModuleDependency>& deps);

    /**
     * @brief Resolve load order for all registered modules.
     *
     * @return Resolution result with load order or error information.
     */
    DependencyResolutionResult resolve() const;

    /**
     * @brief Resolve load order for a specific subset of modules.
     *
     * Transitive dependencies that are registered but not in @p moduleNames
     * are included in the resolution automatically.
     *
     * @param moduleNames  Modules to resolve (by name).
     * @return Resolution result.
     */
    DependencyResolutionResult resolveFor(
        const std::vector<std::string>& moduleNames) const;

    /**
     * @brief Remove all registered module entries.
     */
    void clear();

    /**
     * @brief Information about a single registered module.
     *
     * Returned by @c getRegisteredModules() for inspection and
     * visualisation purposes.
     */
    struct RegisteredModuleInfo {
        std::string name = {};
        std::string version;
        std::vector<ModuleDependency> deps;
    };

    /**
     * @brief Return all registered modules and their dependency declarations.
     *
     * Intended for read-only inspection (e.g., dependency graph
     * visualisation). The returned snapshot is sorted by module name.
     *
     * @return Sorted vector of registered module information.
     */
    std::vector<RegisteredModuleInfo> getRegisteredModules() const;

    /**
     * @brief Check if a version string satisfies the given constraints.
     *
     * Supports semantic version strings of the form "major.minor.patch".
     * An empty constraint string is treated as unconstrained (always passes).
     * An empty @p version satisfies only fully unconstrained dependencies.
     *
     * @param version     Version to test.
     * @param minVersion  Lower bound (inclusive).  "" = no lower bound.
     * @param maxVersion  Upper bound (inclusive).  "" = no upper bound.
     * @return true if @p version satisfies both constraints.
     */
    static bool isVersionCompatible(const std::string& version,
                                    const std::string& minVersion,
                                    const std::string& maxVersion);

private:
    struct ModuleEntry {
        std::string version;
        std::vector<ModuleDependency> deps;
    };
    std::map<std::string, ModuleEntry> modules_;

    /// Topological sort (Kahn's algorithm) over the supplied node set.
    DependencyResolutionResult topologicalSort(
        const std::vector<std::string>& nodes) const;
};

// ============================================================================
// CROSS-PLATFORM PLUGIN BUNDLE FORMAT  (v1.4.0)
// ============================================================================

/**
 * @brief Parsed contents of the manifest.json file inside a PluginBundle.
 *
 * A PluginBundle is a ZIP archive (conventionally using the .tdb extension)
 * that packages native libraries for multiple platforms together with an
 * optional WASM fallback and an Ed25519 signature of the manifest.
 *
 * Archive layout:
 * @code
 *   plugin.tdb
 *   ├── manifest.json       – this structure, serialised as JSON
 *   ├── plugin.sig          – Ed25519 signature of manifest.json (raw bytes)
 *   ├── linux-x86_64/
 *   │   └── libplugin.so
 *   ├── windows-x86_64/
 *   │   └── plugin.dll
 *   ├── macos-arm64/
 *   │   └── libplugin.dylib
 *   └── plugin.wasm         – optional portable WASM fallback
 * @endcode
 *
 * The platform token format is "<os>-<arch>" where os ∈ {linux, windows, macos}
 * and arch ∈ {x86_64, arm64, arm, x86}.
 */
struct PluginBundleManifest {
    std::string name;           ///< Plugin name (e.g., "themis_my_backend")
    std::string version;        ///< Semantic version (e.g., "1.0.0")
    std::string description;    ///< Human-readable description (may be empty)
    std::string author;         ///< Plugin author / organisation (may be empty)

    /// Maps the canonical platform token to the in-archive path of the
    /// corresponding native library.
    /// Example: { "linux-x86_64" → "linux-x86_64/libplugin.so" }
    std::map<std::string, std::string> nativeLibraries;

    /// In-archive path of the optional WASM fallback ("" = none present).
    std::string wasmFallback;

    /// In-archive path of the Ed25519 signature file (default: "plugin.sig").
    std::string signatureFile = "plugin.sig";

    /// Returns true when the mandatory fields (name, version) are non-empty.
    bool isValid() const {
        return !name.empty() && !version.empty();
    }
};

/**
 * @brief Result returned by PluginBundleLoader::loadBundle().
 */
struct PluginBundleLoadResult {
    bool success = false;
    std::string errorMessage;

    /// Full filesystem path to the binary (native .so/.dll/.dylib or .wasm)
    /// that was selected and passed to the ModuleLoader.  Populated on both
    /// success and the WASM-fallback path.
    std::string resolvedBinaryPath;

    /// True when no native library was found for the current platform and the
    /// WASM fallback was used instead.
    bool usedWasmFallback = false;

    /// The parsed manifest from the bundle.  Populated on success.
    PluginBundleManifest manifest;

    /// Path to the temporary directory where the bundle was unpacked.
    /// The directory persists until the loaded native library is unloaded
    /// (dlopen/LoadLibrary hold a reference to files inside it).
    std::string tempDirectory;
};

/**
 * @brief Loader for cross-platform PluginBundle ZIP archives.
 *
 * loadBundle() orchestrates the following steps:
 *  1. Unpack the ZIP archive to a unique temporary directory.
 *  2. Parse manifest.json.
 *  3. Verify the Ed25519 signature of manifest.json (skipped when no public key
 *     has been configured — development/test mode only).
 *  4. Select the native library for the running platform (currentPlatform()).
 *     Falls back to the WASM entry if no native library is present.
 *  5. Delegate the actual binary load to the supplied ModuleLoader instance.
 *
 * Thread-Safety: An individual PluginBundleLoader instance is NOT thread-safe.
 * Create one instance per loading thread or serialise access externally.
 */
class PluginBundleLoader {
public:
    PluginBundleLoader();
    ~PluginBundleLoader();

    PluginBundleLoader(const PluginBundleLoader&) = delete;
    PluginBundleLoader& operator=(const PluginBundleLoader&) = delete;

    /**
     * @brief Set the PEM-encoded Ed25519 public key used to verify bundle
     *        signatures.
     *
     * When no key is set (the default) signature verification is skipped,
     * which is only suitable for development / unit-testing.  In production,
     * always set a public key before calling loadBundle().
     *
     * @param publicKeyPem  PEM-encoded Ed25519 public key.
     */
    void setPublicKey(const std::string& publicKeyPem);

    /**
     * @brief Explicitly allow loading bundles without a signature.
     *
     * By default (allow = false) loadBundle() fails when no public key has
     * been configured, so that unsigned bundles are never silently accepted.
     * Pass allow = true only in development / testing environments where you
     * intentionally want to skip signature verification.
     *
     * @param allow  If true, unsigned bundles are accepted when no public key
     *               is set.  If false (default), missing public key is an error.
     */
    void setAllowUnsignedBundles(bool allow);

    /**
     * @brief Return the canonical platform token for the current build.
     *
     * Examples: "linux-x86_64", "linux-arm64", "windows-x86_64",
     *           "macos-arm64", "macos-x86_64".
     */
    static std::string currentPlatform();

    /**
     * @brief Load a PluginBundle archive and delegate to a ModuleLoader.
     *
     * @param bundlePath  Path to the .tdb (ZIP) bundle file.
     * @param loader      ModuleLoader instance used for the final native-
     *                    library load step.
     * @return PluginBundleLoadResult indicating success/failure and details.
     */
    PluginBundleLoadResult loadBundle(const std::string& bundlePath,
                                      ModuleLoader& loader);

    /**
     * @brief Parse a PluginBundleManifest from raw JSON text.
     *
     * Exposed as a public static helper to facilitate unit testing without
     * requiring a real ZIP archive.
     *
     * @param jsonText  Contents of manifest.json.
     * @param manifest  Output: populated on success.
     * @param error     Output: human-readable error message on failure.
     * @return true if parsing succeeded and the manifest is valid.
     */
    static bool parseManifest(const std::string& jsonText,
                               PluginBundleManifest& manifest,
                               std::string& error);

    /**
     * @brief Verify an Ed25519 signature over a message.
     *
     * @param message       Pointer to the data that was signed.
     * @param messageLen    Length of the data in bytes.
     * @param signatureBytes Raw 64-byte Ed25519 signature.
     * @param publicKeyPem  PEM-encoded Ed25519 public key.
     * @param error         Output: error description on failure.
     * @return true if the signature is valid; false otherwise.
     */
    static bool verifyEd25519Signature(const uint8_t* message,
                                       size_t messageLen,
                                       const std::vector<uint8_t>& signatureBytes,
                                       const std::string& publicKeyPem,
                                       std::string& error);

private:
    std::string publicKeyPem_;
    bool allowUnsignedBundles_ = false;  ///< Must be explicitly opted in; never true by default.

    /// Unpack the ZIP at bundlePath into a new temporary subdirectory.
    /// Returns the path to the temp dir on success or an empty string + error.
    static std::string extractToTempDir(const std::string& bundlePath,
                                        std::string& error);
};

} // namespace modules
} // namespace themis
