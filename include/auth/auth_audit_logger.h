/**
 * @file auth_audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
 * @brief Decision classification for operator-visible audit events.
 *
 * Allows SIEMs, dashboards, and runbook automation to filter audit events by
 * the type of security decision that produced them.  Added in the operator
 * diagnostics hardening pass (ROADMAP.md §2c).
 *
 * Values:
 *  - @c unspecified   — decision class not set (backward-compatible default).
 *  - @c authentication — primary credential verification (JWT, LDAP, MFA, …).
 *  - @c policy         — key-management and rotation policy decisions.
 *  - @c revocation     — token or credential revocation decisions.
 *  - @c federation     — federated-identity and cross-realm decisions.
 */
enum class DecisionClass : uint8_t {
    unspecified    = 0,
    authentication = 1,
    policy         = 2,
    revocation     = 3,
    federation     = 4,
};

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

     * @brief TBD: Describe setLogger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setLogger without additional internal calls.
    /** Attach or detach the underlying logger (nullptr = disabled). */
    void setLogger(utils::AuditLogger* logger) { logger_ = logger; }

    /** @return true if a logger is currently attached. */
    bool isEnabled() const { return logger_ != nullptr; }

    // -----------------------------------------------------------------------
    // Decision-class-tagged emit
    // -----------------------------------------------------------------------

    /**
     * @brief Fire a security event tagged with an operator-visible decision class.
     *
     * When @p dc is @c DecisionClass::unspecified the event is emitted without
     * a @c decision_class field (backward-compatible).  For all other values the
     * field is injected into the JSON detail block so that SIEMs can filter by
     * decision type.
     *
     * This is a low-level helper; prefer the typed @c log*() convenience methods.
     */
    void emitWithDecisionClass(utils::SecurityEventType type,
                               const std::string& user_id,
                               const std::string& resource,
                               DecisionClass dc,
                               const nlohmann::json& details = {});

    // -----------------------------------------------------------------------
    // JWT / Token events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logJWTSuccess.
     * @param[in] sub Input parameter.
     * @param[in] jti Input parameter.
     * @param[in] issuer Input parameter.
     * @param[in] kid Input parameter.
    /** JWT token was accepted and claims extracted. */
    void logJWTSuccess(const std::string& sub,
                       const std::string& jti,
                       const std::string& issuer,
                       const std::string& kid);

    /** JWT token was rejected (expired, bad sig, revoked, …). */
    void logJWTFailure(const std::string& reason,
                       const std::string& kid = "");

     * @brief TBD: Describe logTokenRevoked.
     * @param[in] jti Input parameter.
     * @param[in] sub Input parameter.
    /** Token JTI was added to the revocation blacklist. */
    void logTokenRevoked(const std::string& jti,
                         const std::string& sub);

    // -----------------------------------------------------------------------
    // GSSAPI / Kerberos events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logKerberosSuccess.
     * @param[in] principal Input parameter.
    /** Kerberos / GSSAPI authentication succeeded. */
    void logKerberosSuccess(const std::string& principal);

     * @brief TBD: Describe logKerberosFailure.
     * @param[in] reason Input parameter.
    /** Kerberos / GSSAPI authentication failed. */
    void logKerberosFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // MFA / TOTP events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logTOTPSuccess.
     * @param[in] user_id Input parameter.
    /** TOTP code was accepted. */
    void logTOTPSuccess(const std::string& user_id);

     * @brief TBD: Describe logTOTPFailure.
     * @param[in] user_id Input parameter.
    /** TOTP code was rejected. */
    void logTOTPFailure(const std::string& user_id);

    /** TOTP code validated with a non-zero time step offset (clock drift indicator).
     *
     *  Large or sustained offsets indicate a misconfigured device clock and should
     *  be investigated. The audit entry records the subject, step offset, and the
     *  Unix timestamp of the validation so that operations teams can track trends.
     * @brief TBD: Describe logTOTPDrift.
     * @param[in] user_id Input parameter.
     * @param[in] step_offset Input parameter.
     * @param[in] timestamp Input parameter.
     */
    void logTOTPDrift(const std::string& user_id,
                      int step_offset,
                      std::chrono::system_clock::time_point timestamp);

     * @brief TBD: Describe logRecoveryCodeUsed.
     * @param[in] user_id Input parameter.
    /** Recovery code was used (single-use codes only). */
    void logRecoveryCodeUsed(const std::string& user_id);

     * @brief TBD: Describe logMFAEnrolled.
     * @param[in] user_id Input parameter.
    /** New MFA enrollment generated for user. */
    void logMFAEnrolled(const std::string& user_id);

    // -----------------------------------------------------------------------
    // API Key events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logApiKeySuccess.
     * @param[in] key_id Input parameter.
     * @param[in] principal Input parameter.
    /** API key authentication succeeded. */
    void logApiKeySuccess(const std::string& key_id,
                          const std::string& principal);

     * @brief TBD: Describe logApiKeyFailure.
     * @param[in] key_id Input parameter.
     * @param[in] reason Input parameter.
    /** API key authentication failed (not found, inactive, expired, bad secret). */
    void logApiKeyFailure(const std::string& key_id,
                          const std::string& reason);

    // -----------------------------------------------------------------------
    // OAuth / SAML events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logOAuthDeviceGranted.
     * @param[in] client_id Input parameter.
     * @param[in] sub Input parameter.
    /** OAuth 2.0 device authorization was granted. */
    void logOAuthDeviceGranted(const std::string& client_id,
                               const std::string& sub);

     * @brief TBD: Describe logOAuthDeviceDenied.
     * @param[in] client_id Input parameter.
     * @param[in] reason Input parameter.
    /** OAuth 2.0 device authorization was denied or expired. */
    void logOAuthDeviceDenied(const std::string& client_id,
                              const std::string& reason);

     * @brief TBD: Describe logSAMLSuccess.
     * @param[in] subject Input parameter.
     * @param[in] issuer Input parameter.
    /** SAML assertion was accepted. */
    void logSAMLSuccess(const std::string& subject,
                        const std::string& issuer);

     * @brief TBD: Describe logSAMLFailure.
     * @param[in] reason Input parameter.
    /** SAML assertion was rejected. */
    void logSAMLFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // Passkey / FIDO2 events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logPasskeySuccess.
     * @param[in] user_id Input parameter.
     * @param[in] credential_id Input parameter.
    /** Passkey authentication succeeded. */
    void logPasskeySuccess(const std::string& user_id, const std::string& credential_id);

     * @brief TBD: Describe logPasskeyFailure.
     * @param[in] user_id Input parameter.
     * @param[in] reason Input parameter.
    /** Passkey authentication failed. */
    void logPasskeyFailure(const std::string& user_id, const std::string& reason);

     * @brief TBD: Describe logPasskeyRegistered.
     * @param[in] user_id Input parameter.
     * @param[in] credential_id Input parameter.
     * @param[in] rp_id Input parameter.
    /** Passkey credential was registered for a user. */
    void logPasskeyRegistered(const std::string& user_id,
                              const std::string& credential_id,
                              const std::string& rp_id);

    // -----------------------------------------------------------------------
    // mTLS events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logMTLSSuccess.
     * @param[in] principal Input parameter.
     * @param[in] serial Input parameter.
    /** mTLS client certificate authentication succeeded. */
    void logMTLSSuccess(const std::string& principal, const std::string& serial);

     * @brief TBD: Describe logMTLSFailure.
     * @param[in] reason Input parameter.
    /** mTLS client certificate authentication failed. */
    void logMTLSFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // Role / permission change events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logRoleChange.
     * @param[in] user_id Input parameter.
     * @param[in] old_role Input parameter.
     * @param[in] new_role Input parameter.
    /** A user's role was changed. */
    void logRoleChange(const std::string& user_id,
                       const std::string& old_role,
                       const std::string& new_role);

     * @brief TBD: Describe logPermissionChange.
     * @param[in] user_id Input parameter.
     * @param[in] permission Input parameter.
     * @param[in] granted Input parameter.
    /** A permission was granted or revoked for a user. */
    void logPermissionChange(const std::string& user_id,
                             const std::string& permission,
                             bool granted);

    // -----------------------------------------------------------------------
    // LDAP / Active Directory events
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logLDAPSuccess.
     * @param[in] username Input parameter.
     * @param[in] dn Input parameter.
    /** LDAP direct-bind authentication succeeded. */
    void logLDAPSuccess(const std::string& username,
                        const std::string& dn);

     * @brief TBD: Describe logLDAPFailure.
     * @param[in] username Input parameter.
     * @param[in] reason Input parameter.
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
     * @param[in] user_id Input parameter.
     * @param[in] session_id Input parameter.
     * @param[in] reason Input parameter.
     */
    void logZeroTrustReEvaluationFailed(const std::string& user_id,
                                        const std::string& session_id,
                                        const std::string& reason);

    // -----------------------------------------------------------------------
    // Anomaly detection events (brute-force, credential stuffing)
    // -----------------------------------------------------------------------

     * @brief TBD: Describe logBruteForceDetected.
     * @param[in] user_id Input parameter.
     * @param[in] ip Input parameter.
     * @param[in] failed_attempts Input parameter.
    /** Brute-force attack detected: account locked after repeated failures. */
    void logBruteForceDetected(const std::string& user_id,
                               const std::string& ip,
                               size_t failed_attempts);

     * @brief TBD: Describe logCredentialStuffingSuspected.
     * @param[in] ip Input parameter.
     * @param[in] distinct_users Input parameter.
    /** Credential stuffing suspected: many distinct usernames tried from one IP. */
    void logCredentialStuffingSuspected(const std::string& ip,
                                        size_t distinct_users);

     * @brief TBD: Describe logAccountLockoutTriggered.
     * @param[in] user_id Input parameter.
     * @param[in] ip Input parameter.
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
