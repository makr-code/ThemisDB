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

struct PluginSignature {
    std::string sha256Hash;          ///< SHA-256 hash of the DLL/SO binary file
    std::string signature;           ///< Digital signature (RSA or ECDSA)
    std::string signingCertificate;  ///< X.509 certificate of the signer
    std::string issuer;              ///< Certificate issuer Distinguished Name
    std::string subject;             ///< Certificate subject Distinguished Name
    uint64_t timestamp = 0;          ///< Unix timestamp when signature was created
    bool verified = false;           ///< True if signature verification succeeded
};

struct PluginMetadata {
    std::string name;                          ///< Unique plugin name identifier
    std::string version;                       ///< Semantic version string (e.g., "1.0.0")
    std::string author;                        ///< Plugin author or organization
    std::string description;                   ///< Human-readable description
    std::string license;                       ///< License identifier (SPDX format)
    
    PluginSignature signature;                 ///< Signature and certificate information
    std::vector<std::string> requiredCapabilities;  ///< Minimum required capabilities
    std::vector<std::string> permissions;      ///< Requested permissions (e.g., "gpu_access", "network", "filesystem")
    
    std::string buildDate;                     ///< Build timestamp (ISO 8601 format)
    std::string buildCommit;                   ///< Git commit hash or identifier
    std::string compilerVersion;               ///< Compiler version used for compilation
};

enum class PluginTrustLevel {
    TRUSTED,        ///< Signed by a pinned signer certificate
    UNTRUSTED,      ///< Not signed or signature from unknown issuer
    BLOCKED         ///< On the security blacklist, should not be loaded
};

struct PluginSecurityPolicy {
    /// Signature verification is mandatory; setting this false makes verification fail.
    bool requireSignature = true;

    /// Issuer names are descriptive only; signer authorization requires fingerprint pins.
    std::vector<std::string> trustedIssuers = {
        "CN=ThemisDB Official Plugins, O=ThemisDB, C=DE"
    };
    
    /// Unsigned plugins are never accepted; this flag cannot override mandatory verification.
    bool allowUnsigned = false;
    
    bool verifyFileHash = true;
    
    bool checkRevocation = true;
    
    int revocation_timeout_seconds = 5;
    
    PluginTrustLevel minTrustLevel = PluginTrustLevel::TRUSTED;
    
    std::vector<std::string> blacklistedHashes;
    
    std::vector<std::string> whitelistedHashes;

    /// SHA-256 fingerprints of trusted signer certificates, in lowercase hex.
    /// A signed plugin is rejected unless its embedded signing certificate is
    /// pinned here, even when the certificate signature itself is valid.
    std::vector<std::string> trustedSignerFingerprints;
};

/**
 * @brief Normalize and validate signer certificate fingerprints.
 * @param fingerprints SHA-256 fingerprints in hexadecimal form.
 * @return Unique lowercase fingerprints sorted for deterministic comparison.
 * @throws std::invalid_argument if a fingerprint is not 64 hexadecimal characters.
 */
std::vector<std::string> normalizePluginSignerFingerprints(
    std::vector<std::string> fingerprints);

/**
 * @brief Read signer pins configured for plugin loading.
 * @return Pins from THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS, or an empty list.
 * @throws std::invalid_argument if the variable contains an empty or malformed entry.
 */
std::vector<std::string> pluginSignerFingerprintsFromEnvironment();

class PluginSecurityVerifier {
public:
    /**
     * @brief Create a verifier with mandatory signer identity pins.
     * @param[in] policy Policy supplying the authorized signer fingerprints.
     * @throws std::invalid_argument if a fingerprint is malformed.
     */
    explicit PluginSecurityVerifier(const PluginSecurityPolicy& policy);
    ~PluginSecurityVerifier() = default;
    
    /**
     * @brief Verify a signed plugin against an authorized signer fingerprint.
     * @param[in] pluginPath Path to the plugin binary.
     * @param[out] errorMessage Redacted reason when verification fails.
     * @return True only when the signature and pinned signer identity are valid.
     * @note Missing signatures, unpinned signers, and disabled signature policy fail.
     */
    bool verifyPlugin(const std::string& pluginPath, std::string& errorMessage);
    
