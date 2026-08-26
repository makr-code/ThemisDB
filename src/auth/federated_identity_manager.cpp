/**
 * @file federated_identity_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/federated_identity_manager.h"

#include <chrono>
#include <curl/curl.h>
#include <iomanip>
#include <list>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// [W8-16] Token cache size cap + SHA-256 key helpers
// ---------------------------------------------------------------------------

/// Maximum number of validated tokens held in the in-memory cache.
/// Entries exceeding this limit are evicted LRU-first.
static constexpr std::size_t kTokenCacheMaxSize = 4096;

/**
 * @brief Compute SHA-256 of @p input and return a 64-char lowercase hex string.
 *
 * Using SHA-256 as the cache key prevents the unbounded growth caused by
 * large JWT strings acting as map keys (W8-16 DoS hardening).
 *
 * @param input  Raw token string (JWT).
 * @return       64-character lowercase hex digest.
 */
static std::string sha256Hex(const std::string& input) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()),
           input.size(), digest);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char byte : digest) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// [W8-16] Private static helpers
// ---------------------------------------------------------------------------

namespace {

size_t federatedCurlWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    const auto total = size * nmemb;
    static_cast<std::string *>(userdata)->append(ptr, total);
    return total;
}

} // anonymous namespace

// static
std::string FederatedIdentityManager::normalize(const std::string &url) {
    std::string s = url;
    while (!s.empty() && s.back() == '/') {
        s.pop_back();
    }
    return s;
}

// static
std::string FederatedIdentityManager::extractIssuer(const std::string &raw_token) {
    // Strip optional "Bearer " prefix
    std::string token = raw_token;
    if (token.size() > 7 && (token.substr(0, 7) == "Bearer " || token.substr(0, 7) == "bearer ")) {
        token = token.substr(7);
    }

    // Reject tokens that exceed the system-wide size limit early, before any
    // allocation – consistent with JWTValidator::parseAndValidate().
    if (token.size() > MAX_JWT_TOKEN_SIZE) {
        throw AuthException(AuthError(AuthErrorCode::JWT_INVALID_FORMAT, "Token exceeds maximum allowed size",
                                      "Token size " + std::to_string(token.size()) + " exceeds limit "
                                          + std::to_string(MAX_JWT_TOKEN_SIZE)));
    }

    // A JWT has the form: <base64url-header>.<base64url-payload>.<signature>
    const auto first_dot = token.find('.');
    if (first_dot == std::string::npos) {
        throw AuthException(
            AuthError(AuthErrorCode::JWT_INVALID_FORMAT, "Token is not a valid JWT", "No dot separator found"));
    }
    const auto second_dot = token.find('.', first_dot + 1);
    if (second_dot == std::string::npos) {
        throw AuthException(
            AuthError(AuthErrorCode::JWT_INVALID_FORMAT, "Token is not a valid JWT", "Only one dot separator found"));
    }

    // Decode the payload section (base64url, no padding)
    std::string b64 = token.substr(first_dot + 1, second_dot - first_dot - 1);

    // Convert base64url to standard base64
    for (char &c : b64) {
        if (c == '-') {
            c = '+';
        } else if (c == '_') {
            c = '/';
        }
    }
    // Add padding
    while (b64.size() % 4 != 0) {
        b64 += '=';
    }

    // Decode base64
    static const std::string b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string decoded;
    decoded.reserve(b64.size() * 3 / 4);

    int val  = 0;
    int bits = -8;
    for (unsigned char c : b64) {
        if (c == '=') {
            break;
        }
        const auto pos = b64_chars.find(static_cast<char>(c));
        if (pos == std::string::npos) {
            continue;
        }
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
    } catch (const std::exception &ex) {
        throw AuthException(AuthError(AuthErrorCode::JWT_INVALID_FORMAT, "Token payload is not valid JSON",
                                      std::string("JSON parse error: ") + ex.what()));
    }

    if (!payload.contains("iss") || !payload["iss"].is_string()) {
        throw AuthException(AuthError(AuthErrorCode::JWT_MISSING_REQUIRED_CLAIM,
                                      "Token is missing required 'iss' claim", "iss claim absent or not a string"));
    }

    return payload["iss"].get<std::string>();
}

// ---------------------------------------------------------------------------
// Realm registration
// ---------------------------------------------------------------------------

