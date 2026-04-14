/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_error.h                                       ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:23:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     319                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b428004b92  2026-03-12  feat(auth): implement SAML assertion encryption support (... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 33a346e4e2  2026-02-25  Refactor code structure and remove redundant code blocks ... ║
    • 79129146fd  2026-02-24  feat(auth): implement LDAP/Active Directory direct bind a... ║
    • e18972c328  2026-02-24  fix(auth): resolve PASSWORD_POLICY_VIOLATION enum collisi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/error_registry.h"
#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace auth {

/**
 * @brief Authentication error codes (range: 9300-9399)
 * 
 * These error codes are registered in the global ErrorRegistry
 * for consistent error handling across the system.
 */
enum class AuthErrorCode {
    // General auth errors (9300-9309)
    AUTH_GENERAL_FAILURE = 9300,
    AUTH_INVALID_CREDENTIALS = 9301,
    AUTH_TOKEN_INVALID = 9302,
    AUTH_TOKEN_EXPIRED = 9303,
    AUTH_TOKEN_NOT_YET_VALID = 9304,
    AUTH_INSUFFICIENT_PERMISSIONS = 9305,
    
    // JWT errors (9310-9329)
    JWT_INVALID_FORMAT = 9310,
    JWT_INVALID_SIGNATURE = 9311,
    JWT_ISSUER_MISMATCH = 9312,
    JWT_AUDIENCE_MISMATCH = 9313,
    JWT_TOKEN_TOO_LARGE = 9314,
    JWT_PRINCIPAL_TOO_LONG = 9315,
    JWT_JWKS_FETCH_FAILED = 9316,
    JWT_JWKS_INVALID = 9317,
    JWT_KID_REVOKED = 9318,
    JWT_UNSUPPORTED_ALGORITHM = 9319,
    JWT_MISSING_REQUIRED_CLAIM = 9320,
    
    // GSSAPI/Kerberos errors (9330-9349)
    GSSAPI_INITIALIZATION_FAILED = 9330,
    GSSAPI_TOKEN_INVALID = 9331,
    GSSAPI_TOKEN_TOO_LARGE = 9332,
    GSSAPI_PRINCIPAL_TOO_LONG = 9333,
    GSSAPI_SERVICE_PRINCIPAL_INVALID = 9334,
    GSSAPI_KEYTAB_NOT_FOUND = 9335,
    GSSAPI_KEYTAB_INVALID = 9336,
    GSSAPI_CONTEXT_EXPIRED = 9337,
    GSSAPI_KDC_UNREACHABLE = 9338,
    GSSAPI_CLOCK_SKEW = 9339,
    
    // MFA errors (9350-9369)
    MFA_CODE_INVALID = 9350,
    MFA_CODE_EXPIRED = 9351,
    MFA_SECRET_INVALID = 9352,
    MFA_NOT_ENROLLED = 9353,
    MFA_ALREADY_ENROLLED = 9354,
    MFA_RECOVERY_CODE_INVALID = 9355,
    MFA_RECOVERY_CODE_EXHAUSTED = 9356,
    
    // Rate limiting errors (9370-9379)
    AUTH_RATE_LIMIT_EXCEEDED = 9370,
    AUTH_ACCOUNT_LOCKED = 9371,
    AUTH_IP_BLOCKED = 9372,
    AUTH_TOO_MANY_FAILURES = 9373,
    
    // Configuration errors (9380-9389)
    AUTH_CONFIG_INVALID = 9380,
    AUTH_CONFIG_MISSING = 9381,
    
    // SAML 2.0 errors (9382-9389)
    SAML_INVALID_RESPONSE = 9382,
    SAML_INVALID_SIGNATURE = 9383,
    SAML_CONDITIONS_FAILED = 9384,
    SAML_MISSING_ASSERTION = 9385,
    SAML_REPLAY_DETECTED = 9386,
    SAML_DESTINATION_MISMATCH = 9387,
    SAML_STATUS_FAILURE = 9388,
    SAML_ISSUER_MISMATCH = 9389,

    // Internal errors (9390-9391)
    AUTH_INTERNAL_ERROR = 9390,
    AUTH_NOT_IMPLEMENTED = 9391,

    // API key errors (9392-9396)
    API_KEY_INVALID = 9392,
    API_KEY_SECRET_MISMATCH = 9393,
    API_KEY_EXPIRED = 9394,
    API_KEY_INACTIVE = 9395,
    API_KEY_SCOPE_DENIED = 9396,

    // mTLS / certificate errors (9397-9399)
    MTLS_CERT_INVALID = 9397,
    MTLS_CERT_EXPIRED = 9398,
    MTLS_CERT_REVOKED = 9399,

    // LDAP / Active Directory errors (9400-9409)
    LDAP_CONNECTION_FAILED = 9400,
    LDAP_BIND_FAILED = 9401,
    LDAP_INVALID_CREDENTIALS = 9402,
    LDAP_SERVER_UNREACHABLE = 9403,
    LDAP_SEARCH_FAILED = 9404,
    LDAP_USER_NOT_FOUND = 9405,
    LDAP_TLS_FAILED = 9406,
    LDAP_NOT_INITIALIZED = 9407,
    LDAP_OPERATION_TIMEOUT = 9408,
    LDAP_INVALID_DN = 9409,

    // Password policy errors (9410-9419)
    PASSWORD_POLICY_VIOLATION = 9410,

    // SAML assertion decryption errors (9411)
    SAML_DECRYPTION_FAILED = 9411
};

/**
 * @brief Convert AuthErrorCode to ErrorRegistry ErrorCode
 */
inline errors::ErrorCode toErrorCode(AuthErrorCode code) {
    return static_cast<errors::ErrorCode>(static_cast<int>(code));
}

/**
 * @brief Authentication error with request tracking and sensitive data masking
 * 
 * This class provides a structured error response for authentication failures
 * that masks sensitive information (principals, file paths, internal state)
 * while logging full details server-side for debugging.
 * 
 * Features:
 * - Request ID for tracing across distributed systems
 * - Timestamp for audit trail
 * - Masked public message (safe for clients)
 * - Full internal message (logged server-side only)
 * - Error code for programmatic handling
 * - Optional retry-after for rate limiting
 */
class AuthError {
public:
    /**
     * @brief Create an authentication error
     * @param code Error code
     * @param public_message Safe message for clients (sensitive data masked)
     * @param internal_message Full message for server logs (may contain sensitive data)
     * @param request_id Optional request ID for tracing
     */
    AuthError(
        AuthErrorCode code,
        std::string public_message,
        std::string internal_message = "",
        std::string request_id = ""
    );
    
    /**
     * @brief Get error code
     */
    AuthErrorCode code() const { return code_; }
    
    /**
     * @brief Get public message (safe for clients, sensitive data masked)
     */
    const std::string& publicMessage() const { return public_message_; }
    
    /**
     * @brief Get internal message (full details, for server logs only)
     */
    const std::string& internalMessage() const { return internal_message_; }
    
    /**
     * @brief Get request ID
     */
    const std::string& requestId() const { return request_id_; }
    
    /**
     * @brief Get timestamp
     */
    std::chrono::system_clock::time_point timestamp() const { return timestamp_; }
    
    /**
     * @brief Set retry-after duration (for rate limiting)
     */
    void setRetryAfter(std::chrono::seconds duration) {
        retry_after_ = duration;
    }
    
    /**
     * @brief Get retry-after duration
     */
    std::optional<std::chrono::seconds> retryAfter() const {
        return retry_after_;
    }
    
    /**
     * @brief Convert to JSON (public version for API responses)
     */
    nlohmann::json toPublicJSON() const;
    
    /**
     * @brief Convert to JSON (internal version for logging)
     */
    nlohmann::json toInternalJSON() const;
    
    /**
     * @brief Log this error to server logs with full details
     */
    void logError() const;
    
    /**
     * @brief Create error from exception
     */
    static AuthError fromException(
        const std::exception& e,
        const std::string& request_id = ""
    );
    
    /**
     * @brief Mask sensitive data in a string
     * 
     * Masks:
     * - Email addresses (preserves domain)
     * - Kerberos principals
     * - File paths (preserves filename)
     * - IP addresses (preserves first octet)
     * - UUIDs and tokens (shows first/last 4 chars)
     */
    static std::string maskSensitiveData(const std::string& input);

private:
    AuthErrorCode code_;
    std::string public_message_;
    std::string internal_message_;
    std::string request_id_;
    std::chrono::system_clock::time_point timestamp_;
    std::optional<std::chrono::seconds> retry_after_;
    
    static std::string generateRequestId();
    static std::string maskEmail(const std::string& email);
    static std::string maskPrincipal(const std::string& principal);
    static std::string maskFilePath(const std::string& path);
    static std::string maskIPAddress(const std::string& ip);
    static std::string maskToken(const std::string& token);
};

/**
 * @brief Exception class for authentication errors
 * 
 * Can be thrown and caught to propagate structured auth errors
 */
class AuthException : public std::runtime_error {
public:
    explicit AuthException(const AuthError& error)
        : std::runtime_error(error.publicMessage())
        , error_(error)
    {}
    
    const AuthError& error() const { return error_; }
    
private:
    AuthError error_;
};

/**
 * @brief Register all authentication error codes in the global ErrorRegistry
 * 
 * This should be called during system initialization to ensure all
 * auth error codes are properly registered with metadata.
 */
void registerAuthErrors();

/**
 * @brief Helper macros for throwing auth errors
 */
#define THROW_AUTH_ERROR(code, public_msg, internal_msg) \
    throw themis::auth::AuthException( \
        themis::auth::AuthError( \
            code, \
            public_msg, \
            internal_msg \
        ) \
    )

#define THROW_AUTH_ERROR_WITH_ID(code, public_msg, internal_msg, req_id) \
    throw themis::auth::AuthException( \
        themis::auth::AuthError( \
            code, \
            public_msg, \
            internal_msg, \
            req_id \
        ) \
    )

} // namespace auth
} // namespace themis
