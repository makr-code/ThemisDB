/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mtls_authenticator.h                               ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

// Input validation limits for mTLS
constexpr size_t MAX_MTLS_CERT_PEM_SIZE    = 65536;  ///< 64 KB max for a PEM certificate
constexpr size_t MAX_MTLS_SUBJECT_LENGTH   = 512;    ///< Max bytes for a subject DN string
constexpr size_t MAX_MTLS_PRINCIPAL_LENGTH = 256;    ///< Max bytes for an extracted principal

/**
 * @brief Configuration for mTLS certificate-based authentication.
 *
 * The authenticator validates PEM-encoded X.509 client certificates
 * against a trusted CA and extracts a principal (by default the CN
 * of the Subject DN) that is then mapped to ThemisDB roles.
 */
struct MTLSConfig {
    bool enabled = false;

    /// Path to PEM-encoded CA certificate used to verify client certs.
    /// Mutually exclusive with @c ca_cert_pem – prefer path for large bundles.
    std::string ca_cert_path;

    /// Inline PEM-encoded CA certificate (alternative to @c ca_cert_path).
    std::string ca_cert_pem;

    /// Optional path to a PEM-encoded CRL file for revocation checking.
    std::string crl_path;

    /// Reject certificates whose NotAfter has passed (default: true).
    bool verify_expiry = true;

    /// Require the Subject to contain a CN field (default: true).
    bool require_subject_cn = true;

    /**
     * @brief Specifies which Subject field is used as the principal.
     *
     * Supported values:
     *  - "CN"  (Common Name, default)
     *  - "DN"  (full Subject Distinguished Name)
     */
    std::string principal_field = "CN";

    /**
     * @brief Maps Subject DN patterns to ThemisDB roles.
     *
     * Patterns are matched against the full Subject DN.
     * Wildcards: '*' matches any sequence of characters.
     * Example: @c subject_pattern = "*.example.com" matches
     *           any CN under example.com.
     */
    struct SubjectMapping {
        std::string subject_pattern; ///< Wildcard pattern matched against full Subject DN
        std::string role;            ///< Role granted when pattern matches
        std::string tenant_id;       ///< Optional: tenant scope for this mapping
    };
    std::vector<SubjectMapping> subject_mappings;
};

/**
 * @brief Claims returned after successful mTLS certificate authentication.
 */
struct MTLSClaims {
    std::string subject_dn;    ///< Full Subject Distinguished Name (e.g. "CN=alice,O=Corp")
    std::string principal;     ///< Extracted principal (CN or full DN, per config)
    std::string issuer_dn;     ///< Issuer Distinguished Name
    std::string serial_number; ///< Certificate serial number (hex)
    std::chrono::system_clock::time_point not_before;
    std::chrono::system_clock::time_point not_after;
    std::vector<std::string> roles;     ///< Roles from subject_mappings
    std::string tenant_id;              ///< Tenant from subject_mappings (may be empty)
};

/**
 * @brief Authenticator for certificate-based mutual TLS (mTLS).
 *
 * Validates PEM-encoded X.509 client certificates presented during the
 * TLS handshake.  The typical usage with AuthMiddleware is:
 *
 * @code
 * MTLSConfig cfg;
 * cfg.ca_cert_path = "/etc/themisdb/ca.pem";
 * cfg.subject_mappings = {{"CN=*.ops.example.com", "ops:admin", "tenant-1"}};
 *
 * auth::MTLSAuthenticator authenticator;
 * authenticator.initialize(cfg);
 *
 * // cert_pem is the raw PEM from the TLS layer
 * auto claims = authenticator.authenticate(cert_pem);
 * @endcode
 *
 * Authentication flow:
 *  1. Parse PEM-encoded client certificate (X.509).
 *  2. Build a CA trust store from @c ca_cert_path or @c ca_cert_pem.
 *  3. Verify certificate chain against the trust store.
 *  4. Optionally check certificate expiry and CRL.
 *  5. Extract principal according to @c principal_field.
 *  6. Map Subject DN to roles via @c subject_mappings.
 *  7. Return @c MTLSClaims on success; throw @c AuthException on failure.
 */
class MTLSAuthenticator {
public:
    MTLSAuthenticator();
    ~MTLSAuthenticator();

    // Non-copyable, non-movable (owns OpenSSL resources)
    MTLSAuthenticator(const MTLSAuthenticator&) = delete;
    MTLSAuthenticator& operator=(const MTLSAuthenticator&) = delete;
    MTLSAuthenticator(MTLSAuthenticator&&) = delete;
    MTLSAuthenticator& operator=(MTLSAuthenticator&&) = delete;

    /**
     * @brief Attach an optional AuditLogger for LOGIN_SUCCESS / LOGIN_FAILED events.
     * Pass nullptr to detach.  The authenticator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Initialise the authenticator with the supplied configuration.
     *
     * Loads the CA certificate and, if configured, the CRL.
     *
     * @param config mTLS configuration
     * @return true on success; false on configuration error (details are logged)
     */
    bool initialize(const MTLSConfig& config);

    /** @brief Returns true if @c initialize() has been called successfully. */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Authenticate a PEM-encoded X.509 client certificate.
     *
     * Steps:
     *  1. Validate input size.
     *  2. Parse PEM to an X.509 structure.
     *  3. Verify certificate chain against the loaded CA trust store.
     *  4. Check expiry (if enabled).
     *  5. Check CRL (if configured).
     *  6. Extract principal from Subject DN.
     *  7. Map Subject DN to roles.
     *
     * @param cert_pem PEM-encoded X.509 client certificate
     * @return Populated MTLSClaims on success
     * @throws AuthException on any validation failure
     */
    MTLSClaims authenticate(const std::string& cert_pem);

    /**
     * @brief Map a Subject DN string to ThemisDB roles and tenant.
     *
     * Iterates over @c config_.subject_mappings in order; returns roles from
     * all matching entries.
     *
     * @param subject_dn Full Subject Distinguished Name string
     * @return {roles, tenant_id}
     */
    std::pair<std::vector<std::string>, std::string>
    mapSubjectToRoles(const std::string& subject_dn) const;

    /**
     * @brief Extract the principal from a Subject DN.
     *
     * Extracts the CN value when @c config_.principal_field == "CN",
     * otherwise returns the full @p subject_dn.
     *
     * @param subject_dn Full Subject Distinguished Name string
     * @return extracted principal string
     */
    std::string extractPrincipal(const std::string& subject_dn) const;

    /** @brief Return the current configuration. */
    const MTLSConfig& getConfig() const { return config_; }

private:
    bool initialized_ = false;
    MTLSConfig config_;
    utils::AuditLogger* audit_logger_ = nullptr;

    // PIMPL: hides OpenSSL X509_STORE* and CRL state from callers
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /** Load CA cert from file or inline PEM into the trust store. */
    bool loadCACertificate();

    /** Load CRL file into the trust store (optional). */
    bool loadCRL();

    /**
     * @brief Verify the parsed X.509 certificate against the trust store.
     * @param x509 Opaque pointer to OpenSSL X509 object.
     * @return true if the chain is valid.
     */
    bool verifyCertificateChain(void* x509) const;

    /**
     * @brief Return true if the Subject DN matches the wildcard pattern.
     *
     * '*' matches any sequence of characters.
     */
    static bool subjectMatchesPattern(const std::string& subject_dn,
                                      const std::string& pattern);
};

} // namespace auth
} // namespace themis