void FederatedIdentityManager::addRealm(const OIDCProviderConfig &config) {
    const std::string key = normalize(config.issuer_url);

    if (key.empty()) {
        throw AuthException(
            AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Realm configuration error", "issuer_url must not be empty"));
    }

    // Build adjusted config with normalized issuer_url
    OIDCProviderConfig adjusted = config;
    adjusted.issuer_url         = key;

    auto provider = std::make_shared<OIDCProvider>(adjusted);

    if (http_get_fn_) {
        provider->setHttpGetForTesting(http_get_fn_);
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (realms_.count(key) > 0) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "Realm already registered",
                                      "A realm with issuer '" + key + "' is already registered"));
    }

    realms_.emplace(key, std::move(provider));
    spdlog::info("FederatedIdentityManager: registered realm '{}'", key);
}

bool FederatedIdentityManager::removeRealm(const std::string &issuer_url) {
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

bool FederatedIdentityManager::hasRealm(const std::string &issuer_url) const {
    const std::string key = normalize(issuer_url);
    std::lock_guard<std::mutex> lock(mutex_);
    return realms_.count(key) > 0;
}

std::vector<std::string> FederatedIdentityManager::realmIssuers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> issuers;
    issuers.reserve(realms_.size());
    for (const auto &kv : realms_) {
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

FederatedValidationResult FederatedIdentityManager::validateToken(const std::string &token) {
    // -----------------------------------------------------------------------
    // Fast path: check the in-memory token cache before doing any network I/O.
    // [W8-16] Cache key is SHA-256(token) hex to prevent unbounded key growth
    // from large JWT strings. LRU list tracks recency for eviction.
    // -----------------------------------------------------------------------
    {
        const auto now = std::chrono::system_clock::now();
        const std::string cache_key = sha256Hex(token);
        std::lock_guard<std::mutex> c_lock(cache_mutex_);
        const auto cache_it = token_cache_.find(cache_key);
        if (cache_it != token_cache_.end() && now < cache_it->second.expires_at) {
            // Promote to front of LRU list.
            cache_lru_order_.remove(cache_key);
            cache_lru_order_.push_front(cache_key);
            spdlog::debug("FederatedIdentityManager: cache hit for token sub='{}'",
                          cache_it->second.result.claims.sub);
            return cache_it->second.result;
        }
    }

    // Step 1: peek at the issuer without full validation
    const std::string raw_iss = extractIssuer(token);
    const std::string iss     = normalize(raw_iss);

    // Step 2: find the matching realm (obtain a shared_ptr to keep it alive).
    // Use the canonical FEDERATION_UNKNOWN_REALM code so callers can distinguish
    // "no such realm" from "realm exists but token is cryptographically invalid".
    std::shared_ptr<OIDCProvider> provider;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = realms_.find(iss);
        if (it == realms_.end()) {
            spdlog::warn("FederatedIdentityManager: no realm registered for issuer '{}'", iss);
            throw AuthException(AuthError(AuthErrorCode::FEDERATION_UNKNOWN_REALM,
                                          "Token issuer is not a registered federation realm",
                                          "No realm registered for issuer '" + iss + "'"));
        }
        provider = it->second;
    }

    // Step 3: delegate full validation to the realm's provider.
    // Re-classify network / JWKS-fetch failures as PROVIDER_DEGRADED so callers
    // can apply the fail-closed policy without inspecting provider-internal codes.
    spdlog::debug("FederatedIdentityManager: validating token for realm '{}'", iss);
    JWTClaims claims;
    try {
        claims = provider->validateToken(token);
    } catch (const AuthException &) {
        // Structured auth errors (bad signature, expired, missing claim, …) are
        // already correctly classified — propagate unchanged.
        throw;
    } catch (const std::exception &ex) {
        // Unstructured exceptions from network I/O or internal provider state:
        // reclassify as PROVIDER_DEGRADED (fail-closed).
        spdlog::error("FederatedIdentityManager: provider error for realm '{}': {}", iss, ex.what());
        throw AuthException(AuthError(AuthErrorCode::PROVIDER_DEGRADED,
                                      "Identity provider is temporarily unavailable",
                                      "Provider error for realm '" + iss + "': " + ex.what()));
    }

    // Log JWT success before moving claims
    if (audit_logger_) {
        audit_logger_->logJWTSuccess(claims.sub, claims.jti, iss, "");
    }
    FederatedValidationResult validated{std::move(claims), iss};

    // Populate the token cache so subsequent calls for the same token are fast.
    // [W8-16] Use SHA-256(token) as key; enforce kTokenCacheMaxSize with LRU.
    {
        CachedValidation entry;
        entry.result     = validated;
        entry.expires_at = validated.claims.expiration;
        const std::string cache_key = sha256Hex(token);
        std::lock_guard<std::mutex> c_lock(cache_mutex_);
        // Evict LRU entry if at capacity.
        if (token_cache_.size() >= kTokenCacheMaxSize && token_cache_.count(cache_key) == 0) {
            if (!cache_lru_order_.empty()) {
                token_cache_.erase(cache_lru_order_.back());
                cache_lru_order_.pop_back();
            }
        }
        // Insert or replace entry; move to front of LRU list.
        token_cache_[cache_key] = std::move(entry);
        cache_lru_order_.remove(cache_key);
        cache_lru_order_.push_front(cache_key);
    }

    return validated;
}

OIDCProvider &FederatedIdentityManager::realmProvider(const std::string &issuer_url) {
    const std::string key = normalize(issuer_url);
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = realms_.find(key);
    if (it == realms_.end()) {
        throw AuthException(AuthError(AuthErrorCode::FEDERATION_UNKNOWN_REALM,
                                      "Unknown federation realm",
                                      "No realm registered for issuer '" + key + "'"));
    }
    return *it->second;
}

// ---------------------------------------------------------------------------
// Testing helpers
// ---------------------------------------------------------------------------

void FederatedIdentityManager::setHttpGetForTesting(std::function<std::string(const std::string &url)> fn) {
    http_get_fn_ = std::move(fn);
}

void FederatedIdentityManager::setHttpPostForTesting(
    std::function<std::string(const std::string &url, const std::string &body)> fn) {
    http_post_fn_ = std::move(fn);
}

// ---------------------------------------------------------------------------
// Cross-provider trust registry
// ---------------------------------------------------------------------------

void FederatedIdentityManager::addCrossProviderTrust(const std::string &subject_issuer,
                                                     const std::string &trusting_issuer) {
    const std::string subj = normalize(subject_issuer);
    const std::string trus = normalize(trusting_issuer);
    if (subj.empty() || trus.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID,
                                      "Cross-provider trust registration failed",
                                      "subject_issuer and trusting_issuer must not be empty"));
    }
    std::lock_guard<std::mutex> lock(trust_mutex_);
    trust_map_[trus].insert(subj);
    spdlog::info("FederatedIdentityManager: trust registered: '{}' trusted by '{}'", subj, trus);
}

