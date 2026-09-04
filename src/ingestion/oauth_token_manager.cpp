/**
 * @file oauth_token_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "ingestion/oauth_token_manager.h"
#include "utils/logger.h"

#include <chrono>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#ifdef THEMIS_ENABLE_HTTP_CLIENT
#  include <curl/curl.h>
#endif

namespace themis {
namespace ingestion {

using json = nlohmann::json;
using clock = std::chrono::system_clock;

// ─── Default HTTP POST via libcurl ──────────────────────────────────────────

#ifdef THEMIS_ENABLE_HTTP_CLIENT

namespace {

size_t curl_write_cb(void* ptr, size_t sz, size_t nmemb, void* data) {
    auto* buf = static_cast<std::string*>(data);
    buf->append(static_cast<char*>(ptr), sz * nmemb);
    return sz * nmemb;
}

std::pair<int, std::string> default_http_post(
    const std::string& url,
    const std::string& body,
    const std::string& content_type)
{
    CURL* curl = curl_easy_init();
    if (!curl) return {0, ""};

    std::string response_body;
    long http_code = 0;

    const std::string ct_header = "Content-Type: " + content_type;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ct_header.c_str());

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,           1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,     10000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return {static_cast<int>(http_code), response_body};
}

} // anonymous namespace

#else // !THEMIS_ENABLE_HTTP_CLIENT

namespace {
std::pair<int, std::string> default_http_post(
    const std::string& /*url*/,
    const std::string& /*body*/,
    const std::string& /*content_type*/)
{
    return {0, ""};
}
} // anonymous namespace

#endif // THEMIS_ENABLE_HTTP_CLIENT

// ─── OAuthTokenManager implementation ───────────────────────────────────────

OAuthTokenManager::OAuthTokenManager(
    OAuthConfig                    config,
    std::optional<OAuthHttpPostFn> http_post)
    : config_(std::move(config))
    , http_post_(http_post.has_value() ? std::move(*http_post) : default_http_post)
    , expiry_time_(clock::now())    // start expired — first call triggers refresh
{
}

// ─────────────────────────────────────────────────────────────────────────────

std::chrono::system_clock::time_point OAuthTokenManager::getExpiryTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return expiry_time_;
}

void OAuthTokenManager::setTokenForTesting(
    const std::string& access_token,
    const std::string& refresh_token,
    std::chrono::system_clock::time_point expiry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_.access_token  = access_token;
    config_.refresh_token = refresh_token;
    expiry_time_          = expiry;
}

// ─────────────────────────────────────────────────────────────────────────────

bool OAuthTokenManager::isNearExpiry() const {
    // Must be called under mutex_
    const auto now = clock::now();
    const auto secs_until_expiry =
        std::chrono::duration_cast<std::chrono::seconds>(expiry_time_ - now).count();
    return secs_until_expiry < kRefreshThresholdSecs;
}

// ─────────────────────────────────────────────────────────────────────────────

std::string OAuthTokenManager::getAccessToken() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (isNearExpiry()) {
        // Release lock temporarily to allow refreshToken to re-acquire
        // (refreshToken takes the mutex internally via the public API, but
        // here we call the private logic directly while holding the lock).
        // We do the refresh inline since we already hold the lock.
        const auto body = "grant_type=refresh_token"
                          "&client_id=" + config_.client_id +
                          "&client_secret=" + config_.client_secret +
                          "&refresh_token=" + config_.refresh_token;

        auto [code, response] = doHttpPost(body);

        static std::mt19937_64 rng{std::random_device{}()};

        int attempt = 0;
        while (((code == 429 || code == 503) && attempt < kMaxRetries)) {
            const int backoff_base = kBaseBackoffMs * (1 << attempt);
            std::uniform_int_distribution<int> jitter(0, backoff_base / 2);
            const int sleep_ms = backoff_base + jitter(rng);
            THEMIS_WARN("OAuthTokenManager: HTTP {} — backing off {}ms (attempt {}/{})",
                        code, sleep_ms, attempt + 1, kMaxRetries);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            auto [c2, r2] = doHttpPost(body);
            code     = c2;
            response = r2;
            ++attempt;
        }

        if (code == 401) {
            throw OAuthRefreshExpiredError(
                "OAuth refresh token expired (401 from token endpoint)");
        }
        if (code < 200 || code >= 300) {
            throw std::runtime_error(
                "OAuthTokenManager: token refresh failed with HTTP " + std::to_string(code));
        }

        parseTokenResponse(response);
    }
    return config_.access_token;
}

// ─────────────────────────────────────────────────────────────────────────────

void OAuthTokenManager::refreshToken() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto body = "grant_type=refresh_token"
                      "&client_id=" + config_.client_id +
                      "&client_secret=" + config_.client_secret +
                      "&refresh_token=" + config_.refresh_token;

    auto [code, response] = doHttpPost(body);

    static std::mt19937_64 rng{std::random_device{}()};

    int attempt = 0;
    while (((code == 429 || code == 503) && attempt < kMaxRetries)) {
        const int backoff_base = kBaseBackoffMs * (1 << attempt);
        std::uniform_int_distribution<int> jitter(0, backoff_base / 2);
        const int sleep_ms = backoff_base + jitter(rng);
        THEMIS_WARN("OAuthTokenManager: HTTP {} — backing off {}ms (attempt {}/{})",
                    code, sleep_ms, attempt + 1, kMaxRetries);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        auto [c2, r2] = doHttpPost(body);
        code     = c2;
        response = r2;
        ++attempt;
    }

    if (code == 401) {
        throw OAuthRefreshExpiredError(
            "OAuth refresh token expired (401 from token endpoint)");
    }
    if (code < 200 || code >= 300) {
        throw std::runtime_error(
            "OAuthTokenManager: token refresh failed with HTTP " + std::to_string(code));
    }

    parseTokenResponse(response);
}

// ─────────────────────────────────────────────────────────────────────────────

std::string OAuthTokenManager::getBearerAuthorizationHeader() {
    return "Bearer " + getAccessToken();
}

// ─────────────────────────────────────────────────────────────────────────────

std::pair<int, std::string> OAuthTokenManager::doHttpPost(const std::string& body) {
    return http_post_(config_.token_endpoint, body,
                      "application/x-www-form-urlencoded");
}

// ─────────────────────────────────────────────────────────────────────────────

void OAuthTokenManager::parseTokenResponse(const std::string& body) {
    // Must be called while holding mutex_
    try {
        const auto j = json::parse(body);
        if (j.contains("access_token")) {
            config_.access_token = j["access_token"].get<std::string>();
        }
        if (j.contains("refresh_token")) {
            config_.refresh_token = j["refresh_token"].get<std::string>();
        }
        // expires_in is seconds from now; default 3600 if absent
        const int expires_in = j.value("expires_in", 3600);
        expiry_time_ = clock::now() + std::chrono::seconds(expires_in);
        THEMIS_INFO("OAuthTokenManager: token refreshed, expires in {}s", expires_in);
    } catch (const std::exception& e) {
        THEMIS_ERROR("OAuthTokenManager: failed to parse token response: {}", e.what());
        throw std::runtime_error(
            std::string("OAuthTokenManager: invalid token response: ") + e.what());
    }
}

} // namespace ingestion
} // namespace themis

