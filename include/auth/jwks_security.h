/**
 * @file jwks_security.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/secure_memory.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace themis {
namespace auth {

/**
 * @brief JWKS Transport Security Configuration
 * 
 * Security Feature: Enhances JWKS fetching with certificate pinning and mTLS.
 * Prevents MITM attacks and ensures trusted JWKS sources.
 * 
 * Features:
 * - Certificate pinning (pin public key or certificate)
 * - mTLS (mutual TLS) support for client authentication
 * - TLS version enforcement (minimum TLS 1.2)
 * - Hostname verification
 * - Certificate validation options
 * 
 * Certificate Pinning Methods:
 * - Public Key Pinning: pin SPKI hash (RFC 7469)
 * - Certificate Pinning: pin entire certificate
 * - CA Certificate Pinning: pin CA cert for verification
 * 
 * mTLS Support:
 * - Client certificate for authentication
 * - Private key for signing
 * - CA bundle for server verification
 * 
 * P1 (High Priority) security hardening feature.
 */
class JWKSSecurityConfig {
public:
    enum class PinningMode {
        NONE,                   // No pinning (default)
        PUBLIC_KEY,            // Pin SPKI hash (recommended)
        CERTIFICATE,           // Pin entire certificate
        CA_CERTIFICATE         // Pin CA certificate
    };
    
    enum class TLSVersion {
        TLS_1_0,
        TLS_1_1,
        TLS_1_2,
        TLS_1_3
    };
    
    struct Config {
        // Certificate Pinning
        PinningMode pinning_mode = PinningMode::NONE;
        std::vector<std::string> pinned_hashes;     // SHA256 hashes (base64)
        std::string pinned_cert_path;               // Path to pinned certificate
        
        // mTLS (Mutual TLS)
        bool enable_mtls = false;
        std::string client_cert_path;               // Client certificate
        std::string client_key_path;                // Client private key
        SecureString client_key_password;           // Key password (optional, zeroed on destruction)
        
        // TLS Configuration
        TLSVersion min_tls_version = TLSVersion::TLS_1_2;
        bool verify_hostname = true;
        bool verify_certificate = true;
        std::string ca_bundle_path;                 // Custom CA bundle
        
        // Cipher suites (optional, empty = system default)
        std::vector<std::string> allowed_ciphers;
        
        // Timeout
        int connect_timeout_ms = 5000;
        int read_timeout_ms = 5000;
    };
    
    explicit JWKSSecurityConfig(const Config& config);
    
    /**
     * @brief Get the configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Validate configuration
     * 
     * Checks that paths exist, certificates are valid, etc.
     * 
     * @throws std::runtime_error if configuration is invalid
     */
    void validate() const;
    
    /**
     * @brief Create a config with public key pinning
     * 
     * Recommended approach: pins SPKI (Subject Public Key Info) hash.
     * More flexible than cert pinning (survives cert rotation).
     * 
     * @param spki_hashes SHA256 hashes of SPKI (base64 encoded)
     * @return Config Configuration with pinning enabled
     */
    static Config withPublicKeyPinning(const std::vector<std::string>& spki_hashes);
    
    /**
     * @brief Create a config with certificate pinning
     * 
     * Pins entire certificate. Must update pins when cert rotates.
     * 
     * @param cert_path Path to certificate file
     * @return Config Configuration with cert pinning
     */
    static Config withCertificatePinning(const std::string& cert_path);
    
    /**
     * @brief Create a config with mTLS
     * 
     * Enables mutual TLS authentication.
     * 
     * @param client_cert_path Path to client certificate
     * @param client_key_path Path to client private key
     * @param key_password Password for private key (optional)
     * @return Config Configuration with mTLS
     */
    static Config withMTLS(
        const std::string& client_cert_path,
        const std::string& client_key_path,
        const std::string& key_password = ""
    );
    
    /**
     * @brief Create a secure default config
     * 
     * Enforces TLS 1.2+, hostname verification, cert validation.
     * 
     * @return Config Secure defaults
     */
    static Config secureDefaults();

private:
    Config config_;
};

/**
 * @brief JWKS Secure Fetcher
 * 
 * Fetches JWKS with enhanced transport security (pinning, mTLS).
 * 
 * Usage:
 * ```cpp
 * auto config = JWKSSecurityConfig::withPublicKeyPinning({"hash1", "hash2"});
 * JWKSSecureFetcher fetcher(config);
 * std::string jwks = fetcher.fetch("https://provider.com/.well-known/jwks.json");
 * ```
 */
class JWKSSecureFetcher {
public:
    explicit JWKSSecureFetcher(const JWKSSecurityConfig::Config& config);
    ~JWKSSecureFetcher();
    
    // Disable copy, allow move
    JWKSSecureFetcher(const JWKSSecureFetcher&) = delete;
    JWKSSecureFetcher& operator=(const JWKSSecureFetcher&) = delete;
    JWKSSecureFetcher(JWKSSecureFetcher&&) noexcept;
    JWKSSecureFetcher& operator=(JWKSSecureFetcher&&) noexcept;
    
    /**
     * @brief Fetch JWKS from URL with security enhancements
     * 
     * Applies certificate pinning, mTLS, and TLS validation.
     * 
     * @param url JWKS endpoint URL (must be https://)
     * @return std::string JWKS JSON response
     * @throws std::runtime_error on fetch failure or security violation
     */
    std::string fetch(const std::string& url);
    
    /**
     * @brief Verify certificate pinning
     * 
     * Called during TLS handshake to verify pinned certificate/key.
     * 
     * @param cert_chain Certificate chain from server
     * @return true if pin matches
     */
    bool verifyPinning(const std::vector<std::string>& cert_chain);
    
    /**
     * @brief Get last fetch statistics
     */
    struct FetchStats {
        std::string url;
        int status_code;
        int64_t duration_ms;
        std::string tls_version;
        std::string cipher_suite;
        bool pinning_verified;
        bool mtls_used;
    };
    
    FetchStats getLastFetchStats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Compute SPKI hash from certificate
    std::string computeSPKIHash(const std::string& cert_data);
    
    // Setup TLS context with config
    void setupTLSContext();
};

/**
 * @brief Certificate Utilities
 * 
 * Helper functions for certificate operations.
 */
class CertificateUtils {
public:
    /**
     * @brief Compute SPKI (Subject Public Key Info) hash
     * 
     * Used for public key pinning (RFC 7469).
     * 
     * @param cert_path Path to certificate file
     * @return std::string Base64-encoded SHA256 hash of SPKI
     */
    static std::string computeSPKIHashFromFile(const std::string& cert_path);
    
    /**
     * @brief Compute SPKI hash from PEM certificate string
     * 
     * @param cert_pem PEM-encoded certificate
     * @return std::string Base64-encoded SHA256 hash of SPKI
     */
    static std::string computeSPKIHashFromPEM(const std::string& cert_pem);
    
    /**
     * @brief Verify certificate is valid
     * 
     * Checks expiration, signature, etc.
     * 
     * @param cert_path Path to certificate file
     * @return true if certificate is valid
     */
    static bool verifyCertificate(const std::string& cert_path);
    
    /**
     * @brief Get certificate info
     */
    struct CertInfo {
        std::string subject;
        std::string issuer;
        std::string serial_number;
        std::string not_before;
        std::string not_after;
        bool is_expired;
        int key_size_bits;
        std::string signature_algorithm;
    };
    
    static CertInfo getCertificateInfo(const std::string& cert_path);
};

} // namespace auth
} // namespace themis