bool FederatedIdentityManager::removeCrossProviderTrust(const std::string &subject_issuer,
                                                        const std::string &trusting_issuer) {
    const std::string subj = normalize(subject_issuer);
    const std::string trus = normalize(trusting_issuer);
    std::lock_guard<std::mutex> lock(trust_mutex_);
    auto it = trust_map_.find(trus);
    if (it == trust_map_.end()) {
        return false;
    }
    const bool removed = it->second.erase(subj) > 0;
    if (it->second.empty()) {
        trust_map_.erase(it);
    }
    return removed;
}

bool FederatedIdentityManager::isTrustedBy(const std::string &subject_issuer,
                                           const std::string &trusting_issuer) const {
    const std::string subj = normalize(subject_issuer);
    const std::string trus = normalize(trusting_issuer);
    // A realm always implicitly trusts itself.
    if (subj == trus) {
        return true;
    }
    std::lock_guard<std::mutex> lock(trust_mutex_);
    const auto it = trust_map_.find(trus);
    if (it == trust_map_.end()) {
        return false;
    }
    return it->second.count(subj) > 0;
}

std::vector<std::string> FederatedIdentityManager::getCrossProviderTrusts(
    const std::string &trusting_issuer) const {
    const std::string trus = normalize(trusting_issuer);
    std::lock_guard<std::mutex> lock(trust_mutex_);
    const auto it = trust_map_.find(trus);
    if (it == trust_map_.end()) {
        return {};
    }
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

// ---------------------------------------------------------------------------
// In-memory token validation cache
// ---------------------------------------------------------------------------

void FederatedIdentityManager::cacheValidationResult(const std::string &token,
                                                     const FederatedValidationResult &result) {
    CachedValidation entry;
    entry.result     = result;
    entry.expires_at = result.claims.expiration;
    const std::string cache_key = sha256Hex(token);
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // [W8-16] Enforce LRU cap before inserting new entry.
    if (token_cache_.size() >= kTokenCacheMaxSize && token_cache_.count(cache_key) == 0) {
        if (!cache_lru_order_.empty()) {
            token_cache_.erase(cache_lru_order_.back());
            cache_lru_order_.pop_back();
        }
    }
    token_cache_[cache_key] = std::move(entry);
    cache_lru_order_.remove(cache_key);
    cache_lru_order_.push_front(cache_key);
}

std::optional<FederatedValidationResult> FederatedIdentityManager::getCachedResult(
    const std::string &token) const {
    const auto now = std::chrono::system_clock::now();
    const std::string cache_key = sha256Hex(token);
    std::lock_guard<std::mutex> lock(cache_mutex_);
    const auto it = token_cache_.find(cache_key);
    if (it == token_cache_.end()) {
        return std::nullopt;
    }
    if (now >= it->second.expires_at) {
        return std::nullopt;  // expired — evict on next evictExpiredCacheEntries()
    }
    return it->second.result;
}

size_t FederatedIdentityManager::evictExpiredCacheEntries() {
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(cache_mutex_);
    size_t count = 0;
    for (auto it = token_cache_.begin(); it != token_cache_.end(); ) {
        if (now >= it->second.expires_at) {
            // [W8-16] Also remove from LRU order list.
            cache_lru_order_.remove(it->first);
            it = token_cache_.erase(it);
            ++count;
        } else {
            ++it;
        }
    }
    if (count > 0) {
        spdlog::debug("FederatedIdentityManager: evicted {} expired cache entr{}", count,
                      count == 1 ? "y" : "ies");
    }
    return count;
}

void FederatedIdentityManager::clearTokenCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    token_cache_.clear();
    cache_lru_order_.clear(); // [W8-16] Keep LRU list in sync.
}

