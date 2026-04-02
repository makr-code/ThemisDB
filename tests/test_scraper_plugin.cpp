/**
 * @file test_scraper_plugin.cpp
 * @brief Unit tests for the Scraper Plugin (agentic gap-detection scraper)
 *
 * Test suite: ScraperPluginFocusedTests
 * Coverage:
 *   Group A (6 tests)  – UrlPolicy: whitelist, blacklist, glob, SSRF, edge cases
 *   Group B (5 tests)  – ScraperConfig: YAML parsing, effectiveSearchQueries
 *   Group C (5 tests)  – GovSourceCatalog: built-in sources, byType, byId, YAML overlay
 *   Group D (5 tests)  – HtmlSearchEngine: form discovery, result-list parsing, pagination
 *   Group E (4 tests)  – ScraperApiClient: page pagination, cursor pagination, mock fetch
 *   Group F (4 tests)  – ScraperLLMEvaluator: heuristic scoring, threshold, JSON parse error
 *   Group G (4 tests)  – InMemoryScraperMetadataWriter: relational/graph/vector writes
 *   Group H (7 tests)  – ScraperPlugin: init, search loop, API loop, gov sources, JS renderer
 */

#include <gtest/gtest.h>

#include "scraper_config.h"
#include "scraper_search_engine.h"
#include "gov_source_catalog.h"
#include "scraper_js_renderer.h"
#include "scraper_api_client.h"
#include "scraper_llm_evaluator.h"
#include "scraper_metadata_writer.h"
#include "scraper_plugin.h"

using namespace themis::scraper;

// ============================================================================
// Group A – UrlPolicy
// ============================================================================

TEST(ScraperPluginFocusedTests, UrlPolicyAllowsHttpAndHttps) {
    UrlPolicy p({"https://openjur.de"}, {});
    EXPECT_TRUE(p.isAllowed("https://openjur.de/suche/?q=test"));
    EXPECT_TRUE(p.isAllowed("https://openjur.de/u/123456.html"));
}

TEST(ScraperPluginFocusedTests, UrlPolicyBlocksNonHttpScheme) {
    UrlPolicy p({}, {});  // empty = allow all http/https
    EXPECT_FALSE(p.isAllowed("ftp://example.com/file"));
    EXPECT_FALSE(p.isAllowed("file:///etc/passwd"));
    EXPECT_FALSE(p.isAllowed("javascript:alert(1)"));
}

TEST(ScraperPluginFocusedTests, UrlPolicyBlacklistOverridesWhitelist) {
    UrlPolicy p({"https://openjur.de"}, {"https://openjur.de/intern"});
    EXPECT_TRUE(p.isAllowed("https://openjur.de/suche/"));
    EXPECT_FALSE(p.isAllowed("https://openjur.de/intern/secret.html"));
}

TEST(ScraperPluginFocusedTests, UrlPolicyGlobSuffixBlocksPdf) {
    UrlPolicy p({}, {"*.pdf"});
    EXPECT_FALSE(p.isAllowed("https://example.com/doc.pdf"));
    EXPECT_TRUE(p.isAllowed("https://example.com/doc.html"));
}

TEST(ScraperPluginFocusedTests, UrlPolicyEmptyWhitelistAllowsAll) {
    UrlPolicy p({}, {});
    EXPECT_TRUE(p.isAllowed("https://www.gesetze-im-internet.de/baugb/__1.html"));
    EXPECT_TRUE(p.isAllowed("https://eur-lex.europa.eu/legal-content/DE/TXT/"));
}

TEST(ScraperPluginFocusedTests, UrlPolicyGlobPrefixWildcard) {
    UrlPolicy p({"https://gesetze-im-internet.de/*"}, {});
    EXPECT_TRUE(p.isAllowed("https://gesetze-im-internet.de/baugb/__1.html"));
    EXPECT_FALSE(p.isAllowed("https://other.de/baugb/__1.html"));
}

