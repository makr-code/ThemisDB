/**
 * @file scraper_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: scraper_plugin.h | Version: 0.0.11
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "scraper/scraper_config.h"
#include "scraper/scraper_llm_evaluator.h"
#include "scraper/scraper_metadata_writer.h"
#include "scraper/scraper_search_engine.h"
#include "scraper/scraper_js_renderer.h"
#include "scraper/scraper_api_client.h"
#include "scraper/gov_source_catalog.h"
#include "scraper/scraper_burst_controller.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

namespace themis {
namespace scraper {

// ============================================================================
// Scraped document (runtime result)
// ============================================================================

/**
 * @brief One document collected during a scraper run.
 *
 * Carries a copy of the provenance fields so that callers can inspect the
 * ingestion origin without looking up the relational record.
 */
struct ScrapedDocument {
    std::string url;
    std::string title;
    std::string raw_html;
    std::string extracted_text;
    std::string source_name;     ///< Gov source id or hostname
    std::string document_type;   ///< "Urteil", "Gesetz", "API", …
    std::string date_issued;
    double      quality_score    = 0.0;
    double      gap_relevance    = 0.0;
    bool        discarded        = false;
    std::string discard_reason;
    std::string doc_id;          ///< Content hash (set after evaluation)
    std::map<std::string, std::string> metadata;

    // ── Provenance (MANDATORY – always set by ScraperPlugin) ──────────────
    /// Always true for documents collected by this plugin.
    bool        is_scraper_ingested      = true;
    /// Fixed literal "SCRAPER".
    std::string ingestion_source_type    = "SCRAPER";
    /// Semver of the scraper plugin that produced this document.
    std::string ingestion_plugin_version = "1.0.0";
};

// ============================================================================
// Run statistics
// ============================================================================

/**
 * @brief Counters and timing returned by ScraperPlugin::scrape().
 *
 * All fields are zero-initialised and incremented atomically during a run.
 * Inspect after scrape() returns to measure throughput, acceptance rate,
 * and error counts.
 */
struct ScraperRunStats {
    int  urls_visited       = 0;  ///< Total URLs fetched (includes discarded and policy-blocked URLs)
    int  forms_submitted    = 0;  ///< HTML search forms submitted with gap keywords
    int  result_pages_parsed= 0;  ///< Search result pages paginated through
    int  api_pages_fetched  = 0;  ///< JSON REST / GraphQL API pages fetched
    int  docs_scraped       = 0;  ///< Raw documents collected before quality filtering
    int  docs_accepted      = 0;  ///< Documents that met the quality threshold
    int  docs_discarded     = 0;  ///< Documents rejected by the LLM/heuristic evaluator
    int  docs_written       = 0;  ///< Documents successfully persisted to the DB
    int  write_errors       = 0;  ///< Persistence failures (run continues despite errors)
    int  urls_skipped       = 0;  ///< URLs skipped due to burst-limiter exhaustion
    long elapsed_ms         = 0;  ///< Total wall-clock duration of the scrape() call
};

// ============================================================================
// Plugin interface
// ============================================================================

/**
 * @brief Interface for the agentic scraper plugin.
 *
 * Lifecycle:
 *  1. Call initialize(config) — returns false on invalid config.
 *  2. Call scrape() — blocks until the run completes.
 *  3. Inspect getResults() for the accepted documents.
 *  4. Call reset() before reusing the same instance with a new config.
 */
class IScraperPlugin {
public:
    virtual ~IScraperPlugin() = default;

    /**
     * @brief Initialise the plugin from a configuration.
     * @param config  Complete scraper configuration (seed URLs, crawl options, …).
     * @return true on success; false when the config is structurally invalid.
     * @throws std::runtime_error  When config.loadFromFile/Yaml parsing fails.
     */
    virtual bool initialize(const ScraperConfig& config) = 0;

    /**
     * @brief Execute the agentic scraper loop.
     *
     * Blocks until all seeds and search forms have been processed or the
     * max_pages limit is reached.  Error isolation guarantees that per-URL
     * failures do not abort the run.
     *
     * @pre initialize() must have been called and returned true.
     * @return Run statistics (URLs visited, documents accepted/discarded, …).
     * @throws std::logic_error  When called before a successful initialize().
     */
    virtual ScraperRunStats scrape() = 0;

    /**
     * @brief Returns all documents collected in the most recent scrape() call.
     *
     * Includes both accepted (discarded=false) and discarded (discarded=true)
     * documents.  The vector is cleared by reset().
     */
    virtual const std::vector<ScrapedDocument>& getResults() const = 0;

    /**
     * @brief Reset state so the plugin can be re-initialised for a new run.
     *
     * Clears results, resets statistics, and sets initialized=false.
     * Injected dependencies (evaluator, writer, …) are preserved.
     */
    virtual void reset() = 0;

