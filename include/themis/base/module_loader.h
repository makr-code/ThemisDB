// Module loader with DLL signature verification for modular ThemisDB
// This ensures all themis_* modules are verified before loading
// See docs/architecture/MODULARIZATION_PLAN.md for details

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace themis {
namespace modules {

// Forward declaration
class ModuleSecurityVerifier;

/**
 * @brief Module verification result
 */
struct ModuleVerificationResult {
    bool success = false;
    std::string errorMessage;
    std::string moduleHash;
    std::string modulePath;
    uint64_t verificationTimestamp = 0;
    
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
    std::string version;        // Module version
    std::string fileHash;       // SHA-256 hash
    void* handle = nullptr;     // OS-specific handle
    bool verified = false;      // Signature verification status
    uint64_t loadTime = 0;      // Unix timestamp
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
    
    // Platform-specific loading functions
    void* loadLibrary(const std::string& path);
    void unloadLibrary(void* handle);
    void* getSymbol(void* handle, const std::string& symbolName);
    
    // Helper functions
    bool verifyModuleSignature(const std::string& modulePath, std::string& errorMessage);
    std::string calculateModuleHash(const std::string& modulePath);
    std::string getModuleNameFromPath(const std::string& path);
    bool isThemisModule(const std::string& filename);
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
