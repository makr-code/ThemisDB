/**
 * @file oidc_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/oidc_provider.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>

namespace themis {
namespace auth {

namespace {

size_t oidcWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    const auto total = size * nmemb;
    static_cast<std::string*>(userdata)->append(ptr, total);
    return total;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

OIDCProvider::OIDCProvider(const OIDCProviderConfig& config)
    : config_(config)
{
    if (config_.issuer_url.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "OIDC provider configuration error",
            "issuer_url must not be empty"
        ));
    }
    if (config_.client_id.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "OIDC provider configuration error",
            "client_id must not be empty"
        ));
    }
}

// ---------------------------------------------------------------------------
// Discovery
// ---------------------------------------------------------------------------

void OIDCProvider::discover() {
    // Allow test injection to bypass HTTP
    if (discovery_doc_.has_value()) {
        return; // already fetched (or injected)
    }

    // Strip trailing slash from issuer URL before appending well-known path
    std::string base = config_.issuer_url;
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }

    const std::string discovery_url = base + "/.well-known/openid-configuration";
    spdlog::debug("OIDCProvider: fetching discovery document from {}", discovery_url);

    std::string body;
    try {
        body = httpGet(discovery_url);
    } catch (const std::exception& ex) {
        spdlog::error("OIDCProvider: failed to fetch discovery document: {}", ex.what());
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INTERNAL_ERROR,
            "OIDC discovery failed",
            std::string("HTTP GET error: ") + ex.what()
        ));
    }

    OIDCDiscoveryDocument doc;
    try {
        doc = parseDiscovery(body);
    } catch (const std::exception& ex) {
        spdlog::error("OIDCProvider: failed to parse discovery document: {}", ex.what());
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_INTERNAL_ERROR,
            "OIDC discovery failed",
            std::string("Parse error: ") + ex.what()
        ));
    }

    // Validate that the issuer in the document matches our configured issuer
    // (OpenID Connect Discovery 1.0 §4.3 – issuer MUST match)
    if (doc.issuer != base) {
        spdlog::error("OIDCProvider: issuer mismatch – expected '{}', got '{}'",
                      base, doc.issuer);
        throw AuthException(AuthError(
            AuthErrorCode::JWT_ISSUER_MISMATCH,
            "OIDC issuer mismatch",
            "Discovery document issuer '" + doc.issuer +
                "' does not match configured issuer '" + base + "'"
        ));
    }

    discovery_doc_ = std::move(doc);
    validator_ = std::make_unique<JWTValidator>(buildValidatorConfig());

    spdlog::info("OIDCProvider: discovery complete for issuer '{}'",
                 discovery_doc_->issuer);
}

const OIDCDiscoveryDocument& OIDCProvider::discoveryDocument() {
    if (!discovery_doc_.has_value()) {
        discover();
    }
    return *discovery_doc_;
}

// ---------------------------------------------------------------------------
// Token validation
// ---------------------------------------------------------------------------

JWTClaims OIDCProvider::validateToken(const std::string& token) {
    if (!validator_) {
        discover();
    }
    return validator_->parseAndValidate(token);
}

JWTValidator& OIDCProvider::validator() {
    if (!validator_) {
        discover();
    }
    return *validator_;
}

// ---------------------------------------------------------------------------
// Device flow
// ---------------------------------------------------------------------------

OAuthDeviceFlow OIDCProvider::createDeviceFlow() {
    if (!discovery_doc_.has_value()) {
        discover();
    }

    if (discovery_doc_->device_authorization_endpoint.empty()) {
        throw AuthException(AuthError(
            AuthErrorCode::AUTH_CONFIG_INVALID,
            "OIDC provider does not support device authorization",
            "device_authorization_endpoint is absent from the discovery document"
        ));
    }

    OAuthDeviceFlow::Config cfg;
    cfg.device_authorization_endpoint = discovery_doc_->device_authorization_endpoint;
    cfg.token_endpoint                 = discovery_doc_->token_endpoint;
    cfg.client_id                      = config_.client_id;
    cfg.client_secret                  = config_.client_secret;
    cfg.scopes                         = config_.scopes.empty()
                                           ? std::vector<std::string>{"openid"}
                                           : config_.scopes;
    cfg.jwks_url                       = discovery_doc_->jwks_uri;
    cfg.http_timeout_seconds           = config_.http_timeout_seconds;

    return OAuthDeviceFlow(cfg);
}

// ---------------------------------------------------------------------------
// Testing helpers
// ---------------------------------------------------------------------------

void OIDCProvider::setDiscoveryDocumentForTesting(const OIDCDiscoveryDocument& doc) {
    discovery_doc_ = doc;
    validator_ = std::make_unique<JWTValidator>(buildValidatorConfig());
}

void OIDCProvider::setHttpGetForTesting(
    std::function<std::string(const std::string& url)> fn)
{
    http_get_fn_ = std::move(fn);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string OIDCProvider::httpGet(const std::string& url) const {
    if (http_get_fn_) {
        return http_get_fn_(url);
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize libcurl handle");
    }

    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, oidcWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,
                     static_cast<long>(config_.http_timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,
                     static_cast<long>(config_.http_timeout_seconds));
    // Always verify TLS certificates for security
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Use curl_multi_perform() so this can be driven without blocking the
    // event loop if this function is later migrated to a shared multi-handle.
    CURLM* multi = curl_multi_init();
    if (!multi) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to initialize libcurl multi handle");
    }

    CURLMcode add_rc = curl_multi_add_handle(multi, curl);
    if (add_rc != CURLM_OK) {
        curl_multi_cleanup(multi);
        curl_easy_cleanup(curl);
        throw std::runtime_error(
            std::string("curl_multi_add_handle failed: ") + curl_multi_strerror(add_rc));
    }

    int still_running = 0;
    do {
        CURLMcode mc = curl_multi_perform(multi, &still_running);
        if (mc != CURLM_OK) {
            curl_multi_remove_handle(multi, curl);
            curl_multi_cleanup(multi);
            curl_easy_cleanup(curl);
            throw std::runtime_error(
                std::string("libcurl multi error: ") + curl_multi_strerror(mc));
        }
        if (still_running) {
            mc = curl_multi_wait(multi, nullptr, 0, 1000 /* ms */, nullptr);
            if (mc != CURLM_OK) {
                curl_multi_remove_handle(multi, curl);
                curl_multi_cleanup(multi);
                curl_easy_cleanup(curl);
                throw std::runtime_error(
                    std::string("libcurl multi wait error: ") + curl_multi_strerror(mc));
            }
        }
    } while (still_running);

    // Inspect the per-transfer result via curl_multi_info_read() to get the
    // actionable CURLcode for this easy handle (e.g., DNS/SSL/connect failures
    // would otherwise be masked as HTTP 0).
    CURLcode easy_rc = CURLE_OK;
    {
        CURLMsg* msg = nullptr;
        int msgs_left = 0;
        while ((msg = curl_multi_info_read(multi, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE && msg->easy_handle == curl) {
                easy_rc = msg->data.result;
            }
        }
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_multi_remove_handle(multi, curl);
    curl_multi_cleanup(multi);
    curl_easy_cleanup(curl);

    if (easy_rc != CURLE_OK) {
        throw std::runtime_error(
            std::string("libcurl error: ") + curl_easy_strerror(easy_rc));
    }
    if (http_code != 200) {
        throw std::runtime_error(
            "HTTP " + std::to_string(http_code) + " from " + url);
    }

    return response_body;
}

// static
OIDCDiscoveryDocument OIDCProvider::parseDiscovery(const std::string& json_body) {
    const auto j = nlohmann::json::parse(json_body);
    if (!j.is_object()) {
        throw std::runtime_error("Discovery document is not a JSON object");
    }

    OIDCDiscoveryDocument doc;

    // Required fields (OpenID Connect Discovery 1.0 §3)
    doc.issuer   = j.at("issuer").get<std::string>();
    doc.jwks_uri = j.at("jwks_uri").get<std::string>();

    // Required endpoints
    doc.authorization_endpoint = j.at("authorization_endpoint").get<std::string>();
    doc.token_endpoint         = j.at("token_endpoint").get<std::string>();

    // Optional but commonly present
    if (j.contains("device_authorization_endpoint") &&
        j["device_authorization_endpoint"].is_string())
    {
        doc.device_authorization_endpoint =
            j["device_authorization_endpoint"].get<std::string>();
    }
    if (j.contains("revocation_endpoint") && j["revocation_endpoint"].is_string()) {
        doc.revocation_endpoint = j["revocation_endpoint"].get<std::string>();
    }
    if (j.contains("userinfo_endpoint") && j["userinfo_endpoint"].is_string()) {
        doc.userinfo_endpoint = j["userinfo_endpoint"].get<std::string>();
    }
    if (j.contains("id_token_signing_alg_values_supported") &&
        j["id_token_signing_alg_values_supported"].is_array())
    {
        doc.id_token_signing_alg_values_supported =
            j["id_token_signing_alg_values_supported"]
                .get<std::vector<std::string>>();
    }
    if (j.contains("response_types_supported") &&
        j["response_types_supported"].is_array())
    {
        doc.response_types_supported =
            j["response_types_supported"].get<std::vector<std::string>>();
    }
    if (j.contains("grant_types_supported") &&
        j["grant_types_supported"].is_array())
    {
        doc.grant_types_supported =
            j["grant_types_supported"].get<std::vector<std::string>>();
    }
    if (j.contains("scopes_supported") && j["scopes_supported"].is_array()) {
        doc.scopes_supported =
            j["scopes_supported"].get<std::vector<std::string>>();
    }

    return doc;
}

JWTValidatorConfig OIDCProvider::buildValidatorConfig() const {
    JWTValidatorConfig cfg;
    cfg.jwks_url          = discovery_doc_->jwks_uri;
    cfg.expected_issuer   = discovery_doc_->issuer;
    if (!config_.expected_audience.empty()) {
        cfg.expected_audience = config_.expected_audience;
    } else {
        cfg.require_audience_validation = false;
    }
    cfg.cache_ttl         = config_.jwks_cache_ttl;
    cfg.clock_skew        = config_.clock_skew;
    cfg.jwks_timeout_seconds = config_.http_timeout_seconds;
    return cfg;
}

} // namespace auth
} // namespace themis
