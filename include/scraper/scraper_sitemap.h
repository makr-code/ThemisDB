/**
 * @file scraper_sitemap.h
 * @brief XML sitemap fetching and URL extraction for sitemap-driven crawl mode.
 * @version 1.0.0
 *
 * ## Purpose
 *
 * Provides `SitemapCrawler` — a lightweight XML sitemap parser that extracts
 * `<loc>` URLs from standard sitemaps and sitemap index files.  Supports both
 * single-level sitemaps and two-level sitemap index files (per the
 * sitemaps.org protocol, https://www.sitemaps.org/protocol.html).
 *
 * ## Supported sitemap formats
 * - Standard sitemap:  `<urlset><url><loc>…</loc></url></urlset>`
 * - Sitemap index:     `<sitemapindex><sitemap><loc>…</loc></sitemap></sitemapindex>`
 *   For index files, the crawler fetches each child sitemap and merges its URLs.
 *
 * The parser uses string-based `<loc>` extraction and does not depend on
 * pugixml, so it functions in all build configurations.
 *
 * ## Limits
 * `max_urls_per_sitemap` (default: 5 000) caps how many URLs are returned
 * to protect against memory exhaustion from very large sitemaps.
 *
 * ## Thread safety
 * `SitemapCrawler` is stateless between calls; each `fetchUrls()` call is
 * independent and safe to call from a single thread.
 *
 * @see src/scraper/ROADMAP.md — v1.2 feature: sitemap-driven crawl mode
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace scraper {

// ============================================================================
// SitemapCrawler
// ============================================================================

/**
 * @brief Fetches and parses XML sitemaps into URL lists.
 *
 * Injectable fetch function allows tests to supply sitemap XML without
 * any network I/O.  The default fetch function is `nullptr` (must be set
 * via `ScraperPlugin::setSitemapCrawler()` or the constructor) except when
 * the `ScraperPlugin` supplies the HTTP fetch at runtime.
 */
class SitemapCrawler {
public:
    /**
     * @brief HTTP GET function type.
     *
     * Signature: `(url, user_agent) -> response_body_string`.
     * Throws `std::runtime_error` on network failure (silently skipped by
     * the caller).
     */
    using FetchFn = std::function<std::string(
        const std::string& url,
        const std::string& user_agent)>;

    SitemapCrawler() = default;

    /**
     * @brief Construct with a custom fetch function.
     * @param fetch_fn   HTTP GET implementation.
     * @param max_urls   Maximum URLs returned per sitemap (default: 5 000).
     * @param user_agent User-agent string for HTTP GET requests.
     */
    explicit SitemapCrawler(FetchFn fetch_fn,
                            std::size_t max_urls   = 5000,
                            std::string user_agent = "ThemisDB-Scraper/1.2");

    /**
     * @brief Set the HTTP fetch function.
     * @param fn  Function matching FetchFn; called for each sitemap URL.
     */
    void setFetchFn(FetchFn fn) { fetch_fn_ = std::move(fn); }

    /**
     * @brief Fetch and extract all URLs from a sitemap.
     *
     * Handles both plain sitemaps and sitemap index files.  For index files
     * each child sitemap is fetched once and its `<loc>` entries merged.
     * Errors on individual child sitemaps are silently skipped.
     *
     * @param sitemap_url  URL of the sitemap (or sitemap index) XML file.
     * @return             De-duplicated list of `<loc>` URLs, capped at
     *                     `max_urls_`.  Empty on fetch/parse failure.
     */
    [[nodiscard]] std::vector<std::string> fetchUrls(
        const std::string& sitemap_url) const;

    /**
     * @brief Parse `<loc>` entries from raw sitemap XML.
     *
     * Static utility exposed for unit testing the parser in isolation.
     *
     * @param xml_content  Raw sitemap XML.
     * @return             All `<loc>` values found in @p xml_content.
     */
    [[nodiscard]] static std::vector<std::string> parseLocEntries(
        const std::string& xml_content);

    /**
     * @brief Returns true when @p xml_content is a sitemap index file.
     *
     * Detected by the presence of a `<sitemapindex` root element.
     */
    [[nodiscard]] static bool isSitemapIndex(const std::string& xml_content);

private:
    FetchFn     fetch_fn_;
    std::size_t max_urls_   = 5000;
    std::string user_agent_ = "ThemisDB-Scraper/1.2";
};

} // namespace scraper
} // namespace themis
