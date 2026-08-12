/**
 * @file api_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <memory>
#include <functional>
#include <utility>

namespace themis {
namespace ingestion {

// ApiHttpGetFn is defined in ingestion_manager.h (already included above).

/**
 * @brief Generic REST API source connector
 *
 * Fetches documents from a paginated REST/JSON API endpoint and ingests them
 * into ThemisDB.  Pagination is driven by a configurable cursor/offset
 * parameter; retries use the shared `RetryConfig` back-off logic.
 *
 * Supported `SourceConfig::options` keys:
 * | Key                    | Description                                           | Default        |
 * |------------------------|-------------------------------------------------------|----------------|
 * | `api_key`              | Bearer token sent in `Authorization: Bearer <token>`  | (none)         |
 * | `page_size`            | Items requested per page                              | `100`          |
 * | `cursor_param`         | Query-parameter name for the page cursor / offset     | `offset`       |
 * | `text_field`           | JSON key whose value is treated as the document text  | `text`         |
 * | `max_pages`            | Maximum pages to fetch (0 = unlimited)                | `0`            |
 * | `pagination_mode`      | `"offset"` (numeric) or `"cursor"` (opaque token)     | `offset`       |
 * | `cursor_response_field`| JSON key in the response that contains the next cursor| `next_cursor`  |
 *
 * **Offset mode** (default): each page appends `?<cursor_param>=N&limit=M`
 * where N advances by the number of items received.
 *
 * **Cursor mode**: the first request uses `?limit=M` only; every subsequent
 * request appends `?<cursor_param>=<token>&limit=M` where `<token>` is the
 * value read from `cursor_response_field` in the previous response.
 * Pagination terminates when the response contains no cursor field or the
 * field value is empty.
 *
 * Example usage (offset mode):
 * @code
 * SourceConfig cfg{
 *     .source_id = "my_api",
 *     .type      = SourceType::API,
 *     .location  = "https://api.example.com/v1/documents",
 *     .options   = {{"api_key","secret"},{"page_size","50"},{"text_field","content"}}
 * };
 * GenericApiConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 *
 * Example usage (cursor mode):
 * @code
 * SourceConfig cfg{
 *     .source_id = "cursor_api",
 *     .type      = SourceType::API,
 *     .location  = "https://api.example.com/v2/documents",
 *     .options   = {{"pagination_mode","cursor"},
 *                   {"cursor_param","page_token"},
 *                   {"cursor_response_field","next_page_token"},
 *                   {"page_size","50"}}
 * };
 * GenericApiConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 */
class GenericApiConnector : public ISourceConnector {
public:
    GenericApiConnector();
    ~GenericApiConnector() override;

    // Non-copyable
    GenericApiConnector(const GenericApiConnector&) = delete;
    GenericApiConnector& operator=(const GenericApiConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration
     * @param config  Must have `type == SourceType::API`; `location` is the
     *                base endpoint URL.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /** @brief Poll the endpoint to check availability (HTTP 200) */
    bool isAvailable() const override;

    /**
     * @brief Estimate the total document count
     *
     * Returns the value from a `total` or `count` field in the first-page
     * response, or 0 when the API does not expose a total count.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief Ingest documents from the API endpoint
     *
     * Pages through the endpoint using the configured cursor parameter until
     * no more documents are returned or `max_pages` is reached.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Override the API key (alternative to `options["api_key"]`)
     */
    void setApiKey(const std::string& key);

    /**
     * @brief Override the page size (alternative to `options["page_size"]`)
     */
    void setPageSize(size_t page_size);

    /**
     * @brief Configure retry and timeout behaviour
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Override the pagination mode
     *
     * Equivalent to setting `options["pagination_mode"]` in the source config.
     * Must be called after `initialize()` to take effect.
     *
     * @param mode `PaginationMode::OFFSET` (default) or `PaginationMode::CURSOR`
     */
    void setPaginationMode(PaginationMode mode);

    /**
     * @brief Set the JSON response field that contains the next-page cursor
     *
     * Only relevant in `PaginationMode::CURSOR`.  Equivalent to setting
     * `options["cursor_response_field"]` in the source config.
     *
     * @param field JSON key name (e.g. `"next_cursor"`, `"next_page_token"`)
     */
    void setCursorResponseField(const std::string& field);

    /**
     * @brief Inject a mock HTTP GET function (for unit testing only)
     *
     * When set, every HTTP GET that would normally be performed via libcurl
     * is replaced by a call to @p fn.  Pass an empty `ApiHttpGetFn{}` to
     * restore the real libcurl implementation.
     */
    void setHttpGetForTesting(ApiHttpGetFn fn);

    /**
     * @brief Configure OAuth 2.0 token refresh for this connector
     *
     * When set, the connector will automatically attempt a token refresh
     * (RFC 6749 §6) upon receiving HTTP 401.  The refreshed access token is
     * cached inside the connector and used for all subsequent requests in
     * the same ingestion run.
     *
     * `config.access_token` is used immediately as the Bearer token; if it
     * is empty, the static `api_key` (set via `setApiKey()` or
     * `options["api_key"]`) is used instead until the first refresh.
     *
     * @param config  OAuth configuration including token endpoint, client
     *                credentials, and the refresh token.
     */
    void setOAuthConfig(const OAuthConfig& config);

    /**
     * @brief Inject a mock HTTP POST function for OAuth token refresh (unit testing only)
     *
     * When set, every token-refresh POST that would normally be performed
     * via libcurl is replaced by a call to @p fn.  The function receives the
     * token endpoint URL and the URL-encoded form body, and returns
     * `{status_code, response_body}`.
     *
     * Pass an empty `ApiHttpPostFn{}` to restore the real libcurl path.
     */
    void setHttpPostForTesting(ApiHttpPostFn fn);

    /**
     * @brief Inject a per-document validator called before each write.
     *
     * When set, the validator is called for every document fetched from the
     * API.  Documents that fail validation are counted as failed (not processed).
     * Pass an empty `DocumentValidatorFn` to remove a previously set validator.
     *
     * @param validator Validator callback; empty = disable
     */
    void setDocumentValidator(DocumentValidatorFn validator) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis

