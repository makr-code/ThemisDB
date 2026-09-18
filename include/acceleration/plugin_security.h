/**
 * @file plugin_security.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <chrono>
#include <mutex>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/err.h>

namespace themis {
namespace acceleration {

/// @brief Digital signature and hash information for plugin verification.
struct PluginSignature {
    std::string sha256Hash;          ///< SHA-256 hash of the DLL/SO binary file
    std::string signature;           ///< Digital signature (RSA or ECDSA)
    std::string signingCertificate;  ///< X.509 certificate of the signer
    std::string issuer;              ///< Certificate issuer Distinguished Name
    std::string subject;             ///< Certificate subject Distinguished Name
    uint64_t timestamp = 0;          ///< Unix timestamp when signature was created
    bool verified = false;           ///< True if signature verification succeeded
};

/// @brief Plugin metadata including identification, security, and build information.
struct PluginMetadata {
    std::string name;                          ///< Unique plugin name identifier
    std::string version;                       ///< Semantic version string (e.g., "1.0.0")
    std::string author;                        ///< Plugin author or organization
    std::string description;                   ///< Human-readable description
    std::string license;                       ///< License identifier (SPDX format)
    
    /// @brief Security-related metadata
    PluginSignature signature;                 ///< Signature and certificate information
    std::vector<std::string> requiredCapabilities;  ///< Minimum required capabilities
    std::vector<std::string> permissions;      ///< Requested permissions (e.g., "gpu_access", "network", "filesystem")
    
    /// @brief Build and compilation metadata
    std::string buildDate;                     ///< Build timestamp (ISO 8601 format)
    std::string buildCommit;                   ///< Git commit hash or identifier
    std::string compilerVersion;               ///< Compiler version used for compilation
};

/// @brief Trust levels for plugin classification.
enum class PluginTrustLevel {
    TRUSTED,        ///< Signed by a trusted certificate issuer
    UNTRUSTED,      ///< Not signed or signature from unknown issuer
    BLOCKED         ///< On the security blacklist, should not be loaded
};

/// @brief Security policy configuration for plugin verification and loading.
struct PluginSecurityPolicy {
    /// @brief Require all plugins to have valid digital signatures
    bool requireSignature = true;
    
    /// @brief List of trusted certificate issuers (Distinguished Names)
    std::vector<std::string> trustedIssuers = {
        "CN=ThemisDB Official Plugins, O=ThemisDB, C=DE"
    };
    
    /// @brief Allow loading unsigned plugins (only for development/testing)
    bool allowUnsigned = false;
    
    /// @brief Verify file hash before plugin loading to detect tampering
    bool verifyFileHash = true;
    
    /// @brief Check certificate revocation status (CRL/OCSP lookups)
    bool checkRevocation = true;
    
    /// @brief Network timeout in seconds for CRL/OCSP revocation checks (default: 5)
    int revocation_timeout_seconds = 5;
    
    /// @brief Minimum trust level required for plugin execution
    PluginTrustLevel minTrustLevel = PluginTrustLevel::TRUSTED;
    
    /// @brief SHA-256 hashes of known malicious plugins (blacklist)
    std::vector<std::string> blacklistedHashes;
    
    /// @brief SHA-256 hashes of explicitly allowed plugins (whitelist)
    std::vector<std::string> whitelistedHashes;
};

/// @brief Primary plugin security verification and signature validation.
///
/// This class handles all cryptographic security checks for plugin verification including
/// hash computation, digital signature verification, certificate chain validation, and
/// revocation checking (CRL/OCSP). It enforces the security policy configured at construction.
class PluginSecurityVerifier {
public:
    /**
     * @brief @brief Constructor @param policy Security policy rules to enforce during verification
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    explicit PluginSecurityVerifier(const PluginSecurityPolicy& policy);
    ~PluginSecurityVerifier() = default;
    
    /**
     * @brief @brief Verify a plugin before loading @param pluginPath Path to the plugin DLL/SO file @param errorMessage Output parameter containing detailed error message on failure @return true if plugin passes all security checks and is safe to load; false otherwise
     * @param[in] pluginPath Input parameter.
     * @param[in,out] errorMessage Input/output parameter.
     * @return True on success.
     */
    bool verifyPlugin(const std::string& pluginPath, std::string& errorMessage);
    
    /**
     * @brief @brief Calculate SHA-256 hash of plugin file @param filePath Path to the file to hash @return Hex-encoded SHA-256 hash string (64 characters)
     * @param[in] filePath Input parameter.
     * @return Return value.
     */
    std::string calculateFileHash(const std::string& filePath);
    
    /**
     * @brief @brief Verify digital signature on the plugin @param filePath Path to the plugin file @param signature Signature data to verify against @return true if signature is valid; false if invalid, corrupted, or verification failed
     * @param[in] filePath Input parameter.
     * @param[in] signature Input parameter.
     * @return True on success.
     */
    bool verifySignature(const std::string& filePath, const PluginSignature& signature);
    
    /**
     * @brief @brief Parse plugin metadata from JSON sidecar file @param pluginPath Path to the plugin; metadata file must be at <pluginPath>.
     * @param[in] pluginPath Input parameter.
     * @return Return value.
     * @details metadata.json @return PluginMetadata if file exists and is valid JSON; std::nullopt otherwise
     */
    std::optional<PluginMetadata> loadMetadata(const std::string& pluginPath);
    
    /**
     * @brief @brief Verify complete certificate chain @param certificate PEM-encoded certificate string @return true if chain is valid and properly issued; false if broken or invalid
     * @param[in] certificate Input parameter.
     * @return True on success.
     */
    bool verifyCertificateChain(const std::string& certificate);
    
    /**
     * @brief @brief Check if plugin hash is on security blacklist @param fileHash Hex-encoded SHA-256 hash string @return true if hash matches a blacklisted entry; false otherwise
     * @param[in] fileHash Input parameter.
     * @return True on success.
     */
    bool isBlacklisted(const std::string& fileHash) const;
    
    /**
     * @brief @brief Check if plugin hash is on whitelist (explicitly allowed) @param fileHash Hex-encoded SHA-256 hash string @return true if hash matches a whitelisted entry; false otherwise
     * @param[in] fileHash Input parameter.
     * @return True on success.
     */
    bool isWhitelisted(const std::string& fileHash) const;
    
    /**
     * @brief @brief Determine trust level for a plugin @param metadata Plugin metadata to evaluate @return Trust level (TRUSTED, UNTRUSTED, or BLOCKED)
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    PluginTrustLevel getTrustLevel(const PluginMetadata& metadata);
    
    /**
     * @brief @brief Update security policy at runtime @param policy New security policy to apply @note This is thread-safe; policy changes take effect immediately
     * @param[in] policy Input parameter.
     */
    void updatePolicy(const PluginSecurityPolicy& policy);
    
    /// @brief Get current policy
    /// @return Reference to the currently active security policy
    const PluginSecurityPolicy& getPolicy() const { return policy_; }
    
    /**
     * @brief @brief Check certificate revocation list (CRL) @param certificate PEM-encoded certificate string @return true if certificate is not revoked or CRL check passed; false if revoked @note Public for white-box testing; normally called internally by verifyCertificateChain
     * @param[in] certificate Input parameter.
     * @return True on success.
     */
    bool checkCRL(const std::string& certificate);
    
    /**
     * @brief @brief Check Online Certificate Status Protocol (OCSP) @param certificate PEM-encoded certificate string @return true if certificate status is valid; false if revoked or status unknown @note Public for white-box testing; normally called internally by verifyCertificateChain
     * @param[in] certificate Input parameter.
     * @return True on success.
     */
    bool checkOCSP(const std::string& certificate);
    
    /**
     * @brief @brief Validate plugin path to prevent path traversal attacks @param path Path to validate @param errorMessage Output parameter containing error details on failure @return true if path is safe (no ".
     * @param[in] path Input parameter.
     * @param[in,out] errorMessage Input/output parameter.
     * @return True on success.
     * @details ." components, absolute or relative within allowed directory); false if path contains traversal sequences or disallowed patterns
     */
    static bool validatePluginPath(const std::string& path, std::string& errorMessage);
    
