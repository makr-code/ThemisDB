/**
 * @file auth_audit_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/auth_audit_logger.h"

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// Private helper
// ---------------------------------------------------------------------------

/**
 * @brief Emit.
 * @param[in] type Input parameter.
 * @param[in] user_id Identifier of the user.
 * @param[in] resource Input parameter.
 * @param[in] details Input parameter.
 * @details Calls: logSecurityEvent().
 */
void AuthAuditLogger::emit(utils::SecurityEventType type, const std::string &user_id, const std::string &resource,
                           const nlohmann::json &details) {
    if (logger_) {
        logger_->logSecurityEvent(type, user_id, resource, details);
    }
}

/**
 * @brief Emit With Decision Class.
 * @param[in] type Input parameter.
 * @param[in] user_id Identifier of the user.
 * @param[in] resource Input parameter.
 * @param[in] dc Input parameter.
 * @param[in] details Input parameter.
 * @details Calls: logSecurityEvent().
 */
void AuthAuditLogger::emitWithDecisionClass(utils::SecurityEventType type,
                                             const std::string &user_id,
                                             const std::string &resource,
                                             DecisionClass dc,
                                             const nlohmann::json &details) {
    if (!logger_) {
        return;
    }
    if (dc == DecisionClass::unspecified) {
        logger_->logSecurityEvent(type, user_id, resource, details);
        return;
    }
    // Inject decision_class into a copy of the detail block.
    nlohmann::json tagged = details;
    switch (dc) {
        case DecisionClass::authentication: tagged["decision_class"] = "authentication"; break;
        case DecisionClass::policy:         tagged["decision_class"] = "policy";         break;
        case DecisionClass::revocation:     tagged["decision_class"] = "revocation";     break;
        case DecisionClass::federation:     tagged["decision_class"] = "federation";     break;
        default: break;
    }
    logger_->logSecurityEvent(type, user_id, resource, tagged);
}

// ---------------------------------------------------------------------------
// JWT / Token events
// ---------------------------------------------------------------------------

/**
 * @brief Log JWTSuccess.
 * @param[in] sub Input parameter.
 * @param[in] jti Input parameter.
 * @param[in] issuer Input parameter.
 * @param[in] kid Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logJWTSuccess(const std::string &sub, const std::string &jti, const std::string &issuer,
                                    const std::string &kid) {
    nlohmann::json d;
    d["jti"]    = jti;
    d["issuer"] = issuer;
    d["kid"]    = kid;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, sub, "jwt/token", d);
}

/**
 * @brief Log JWTFailure.
 * @param[in] reason Input parameter.
 * @param[in] kid Input parameter.
 * @details Calls: empty(), emit().
 */
void AuthAuditLogger::logJWTFailure(const std::string &reason, const std::string &kid) {
    nlohmann::json d;
    d["reason"] = reason;
    if (!kid.empty()) {
        d["kid"] = kid;
    }
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token", d);
}

/**
 * @brief Log Token Revoked.
 * @param[in] jti Input parameter.
 * @param[in] sub Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logTokenRevoked(const std::string &jti, const std::string &sub) {
    nlohmann::json d;
    d["jti"] = jti;
    emit(utils::SecurityEventType::TOKEN_REVOKED, sub, "jwt/token/" + jti, d);
}

// ---------------------------------------------------------------------------
// GSSAPI / Kerberos events
// ---------------------------------------------------------------------------

/**
 * @brief Log Kerberos Success.
 * @param[in] principal Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logKerberosSuccess(const std::string &principal) {
    emit(utils::SecurityEventType::LOGIN_SUCCESS, principal, "kerberos/principal", {});
}

/**
 * @brief Log Kerberos Failure.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logKerberosFailure(const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "kerberos/principal", d);
}

// ---------------------------------------------------------------------------
// MFA / TOTP events
// ---------------------------------------------------------------------------

/**
 * @brief Log TOTPSuccess.
 * @param[in] user_id Identifier of the user.
 * @details Calls: emit().
 */
void AuthAuditLogger::logTOTPSuccess(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_TOTP_SUCCESS, user_id, "mfa/totp", {});
}

/**
 * @brief Log TOTPFailure.
 * @param[in] user_id Identifier of the user.
 * @details Calls: emit().
 */
void AuthAuditLogger::logTOTPFailure(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_TOTP_FAILED, user_id, "mfa/totp", {});
}

/**
 * @brief Log TOTPDrift.
 * @param[in] user_id Identifier of the user.
 * @param[in] step_offset Input parameter.
 * @param[in] timestamp Input parameter.
 * @details Calls: time_since_epoch(), count(), emit().
 */
void AuthAuditLogger::logTOTPDrift(const std::string &user_id, int step_offset,
                                   std::chrono::system_clock::time_point timestamp) {
    nlohmann::json details;
    details["step_offset"] = step_offset;
    details["timestamp"]   = std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();
    emit(utils::SecurityEventType::MFA_TOTP_SUCCESS, user_id, "mfa/totp/drift", details);
}

