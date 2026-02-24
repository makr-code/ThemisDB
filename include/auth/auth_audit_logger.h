/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_audit_logger.h                                ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <optional>
#include <memory>

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

    /** Recovery code was used (single-use codes only). */
    void logRecoveryCodeUsed(const std::string& user_id);

    /** New MFA enrollment generated for user. */
    void logMFAEnrolled(const std::string& user_id);

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