    /**
     * @brief Calculate File Hash.
     * @param[in] filePath Input parameter.
     * @return Return value.
     */
    std::string calculateFileHash(const std::string& filePath);
    
    /**
     * @brief Verify the cryptographic signature over a plugin binary hash.
     * @param[in] filePath Path to the plugin binary.
     * @param[in] signature Signature and signer certificate metadata.
     * @return True when the cryptographic signature is valid.
     * @note This operation alone does not authorize an unpinned signer.
     */
    bool verifySignature(const std::string& filePath, const PluginSignature& signature);
    
    /**
     * @brief Load Metadata.
     * @param[in] pluginPath Input parameter.
     * @return Return value.
     */
    std::optional<PluginMetadata> loadMetadata(const std::string& pluginPath);
    
    /**
     * @brief Verify Certificate Chain.
     * @param[in] certificate Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyCertificateChain(const std::string& certificate);
    
    /**
     * @brief Is Blacklisted.
     * @param[in] fileHash Input parameter.
     * @return True when the operation succeeds.
     */
    bool isBlacklisted(const std::string& fileHash) const;
    
    /**
     * @brief Is Whitelisted.
     * @param[in] fileHash Input parameter.
     * @return True when the operation succeeds.
     */
    bool isWhitelisted(const std::string& fileHash) const;
    
    /**
     * @brief Get Trust Level.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    PluginTrustLevel getTrustLevel(const PluginMetadata& metadata);
    
    /**
     * @brief Update Policy.
     * @param[in] policy Input parameter.
     */
    void updatePolicy(const PluginSecurityPolicy& policy);
    
    const PluginSecurityPolicy& getPolicy() const { return policy_; }
    
    /**
     * @brief Check CRL.
     * @param[in] certificate Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkCRL(const std::string& certificate);
    
    /**
     * @brief Check OCSP.
     * @param[in] certificate Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkOCSP(const std::string& certificate);
    
    /**
     * @brief Validate Plugin Path.
     * @param[in] path Input parameter.
     * @param[in,out] errorMessage Input/output parameter.
     * @return True when the operation succeeds.
     */
    static bool validatePluginPath(const std::string& path, std::string& errorMessage);
    
private:
    PluginSecurityPolicy policy_;
};

// ============================================================================
// Enhanced Plugin Security with Embedded Signatures
// ============================================================================

class EnhancedPluginSecurityVerifier {
public:
    enum class VerificationLevel {
        LEVEL_1_HASH_ONLY,           ///< Hash only; rejected as insufficient plugin authorization
        LEVEL_2_EMBEDDED_SIGNATURE,  ///< Embedded signature check
        LEVEL_3_PLATFORM_SIGNATURE,  ///< Platform code-signing (PE/ELF/Mach-O)
        LEVEL_4_FULL_CHAIN           ///< Complete cert chain + CRL/OCSP
    };
    
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
     * @brief Create a verifier with mandatory signer identity pins.
     * @param[in] policy Policy supplying the authorized signer fingerprints.
     * @throws std::invalid_argument if a fingerprint is malformed.
     */
    explicit EnhancedPluginSecurityVerifier(const PluginSecurityPolicy& policy);
    ~EnhancedPluginSecurityVerifier() = default;