private:
    PluginSecurityPolicy policy_;
};

// ============================================================================
// Enhanced Plugin Security with Embedded Signatures
// ============================================================================

/**
 * @brief Enhanced plugin security verifier with multi-level verification
 * 
 * This class extends the basic PluginSecurityVerifier with support for:
 * - Embedded manufacturer certificates (in DLL/SO)
 * - Platform-native code signing (Authenticode, codesign, GPG)
 * - Multi-level verification (4 levels from hash-only to full chain)
 * - Certificate chain validation
 */
class EnhancedPluginSecurityVerifier {
public:
    /**
     * @brief Verification level defines how thorough the plugin check should be
     */
    enum class VerificationLevel {
        LEVEL_1_HASH_ONLY,           ///< Only SHA-256 hash (fast)
        LEVEL_2_EMBEDDED_SIGNATURE,  ///< Embedded signature check
        LEVEL_3_PLATFORM_SIGNATURE,  ///< Platform code-signing (PE/ELF/Mach-O)
        LEVEL_4_FULL_CHAIN           ///< Complete cert chain + CRL/OCSP
    };
    
    /**
     * @brief Detailed verification result
     */
    struct VerificationResult {
        bool passed = false;
        VerificationLevel level_achieved = VerificationLevel::LEVEL_1_HASH_ONLY;
        std::string error_message;
        
