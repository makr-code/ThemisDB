/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_security.h                                  ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     351                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <chrono>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/err.h>

namespace themis {
namespace acceleration {

// Plugin signature verification
struct PluginSignature {
    std::string sha256Hash;          // SHA-256 Hash der DLL/SO Datei
    std::string signature;           // Digitale Signatur (RSA/ECDSA)
    std::string signingCertificate;  // X.509 Zertifikat des Signierers
    std::string issuer;              // Zertifikat-Aussteller
    std::string subject;             // Zertifikat-Inhaber
    uint64_t timestamp = 0;          // Unix-Timestamp der Signatur
    bool verified = false;
};

// Plugin metadata and security info
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string license;
    
    // Security
    PluginSignature signature;
    std::vector<std::string> requiredCapabilities;
    std::vector<std::string> permissions;  // z.B. "gpu_access", "network", "filesystem"
    
    // Build info
    std::string buildDate;
    std::string buildCommit;
    std::string compilerVersion;
};

// Plugin security policy
enum class PluginTrustLevel {
    TRUSTED,        // Signiert von vertrauenswürdigem Zertifikat
    UNTRUSTED,      // Nicht signiert oder unbekannter Signatur
    BLOCKED         // Auf Blacklist
};

struct PluginSecurityPolicy {
    // Require signature verification
    bool requireSignature = true;
    
    // Require specific certificate issuer
    std::vector<std::string> trustedIssuers = {
        "CN=ThemisDB Official Plugins, O=ThemisDB, C=DE"
    };
    
    // Allow unsigned plugins (for development)
    bool allowUnsigned = false;
    
    // Verify file hash before loading
    bool verifyFileHash = true;
    
    // Check certificate revocation (CRL/OCSP)
    bool checkRevocation = true;
    
    // Minimum trust level required
    PluginTrustLevel minTrustLevel = PluginTrustLevel::TRUSTED;
    
    // Blacklist of plugin hashes (known malicious)
    std::vector<std::string> blacklistedHashes;
    
    // Whitelist of plugin hashes (explicitly allowed)
    std::vector<std::string> whitelistedHashes;
};

// Plugin security verifier
class PluginSecurityVerifier {
public:
    explicit PluginSecurityVerifier(const PluginSecurityPolicy& policy);
    ~PluginSecurityVerifier() = default;
    
    // Verify a plugin before loading
    // Returns true if plugin is safe to load, false otherwise
    bool verifyPlugin(const std::string& pluginPath, std::string& errorMessage);
    
    // Calculate SHA-256 hash of plugin file
    std::string calculateFileHash(const std::string& filePath);
    
    // Verify digital signature
    bool verifySignature(const std::string& filePath, const PluginSignature& signature);
    
    // Parse plugin metadata from JSON sidecar file
    std::optional<PluginMetadata> loadMetadata(const std::string& pluginPath);
    
    // Verify certificate chain
    bool verifyCertificateChain(const std::string& certificate);
    
    // Check if plugin is on blacklist
    bool isBlacklisted(const std::string& fileHash) const;
    
    // Check if plugin is on whitelist
    bool isWhitelisted(const std::string& fileHash) const;
    
    // Get trust level for plugin
    PluginTrustLevel getTrustLevel(const PluginMetadata& metadata);
    
    // Update security policy at runtime
    void updatePolicy(const PluginSecurityPolicy& policy);
    
    // Get current policy
    const PluginSecurityPolicy& getPolicy() const { return policy_; }
    
    // Check certificate revocation list (public for testing)
    bool checkCRL(const std::string& certificate);
    
    // OCSP (Online Certificate Status Protocol) check (public for testing)
    bool checkOCSP(const std::string& certificate);
    
private:
    PluginSecurityPolicy policy_;
    
    // OpenSSL integration for signature verification
    bool verifyRSASignature(const std::vector<uint8_t>& data, 
                           const std::vector<uint8_t>& signature,
                           const std::string& publicKey);
    
    bool verifyECDSASignature(const std::vector<uint8_t>& data,
                             const std::vector<uint8_t>& signature, 
                             const std::string& publicKey);
    
    // Load and verify X.509 certificate
    bool loadCertificate(const std::string& certPEM);
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
    
    explicit EnhancedPluginSecurityVerifier(const PluginSecurityPolicy& policy);
    ~EnhancedPluginSecurityVerifier() = default;
    
