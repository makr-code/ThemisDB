/**
 * @file scraper_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_plugin.h"
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>

#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

#ifdef THEMIS_ENABLE_PUGIXML
#include <pugixml.hpp>
#endif

namespace themis {
namespace scraper {

// ============================================================================
// Construction
// ============================================================================

ScraperPlugin::ScraperPlugin()
    : evaluator_(std::make_shared<ScraperLLMEvaluator>())
    , writer_(std::make_shared<InMemoryScraperMetadataWriter>())
    , search_engine_(std::make_shared<HtmlSearchEngine>())
    , js_renderer_(nullptr)
    , api_client_(std::make_shared<HttpScraperApiClient>()) {}

ScraperPlugin::ScraperPlugin(
    std::shared_ptr<IScraperLLMEvaluator>   evaluator,
    std::shared_ptr<IScraperMetadataWriter> writer,
    std::shared_ptr<IScraperSearchEngine>   search_engine,
    std::shared_ptr<IScraperJSRenderer>     js_renderer,
    std::shared_ptr<IScraperApiClient>      api_client)
    : evaluator_(std::move(evaluator))
    , writer_(std::move(writer))
    , search_engine_(std::move(search_engine))
    , js_renderer_(std::move(js_renderer))
    , api_client_(std::move(api_client)) {}

// ============================================================================
// Dependency injection
// ============================================================================

void ScraperPlugin::setEvaluator(std::shared_ptr<IScraperLLMEvaluator> e)   { evaluator_     = std::move(e); }
void ScraperPlugin::setWriter(std::shared_ptr<IScraperMetadataWriter> w)     { writer_        = std::move(w); }
void ScraperPlugin::setSearchEngine(std::shared_ptr<IScraperSearchEngine> s) { search_engine_ = std::move(s); }
void ScraperPlugin::setJsRenderer(std::shared_ptr<IScraperJSRenderer> r)     { js_renderer_   = std::move(r); }
void ScraperPlugin::setApiClient(std::shared_ptr<IScraperApiClient> c)       { api_client_    = std::move(c); }
void ScraperPlugin::setHttpFetch(HttpFn fn)                                   { http_fn_       = std::move(fn); }
void ScraperPlugin::setBurstController(std::shared_ptr<BurstCrawlController> bc) { burst_controller_ = std::move(bc); }

// ============================================================================
// initialize()
// ============================================================================

bool ScraperPlugin::initialize(const ScraperConfig& config) {
    std::lock_guard<std::mutex> lk(mutex_);
    config_ = config;
    results_.clear();
    stats_ = ScraperRunStats{};

    // Set up JS renderer if configured
    if (config_.crawl_options.render_mode == ScraperRenderMode::JS_RENDERED &&
        !config_.crawl_options.js_renderer_cmd.empty()) {
        if (!js_renderer_ ||
            dynamic_cast<InMemoryJSRenderer*>(js_renderer_.get()) == nullptr) {
            js_renderer_ = std::make_shared<SubprocessJSRenderer>(
                config_.crawl_options.js_renderer_cmd);
        }
    }

    // Load gov catalog overrides
    if (!config_.gov_sources.custom_catalog_path.empty()) {
        try {
            gov_catalog_.loadFromFile(config_.gov_sources.custom_catalog_path);
        } catch (const std::exception& e) {
            // Non-fatal: log and continue with built-in catalog
            (void)e;
        }
    }

    initialized_ = true;
    return true;
}

bool ScraperPlugin::isInitialized() const {
    return initialized_;
}

// ============================================================================
// reset()
// ============================================================================

void ScraperPlugin::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    results_.clear();
    stats_ = ScraperRunStats{};
}

const std::vector<ScrapedDocument>& ScraperPlugin::getResults() const {
    return results_;
}

// ============================================================================
// collectSeeds()
// ============================================================================

std::vector<std::pair<std::string, std::string>> ScraperPlugin::collectSeeds() const {
    std::vector<std::pair<std::string, std::string>> seeds;
    UrlPolicy policy(config_);

    // Explicit seed_urls from config
    for (const auto& url : config_.seed_urls) {
        if (policy.isAllowed(url))
            seeds.emplace_back(url, "");
    }

    // Gov catalog sources
    const auto& gs = config_.gov_sources;
    std::vector<const GovDataSource*> gov_sources;

    if (!gs.source_ids.empty()) {
        gov_sources = gov_catalog_.byIds(gs.source_ids);
    } else {
        if (gs.bund_enabled) {
            for (const auto* s : gov_catalog_.byType(GovSourceType::BUND))
                if (s->enabled) {
                  gov_sources.push_back(s);
                }
        }
        if (gs.bundeslaender_enabled) {
            for (const auto* s : gov_catalog_.byType(GovSourceType::BUNDESLAND))
                if (s->enabled) {
                  gov_sources.push_back(s);
                }
        }
        if (gs.eu_enabled) {
            for (const auto* s : gov_catalog_.byType(GovSourceType::EU))
                if (s->enabled) {
                  gov_sources.push_back(s);
                }
        }
    }

    for (const auto* src : gov_sources) {
        const std::string& url = src->search_url.empty()
                               ? src->base_url : src->search_url;
        if (!url.empty()) {
          seeds.emplace_back(url, src->id);
        }
    }

    return seeds;
}

// ============================================================================
// fetchPage()
// ============================================================================

namespace {
#ifdef THEMIS_ENABLE_CURL
struct CurlBuf {
    std::string data = {};
    static std::size_t write(char* p, std::size_t sz, std::size_t nmemb, void* ud) {
        static_cast<CurlBuf*>(ud)->data.append(p, sz * nmemb);
        return sz * nmemb;
    }
};
#endif
} // anonymous namespace

std::string ScraperPlugin::fetchPage(const std::string& url) const {
    // Injected function takes priority (tests)
    if (http_fn_) {
        try { return http_fn_(url, config_.crawl_options.user_agent); }
        catch (...) { return {}; }
    }

#ifdef THEMIS_ENABLE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    CurlBuf buf;
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      config_.crawl_options.user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  CurlBuf::write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &buf);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        20L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return buf.data;
#else
    return {};
#endif
}

// ============================================================================
// extractText()
// ============================================================================

/*static*/ std::string ScraperPlugin::extractText(const std::string& html) {
#ifdef THEMIS_ENABLE_PUGIXML
    pugi::xml_document doc;
    doc.load_string(html.c_str(), pugi::parse_default | pugi::parse_fragment);
    std::ostringstream out = {};
    std::function<void(const pugi::xml_node&)> walk = [&]([[maybe_unused]] const pugi::xml_node& n) {
        for (const auto& child : n.children()) {
            if (child.type() == pugi::node_pcdata ||
                child.type() == pugi::node_cdata) {
                const char* v = child.value();
                if (v && *v) { out << v << ' '; }
            } else {
                // Skip script/style content
                const std::string name = child.name();
                if (name != "script" && name != "style") {
                  walk(child);
                }
            }
        }
    };
    walk(doc);
    return out.str();
#else
    // Minimal tag stripper
    std::string out = {};
    bool in_tag = false;
    bool in_script = false;
    for (std::size_t i = 0; i <static_cast<int>(html.size()); ++i) {
        if (html[i] == '<') {
            in_tag = true;
            // Detect <script or <style
            if (i + 6 <static_cast<int>(html.size())) {
                std::string tag6 = html.substr(i + 1, 6);
                std::transform(tag6.begin(), tag6.end(), tag6.begin(),
                               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                if (tag6.compare(0, 6, "script") == 0 ||
                    tag6.compare(0, 5, "style") == 0) in_script = true;
                if (tag6.compare(0, 7, "/script") == 0 ||
                    tag6.compare(0, 6, "/style") == 0) in_script = false;
            }
            continue;
        }
        if (html[i] == '>') { in_tag = false; out += ' '; continue; }
        if (!in_tag && !in_script) {
          out += html[i];
        }
    }
    return out;
#endif
}

// ============================================================================
// govSourceToApiConfig()
// ============================================================================

/*static*/ ApiEndpointConfig ScraperPlugin::govSourceToApiConfig(
        const GovDataSource& src) {
    ApiEndpointConfig cfg;
    cfg.url          = src.api_endpoint.empty() ? src.search_url : src.api_endpoint;
    cfg.method       = (src.form_method == "POST") ? "POST" : "GET";
    cfg.search_param = src.search_param;
    cfg.page_param   = src.page_param;
    cfg.results_field = src.api_results_field;
    cfg.cursor_field  = src.api_cursor_field;
    cfg.pagination_mode = src.api_cursor_field.empty() ? "page" : "cursor";
    cfg.page_size    = src.results_per_page;
    if (!src.api_key_env.empty()) {
        const char* key = std::getenv(src.api_key_env.c_str());
        if (key) {
          cfg.headers["Authorization"] = std::string("Bearer ") + key;
        }
    }
    for (const auto& kv : src.extra_params)
        cfg.headers[kv.first] = kv.second;
    return cfg;
}

// ============================================================================
// processDocument()
// ============================================================================

void ScraperPlugin::processDocument(
        const std::string& url,
        const std::string& html,
        const std::string& source_name,
        const std::string& gov_source_id,
        const std::string& document_type,
        const std::string& date_issued,
        const std::string& title_hint) {
    const std::string text = extractText(html);
    if (static_cast<int>(text.size()) < 50) return; // skip near-empty pages

    // LLM evaluation
    const EvaluationResult eval = evaluator_->evaluate(
        text, url, config_.gap_context, config_.llm_options.quality_threshold);

    // Simple title extraction: first non-empty line of text or hint
    std::string title = title_hint;
    if (title.empty()) {
        std::istringstream ss(text);
        std::string line = {};
        while (std::getline(ss, line)) {
            // Trim
            const auto begin = line.find_first_not_of(" \t\r\n");
            if (begin != std::string::npos && (static_cast<int>(line.size()) - begin) > 5) {
                title = line.substr(begin, 120);
                break;
            }
        }
    }

    ScrapedDocument doc;
    doc.url           = url;
    doc.title         = title;
    doc.raw_html      = html;
    doc.extracted_text = text;
    doc.source_name   = source_name;
    doc.document_type = document_type;
    doc.date_issued   = date_issued;
    doc.quality_score = eval.quality_score;
    doc.gap_relevance = eval.gap_relevance;
    doc.discarded     = eval.shouldDiscard();
    doc.discard_reason = eval.discard_reason;

    if (!eval.shouldDiscard()) {
        const auto rel   = ScraperRecordBuilder::buildRelational(
            url, title, text, source_name, gov_source_id, eval, config_.gap_context);
        doc.doc_id = rel.doc_id;
        const auto node  = ScraperRecordBuilder::buildNode(rel);
        const auto edges = ScraperRecordBuilder::buildEdges(rel, eval);
        const auto vec   = ScraperRecordBuilder::buildVector(rel);

        const WriteResult wr = writer_->write(rel, node, edges, vec);
        if (wr.success) {
            ++stats_.docs_written;
        } else {
            ++stats_.write_errors;
        }
        ++stats_.docs_accepted;
    } else {
        ++stats_.docs_discarded;
    }

    ++stats_.docs_scraped;
    results_.push_back(std::move(doc));
}

// ============================================================================
// runSearchLoop()
// ============================================================================

void ScraperPlugin::runSearchLoop(
        const std::string& seed_url,
        const std::string& page_html,
        const std::string& source_name,
        const std::string& gov_source_id) {
    if (!config_.search_options.enabled) {
      return;
    }
    if (!search_engine_) {
      return;
    }

    const auto forms = search_engine_->discoverForms(page_html, seed_url);
    if (forms.empty()) {
      return;
    }

    const auto& form = forms.front(); // use first discovered form
    const auto queries = config_.effectiveSearchQueries();
    const int max_pages = config_.search_options.max_result_pages;
    UrlPolicy policy(config_);

    for (const auto& query : queries) {
        for (int pg = 1; pg <= max_pages; ++pg) {
            const std::string search_url = search_engine_->buildSearchUrl(form, query, pg);
            if (!policy.isAllowed(search_url)) {
              break;
            }

            // Polite delay
            if (pg > 1 && config_.crawl_options.request_delay_ms > 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(config_.crawl_options.request_delay_ms));
            }

            std::string result_html = {};
            if (form.method == "POST") {
                const std::string body = search_engine_->buildSearchBody(form, query, pg);
                // For POST: use api_client with form body
                ApiEndpointConfig api_cfg;
                api_cfg.url          = form.action_url;
                api_cfg.method       = "POST";
                api_cfg.body_template = body;
                api_cfg.pagination_mode = "none";
                // Fetch raw HTML via curl (not JSON), treat as single page
                result_html = fetchPage(search_url);
            } else {
                result_html = fetchPage(search_url);
            }

            if (result_html.empty()) {
              break;
            }
            ++stats_.result_pages_parsed;

            const SearchResultPage result_page =
                search_engine_->parseResults(
                    result_html, search_url,
                    config_.search_options.result_list_selector);

            // Process each result item
            for (const auto& item : result_page.items) {
                if (item.url.empty() || !policy.isAllowed(item.url)) {
                  continue;
                }
                if (config_.crawl_options.request_delay_ms > 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(config_.crawl_options.request_delay_ms));
                }

                std::string doc_html = {};
                if (config_.crawl_options.render_mode == ScraperRenderMode::JS_RENDERED
                    && js_renderer_ && js_renderer_->isAvailable()) {
                    JsRenderRequest jreq;
                    jreq.url        = item.url;
                    jreq.timeout_ms = config_.crawl_options.js_timeout_ms;
                    const auto jres = js_renderer_->render(jreq);
                    if (jres.success) {
                      doc_html = jres.html;
                    }
                } else {
                    doc_html = fetchPage(item.url);
                }

                if (!doc_html.empty()) {
                    ++stats_.urls_visited;
                    processDocument(item.url, doc_html, source_name, gov_source_id,
                                    item.document_type, item.date, item.title);
                }
            }

            ++stats_.forms_submitted;
            if (!result_page.has_more || result_page.next_page_url.empty()) {
              break;
            }
        }
    }
}