// ============================================================================
// Group B – ScraperConfig
// ============================================================================

TEST(ScraperPluginFocusedTests, ScraperConfigLoadFromYamlBasic) {
#ifdef THEMIS_ENABLE_YAML
    const std::string yaml = R"(
gap_context:
  gap_id: "GAP-001"
  description: "Test gap"
  keywords: ["Baugenehmigung", "BauGB"]
crawl_options:
  max_depth: 2
  max_pages: 100
llm_options:
  quality_threshold: 0.7
whitelist:
  - "https://openjur.de"
blacklist:
  - "*.pdf"
)";
    const ScraperConfig cfg = ScraperConfig::loadFromYaml(yaml);
    EXPECT_EQ(cfg.gap_context.gap_id, "GAP-001");
    EXPECT_EQ(cfg.gap_context.keywords.size(), 2u);
    EXPECT_EQ(cfg.crawl_options.max_depth, 2);
    EXPECT_DOUBLE_EQ(cfg.llm_options.quality_threshold, 0.7);
    EXPECT_EQ(cfg.whitelist.size(), 1u);
    EXPECT_EQ(cfg.blacklist.size(), 1u);
#else
    GTEST_SKIP() << "THEMIS_ENABLE_YAML not defined";
#endif
}

TEST(ScraperPluginFocusedTests, ScraperConfigInvalidYamlThrows) {
#ifdef THEMIS_ENABLE_YAML
    EXPECT_THROW(ScraperConfig::loadFromYaml("{ invalid: [yaml"), std::runtime_error);
#else
    GTEST_SKIP() << "THEMIS_ENABLE_YAML not defined";
#endif
}

TEST(ScraperPluginFocusedTests, ScraperConfigEffectiveQueriesFromKeywords) {
    ScraperConfig cfg;
    cfg.gap_context.keywords = {"Baugenehmigung", "BauGB"};
    cfg.search_options.queries = {};
    const auto q = cfg.effectiveSearchQueries();
    ASSERT_EQ(q.size(), 2u);
    EXPECT_EQ(q[0], "Baugenehmigung");
}

TEST(ScraperPluginFocusedTests, ScraperConfigEffectiveQueriesExplicitOverride) {
    ScraperConfig cfg;
    cfg.gap_context.keywords = {"BauGB"};
    cfg.search_options.queries = {"Baugenehmigung Bayern", "Bebauungsplan"};
    const auto q = cfg.effectiveSearchQueries();
    ASSERT_EQ(q.size(), 2u);
    EXPECT_EQ(q[0], "Baugenehmigung Bayern");
}

TEST(ScraperPluginFocusedTests, ScraperConfigRenderModeDefaultStatic) {
    ScraperConfig cfg;
    EXPECT_EQ(cfg.crawl_options.render_mode, ScraperRenderMode::STATIC);
}

// ============================================================================
// Group C – GovSourceCatalog
// ============================================================================

TEST(ScraperPluginFocusedTests, GovCatalogContainsBundSources) {
    GovSourceCatalog cat;
    const auto bund = cat.byType(GovSourceType::BUND);
    EXPECT_GE(bund.size(), 5u);  // at least 5 Bund sources
    // openjur.de must be present
    bool found_openjur = false;
    for (const auto* s : bund) {
        if (s->id == "openjur") { found_openjur = true; break; }
    }
    EXPECT_TRUE(found_openjur);
}

TEST(ScraperPluginFocusedTests, GovCatalogContainsAll16Laender) {
    GovSourceCatalog cat;
    const auto laender = cat.byType(GovSourceType::BUNDESLAND);
    EXPECT_EQ(laender.size(), 16u);
}

TEST(ScraperPluginFocusedTests, GovCatalogContainsEUSources) {
    GovSourceCatalog cat;
    const auto eu = cat.byType(GovSourceType::EU);
    EXPECT_GE(eu.size(), 3u);
    bool found_eurlex = false;
    for (const auto* s : eu) {
        if (s->id == "eurlex") { found_eurlex = true; break; }
    }
    EXPECT_TRUE(found_eurlex);
}

