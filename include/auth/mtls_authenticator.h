/**
 * @file mtls_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/auth_error.h"

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace auth {

// Forward declaration
class AuthAuditLogger;

struct MTLSClaims {
    std::string principal;               ///< Resolved principal (Common Name or SAN email)
    std::string subject_dn;             ///< Full Subject Distinguished Name
    std::string issuer_dn;              ///< Full Issuer Distinguished Name
    std::string serial_number;          ///< Certificate serial number (hex)
    std::string fingerprint_sha256;     ///< SHA-256 fingerprint of DER-encoded cert (hex)
    std::vector<std::string> san_dns_names;   ///< Subject Alternative Names – DNS entries
    std::vector<std::string> san_ip_addresses; ///< Subject Alternative Names – IP entries
    std::vector<std::string> san_email_addresses; ///< Subject Alternative Names – email entries
    std::vector<std::string> roles;     ///< Optional roles mapped from the certificate
    std::chrono::system_clock::time_point not_before; ///< Certificate validity start
    std::chrono::system_clock::time_point not_after;  ///< Certificate validity end

    bool isExpired() const {
        return std::chrono::system_clock::now() > not_after;
    }
};

class MTLSAuthenticator {
public:
    struct Config {
        std::string ca_cert_pem;

        std::string crl_pem;

        bool verify_chain{true};

        bool check_revocation{true};

        bool require_client_cert{true};
    };

    /**
     * @brief MTLSAuthenticator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MTLSAuthenticator(const Config& config);

    ~MTLSAuthenticator();

    // -------------------------------------------------------------------------
    // Core authentication
    // -------------------------------------------------------------------------

    /**
     * @brief Authenticate.
     * @param[in] cert_pem Input parameter.
     * @return Authentication result.
     */
    MTLSClaims authenticate(const std::string& cert_pem);

    /**
     * @brief Authenticate DER.
     * @param[in] cert_der Input parameter.
     * @return Return value.
     */
    MTLSClaims authenticateDER(const std::vector<uint8_t>& cert_der);

    // -------------------------------------------------------------------------
    // Runtime certificate revocation
    // -------------------------------------------------------------------------

    /**
     * @brief Revoke Certificate.
     * @param[in] serial_hex Input parameter.
     */
    void revokeCertificate(const std::string& serial_hex);

    /**
     * @brief Unrevoke Certificate.
     * @param[in] serial_hex Input parameter.
     */
    void unrevokeCertificate(const std::string& serial_hex);

    /**
     * @brief Is Revoked.
     * @param[in] serial_hex Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRevoked(const std::string& serial_hex) const;

    /**
     * @brief Revoked Count.
     * @return Return value.
     */
    size_t revokedCount() const;

    // -------------------------------------------------------------------------
    // Utility helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Cert Fingerprint.
     * @param[in] cert_pem Input parameter.
     * @return Return value.
     */
    static std::string certFingerprint(const std::string& cert_pem);

    /**
     * @brief Extract Subject CN.
     * @param[in] cert_pem Input parameter.
     * @return Return value.
     */
    static std::string extractSubjectCN(const std::string& cert_pem);

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(AuthAuditLogger* logger) { audit_logger_ = logger; }

private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_set<std::string> revoked_serials_;
    AuthAuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.

    // OpenSSL X509_STORE for CA chain verification (PIMPL via void*)
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /**
     * @brief Init CAStore.
     * @return True when the operation succeeds.
     */
    bool initCAStore();
    /**
     * @brief Init CRL.
     * @return True when the operation succeeds.
     */
    bool initCRL();

    /**
     * @brief X509 Name To String.
     * @param[in,out] name Input/output parameter.
     * @return Return value.
     */
    static std::string x509NameToString(void* name);
    /**
     * @brief Serial To Hex.
     * @param[in,out] serial Input/output parameter.
     * @return Return value.
     */
    static std::string serialToHex(void* serial);
    /**
     * @brief Compute Fingerprint.
     * @param[in,out] x509 Input/output parameter.
     * @return Return value.
     */
    static std::string computeFingerprint(void* x509);
    /**
     * @brief Extract SANs.
     * @param[in,out] x509 Input/output parameter.
     * @param[in] san_type Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> extractSANs(void* x509, int san_type);
};

} // namespace auth
} // namespace themis
