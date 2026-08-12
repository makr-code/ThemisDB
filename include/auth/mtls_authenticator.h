/**
 * @file mtls_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Claims extracted from a validated client certificate during mTLS authentication.
 *
 * Identity information is derived from the X.509 certificate's Subject Distinguished
 * Name (DN) and Subject Alternative Names (SAN).  The authenticating server validates
 * the certificate chain and, optionally, a Certificate Revocation List (CRL) before
 * populating these claims.
 */
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

    /**
     * @brief Return true if the certificate has passed its not_after timestamp.
     */
    bool isExpired() const {
        return std::chrono::system_clock::now() > not_after;
    }
};

/**
 * @brief Mutual TLS (mTLS) authenticator for certificate-based client authentication.
 *
 * Validates X.509 client certificates presented during a TLS handshake (or supplied
 * directly as PEM/DER bytes in test and proxy-termination scenarios).  The authenticator:
 *
 *   1. Parses the certificate with OpenSSL.
 *   2. Verifies the certificate chain against one or more trusted CA certificates.
 *   3. Checks certificate validity window (not-before / not-after).
 *   4. Optionally verifies against a loaded CRL (Certificate Revocation List).
 *   5. Optionally checks whether the serial number appears in a runtime revocation list.
 *   6. Extracts identity fields (CN, SAN DNS, SAN IP, SAN email) and returns MTLSClaims.
 *
 * Thread safety: all public methods are protected by an internal mutex and are safe
 * to call from multiple threads.
 *
 * Usage example:
 * @code
 *   MTLSAuthenticator::Config cfg;
 *   cfg.ca_cert_pem = loadFile("/etc/themis/ca.crt");
 *   cfg.require_client_cert = true;
 *
 *   MTLSAuthenticator auth(cfg);
 *
 *   // At connection time, after TLS handshake, obtain the peer certificate PEM
 *   // (e.g., from SSL_get_peer_certificate + PEM_write_bio_X509) and call:
 *   auto claims = auth.authenticate(peer_cert_pem);
 * @endcode
 */
class MTLSAuthenticator {
public:
    /**
     * @brief Configuration for the mTLS authenticator.
     */
    struct Config {
        /// PEM-encoded trusted CA certificate(s).  Multiple CAs may be concatenated.
        std::string ca_cert_pem;

        /// Optional: PEM-encoded CRL to check against during validation.
        std::string crl_pem;

        /// When true, authenticate() throws if the certificate is not signed by a
        /// trusted CA.  When false, chain validation is skipped (useful for
        /// testing with self-signed certs).  Default: true.
        bool verify_chain{true};

        /// When true, certificates whose serial appears in the runtime revocation set
        /// (managed via revokeCertificate() / unrevokeCertificate()) are rejected.
        /// Default: true.
        bool check_revocation{true};

        /// When true, authenticate() throws AUTH_CONFIG_INVALID if no CA cert is
        /// configured and verify_chain is true.  Default: true.
        bool require_client_cert{true};
    };

    /**
     * @brief Construct the authenticator with the given configuration.
     *
     * @throws AuthException (AUTH_CONFIG_INVALID) if verify_chain is true and
     *         ca_cert_pem is empty.
     */
    explicit MTLSAuthenticator(const Config& config);

    ~MTLSAuthenticator();

    // -------------------------------------------------------------------------
    // Core authentication
    // -------------------------------------------------------------------------

    /**
     * @brief Authenticate a client by validating its PEM-encoded X.509 certificate.
     *
     * Steps performed:
     *   1. Parse the PEM certificate with OpenSSL.
     *   2. If verify_chain is true, build and verify the chain against the loaded CA(s).
     *   3. Check the validity window (not-before / not-after).
     *   4. If check_revocation is true and a CRL is configured, verify the CRL signature
     *      and check that the serial number is not listed.
     *   5. Check the runtime revocation set.
     *   6. Extract Subject DN, Issuer DN, serial number, SHA-256 fingerprint, and SANs.
     *   7. Resolve principal: first SAN email address if present, otherwise Subject CN.
     *
     * @param cert_pem  PEM-encoded X.509 certificate (BEGIN CERTIFICATE … END CERTIFICATE).
     * @return MTLSClaims populated with identity information.
     * @throws AuthException(MTLS_CERT_INVALID) if the certificate cannot be parsed or
     *         the chain fails to verify.
     * @throws AuthException(MTLS_CERT_EXPIRED) if the certificate's validity window has
     *         passed or has not yet started.
     * @throws AuthException(MTLS_CERT_REVOKED) if the certificate's serial number appears
     *         in the CRL or the runtime revocation set.
     */
    MTLSClaims authenticate(const std::string& cert_pem);

    /**
     * @brief Authenticate a client by validating its DER-encoded X.509 certificate.
     *
     * Converts the DER bytes to PEM internally and delegates to authenticate(const std::string&).
     *
     * @param cert_der  DER-encoded certificate bytes.
     * @return MTLSClaims populated with identity information.
     * @throws AuthException on any validation failure (see authenticate(PEM) above).
     */
    MTLSClaims authenticateDER(const std::vector<uint8_t>& cert_der);

    // -------------------------------------------------------------------------
    // Runtime certificate revocation
    // -------------------------------------------------------------------------

    /**
     * @brief Add a certificate serial number (hex string) to the runtime revocation set.
     *
     * Thread-safe.  Has no effect if the serial is already present.
     *
     * @param serial_hex  Hex-encoded serial number (e.g., "0A1B2C3D").
     * @throws AuthException (AUTH_CONFIG_INVALID) if serial_hex is empty.
     */
    void revokeCertificate(const std::string& serial_hex);

    /**
     * @brief Remove a certificate serial number from the runtime revocation set.
     *
     * Thread-safe.  A no-op if the serial is not present.
     *
     * @param serial_hex  Hex-encoded serial number to unrevoke.
     */
    void unrevokeCertificate(const std::string& serial_hex);

    /**
     * @brief Return true if a given serial number is in the runtime revocation set.
     *
     * @param serial_hex  Hex-encoded serial number to query.
     */
    bool isRevoked(const std::string& serial_hex) const;

    /**
     * @brief Return the number of entries in the runtime revocation set.
     */
    size_t revokedCount() const;

    // -------------------------------------------------------------------------
    // Utility helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the SHA-256 fingerprint of a PEM certificate.
     *
     * @param cert_pem  PEM-encoded certificate.
     * @return Lowercase hex-encoded SHA-256 digest of the DER form.
     * @throws AuthException (MTLS_CERT_INVALID) if the certificate cannot be parsed.
     */
    static std::string certFingerprint(const std::string& cert_pem);

    /**
     * @brief Extract the Subject Common Name from a PEM certificate.
     *
     * @param cert_pem  PEM-encoded certificate.
     * @return Subject CN string, or empty string if not present.
     * @throws AuthException (MTLS_CERT_INVALID) if the certificate cannot be parsed.
     */
    static std::string extractSubjectCN(const std::string& cert_pem);

private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_set<std::string> revoked_serials_;

    // OpenSSL X509_STORE for CA chain verification (PIMPL via void*)
    struct Impl;
    std::unique_ptr<Impl> impl_;

    bool initCAStore();
    bool initCRL();

    static std::string x509NameToString(void* name);
    static std::string serialToHex(void* serial);
    static std::string computeFingerprint(void* x509);
    static std::vector<std::string> extractSANs(void* x509, int san_type);
};

} // namespace auth
} // namespace themis