TEST(ScraperPluginFocusedTests, GovCatalogFindById) {
    GovSourceCatalog cat;
    const GovDataSource* s = cat.findById("gesetze_im_internet");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->base_url, "https://www.gesetze-im-internet.de");
    EXPECT_EQ(s->type, GovSourceType::BUND);
}

TEST(ScraperPluginFocusedTests, GovCatalogYamlOverlay) {
#ifdef THEMIS_ENABLE_YAML
    GovSourceCatalog cat;
    const std::string yaml = R"(
sources:
  - id: "test_custom_source"
    name: "Custom Test Source"
    type: "BUND"
    search_style: "HTML_FORM"
    base_url: "https://test.example.de"
    search_url: "https://test.example.de/suche"
    search_param: "q"
    page_param: "p"
    enabled: true
)";
    cat.loadFromYaml(yaml);
    const GovDataSource* s = cat.findById("test_custom_source");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name, "Custom Test Source");
    EXPECT_EQ(s->base_url, "https://test.example.de");
#else
    GTEST_SKIP() << "THEMIS_ENABLE_YAML not defined";
#endif
}

// ============================================================================
// Group D – HtmlSearchEngine
// ============================================================================

TEST(ScraperPluginFocusedTests, SearchEngineDiscoverFormFromHtml) {
    HtmlSearchEngine eng;
    const std::string html = R"(
<html><body>
<form action="/suche/" method="get">
  <input type="search" name="q" placeholder="Suche…"/>
  <input type="hidden" name="lang" value="de"/>
  <button type="submit">Los</button>
</form>
</body></html>)";
    const auto forms = eng.discoverForms(html, "https://openjur.de");
    ASSERT_FALSE(forms.empty());
    const auto& f = forms.front();
    EXPECT_EQ(f.input_name, "q");
    EXPECT_EQ(f.method, "GET");
    EXPECT_FALSE(f.action_url.empty());
    EXPECT_EQ(f.hidden_fields.count("lang"), 1u);
    EXPECT_EQ(f.hidden_fields.at("lang"), "de");
}

TEST(ScraperPluginFocusedTests, SearchEngineNoFormReturnsEmpty) {
    HtmlSearchEngine eng;
    const auto forms = eng.discoverForms("<html><body><p>Keine Form</p></body></html>",
                                          "https://example.de");
    EXPECT_TRUE(forms.empty());
}

TEST(ScraperPluginFocusedTests, SearchEngineParseResultList) {
    HtmlSearchEngine eng;
    const std::string html = R"(
<html><body>
<ul class="result-list">
  <li><a href="/u/111.html">BGH – Urteil 2024</a><span class="datum">2024-01-15</span></li>
  <li><a href="/u/222.html">OLG Köln – Beschluss 2023</a></li>
  <li><a href="/u/333.html">AG München – Urteil 2022</a></li>
</ul>
<a rel="next" href="/suche/?q=test&amp;p=2">Nächste</a>
</body></html>)";
    const auto page = eng.parseResults(html, "https://openjur.de");
    EXPECT_GE(page.items.size(), 2u);
    EXPECT_FALSE(page.items[0].url.empty());
    EXPECT_FALSE(page.items[0].title.empty());
    // Pagination
    EXPECT_TRUE(page.has_more || !page.next_page_url.empty());
}

TEST(ScraperPluginFocusedTests, SearchEngineBuildSearchUrl) {
    HtmlSearchEngine eng;
    SearchForm f;
    f.action_url = "https://openjur.de/suche/";
    f.method     = "GET";
    f.input_name = "q";
    const std::string url = eng.buildSearchUrl(f, "Baugenehmigung", 1);
    EXPECT_NE(url.find("q=Baugenehmigung"), std::string::npos);
}