// ============================================================================
// runApiLoop()
// ============================================================================

void ScraperPlugin::runApiLoop(
        const std::string& endpoint_url,
        const std::string& source_name,
        const std::string& gov_source_id) {
    if (!api_client_) {
      return;
    }

    ApiEndpointConfig cfg;
    cfg.url = endpoint_url;

    // Check if there is a GovDataSource for this endpoint
    const GovDataSource* src = gov_catalog_.findById(gov_source_id);
    if (src && src->search_style == GovSearchStyle::REST_JSON) {
        cfg = govSourceToApiConfig(*src);
    }

    const auto queries = config_.effectiveSearchQueries();
    UrlPolicy policy(config_);

    for (const auto& query : queries) {
        const auto results = api_client_->fetchAll(cfg, query);
        stats_.api_pages_fetched += static_cast<int>(results.size());
        for (const auto& r : results) {
            if (!policy.isAllowed(r.url) && r.url != endpoint_url) {
              continue;
            }
            ++stats_.docs_scraped;
            ++stats_.urls_visited;
            const EvaluationResult eval = evaluator_->evaluate(
                r.extracted_text, r.url,
                config_.gap_context, config_.llm_options.quality_threshold);
            if (!eval.shouldDiscard()) {
                const auto rel = ScraperRecordBuilder::buildRelational(
                    r.url, r.title, r.extracted_text,
                    source_name, gov_source_id, eval, config_.gap_context);
                ScrapedDocument doc;
                doc.url                     = r.url;
                doc.title                   = r.title;
                doc.extracted_text          = r.extracted_text;
                doc.source_name             = source_name;
                doc.document_type           = "API";
                doc.date_issued             = r.date;
                doc.quality_score           = eval.quality_score;
                doc.gap_relevance           = eval.gap_relevance;
                doc.doc_id                  = rel.doc_id;
                doc.discarded               = false;
                doc.is_scraper_ingested     = true;
                doc.ingestion_source_type   = rel.ingestion_source_type;
                doc.ingestion_plugin_version = rel.ingestion_plugin_version;
                results_.push_back(doc);

                const auto node  = ScraperRecordBuilder::buildNode(rel);
                const auto edges = ScraperRecordBuilder::buildEdges(rel, eval);
                const auto vec   = ScraperRecordBuilder::buildVector(rel);
                const WriteResult wr = writer_->write(rel, node, edges, vec);
                if (wr.success) {
                  ++stats_.docs_written;
                }
                else            ++stats_.write_errors;
                ++stats_.docs_accepted;
            } else {
                ++stats_.docs_discarded;
                ScrapedDocument doc;
                doc.url                     = r.url;
                doc.discarded               = true;
                doc.discard_reason          = eval.discard_reason;
                doc.is_scraper_ingested     = true;
                doc.ingestion_source_type   = "SCRAPER";
                doc.ingestion_plugin_version = ScraperRecordBuilder::kPluginVersion;
                results_.push_back(doc);
            }
        }
    }
}

