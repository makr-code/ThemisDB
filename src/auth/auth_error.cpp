/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_error.cpp                                     ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     410                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/auth_error.h"
#include "utils/logger.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>

namespace themis {
namespace auth {

// ============================================================================
// AuthError Implementation
// ============================================================================

AuthError::AuthError(
    AuthErrorCode code,
    std::string public_message,
    std::string internal_message,
    std::string request_id)
    : code_(code)
    , public_message_(std::move(public_message))
    , internal_message_(internal_message.empty() ? public_message : std::move(internal_message))
    , request_id_(request_id.empty() ? generateRequestId() : std::move(request_id))
    , timestamp_(std::chrono::system_clock::now())
{
}

nlohmann::json AuthError::toPublicJSON() const {
    nlohmann::json j;
    j["error"] = {
        {"code", static_cast<int>(code_)},
        {"message", public_message_},
        {"request_id", request_id_},
        {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
            timestamp_.time_since_epoch()).count()}
    };
    
    if (retry_after_.has_value()) {
        j["error"]["retry_after_seconds"] = retry_after_->count();
    }
    
    return j;
}

nlohmann::json AuthError::toInternalJSON() const {
    nlohmann::json j = toPublicJSON();
    j["error"]["internal_message"] = internal_message_;
    return j;
}

void AuthError::logError() const {
    utils::Logger::error(
        "[AUTH_ERROR] Code: {}, RequestID: {}, Internal: {}, Public: {}",
        static_cast<int>(code_),
        request_id_,
        internal_message_,
        public_message_
    );
}

AuthError AuthError::fromException(
    const std::exception& e,
    const std::string& request_id)
{
    std::string msg = e.what();
    
    // Try to detect error type from message
    AuthErrorCode code = AuthErrorCode::AUTH_GENERAL_FAILURE;
    
    if (msg.find("expired") != std::string::npos) {
        code = AuthErrorCode::AUTH_TOKEN_EXPIRED;
    } else if (msg.find("invalid") != std::string::npos || 
               msg.find("Invalid") != std::string::npos) {
        code = AuthErrorCode::AUTH_TOKEN_INVALID;
    } else if (msg.find("signature") != std::string::npos) {
        code = AuthErrorCode::JWT_INVALID_SIGNATURE;
    }
    
    // Mask sensitive data in public message
    std::string public_msg = maskSensitiveData(msg);
    
    return AuthError(code, public_msg, msg, request_id);
}

std::string AuthError::maskSensitiveData(const std::string& input) {
    std::string result = input;
    
    // Mask email addresses (preserve domain)
    result = std::regex_replace(result, 
        std::regex(R"(([a-zA-Z0-9._%+-]+)@([a-zA-Z0-9.-]+\.[a-zA-Z]{2,}))"),
        maskEmail("$1@$2"));
    
    // Mask Kerberos principals (principal@REALM)
    result = std::regex_replace(result,
        std::regex(R"(([a-zA-Z0-9._-]+)@([A-Z0-9.-]+))"),
        "***@$2");
    
    // Mask file paths
    result = std::regex_replace(result,
        std::regex(R"((/[a-zA-Z0-9._/-]+))"),
        maskFilePath("$1"));
    
    // Mask IP addresses
    result = std::regex_replace(result,
        std::regex(R"(\b(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})\b)"),
        "$1.*.*.*");
    
    return result;
}

std::string AuthError::maskEmail(const std::string& email) {
    size_t at_pos = email.find('@');
    if (at_pos == std::string::npos || at_pos == 0) {
        return "***";
    }
    
    // Show first 2 chars of local part and full domain
    std::string local = email.substr(0, at_pos);
    std::string domain = email.substr(at_pos);
    
    if (local.length() <= 2) {
        return "***" + domain;
    }
    
    return local.substr(0, 2) + "***" + domain;
}

std::string AuthError::maskPrincipal(const std::string& principal) {
    size_t at_pos = principal.find('@');
    if (at_pos == std::string::npos) {
        return "***";
    }
    
    // Keep realm, mask principal name
    return "***" + principal.substr(at_pos);
}

std::string AuthError::maskFilePath(const std::string& path) {
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) {
        return "***";
    }
    
    // Keep filename, mask directory structure
    return "***/" + path.substr(last_slash + 1);
}

std::string AuthError::maskIPAddress(const std::string& ip) {
    size_t first_dot = ip.find('.');
    if (first_dot == std::string::npos) {
        return "***";
    }
    
    // Keep first octet only
    return ip.substr(0, first_dot) + ".*.*.*";
}

std::string AuthError::maskToken(const std::string& token) {
    if (token.length() <= 8) {
        return "***";
    }
    
    // Show first and last 4 characters
    return token.substr(0, 4) + "..." + token.substr(token.length() - 4);
}

