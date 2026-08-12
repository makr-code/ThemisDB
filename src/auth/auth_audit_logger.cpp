/**
 * @file auth_audit_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/auth_audit_logger.h"

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// Private helper
// ---------------------------------------------------------------------------

void AuthAuditLogger::emit(utils::SecurityEventType type, const std::string &user_id, const std::string &resource,
                           const nlohmann::json &details) {
    if (logger_) {
        logger_->logSecurityEvent(type, user_id, resource, details);
    }
}

// ---------------------------------------------------------------------------
// JWT / Token events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logJWTSuccess(const std::string &sub, const std::string &jti, const std::string &issuer,
                                    const std::string &kid) {
    nlohmann::json d;
    d["jti"]    = jti;
    d["issuer"] = issuer;
    d["kid"]    = kid;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, sub, "jwt/token", d);
}

void AuthAuditLogger::logJWTFailure(const std::string &reason, const std::string &kid) {
    nlohmann::json d;
    d["reason"] = reason;
    if (!kid.empty()) {
        d["kid"] = kid;
    }
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token", d);
}

void AuthAuditLogger::logTokenRevoked(const std::string &jti, const std::string &sub) {
    nlohmann::json d;
    d["jti"] = jti;
    emit(utils::SecurityEventType::TOKEN_REVOKED, sub, "jwt/token/" + jti, d);
}

// ---------------------------------------------------------------------------
// GSSAPI / Kerberos events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logKerberosSuccess(const std::string &principal) {
    emit(utils::SecurityEventType::LOGIN_SUCCESS, principal, "kerberos/principal", {});
}

void AuthAuditLogger::logKerberosFailure(const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "kerberos/principal", d);
}

// ---------------------------------------------------------------------------
// MFA / TOTP events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logTOTPSuccess(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_TOTP_SUCCESS, user_id, "mfa/totp", {});
}

void AuthAuditLogger::logTOTPFailure(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_TOTP_FAILED, user_id, "mfa/totp", {});
}

void AuthAuditLogger::logTOTPDrift(const std::string &user_id, int step_offset,
                                   std::chrono::system_clock::time_point timestamp) {
    nlohmann::json details;
    details["step_offset"] = step_offset;
    details["timestamp"]   = std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();
    emit(utils::SecurityEventType::MFA_TOTP_SUCCESS, user_id, "mfa/totp/drift", details);
}

void AuthAuditLogger::logRecoveryCodeUsed(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_RECOVERY_CODE_USED, user_id, "mfa/recovery_code", {});
}

void AuthAuditLogger::logMFAEnrolled(const std::string &user_id) {
    emit(utils::SecurityEventType::MFA_ENROLLED, user_id, "mfa/enrollment", {});
}

// ---------------------------------------------------------------------------
// API Key events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logApiKeySuccess(const std::string &key_id, const std::string &principal) {
    nlohmann::json d;
    d["key_id"] = key_id;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, principal, "api_key/" + key_id, d);
}

void AuthAuditLogger::logApiKeyFailure(const std::string &key_id, const std::string &reason) {
    nlohmann::json d;
    d["key_id"] = key_id;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "api_key/" + key_id, d);
}

// ---------------------------------------------------------------------------
// OAuth / SAML events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logOAuthDeviceGranted(const std::string &client_id, const std::string &sub) {
    nlohmann::json d;
    d["client_id"] = client_id;
    emit(utils::SecurityEventType::TOKEN_CREATED, sub, "oauth/device/" + client_id, d);
}

void AuthAuditLogger::logOAuthDeviceDenied(const std::string &client_id, const std::string &reason) {
    nlohmann::json d;
    d["client_id"] = client_id;
    d["reason"]    = reason;
    emit(utils::SecurityEventType::UNAUTHORIZED_ACCESS, "", "oauth/device/" + client_id, d);
}

void AuthAuditLogger::logSAMLSuccess(const std::string &subject, const std::string &issuer) {
    nlohmann::json d;
    d["issuer"] = issuer;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, subject, "saml/assertion", d);
}

void AuthAuditLogger::logSAMLFailure(const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "saml/assertion", d);
}

// ---------------------------------------------------------------------------
// LDAP / Active Directory events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logLDAPSuccess(const std::string &username, const std::string &dn) {
    nlohmann::json d;
    d["dn"] = dn;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, username, "ldap/bind", d);
}

void AuthAuditLogger::logLDAPFailure(const std::string &username, const std::string &reason) {
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, username, "ldap/bind", d);
}

// ---------------------------------------------------------------------------
// Zero-trust continuous verification events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logZeroTrustAllowed(const std::string &user_id, const std::string &resource, double trust_score,
                                          const std::string &request_id) {
    nlohmann::json d;
    d["trust_score"] = trust_score;
    if (!request_id.empty()) {
        d["request_id"] = request_id;
    }
    emit(utils::SecurityEventType::LOGIN_SUCCESS, user_id, "zero_trust/" + resource, d);
}

void AuthAuditLogger::logZeroTrustDenied(const std::string &user_id, const std::string &resource,
                                         const std::string &reason, const std::string &request_id) {
    nlohmann::json d;
    d["reason"] = reason;
    if (!request_id.empty()) {
        d["request_id"] = request_id;
    }
    emit(utils::SecurityEventType::UNAUTHORIZED_ACCESS, user_id, "zero_trust/" + resource, d);
}

void AuthAuditLogger::logZeroTrustReEvaluationFailed(const std::string &user_id, const std::string &session_id,
                                                     const std::string &reason) {
    nlohmann::json d;
    d["reason"]     = reason;
    d["session_id"] = session_id;
    emit(utils::SecurityEventType::TOKEN_REVOKED, user_id, "zero_trust/re_evaluation_failed", d);
}

// ---------------------------------------------------------------------------
// Anomaly detection events (brute-force, credential stuffing)
// ---------------------------------------------------------------------------

void AuthAuditLogger::logBruteForceDetected(const std::string &user_id, const std::string &ip, size_t failed_attempts) {
    nlohmann::json d;
    d["ip"]              = ip;
    d["failed_attempts"] = failed_attempts;
    emit(utils::SecurityEventType::BRUTE_FORCE_DETECTED, user_id, "auth/brute_force", d);
}

void AuthAuditLogger::logCredentialStuffingSuspected(const std::string &ip, size_t distinct_users) {
    nlohmann::json d;
    d["ip"]             = ip;
    d["distinct_users"] = distinct_users;
    emit(utils::SecurityEventType::SUSPICIOUS_ACTIVITY, "", "auth/credential_stuffing", d);
}

void AuthAuditLogger::logAccountLockoutTriggered(const std::string &user_id, const std::string &ip) {
    nlohmann::json d;
    d["ip"] = ip;
    emit(utils::SecurityEventType::BRUTE_FORCE_DETECTED, user_id, "auth/account_lockout", d);
}

} // namespace auth
} // namespace themis