        // Individual check results
        bool hash_verified = false;
        bool embedded_signature_verified = false;
        bool platform_signature_verified = false;
        bool certificate_chain_verified = false;
        bool certificate_not_revoked = false;
        
        // Certificate information
        std::string issuer;
        std::string subject;
        std::chrono::system_clock::time_point valid_from;
        std::chrono::system_clock::time_point valid_until;
        bool is_themisdb_official = false;
    };
    
    /**
     * @brief @brief Constructor @param policy Security policy rules to enforce during verification
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    explicit EnhancedPluginSecurityVerifier(const PluginSecurityPolicy& policy);
    ~EnhancedPluginSecurityVerifier() = default;
    
    /// @brief Verify plugin with multi-level checks
    /// @param plugin_path Path to DLL/SO file
    /// @param required_level Minimum verification level required (defaults to LEVEL_3)
    /// @return Comprehensive verification result with all check details
    VerificationResult verifyPlugin(
        const std::string& plugin_path,
        VerificationLevel required_level = VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    /**
     * @brief @brief Update security policy at runtime @param policy New security policy to apply
     * @param[in] policy Input parameter.
     */
    void updatePolicy(const PluginSecurityPolicy& policy);
    
    /// @brief Get current policy
    /// @return Reference to the currently active security policy
    const PluginSecurityPolicy& getPolicy() const { return policy_; }
    
private:
    PluginSecurityPolicy policy_;
    
    /**
     * @brief Level 1: Hash verification (from base class)
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyHash(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Level 2: Embedded signature verification
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyEmbeddedSignature(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Level 3: Platform-specific code signing
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyPlatformSignature(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Level 4: Full certificate chain + revocation
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyFullChain(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Extract embedded certificate from DLL/SO
     * @param[in] plugin_path Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<uint8_t>> extractEmbeddedCertificate(
        const std::string& plugin_path
    );
    
    /**
     * @brief Extract embedded signature from DLL/SO
     * @param[in] plugin_path Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<uint8_t>> extractEmbeddedSignature(
        const std::string& plugin_path
    );
    
    /**
     * @brief Verify ThemisDB.
     * @param[in,out] cert Input/output parameter.
     * @return True on success.
     * @details org official certificate
     */
    bool isOfficialThemisDBCertificate(X509* cert);
    
    // Platform-specific signature verification
#ifdef _WIN32
    /**
     * @brief TBD: Describe verifyAuthenticodeSignature.
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyAuthenticodeSignature(const std::string& plugin_path, VerificationResult& result);
#elif defined(__APPLE__)
    /**
     * @brief TBD: Describe verifyMacOSCodeSignature.
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyMacOSCodeSignature(const std::string& plugin_path, VerificationResult& result);
#else
    /**
     * @brief TBD: Describe verifyGPGSignature.
     * @param[in] plugin_path Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True on success.
     */
    bool verifyGPGSignature(const std::string& plugin_path, VerificationResult& result);
#endif
    
    /**
     * @brief Helper: Calculate hash excluding signature section
     * @param[in] plugin_path Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> calculateHashExcludingSignature(const std::string& plugin_path);
    
    /**
     * @brief Helper: Verify RSA signature
     * @param[in] data Input parameter.
     * @param[in] signature Input parameter.
     * @param[in,out] pubkey Input/output parameter.
     * @return True on success.
     */
    bool verifyRSASignature(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        EVP_PKEY* pubkey
    );
    
    /**
     * @brief Helper: Get certificate issuer DN
     * @param[in,out] cert Input/output parameter.
     * @return Return value.
     */
    std::string getCertificateIssuer(X509* cert);
    
