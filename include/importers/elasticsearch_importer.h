#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <string>
#include <vector>
#include <map>

namespace themis {
namespace importers {

/**
 * @brief Elasticsearch Index Importer
 *
 * Migrates documents from an Elasticsearch (or OpenSearch) cluster into
 * ThemisDB collections using the Scroll API for paginated bulk export.
 *
 * Supported capabilities:
 * - Scroll-based bulk migration for arbitrarily large indices
 * - Index mapping → ThemisDB schema conversion (auto-detection)
 * - Multi-index glob pattern support (e.g., "logs-*")
 * - Source filtering: include/exclude specific fields
 * - Type coercion for Elasticsearch field types (keyword, text, integer,
 *   float, date, boolean, geo_point, nested)
 * - Conflict resolution (skip / overwrite / merge) on duplicate `_id`
 * - Structured error reporting via ImportErrorCode
 * - Observability: progress callback, metrics callback, trace spans
 * - Permission-check callback for ACL enforcement
 * - Dry-run mode (validates without writing)
 * - Async import via importDataAsync()
 * - Credential redaction: passwords/API keys never appear in logs or errors
 *
 * Build guard: define @c THEMIS_ENABLE_ELASTICSEARCH to compile the full
 * HTTP-backed implementation.  Without it every @c importData() call returns
 * immediately with @c ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE and a
 * human-readable message describing the missing build flag.
 *
 * @note Thread-safety: ElasticsearchImporter instances are not thread-safe.
 *       Create one instance per import job, or use importDataAsync() which
 *       manages its own thread.
 *
 * Configuration JSON keys (passed to initialize()):
 * @code{.json}
 * {
 *   "host":        "http://localhost:9200",  // required
 *   "index":       "my-index",               // or glob "logs-*"
 *   "api_key":     "...",                    // or "username"/"password"
 *   "username":    "elastic",
 *   "password":    "...",                    // never logged
 *   "scroll_ttl":  "2m",                     // scroll context TTL
 *   "batch_size":  1000,                     // docs per scroll page
 *   "max_retries": 3,
 *   "timeout_ms":  30000
 * }
 * @endcode
 *
 * References:
 *   Elasticsearch Reference – Scroll API (https://www.elastic.co/guide/en/elasticsearch/reference/current/scroll-api.html)
 *   Shay Banon, "Elasticsearch: The Definitive Guide," O'Reilly, 2015.
 */
class ElasticsearchImporter : public IImporter {
public:
    ElasticsearchImporter();
    ~ElasticsearchImporter() override;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    /** @brief Returns "Elasticsearch Importer". */
    const char* getName() const override { return "Elasticsearch Importer"; }

    /**
     * @brief Returns supported source type identifiers.
     * @return {"elasticsearch", "opensearch"}
     */
    std::vector<std::string> getSupportedTypes() const override;

    /**
     * @brief Initializes the importer with a JSON configuration string.
     *
     * Parses host, credentials, scroll parameters.  Credentials are validated
     * at initialization time but are never stored in plaintext in log output.
     *
     * @param config  JSON configuration (see class-level documentation).
     * @return true on success; false if required fields are missing or invalid.
     */
    bool initialize(const std::string& config) override;

    /**
     * @brief Validates that the configured index exists and is accessible.
     *
     * Performs a lightweight HEAD request against the index.  On failure,
     * appends a human-readable diagnostic to @p errors.  Connection strings
     * are sanitised (password replaced with "***") before inclusion in error
     * messages.
     *
     * @param source_path  Elasticsearch index name (overrides config "index" if non-empty).
     * @param errors       Output: list of validation error messages.
     * @return true if the source is reachable and the index exists.
     */
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    /**
     * @brief Imports all documents from the configured index.
     *
     * Uses the Scroll API to paginate through the full index.  Each page is
     * decoded into JSON records and dispatched through the optional
     * @p progress_callback.  Buffer size is bounded to ≤ 64 MB regardless of
     * document count.
     *
     * @param source_path        Index name (empty = use value from initialize()).
     * @param options            Import options (batch size, conflict strategy,
     *                           include/exclude field filters, deadline_ms).
     * @param progress_callback  Optional callback invoked after each scroll page.
     * @return Import statistics (rows_imported, rows_skipped, errors).
     */
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr) override;

    /**
     * @brief Starts an asynchronous import job.
     *
     * Launches a background thread that calls importData().  The returned
     * handle can be polled for progress and completion status.
     *
     * @param source_path  Index name.
     * @param options      Import options.
     * @return Shared handle to the running import job.
     */
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;

    /** @brief Requests cancellation of an in-progress import. */
    void cancel() override;

    /**
     * @brief Returns the index mapping as a ThemisDB schema description.
     *
     * Performs a GET /<index>/_mapping request and converts the Elasticsearch
     * field-type graph into a JSON object compatible with ThemisDB's
     * SchemaInferenceEngine output format.
     *
     * @param source_path  Index name.
     * @return JSON schema description, or an empty object on failure.
     */
    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // Testing support
    // -------------------------------------------------------------------------

    /**
     * @brief Injects a mock HTTP response function for unit testing.
     *
     * When set, the importer calls this function instead of making real HTTP
     * requests.  The injected function receives the full request URL and
     * returns a JSON response body.
     *
     * @note Available in all build configurations (no THEMIS_ENABLE_ELASTICSEARCH guard).
     */
    using MockHttpFn = std::function<std::string(const std::string& url,
                                                  const std::string& body)>;
    void setMockHttpForTesting(MockHttpFn fn);

private:
    struct Config {
        std::string host;
        std::string index;
        std::string api_key;
        std::string username;
        std::string password_redacted; ///< Never the real password; stored as "***"
        std::string scroll_ttl{"2m"};
        int batch_size{1000};
        int max_retries{3};
        uint32_t timeout_ms{30000};
    };

    /// Maps an Elasticsearch field type string to an ImporterFieldType.
    static std::string mapEsTypeToThemisType(const std::string& es_type);

    /// Sanitises a connection URL: replaces password in userinfo with "***".
    static std::string sanitiseUrl(const std::string& url);

    /// Performs a single scroll-page request; returns parsed document array.
    /// Returns empty vector on error; sets @p error_out on failure.
    std::vector<json> fetchScrollPage(const std::string& scroll_id,
                                      std::string& error_out);

    /// Performs the initial scroll search request; returns (scroll_id, first page).
    std::pair<std::string, std::vector<json>> initScroll(
        const std::string& index,
        const ImportOptions& options,
        std::string& error_out);

    /// Deletes the scroll context on the cluster to free server-side resources.
    void clearScroll(const std::string& scroll_id) noexcept;

    Config config_;
    std::atomic<bool> cancelled_{false};
    MockHttpFn mock_http_fn_;

#ifdef THEMIS_ENABLE_ELASTICSEARCH
    /**
     * @brief Performs a single HTTP request via libcurl.
     *
     * Handles GET, HEAD, POST, and DELETE methods.  Authentication headers are
     * set from config_.api_key (Bearer) or config_.username/password_redacted
     * (Basic).  TLS certificate verification is enabled by default.
     *
     * @param method   HTTP method string: "GET", "POST", "HEAD", or "DELETE".
     * @param url      Full target URL.
     * @param body     Request body (empty for GET/HEAD).
     * @return Pair of (HTTP status code, response body).
     *         Status 0 indicates a transport-level failure (curl error).
     */
    std::pair<long, std::string> performHttp(const std::string& method,
                                              const std::string& url,
                                              const std::string& body) const;
#endif
};

} // namespace importers
} // namespace themis