// ============================================================================
// scrape() – main agentic loop
// ============================================================================

ScraperRunStats ScraperPlugin::scrape() {
    if (!initialized_) {
      throw std::runtime_error("ScraperPlugin not initialized");
    }

    std::lock_guard<std::mutex> lk(mutex_);
    const auto t0 = std::chrono::steady_clock::now();

    const auto seeds = collectSeeds();
    UrlPolicy policy(config_);

    for (const auto& [seed_url, gov_id] : seeds) {
        // Burst-rate guard: skip this seed when the token bucket is exhausted.
        if (burst_controller_ && !burst_controller_->tryAcquire()) {
            ++stats_.urls_skipped;
            continue;
        }

        const GovDataSource* gov_src = gov_id.empty()
                                     ? nullptr
                                     : gov_catalog_.findById(gov_id);

        const std::string source_name = gov_src
            ? gov_src->name
            : ([&]() {
                    // Extract hostname as source name
                    const std::size_t p = seed_url.find("://");
                    if (p == std::string::npos) {
                      return seed_url;
                    }
                    const std::size_t e = seed_url.find('/', p + 3);
                    return e != std::string::npos
                        ? seed_url.substr(p + 3, e - p - 3)
                        : seed_url.substr(p + 3);
                }());

        // REST API sources → API loop
        if (gov_src && gov_src->search_style == GovSearchStyle::REST_JSON) {
            runApiLoop(seed_url, source_name, gov_id);
            continue;
        }
        if (config_.crawl_options.render_mode == ScraperRenderMode::API_JSON ||
            config_.crawl_options.render_mode == ScraperRenderMode::API_GRAPHQL) {
            runApiLoop(seed_url, source_name, gov_id);
            continue;
        }

        // HTML / JS_RENDERED path
        std::string page_html = {};
        if (config_.crawl_options.render_mode == ScraperRenderMode::JS_RENDERED &&
            js_renderer_ && js_renderer_->isAvailable()) {
            JsRenderRequest jreq;
            jreq.url        = seed_url;
            jreq.timeout_ms = config_.crawl_options.js_timeout_ms;
            const auto jres = js_renderer_->render(jreq);
            if (jres.success) {
              page_html = jres.html;
            }
        } else {
            page_html = fetchPage(seed_url);
        }

        if (page_html.empty()) {
          continue;
        }
        ++stats_.urls_visited;

        // Search loop
        if (config_.search_options.enabled) {
            runSearchLoop(seed_url, page_html, source_name, gov_id);
        } else {
            // Direct scrape of the seed page itself
            processDocument(seed_url, page_html, source_name, gov_id, "", "");
        }
    }

    writer_->flush();

    const auto t1 = std::chrono::steady_clock::now();
    stats_.elapsed_ms = static_cast<long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    return stats_;
}

} // namespace scraper
} // namespace themis