/**
 * @brief Log Recovery Code Used.
 * @param[in] user_id Identifier of the user.
 * @details Calls: emit().
 */
void AuthAuditLogger::logRecoveryCodeUsed(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_RECOVERY_CODE_USED, user_id, "mfa/recovery_code", {});
}

/**
 * @brief Log MFAEnrolled.
 * @param[in] user_id Identifier of the user.
 * @details Calls: emit().
 */
void AuthAuditLogger::logMFAEnrolled(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_ENROLLED, user_id, "mfa/enrollment", {});
}

// ---------------------------------------------------------------------------
// API Key events
// ---------------------------------------------------------------------------

/**
 * @brief Log Api Key Success.
 * @param[in] key_id Identifier of the key.
 * @param[in] principal Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logApiKeySuccess(const std::string &key_id, const std::string &principal) {
    nlohmann::json d;
    d["key_id"] = key_id;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, principal, "api_key/" + key_id, d);
}

/**
 * @brief Log Api Key Failure.
 * @param[in] key_id Identifier of the key.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logApiKeyFailure(const std::string &key_id, const std::string &reason) {
    nlohmann::json d;
    d["key_id"] = key_id;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "api_key/" + key_id, d);
}

// ---------------------------------------------------------------------------
// OAuth / SAML events
// ---------------------------------------------------------------------------

/**
 * @brief Log OAuth Device Granted.
 * @param[in] client_id Identifier of the client.
 * @param[in] sub Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logOAuthDeviceGranted(const std::string &client_id, const std::string &sub) {
    nlohmann::json d;
    d["client_id"] = client_id;
    emit(utils::SecurityEventType::TOKEN_CREATED, sub, "oauth/device/" + client_id, d);
}

/**
 * @brief Log OAuth Device Denied.
 * @param[in] client_id Identifier of the client.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logOAuthDeviceDenied(const std::string &client_id, const std::string &reason) {
    nlohmann::json d;
    d["client_id"] = client_id;
    d["reason"]    = reason;
    emit(utils::SecurityEventType::UNAUTHORIZED_ACCESS, "", "oauth/device/" + client_id, d);
}

/**
 * @brief Log SAMLSuccess.
 * @param[in] subject Input parameter.
 * @param[in] issuer Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logSAMLSuccess(const std::string &subject, const std::string &issuer) {
    nlohmann::json d;
    d["issuer"] = issuer;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, subject, "saml/assertion", d);
}

/**
 * @brief Log SAMLFailure.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logSAMLFailure(const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "saml/assertion", d);
}

// ---------------------------------------------------------------------------
// Passkey / FIDO2 events
// ---------------------------------------------------------------------------

/**
 * @brief Log Passkey Success.
 * @param[in] user_id Identifier of the user.
 * @param[in] credential_id Identifier of the credential.
 * @details Calls: emit().
 */
void AuthAuditLogger::logPasskeySuccess(const std::string &user_id, const std::string &credential_id) {
    nlohmann::json d;
    d["credential_id"] = credential_id;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, user_id, "passkey/authenticate", d);
}

/**
 * @brief Log Passkey Failure.
 * @param[in] user_id Identifier of the user.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logPasskeyFailure(const std::string &user_id, const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, user_id, "passkey/authenticate", d);
}

/**
 * @brief Log Passkey Registered.
 * @param[in] user_id Identifier of the user.
 * @param[in] credential_id Identifier of the credential.
 * @param[in] rp_id Identifier of the rp.
 * @details Calls: emit().
 */
void AuthAuditLogger::logPasskeyRegistered(const std::string &user_id,
                                           const std::string &credential_id,
                                           const std::string &rp_id) {
    nlohmann::json d;
    d["credential_id"] = credential_id;
    d["rp_id"]         = rp_id;
    emit(utils::SecurityEventType::TOKEN_CREATED, user_id, "passkey/register", d);
}

// ---------------------------------------------------------------------------
// mTLS events
// ---------------------------------------------------------------------------