    /**
     * @brief Verify a plugin with mandatory signature and pinned-signer checks.
     * @param plugin_path Path to the plugin binary.
     * @param required_level Minimum verification level; hash-only is insufficient
     *        and returns a failed result.
     * @return Verification results. Missing signatures, unpinned signers, and
     *         disabled signature policy return `passed == false`.
     */
    VerificationResult verifyPlugin(
        const std::string& plugin_path,
        VerificationLevel required_level = VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    /**
     * @brief Replace the verification policy and normalize its signer pins.
     * @param[in] policy Policy supplying the authorized signer fingerprints.
     * @throws std::invalid_argument if a fingerprint is malformed.
     */
    void updatePolicy(const PluginSecurityPolicy& policy);
    
    const PluginSecurityPolicy& getPolicy() const { return policy_; }
    
private:
    PluginSecurityPolicy policy_;
    
    /**
     * @brief Verify Hash.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyHash(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Verify Embedded Signature.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyEmbeddedSignature(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Verify Platform Signature.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyPlatformSignature(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Verify Full Chain.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyFullChain(const std::string& plugin_path, VerificationResult& result);
    
    /**
     * @brief Extract Embedded Certificate.
     * @param[in] plugin_path Path to the plugin.
     * @return Return value.
     */
    std::optional<std::vector<uint8_t>> extractEmbeddedCertificate(
        const std::string& plugin_path
    );
    
    /**
     * @brief Extract Embedded Signature.
     * @param[in] plugin_path Path to the plugin.
     * @return Return value.
     */
    std::optional<std::vector<uint8_t>> extractEmbeddedSignature(
        const std::string& plugin_path
    );
    
    /**
     * @brief Is Official Themis DBCertificate.
     * @param[in,out] cert Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool isOfficialThemisDBCertificate(X509* cert);
    
    // Platform-specific signature verification
#ifdef _WIN32
    /**
     * @brief Verify Authenticode Signature.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyAuthenticodeSignature(const std::string& plugin_path, VerificationResult& result);
#elif defined(__APPLE__)
    /**
     * @brief Verify Mac OSCode Signature.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyMacOSCodeSignature(const std::string& plugin_path, VerificationResult& result);
#else
    /**
     * @brief Verify GPGSignature.
     * @param[in] plugin_path Path to the plugin.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyGPGSignature(const std::string& plugin_path, VerificationResult& result);
#endif
    
    /**
     * @brief Calculate Hash Excluding Signature.
     * @param[in] plugin_path Path to the plugin.
     * @return Return value.
     */
    std::vector<uint8_t> calculateHashExcludingSignature(const std::string& plugin_path);
    
    /**
     * @brief Verify RSASignature.
     * @param[in] data Input parameter.
     * @param[in] signature Input parameter.
     * @param[in,out] pubkey Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyRSASignature(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        EVP_PKEY* pubkey
    );
    
    /**
     * @brief Get Certificate Issuer.
     * @param[in,out] cert Input/output parameter.
     * @return Return value.
     */
    std::string getCertificateIssuer(X509* cert);
    
    /**
     * @brief Get Certificate Subject.
     * @param[in,out] cert Input/output parameter.
     * @return Return value.
     */
    std::string getCertificateSubject(X509* cert);
    
    /**
     * @brief Is Certificate Valid.
     * @param[in,out] cert Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool isCertificateValid(X509* cert);
    
    /**
     * @brief Load Plugin Metadata For Chain Validation.
     * @param[in] plugin_path Path to the plugin.
     * @return Return value.
     */
    std::optional<PluginMetadata> loadPluginMetadataForChainValidation(
        const std::string& plugin_path
    );

public:
    /**
     * @brief Extract Signing Certificate For Testing.
     * @param[in] plugin_path Path to the plugin.
     * @return Return value.
     * @details Calls: extractEmbeddedCertificate().
     */
    std::optional<std::vector<uint8_t>> extractSigningCertificateForTesting(
        const std::string& plugin_path) {
        return extractEmbeddedCertificate(plugin_path);
    }
};

struct PluginSecurityEvent {
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

class PluginSecurityAuditor {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static PluginSecurityAuditor& instance();
    
    /**
     * @brief Log Event.
     * @param[in] event Input parameter.
     */
    void logEvent(const PluginSecurityEvent& event);
    
    /**
     * @brief Get Events For Plugin.
     * @param[in] pluginPath Input parameter.
     * @return Return value.
     */
    std::vector<PluginSecurityEvent> getEventsForPlugin(const std::string& pluginPath) const;
    
    /**
     * @brief Get All Events.
     * @return Return value.
     */
    std::vector<PluginSecurityEvent> getAllEvents() const;
    
    /**
     * @brief Clear Events.
     */
    void clearEvents();
    
    /**
     * @brief Export Events.
     * @param[in] outputPath Input parameter.
     * @return True when the operation succeeds.
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
