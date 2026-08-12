/**
 * @file web_crawler_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <utility>

namespace themis {
namespace ingestion {

/**
 * @brief HTTP web crawler and XML sitemap ingestion source connector
 *
 * Crawls web pages starting from a seed URL and ingests their text content
 * into ThemisDB.  Supports both breadth-first crawling and sitemap-driven
 * discovery.  When `THEMIS_ENABLE_CURL` is not defined at compile time the
 * connector still compiles but every live HTTP call is replaced by the
 * injected mock (for unit tests).
 *
 * Sitemap support:
 * - When `follow_sitemaps=true` the connector fetches `/sitemap.xml` from
 *   the seed domain (and any nested `<sitemap>` entries) and adds all
 *   discovered URLs to the crawl queue before starting BFS traversal.
 * - Sitemap index files (`<sitemapindex>`) are recursively resolved up to
 *   a depth of 5 to avoid infinite loops.
 *
 * Robots.txt:
 * - When `respect_robots=true` the connector fetches `/robots.txt` from
 *   the seed domain once and honours `Disallow` rules for the configured
 *   User-Agent.
 *
 * Supported `SourceConfig::options` keys:
 * | Key                | Description                                          | Default   |
 * |--------------------|------------------------------------------------------|-----------|
 * | `max_depth`        | Maximum crawl depth (0 = seed URL only)              | `3`       |
 * | `max_pages`        | Maximum pages to crawl (0 = unlimited)               | `0`       |
 * | `user_agent`       | HTTP User-Agent header value                         | `ThemisDB-Crawler/1.0` |
 * | `follow_sitemaps`  | Parse XML sitemap at /sitemap.xml automatically      | `true`    |
 * | `respect_robots`   | Honour robots.txt disallow rules                     | `true`    |
 * | `same_domain_only` | Follow only URLs on the same domain as the seed      | `true`    |
 *
 * The seed URL is taken from `SourceConfig::location`.
 *
 * Example usage:
 * @code
 * SourceConfig cfg{
 *     .source_id = "docs_site",
 *     .type      = SourceType::WEB_CRAWLER,
 *     .location  = "https://docs.example.com",
 *     .options   = {{"max_depth","2"},{"max_pages","500"}}
 * };
 * WebCrawlerConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 *
 * Example usage via IngestionBuilder:
 * @code
 * auto mgr = IngestionBuilder("mydb")
 *     .withWebCrawlerSource("docs_site",
 *                            "https://docs.example.com",
 *                            {{"max_depth","2"},{"max_pages","500"}})
 *     .build();
 * auto report = mgr->ingestAll();
 * @endcode
 */
class WebCrawlerConnector : public ISourceConnector {
public:
    WebCrawlerConnector();
    ~WebCrawlerConnector() override;

    // Non-copyable
    WebCrawlerConnector(const WebCrawlerConnector&) = delete;
    WebCrawlerConnector& operator=(const WebCrawlerConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration.
     * @param config  Must have `type == SourceType::WEB_CRAWLER`; `location`
     *                is the seed URL or sitemap URL.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /**
     * @brief Check whether the seed URL is reachable (HTTP 2xx response).
     *
     * Returns false when the seed URL is empty or malformed.
     */
    bool isAvailable() const override;

    /**
     * @brief Returns 0 – the total page count is not known before crawling.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief Crawl pages starting from the seed URL and ingest their content.
     *
     * Performs breadth-first crawling up to `max_depth` levels and
     * `max_pages` total pages.  Each page's text content is extracted and
     * accumulated in the returned statistics.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Configure retry behaviour for this connector.
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Function type for injecting mock HTTP responses in unit tests.
     *
     * Receives the URL being fetched; returns `{status_code, body}`.
     * An empty function restores the real libcurl implementation.
     */
    using HttpFetchFn =
        std::function<std::pair<int, std::string>(const std::string& url)>;

    /**
     * @brief Inject a mock HTTP fetch function (unit testing only).
     *
     * When set, every HTTP GET that would normally be performed via libcurl
     * is replaced by a call to @p fn.  Pass an empty `HttpFetchFn{}` to
     * restore the real libcurl implementation.
     */
    void setHttpFetchForTesting(HttpFetchFn fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
