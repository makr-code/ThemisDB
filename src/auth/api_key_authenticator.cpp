/**
 * @file api_key_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/api_key_authenticator.h"
#include "auth/auth_audit_logger.h"
#include "utils/audit_logger.h"

#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <spdlog/spdlog.h>

#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace themis {
namespace auth {

// ============================================================================
// Construction
// ============================================================================

ApiKeyAuthenticator::ApiKeyAuthenticator(const Config& config)
    : config_(config)
{}

// ============================================================================
// Credential management
// ============================================================================

void ApiKeyAuthenticator::addCredential(const ApiKeyCredential& credential) {
    if (credential.key_id.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "API key credential error",
            "key_id must not be empty"
        ));
    }
    // A valid SHA-256 hex digest is exactly 64 lower-case hex characters.
    if (static_cast<int>(credential.secret_hash.size()) != 64) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "API key credential error",
            "secret_hash must be a 64-character SHA-256 hex digest"
        ));
    }

    std::lock_guard<std::mutex> lock(mutex_);
    credentials_[credential.key_id] = credential;
    spdlog::debug("ApiKeyAuthenticator: credential added for key_id='{}'",
                  credential.key_id);
}

void ApiKeyAuthenticator::removeCredential(const std::string& key_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (credentials_.erase(key_id) > 0) {
        spdlog::debug("ApiKeyAuthenticator: credential removed for key_id='{}'", key_id);
    }
}

size_t ApiKeyAuthenticator::credentialCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(credentials_.size());
}

// ============================================================================
// Authentication
// ============================================================================

ApiKeyClaims ApiKeyAuthenticator::authenticate(const std::string& key_id,
                                                const std::string& secret)
{
    // --- Input validation ---------------------------------------------------
    if (key_id.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "key_id must not be empty"
        ));
    }
    if (static_cast<int>(key_id.size()) > config_.max_key_id_length) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "key_id exceeds maximum allowed length"
        ));
    }
    if (secret.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "secret must not be empty"
        ));
    }
    if (static_cast<int>(secret.size()) > config_.max_secret_length) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "secret exceeds maximum allowed length"
        ));
    }

    // --- Look up credential -------------------------------------------------
    ApiKeyCredential cred;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = credentials_.find(key_id);
        if (it == credentials_.end()) {
            spdlog::warn("ApiKeyAuthenticator: unknown key_id='{}'", key_id);
            if (audit_logger_) {
                AuthAuditLogger al(audit_logger_);
                al.logApiKeyFailure(key_id, "key_id_not_found");
            }
            throw AuthException(AuthError(
                AuthErrorCode::API_KEY_INVALID,
                "Authentication failed",
                "key_id not found: " + key_id
            ));
        }
        cred = it->second;
    }

    // --- Check active flag --------------------------------------------------
    if (!cred.active) {
        spdlog::warn("ApiKeyAuthenticator: inactive key_id='{}'", key_id);
        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logApiKeyFailure(key_id, "key_inactive");
        }
        throw AuthException(AuthError(
            AuthErrorCode::API_KEY_INACTIVE,
            "Authentication failed",
            "API key is inactive: " + key_id
        ));
    }

    // --- Check expiry -------------------------------------------------------
    if (config_.check_expiry) {
        static const std::chrono::system_clock::time_point epoch{};
        if (cred.expires_at != epoch &&
            std::chrono::system_clock::now() > cred.expires_at)
        {
            spdlog::warn("ApiKeyAuthenticator: expired key_id='{}'", key_id);
            if (audit_logger_) {
                AuthAuditLogger al(audit_logger_);
                al.logApiKeyFailure(key_id, "key_expired");
            }
            throw AuthException(AuthError(
                AuthErrorCode::API_KEY_EXPIRED,
                "Authentication failed",
                "API key has expired: " + key_id
            ));
        }
    }

    // --- Verify secret (constant-time comparison) ---------------------------
    std::string presented_hash = {};
    try {
        presented_hash = hashSecret(secret);
    } catch (const std::exception& ex) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INTERNAL_ERROR,
            "Authentication failed",
            std::string("Failed to hash presented secret: ") + ex.what()
        ));
    }

    if (!constantTimeEqual(presented_hash, cred.secret_hash)) {
        spdlog::warn("ApiKeyAuthenticator: secret mismatch for key_id='{}'", key_id); // NOPII: key_id is the public key identifier; "secret" here names the credential type, not a secret value
        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logApiKeyFailure(key_id, "secret_mismatch");
        }
        throw AuthException(AuthError(
            AuthErrorCode::API_KEY_SECRET_MISMATCH,
            "Authentication failed",
            "Secret mismatch for key_id: " + key_id
        ));
    }

    spdlog::info("ApiKeyAuthenticator: authenticated key_id='{}' principal='{}'",
                 key_id, cred.principal);
    if (audit_logger_) {
        AuthAuditLogger al(audit_logger_);
        al.logApiKeySuccess(key_id, cred.principal);
    }
    return claimsFromCredential(cred);
}

ApiKeyClaims ApiKeyAuthenticator::authenticateCombined(const std::string& combined) {
    const auto dot = combined.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 >= combined.size()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INVALID_CREDENTIALS,
            "Authentication failed",
            "Combined API key must be in '<key_id>.<secret>' format"
        ));
    }
    return authenticate(combined.substr(0, dot), combined.substr(dot + 1));
}

// ============================================================================
// Static helpers
// ============================================================================

std::string ApiKeyAuthenticator::hashSecret(const std::string& secret) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    if (SHA256(reinterpret_cast<const unsigned char*>(secret.data()),
               secret.size(), digest) == nullptr) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INTERNAL_ERROR,
            "Internal error",
            "OpenSSL SHA256 failed"
        ));
    }
    return hexEncode(digest, SHA256_DIGEST_LENGTH);
}

ApiKeyCredential ApiKeyAuthenticator::createCredential(
    const std::string& key_id,
    const std::string& secret,
    const std::string& principal,
    const std::vector<std::string>& scopes,
    const std::vector<std::string>& roles,
    const std::string& tenant_id,
    std::chrono::system_clock::time_point expires_at)
{
    ApiKeyCredential cred;
    cred.key_id      = key_id;
    cred.secret_hash = hashSecret(secret);
    cred.principal   = principal;
    cred.tenant_id   = tenant_id;
    cred.scopes      = scopes;
    cred.roles       = roles;
    cred.expires_at  = expires_at;
    cred.active      = true;
    return cred;
}

// ============================================================================
// Private helpers
// ============================================================================

bool ApiKeyAuthenticator::constantTimeEqual(const std::string& a,
                                             const std::string& b)
{
    if (static_cast<int>(a.size()) != static_cast<int>(b.size())) {
        return false;
    }
    return CRYPTO_memcmp(a.data(), b.data(),static_cast<int>(a.size())) == 0;
}

std::string ApiKeyAuthenticator::hexEncode(const unsigned char* data, size_t len) {
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}

ApiKeyClaims ApiKeyAuthenticator::claimsFromCredential(
    const ApiKeyCredential& cred) const
{
    ApiKeyClaims claims;
    claims.key_id     = cred.key_id;
    claims.principal  = cred.principal;
    claims.tenant_id  = cred.tenant_id;
    claims.scopes     = cred.scopes;
    claims.roles      = cred.roles;
    claims.expires_at = cred.expires_at;
    return claims;
}

} // namespace auth
} // namespace themis