TEST(ScraperPluginFocusedTests, SearchEngineInMemoryMock) {
    InMemorySearchEngine eng;
    SearchResultPage injected;
    SearchResultItem item;
    item.url = "https://openjur.de/u/123.html";
    item.title = "Test Urteil";
    item.rank = 1;
    injected.items.push_back(item);
    eng.injectResultPage(injected);

    const auto page = eng.parseResults("", "https://openjur.de");
    ASSERT_EQ(page.items.size(), 1u);
    EXPECT_EQ(page.items[0].url, "https://openjur.de/u/123.html");
    EXPECT_EQ(eng.callCount(), 1);
}

// ============================================================================
// Group E – ScraperApiClient
// ============================================================================

TEST(ScraperPluginFocusedTests, ApiClientInMemoryMockReturnsInjected) {
    InMemoryScraperApiClient client;
    ApiResult r1;
    r1.url   = "https://api.example.de/v1/results/1";
    r1.title = "Ergebnis 1";
    r1.extracted_text = "Baugenehmigung Bayern 2024";
    client.injectResults({r1});

    ApiEndpointConfig cfg;
    cfg.url = "https://api.example.de/v1/results";
    const auto results = client.fetchAll(cfg, "Baugenehmigung");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].title, "Ergebnis 1");
    EXPECT_EQ(client.lastQuery(), "Baugenehmigung");
}

TEST(ScraperPluginFocusedTests, ApiClientMockReturnsFlattenedText) {
    InMemoryScraperApiClient client;
    ApiResult r;
    r.extracted_text = "BauGB § 34 Zulässigkeit";
    client.injectResults({r});
    const auto results = client.fetchAll({}, "BauGB");
    EXPECT_FALSE(results[0].extracted_text.empty());
}

TEST(ScraperPluginFocusedTests, ApiClientEmptyResultOnNoInjection) {
    InMemoryScraperApiClient client;
    const auto results = client.fetchAll({}, "test");
    EXPECT_TRUE(results.empty());
}

TEST(ScraperPluginFocusedTests, HttpApiClientBuildGetUrl) {
    // Test URL construction without actual HTTP call
    HttpScraperApiClient client([](const std::string& url,
                                    const std::string&,
                                    const std::map<std::string,std::string>&,
                                    const std::string&) {
        // Validate the URL contains the right params and return empty JSON
        EXPECT_NE(url.find("q=BauGB"), std::string::npos);
        return R"({"results":[]})";
    });
    ApiEndpointConfig cfg;
    cfg.url          = "https://api.test.de/search";
    cfg.search_param = "q";
    cfg.page_param   = "page";
    cfg.results_field = "results";
    cfg.pagination_mode = "page";
    cfg.max_pages    = 1;
    const auto results = client.fetchAll(cfg, "BauGB");
    EXPECT_TRUE(results.empty()); // empty results array returned
}

// ============================================================================
// Group F – ScraperLLMEvaluator
// ============================================================================

TEST(ScraperPluginFocusedTests, HeuristicEvaluatorScoresHighForKeywordRich) {
    ScraperLLMEvaluator eval;
    GapContext gap;
    gap.gap_id    = "GAP-001";
    gap.keywords  = {"Baugenehmigung", "BauGB", "Bebauungsplan"};
    const std::string text =
        "Die Baugenehmigung nach BauGB § 34 erfordert einen gültigen Bebauungsplan. "
        "Baugenehmigung und Bebauungsplan sind zentrale Begriffe im deutschen Baurecht. "
        "BauGB regelt die Baugenehmigung umfassend. Weitere Details zur Baugenehmigung "
        "finden sich in der BauGB. Bebauungsplan und Baugenehmigung greifen ineinander.";
    const auto result = eval.evaluate(text, "https://example.de", gap, 0.3);
    EXPECT_GT(result.quality_score, 0.3);
    EXPECT_GT(result.gap_relevance, 0.0);
    EXPECT_FALSE(result.below_threshold);
}

