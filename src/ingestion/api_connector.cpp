/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_connector.cpp                                  ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   89.0/100                                       ║
    • Total Lines:     362                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/api_connector.h"
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>

// Note: For actual HTTP requests, libcurl would be used in production (same
// pattern as HuggingFaceConnector).  This implementation provides the full
// pagination/retry structure with a simulated HTTP layer.

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// Minimal simulated HTTP client (shared pattern with HuggingFaceConnector)
// ---------------------------------------------------------------------------

namespace {

struct ApiHttpResponse {
    int         status_code = 0;
    std::string body;
    std::string error;
};

// Simulated HTTP GET – replace body with libcurl in production.
static ApiHttpResponse apiHttpGet(const std::string& /*url*/,
                                   const std::string& /*auth*/,
                                   int /*timeout_ms*/) {
    // Production stub: would do
    //   CURL* curl = curl_easy_init();
    //   curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    //   … Bearer token, timeout, response capture …
    //   curl_easy_perform(curl); curl_easy_cleanup(curl);
    ApiHttpResponse r;
    r.status_code = 200;
    // Simulate a page of 3 documents; real code would parse r.body
    r.body = R"({"total":6,"items":[)"
             R"({"text":"doc alpha"},)"
             R"({"text":"doc beta"},)"
             R"({"text":"doc gamma"}]})";
    return r;
}

// HTTP GET with exponential back-off retry.
static ApiHttpResponse apiGetWithRetry(const std::string& url,
                                        const std::string& auth,
                                        const RetryConfig& cfg,
                                        IngestionStats& stats) {
    ApiHttpResponse response;
    double delay_ms = cfg.initial_delay_ms;

    for (int attempt = 1; attempt <= cfg.max_attempts; ++attempt) {
        response = apiHttpGet(url, auth, cfg.timeout_ms);

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

} // anonymous namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

class GenericApiConnector::Impl {
public:
    Impl() : page_size_(100), max_pages_(0) {}
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

        return !endpoint_.empty();
    }

    bool isAvailable() const {
        if (endpoint_.empty()) return false;
        try {
            auto r = apiHttpGet(endpoint_, buildAuthHeader(), retry_config_.timeout_ms);
            return r.status_code == 200;
        } catch (...) {
            return false;
        }
    }

    size_t getDocumentCount() const {
        if (endpoint_.empty()) return 0;
        try {
            auto r = apiHttpGet(endpoint_, buildAuthHeader(), retry_config_.timeout_ms);
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

        size_t offset     = 0;
        size_t page_num   = 0;
        size_t total_hint = 0; // populated from first response

        try {
            while (true) {
                if (max_pages_ > 0 && page_num >= max_pages_) break;

                // Build paginated URL
                std::string url = endpoint_;
                url += (url.find('?') == std::string::npos) ? '?' : '&';
                url += cursor_param_ + "=" + std::to_string(offset);
                url += "&limit=" + std::to_string(page_size_);

                auto response = apiGetWithRetry(url, buildAuthHeader(),
                                                retry_config_, stats);

                if (response.status_code != 200) {
                    stats.addError(IngestionErrorCode::HTTP_REQUEST_FAILED,
                                   IngestionErrorSeverity::ERROR,
                                   "Page fetch failed at offset " +
                                   std::to_string(offset) + " (HTTP " +
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

                // In production: insert docs into target_collection
                stats.documents_processed += docs.size();
                stats.bytes_processed     += response.body.size();
                offset += docs.size();
                ++page_num;

                if (progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      total_hint,
                                      "Page " + std::to_string(page_num));
                }

                // Stop if we received fewer docs than requested (last page)
                if (docs.size() < page_size_) break;
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

private:
    std::string buildAuthHeader() const {
        return api_key_.empty() ? "" : ("Bearer " + api_key_);
    }

    SourceConfig config_;
    std::string  endpoint_;
    std::string  api_key_;
    std::string  cursor_param_;
    std::string  text_field_;
    size_t       page_size_;
    size_t       max_pages_;
    RetryConfig  retry_config_;
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

} // namespace ingestion
} // namespace themis