size_t FederatedIdentityManager::tokenCacheSize() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return token_cache_.size();
}

// ---------------------------------------------------------------------------
// HTTP helpers
// ---------------------------------------------------------------------------

// static
std::string FederatedIdentityManager::buildFormBody(const std::vector<std::pair<std::string, std::string>> &params) {
    // Create a single CURL handle and reuse it for all escape operations,
    // avoiding repeated curl_easy_init/cleanup overhead per parameter.
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl handle for form encoding");
    }

    std::string body;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) {
            body += '&';
        }

        char *enc_key = curl_easy_escape(curl, params[i].first.c_str(), static_cast<int>(params[i].first.size()));
        if (!enc_key) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("curl_easy_escape failed to URL-encode form key");
        }
        body += enc_key;
        curl_free(enc_key);

        body += '=';

        char *enc_val = curl_easy_escape(curl, params[i].second.c_str(), static_cast<int>(params[i].second.size()));
        if (!enc_val) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("curl_easy_escape failed to URL-encode form value");
        }
        body += enc_val;
        curl_free(enc_val);
    }

    curl_easy_cleanup(curl);
    return body;
}

std::string FederatedIdentityManager::httpPost(const std::string &url, const std::string &body) const {
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, federatedCurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    // Always verify TLS certificates
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    struct curl_slist *headers = nullptr;
    headers                    = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

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

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_multi_cleanup(multi);
    curl_easy_cleanup(curl);

    if (mc != CURLM_OK) {
        throw std::runtime_error(std::string("libcurl multi error: ") + curl_multi_strerror(mc));
    }
    if (easy_rc != CURLE_OK) {
        throw std::runtime_error(std::string("libcurl error: ") + curl_easy_strerror(easy_rc));
    }
    if (http_code < 200 || http_code >= 300) {
        throw std::runtime_error("HTTP " + std::to_string(http_code) + " from " + url);
    }

    return response_body;
}

// ---------------------------------------------------------------------------
// RFC 8693 Token Exchange
// ---------------------------------------------------------------------------