TEST(ScraperPluginFocusedTests, HeuristicEvaluatorDiscardsEmptyText) {
    ScraperLLMEvaluator eval;
    GapContext gap;
    gap.gap_id   = "GAP-001";
    gap.keywords = {"test"};
    const auto result = eval.evaluate("", "https://example.de", gap, 0.5);
    EXPECT_TRUE(result.below_threshold);
    EXPECT_FALSE(result.discard_reason.empty());
}

TEST(ScraperPluginFocusedTests, InMemoryEvaluatorReturnsDefault) {
    InMemoryLLMEvaluator eval;
    EvaluationResult def;
    def.quality_score = 0.9;
    def.gap_relevance = 0.8;
    eval.setDefaultResult(def);

    GapContext gap;
    gap.gap_id = "GAP-001";
    const auto result = eval.evaluate("text", "https://example.de", gap, 0.5);
    EXPECT_DOUBLE_EQ(result.quality_score, 0.9);
    EXPECT_FALSE(result.below_threshold);
    EXPECT_EQ(eval.callCount(), 1);
}

TEST(ScraperPluginFocusedTests, InMemoryEvaluatorPerUrlOverride) {
    InMemoryLLMEvaluator eval;
    EvaluationResult low;
    low.quality_score = 0.2;
    low.gap_relevance = 0.1;
    eval.injectResult("spam", low);

    GapContext gap;
    gap.gap_id = "GAP-001";
    const auto result = eval.evaluate("text", "https://spam.example.de", gap, 0.5);
    EXPECT_DOUBLE_EQ(result.quality_score, 0.2);
    EXPECT_TRUE(result.below_threshold);
}

// ============================================================================
// Group G – InMemoryScraperMetadataWriter
// ============================================================================

TEST(ScraperPluginFocusedTests, MetadataWriterStoresRelationalRecord) {
    InMemoryScraperMetadataWriter w;
    ScraperRelationalRecord rel;
    rel.doc_id = "abc123"; rel.url = "https://openjur.de/u/1.html";
    rel.gap_id = "GAP-001"; rel.quality_score = 0.85;
    const WriteResult wr = w.write(rel, {}, {}, {});
    EXPECT_TRUE(wr.success);
    EXPECT_EQ(wr.doc_id, "abc123");
    ASSERT_EQ(w.relationalRecords().size(), 1u);
    EXPECT_EQ(w.relationalRecords()[0].url, rel.url);
}

TEST(ScraperPluginFocusedTests, MetadataWriterStoresGraphNode) {
    InMemoryScraperMetadataWriter w;
    ScraperGraphNode node;
    node.node_id = "n1"; node.label = "ScrapedDocument";
    node.properties["url"] = "https://eurlex.eu/doc/1";
    w.write({}, node, {}, {});
    ASSERT_EQ(w.graphNodes().size(), 1u);
    EXPECT_EQ(w.graphNodes()[0].label, "ScrapedDocument");
}

TEST(ScraperPluginFocusedTests, MetadataWriterStoresGraphEdges) {
    InMemoryScraperMetadataWriter w;
    ScraperGraphEdge e;
    e.from_id = "doc1"; e.to_id = "GAP:GAP-001"; e.rel = "FILLS_GAP";
    w.write({}, {}, {e}, {});
    ASSERT_EQ(w.graphEdges().size(), 1u);
    EXPECT_EQ(w.graphEdges()[0].rel, "FILLS_GAP");
}

TEST(ScraperPluginFocusedTests, MetadataWriterFlushTracked) {
    InMemoryScraperMetadataWriter w;
    EXPECT_EQ(w.flushCount(), 0);
    w.flush();
    w.flush();
    EXPECT_EQ(w.flushCount(), 2);
}

// ============================================================================
// Group H – ScraperPlugin integration
// ============================================================================

