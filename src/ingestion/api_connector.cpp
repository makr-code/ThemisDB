/**
 * @file api_connector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/api_connector.h"
#include <curl/curl.h>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <functional>
#include <fstream>

#ifdef ERROR
#undef ERROR
#endif

// HTTP requests are performed via libcurl (`curl_easy_perform`).
// The `Impl::httpGet()` wrapper delegates to an injectable test function
// when one is set, allowing unit tests to run without network access.

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// HTTP helpers
// ---------------------------------------------------------------------------

namespace {

struct ApiHttpResponse {
    int         status_code = 0;
    std::string body;
    std::string error;
};

// libcurl write callback – appends received data to a std::string.
static size_t apiCurlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                   void* userdata) {
    const auto total = size * nmemb;
    static_cast<std::string*>(userdata)->append(ptr, total);
    return total;
}

// Production HTTP GET using libcurl.
static ApiHttpResponse apiHttpGet(const std::string& url,
                                   const std::string& auth,
                                   int timeout_ms,
                                   const std::string& ca_bundle_path = {}) {
    ApiHttpResponse r;
    CURL* curl = curl_easy_init();
    if (!curl) {
        r.error = "Failed to initialize libcurl handle";
        return r;
    }

    struct curl_slist* headers = nullptr;
    if (!auth.empty()) {
        std::string auth_header = "Authorization: " + auth;
        headers = curl_slist_append(headers, auth_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, apiCurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (!ca_bundle_path.empty()) {
        if (!std::ifstream(ca_bundle_path).good()) {
            if (headers) curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            r.error = "ca_bundle_path not found or not readable: " + ca_bundle_path;
            return r;
        }
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_bundle_path.c_str());
    }

    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        r.error = curl_easy_strerror(res);
        r.status_code = 0;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        r.status_code = static_cast<int>(http_code);
    }

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return r;
}

// Production HTTP POST using libcurl (used for OAuth 2.0 token refresh).
static ApiHttpResponse apiHttpPost(const std::string& url,
                                    const std::string& body,
                                    int timeout_ms,
                                    const std::string& ca_bundle_path = {}) {
    ApiHttpResponse r;
    CURL* curl = curl_easy_init();
    if (!curl) {
        r.error = "Failed to initialize libcurl handle";
        return r;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers,
                                "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, apiCurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    if (!ca_bundle_path.empty()) {
        if (!std::ifstream(ca_bundle_path).good()) {
            if (headers) curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            r.error = "ca_bundle_path not found or not readable: " + ca_bundle_path;
            return r;
        }
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_bundle_path.c_str());
    }

    const CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        r.error = curl_easy_strerror(res);
        r.status_code = 0;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        r.status_code = static_cast<int>(http_code);
    }

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return r;
}

// HTTP GET with exponential back-off retry.
static ApiHttpResponse apiGetWithRetry(const std::string& url,
                                        const std::string& auth,
                                        const RetryConfig& cfg,
                                        IngestionStats& stats,
                                        const std::function<ApiHttpResponse(const std::string&, const std::string&, int)>& http_get) {
    ApiHttpResponse response;
    double delay_ms = cfg.initial_delay_ms;

    for (int attempt = 1; attempt <= cfg.max_attempts; ++attempt) {
        response = http_get(url, auth, cfg.timeout_ms);

        if (response.status_code == 200) return response;

        IngestionErrorCode code = IngestionErrorCode::HTTP_REQUEST_FAILED;
        if (response.status_code == 401 || response.status_code == 403)
            code = IngestionErrorCode::HTTP_UNAUTHORIZED;
        else if (response.status_code == 404)
            code = IngestionErrorCode::HTTP_NOT_FOUND;
        else if (response.status_code == 429)
            code = IngestionErrorCode::HTTP_RATE_LIMITED;
        else if (response.status_code >= 500)
            code = IngestionErrorCode::HTTP_SERVER_ERROR;

        IngestionError err{code, IngestionErrorSeverity::WARNING,
                           "HTTP " + std::to_string(response.status_code) +
                           " on attempt " + std::to_string(attempt) +
                           " for: " + url};
        bool retryable = err.isRetryable();
        stats.errors.push_back(err);
        stats.metrics.error_count++;

        if (!retryable || attempt == cfg.max_attempts) break;

        stats.metrics.retry_count++;
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(delay_ms)));
        delay_ms = std::min(delay_ms * cfg.backoff_factor, cfg.max_delay_ms);
    }
    return response;
}

/// Minimal JSON integer extractor: find first occurrence of `"key":N`
static size_t jsonExtractSizeT(const std::string& json,
                                const std::string& key) {
    std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    // Skip optional whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return 0;
    // Parse digits
    size_t value = 0;
    bool found = false;
    while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
        value = value * 10 + static_cast<size_t>(json[pos] - '0');
        ++pos;
        found = true;
    }
    return found ? value : 0;
}

/// Minimal JSON string-field extractor: collect all values for `"key":"<value>"`
static std::vector<std::string> jsonExtractStringList(const std::string& json,
                                                       const std::string& key) {
    std::vector<std::string> results;
    std::string needle = "\"" + key + "\":\"";
    size_t search_pos = 0;
    while (true) {
        auto start = json.find(needle, search_pos);
        if (start == std::string::npos) break;
        start += needle.size();
        // Collect until unescaped closing quote
        std::string value;
        bool escape = false;
        for (size_t i = start; i < json.size(); ++i) {
            char c = json[i];
            if (escape) { value += c; escape = false; continue; }
            if (c == '\\') { escape = true; continue; }
            if (c == '"') break;
            value += c;
        }
        if (!value.empty()) results.push_back(std::move(value));
        search_pos = start;
    }
    return results;
}

/// Extract the first string value for `"key":"<value>"` from JSON, or "" if absent.
static std::string jsonExtractStringValue(const std::string& json,
                                          const std::string& key) {
    auto list = jsonExtractStringList(json, key);
    return list.empty() ? "" : list[0];
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

/** @brief Pimpl. */
class GenericApiConnector::Impl {
public:
    Impl() : page_size_(100), max_pages_(0),
             pagination_mode_(PaginationMode::OFFSET) {}
    ~Impl() = default;

    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::API) return false;
        config_     = config;
        endpoint_   = config.location;

        auto opt = [&](const std::string& k, const std::string& def) -> std::string {
            auto it = config.options.find(k);
            return (it != config.options.end()) ? it->second : def;
        };

        api_key_      = opt("api_key",      "");
        cursor_param_ = opt("cursor_param", "offset");
        text_field_   = opt("text_field",   "text");

        std::string ps = opt("page_size", "100");
        try { page_size_ = static_cast<size_t>(std::stoul(ps)); }
        catch (...) { page_size_ = 100; }

        std::string mp = opt("max_pages", "0");
        try { max_pages_ = static_cast<size_t>(std::stoul(mp)); }
        catch (...) { max_pages_ = 0; }

        // Pagination mode: "offset" (default) or "cursor"
        std::string pm = opt("pagination_mode", "offset");
        pagination_mode_ = (pm == "cursor") ? PaginationMode::CURSOR
                                            : PaginationMode::OFFSET;

        cursor_response_field_ = opt("cursor_response_field", "next_cursor");

        // OAuth 2.0 configuration from options
        oauth_config_.token_endpoint = opt("oauth_token_endpoint", "");
        oauth_config_.client_id      = opt("oauth_client_id",      "");
        oauth_config_.client_secret  = opt("oauth_client_secret",  "");
        oauth_config_.refresh_token  = opt("oauth_refresh_token",  "");
        oauth_config_.access_token   = opt("oauth_access_token",   "");

        // TLS configuration: optional custom CA bundle path
        retry_config_.ca_bundle_path = opt("ca_bundle_path", "");

        return !endpoint_.empty();
    }

    // Wrapper that delegates to the test hook when set, or real curl otherwise.
    ApiHttpResponse httpGet(const std::string& url, const std::string& auth,
                            int timeout_ms) const {
        if (http_get_fn_) {
            auto [status, body] = http_get_fn_(url, auth);
            ApiHttpResponse r;
            r.status_code = status;
            r.body        = std::move(body);
            return r;
        }
        return apiHttpGet(url, auth, timeout_ms, retry_config_.ca_bundle_path);
    }

    // Wrapper for HTTP POST: delegates to test hook or real libcurl.
    ApiHttpResponse httpPost(const std::string& url, const std::string& body,
                             int timeout_ms) const {
        if (http_post_fn_) {
            auto [status, resp_body] = http_post_fn_(url, body);
            ApiHttpResponse r;
            r.status_code = status;
            r.body        = std::move(resp_body);
            return r;
        }
        return apiHttpPost(url, body, timeout_ms, retry_config_.ca_bundle_path);
    }

    bool isAvailable() const {
        if (endpoint_.empty()) return false;
        try {
            auto r = httpGet(endpoint_, buildAuthHeader(), retry_config_.timeout_ms);
            return r.status_code == 200;
        } catch (...) {
            return false;
        }
    }

    size_t getDocumentCount() const {
        if (endpoint_.empty()) return 0;
        try {
            auto r = httpGet(endpoint_, buildAuthHeader(), retry_config_.timeout_ms);
            if (r.status_code == 200) {
                // Try common total-count fields
                size_t total = jsonExtractSizeT(r.body, "total");
                if (total == 0) total = jsonExtractSizeT(r.body, "count");
                if (total == 0) total = jsonExtractSizeT(r.body, "totalResults");
                return total;
            }
        } catch (...) {}
        return 0;
    }

    IngestionStats ingest(const std::string& /*target_collection*/,
                          ProgressCallback progress_callback) {
        IngestionStats stats;
        auto start_time = std::chrono::steady_clock::now();

        if (endpoint_.empty()) {
            stats.addError(IngestionErrorCode::SOURCE_NOT_CONFIGURED,
                           IngestionErrorSeverity::FATAL,
                           "No endpoint URL configured");
            return stats;
        }

        size_t page_num   = 0;
        size_t total_hint = 0; // populated from first response

        // Offset mode state
        size_t offset = 0;

        // Cursor mode state
        std::string current_cursor; // empty on first page

        // Capture httpGet as a lambda for the retry helper
        auto http_get = [this](const std::string& u, const std::string& a, int t) {
            return httpGet(u, a, t);
        };

        try {
            while (true) {
                if (max_pages_ > 0 && page_num >= max_pages_) break;

                // Build paginated URL
                std::string url = endpoint_;
                url += (url.find('?') == std::string::npos) ? '?' : '&';

                if (pagination_mode_ == PaginationMode::CURSOR) {
                    // Cursor mode: append cursor param only when we have a token
                    url += "limit=" + std::to_string(page_size_);
                    if (!current_cursor.empty()) {
                        url += "&" + cursor_param_ + "=" + current_cursor;
                    }
                } else {
                    // Offset mode (default): numeric offset + limit
                    url += cursor_param_ + "=" + std::to_string(offset);
                    url += "&limit=" + std::to_string(page_size_);
                }

                auto response = apiGetWithRetry(url, buildAuthHeader(),
                                                retry_config_, stats, http_get);

                // OAuth 2.0 token refresh (RFC 6749 §6): on HTTP 401, attempt to
                // refresh the access token once and retry the page request.
                if (response.status_code == 401 && oauth_config_.isRefreshable()) {
                    if (refreshOAuthToken(retry_config_.timeout_ms)) {
                        // Remove the 401 error added by apiGetWithRetry before
                        // retrying so it doesn't inflate the error counter.
                        if (!stats.errors.empty() &&
                            stats.errors.back().code == IngestionErrorCode::HTTP_UNAUTHORIZED) {
                            stats.errors.pop_back();
                            --stats.metrics.error_count;
                        }
                        response = httpGet(url, buildAuthHeader(),
                                           retry_config_.timeout_ms);
                    }
                }

                if (response.status_code != 200) {
                    stats.addError(IngestionErrorCode::HTTP_REQUEST_FAILED,
                                   IngestionErrorSeverity::ERROR,
                                   "Page fetch failed at " +
                                   (pagination_mode_ == PaginationMode::CURSOR
                                        ? "cursor '" + current_cursor + "'"
                                        : "offset " + std::to_string(offset)) +
                                   " (HTTP " +
                                   std::to_string(response.status_code) + ")");
                    break;
                }

                // Extract documents from response body
                auto docs = jsonExtractStringList(response.body, text_field_);
                if (docs.empty()) break; // no more items

                // Update total hint from first page
                if (page_num == 0) {
                    total_hint = jsonExtractSizeT(response.body, "total");
                    if (total_hint == 0)
                        total_hint = jsonExtractSizeT(response.body, "count");
                }

                // Accumulate stats; persistence is delegated to the caller
                // (IngestionManager writes extracted text to the target collection).
                // When a document validator is set, validate each document individually.
                if (document_validator_) {
                    for (const auto& doc : docs) {
                        auto vr = document_validator_(doc);
                        if (vr.is_valid) {
                            ++stats.documents_processed;
                            stats.bytes_processed += doc.size();
                            // reject_invalid=false: log INFO warning when violations present
                            if (!vr.violations.empty()) {
                                ++stats.metrics.schema_violations;
                                stats.addError(
                                    IngestionErrorCode::SCHEMA_VALIDATION_FAILED,
                                    IngestionErrorSeverity::INFO,
                                    "Schema warning – " + vr.summary(),
                                    config_.source_id);
                            }
                        } else {
                            ++stats.documents_failed;
                            ++stats.metrics.schema_violations;
                            stats.addError(
                                IngestionErrorCode::SCHEMA_VALIDATION_FAILED,
                                IngestionErrorSeverity::WARNING,
                                "Schema validation failed – " + vr.summary(),
                                config_.source_id);
                        }
                    }
                } else {
                    stats.documents_processed += docs.size();
                    stats.bytes_processed     += response.body.size();
                }
                ++page_num;

                if (progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      total_hint,
                                      "Page " + std::to_string(page_num));
                }

                if (pagination_mode_ == PaginationMode::CURSOR) {
                    // Advance cursor; stop when the response carries no next cursor
                    current_cursor = jsonExtractStringValue(response.body,
                                                            cursor_response_field_);
                    if (current_cursor.empty()) break;
                } else {
                    // Advance offset; keep paging until the API returns an empty
                    // page or an explicit total has been reached.
                    offset += docs.size();
                    if (total_hint > 0 && offset >= total_hint) break;
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception during API ingestion: " +
                           std::string(e.what()));
        }

        auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end_time - start_time).count();
        if (stats.elapsed_seconds > 0.0 && stats.documents_processed > 0) {
            stats.metrics.throughput_docs_per_sec =
                static_cast<double>(stats.documents_processed) /
                stats.elapsed_seconds;
        }

        return stats;
    }

    void setApiKey(const std::string& key)    { api_key_   = key; }
    void setPageSize(size_t ps)               { page_size_ = ps;  }
    void setRetryConfig(const RetryConfig& c) { retry_config_ = c; }
    void setPaginationMode(PaginationMode m)  { pagination_mode_ = m; }
    void setCursorResponseField(const std::string& f) { cursor_response_field_ = f; }
    void setHttpGetForTesting(ApiHttpGetFn fn) { http_get_fn_ = std::move(fn); }
    void setOAuthConfig(const OAuthConfig& c) { oauth_config_ = c; }
    void setHttpPostForTesting(ApiHttpPostFn fn) { http_post_fn_ = std::move(fn); }
    void setDocumentValidator(DocumentValidatorFn v) { document_validator_ = std::move(v); }

