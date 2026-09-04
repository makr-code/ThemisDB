/**
 * @file oauth_pkce_flow.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/oauth_pkce_flow.h"

#include <array>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "auth/jwt_validator.h"

namespace themis {
namespace auth {

namespace {

// libcurl write callback – appends received data to a std::string.
size_t oauthPkceWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    const auto total = size * nmemb;
    static_cast<std::string *>(userdata)->append(ptr, total);
    return total;
}

} // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

OAuthPKCEFlow::OAuthPKCEFlow(const Config &config) : config_(config) {
    if (config_.authorization_endpoint.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
                                      "authorization_endpoint must not be empty"));
    }
    if (config_.token_endpoint.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
                                      "token_endpoint must not be empty"));
    }
    if (config_.client_id.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
                                      "client_id must not be empty"));
    }
    if (config_.redirect_uri.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth PKCE configuration error",
                                      "redirect_uri must not be empty"));
    }
}

// ============================================================================
// Testing helpers
// ============================================================================

void OAuthPKCEFlow::setHttpPostForTesting(
    std::function<std::string(const std::string &url, const std::string &body)> fn) {
    http_post_fn_ = std::move(fn);
}

void OAuthPKCEFlow::setRandBytesForTesting(std::function<void(unsigned char *buf, std::size_t len)> fn) {
    rand_bytes_fn_ = std::move(fn);
}

// ============================================================================
// RFC 7636 §4.1 – Generate code_verifier and code_challenge
// ============================================================================

OAuthPKCEFlow::PKCEChallenge OAuthPKCEFlow::generateChallenge() {
    // Generate 96 random bytes → 128 Base64URL characters (fits 43–128 limit).
    constexpr std::size_t kVerifierBytes = 96;
    std::array<unsigned char, kVerifierBytes> raw{};

    fillRandomBytes(raw.data(),static_cast<int>(raw.size()));

    const std::string verifier = base64UrlEncode(raw.data(),static_cast<int>(raw.size()));

    // code_challenge = BASE64URL(SHA256(ASCII(code_verifier)))
    const std::string digest = sha256(verifier);
    const std::string challenge
        = base64UrlEncode(reinterpret_cast<const unsigned char *>(digest.data()),static_cast<int>(digest.size()));

    spdlog::debug("OAuthPKCEFlow: generated PKCE challenge (method=S256, "
                  "verifier_len={})",
                  verifier.size());

    PKCEChallenge result;
    result.code_verifier  = verifier;
    result.code_challenge = challenge;
    // challenge_method is already defaulted to "S256" in the struct
    return result;
}

// ============================================================================
// Build authorization URL
// ============================================================================

std::string OAuthPKCEFlow::buildAuthorizationUrl(const PKCEChallenge &challenge, const std::string &state) const {
    std::string url = config_.authorization_endpoint;
    url += (url.find('?') == std::string::npos) ? '?' : '&';

    auto append = [&url](const std::string &key, const std::string &val) {
        url += urlEncode(key) + '=' + urlEncode(val) + '&';
    };

    append("response_type", "code");
    append("client_id", config_.client_id);
    append("redirect_uri", config_.redirect_uri);

    if (!config_.scopes.empty()) {
        std::string scope_str = {};
        for (std::size_t i = 0; i <static_cast<int>(config_.scopes.size()); ++i) {
            if (i > 0) {
                scope_str += ' ';
            }
            scope_str += config_.scopes[i];
        }
        append("scope", scope_str);
    }

    append("code_challenge", challenge.code_challenge);
    append("code_challenge_method", challenge.challenge_method);

    if (!state.empty()) {
        append("state", state);
    }

    // Remove trailing '&'
    if (!url.empty() && url.back() == '&') {
        url.pop_back();
    }

    return url;
}

// ============================================================================
// RFC 7636 §4.5 – Token Exchange
// ============================================================================

OAuthPKCEFlow::TokenResponse OAuthPKCEFlow::exchangeCode(const std::string &authorization_code,
                                                         const std::string &code_verifier) {
    if (authorization_code.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "PKCE token exchange failed",
                                      "authorization_code must not be empty"));
    }
    if (code_verifier.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "PKCE token exchange failed",
                                      "code_verifier must not be empty"));
    }

    const std::vector<std::pair<std::string, std::string>> params = {{"grant_type", "authorization_code"},
                                                                     {"code", authorization_code},
                                                                     {"redirect_uri", config_.redirect_uri},
                                                                     {"client_id", config_.client_id},
                                                                     {"code_verifier", code_verifier}};

    const std::string body = buildFormBody(params);
    spdlog::debug("OAuthPKCEFlow: exchanging authorization code at {}", config_.token_endpoint);

    std::string response_body = {};
    {
        // B3: retry httpPost() with exponential backoff on transient errors
        constexpr int kMaxRetries  = 3;
        constexpr int kBaseDelayMs = 100;
        std::exception_ptr last_exc = {};
        for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
            try {
                response_body = httpPost(config_.token_endpoint, body);
                last_exc = nullptr;
                break;
            } catch (const std::exception &ex) {
                last_exc = std::current_exception();
                const std::string what = ex.what();
                const bool retryable   = (what.find("HTTP 429") != std::string::npos)
                                       || (what.find("HTTP 503") != std::string::npos)
                                       || (what.find("libcurl") != std::string::npos);
                if (!retryable || attempt + 1 == kMaxRetries) {
                  break;
                }
                const int delay_ms = kBaseDelayMs * (1 << attempt);
                spdlog::warn("OAuthPKCEFlow: token exchange attempt {} failed ({}), retrying in {}ms",
                             attempt + 1, what, delay_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
        if (last_exc) {
            try { std::rethrow_exception(last_exc); }
            catch (const std::exception &ex) {
                spdlog::error("OAuthPKCEFlow: token exchange HTTP error: {}", ex.what());
                throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "PKCE token exchange failed",
                                             std::string("HTTP error: ") + ex.what()));
            }
        }
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(response_body);
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("OAuthPKCEFlow: failed to parse token response: {}", ex.what());
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Invalid token response",
                                      std::string("JSON parse error: ") + ex.what()));
    }

    // RFC 6749 §5.2 – error response
    if (j.contains("error")) {
        const std::string err  = j.value("error", "unknown_error");
        const std::string desc = j.value("error_description", "");
        spdlog::error("OAuthPKCEFlow: token endpoint error '{}': {}", err, desc);

        const AuthErrorCode code
            = (err == "invalid_grant") ? AuthErrorCode::AUTH_INVALID_CREDENTIALS : AuthErrorCode::AUTH_INTERNAL_ERROR;
        throw AuthException(AuthError(code, "PKCE token exchange error", "error=" + err + " description=" + desc));
    }

    TokenResponse token;
    try {
        token.access_token  = j.at("access_token").get<std::string>();
        token.token_type    = j.value("token_type", "Bearer");
        token.expires_in    = j.value("expires_in", 0);
        token.refresh_token = j.value("refresh_token", std::string{});
        token.scope         = j.value("scope", std::string{});
        token.id_token      = j.value("id_token", std::string{});
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("OAuthPKCEFlow: incomplete token response: {}", ex.what());
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Incomplete token response",
                                      std::string("Missing field: ") + ex.what()));
    }

    spdlog::info("OAuthPKCEFlow: access token obtained (type='{}')", token.token_type);
    return token;
}

// ============================================================================
// id_token validation
// ============================================================================

JWTClaims OAuthPKCEFlow::validateIdToken(const TokenResponse &token_response) {
    if (token_response.id_token.empty()) {
        throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM, "No id_token in token response",
                                      "id_token is empty; ensure 'openid' scope was requested"));
    }
    if (config_.jwks_url.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "JWKS URL not configured",
                                      "jwks_url must be set in OAuthPKCEFlow::Config to validate id_token"));
    }

    JWTValidator validator(config_.jwks_url);
    return validator.parseAndValidate(token_response.id_token);
}

// ============================================================================
// HTTP helper
// ============================================================================

std::string OAuthPKCEFlow::httpPost(const std::string &url, const std::string &body) {
    if (http_post_fn_) {
        return http_post_fn_(url, body);
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl handle");
    }

    std::string response_body = {};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, oauthPkceWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(config_.http_timeout_seconds));
    // Always verify TLS certificates
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    struct curl_slist *headers = nullptr;
    headers                    = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Use curl_multi_perform() so this transfer can participate in a shared
    // multi-handle event loop in the future.
    CURLM *multi = curl_multi_init();
    if (!multi) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to initialize libcurl multi handle");
    }

    CURLMcode add_rc = curl_multi_add_handle(multi, curl);
    if (add_rc != CURLM_OK) {
        curl_slist_free_all(headers);
        curl_multi_cleanup(multi);
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("curl_multi_add_handle failed: ") + curl_multi_strerror(add_rc));
    }

    int still_running = 0;
    CURLMcode mc      = CURLM_OK;
    do {
        mc = curl_multi_perform(multi, &still_running);
        if (mc != CURLM_OK) {
            break;
        }
        if (still_running) {
            mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
        }
    } while (still_running && mc == CURLM_OK);

    // Inspect per-transfer result so network/TLS errors are not silently
    // swallowed as an empty response body.
    CURLcode easy_rc = (mc == CURLM_OK) ? CURLE_OK : CURLE_FAILED_INIT;
    if (mc == CURLM_OK) {
        CURLMsg *msg  = nullptr;
        int msgs_left = 0;
        while ((msg = curl_multi_info_read(multi, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE && msg->easy_handle == curl) {
                easy_rc = msg->data.result;
            }
        }
    }

    curl_slist_free_all(headers);
    curl_multi_remove_handle(multi, curl);
    curl_multi_cleanup(multi);
    curl_easy_cleanup(curl);

    if (mc != CURLM_OK) {
        throw std::runtime_error(std::string("libcurl multi error: ") + curl_multi_strerror(mc));
    }
    if (easy_rc != CURLE_OK) {
        throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));
    }

    return response_body;
}

// ============================================================================
// Crypto / encoding helpers
// ============================================================================

void OAuthPKCEFlow::fillRandomBytes(unsigned char *buf, std::size_t len) {
    if (rand_bytes_fn_) {
        rand_bytes_fn_(buf, len);
        return;
    }
    if (RAND_bytes(buf, static_cast<int>(len)) != 1) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to generate secure random bytes",
                                      "OpenSSL RAND_bytes returned error"));
    }
}

std::string OAuthPKCEFlow::sha256(const std::string &input) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to compute SHA-256 hash",
                                      "EVP_MD_CTX_new returned null"));
    }

    const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1
                    && EVP_DigestUpdate(ctx, input.data(),static_cast<int>(input.size())) == 1
                    && EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) == 1;

    EVP_MD_CTX_free(ctx);

    if (!ok) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to compute SHA-256 hash",
                                      "OpenSSL EVP digest operation failed"));
    }

    return std::string(reinterpret_cast<const char *>(digest.data()), digest_len);
}

std::string OAuthPKCEFlow::base64UrlEncode(const unsigned char *data, std::size_t len) {
    // Standard Base64 alphabet
    static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out = {};
    out.reserve(((len + 2) / 3) * 4);

    for (std::size_t i = 0; i < len; i += 3) {
        const uint32_t b0     = data[i];
        const uint32_t b1     = (i + 1 < len) ? data[i + 1] : 0;
        const uint32_t b2     = (i + 2 < len) ? data[i + 2] : 0;
        const uint32_t triple = (b0 << 16) | (b1 << 8) | b2;

        out += kTable[(triple >> 18) & 0x3F];
        out += kTable[(triple >> 12) & 0x3F];
        out += (i + 1 < len) ? kTable[(triple >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? kTable[(triple) & 0x3F] : '=';
    }

    // Convert standard Base64 → Base64URL (RFC 4648 §5): replace +→-, /→_, strip =
    for (char &c : out) {
        if (c == '+') {
            c = '-';
        } else if (c == '/') {
            c = '_';
        }
    }
    // Strip padding
    while (!out.empty() && out.back() == '=') {
        out.pop_back();
    }

    return out;
}

std::string OAuthPKCEFlow::urlEncode(const std::string &value) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return value;
    }

    char *encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    std::string result = {};
    if (encoded) {
        result = encoded;
        curl_free(encoded);
    }
    curl_easy_cleanup(curl);
    return result;
}

std::string OAuthPKCEFlow::buildFormBody(const std::vector<std::pair<std::string, std::string>> &params) {
    std::string body = {};
    for (std::size_t i = 0; i <static_cast<int>(params.size()); ++i) {
        if (i > 0) {
            body += '&';
        }
        body += urlEncode(params[i].first);
        body += '=';
        body += urlEncode(params[i].second);
    }
    return body;
}

} // namespace auth
} // namespace themis
