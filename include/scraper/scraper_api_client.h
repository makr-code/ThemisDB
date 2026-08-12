/**
 * @file scraper_api_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace themis {
namespace scraper {

// ============================================================================
// API endpoint configuration
// ============================================================================

/**
 * @brief Configures a single JSON REST or GraphQL API endpoint.
 *
 * Used for:
 *  - Government REST APIs (e.g. Bundesanzeiger, EUR-Lex REST)
 *  - JS-build API endpoints (webpack-dev-server, Vite, Next.js)
 *  - React SPA backends that expose /api/\* routes
 */
struct ApiEndpointConfig {
    std::string url;
    std::string method       = "GET";   ///< "GET" or "POST"

    /// Pagination style: "page" | "cursor" | "offset" | "none"
    std::string pagination_mode   = "page";
    std::string page_param        = "page";
    std::string page_size_param   = "per_page";
    int         page_size         = 20;
    std::string cursor_field;           ///< JSON field for cursor token
    std::string next_url_field;         ///< JSON field containing next-page URL
    std::string results_field   = "results"; ///< JSON field for items array
    std::string total_field     = "total";   ///< JSON field for total count
    int         max_pages       = 50;

    std::map<std::string, std::string> headers;

    /// POST body template; {{QUERY}}, {{PAGE}}, {{CURSOR}} are substituted.
    std::string body_template;

    /// Optional query parameter appended to GET requests: ?<search_param>=<query>
    std::string search_param = "q";
};

// ============================================================================
// API result
// ============================================================================

/**
 * @brief One item collected from a JSON REST or GraphQL API response.
 *
 * Common fields (url, title, date) are extracted from well-known JSON keys;
 * all top-level string values are also stored in fields for downstream access.
 */
struct ApiResult {
    std::string url;             ///< Source endpoint URL (with page params)
    std::string raw_json;        ///< Raw JSON response text
    std::string extracted_text;  ///< Flattened plain text extracted from JSON
    std::string title;
    std::string date;
    std::map<std::string, std::string> fields;  ///< Parsed top-level string fields
};

// ============================================================================
// HTTP fetch function type (injectable for testing)
// ============================================================================

/**
 * @brief Injectable HTTP GET/POST function.
 * Signature: (url, method, headers, body) -> response_body
 * Throws std::runtime_error on network error.
 */
using HttpFetchFn = std::function<std::string(
    const std::string& url,
    const std::string& method,
    const std::map<std::string, std::string>& headers,
    const std::string& body)>;

// ============================================================================
// Interface
// ============================================================================

/**
 * @brief Fetches all pages from a JSON/REST API endpoint.
 *
 * Handles page-based, cursor-based, and offset-based pagination automatically.
 * For GraphQL endpoints the caller supplies a POST body_template with the
 * query; {{QUERY}}, {{PAGE}}, and {{CURSOR}} placeholders are substituted.
 */
class IScraperApiClient {
public:
    virtual ~IScraperApiClient() = default;

    /**
     * @brief Fetch all results from an API endpoint.
     * @param cfg    Endpoint configuration.
     * @param query  Search query (replaces {{QUERY}} in body_template or
     *               appended as search_param to GET URLs).
     * @return All collected results across all pages.
     */
    virtual std::vector<ApiResult> fetchAll(
        const ApiEndpointConfig& cfg,
        const std::string& query) = 0;
};

// ============================================================================
// HTTP implementation (libcurl)
// ============================================================================

/**
 * @brief Production API client backed by libcurl.
 *
 * When THEMIS_ENABLE_CURL is not defined the client still compiles but
 * fetchAll() returns an empty vector (for environments without libcurl).
 */
class HttpScraperApiClient : public IScraperApiClient {
public:
    explicit HttpScraperApiClient(HttpFetchFn fetch_fn = {});
    ~HttpScraperApiClient() override = default;

    HttpScraperApiClient(const HttpScraperApiClient&) = delete;
    HttpScraperApiClient& operator=(const HttpScraperApiClient&) = delete;

    std::vector<ApiResult> fetchAll(
        const ApiEndpointConfig& cfg,
        const std::string& query) override;

private:
    HttpFetchFn fetch_fn_;

    std::string buildGetUrl(const ApiEndpointConfig& cfg,
                            const std::string& query,
                            int page, int offset,
                            const std::string& cursor) const;

    std::string buildBody(const ApiEndpointConfig& cfg,
                          const std::string& query,
                          int page,
                          const std::string& cursor) const;

    /// Parse a JSON array field into ApiResult objects.
    static std::vector<ApiResult> parseResultsArray(
        const std::string& json_text,
        const std::string& results_field,
        const std::string& source_url);

    /// Flatten a JSON value to plain text recursively.
    static std::string flattenJson(const std::string& json_text);

    /// Default libcurl-backed HTTP fetch implementation.
    static std::string curlFetch(
        const std::string& url,
        const std::string& method,
        const std::map<std::string, std::string>& headers,
        const std::string& body);
};

// ============================================================================
// In-memory mock (tests)
// ============================================================================

/**
 * @brief Test double for IScraperApiClient.
 *
 * Returns pre-injected results regardless of the endpoint configuration.
 * Tracks call count and the last query string for test assertions.
 */
class InMemoryScraperApiClient : public IScraperApiClient {
public:
    InMemoryScraperApiClient() = default;

    void injectResults(std::vector<ApiResult> results) {
        injected_ = std::move(results);
    }
    int callCount() const { return call_count_; }
    const std::string& lastQuery() const { return last_query_; }

    std::vector<ApiResult> fetchAll(
        const ApiEndpointConfig& /*cfg*/,
        const std::string& query) override {
        ++call_count_;
        last_query_ = query;
        return injected_;
    }

private:
    std::vector<ApiResult> injected_;
    mutable int call_count_ = 0;
    std::string last_query_;
};

} // namespace scraper
} // namespace themis

