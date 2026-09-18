/**
 * @file auth_error.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/error_registry.h"
#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace auth {

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
    SAML_DECRYPTION_FAILED = 9411,

    // Provider / federation availability errors (9420-9429)
    PROVIDER_DEGRADED = 9420,
    PROVIDER_CAPABILITY_MISMATCH = 9421,
    FEDERATION_REALM_UNAVAILABLE = 9422,
    FEDERATION_UNKNOWN_REALM = 9423,

    // Revocation backend errors (9430-9439)
    REVOCATION_BACKEND_UNAVAILABLE = 9430,
    REVOCATION_ENTRY_INVALID = 9431,
    REVOCATION_CLUSTER_SYNC_FAILED = 9432,

    // Policy / authorization edge errors (9440-9449)
    POLICY_EDGE_UNDEFINED = 9440,
    POLICY_MISSING_REQUIRED_CLAIM = 9441,

    // Async provider / timeout errors (9450-9459)
    ASYNC_PROVIDER_TIMEOUT = 9450,
    ASYNC_POOL_EXHAUSTED = 9451,
    ASYNC_PROVIDER_EXCEPTION = 9452
};

/**
 * @brief To Error Code.
 * @param[in] code Input parameter.
 * @return Return value.
 * @details Implements toErrorCode without additional internal calls.
 */
inline errors::ErrorCode toErrorCode(AuthErrorCode code) {
    return static_cast<errors::ErrorCode>(static_cast<int>(code));
}

class AuthError {
public:
    AuthError(
        AuthErrorCode code,
        std::string public_message,
        std::string internal_message = "",
        std::string request_id = ""
    );
    
    AuthErrorCode code() const { return code_; }
    
    const std::string& publicMessage() const { return public_message_; }
    
    const std::string& internalMessage() const { return internal_message_; }
    
    const std::string& requestId() const { return request_id_; }
    
    std::chrono::system_clock::time_point timestamp() const { return timestamp_; }
    
    /**
     * @brief Set Retry After.
     * @param[in] duration Input parameter.
     * @details Implements setRetryAfter without additional internal calls.
     */
    void setRetryAfter(std::chrono::seconds duration) {
        retry_after_ = duration;
    }
    
    std::optional<std::chrono::seconds> retryAfter() const {
        return retry_after_;
    }
    
    /**
     * @brief To Public JSON.
     * @return Return value.
     */
    nlohmann::json toPublicJSON() const;
    
    /**
     * @brief To Internal JSON.
     * @return Return value.
     */
    nlohmann::json toInternalJSON() const;
    
    /**
     * @brief Log Error.
     */
    void logError() const;
    
    static AuthError fromException(
        const std::exception& e,
        const std::string& request_id = ""
    );
    
    /**
     * @brief Mask Sensitive Data.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    static std::string maskSensitiveData(const std::string& input);

private:
    AuthErrorCode code_;
    std::string public_message_;
    std::string internal_message_;
    std::string request_id_;
    std::chrono::system_clock::time_point timestamp_;
    std::optional<std::chrono::seconds> retry_after_;
    
    /**
     * @brief Generate Request Id.
     * @return Return value.
     */
    static std::string generateRequestId();
    /**
     * @brief Mask Email.
     * @param[in] email Input parameter.
     * @return Return value.
     */
    static std::string maskEmail(const std::string& email);
    /**
     * @brief Mask Principal.
     * @param[in] principal Input parameter.
     * @return Return value.
     */
    static std::string maskPrincipal(const std::string& principal);
    /**
     * @brief Mask File Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string maskFilePath(const std::string& path);
    /**
     * @brief Mask IPAddress.
     * @param[in] ip Input parameter.
     * @return Return value.
     */
    static std::string maskIPAddress(const std::string& ip);
    /**
     * @brief Mask Token.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    static std::string maskToken(const std::string& token);
};

class AuthException : public std::runtime_error {
public:
    /**
     * @brief Auth Exception.
     * @param[in] error Input parameter.
     * @return Return value.
     */
    explicit AuthException(const AuthError& error)
        : std::runtime_error(error.publicMessage())
        , error_(error)
    {}
    
    const AuthError& error() const { return error_; }
    
private:
    AuthError error_;
};

/**
 * @brief Register Auth Errors.
 */
void registerAuthErrors();

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