std::string AuthError::generateRequestId() {
    // Generate a random request ID (format: auth-XXXXXXXX)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "auth-";
    
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

// ============================================================================
// Error Registration
// ============================================================================

void registerAuthErrors() {
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // General auth errors
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_GENERAL_FAILURE),
        "Authentication",
        "Error",
        "Authentication failed",
        "General authentication failure occurred",
        "Check credentials and try again",
        {},
        {"authentication", "auth", "failure"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_INVALID_CREDENTIALS),
        "Authentication",
        "Error",
        "Invalid credentials provided",
        "Username or password is incorrect",
        "Verify credentials and try again",
        {},
        {"authentication", "credentials", "password"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_TOKEN_INVALID),
        "Authentication",
        "Error",
        "Authentication token is invalid",
        "Token format, signature, or content is invalid",
        "Obtain a new token from the identity provider",
        {},
        {"token", "jwt", "invalid"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_TOKEN_EXPIRED),
        "Authentication",
        "Error",
        "Authentication token has expired",
        "Token expiration time has passed",
        "Refresh your token or obtain a new one",
        {},
        {"token", "expired", "expiration"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_INSUFFICIENT_PERMISSIONS),
        "Authentication",
        "Error",
        "Insufficient permissions for this operation",
        "User lacks required roles or scopes",
        "Request appropriate permissions from administrator",
        {},
        {"permissions", "authorization", "rbac"}
    });
    
    // JWT errors
    registry.registerError({
        toErrorCode(AuthErrorCode::JWT_INVALID_FORMAT),
        "Authentication",
        "Error",
        "JWT token format is invalid",
        "Token does not follow JWT standard (header.payload.signature)",
        "Ensure token is properly formatted",
        {},
        {"jwt", "format", "structure"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::JWT_INVALID_SIGNATURE),
        "Authentication",
        "Error",
        "JWT signature verification failed",
        "Token signature does not match expected value",
        "Token may have been tampered with or signed with wrong key",
        {},
        {"jwt", "signature", "verification"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::JWT_TOKEN_TOO_LARGE),
        "Authentication",
        "Error",
        "JWT token exceeds maximum size",
        "Token size exceeds security limit (16KB)",
        "Reduce token claims or contact administrator",
        {},
        {"jwt", "size", "limit"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::JWT_JWKS_FETCH_FAILED),
        "Authentication",
        "Error",
        "Failed to fetch JWKS from identity provider",
        "Network error or identity provider unreachable",
        "Check network connectivity and JWKS URL",
        {},
        {"jwt", "jwks", "network"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::JWT_KID_REVOKED),
        "Authentication",
        "Error",
        "JWT signed with revoked key",
        "Key ID (kid) is on revocation list",
        "Obtain token signed with current key",
        {},
        {"jwt", "kid", "revoked"}
    });
    
    // GSSAPI errors
    registry.registerError({
        toErrorCode(AuthErrorCode::GSSAPI_INITIALIZATION_FAILED),
        "Authentication",
        "Error",
        "Kerberos authentication initialization failed",
        "GSSAPI context initialization error",
        "Check Kerberos configuration and keytab file",
        {},
        {"kerberos", "gssapi", "initialization"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::GSSAPI_KEYTAB_NOT_FOUND),
        "Authentication",
        "Error",
        "Kerberos keytab file not found",
        "Specified keytab file does not exist",
        "Verify keytab file path in configuration",
        {},
        {"kerberos", "keytab", "file"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::GSSAPI_KDC_UNREACHABLE),
        "Authentication",
        "Error",
        "Kerberos KDC unreachable",
        "Cannot connect to Key Distribution Center",
        "Check network connectivity and KDC configuration",
        {},
        {"kerberos", "kdc", "network"}
    });
    
    // MFA errors
    registry.registerError({
        toErrorCode(AuthErrorCode::MFA_CODE_INVALID),
        "Authentication",
        "Error",
        "MFA code is invalid",
        "TOTP code does not match expected value",
        "Verify code and try again",
        {},
        {"mfa", "totp", "invalid"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::MFA_NOT_ENROLLED),
        "Authentication",
        "Error",
        "MFA not enrolled for this account",
        "User has not completed MFA enrollment",
        "Complete MFA enrollment process",
        {},
        {"mfa", "enrollment", "totp"}
    });
    
    // Rate limiting errors
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_RATE_LIMIT_EXCEEDED),
        "Authentication",
        "Warning",
        "Too many authentication attempts",
        "Rate limit exceeded for IP or user",
        "Wait before retrying",
        {},
        {"rate-limit", "throttle", "429"}
    });
    
    registry.registerError({
        toErrorCode(AuthErrorCode::AUTH_ACCOUNT_LOCKED),
        "Authentication",
        "Error",
        "Account is temporarily locked",
        "Too many failed authentication attempts",
        "Wait for lockout period to expire or contact administrator",
        {},
        {"lockout", "brute-force", "security"}
    });
}

} // namespace auth
} // namespace themis
