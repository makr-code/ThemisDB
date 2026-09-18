/**
 * @file jwks_security.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    
    /**
     * @brief JWKSSecurity Config.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit JWKSSecurityConfig(const Config& config);
    
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Validate.
     */
    void validate() const;
    
    /**
     * @brief With Public Key Pinning.
     * @param[in] spki_hashes Input parameter.
     * @return Return value.
     */
    static Config withPublicKeyPinning(const std::vector<std::string>& spki_hashes);
    
    /**
     * @brief With Certificate Pinning.
     * @param[in] cert_path Path to the cert.
     * @return Return value.
     */
    static Config withCertificatePinning(const std::string& cert_path);
    
    static Config withMTLS(
        const std::string& client_cert_path,
        const std::string& client_key_path,
        const std::string& key_password = ""
    );
    
    /**
     * @brief Secure Defaults.
     * @return Return value.
     */
    static Config secureDefaults();

private:
    Config config_;
};

class JWKSSecureFetcher {
public:
    /**
     * @brief JWKSSecure Fetcher.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit JWKSSecureFetcher(const JWKSSecurityConfig::Config& config);
    ~JWKSSecureFetcher();
    
    // Disable copy, allow move
    JWKSSecureFetcher(const JWKSSecureFetcher&) = delete;
    JWKSSecureFetcher& operator=(const JWKSSecureFetcher&) = delete;
    JWKSSecureFetcher(JWKSSecureFetcher&&) noexcept;
    JWKSSecureFetcher& operator=(JWKSSecureFetcher&&) noexcept;
    
    /**
     * @brief Fetch.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    std::string fetch(const std::string& url);
    
    /**
     * @brief Verify Pinning.
     * @param[in] cert_chain Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyPinning(const std::vector<std::string>& cert_chain);
    
    struct FetchStats {
        std::string url;
        int status_code;
        int64_t duration_ms;
        std::string tls_version;
        std::string cipher_suite;
        bool pinning_verified;
        bool mtls_used;
    };
    
    /**
     * @brief Get Last Fetch Stats.
     * @return Return value.
     */
    FetchStats getLastFetchStats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Compute SPKI hash from certificate
     * @param[in] cert_data Input parameter.
     * @return Return value.
     */
    std::string computeSPKIHash(const std::string& cert_data);
    
    /**
     * @brief Setup TLS context with config
     */
    void setupTLSContext();
};

class CertificateUtils {
public:
    /**
     * @brief Compute SPKIHash From File.
     * @param[in] cert_path Path to the cert.
     * @return Return value.
     */
    static std::string computeSPKIHashFromFile(const std::string& cert_path);
    
    /**
     * @brief Compute SPKIHash From PEM.
     * @param[in] cert_pem Input parameter.
     * @return Return value.
     */
    static std::string computeSPKIHashFromPEM(const std::string& cert_pem);
    
    /**
     * @brief Verify Certificate.
     * @param[in] cert_path Path to the cert.
     * @return True when the operation succeeds.
     */
    static bool verifyCertificate(const std::string& cert_path);
    
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
    
    /**
     * @brief Get Certificate Info.
     * @param[in] cert_path Path to the cert.
     * @return Return value.
     */
    static CertInfo getCertificateInfo(const std::string& cert_path);
};

} // namespace auth
} // namespace themis
