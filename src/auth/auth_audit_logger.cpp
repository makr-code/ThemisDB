/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_audit_logger.cpp                              ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/auth_audit_logger.h"

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// Private helper
// ---------------------------------------------------------------------------

void AuthAuditLogger::emit(utils::SecurityEventType type,
                           const std::string& user_id,
                           const std::string& resource,
                           const nlohmann::json& details)
{
    if (logger_) {
        logger_->logSecurityEvent(type, user_id, resource, details);
    }
}

// ---------------------------------------------------------------------------
// JWT / Token events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logJWTSuccess(const std::string& sub,
                                    const std::string& jti,
                                    const std::string& issuer,
                                    const std::string& kid)
{
    nlohmann::json d;
    d["jti"]    = jti;
    d["issuer"] = issuer;
    d["kid"]    = kid;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, sub, "jwt/token", d);
}

void AuthAuditLogger::logJWTFailure(const std::string& reason,
                                    const std::string& kid)
{
    nlohmann::json d;
    d["reason"] = reason;
    if (!kid.empty()) d["kid"] = kid;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token", d);
}

void AuthAuditLogger::logTokenRevoked(const std::string& jti,
                                      const std::string& sub)
{
    nlohmann::json d;
    d["jti"] = jti;
    emit(utils::SecurityEventType::TOKEN_REVOKED, sub, "jwt/token/" + jti, d);
}

// ---------------------------------------------------------------------------
// GSSAPI / Kerberos events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logKerberosSuccess(const std::string& principal)
{
    emit(utils::SecurityEventType::LOGIN_SUCCESS, principal, "kerberos/principal", {});
}

void AuthAuditLogger::logKerberosFailure(const std::string& reason)
{
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "kerberos/principal", d);
}

// ---------------------------------------------------------------------------
// MFA / TOTP events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logTOTPSuccess(const std::string& user_id)
{
    emit(utils::SecurityEventType::MFA_TOTP_SUCCESS, user_id, "mfa/totp", {});
}

void AuthAuditLogger::logTOTPFailure(const std::string& user_id)
{
    emit(utils::SecurityEventType::MFA_TOTP_FAILED, user_id, "mfa/totp", {});
}

void AuthAuditLogger::logRecoveryCodeUsed(const std::string& user_id)
{
    emit(utils::SecurityEventType::MFA_RECOVERY_CODE_USED, user_id, "mfa/recovery_code", {});
}

void AuthAuditLogger::logMFAEnrolled(const std::string& user_id)
{
    emit(utils::SecurityEventType::MFA_ENROLLED, user_id, "mfa/enrollment", {});
}

// ---------------------------------------------------------------------------
// OAuth / SAML events
// ---------------------------------------------------------------------------

void AuthAuditLogger::logOAuthDeviceGranted(const std::string& client_id,
                                            const std::string& sub)
{
    nlohmann::json d;
    d["client_id"] = client_id;
    emit(utils::SecurityEventType::TOKEN_CREATED, sub, "oauth/device/" + client_id, d);
}

void AuthAuditLogger::logOAuthDeviceDenied(const std::string& client_id,
                                           const std::string& reason)
{
    nlohmann::json d;
    d["client_id"] = client_id;
    d["reason"]    = reason;
    emit(utils::SecurityEventType::UNAUTHORIZED_ACCESS, "", "oauth/device/" + client_id, d);
}

void AuthAuditLogger::logSAMLSuccess(const std::string& subject,
                                     const std::string& issuer)
{
    nlohmann::json d;
    d["issuer"] = issuer;
    emit(utils::SecurityEventType::LOGIN_SUCCESS, subject, "saml/assertion", d);
}

void AuthAuditLogger::logSAMLFailure(const std::string& reason)
{
    nlohmann::json d;
    d["reason"] = reason;
    emit(utils::SecurityEventType::LOGIN_FAILED, "", "saml/assertion", d);
}

} // namespace auth
} // namespace themis