private:
    std::string buildAuthHeader() const {
        // Prefer the OAuth access token when one is available.
        if (!oauth_config_.access_token.empty())
            return "Bearer " + oauth_config_.access_token;
        return api_key_.empty() ? "" : ("Bearer " + api_key_);
    }

    // Percent-encode a string for use in an application/x-www-form-urlencoded body.
    static std::string urlEncode(const std::string& value) {
        std::string encoded;
        for (unsigned char c : value) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += static_cast<char>(c);
            } else {
                char buf[4];
                std::snprintf(buf, sizeof(buf), "%%%02X", c);
                encoded += buf;
            }
        }
        return encoded;
    }

    // Attempt an OAuth 2.0 token refresh (RFC 6749 §6).
    // Returns true and updates oauth_config_.access_token on success.
    bool refreshOAuthToken(int timeout_ms) {
        std::string body = "grant_type=refresh_token"
                           "&refresh_token=" + urlEncode(oauth_config_.refresh_token);
        if (!oauth_config_.client_id.empty())
            body += "&client_id=" + urlEncode(oauth_config_.client_id);
        if (!oauth_config_.client_secret.empty())
            body += "&client_secret=" + urlEncode(oauth_config_.client_secret);

        auto resp = httpPost(oauth_config_.token_endpoint, body, timeout_ms);
        if (resp.status_code != 200) return false;

        std::string new_token = jsonExtractStringValue(resp.body, "access_token");
        if (new_token.empty()) return false;

        oauth_config_.access_token = std::move(new_token);

        // Update the refresh token if the server issued a new one (RFC 6749 §6).
        std::string new_refresh = jsonExtractStringValue(resp.body, "refresh_token");
        if (!new_refresh.empty())
            oauth_config_.refresh_token = std::move(new_refresh);

        return true;
    }

    SourceConfig config_;
    std::string  endpoint_;
    std::string  api_key_;
    std::string  cursor_param_;
    std::string  text_field_;
    size_t       page_size_;
    size_t       max_pages_;
    RetryConfig  retry_config_;
    PaginationMode pagination_mode_;
    std::string  cursor_response_field_;
    ApiHttpGetFn  http_get_fn_;  // testing hook; empty = use real libcurl GET
    ApiHttpPostFn http_post_fn_; // testing hook; empty = use real libcurl POST
    OAuthConfig   oauth_config_; // OAuth 2.0 token refresh configuration
    DocumentValidatorFn document_validator_; ///< Optional per-document validator
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