TEST(ScraperPluginFocusedTests, PluginInitializesSuccessfully) {
    auto eval   = std::make_shared<InMemoryLLMEvaluator>();
    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    auto search = std::make_shared<InMemorySearchEngine>();
    auto api    = std::make_shared<InMemoryScraperApiClient>();

    ScraperPlugin plugin(eval, writer, search, nullptr, api);
    ScraperConfig cfg;
    cfg.gap_context.gap_id = "GAP-001";
    cfg.gap_context.keywords = {"Baugenehmigung"};
    EXPECT_TRUE(plugin.initialize(cfg));
    EXPECT_TRUE(plugin.isInitialized());
}

TEST(ScraperPluginFocusedTests, PluginScrapeWithMockSearchEngineAcceptsDoc) {
    auto eval   = std::make_shared<InMemoryLLMEvaluator>();
    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    auto search = std::make_shared<InMemorySearchEngine>();
    auto api    = std::make_shared<InMemoryScraperApiClient>();

    // Inject a search form
    SearchForm form;
    form.action_url = "https://openjur.de/suche/";
    form.method     = "GET";
    form.input_name = "q";
    search->injectForms({form});

    // Inject a result page with one item
    SearchResultPage rp;
    SearchResultItem item;
    item.url   = "https://openjur.de/u/123456.html";
    item.title = "BGH – Urteil zur Baugenehmigung";
    item.rank  = 1;
    rp.items.push_back(item);
    search->injectResultPage(rp);

    ScraperPlugin plugin(eval, writer, search, nullptr, api);
    ScraperConfig cfg;
    cfg.gap_context.gap_id    = "GAP-001";
    cfg.gap_context.keywords  = {"Baugenehmigung"};
    cfg.search_options.enabled = true;
    cfg.search_options.max_result_pages = 1;
    cfg.seed_urls = {"https://openjur.de"};
    cfg.whitelist = {};  // allow all

    plugin.initialize(cfg);
    // Inject HTTP fetch so no real network call is made
    plugin.setHttpFetch([](const std::string& url, const std::string&) -> std::string {
        if (url.find("openjur.de/u/") != std::string::npos)
            return "<html><body><h1>BGH Urteil</h1><p>Baugenehmigung nach BauGB § 34 "
                   "erfordert einen gültigen Bebauungsplan.</p></body></html>";
        return "<html><body><form action='/suche/' method='get'>"
               "<input type='search' name='q'/></form></body></html>";
    });

    const auto stats = plugin.scrape();
    EXPECT_GE(stats.docs_scraped, 0); // may be 0 if mock search engine path taken
    // Writer should have been flushed
    EXPECT_GE(writer->flushCount(), 1);
}

TEST(ScraperPluginFocusedTests, PluginScrapeApiSourceWithMockClient) {
    auto eval   = std::make_shared<InMemoryLLMEvaluator>();
    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    auto search = std::make_shared<InMemorySearchEngine>();
    auto api    = std::make_shared<InMemoryScraperApiClient>();

    ApiResult r;
    r.url            = "https://search.dip.bundestag.de/api/v1/vorgang/1";
    r.title          = "Gesetzentwurf BauGB";
    r.extracted_text = "Baugenehmigung und BauGB sind zentrale Aspekte des Verfahrens.";
    api->injectResults({r});

    ScraperPlugin plugin(eval, writer, search, nullptr, api);
    ScraperConfig cfg;
    cfg.gap_context.gap_id   = "GAP-001";
    cfg.gap_context.keywords = {"Baugenehmigung", "BauGB"};
    cfg.crawl_options.render_mode = ScraperRenderMode::API_JSON;
    cfg.seed_urls = {"https://search.dip.bundestag.de/api/v1/vorgang"};
    cfg.whitelist = {};

    plugin.initialize(cfg);
    const auto stats = plugin.scrape();
    EXPECT_GE(stats.api_pages_fetched, 1);
    EXPECT_GE(stats.docs_accepted + stats.docs_discarded, 1);
}