    /**
     * @brief Helper: Get certificate subject DN
     * @param[in,out] cert Input/output parameter.
     * @return Return value.
     */
    std::string getCertificateSubject(X509* cert);
    
    /**
     * @brief Helper: Check if certificate is currently valid
     * @param[in,out] cert Input/output parameter.
     * @return True on success.
     */
    bool isCertificateValid(X509* cert);
    
    /**
     * @brief Helper: Load plugin metadata for chain validation
     * @param[in] plugin_path Input parameter.
     * @return Return value.
     */
    std::optional<PluginMetadata> loadPluginMetadataForChainValidation(
        const std::string& plugin_path
    );

public:
    /**
     * @brief Exposes extractEmbeddedCertificate() for white-box unit testing only.
     * @param[in] plugin_path Input parameter.
     * @return Return value.
     * @details Calls: extractEmbeddedCertificate().
     */
    std::optional<std::vector<uint8_t>> extractSigningCertificateForTesting(
        const std::string& plugin_path) {
        return extractEmbeddedCertificate(plugin_path);
    }
};

/// @brief Security audit event log entry for plugin operations.
struct PluginSecurityEvent {
    /// @brief Type of security event that occurred
    enum class EventType {
        PLUGIN_LOADED,                  ///< Plugin successfully loaded
        PLUGIN_LOAD_FAILED,            ///< Plugin load operation failed
        SIGNATURE_VERIFIED,            ///< Digital signature verification succeeded
        SIGNATURE_VERIFICATION_FAILED, ///< Signature verification failed
        HASH_MISMATCH,                 ///< File hash does not match expected value
        BLACKLISTED,                   ///< Plugin found on security blacklist
        UNTRUSTED_ISSUER,              ///< Certificate issuer not in trusted list
        CERTIFICATE_EXPIRED,           ///< Plugin certificate has expired
        CERTIFICATE_REVOKED,           ///< Plugin certificate is revoked
        POLICY_VIOLATION,              ///< Security policy constraint violated
        PLUGIN_UNLOADED                ///< Plugin successfully unloaded
    };
    
    EventType type;                     ///< Type of security event
    std::string pluginPath;             ///< Path to the plugin file
    std::string pluginHash;             ///< SHA-256 hash of the plugin
    std::string message;                ///< Human-readable event description
    uint64_t timestamp;                 ///< Unix timestamp of event occurrence
    std::string severity;               ///< Event severity level: "INFO", "WARNING", "ERROR", "CRITICAL"
};

/// @brief Thread-safe audit logger for plugin security events.
///
/// Singleton class that collects and persists security events for compliance, audit trails,
/// and security incident investigation. All operations are thread-safe.
class PluginSecurityAuditor {
public:
    /**
     * @brief @brief Get the singleton instance @return Reference to the global auditor instance
     * @return Return value.
     */
    static PluginSecurityAuditor& instance();
    
    /**
     * @brief @brief Log a security event (thread-safe) @param event Security event to log
     * @param[in] event Input parameter.
     */
    void logEvent(const PluginSecurityEvent& event);
    
    /**
     * @brief @brief Get all security events for a specific plugin (thread-safe) @param pluginPath Plugin path to filter events by @return Copy of all events matching the plugin path
     * @param[in] pluginPath Input parameter.
     * @return Return value.
     */
    std::vector<PluginSecurityEvent> getEventsForPlugin(const std::string& pluginPath) const;
    
    /**
     * @brief @brief Get a snapshot of all security events (thread-safe) @return Copy of the complete event log
     * @return Return value.
     */
    std::vector<PluginSecurityEvent> getAllEvents() const;
    
    /// @brief Clear all event log entries (thread-safe)
    void clearEvents();
    
    /**
     * @brief @brief Export events to file for compliance and audit (thread-safe) @param outputPath Path where to write event log file (JSON format recommended) @return true if export succeeded; false if file write failed
     * @param[in] outputPath Input parameter.
     * @return True on success.
     */
    bool exportEvents(const std::string& outputPath) const;
    
private:
    PluginSecurityAuditor() = default;
    ~PluginSecurityAuditor() = default;
    PluginSecurityAuditor(const PluginSecurityAuditor&) = delete;
    PluginSecurityAuditor& operator=(const PluginSecurityAuditor&) = delete;
    
    mutable std::mutex mutex_;
    std::vector<PluginSecurityEvent> events_;
};

} // namespace acceleration
} // namespace themis