GenericApiConnector::GenericApiConnector()
    : impl_(std::make_unique<Impl>()) {}

GenericApiConnector::~GenericApiConnector() = default;

bool GenericApiConnector::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool GenericApiConnector::isAvailable() const {
    return impl_->isAvailable();
}

size_t GenericApiConnector::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats GenericApiConnector::ingest(const std::string& target_collection,
                                            ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, progress_callback);
}

void GenericApiConnector::setApiKey(const std::string& key) {
    impl_->setApiKey(key);
}

void GenericApiConnector::setPageSize(size_t page_size) {
    impl_->setPageSize(page_size);
}

void GenericApiConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void GenericApiConnector::setPaginationMode(PaginationMode mode) {
    impl_->setPaginationMode(mode);
}

void GenericApiConnector::setCursorResponseField(const std::string& field) {
    impl_->setCursorResponseField(field);
}

void GenericApiConnector::setHttpGetForTesting(ApiHttpGetFn fn) {
    impl_->setHttpGetForTesting(std::move(fn));
}

void GenericApiConnector::setOAuthConfig(const OAuthConfig& config) {
    impl_->setOAuthConfig(config);
}

void GenericApiConnector::setHttpPostForTesting(ApiHttpPostFn fn) {
    impl_->setHttpPostForTesting(std::move(fn));
}

void GenericApiConnector::setDocumentValidator(DocumentValidatorFn validator) {
    impl_->setDocumentValidator(std::move(validator));
}

} // namespace ingestion
} // namespace themis