    /**
     * @brief Returns true after a successful call to initialize().
     */
    virtual bool isInitialized() const = 0;
};

// ============================================================================
// Production implementation
// ============================================================================

/**
 * @brief Agentic scraper plugin.
 *
 * Agentic loop per run:
 *  1. Expand seed URLs from config + gov source catalog (Bund/Länder/EU).
 *  2. For each seed:
 *     a. Fetch page (STATIC / JS_RENDERED / API_JSON based on render_mode).
 *     b. Discover HTML search forms; submit with gap keywords.
 *     c. Parse result lists; follow pagination up to max_result_pages.
 *     d. For each result URL: fetch and extract text.
 *     e. Apply UrlPolicy filter.
 *  3. Evaluate each document with LLM (or heuristic fallback).
 *  4. Discard documents below quality_threshold.
 *  5. Write accepted documents to DB via IScraperMetadataWriter.
 */
class ScraperPlugin : public IScraperPlugin {
public:
    ScraperPlugin();
    explicit ScraperPlugin(
        std::shared_ptr<IScraperLLMEvaluator>   evaluator,
        std::shared_ptr<IScraperMetadataWriter> writer,
        std::shared_ptr<IScraperSearchEngine>   search_engine,
        std::shared_ptr<IScraperJSRenderer>     js_renderer,
        std::shared_ptr<IScraperApiClient>      api_client);

    ~ScraperPlugin() override = default;

    ScraperPlugin(const ScraperPlugin&) = delete;
    ScraperPlugin& operator=(const ScraperPlugin&) = delete;

    bool initialize(const ScraperConfig& config) override;
    ScraperRunStats scrape() override;
    const std::vector<ScrapedDocument>& getResults() const override;
    void reset() override;
    bool isInitialized() const override;

    // Dependency injection (for tests / custom backends)
    /// Replace the LLM quality evaluator (default: ScraperLLMEvaluator with heuristic fallback).
    void setEvaluator(std::shared_ptr<IScraperLLMEvaluator> e);
    /// Replace the metadata persistence layer (default: InMemoryScraperMetadataWriter).
    void setWriter(std::shared_ptr<IScraperMetadataWriter> w);
    /// Replace the HTML search-form engine (default: HtmlSearchEngine).
    void setSearchEngine(std::shared_ptr<IScraperSearchEngine> se);
    /// Replace the headless JS renderer (default: nullptr — JS_RENDERED mode disabled).
    void setJsRenderer(std::shared_ptr<IScraperJSRenderer> r);
    /// Replace the REST/GraphQL API client (default: HttpScraperApiClient).
    void setApiClient(std::shared_ptr<IScraperApiClient> c);
    /// Inject a custom HTTP fetch function (replaces libcurl in tests).
    using HttpFn = std::function<std::string(const std::string& url,
                                              const std::string& user_agent)>;
    void setHttpFetch(HttpFn fn);

private:
    // --- State ---
    ScraperConfig               config_;
    std::vector<ScrapedDocument> results_;
    ScraperRunStats             stats_;
    bool                        initialized_ = false;
    mutable std::mutex          mutex_;

    // --- Dependencies ---
    std::shared_ptr<IScraperLLMEvaluator>   evaluator_;
    std::shared_ptr<IScraperMetadataWriter> writer_;
    std::shared_ptr<IScraperSearchEngine>   search_engine_;
    std::shared_ptr<IScraperJSRenderer>     js_renderer_;
    std::shared_ptr<IScraperApiClient>      api_client_;
    HttpFn                                  http_fn_;
    GovSourceCatalog                        gov_catalog_;

    // --- Internal methods ---

    /// Collect all effective seed URLs (config seeds + gov catalog sources).
    std::vector<std::pair<std::string, std::string>> collectSeeds() const;
    // returns pairs of (url, gov_source_id)

    /// Fetch page HTML using the configured render mode.
    std::string fetchPage(const std::string& url) const;

    /// Extract plain text from HTML.
    static std::string extractText(const std::string& html);

    /// Evaluate + conditionally store one document.
    void processDocument(const std::string& url,
                         const std::string& html,
                         const std::string& source_name,
                         const std::string& gov_source_id,
                         const std::string& document_type,
                         const std::string& date_issued,
                         const std::string& title_hint = "");

    /// Run the search-form agentic loop for one seed URL.
    void runSearchLoop(const std::string& seed_url,
                       const std::string& page_html,
                       const std::string& source_name,
                       const std::string& gov_source_id);

    /// Run the API crawl loop for one endpoint.
    void runApiLoop(const std::string& endpoint_url,
                    const std::string& source_name,
                    const std::string& gov_source_id);

    /// Build an ApiEndpointConfig from a GovDataSource.
    static ApiEndpointConfig govSourceToApiConfig(const GovDataSource& src);
};

} // namespace scraper
} // namespace themis
