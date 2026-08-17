/**
 * @file gov_source_catalog.h
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
#include <memory>
#include <functional>
#include <stdexcept>

namespace themis {
namespace scraper {

// ============================================================================
// Source type & search style
// ============================================================================

enum class GovSourceType {
    BUND,         ///< German Federal
    BUNDESLAND,   ///< One of the 16 German states
    EU,           ///< European Union
    OTHER
};

/**
 * @brief How a source is queried.
 *
 * HTML_FORM    – standard HTML search form (POST/GET)
 * REST_JSON    – REST API returning JSON (e.g. Bundesanzeiger API)
 * EURLEX_API   – EUR-Lex CELLAR SPARQL or REST web-service
 * LISTING      – hierarchical index pages (e.g. gesetze-im-internet.de lists)
 * SITEMAP      – XML sitemap driven crawl
 */
enum class GovSearchStyle {
    HTML_FORM,
    REST_JSON,
    EURLEX_API,
    LISTING,
    SITEMAP
};

// ============================================================================
// GovDataSource
// ============================================================================

/**
 * @brief One entry in the government source catalog.
 *
 * Carries everything needed to search and scrape a specific official portal.
 */
struct GovDataSource {
    // --- Identity ---
    std::string id;           ///< Unique key, e.g. "gesetze_im_internet"
    std::string name;         ///< Human-readable name
    GovSourceType type;
    GovSearchStyle search_style;
    std::string bundesland;   ///< ISO 3166-2:DE code for state sources, e.g. "DE-BY"
    std::string language;     ///< Primary language ("de", "en", "fr", …)

    // --- URLs ---
    std::string base_url;
    std::string search_url;   ///< URL of the search endpoint / form page
    std::string sitemap_url;  ///< XML sitemap URL (may be empty)

    // --- Search parameters ---
    std::string search_param;       ///< Query parameter name, e.g. "query" or "q"
    std::string page_param;         ///< Pagination parameter name, e.g. "p" or "page"
    int         results_per_page = 20;
    /// Method for HTML_FORM sources: "GET" or "POST"
    std::string form_method = "GET";
    /// Additional fixed parameters appended to every search request
    std::map<std::string, std::string> extra_params;

    // --- Result parsing hints ---
    /// CSS-like selector hint for the result list container
    std::string result_list_selector;
    /// CSS-like selector for the "next page" link
    std::string next_page_selector;

    // --- API specifics (REST_JSON / EURLEX_API) ---
    std::string api_endpoint;
    std::string api_results_field;  ///< JSON field containing the results array
    std::string api_cursor_field;   ///< JSON field for cursor-based pagination
    std::string api_key_env;        ///< Environment variable holding the API key

    // --- Metadata ---
    std::string notes;
    bool        requires_auth = false;
    bool        enabled       = true;
};

// ============================================================================
// Catalog
// ============================================================================

/**
 * @brief Registry of all known official government and EU data sources.
 *
 * The built-in catalog covers:
 *  - Federal Germany (Bund): 8 portals
 *  - All 16 Bundesländer state law portals
 *  - EU: 5 portals (EUR-Lex, CURIA, europarl.europa.eu, ec.europa.eu,
 *        publications.europa.eu)
 *
 * The catalog can be extended or overridden by loading a custom YAML file.
 */
class GovSourceCatalog {
public:
    GovSourceCatalog();
    ~GovSourceCatalog() = default;

    // Non-copyable, movable
    GovSourceCatalog(const GovSourceCatalog&) = delete;
    GovSourceCatalog& operator=(const GovSourceCatalog&) = delete;
    GovSourceCatalog(GovSourceCatalog&&) noexcept = default;
    GovSourceCatalog& operator=(GovSourceCatalog&&) noexcept = default;

    // --- Querying ---

    /// All sources in the catalog.
    const std::vector<GovDataSource>& all() const;

    /// Look up by unique id; returns nullptr when not found.
    const GovDataSource* findById(const std::string& id) const;

    /// All sources of a given type.
    std::vector<const GovDataSource*> byType(GovSourceType type) const;

    /// All Bundesland sources for a specific state (ISO code).
    std::vector<const GovDataSource*> byBundesland(const std::string& iso) const;

    /// All sources that are currently enabled.
    std::vector<const GovDataSource*> enabled() const;

    /// Filter by a list of explicit IDs (preserves order).
    std::vector<const GovDataSource*> byIds(
        const std::vector<std::string>& ids) const;

    // --- Mutation ---

    /// Add or replace a source.
    void upsert(GovDataSource source);

    /// Enable/disable a source by id.
    bool setEnabled(const std::string& id, bool enabled);

    // --- Persistence ---

    /**
     * @brief Overlay entries from a YAML file on top of the built-in catalog.
     *
     * Unknown sources are added; known sources are merged (only non-empty
     * fields from the YAML override the built-in defaults).
     *
     * @throws std::runtime_error on parse error.
     */
    void loadFromYaml(const std::string& yaml_content);

    /**
     * @brief Load overlay from a file path.
     * @throws std::runtime_error when file cannot be read.
     */
    void loadFromFile(const std::string& path);

private:
    void populateBuiltinBund();
    void populateBuiltinBundeslaender();
    void populateBuiltinEU();

    std::vector<GovDataSource> sources_;
};

} // namespace scraper
} // namespace themis
