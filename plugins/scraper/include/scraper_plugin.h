/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            scraper_plugin.h                                   ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 18:45:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 09aaa40562  2026-04-02  feat(scraper): add provenance flags, knowledge_sources.ya... ║
    • c2cc8e90ab  2026-04-02  feat(plugins/scraper): add agentic scraper plugin with go... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "scraper_config.h"
#include "scraper_llm_evaluator.h"
#include "scraper_metadata_writer.h"
#include "scraper_search_engine.h"
#include "scraper_js_renderer.h"
#include "scraper_api_client.h"
#include "gov_source_catalog.h"

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

struct ScraperRunStats {
    int urls_visited      = 0;
    int forms_submitted   = 0;
    int result_pages_parsed = 0;
    int api_pages_fetched = 0;
    int docs_scraped      = 0;
    int docs_accepted     = 0;
    int docs_discarded    = 0;
    int docs_written      = 0;
    int write_errors      = 0;
    long elapsed_ms       = 0;
};

// ============================================================================
// Plugin interface
// ============================================================================

/**
 * @brief Interface for the agentic scraper plugin.
 */
class IScraperPlugin {
public:
    virtual ~IScraperPlugin() = default;

    virtual bool initialize(const ScraperConfig& config) = 0;
    virtual ScraperRunStats scrape() = 0;
    virtual const std::vector<ScrapedDocument>& getResults() const = 0;
    virtual void reset() = 0;
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

    // Dependency injection (for tests)
    void setEvaluator(std::shared_ptr<IScraperLLMEvaluator> e);
    void setWriter(std::shared_ptr<IScraperMetadataWriter> w);
    void setSearchEngine(std::shared_ptr<IScraperSearchEngine> se);
    void setJsRenderer(std::shared_ptr<IScraperJSRenderer> r);
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