TokenExchangeResult FederatedIdentityManager::exchangeToken(const std::string &subject_token,
                                                            const std::string &subject_token_type,
                                                            const std::string &requested_token_type,
                                                            const std::vector<std::string> &target_scopes) {
    // Step 1: strip any "Bearer " prefix so only the raw JWT is forwarded to
    // the IdP's token endpoint (RFC 8693 §2.1 expects the token value, not
    // an Authorization header value).
    std::string raw_subject_token = subject_token;
    if (raw_subject_token.starts_with("Bearer ") || raw_subject_token.starts_with("bearer ")) {
        raw_subject_token = raw_subject_token.substr(7);
    }

    // Step 2: peek at the issuer claim to find the responsible realm
    const std::string raw_iss = extractIssuer(raw_subject_token);
    const std::string iss     = normalize(raw_iss);

    // Step 3: locate the matching realm
    std::shared_ptr<OIDCProvider> provider;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = realms_.find(iss);
        if (it == realms_.end()) {
            spdlog::warn("FederatedIdentityManager::exchangeToken: "
                         "no realm registered for issuer '{}'",
                         iss);
            throw AuthException(AuthError(AuthErrorCode::FEDERATION_UNKNOWN_REALM,
                                          "Token issuer is not a registered federation realm",
                                          "No realm registered for issuer '" + iss + "'"));
        }
        provider = it->second;
    }

    // Step 4: validate the subject token through the realm's JWTValidator
    // pipeline to ensure the caller presents a valid credential before we
    // forward it to the IdP.
    spdlog::debug("FederatedIdentityManager::exchangeToken: "
                  "validating subject token for realm '{}'",
                  iss);
    provider->validateToken(raw_subject_token);

    // Step 5: obtain the token_endpoint from the realm's discovery document
    const std::string token_endpoint = provider->discoveryDocument().token_endpoint;

    if (token_endpoint.empty()) {
        throw AuthException(AuthError(AuthErrorCode::PROVIDER_CAPABILITY_MISMATCH,
                                      "Token exchange not available for this realm",
                                      "Realm '" + iss + "' discovery document does not contain a token_endpoint"));
    }

    // Reject non-HTTPS endpoints to prevent accidental secret leakage over
    // cleartext connections (RFC 8693 §2.1 mandates TLS for the token endpoint).
    if (token_endpoint.compare(0, 8, "https://") != 0) {
        throw AuthException(AuthError(AuthErrorCode::PROVIDER_CAPABILITY_MISMATCH,
                                      "Token exchange requires a secure connection",
                                      "token_endpoint '" + token_endpoint + "' must use HTTPS (RFC 8693 §2.1)"));
    }

    // Step 6: build the RFC 8693 token-exchange POST body
    // (grant_type + subject_token + subject_token_type + requested_token_type
    //  + client_id + optional client_secret + optional scope)
    std::vector<std::pair<std::string, std::string>> params = {
        {"grant_type", "urn:ietf:params:oauth:grant-type:token-exchange"},
        {"subject_token", raw_subject_token},
        {"subject_token_type", subject_token_type},
        {"requested_token_type", requested_token_type},
        {"client_id", provider->clientId()},
    };

    if (!provider->clientSecret().empty()) {
        params.emplace_back("client_secret", provider->clientSecret());
    }

    // Scope the exchanged token to the minimum required permissions
    if (!target_scopes.empty()) {
        std::string scope_str;
        for (size_t i = 0; i < target_scopes.size(); ++i) {
            if (i > 0) {
                scope_str += ' ';
            }
            scope_str += target_scopes[i];
        }
        params.emplace_back("scope", scope_str);
    }

    const std::string form_body = buildFormBody(params);

    // Step 7: POST the token-exchange request to the IdP with retry/backoff (B2)
    spdlog::debug("FederatedIdentityManager::exchangeToken: "
                  "posting to token_endpoint '{}'",
                  token_endpoint);

    std::string response_body;
    {
        constexpr int kMaxRetries = 3;
        constexpr int kBaseDelayMs = 100;
        std::exception_ptr last_exc;
        for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
            try {
                response_body = httpPost(token_endpoint, form_body);
                last_exc = nullptr;
                break;
            } catch (const std::exception &ex) {
                last_exc = std::current_exception();
                const std::string what = ex.what();
                // Retry on connection errors and HTTP 429/503
                const bool retryable = (what.find("HTTP 429") != std::string::npos)
                                     || (what.find("HTTP 503") != std::string::npos)
                                     || (what.find("libcurl") != std::string::npos);
                if (!retryable || attempt + 1 == kMaxRetries) {
                    break;
                }
                const int delay_ms = kBaseDelayMs * (1 << attempt);
                spdlog::warn("FederatedIdentityManager::exchangeToken: "
                             "HTTP POST attempt {} failed ({}), retrying in {}ms",
                             attempt + 1, what, delay_ms);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
        if (last_exc) {
            try { std::rethrow_exception(last_exc); }
            catch (const std::exception &ex) {
                if (audit_logger_) audit_logger_->logJWTFailure("token_exchange_http_error: " + std::string(ex.what()));
                spdlog::error("FederatedIdentityManager::exchangeToken: "
                              "HTTP POST failed after retries: {}",
                              ex.what());
                throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Token exchange request failed",
                                             std::string("HTTP POST error: ") + ex.what()));
            }
        }
    }

    // Step 8: parse the IdP response
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(response_body);
    } catch (const std::exception &ex) {
        spdlog::error("FederatedIdentityManager::exchangeToken: "
                      "failed to parse token response: {}",
                      ex.what());
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Token exchange response is not valid JSON",
                                      std::string("JSON parse error: ") + ex.what()));
    }

    // RFC 6749 / RFC 8693 error response
    if (j.contains("error")) {
        const std::string err  = j.value("error", "");
        const std::string desc = j.value("error_description", "");
        spdlog::warn("FederatedIdentityManager::exchangeToken: "
                     "IdP returned error '{}': {}",
                     err, desc);
        throw AuthException(AuthError(AuthErrorCode::AUTH_INVALID_CREDENTIALS,
                                      "Token exchange denied by identity provider",
                                      "IdP error '" + err + "': " + desc));
    }

    if (!j.contains("access_token") || !j["access_token"].is_string()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR,
                                      "Token exchange response missing access_token",
                                      "IdP response did not contain a string 'access_token' field"));
    }

    TokenExchangeResult result;
    result.access_token      = j["access_token"].get<std::string>();
    result.issued_token_type = j.value("issued_token_type", requested_token_type);
    result.token_type        = j.value("token_type", "Bearer");
    result.expires_in        = j.value("expires_in", 0);
    result.scope             = j.value("scope", "");
    result.realm             = iss;

    // Step 9: validate the exchanged token through the JWTValidator pipeline
    spdlog::debug("FederatedIdentityManager::exchangeToken: "
                  "validating exchanged token for realm '{}'",
                  iss);
    result.claims = provider->validateToken(result.access_token);

    // Step 10: verify that the IdP granted all minimum-required scopes.
    // The scope field is OPTIONAL in the response (RFC 8693 §2.2.1); when
    // present it lists the scopes actually granted, which may be a subset of
    // what was requested.  If it is absent the IdP implicitly confirms that
    // all requested scopes were granted, so we only enforce when it is present.
    if (!target_scopes.empty() && !result.scope.empty()) {
        // Parse the space-separated scope string into a set for O(1) lookup
        std::unordered_set<std::string> granted;
        {
            std::istringstream ss(result.scope);
            std::string tok;
            while (ss >> tok) {
                granted.insert(tok);
            }
        }
        for (const auto &required : target_scopes) {
            if (granted.find(required) == granted.end()) {
                spdlog::warn("FederatedIdentityManager::exchangeToken: "
                             "required scope '{}' not in granted scope '{}' for realm '{}'",
                             required, result.scope, iss);
                throw AuthException(AuthError(
                    AuthErrorCode::AUTH_INSUFFICIENT_PERMISSIONS, "Exchanged token is missing a required scope",
                    "Required scope '" + required + "' was not granted; returned scope: '" + result.scope + "'"));
            }
        }
    }

    spdlog::info("FederatedIdentityManager::exchangeToken: "
                 "token exchange successful for realm '{}', subject='{}'",
                 iss, result.claims.sub);
    if (audit_logger_) {
        audit_logger_->logJWTSuccess(result.claims.sub, result.claims.jti, iss, "");
    }

    return result;
}

} // namespace auth
} // namespace themis