    /**
     * @brief Verify plugin with multi-level checks
     * @param plugin_path Path to DLL/SO file
     * @param required_level Minimum verification level required
     * @return Verification result with detailed information
     */
    VerificationResult verifyPlugin(
        const std::string& plugin_path,
        VerificationLevel required_level = VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    /**
     * @brief Update security policy at runtime
     */
    void updatePolicy(const PluginSecurityPolicy& policy);
    
    /**
     * @brief Get current policy
     */
    const PluginSecurityPolicy& getPolicy() const { return policy_; }
    
private:
    PluginSecurityPolicy policy_;
    
    // Level 1: Hash verification (from base class)
    bool verifyHash(const std::string& plugin_path, VerificationResult& result);
    
    // Level 2: Embedded signature verification
    bool verifyEmbeddedSignature(const std::string& plugin_path, VerificationResult& result);
    
    // Level 3: Platform-specific code signing
    bool verifyPlatformSignature(const std::string& plugin_path, VerificationResult& result);
    
    // Level 4: Full certificate chain + revocation
    bool verifyFullChain(const std::string& plugin_path, VerificationResult& result);
    
    // Extract embedded certificate from DLL/SO
    std::optional<std::vector<uint8_t>> extractEmbeddedCertificate(
        const std::string& plugin_path
    );
    
    // Extract embedded signature from DLL/SO
    std::optional<std::vector<uint8_t>> extractEmbeddedSignature(
        const std::string& plugin_path
    );
    
    // Verify ThemisDB.org official certificate
    bool isOfficialThemisDBCertificate(X509* cert);
    
    // Platform-specific signature verification
#ifdef _WIN32
    bool verifyAuthenticodeSignature(const std::string& plugin_path, VerificationResult& result);
#elif defined(__APPLE__)
    bool verifyMacOSCodeSignature(const std::string& plugin_path, VerificationResult& result);
#else
    bool verifyGPGSignature(const std::string& plugin_path, VerificationResult& result);
#endif
    
    // Helper: Calculate hash excluding signature section
    std::vector<uint8_t> calculateHashExcludingSignature(const std::string& plugin_path);
    
    // Helper: Verify RSA signature
    bool verifyRSASignature(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        EVP_PKEY* pubkey
    );
    
    // Helper: Get certificate issuer DN
    std::string getCertificateIssuer(X509* cert);
    
    // Helper: Get certificate subject DN
    std::string getCertificateSubject(X509* cert);
    
    // Helper: Check if certificate is currently valid
    bool isCertificateValid(X509* cert);
    
    // Helper: Load plugin metadata for chain validation
    std::optional<PluginMetadata> loadPluginMetadataForChainValidation(
        const std::string& plugin_path
    );
};

// Audit logging for plugin security events
struct PluginSecurityEvent {
    enum class EventType {
        PLUGIN_LOADED,
        PLUGIN_LOAD_FAILED,
        SIGNATURE_VERIFIED,
        SIGNATURE_VERIFICATION_FAILED,
        HASH_MISMATCH,
        BLACKLISTED,
        UNTRUSTED_ISSUER,
        CERTIFICATE_EXPIRED,
        CERTIFICATE_REVOKED,
        POLICY_VIOLATION
    };
    
    EventType type;
    std::string pluginPath;
    std::string pluginHash;
    std::string message;
    uint64_t timestamp;
    std::string severity;  // INFO, WARNING, ERROR, CRITICAL
};

class PluginSecurityAuditor {
public:
    static PluginSecurityAuditor& instance();
    
    // Log security event
    void logEvent(const PluginSecurityEvent& event);
    
    // Get security events for a specific plugin
    std::vector<PluginSecurityEvent> getEventsForPlugin(const std::string& pluginPath) const;
    
    // Get all security events
    const std::vector<PluginSecurityEvent>& getAllEvents() const { return events_; }
    
    // Clear event log
    void clearEvents();
    
    // Export events to file (for compliance/audit)
    bool exportEvents(const std::string& outputPath) const;
    
private:
    PluginSecurityAuditor() = default;
    ~PluginSecurityAuditor() = default;
    PluginSecurityAuditor(const PluginSecurityAuditor&) = delete;
    PluginSecurityAuditor& operator=(const PluginSecurityAuditor&) = delete;
    
    std::vector<PluginSecurityEvent> events_;
};

} // namespace acceleration
} // namespace themis