/**
 * @brief Log MTLSSuccess.
 * @param[in] principal Input parameter.
 * @param[in] serial Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logMTLSSuccess(const std::string &principal, const std::string &serial) {
    nlohmann::json d;
    d["serial"] = serial;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, principal, "mtls/authenticate", d);
}

/**
 * @brief Log MTLSFailure.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logMTLSFailure(const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "mtls/authenticate", d);
}

// ---------------------------------------------------------------------------
// Role / permission change events
// ---------------------------------------------------------------------------

/**
 * @brief Log Role Change.
 * @param[in] user_id Identifier of the user.
 * @param[in] old_role Input parameter.
 * @param[in] new_role Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logRoleChange(const std::string &user_id,
                                    const std::string &old_role,
                                    const std::string &new_role) {
    nlohmann::json d;
    d["old_role"] = old_role;
    d["new_role"] = new_role;
    emit(utils::SecurityEventType::ROLE_CHANGED, user_id, "auth/role", d);
}

/**
 * @brief Log Permission Change.
 * @param[in] user_id Identifier of the user.
 * @param[in] permission Input parameter.
 * @param[in] granted Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logPermissionChange(const std::string &user_id,
                                          const std::string &permission,
                                          bool granted) {
    nlohmann::json d;
    d["permission"] = permission;
    d["granted"]    = granted;
    emit(utils::SecurityEventType::PERMISSION_CHANGED, user_id, "auth/permission", d);
}

// ---------------------------------------------------------------------------
// LDAP / Active Directory events
// ---------------------------------------------------------------------------

/**
 * @brief Log LDAPSuccess.
 * @param[in] username Input parameter.
 * @param[in] dn Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logLDAPSuccess(const std::string &username, const std::string &dn) {
    nlohmann::json d;
    d["dn"] = dn;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, username, "ldap/bind", d);
}

/**
 * @brief Log LDAPFailure.
 * @param[in] username Input parameter.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logLDAPFailure(const std::string &username, const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, username, "ldap/bind", d);
}

/**
 * @brief --------------------------------------------------------------------------- Zero-trust continuous verification events ---------------------------------------------------------------------------
 * @param[in] user_id Identifier of the user.
 * @param[in] resource Input parameter.
 * @param[in] trust_score Input parameter.
 * @param[in] request_id Identifier of the request.
 * @details Calls: empty(), emit().
 */

void AuthAuditLogger::logZeroTrustAllowed(const std::string &user_id, const std::string &resource, double trust_score,
                                          const std::string &request_id) {
    nlohmann::json d;
    d["trust_score"] = trust_score;
    if (!request_id.empty()) {
        d["request_id"] = request_id;
    }
    emit(utils::SecurityEventType::LOGIN_SUCCESS, user_id, "zero_trust/" + resource, d);
}

/**
 * @brief Log Zero Trust Denied.
 * @param[in] user_id Identifier of the user.
 * @param[in] resource Input parameter.
 * @param[in] reason Input parameter.
 * @param[in] request_id Identifier of the request.
 * @details Calls: empty(), emit().
 */
void AuthAuditLogger::logZeroTrustDenied(const std::string &user_id, const std::string &resource,
                                         const std::string &reason, const std::string &request_id) {
    nlohmann::json d;
    d["reason"] = reason;
    if (!request_id.empty()) {
        d["request_id"] = request_id;
    }
    emit(utils::SecurityEventType::UNAUTHORIZED_ACCESS, user_id, "zero_trust/" + resource, d);
}

/**
 * @brief Log Zero Trust Re Evaluation Failed.
 * @param[in] user_id Identifier of the user.
 * @param[in] session_id Identifier of the session.
 * @param[in] reason Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logZeroTrustReEvaluationFailed(const std::string &user_id, const std::string &session_id,
                                                     const std::string &reason) {
    nlohmann::json d;
    d["reason"]     = reason;
    d["session_id"] = session_id;
    emit(utils::SecurityEventType::TOKEN_REVOKED, user_id, "zero_trust/re_evaluation_failed", d);
}

/**
 * @brief --------------------------------------------------------------------------- Anomaly detection events (brute-force, credential stuffing) ---------------------------------------------------------------------------
 * @param[in] user_id Identifier of the user.
 * @param[in] ip Input parameter.
 * @param[in] failed_attempts Input parameter.
 * @details Calls: emit().
 */

void AuthAuditLogger::logBruteForceDetected(const std::string &user_id, const std::string &ip, size_t failed_attempts) {
    nlohmann::json d;
    d["ip"]              = ip;
    d["failed_attempts"] = failed_attempts;
    emit(utils::SecurityEventType::BRUTE_FORCE_DETECTED, user_id, "auth/brute_force", d);
}

/**
 * @brief Log Credential Stuffing Suspected.
 * @param[in] ip Input parameter.
 * @param[in] distinct_users Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logCredentialStuffingSuspected(const std::string &ip, size_t distinct_users) {
    nlohmann::json d;
    d["ip"]             = ip;
    d["distinct_users"] = distinct_users;
    emit(utils::SecurityEventType::SUSPICIOUS_ACTIVITY, "", "auth/credential_stuffing", d);
}

/**
 * @brief Log Account Lockout Triggered.
 * @param[in] user_id Identifier of the user.
 * @param[in] ip Input parameter.
 * @details Calls: emit().
 */
void AuthAuditLogger::logAccountLockoutTriggered(const std::string &user_id, const std::string &ip) {
    nlohmann::json d;
    d["ip"] = ip;
    emit(utils::SecurityEventType::BRUTE_FORCE_DETECTED, user_id, "auth/account_lockout", d);
}

} // namespace auth
} // namespace themis