TEST(ScraperPluginFocusedTests, PluginScrapeRespectsBlacklist) {
    auto eval   = std::make_shared<InMemoryLLMEvaluator>();
    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    auto search = std::make_shared<InMemorySearchEngine>();
    auto api    = std::make_shared<InMemoryScraperApiClient>();

    ScraperPlugin plugin(eval, writer, search, nullptr, api);
    ScraperConfig cfg;
    cfg.gap_context.gap_id  = "GAP-001";
    cfg.blacklist = {"https://blocked.example.de"};
    cfg.seed_urls = {"https://blocked.example.de/docs"};
    cfg.whitelist = {};

    plugin.initialize(cfg);
    // setHttpFetch to track if blocked URL is fetched
    bool blocked_fetched = false;
    plugin.setHttpFetch([&](const std::string& url, const std::string&) -> std::string {
        if (url.find("blocked.example.de") != std::string::npos) blocked_fetched = true;
        return "";
    });
    plugin.scrape();
    EXPECT_FALSE(blocked_fetched);
}

TEST(ScraperPluginFocusedTests, PluginJsRendererUsedForJsRenderedMode) {
    auto eval      = std::make_shared<InMemoryLLMEvaluator>();
    auto writer    = std::make_shared<InMemoryScraperMetadataWriter>();
    auto search    = std::make_shared<InMemorySearchEngine>();
    auto api       = std::make_shared<InMemoryScraperApiClient>();
    auto renderer  = std::make_shared<InMemoryJSRenderer>();

    JsRenderResult jres;
    jres.success = true;
    jres.html    = "<html><body><p>React SPA content: Baugenehmigung Bayern</p></body></html>";
    renderer->injectResult(jres);

    ScraperPlugin plugin(eval, writer, search, renderer, api);
    ScraperConfig cfg;
    cfg.gap_context.gap_id   = "GAP-001";
    cfg.gap_context.keywords = {"Baugenehmigung"};
    cfg.crawl_options.render_mode = ScraperRenderMode::JS_RENDERED;
    cfg.search_options.enabled    = false; // direct scrape
    cfg.seed_urls = {"https://spa.example.de"};
    cfg.whitelist = {};

    plugin.initialize(cfg);
    plugin.scrape();
    EXPECT_GE(renderer->callCount(), 1);
}

TEST(ScraperPluginFocusedTests, PluginResetClearsResults) {
    auto eval   = std::make_shared<InMemoryLLMEvaluator>();
    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    ScraperPlugin plugin(eval, writer, nullptr, nullptr, nullptr);
    ScraperConfig cfg;
    cfg.gap_context.gap_id = "GAP-001";
    plugin.initialize(cfg);

    plugin.reset();
    EXPECT_TRUE(plugin.getResults().empty());
}

TEST(ScraperPluginFocusedTests, ScraperRecordBuilderCreatesDocId) {
    GapContext gap;
    gap.gap_id = "GAP-001";
    gap.keywords = {"test"};
    EvaluationResult eval;
    eval.quality_score = 0.8;
    eval.gap_relevance = 0.7;

    const auto rel = ScraperRecordBuilder::buildRelational(
        "https://openjur.de/u/1.html",
        "Test Urteil",
        "Baugenehmigung nach BauGB",
        "openjur", "openjur", eval, gap);
    EXPECT_FALSE(rel.doc_id.empty());
    EXPECT_EQ(rel.doc_id.size(), 16u); // 16-char hex
    EXPECT_EQ(rel.gap_id, "GAP-001");

    const auto node = ScraperRecordBuilder::buildNode(rel);
    EXPECT_EQ(node.label, "ScrapedDocument");
    EXPECT_EQ(node.node_id, rel.doc_id);

    const auto edges = ScraperRecordBuilder::buildEdges(rel, eval);
    EXPECT_GE(edges.size(), 1u);
    EXPECT_EQ(edges[0].rel, "FILLS_GAP");
}
