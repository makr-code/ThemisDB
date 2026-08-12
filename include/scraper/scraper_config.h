/**
 * @file scraper_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace themis {
namespace scraper {

// ============================================================================
// Render mode
// ============================================================================

/**
 * @brief Controls how page content is fetched.
 *
 * STATIC       – plain HTTP GET + HTML parsing (libcurl, default)
 * JS_RENDERED  – headless browser renders the page before extracting text
 *                (React / Vue SPAs, webpack-dev-server, Next.js, etc.)
 * API_JSON     – seed URL is a JSON REST endpoint; pages are fetched as JSON
 * API_GRAPHQL  – seed URL is a GraphQL endpoint; queries are sent as POST
 */
enum class ScraperRenderMode {
    STATIC,
    JS_RENDERED,
    API_JSON,
    API_GRAPHQL
};

// ============================================================================
// Gap context
// ============================================================================

/**
 * @brief Describes the data gap that the scraper is trying to fill.
 *
 * Provided to the LLM evaluator so it can score each scraped document
 * against this specific gap rather than performing a generic quality check.
 */
struct GapContext {
    std::string gap_id;                   ///< Unique gap identifier, e.g. "GAP-001"
    std::string description;              ///< Human-readable gap description
    std::vector<std::string> keywords;    ///< Thematic keywords for relevance scoring
};

// ============================================================================
// Sub-configurations
// ============================================================================

struct CrawlOptions {
    int         max_depth        = 3;
    int         max_pages        = 500;
    std::string user_agent       = "ThemisDB-Scraper/1.0";
    bool        respect_robots   = true;
    bool        same_domain_only = true;
    int         request_delay_ms = 250;    ///< Polite delay between requests
    ScraperRenderMode render_mode = ScraperRenderMode::STATIC;
    std::string js_renderer_cmd;           ///< Path/command for headless renderer
    int         js_timeout_ms    = 10000;  ///< Max wait for JS rendering
};

struct SearchOptions {
    bool        enabled          = true;
    /// Explicit queries; when empty, gap_context.keywords are used.
    std::vector<std::string> queries;
    int         max_result_pages = 10;
    int         results_per_page = 20;
    /// CSS selector hint for result list container (empty = auto-detect)
    std::string result_list_selector;
};

struct ApiOptions {
    /// "page" | "cursor" | "offset" | "none"
    std::string pagination_mode  = "page";
    std::string page_param       = "p";
    std::string cursor_field     = "next_cursor";
    std::string results_field    = "results";
    int         max_pages        = 20;
    std::map<std::string, std::string> headers;
    /// POST body template; {{QUERY}} and {{PAGE}} are replaced at runtime.
    std::string body_template;
};

struct LlmOptions {
    double      quality_threshold = 0.65;  ///< Documents below this are discarded
    std::string model_path;                ///< GGUF path; empty = use plugin manager
    double      temperature       = 0.1;
};

// ============================================================================
// Gov sources selection
// ============================================================================

/**
 * @brief Controls which predefined government source groups are activated.
 *
 * Each group maps to entries in the GovSourceCatalog.  An empty list means
 * no predefined sources are used (only whitelist seed_urls are crawled).
 */
struct GovSourcesOptions {
    bool        bund_enabled      = false;
    bool        bundeslaender_enabled = false;
    bool        eu_enabled        = false;
    /// Optional explicit list of source IDs from the catalog (overrides the
    /// group flags when non-empty, e.g. ["gesetze_im_internet","eurlex"]).
    std::vector<std::string> source_ids;
    /// Path to a custom gov_sources YAML file; empty = built-in catalog.
    std::string custom_catalog_path;
};

// ============================================================================
// Main config
// ============================================================================

/**
 * @brief Complete configuration for one scraper run.
 *
 * Load from a YAML file via ScraperConfig::loadFromFile() or parse an
 * in-memory YAML string via ScraperConfig::loadFromYaml().
 */
struct ScraperConfig {
    GapContext         gap_context;
    CrawlOptions       crawl_options;
    SearchOptions      search_options;
    ApiOptions         api_options;
    LlmOptions         llm_options;
    GovSourcesOptions  gov_sources;

    /// Explicit seed URLs to crawl (in addition to gov sources)
    std::vector<std::string> seed_urls;
    /// URL-prefix / glob patterns that must match for a URL to be crawled
    std::vector<std::string> whitelist;
    /// URL-prefix / glob patterns that block a URL even if whitelisted
    std::vector<std::string> blacklist;

    /**
     * @brief Load config from a YAML file on disk.
     * @throws std::runtime_error when the file cannot be read or is invalid.
     */
    static ScraperConfig loadFromFile(const std::string& path);

    /**
     * @brief Parse config from an in-memory YAML string.
     * @throws std::runtime_error when the YAML is invalid.
     */
    static ScraperConfig loadFromYaml(const std::string& yaml_content);

    /**
     * @brief Returns the effective search queries for this run.
     *
     * If search_options.queries is non-empty, those are returned as-is.
     * Otherwise the gap_context keywords are used.
     */
    std::vector<std::string> effectiveSearchQueries() const;
};

// ============================================================================
// URL policy
// ============================================================================

/**
 * @brief Decides whether a URL may be scraped.
 *
 * A URL is allowed when:
 *   1. The whitelist is empty  OR  the URL starts with at least one
 *      whitelist entry (prefix match) or matches a glob pattern (*)
 *   AND
 *   2. The URL does NOT start with any blacklist entry and does not match
 *      any blacklist glob pattern.
 *
 * Scheme must be http or https; any other scheme is rejected regardless of
 * the whitelist to prevent SSRF.
 */
class UrlPolicy {
public:
    explicit UrlPolicy(const ScraperConfig& config);
    UrlPolicy(const std::vector<std::string>& whitelist,
              const std::vector<std::string>& blacklist);

    bool isAllowed(const std::string& url) const;

private:
    static bool matchesPattern(const std::string& url,
                               const std::string& pattern);

    std::vector<std::string> whitelist_;
    std::vector<std::string> blacklist_;
};

} // namespace scraper
} // namespace themis
