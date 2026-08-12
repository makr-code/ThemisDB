/**
 * @file auth_error.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    auto applyMask = [](const std::string& source,
                        const std::regex& pattern,
                        const auto& masker) {
        std::string output;
        output.reserve(source.size());

        std::sregex_iterator it(source.begin(), source.end(), pattern);
        std::sregex_iterator end;
        std::size_t last = 0;
        for (; it != end; ++it) {
            const auto& match = *it;
            output.append(source, last, static_cast<std::size_t>(match.position()) - last);
            output.append(masker(match));
            last = static_cast<std::size_t>(match.position() + match.length());
        }
        output.append(source, last, std::string::npos);
        return output;
    };
    
    // Mask email addresses (preserve domain)
    result = applyMask(
        result,
        std::regex(R"(([a-zA-Z0-9._%+-]+)@([a-zA-Z0-9.-]+\.[a-zA-Z]{2,}))"),
        [](const std::smatch& m) { return maskEmail(m.str(0)); });
    
    // Mask Kerberos principals (principal@REALM)
    result = applyMask(
        result,
        std::regex(R"(([a-zA-Z0-9._-]+)@([A-Z0-9.-]+))"),
        [](const std::smatch& m) { return std::string("***@") + m.str(2); });
    
    // Mask file paths
    result = applyMask(
        result,
        std::regex(R"((/[a-zA-Z0-9._/-]+))"),
        [](const std::smatch& m) { return maskFilePath(m.str(0)); });
    
    // Mask IP addresses
    result = applyMask(
        result,
        std::regex(R"(\b(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})\b)"),
        [](const std::smatch& m) { return m.str(1) + ".*.*.*"; });
    
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
    static std::random_device local_rd;
    static std::mt19937 local_gen(local_rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "auth-";
    
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(local_gen);
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

    // SAML 2.0 errors
    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_INVALID_RESPONSE),
        "Authentication",
        "Error",
        "Invalid SAML response",
        "SAMLResponse could not be decoded or parsed",
        "Check IdP configuration and certificate",
        {},
        {"saml", "response", "parse"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_INVALID_SIGNATURE),
        "Authentication",
        "Error",
        "Invalid SAML signature",
        "XML signature verification on SAMLResponse or Assertion failed",
        "Verify IdP certificate is correctly configured",
        {},
        {"saml", "signature", "xml"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_CONDITIONS_FAILED),
        "Authentication",
        "Error",
        "SAML assertion conditions not met",
        "NotBefore, NotOnOrAfter, or AudienceRestriction condition failed",
        "Check server clock synchronization and SP entity ID configuration",
        {},
        {"saml", "conditions", "assertion"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_MISSING_ASSERTION),
        "Authentication",
        "Error",
        "SAML assertion missing",
        "No Assertion element found in SAMLResponse",
        "Check IdP configuration and response format",
        {},
        {"saml", "assertion", "missing"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_REPLAY_DETECTED),
        "Authentication",
        "Error",
        "Authentication replay detected",
        "SAML Assertion ID has already been used",
        "Do not reuse assertion tokens",
        {},
        {"saml", "replay", "security"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_DESTINATION_MISMATCH),
        "Authentication",
        "Error",
        "Invalid SAML response destination",
        "SubjectConfirmationData Recipient does not match SP ACS URL",
        "Verify SP Assertion Consumer Service URL configuration",
        {},
        {"saml", "destination", "recipient"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_STATUS_FAILURE),
        "Authentication",
        "Error",
        "Authentication failed",
        "SAML Status is not Success",
        "Check IdP logs for authentication failure reason",
        {},
        {"saml", "status", "failure"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_ISSUER_MISMATCH),
        "Authentication",
        "Error",
        "Invalid SAML issuer",
        "Assertion Issuer does not match configured IdP entity ID",
        "Verify IdP entity ID configuration",
        {},
        {"saml", "issuer", "entity-id"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::API_KEY_INVALID),
        "Authentication",
        "Error",
        "Invalid API key",
        "The presented API key identifier is unknown",
        "Verify the API key identifier is correct",
        {},
        {"api-key", "authentication"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::API_KEY_SECRET_MISMATCH),
        "Authentication",
        "Error",
        "Invalid API key secret",
        "The presented API key secret does not match the stored credential",
        "Verify the API key secret is correct",
        {},
        {"api-key", "authentication", "secret"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::API_KEY_EXPIRED),
        "Authentication",
        "Error",
        "API key expired",
        "The API key has passed its expiry date",
        "Request a new API key from your administrator",
        {},
        {"api-key", "expiry"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::API_KEY_INACTIVE),
        "Authentication",
        "Error",
        "API key inactive",
        "The API key has been deactivated or revoked",
        "Contact your administrator to reactivate or replace the key",
        {},
        {"api-key", "revocation"}
    });

    registry.registerError({
        toErrorCode(AuthErrorCode::API_KEY_SCOPE_DENIED),
        "Authorization",
        "Error",
        "API key scope denied",
        "The API key does not have the required scope for this operation",
        "Request a key with the required scope from your administrator",
        {},
        {"api-key", "scope", "authorization"}
    });

    // Password policy errors
    registry.registerError({
        toErrorCode(AuthErrorCode::PASSWORD_POLICY_VIOLATION),
        "Authentication",
        "Error",
        "Password does not meet policy requirements",
        "The provided password violates one or more configured policy rules",
        "Choose a password that satisfies all policy requirements",
        {},
        {"password", "policy", "validation"}
    });

    // SAML assertion decryption errors
    registry.registerError({
        toErrorCode(AuthErrorCode::SAML_DECRYPTION_FAILED),
        "Authentication",
        "Error",
        "Assertion decryption failed",
        "The EncryptedAssertion could not be decrypted with the configured SP private key",
        "Verify that SAMLConfig::sp_private_key_loader returns the correct unencrypted SP "
        "private key and that the IdP is encrypting assertions with the matching SP certificate",
        {},
        {"saml", "encryption", "assertion", "decryption"}
    });

    // Provider / federation availability errors
    registry.registerError({
        toErrorCode(AuthErrorCode::PROVIDER_DEGRADED),
        "Authentication", "Error",
        "Identity provider is temporarily unavailable",
        "The backend identity provider is unreachable or returned an unexpected transient error. "
        "Access is denied until the provider is confirmed healthy (fail-closed policy).",
        "Check provider connectivity, certificates, and health endpoints. "
        "Review auth module logs for the specific network or RPC error.",
        {}, {"provider", "federation", "availability", "fail-closed"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::PROVIDER_CAPABILITY_MISMATCH),
        "Authentication", "Error",
        "Provider capability not available in current environment",
        "The identity provider requires a capability (e.g. TLS, JWKS cache) that is not "
        "available or not satisfied in the current runtime environment.",
        "Verify provider configuration against the runtime environment "
        "(TLS certificates, JWKS endpoint reachability, connection pool health).",
        {}, {"provider", "capability", "configuration"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::FEDERATION_REALM_UNAVAILABLE),
        "Authentication", "Error",
        "Federated identity realm is currently unavailable",
        "The OIDC discovery endpoint for a registered federation realm is unreachable "
        "or returned an invalid discovery document.",
        "Check realm issuer URL and network connectivity. Review JWKS cache TTL settings.",
        {}, {"federation", "realm", "oidc", "discovery"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::FEDERATION_UNKNOWN_REALM),
        "Authentication", "Error",
        "Token issuer is not a registered federation realm",
        "The iss claim in the presented token does not match any realm registered "
        "in FederatedIdentityManager.",
        "Register the issuer URL as a realm via FederatedIdentityManager::addRealm() "
        "or reject tokens from unknown issuers.",
        {}, {"federation", "realm", "issuer", "jwt"}
    });

    // Revocation backend errors
    registry.registerError({
        toErrorCode(AuthErrorCode::REVOCATION_BACKEND_UNAVAILABLE),
        "Authentication", "Error",
        "Token revocation backend is unavailable",
        "The revocation backend (in-memory, Redis, RocksDB, or distributed cluster) "
        "is unreachable. isRevoked() returns true (deny) for unconfirmed tokens.",
        "Check revocation backend health. Review DistributedTokenBlacklist replication stats. "
        "Ensure RocksDB or Redis endpoint is reachable from this node.",
        {}, {"revocation", "blacklist", "backend", "availability"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::REVOCATION_ENTRY_INVALID),
        "Authentication", "Error",
        "Revocation entry is structurally invalid",
        "A JTI or expiry value presented to the revocation backend violates size or format constraints.",
        "Validate JTI length (<= 1024 bytes) and expiry epoch. "
        "Ensure callers do not pass empty or oversized JTI strings.",
        {}, {"revocation", "jti", "validation"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::REVOCATION_CLUSTER_SYNC_FAILED),
        "Authentication", "Warning",
        "Distributed revocation cluster sync failed",
        "The distributed token blacklist failed to synchronize with one or more peer nodes. "
        "Local state may be stale; the node continues to accept local revocations.",
        "Check cluster node connectivity, TBLK/v1 RPC port reachability, and peer_rpc_timeout_ms. "
        "Review DistributedTokenBlacklist::getReplicationStats() for sync failure counts.",
        {}, {"revocation", "cluster", "sync", "distributed"}
    });

    // Policy / authorization edge errors
    registry.registerError({
        toErrorCode(AuthErrorCode::POLICY_EDGE_UNDEFINED),
        "Authorization", "Error",
        "No applicable policy rule found; access denied by default",
        "The policy engine evaluated all applicable rules and found no matching rule. "
        "The default-deny policy applies (fail-closed).",
        "Ensure all expected resource types and actions have explicit policy entries. "
        "Review authorization_policy configuration for missing rules.",
        {}, {"policy", "authorization", "default-deny", "fail-closed"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::POLICY_MISSING_REQUIRED_CLAIM),
        "Authorization", "Error",
        "Required claim absent from token",
        "The policy gate requires a claim (e.g. groups, tenant_id, clearance_level) "
        "that is not present in the validated principal.",
        "Ensure the identity provider includes the required claims in issued tokens. "
        "Check token scope and IdP claim mapping configuration.",
        {}, {"policy", "claim", "authorization", "jwt"}
    });

    // Async provider / timeout errors
    registry.registerError({
        toErrorCode(AuthErrorCode::ASYNC_PROVIDER_TIMEOUT),
        "Authentication", "Error",
        "Async provider call timed out",
        "An asynchronous identity provider call exceeded its configured timeout. "
        "The outstanding future holds this error; access is denied (fail-closed).",
        "Increase async_timeout_ms if the provider legitimately needs more time, "
        "or investigate provider latency spikes. Review AsyncHTTPAuth and LDAP pool health.",
        {}, {"async", "timeout", "provider", "fail-closed"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::ASYNC_POOL_EXHAUSTED),
        "Authentication", "Error",
        "Async worker thread pool is exhausted",
        "All async worker threads are busy; the auth request cannot be dispatched. "
        "Access is denied to avoid silent queue build-up.",
        "Increase AuthWorkerThreadPool max_threads or reduce request concurrency. "
        "Monitor pool utilisation metrics.",
        {}, {"async", "pool", "exhaustion", "capacity"}
    });
    registry.registerError({
        toErrorCode(AuthErrorCode::ASYNC_PROVIDER_EXCEPTION),
        "Authentication", "Error",
        "Async provider call raised an unexpected exception",
        "A future from an async provider propagated an unclassified exception. "
        "Access is denied (fail-closed).",
        "Review auth module logs for the underlying exception message. "
        "Ensure provider adapters wrap all exceptions as structured AuthExceptions.",
        {}, {"async", "exception", "provider"}
    });
}

} // namespace auth
} // namespace themis


