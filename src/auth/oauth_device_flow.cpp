/**
 * @file oauth_device_flow.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.20
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=6, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/oauth_device_flow.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "auth/jwt_validator.h"
#include "utils/audit_logger.h"

namespace themis {
namespace auth {

namespace {

// libcurl write callback – appends received data to a std::string.
size_t oauthDeviceWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    const auto total = size * nmemb;
    static_cast<std::string *>(userdata)->append(ptr, total);
    return total;
}

} // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

OAuthDeviceFlow::OAuthDeviceFlow(const Config &config) : config_(config) {
    if (config_.device_authorization_endpoint.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration error",
                                      "device_authorization_endpoint must not be empty"));
    }
    if (config_.token_endpoint.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration error",
                                      "token_endpoint must not be empty"));
    }
    if (config_.client_id.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "OAuth device flow configuration error",
                                      "client_id must not be empty"));
    }
}

// ============================================================================
// Testing helper
// ============================================================================

void OAuthDeviceFlow::setHttpPostForTesting(
    std::function<std::string(const std::string &url, const std::string &body)> fn) {
    http_post_fn_ = std::move(fn);
}

// ============================================================================
// RFC 8628 §3.1 – Device Authorization Request
// ============================================================================

OAuthDeviceFlow::DeviceCodeResponse OAuthDeviceFlow::requestDeviceCode() {
    std::vector<std::pair<std::string, std::string>> params = {{"client_id", config_.client_id}};

    if (!config_.scopes.empty()) {
        std::string scope_str;
        for (size_t i = 0; i < config_.scopes.size(); ++i) {
            if (i > 0) {
                scope_str += ' ';
            }
            scope_str += config_.scopes[i];
        }
        params.emplace_back("scope", scope_str);
    }

    const std::string body = buildFormBody(params);
    spdlog::debug("OAuthDeviceFlow: requesting device code from {}", config_.device_authorization_endpoint);

    std::string response_body;
    try {
        response_body = httpPost(config_.device_authorization_endpoint, body);
    } catch (const std::exception &ex) {
        spdlog::error("OAuthDeviceFlow: device code request failed: {}", ex.what());
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Failed to request device code",
                                      std::string("HTTP error: ") + ex.what()));
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(response_body);
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("OAuthDeviceFlow: failed to parse device code response: {}", ex.what());
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Invalid device code response",
                                      std::string("JSON parse error: ") + ex.what()));
    }

    // RFC 8628 §3.2 – error response
    if (j.contains("error")) {
        const std::string err  = j.value("error", "unknown_error");
        const std::string desc = j.value("error_description", "");
        spdlog::error("OAuthDeviceFlow: server returned error '{}': {}", err, desc);
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Device authorization server error",
                                      "error=" + err + " description=" + desc));
    }

    DeviceCodeResponse resp;
    try {
        resp.device_code               = j.at("device_code").get<std::string>();
        resp.user_code                 = j.at("user_code").get<std::string>();
        resp.verification_uri          = j.at("verification_uri").get<std::string>();
        resp.expires_in                = j.value("expires_in", 600);
        resp.interval                  = j.value("interval", 5);
        resp.verification_uri_complete = j.value("verification_uri_complete", resp.verification_uri);
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("OAuthDeviceFlow: missing required field in device code response: {}", ex.what());
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Incomplete device code response",
                                      std::string("Missing field: ") + ex.what()));
    }

    spdlog::info("OAuthDeviceFlow: device code obtained; user_code='{}' uri='{}'", resp.user_code,
                 resp.verification_uri);
    return resp;
}

// ============================================================================
// RFC 8628 §3.4 – Device Access Token Request
// ============================================================================

OAuthDeviceFlow::TokenResponse OAuthDeviceFlow::pollForToken(const std::string &device_code, PollStatus &status_out) {
    std::vector<std::pair<std::string, std::string>> params
        = {{"grant_type", "urn:ietf:params:oauth:grant-type:device_code"},
           {"device_code", device_code},
           {"client_id", config_.client_id}};
    if (!config_.client_secret.empty()) {
        params.emplace_back("client_secret", config_.client_secret);
    }

    const std::string body = buildFormBody(params);
    spdlog::debug("OAuthDeviceFlow: polling token endpoint {}", config_.token_endpoint);

    std::string response_body;
    {
        // B4: retry httpPost() with exponential backoff on transient transport errors
        constexpr int kMaxRetries  = 3;
        constexpr int kBaseDelayMs = 100;
        bool success = false;
        for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
            try {
                response_body = httpPost(config_.token_endpoint, body);
                success = true;
                break;
            } catch (const std::exception &ex) {
                const std::string what = ex.what();
                const bool retryable   = (what.find("HTTP 429") != std::string::npos)
                                       || (what.find("HTTP 503") != std::string::npos)
                                       || (what.find("libcurl") != std::string::npos);
                if (!retryable || attempt + 1 == kMaxRetries) {
                    spdlog::warn("OAuthDeviceFlow: token poll HTTP error: {}", what);
                    status_out = PollStatus::Error;
                    return {};
                }
                const int delay_ms = kBaseDelayMs * (1 << attempt);
                spdlog::warn("OAuthDeviceFlow: token poll attempt {} failed ({}), retrying in {}ms",
                             attempt + 1, what, delay_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
        if (!success) {
            status_out = PollStatus::Error;
            return {};
        }
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(response_body);
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("OAuthDeviceFlow: failed to parse token response: {}", ex.what());
        status_out = PollStatus::Error;
        return {};
    }

    // Check for error
    if (j.contains("error")) {
        const std::string err = j.value("error", "");

        if (err == "authorization_pending") {
            status_out = PollStatus::AuthorizationPending;
            return {};
        }
        if (err == "slow_down") {
            status_out = PollStatus::SlowDown;
            return {};
        }
        if (err == "access_denied") {
            spdlog::info("OAuthDeviceFlow: user denied authorization");
            status_out = PollStatus::AccessDenied;
            throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "Authorization denied",
                                          "User denied OAuth device flow authorization"));
        }
        if (err == "expired_token") {
            spdlog::warn("OAuthDeviceFlow: device code expired");
            status_out = PollStatus::ExpiredToken;
            throw AuthException(AuthError(AuthErrorCode::AUTH_TOKEN_EXPIRED, "Device code expired",
                                          "OAuth device code expired before user authorized"));
        }

        // Unexpected error
        const std::string desc = j.value("error_description", "");
        spdlog::error("OAuthDeviceFlow: token endpoint error '{}': {}", err, desc);
        status_out = PollStatus::Error;
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Token endpoint error",
                                      "error=" + err + " description=" + desc));
    }

    // Success – parse token response
    TokenResponse token;
    try {
        token.access_token  = j.at("access_token").get<std::string>();
        token.token_type    = j.value("token_type", "Bearer");
        token.expires_in    = j.value("expires_in", 0);
        token.refresh_token = j.value("refresh_token", std::string{});
        token.scope         = j.value("scope", std::string{});
        token.id_token      = j.value("id_token", std::string{});
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("OAuthDeviceFlow: incomplete token response: {}", ex.what());
        status_out = PollStatus::Error;
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Incomplete token response",
                                      std::string("Missing field: ") + ex.what()));
    }

    spdlog::info("OAuthDeviceFlow: access token obtained (type='{}')", token.token_type);
    status_out = PollStatus::Authorized;
    return token;
}

// ============================================================================
// id_token Validation
// ============================================================================

JWTClaims OAuthDeviceFlow::validateIdToken(const TokenResponse &token_response) {
    if (token_response.id_token.empty()) {
        throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM, "No id_token in token response",
                                      "id_token is empty; ensure 'openid' scope was requested"));
    }
    if (config_.jwks_url.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "JWKS URL not configured",
                                      "jwks_url must be set in OAuthDeviceFlow::Config to validate id_token"));
    }

    JWTValidator validator(config_.jwks_url);
    return validator.parseAndValidate(token_response.id_token);
}

// ============================================================================
// High-level authenticate()
// ============================================================================

JWTClaims OAuthDeviceFlow::authenticate(std::function<void(const DeviceCodeResponse &)> progress_cb) {
    try {
        const DeviceCodeResponse device_resp = requestDeviceCode();

        if (progress_cb) {
            progress_cb(device_resp);
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(device_resp.expires_in);

        int poll_interval = std::max(1, device_resp.interval);

        while (std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::seconds(poll_interval));

            PollStatus status   = PollStatus::Error;
            TokenResponse token = pollForToken(device_resp.device_code, status);

            switch (status) {
                case PollStatus::Authorized: {
                    JWTClaims claims = validateIdToken(token);
                    if (audit_logger_) {
                        nlohmann::json d;
                        d["client_id"] = config_.client_id;
                        audit_logger_->logSecurityEvent(utils::SecurityEventType::TOKEN_CREATED, claims.sub,
                                                        "oauth/device/" + config_.client_id, d);
                    }
                    return claims;
                }

                case PollStatus::AuthorizationPending:
                    spdlog::debug("OAuthDeviceFlow: authorization pending, retrying in {}s", poll_interval);
                    break;

                case PollStatus::SlowDown:
                    poll_interval = std::min(poll_interval + 5, config_.max_poll_interval_seconds);
                    spdlog::debug("OAuthDeviceFlow: slow_down received, new interval={}s", poll_interval);
                    break;

                case PollStatus::AccessDenied:
                case PollStatus::ExpiredToken:
                case PollStatus::Error:
                    // pollForToken has already thrown for AccessDenied/ExpiredToken.
                    // For Error, surface a generic exception.
                    throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "OAuth device flow failed",
                                                  "Unexpected PollStatus during authenticate()"));
            }
        }

        // Deadline exceeded without a response from the user
        spdlog::warn("OAuthDeviceFlow: device code expired (authenticate deadline)");
        throw AuthException(AuthError(AuthErrorCode::AUTH_TOKEN_EXPIRED, "Device authorization timed out",
                                      "User did not authorize within the device code validity period"));
    } catch (const AuthException &ex) {
        if (audit_logger_) {
            const auto &err          = ex.error();
            const std::string reason = err.internalMessage().empty() ? err.publicMessage() : err.internalMessage();
            audit_logger_->logSecurityEvent(utils::SecurityEventType::UNAUTHORIZED_ACCESS, "",
                                            "oauth/device/" + config_.client_id,
                                            {{"client_id", config_.client_id}, {"reason", reason}});
        }
        throw;
    } catch (const std::exception &ex) {
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::UNAUTHORIZED_ACCESS, "",
                                            "oauth/device/" + config_.client_id,
                                            {{"client_id", config_.client_id}, {"reason", std::string(ex.what())}});
        }
        throw;
    }
}

// ============================================================================
// HTTP helper
// ============================================================================

std::string OAuthDeviceFlow::httpPost(const std::string &url, const std::string &body) {
    if (http_post_fn_) {
        return http_post_fn_(url, body);
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl handle");
    }

    std::string response_body;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, oauthDeviceWriteCallback);
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
// URL encoding helpers
// ============================================================================

std::string OAuthDeviceFlow::urlEncode(const std::string &value) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return value;
    }

    char *encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    std::string result;
    if (encoded) {
        result = encoded;
        curl_free(encoded);
    }
    curl_easy_cleanup(curl);
    return result;
}

std::string OAuthDeviceFlow::buildFormBody(const std::vector<std::pair<std::string, std::string>> &params) {
    std::string body;
    for (size_t i = 0; i < params.size(); ++i) {
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
