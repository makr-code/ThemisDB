#include "auth/federated_identity_manager.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// Private static helpers
// ---------------------------------------------------------------------------

// static
std::string FederatedIdentityManager::normalize(const std::string& url) {
    std::string s = url;
    while (!s.empty() && s.back() == '/') {
        s.pop_back();
    }
    return s;
}

// static
std::string FederatedIdentityManager::extractIssuer(const std::string& raw_token) {
    // Strip optional "Bearer " prefix
    std::string token = raw_token;
    if (token.size() > 7 &&
        (token.substr(0, 7) == "Bearer " || token.substr(0, 7) == "bearer ")) {
        token = token.substr(7);
    }

    // A JWT has the form: <base64url-header>.<base64url-payload>.<signature>
    const auto first_dot = token.find('.');
    if (first_dot == std::string::npos) {
        throw AuthException(AuthError(
            AuthErrorCode::JWT_INVALID_FORMAT,
            "Token is not a valid JWT",
            "No dot separator found"
        ));
    }
    const auto second_dot = token.find('.', first_dot + 1);
    if (second_dot == std::string::npos) {
        throw AuthException(AuthError(
            AuthErrorCode::JWT_INVALID_FORMAT,
            "Token is not a valid JWT",
            "Only one dot separator found"
        ));
    }

    // Decode the payload section (base64url, no padding)
    std::string b64 = token.substr(first_dot + 1, second_dot - first_dot - 1);

    // Convert base64url to standard base64
    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding
    while (b64.size() % 4 != 0) {
        b64 += '=';
    }

    // Decode base64
    static const std::string b64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;
    decoded.reserve(b64.size() * 3 / 4);

    int val = 0;
    int bits = -8;
    for (unsigned char c : b64) {
        if (c == '=') break;
        const auto pos = b64_chars.find(static_cast<char>(c));
        if (pos == std::string::npos) continue;
        val = (val << 6) + static_cast<int>(pos);
        bits += 6;
        if (bits >= 0) {
            decoded.push_back(static_cast<char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }

    // Parse JSON and extract "iss"
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(decoded);
    } catch (const std::exception& ex) {
        throw AuthException(AuthError(
            AuthErrorCode::JWT_INVALID_FORMAT,
            "Token payload is not valid JSON",
            std::string("JSON parse error: ") + ex.what()
        ));
    }

    if (!payload.contains("iss") || !payload["iss"].is_string()) {
        throw AuthException(AuthError(
            AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM,
            "Token is missing required 'iss' claim",
            "iss claim absent or not a string"
        ));
    }

    return payload["iss"].get<std::string>();
}

// ---------------------------------------------------------------------------
// Realm registration
// ---------------------------------------------------------------------------

void FederatedIdentityManager::addRealm(const OIDCProviderConfig& config) {
    const std::string key = normalize(config.issuer_url);

    if (key.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "Realm configuration error",
            "issuer_url must not be empty"
        ));
    }

    // Build adjusted config with normalized issuer_url
    OIDCProviderConfig adjusted = config;
    adjusted.issuer_url = key;

    auto provider = std::make_shared<OIDCProvider>(adjusted);

    if (http_get_fn_) {
        provider->setHttpGetForTesting(http_get_fn_);
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (realms_.count(key) > 0) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "Realm already registered",
            "A realm with issuer '" + key + "' is already registered"
        ));
    }

    realms_.emplace(key, std::move(provider));
    spdlog::info("FederatedIdentityManager: registered realm '{}'", key);
}

bool FederatedIdentityManager::removeRealm(const std::string& issuer_url) {
    const std::string key = normalize(issuer_url);
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = realms_.find(key);
    if (it == realms_.end()) {
        return false;
    }
    realms_.erase(it);
    spdlog::info("FederatedIdentityManager: removed realm '{}'", key);
    return true;
}

bool FederatedIdentityManager::hasRealm(const std::string& issuer_url) const {
    const std::string key = normalize(issuer_url);
    std::lock_guard<std::mutex> lock(mutex_);
    return realms_.count(key) > 0;
}

std::vector<std::string> FederatedIdentityManager::realmIssuers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> issuers;
    issuers.reserve(realms_.size());
    for (const auto& kv : realms_) {
        issuers.push_back(kv.first);
    }
    return issuers;
}

size_t FederatedIdentityManager::realmCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return realms_.size();
}

// ---------------------------------------------------------------------------
// Token validation
// ---------------------------------------------------------------------------

FederatedValidationResult FederatedIdentityManager::validateToken(
    const std::string& token)
{
    // Step 1: peek at the issuer without full validation
    const std::string raw_iss = extractIssuer(token);
    const std::string iss     = normalize(raw_iss);

    // Step 2: find the matching realm (obtain a shared_ptr to keep it alive)
    std::shared_ptr<OIDCProvider> provider;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = realms_.find(iss);
        if (it == realms_.end()) {
            spdlog::warn("FederatedIdentityManager: no realm registered for issuer '{}'",
                         iss);
            throw AuthException(AuthError(
                AuthErrorCode::JWT_ISSUER_MISMATCH,
                "Token issuer is not trusted",
                "No realm registered for issuer '" + iss + "'"
            ));
        }
        provider = it->second;
    }

    // Step 3: delegate full validation to the realm's provider
    // (lock released – provider kept alive via shared_ptr)
    spdlog::debug("FederatedIdentityManager: validating token for realm '{}'", iss);
    JWTClaims claims = provider->validateToken(token);

    return FederatedValidationResult{std::move(claims), iss};
}

OIDCProvider& FederatedIdentityManager::realmProvider(const std::string& issuer_url) {
    const std::string key = normalize(issuer_url);
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = realms_.find(key);
    if (it == realms_.end()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "Unknown realm",
            "No realm registered for issuer '" + key + "'"
        ));
    }
    return *it->second;
}

// ---------------------------------------------------------------------------
// Testing helpers
// ---------------------------------------------------------------------------

void FederatedIdentityManager::setHttpGetForTesting(
    std::function<std::string(const std::string& url)> fn)
{
    http_get_fn_ = std::move(fn);
}

} // namespace auth
} // namespace themis
