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

enum class DecisionClass : uint8_t {
    unspecified    = 0,
    authentication = 1,
    policy         = 2,
    revocation     = 3,
    federation     = 4,
};

class AuthAuditLogger {
public:
    explicit AuthAuditLogger(utils::AuditLogger* logger = nullptr)
        : logger_(logger) {}

    /**
     * @brief Set Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setLogger without additional internal calls.
     */
    void setLogger(utils::AuditLogger* logger) { logger_ = logger; }

    bool isEnabled() const { return logger_ != nullptr; }

    // -----------------------------------------------------------------------
    // Decision-class-tagged emit
    // -----------------------------------------------------------------------

    void emitWithDecisionClass(utils::SecurityEventType type,
                               const std::string& user_id,
                               const std::string& resource,
                               DecisionClass dc,
                               const nlohmann::json& details = {});

    // -----------------------------------------------------------------------
    // JWT / Token events
    // -----------------------------------------------------------------------

    /**
     * @brief Log JWTSuccess.
     * @param[in] sub Input parameter.
     * @param[in] jti Input parameter.
     * @param[in] issuer Input parameter.
     * @param[in] kid Input parameter.
     */
    void logJWTSuccess(const std::string& sub,
                       const std::string& jti,
                       const std::string& issuer,
                       const std::string& kid);

    void logJWTFailure(const std::string& reason,
                       const std::string& kid = "");

    /**
     * @brief Log Token Revoked.
     * @param[in] jti Input parameter.
     * @param[in] sub Input parameter.
     */
    void logTokenRevoked(const std::string& jti,
                         const std::string& sub);

    // -----------------------------------------------------------------------
    // GSSAPI / Kerberos events
    // -----------------------------------------------------------------------

    /**
     * @brief Log Kerberos Success.
     * @param[in] principal Input parameter.
     */
    void logKerberosSuccess(const std::string& principal);

    /**
     * @brief Log Kerberos Failure.
     * @param[in] reason Input parameter.
     */
    void logKerberosFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // MFA / TOTP events
    // -----------------------------------------------------------------------

    /**
     * @brief Log TOTPSuccess.
     * @param[in] user_id Identifier of the user.
     */
    void logTOTPSuccess(const std::string& user_id);

    /**
     * @brief Log TOTPFailure.
     * @param[in] user_id Identifier of the user.
     */
    void logTOTPFailure(const std::string& user_id);

    /**
     * @brief Log TOTPDrift.
     * @param[in] user_id Identifier of the user.
     * @param[in] step_offset Input parameter.
     * @param[in] timestamp Input parameter.
     */
    void logTOTPDrift(const std::string& user_id,
                      int step_offset,
                      std::chrono::system_clock::time_point timestamp);

    /**
     * @brief Log Recovery Code Used.
     * @param[in] user_id Identifier of the user.
     */
    void logRecoveryCodeUsed(const std::string& user_id);

    /**
     * @brief Log MFAEnrolled.
     * @param[in] user_id Identifier of the user.
     */
    void logMFAEnrolled(const std::string& user_id);

    // -----------------------------------------------------------------------
    // API Key events
    // -----------------------------------------------------------------------

    /**
     * @brief Log Api Key Success.
     * @param[in] key_id Identifier of the key.
     * @param[in] principal Input parameter.
     */
    void logApiKeySuccess(const std::string& key_id,
                          const std::string& principal);

    /**
     * @brief Log Api Key Failure.
     * @param[in] key_id Identifier of the key.
     * @param[in] reason Input parameter.
     */
    void logApiKeyFailure(const std::string& key_id,
                          const std::string& reason);

    // -----------------------------------------------------------------------
    // OAuth / SAML events
    // -----------------------------------------------------------------------

    /**
     * @brief Log OAuth Device Granted.
     * @param[in] client_id Identifier of the client.
     * @param[in] sub Input parameter.
     */
    void logOAuthDeviceGranted(const std::string& client_id,
                               const std::string& sub);

    /**
     * @brief Log OAuth Device Denied.
     * @param[in] client_id Identifier of the client.
     * @param[in] reason Input parameter.
     */
    void logOAuthDeviceDenied(const std::string& client_id,
                              const std::string& reason);

    /**
     * @brief Log SAMLSuccess.
     * @param[in] subject Input parameter.
     * @param[in] issuer Input parameter.
     */
    void logSAMLSuccess(const std::string& subject,
                        const std::string& issuer);

