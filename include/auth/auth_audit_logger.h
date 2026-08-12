/**
 * @file auth_audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <optional>
#include <memory>
#include <chrono>

#include "utils/audit_logger.h"

namespace themis {
namespace auth {

/**
 * @brief Strongly-typed audit logging facade for authentication events.
 *
 * Wraps `utils::AuditLogger` and provides convenience methods for every
 * auth-module event category.  All methods are no-ops when no logger is
 * attached, so callers never need to guard with if (logger_) checks.
 *
 * Usage pattern (mirrors JWTKeyRotationManager):
 * @code
 *   JWTValidator validator(config);
 *   validator.setAuditLogger(&audit_logger);   // attach; non-owning
 * @endcode
 */
class AuthAuditLogger {
public:
    /**
     * @brief Construct with an optional underlying AuditLogger.
     * @param logger Non-owning pointer; may be nullptr (disables logging).
     */
    explicit AuthAuditLogger(utils::AuditLogger* logger = nullptr)
        : logger_(logger) {}

    /** Attach or detach the underlying logger (nullptr = disabled). */
    void setLogger(utils::AuditLogger* logger) { logger_ = logger; }

    /** @return true if a logger is currently attached. */
    bool isEnabled() const { return logger_ != nullptr; }

    // -----------------------------------------------------------------------
    // JWT / Token events
    // -----------------------------------------------------------------------

    /** JWT token was accepted and claims extracted. */
    void logJWTSuccess(const std::string& sub,
                       const std::string& jti,
                       const std::string& issuer,
                       const std::string& kid);

    /** JWT token was rejected (expired, bad sig, revoked, …). */
    void logJWTFailure(const std::string& reason,
                       const std::string& kid = "");

    /** Token JTI was added to the revocation blacklist. */
    void logTokenRevoked(const std::string& jti,
                         const std::string& sub);

    // -----------------------------------------------------------------------
    // GSSAPI / Kerberos events
    // -----------------------------------------------------------------------

    /** Kerberos / GSSAPI authentication succeeded. */
    void logKerberosSuccess(const std::string& principal);

    /** Kerberos / GSSAPI authentication failed. */
    void logKerberosFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // MFA / TOTP events
    // -----------------------------------------------------------------------

    /** TOTP code was accepted. */
    void logTOTPSuccess(const std::string& user_id);

    /** TOTP code was rejected. */
    void logTOTPFailure(const std::string& user_id);

    /** TOTP code validated with a non-zero time step offset (clock drift indicator).
     *
     *  Large or sustained offsets indicate a misconfigured device clock and should
     *  be investigated. The audit entry records the subject, step offset, and the
     *  Unix timestamp of the validation so that operations teams can track trends.
     */
    void logTOTPDrift(const std::string& user_id,
                      int step_offset,
                      std::chrono::system_clock::time_point timestamp);

    /** Recovery code was used (single-use codes only). */
    void logRecoveryCodeUsed(const std::string& user_id);

    /** New MFA enrollment generated for user. */
    void logMFAEnrolled(const std::string& user_id);

    // -----------------------------------------------------------------------
    // API Key events
    // -----------------------------------------------------------------------

    /** API key authentication succeeded. */
    void logApiKeySuccess(const std::string& key_id,
                          const std::string& principal);

    /** API key authentication failed (not found, inactive, expired, bad secret). */
    void logApiKeyFailure(const std::string& key_id,
                          const std::string& reason);

    // -----------------------------------------------------------------------
    // OAuth / SAML events
    // -----------------------------------------------------------------------

    /** OAuth 2.0 device authorization was granted. */
    void logOAuthDeviceGranted(const std::string& client_id,
                               const std::string& sub);

    /** OAuth 2.0 device authorization was denied or expired. */
    void logOAuthDeviceDenied(const std::string& client_id,
                              const std::string& reason);

    /** SAML assertion was accepted. */
    void logSAMLSuccess(const std::string& subject,
                        const std::string& issuer);

    /** SAML assertion was rejected. */
    void logSAMLFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // LDAP / Active Directory events
    // -----------------------------------------------------------------------

    /** LDAP direct-bind authentication succeeded. */
    void logLDAPSuccess(const std::string& username,
                        const std::string& dn);

    /** LDAP direct-bind authentication failed. */
    void logLDAPFailure(const std::string& username,
                        const std::string& reason);

    // -----------------------------------------------------------------------
    // Zero-trust continuous verification events
    // -----------------------------------------------------------------------

    /** Zero-trust continuous verification passed for a request. */
    void logZeroTrustAllowed(const std::string& user_id,
                             const std::string& resource,
                             double trust_score,
                             const std::string& request_id = "");

    /** Zero-trust continuous verification denied a request. */
    void logZeroTrustDenied(const std::string& user_id,
                            const std::string& resource,
                            const std::string& reason,
                            const std::string& request_id = "");

    /**
     * @brief Emitted when background async re-evaluation revokes an active session.
     *
     * Resource path: "zero_trust/re_evaluation_failed"
     */
    void logZeroTrustReEvaluationFailed(const std::string& user_id,
                                        const std::string& session_id,
                                        const std::string& reason);

    // -----------------------------------------------------------------------
    // Anomaly detection events (brute-force, credential stuffing)
    // -----------------------------------------------------------------------

    /** Brute-force attack detected: account locked after repeated failures. */
    void logBruteForceDetected(const std::string& user_id,
                               const std::string& ip,
                               size_t failed_attempts);

    /** Credential stuffing suspected: many distinct usernames tried from one IP. */
    void logCredentialStuffingSuspected(const std::string& ip,
                                        size_t distinct_users);

    /** Account locked due to repeated authentication failures. */
    void logAccountLockoutTriggered(const std::string& user_id,
                                    const std::string& ip);

private:
    utils::AuditLogger* logger_;  ///< Non-owning; may be nullptr.

    /** Fire a security event if a logger is attached. */
    void emit(utils::SecurityEventType type,
              const std::string& user_id,
              const std::string& resource,
              const nlohmann::json& details = {});
};

} // namespace auth
} // namespace themis
