/**
 * @file oauth2_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/oauth2_provider.h"
#include "auth/auth_error.h"
#include "utils/logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <curl/curl.h>
#include <openssl/rand.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace themis {
namespace server {

namespace {

// libcurl write callback – appends received data to a std::string.
// size is always 1 per the libcurl contract; nmemb is the byte count.
size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    const size_t total = size * nmemb;  // safe: size==1 per libcurl contract
    static_cast<std::string*>(userdata)->append(ptr, total);
    return total;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/*static*/ nlohmann::json OAuth2Provider::makeError(int status_code,
                                                     const std::string& message)
{
    return {{"status_code", status_code}, {"error", message}};
}

/*static*/ std::string OAuth2Provider::urlEncode(const std::string& input)
{
    std::ostringstream oss = {};
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<unsigned>(c);
        }
    }
    return oss.str();
}

/*static*/ std::string OAuth2Provider::generateState()
{
    unsigned char buf[16]{};
    RAND_bytes(buf, static_cast<int>(sizeof(buf)));
    std::ostringstream oss = {};
    for (unsigned char b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(b);
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

OAuth2Provider::OAuth2Provider(const Config& config)
    : config_(config)
{
    if (config_.oidc.issuer_url.empty()) {
        throw auth::AuthException(auth::AuthError(
            auth::AuthErrorCode::AUTH_CONFIG_INVALID,
            "OAuth2Provider configuration error",
            "oidc.issuer_url must not be empty"
        ));
    }
    if (config_.oidc.client_id.empty()) {
        throw auth::AuthException(auth::AuthError(
            auth::AuthErrorCode::AUTH_CONFIG_INVALID,
            "OAuth2Provider configuration error",
            "oidc.client_id must not be empty"
        ));
    }
    if (config_.redirect_uri.empty()) {
        throw auth::AuthException(auth::AuthError(
            auth::AuthErrorCode::AUTH_CONFIG_INVALID,
            "OAuth2Provider configuration error",
            "redirect_uri must not be empty"
        ));
    }

    oidc_provider_ = std::make_unique<auth::OIDCProvider>(config_.oidc);

    THEMIS_INFO("OAuth2Provider initialized: issuer={}, client_id={}, redirect_uri={}",
                config_.oidc.issuer_url, config_.oidc.client_id, config_.redirect_uri);
}

// ---------------------------------------------------------------------------
// Pending-state helpers
// ---------------------------------------------------------------------------

std::string OAuth2Provider::storePendingState(const std::string& code_verifier,
                                               const std::string& requested_state)
{
    const std::string state = requested_state.empty() ? generateState() : requested_state;

    std::lock_guard<std::mutex> lock(pending_mutex_);
    evictExpiredStates();
    pending_states_[state] = {
        code_verifier,
        std::chrono::system_clock::now() + config_.state_ttl
    };
    return state;
}

std::optional<std::string> OAuth2Provider::consumePendingState(const std::string& state)
{
    std::lock_guard<std::mutex> lock(pending_mutex_);
    evictExpiredStates();
    auto it = pending_states_.find(state);
    if (it == pending_states_.end()) {
        return std::nullopt;
    }
    std::string verifier = it->second.code_verifier;
    pending_states_.erase(it);
    return verifier;
}

void OAuth2Provider::evictExpiredStates()
{
    // Caller must hold pending_mutex_
    const auto now = std::chrono::system_clock::now();
    for (auto it = pending_states_.begin(); it != pending_states_.end(); ) {
        if (it->second.expires_at <= now) {
            it = pending_states_.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// PKCE flow helpers
// ---------------------------------------------------------------------------

auth::OAuthPKCEFlow::Config OAuth2Provider::buildPKCEConfig(
    const std::string& redirect_uri_override) const
{
    const auto& doc = oidc_provider_->discoveryDocument();

    auth::OAuthPKCEFlow::Config cfg;
    cfg.authorization_endpoint = doc.authorization_endpoint;
    cfg.token_endpoint         = doc.token_endpoint;
    cfg.client_id              = config_.oidc.client_id;
    cfg.redirect_uri           = redirect_uri_override.empty()
                                     ? config_.redirect_uri
                                     : redirect_uri_override;
    cfg.scopes                 = config_.oidc.scopes.empty()
                                     ? std::vector<std::string>{"openid"}
                                     : config_.oidc.scopes;
    cfg.jwks_url               = doc.jwks_uri;
    cfg.http_timeout_seconds   = config_.oidc.http_timeout_seconds;
    return cfg;
}

void OAuth2Provider::ensurePKCEFlow(const std::string& redirect_uri_override)
{
    pkce_flow_ = std::make_unique<auth::OAuthPKCEFlow>(
        buildPKCEConfig(redirect_uri_override));

    if (http_post_fn_) {
        pkce_flow_->setHttpPostForTesting(http_post_fn_);
    }
    if (rand_bytes_fn_) {
        pkce_flow_->setRandBytesForTesting(rand_bytes_fn_);
    }
}

// ---------------------------------------------------------------------------
// HTTP POST helper (for refresh and revocation)
// ---------------------------------------------------------------------------

std::string OAuth2Provider::httpPost(const std::string& url,
                                     const std::string& body) const
{
    if (http_post_fn_) {
        return http_post_fn_(url, body);
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl handle");
    }

    std::string response = {};
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,
                     static_cast<long>(config_.oidc.http_timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers,
                                "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    const CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(
            std::string("libcurl error: ") + curl_easy_strerror(res));
    }
    return response;
}

// ---------------------------------------------------------------------------
// Token response helper
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::doTokenExchange(const std::string& code,
                                                const std::string& code_verifier,
                                                const std::string& redirect_uri_override)
{
    ensurePKCEFlow(redirect_uri_override);
    const auto token_response = pkce_flow_->exchangeCode(code, code_verifier);

    const std::string issued_token = config_.token_factory
        ? config_.token_factory(token_response.access_token)
        : token_response.access_token;

    nlohmann::json result = {
        {"access_token",  issued_token},
        {"token_type",    token_response.token_type.empty()
                              ? "Bearer" : token_response.token_type},
        {"expires_in",    token_response.expires_in},
        {"refresh_token", token_response.refresh_token},
        {"scope",         token_response.scope}
    };
    if (!token_response.id_token.empty()) {
        result["id_token"] = token_response.id_token;
    }
    return result;
}

// ---------------------------------------------------------------------------
// GET /api/v1/auth/oauth2/authorize
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::handleAuthorize(const std::string& state,
                                                const std::string& redirect_uri)
{
    if (!state.empty() && static_cast<int>(state.size()) > config_.max_state_length) {
        return makeError(400, "state parameter exceeds maximum allowed length");
    }

    try {
        ensurePKCEFlow(redirect_uri);

        const auto challenge = pkce_flow_->generateChallenge();
        const std::string actual_state =
            storePendingState(challenge.code_verifier, state);
        const std::string auth_url =
            pkce_flow_->buildAuthorizationUrl(challenge, actual_state);

        THEMIS_INFO("OAuth2Provider::handleAuthorize – state={}", actual_state);
        return {
            {"authorization_url", auth_url},
            {"state",             actual_state},
            {"code_verifier",     challenge.code_verifier}
        };
    } catch (const auth::AuthException& ex) {
        THEMIS_ERROR("OAuth2Provider::handleAuthorize auth exception: {}", ex.what());
        return makeError(500, std::string("Failed to build authorization URL: ")
                              + ex.error().publicMessage());
    } catch (const std::exception& e) {
        THEMIS_ERROR("OAuth2Provider::handleAuthorize exception: {}", e.what());
        return makeError(500, std::string("Failed to build authorization URL: ")
                              + e.what());
    }
}

// ---------------------------------------------------------------------------
// GET /api/v1/auth/oauth2/callback
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::handleCallback(const std::string& code,
                                               const std::string& state)
{
    if (code.empty()) {
        return makeError(400, "Missing 'code' parameter");
    }
    if (state.empty()) {
        return makeError(400, "Missing 'state' parameter");
    }

    auto maybe_verifier = consumePendingState(state);
    if (!maybe_verifier.has_value()) {
        THEMIS_WARN("OAuth2Provider::handleCallback – unknown or expired state={}",
                    state);
        return makeError(400, "Unknown or expired state parameter");
    }

    try {
        auto result = doTokenExchange(code, *maybe_verifier);
        THEMIS_INFO([[maybe_unused]] "OAuth2Provider::handleCallback – token exchange successful");
        return result;
    } catch (const auth::AuthException& ex) {
        THEMIS_WARN("OAuth2Provider::handleCallback – auth failure: {}", ex.what());
        const bool is_client_error =
            ex.error().code() == auth::AuthErrorCode::AUTH_TOKEN_INVALID    ||
            ex.error().code() == auth::AuthErrorCode::AUTH_TOKEN_EXPIRED    ||
            ex.error().code() == auth::AuthErrorCode::AUTH_INVALID_CREDENTIALS;
        return makeError(is_client_error ? 401 : 400, ex.error().publicMessage());
    } catch (const std::exception& e) {
        THEMIS_ERROR("OAuth2Provider::handleCallback exception: {}", e.what());
        return makeError(500, "Internal token exchange error");
    }
}

// ---------------------------------------------------------------------------
// POST /api/v1/auth/oauth2/token
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::handleTokenExchange(const std::string& code,
                                                    const std::string& code_verifier,
                                                    const std::string& state)
{
    if (code.empty()) {
        return makeError(400, "Missing 'code' field");
    }
    if (code_verifier.empty()) {
        return makeError(400, "Missing 'code_verifier' field");
    }

    // If state was provided, validate against the stored pending entry.
    if (!state.empty()) {
        auto maybe_verifier = consumePendingState(state);
        if (!maybe_verifier.has_value()) {
            THEMIS_WARN("OAuth2Provider::handleTokenExchange – unknown/expired state={}",
                        state);
            return makeError(400, "Unknown or expired state parameter");
        }
        // The caller-supplied verifier must match the server-stored one.
        if (*maybe_verifier != code_verifier) {
            THEMIS_WARN("OAuth2Provider::handleTokenExchange – code_verifier mismatch");
            return makeError(400,
                "code_verifier does not match the stored value for this state");
        }
    }

    try {
        auto result = doTokenExchange(code, code_verifier);
        THEMIS_INFO("OAuth2Provider::handleTokenExchange – token exchange successful");
        return result;
    } catch (const auth::AuthException& ex) {
        THEMIS_WARN("OAuth2Provider::handleTokenExchange – auth failure: {}", ex.what());
        const bool is_client_error =
            ex.error().code() == auth::AuthErrorCode::AUTH_TOKEN_INVALID    ||
            ex.error().code() == auth::AuthErrorCode::AUTH_TOKEN_EXPIRED    ||
            ex.error().code() == auth::AuthErrorCode::AUTH_INVALID_CREDENTIALS;
        return makeError(is_client_error ? 401 : 400, ex.error().publicMessage());
    } catch (const std::exception& e) {
        THEMIS_ERROR("OAuth2Provider::handleTokenExchange exception: {}", e.what());
        return makeError(500, "Internal token exchange error");
    }
}

// ---------------------------------------------------------------------------
// POST /api/v1/auth/oauth2/refresh
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::handleRefresh(const std::string& refresh_token)
{
    if (refresh_token.empty()) {
        return makeError(400, "Missing 'refresh_token' field");
    }

    try {
        const auto& doc = oidc_provider_->discoveryDocument();

        if (doc.token_endpoint.empty()) {
            return makeError(500, "OIDC token endpoint not available");
        }

        // Build an RFC 6749 refresh_token grant body
        auto enc = [](const std::string& v) { return urlEncode(v); };

        std::string body = {};
        body += "grant_type=" + enc("refresh_token");
        body += "&refresh_token=" + enc(refresh_token);
        body += "&client_id=" + enc(config_.oidc.client_id);
        if (!config_.oidc.client_secret.empty()) {
            body += "&client_secret=" + enc(config_.oidc.client_secret);
        }

        THEMIS_INFO("OAuth2Provider::handleRefresh – posting refresh grant to {}",
                    doc.token_endpoint);

        const std::string response_body = httpPost(doc.token_endpoint, body);

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(response_body);
        } catch (const nlohmann::json::exception& ex) {
            THEMIS_ERROR("OAuth2Provider::handleRefresh – JSON parse error: {}", ex.what());
            return makeError(500, "Invalid refresh token response from IdP");
        }

        if (j.contains("error")) {
            const std::string err  = j.value("error", "unknown_error");
            const std::string desc = j.value("error_description", "");
            THEMIS_WARN("OAuth2Provider::handleRefresh – IdP error '{}': {}", err, desc);
            return makeError(401,
                "Refresh token rejected by IdP: " + err +
                (desc.empty() ? "" : " – " + desc));
        }

        const std::string access_token  = j.value("access_token",  std::string{});
        const std::string token_type    = j.value("token_type",    std::string{"Bearer"});
        const int         expires_in    = j.value("expires_in",    0);
        const std::string new_refresh   = j.value("refresh_token", std::string{});
        const std::string scope         = j.value("scope",         std::string{});

        if (access_token.empty()) {
            return makeError(500, "IdP did not return an access_token");
        }

        const std::string issued_token = config_.token_factory
            ? config_.token_factory(access_token)
            : access_token;

        THEMIS_INFO([[maybe_unused]] "OAuth2Provider::handleRefresh – refresh successful");

        nlohmann::json result = {
            {"access_token",  issued_token},
            {"token_type",    token_type},
            {"expires_in",    expires_in},
            {"scope",         scope}
        };
        if (!new_refresh.empty()) {
            result["refresh_token"] = new_refresh;
        }
        return result;

    } catch (const auth::AuthException& ex) {
        THEMIS_WARN("OAuth2Provider::handleRefresh – auth failure: {}", ex.what());
        return makeError(400, ex.error().publicMessage());
    } catch (const std::exception& e) {
        THEMIS_ERROR("OAuth2Provider::handleRefresh exception: {}", e.what());
        return makeError(500, "Internal refresh error");
    }
}

// ---------------------------------------------------------------------------
// POST /api/v1/auth/token/introspect
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::handleIntrospect(const std::string& token)
{
    if (token.empty()) {
        return makeError(400, "Missing 'token' field");
    }

    try {
        const auto claims = oidc_provider_->validateToken(token);

        const auto exp_t = std::chrono::system_clock::to_time_t(claims.expiration);

        nlohmann::json result = {
            {"active", true},
            {"sub",    claims.sub},
            {"iss",    claims.issuer},
            {"exp",    static_cast<long long>(exp_t)},
            {"jti",    claims.jti}
        };

        if (!claims.email.empty()) {
            result["email"] = claims.email;
        }
        if (!claims.tenant_id.empty()) {
            result["tenant_id"] = claims.tenant_id;
        }
        if (!claims.groups.empty()) {
            result["groups"] = claims.groups;
        }
        if (!claims.roles.empty()) {
            result["roles"] = claims.roles;
        }
        if (!claims.audience.empty()) {
            if (static_cast<int>(claims.audience.size()) == 1) {
                result["aud"] = claims.audience[0];
            } else {
                result["aud"] = claims.audience;
            }
        }
        if (claims.issued_at.has_value()) {
            result["iat"] = static_cast<long long>(
                std::chrono::system_clock::to_time_t(*claims.issued_at));
        }

        THEMIS_INFO("OAuth2Provider::handleIntrospect – active token: sub={}",
                    claims.sub);
        return result;

    } catch (const auth::AuthException&) {
        // Any validation failure → inactive (RFC 7662 §2.2)
        return {{"active", false}};
    } catch (const std::exception& e) {
        THEMIS_ERROR("OAuth2Provider::handleIntrospect exception: {}", e.what());
        return {{"active", false}};
    }
}

// ---------------------------------------------------------------------------
// POST /api/v1/auth/oauth2/logout
// ---------------------------------------------------------------------------

nlohmann::json OAuth2Provider::handleLogout(const std::string& refresh_token)
{
    if (!refresh_token.empty()) {
        try {
            const auto& doc = oidc_provider_->discoveryDocument();
            if (!doc.revocation_endpoint.empty()) {
                std::string body =
                    "token=" + urlEncode(refresh_token) +
                    "&token_type_hint=refresh_token" +
                    "&client_id=" + urlEncode(config_.oidc.client_id);
                if (!config_.oidc.client_secret.empty()) {
                    body += "&client_secret=" + urlEncode(config_.oidc.client_secret);
                }
                (void)httpPost(doc.revocation_endpoint, body);
                THEMIS_INFO("OAuth2Provider::handleLogout – refresh token revocation posted");
            } else {
                THEMIS_WARN("OAuth2Provider::handleLogout – revocation endpoint unavailable in discovery metadata");
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("OAuth2Provider::handleLogout – refresh token revocation failed: {}", e.what());
        }
    }
    THEMIS_INFO("OAuth2Provider::handleLogout – session ended");
    return {{"success", true}};
}

// ---------------------------------------------------------------------------
// Testing helpers
// ---------------------------------------------------------------------------

void OAuth2Provider::setDiscoveryDocumentForTesting(
    const auth::OIDCDiscoveryDocument& doc)
{
    oidc_provider_->setDiscoveryDocumentForTesting(doc);
}

void OAuth2Provider::setHttpGetForTesting(
    std::function<std::string(const std::string& url)> fn)
{
    oidc_provider_->setHttpGetForTesting(std::move(fn));
}

void OAuth2Provider::setHttpPostForTesting(
    std::function<std::string(const std::string& url, const std::string& body)> fn)
{
    http_post_fn_ = fn;
    if (pkce_flow_) {
        pkce_flow_->setHttpPostForTesting(fn);
    }
}

void OAuth2Provider::setRandBytesForTesting(
    std::function<void(unsigned char* buf, std::size_t len)> fn)
{
    rand_bytes_fn_ = fn;
    if (pkce_flow_) {
        pkce_flow_->setRandBytesForTesting(fn);
    }
}

void OAuth2Provider::setRefreshTokenRevocationFn(RefreshTokenRevocationFn fn)
{
    refresh_token_revocation_fn_ = std::move(fn);
}

} // namespace server
} // namespace themis