    /**
     * @brief Log SAMLFailure.
     * @param[in] reason Input parameter.
     */
    void logSAMLFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // Passkey / FIDO2 events
    // -----------------------------------------------------------------------

    /**
     * @brief Log Passkey Success.
     * @param[in] user_id Identifier of the user.
     * @param[in] credential_id Identifier of the credential.
     */
    void logPasskeySuccess(const std::string& user_id, const std::string& credential_id);

    /**
     * @brief Log Passkey Failure.
     * @param[in] user_id Identifier of the user.
     * @param[in] reason Input parameter.
     */
    void logPasskeyFailure(const std::string& user_id, const std::string& reason);

    /**
     * @brief Log Passkey Registered.
     * @param[in] user_id Identifier of the user.
     * @param[in] credential_id Identifier of the credential.
     * @param[in] rp_id Identifier of the rp.
     */
    void logPasskeyRegistered(const std::string& user_id,
                              const std::string& credential_id,
                              const std::string& rp_id);

    // -----------------------------------------------------------------------
    // mTLS events
    // -----------------------------------------------------------------------

    /**
     * @brief Log MTLSSuccess.
     * @param[in] principal Input parameter.
     * @param[in] serial Input parameter.
     */
    void logMTLSSuccess(const std::string& principal, const std::string& serial);

    /**
     * @brief Log MTLSFailure.
     * @param[in] reason Input parameter.
     */
    void logMTLSFailure(const std::string& reason);

    // -----------------------------------------------------------------------
    // Role / permission change events
    // -----------------------------------------------------------------------

    /**
     * @brief Log Role Change.
     * @param[in] user_id Identifier of the user.
     * @param[in] old_role Input parameter.
     * @param[in] new_role Input parameter.
     */
    void logRoleChange(const std::string& user_id,
                       const std::string& old_role,
                       const std::string& new_role);

    /**
     * @brief Log Permission Change.
     * @param[in] user_id Identifier of the user.
     * @param[in] permission Input parameter.
     * @param[in] granted Input parameter.
     */
    void logPermissionChange(const std::string& user_id,
                             const std::string& permission,
                             bool granted);

    // -----------------------------------------------------------------------
    // LDAP / Active Directory events
    // -----------------------------------------------------------------------

    /**
     * @brief Log LDAPSuccess.
     * @param[in] username Input parameter.
     * @param[in] dn Input parameter.
     */
    void logLDAPSuccess(const std::string& username,
                        const std::string& dn);

    /**
     * @brief Log LDAPFailure.
     * @param[in] username Input parameter.
     * @param[in] reason Input parameter.
     */
    void logLDAPFailure(const std::string& username,
                        const std::string& reason);

    // -----------------------------------------------------------------------
    // Zero-trust continuous verification events
    // -----------------------------------------------------------------------

    void logZeroTrustAllowed(const std::string& user_id,
                             const std::string& resource,
                             double trust_score,
                             const std::string& request_id = "");

    void logZeroTrustDenied(const std::string& user_id,
                            const std::string& resource,
                            const std::string& reason,
                            const std::string& request_id = "");

    /**
     * @brief Log Zero Trust Re Evaluation Failed.
     * @param[in] user_id Identifier of the user.
     * @param[in] session_id Identifier of the session.
     * @param[in] reason Input parameter.
     */
    void logZeroTrustReEvaluationFailed(const std::string& user_id,
                                        const std::string& session_id,
                                        const std::string& reason);

    /**
     * @brief ----------------------------------------------------------------------- Anomaly detection events (brute-force, credential stuffing) -----------------------------------------------------------------------
     * @param[in] user_id Identifier of the user.
     * @param[in] ip Input parameter.
     * @param[in] failed_attempts Input parameter.
     */

    void logBruteForceDetected(const std::string& user_id,
                               const std::string& ip,
                               size_t failed_attempts);

    /**
     * @brief Log Credential Stuffing Suspected.
     * @param[in] ip Input parameter.
     * @param[in] distinct_users Input parameter.
     */
    void logCredentialStuffingSuspected(const std::string& ip,
                                        size_t distinct_users);

    /**
     * @brief Log Account Lockout Triggered.
     * @param[in] user_id Identifier of the user.
     * @param[in] ip Input parameter.
     */
    void logAccountLockoutTriggered(const std::string& user_id,
                                    const std::string& ip);

private:
    utils::AuditLogger* logger_;  ///< Non-owning; may be nullptr.

    void emit(utils::SecurityEventType type,
              const std::string& user_id,
              const std::string& resource,
              const nlohmann::json& details = {});
};

} // namespace auth
} // namespace themis
