/**
 * @file scraper_search_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
// Search form abstraction
// ============================================================================

/**
 * @brief Represents an HTML search form discovered on a page.
 */
struct SearchForm {
    std::string action_url;    ///< Resolved absolute action URL
    std::string method;        ///< "GET" or "POST" (normalised to upper-case)
    std::string input_name;    ///< name= attribute of the primary search input
    std::map<std::string, std::string> hidden_fields; ///< Hidden <input> fields
    std::string enctype;       ///< Form enctype (default: application/x-www-form-urlencoded)
};

// ============================================================================
// Search result structures
// ============================================================================

/**
 * @brief A single item in a search result list.
 */
struct SearchResultItem {
    int         rank      = 0;
    std::string title;
    std::string url;
    std::string snippet;
    std::string date;
    std::string source_label;   ///< Court name, publisher, etc.
    std::string document_type;  ///< Urteil, Beschluss, Gesetz, …
    std::map<std::string, std::string> extra;  ///< Site-specific metadata
};

/**
 * @brief One page of search results with pagination info.
 */
struct SearchResultPage {
    std::vector<SearchResultItem> items;
    std::string  next_page_url;   ///< Empty when there is no next page
    std::string  prev_page_url;
    int          current_page    = 1;
    int          total_results   = 0;   ///< 0 when not reported by the site
    bool         has_more        = false;
};

// ============================================================================
// Search engine interface
// ============================================================================

/**
 * @brief Interface for discovering search forms and parsing result lists.
 *
 * Two implementations are provided:
 *  - HtmlSearchEngine  – uses pugixml to analyse real HTML
 *  - InMemorySearchEngine – injects controlled HTML for unit tests
 */
class IScraperSearchEngine {
public:
    virtual ~IScraperSearchEngine() = default;

    /**
     * @brief Discover all HTML search forms in a page.
     * @param html      Raw HTML of the page.
     * @param base_url  Absolute URL of the page (used to resolve relative action URLs).
     * @return List of discovered SearchForm objects (may be empty).
     */
    virtual std::vector<SearchForm> discoverForms(
        const std::string& html,
        const std::string& base_url) const = 0;

    /**
     * @brief Parse a search-result page into a structured SearchResultPage.
     *
     * Heuristic detection order:
     *  1. JSON-LD / microdata structured data
     *  2. Common CSS patterns: ol.results li, ul.results li,
     *     div.result, article.result, .search-result, [data-result]
     *  3. Generic list fallback: largest <ul>/<ol> by item count
     *
     * For openjur.de the relevant container is `.result-list > li`.
     *
     * @param html       Raw HTML of the result page.
     * @param base_url   Absolute URL (for resolving relative links).
     * @param selector   Optional CSS-like selector hint (class or id prefix
     *                   such as ".result-list" or "#results"); empty = auto.
     * @return Parsed SearchResultPage.
     */
    virtual SearchResultPage parseResults(
        const std::string& html,
        [[maybe_unused]] const std::string& base_url,
        [[maybe_unused]] const std::string& selector = "") const = 0;

    /**
     * @brief Build the URL (or POST body) to submit a search form.
     * @param form   SearchForm to submit.
     * @param query  The search query string.
     * @param page   1-based page number.
     * @return Fully-qualified URL with query string encoded for GET forms,
     *         or base action URL for POST forms (caller must supply body).
     */
    virtual std::string buildSearchUrl(
        const SearchForm& form,
        const std::string& query,
        int page = 1) const = 0;

    /**
     * @brief Build a POST body for a search form.
     * @return URL-encoded form body.
     */
    virtual std::string buildSearchBody(
        const SearchForm& form,
        const std::string& query,
        int page = 1) const = 0;
};

// ============================================================================
// HTML implementation (pugixml)
// ============================================================================

/**
 * @brief Production HTML search engine backed by pugixml.
 *
 * Compiles and links against pugixml.  When THEMIS_ENABLE_PUGIXML is not
 * defined the class still compiles but discoverForms / parseResults return
 * empty results (for CI environments that don't include pugixml).
 */
class HtmlSearchEngine : public IScraperSearchEngine {
public:
    HtmlSearchEngine() = default;
    ~HtmlSearchEngine() override = default;

    std::vector<SearchForm> discoverForms(
        const std::string& html,
        const std::string& base_url) const override;

    SearchResultPage parseResults(
        const std::string& html,
        [[maybe_unused]] const std::string& base_url,
        [[maybe_unused]] const std::string& selector = "") const override;

    std::string buildSearchUrl(
        const SearchForm& form,
        const std::string& query,
        int page = 1) const override;

    std::string buildSearchBody(
        const SearchForm& form,
        const std::string& query,
        int page = 1) const override;

private:
    /// Resolve a relative URL against base_url.
    static std::string resolveUrl(const std::string& href,
                                  const std::string& base_url);

    /// URL-encode a single string component.
    static std::string urlEncode(const std::string& s);

    /// Detect whether a node looks like the primary search input.
    static bool isSearchInput(const std::string& input_type,
                               const std::string& input_name,
                               const std::string& input_id,
                               const std::string& placeholder);

    /// Collect all plain text from an XML subtree.
    static std::string extractText(const std::string& html_fragment);
};

// ============================================================================
// In-memory mock (tests)
// ============================================================================

/**
 * @brief Test double that returns pre-programmed results.
 */
class InMemorySearchEngine : public IScraperSearchEngine {
public:
    InMemorySearchEngine() = default;

    void injectForms(std::vector<SearchForm> forms) {
        injected_forms_ = std::move(forms);
    }
    void injectResultPage(SearchResultPage page) {
        injected_pages_.push_back(std::move(page));
    }
    void clearInjections() {
        injected_forms_.clear();
        injected_pages_.clear();
        call_count_ = 0;
    }
    int callCount() const { return call_count_; }

    std::vector<SearchForm> discoverForms(
        const std::string& /*html*/,
        const std::string& /*base_url*/) const override {
        ++call_count_;
        return injected_forms_;
    }

    SearchResultPage parseResults(
        const std::string& /*html*/,
        const std::string& /*base_url*/,
        const std::string& /*selector*/ = "") const override {
        ++call_count_;
        if (page_index_ < static_cast<int>(injected_pages_.size())) {
            return injected_pages_[page_index_++];
        }
        return {};
    }

    std::string buildSearchUrl(
        const SearchForm& form,
        const std::string& query,
        int page = 1) const override {
        return form.action_url + "?" + form.input_name + "=" + query
               + "&p=" + std::to_string(page);
    }

    std::string buildSearchBody(
        const SearchForm& form,
        const std::string& query,
        int page = 1) const override {
        return form.input_name + "=" + query + "&p=" + std::to_string(page);
    }

private:
    std::vector<SearchForm>       injected_forms_;
    std::vector<SearchResultPage> injected_pages_;
    mutable int                   call_count_  = 0;
    mutable int                   page_index_  = 0;
};

} // namespace scraper
} // namespace themis

