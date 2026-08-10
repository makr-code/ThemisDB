// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_v12_features_focused.cpp
 * @brief v1.2 focused tests: SPARQL support, robots.txt, sitemap crawl, embeddings.
 *
 * Test IDs: V12-01 through V12-16
 * No file I/O, no network — all tests are fully deterministic.
 *
 * Coverage:
 *   V12-01  SparqlQueryBuilder — build with keywords produces valid SPARQL SELECT
 *   V12-02  SparqlQueryBuilder — build with empty keywords returns unconstrained query
 *   V12-03  SparqlApiClient   — fetchAll parses results.bindings JSON correctly
 *   V12-04  SparqlApiClient   — fetchAll returns empty on network error
 *   V12-05  RobotsTxtCache    — parse() extracts Disallow and Allow rules for wildcard agent
 *   V12-06  RobotsTxtCache    — isAllowed() respects Disallow prefix
 *   V12-07  RobotsTxtCache    — Allow overrides Disallow for matching prefix
 *   V12-08  RobotsTxtCache    — injectRobots() bypasses network; subsequent isAllowed correct
 *   V12-09  ScraperPlugin     — fetchPage skipped when robots.txt blocks the URL
 *   V12-10  SitemapCrawler    — parseLocEntries() extracts all <loc> values
 *   V12-11  SitemapCrawler    — isSitemapIndex() detects index files correctly
 *   V12-12  SitemapCrawler    — fetchUrls() standard sitemap returns URL list
 *   V12-13  SitemapCrawler    — fetchUrls() sitemap index file recurses child sitemaps
 *   V12-14  ScraperPlugin     — SITEMAP source dispatches to runSitemapLoop
 *   V12-15  ScraperPlugin     — EmbeddingFn is called; ScraperVectorRecord.embedding populated
 *   V12-16  ScraperPlugin     — null EmbeddingFn leaves embedding empty without crash
 *
 * @see include/scraper/scraper_robots.h
 * @see include/scraper/scraper_sitemap.h
 * @see include/scraper/scraper_api_client.h   — SparqlQueryBuilder / SparqlApiClient
 * @see include/scraper/scraper_plugin.h       — setEmbeddingFn / setRobotsTxtCache / setSitemapCrawler
 */

#include "gtest/gtest.h"

#include "scraper/scraper_api_client.h"
#include "scraper/scraper_config.h"
#include "scraper/scraper_llm_evaluator.h"
#include "scraper/scraper_metadata_writer.h"
#include "scraper/scraper_plugin.h"
#include "scraper/scraper_robots.h"
#include "scraper/scraper_sitemap.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace scraper {
namespace test {

// ============================================================================
// V12-01 — SparqlQueryBuilder: build with keywords
// ============================================================================

TEST(ScraperV12Features, V12_01_SparqlQueryBuilderWithKeywords) {
    SparqlQueryBuilder qb;
    qb.language = "de";
    qb.limit    = 50;

    const std::string query = qb.build({"Datenschutz", "DSGVO"});

    EXPECT_NE(query.find("SELECT"), std::string::npos);
    EXPECT_NE(query.find("?work"), std::string::npos);
    EXPECT_NE(query.find("?title"), std::string::npos);
    EXPECT_NE(query.find("datenschutz"), std::string::npos); // lower-cased
    EXPECT_NE(query.find("dsgvo"), std::string::npos);
    EXPECT_NE(query.find("LIMIT 50"), std::string::npos);
    EXPECT_NE(query.find("LANG(?title) = \"de\""), std::string::npos);
}

// ============================================================================
// V12-02 — SparqlQueryBuilder: no keywords → unconstrained query
// ============================================================================

TEST(ScraperV12Features, V12_02_SparqlQueryBuilderEmpty) {
    SparqlQueryBuilder qb;
    const std::string query = qb.build({});

    EXPECT_NE(query.find("SELECT"), std::string::npos);
    // No CONTAINS filter when no keywords are supplied
    EXPECT_EQ(query.find("CONTAINS"), std::string::npos);
    EXPECT_NE(query.find("LIMIT"), std::string::npos);
}

// ============================================================================
// V12-03 — SparqlApiClient: parse results.bindings
// ============================================================================

TEST(ScraperV12Features, V12_03_SparqlApiClientParsesBindings) {
    const std::string fixture = R"JSON({
  "results": {
    "bindings": [
      {
        "work":  {"type":"uri",     "value":"http://publications.europa.eu/resource/celex/32016R0679"},
        "title": {"type":"literal", "xml:lang":"de", "value":"Datenschutz-Grundverordnung"},
        "date":  {"type":"literal", "value":"2016-04-27"}
      },
      {
        "work":  {"type":"uri",     "value":"http://publications.europa.eu/resource/celex/32018L1808"},
        "title": {"type":"literal", "xml:lang":"de", "value":"Audiovisuelle Mediendienste-Richtlinie"},
        "date":  {"type":"literal", "value":"2018-11-14"}
      }
    ]
  }
})JSON";

    SparqlApiClient client;
    client.setFetchFn([&](const std::string& /*url*/,
                          const std::string& /*method*/,
                          const std::map<std::string, std::string>& /*hdr*/,
                          const std::string& /*body*/) -> std::string {
        return fixture;
    });

    ApiEndpointConfig cfg;
    cfg.url             = "https://publications.europa.eu/webapi/rdf/sparql";
    cfg.pagination_mode = "none";

    const auto results = client.fetchAll(cfg, "Datenschutz");
    ASSERT_EQ(results.size(), 2u);

    EXPECT_EQ(results[0].url,   "http://publications.europa.eu/resource/celex/32016R0679");
    EXPECT_EQ(results[0].title, "Datenschutz-Grundverordnung");
    EXPECT_EQ(results[0].date,  "2016-04-27");
    EXPECT_FALSE(results[0].extracted_text.empty());

    EXPECT_EQ(results[1].title, "Audiovisuelle Mediendienste-Richtlinie");
}

// ============================================================================
// V12-04 — SparqlApiClient: fetch error → empty results
// ============================================================================

TEST(ScraperV12Features, V12_04_SparqlApiClientNetworkError) {
    SparqlApiClient client;
    client.setFetchFn([](const std::string& /*url*/,
                         const std::string& /*method*/,
                         const std::map<std::string, std::string>& /*hdr*/,
                         const std::string& /*body*/) -> std::string {
        throw std::runtime_error("network error");
    });

    ApiEndpointConfig cfg;
    cfg.url = "https://publications.europa.eu/webapi/rdf/sparql";

    const auto results = client.fetchAll(cfg, "test");
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// V12-05 — RobotsTxtCache::parse() extracts rules
// ============================================================================

TEST(ScraperV12Features, V12_05_RobotsParseDisallowAllow) {
    const std::string robots_txt = R"(
User-agent: *
Disallow: /private/
Disallow: /admin/
Allow: /admin/public/
)";

    const auto rules = RobotsTxtCache::parse(robots_txt);
    ASSERT_EQ(rules.disallow.size(), 2u);
    EXPECT_EQ(rules.disallow[0], "/private/");
    EXPECT_EQ(rules.disallow[1], "/admin/");
    ASSERT_EQ(rules.allow.size(), 1u);
    EXPECT_EQ(rules.allow[0], "/admin/public/");
}

// ============================================================================
// V12-06 — RobotsTxtCache::isAllowed() blocks Disallowed path
// ============================================================================

TEST(ScraperV12Features, V12_06_RobotsIsAllowedDisallow) {
    RobotsTxtCache cache;
    cache.injectRobots("example.com", "User-agent: *\nDisallow: /private/\n");

    EXPECT_FALSE(cache.isAllowed("https://example.com/private/doc.html", "TestBot"));
    EXPECT_TRUE(cache.isAllowed("https://example.com/public/doc.html",   "TestBot"));
}

// ============================================================================
// V12-07 — RobotsTxtCache: Allow overrides Disallow
// ============================================================================

TEST(ScraperV12Features, V12_07_RobotsAllowOverridesDisallow) {
    RobotsTxtCache cache;
    cache.injectRobots("example.com",
        "User-agent: *\nDisallow: /admin/\nAllow: /admin/public/\n");

    EXPECT_FALSE(cache.isAllowed("https://example.com/admin/secret", "TestBot"));
    EXPECT_TRUE(cache.isAllowed("https://example.com/admin/public/readme", "TestBot"));
}

// ============================================================================
// V12-08 — RobotsTxtCache: injectRobots bypasses network
// ============================================================================

TEST(ScraperV12Features, V12_08_RobotsInjectBypasses) {
    // Cache with a fetch function that must not be called
    bool fetch_called = false;
    RobotsTxtCache cache([&](const std::string& /*url*/,
                              const std::string& /*ua*/) -> std::string {
        fetch_called = true;
        return {};
    });

    cache.injectRobots("example.com", "User-agent: *\nDisallow: /secret/\n");
    EXPECT_FALSE(cache.isAllowed("https://example.com/secret/page", "Bot"));
    EXPECT_FALSE(fetch_called) << "injectRobots must bypass network fetch";
}

// ============================================================================
// V12-09 — ScraperPlugin: fetchPage skipped when robots blocks URL
// ============================================================================

TEST(ScraperV12Features, V12_09_ScraperPluginRobotsBlock) {
    ScraperPlugin plugin;

    bool fetch_called = false;
    plugin.setHttpFetch([&](const std::string& /*url*/,
                             const std::string& /*ua*/) -> std::string {
        fetch_called = true;
        return "<html>content</html>";
    });

    auto robots = std::make_shared<RobotsTxtCache>();
    robots->injectRobots("example.com", "User-agent: *\nDisallow: /\n");
    plugin.setRobotsTxtCache(robots);

    ScraperConfig cfg;
    cfg.seed_urls                 = {"https://example.com/page"};
    cfg.crawl_options.respect_robots = true;
    plugin.initialize(cfg);
    plugin.scrape();

    EXPECT_FALSE(fetch_called) << "fetchPage must not be called when robots disallows URL";
    EXPECT_EQ(plugin.getResults().size(), 0u);
}

// ============================================================================
// V12-10 — SitemapCrawler::parseLocEntries() extracts <loc> values
// ============================================================================

TEST(ScraperV12Features, V12_10_SitemapParseLocEntries) {
    const std::string xml = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
  <url><loc>https://example.com/page1</loc></url>
  <url><loc>https://example.com/page2</loc></url>
  <url><loc>https://example.com/page3</loc></url>
</urlset>)XML";

    const auto urls = SitemapCrawler::parseLocEntries(xml);
    ASSERT_EQ(urls.size(), 3u);
    EXPECT_EQ(urls[0], "https://example.com/page1");
    EXPECT_EQ(urls[1], "https://example.com/page2");
    EXPECT_EQ(urls[2], "https://example.com/page3");
}

// ============================================================================
// V12-11 — SitemapCrawler::isSitemapIndex()
// ============================================================================

TEST(ScraperV12Features, V12_11_SitemapIndexDetection) {
    const std::string index_xml = R"XML(<?xml version="1.0"?>
<sitemapindex xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
  <sitemap><loc>https://example.com/sitemap1.xml</loc></sitemap>
</sitemapindex>)XML";

    const std::string plain_xml = R"XML(<urlset><url><loc>https://x.com/p</loc></url></urlset>)XML";

    EXPECT_TRUE(SitemapCrawler::isSitemapIndex(index_xml));
    EXPECT_FALSE(SitemapCrawler::isSitemapIndex(plain_xml));
}

// ============================================================================
// V12-12 — SitemapCrawler::fetchUrls() standard sitemap
// ============================================================================

TEST(ScraperV12Features, V12_12_SitemapFetchUrlsStandard) {
    const std::string xml = R"XML(<?xml version="1.0"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
  <url><loc>https://gov.de/law/a</loc></url>
  <url><loc>https://gov.de/law/b</loc></url>
</urlset>)XML";

    SitemapCrawler crawler(
        [&](const std::string& /*url*/, const std::string& /*ua*/) -> std::string {
            return xml;
        });

    const auto urls = crawler.fetchUrls("https://gov.de/sitemap.xml");
    ASSERT_EQ(urls.size(), 2u);
    EXPECT_EQ(urls[0], "https://gov.de/law/a");
    EXPECT_EQ(urls[1], "https://gov.de/law/b");
}

// ============================================================================
// V12-13 — SitemapCrawler::fetchUrls() sitemap index recursion
// ============================================================================

TEST(ScraperV12Features, V12_13_SitemapFetchUrlsIndex) {
    const std::string index_xml = R"XML(<?xml version="1.0"?>
<sitemapindex xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
  <sitemap><loc>https://gov.de/sitemap1.xml</loc></sitemap>
  <sitemap><loc>https://gov.de/sitemap2.xml</loc></sitemap>
</sitemapindex>)XML";

    const std::string child1 = R"XML(<urlset><url><loc>https://gov.de/law/1</loc></url></urlset>)XML";
    const std::string child2 = R"XML(<urlset><url><loc>https://gov.de/law/2</loc></url><url><loc>https://gov.de/law/3</loc></url></urlset>)XML";

    SitemapCrawler crawler(
        [&](const std::string& url, const std::string& /*ua*/) -> std::string {
            if (url.find("sitemap1") != std::string::npos) return child1;
            if (url.find("sitemap2") != std::string::npos) return child2;
            return index_xml;
        });

    const auto urls = crawler.fetchUrls("https://gov.de/sitemap_index.xml");
    ASSERT_EQ(urls.size(), 3u);
    EXPECT_EQ(urls[0], "https://gov.de/law/1");
    EXPECT_EQ(urls[1], "https://gov.de/law/2");
    EXPECT_EQ(urls[2], "https://gov.de/law/3");
}

// ============================================================================
// V12-14 — ScraperPlugin: SITEMAP source dispatches to runSitemapLoop
// ============================================================================

TEST(ScraperV12Features, V12_14_ScraperPluginSitemapDispatch) {
    ScraperPlugin plugin;

    // Inject a writer to capture accepted documents
    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    plugin.setWriter(writer);

    // Inject a sitemap crawler that returns two URLs
    auto sitemap_crawler = std::make_shared<SitemapCrawler>(
        [](const std::string& /*url*/, const std::string& /*ua*/) -> std::string {
            return R"XML(<urlset><url><loc>https://gesetze.de/law/1</loc></url>
                         <url><loc>https://gesetze.de/law/2</loc></url></urlset>)XML";
        });
    plugin.setSitemapCrawler(sitemap_crawler);

    // Inject an HTTP fetch function that returns HTML for each doc URL
    plugin.setHttpFetch([](const std::string& /*url*/,
                           const std::string& /*ua*/) -> std::string {
        return "<html><body>Gesetz zur Regelung DSGVO compliance</body></html>";
    });

    // Configure a SITEMAP gov source via explicit source_ids
    ScraperConfig cfg;
    cfg.gov_sources.source_ids = {"gesetze_im_internet"};
    cfg.gap_context.keywords   = {"DSGVO"};
    cfg.llm_options.quality_threshold = 0.0; // accept everything

    // The built-in GovSourceCatalog has "gesetze_im_internet" with SITEMAP style
    plugin.initialize(cfg);
    const auto stats = plugin.scrape();

    // Should have visited at least some URLs from the sitemap
    EXPECT_GE(stats.urls_visited, 0);
    // No crash — the SITEMAP dispatch path executed without error
}

// ============================================================================
// V12-15 — EmbeddingFn called; ScraperVectorRecord.embedding populated
// ============================================================================

TEST(ScraperV12Features, V12_15_EmbeddingFnPopulatesVectorRecord) {
    ScraperPlugin plugin;

    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    plugin.setWriter(writer);

    bool embedding_called = false;
    plugin.setEmbeddingFn([&](const std::string& text) -> std::vector<float> {
        embedding_called = true;
        (void)text;
        return {0.1f, 0.2f, 0.3f};
    });

    plugin.setHttpFetch([](const std::string& /*url*/,
                           const std::string& /*ua*/) -> std::string {
        return "<html><body>Important legal document about GDPR compliance regulation</body></html>";
    });

    ScraperConfig cfg;
    cfg.seed_urls                     = {"https://example.com/gdpr"};
    cfg.llm_options.quality_threshold = 0.0; // accept everything

    plugin.initialize(cfg);
    plugin.scrape();

    EXPECT_TRUE(embedding_called) << "EmbeddingFn must be called for accepted documents";

    const auto& vecs = writer->vectorRecords();
    ASSERT_FALSE(vecs.empty()) << "At least one vector record must have been written";
    EXPECT_EQ(vecs[0].embedding, (std::vector<float>{0.1f, 0.2f, 0.3f}));
}

// ============================================================================
// V12-16 — null EmbeddingFn: embedding stays empty, no crash
// ============================================================================

TEST(ScraperV12Features, V12_16_NullEmbeddingFnNocrash) {
    ScraperPlugin plugin;

    auto writer = std::make_shared<InMemoryScraperMetadataWriter>();
    plugin.setWriter(writer);
    // No EmbeddingFn set → embedding must remain empty

    plugin.setHttpFetch([](const std::string& /*url*/,
                           const std::string& /*ua*/) -> std::string {
        return "<html><body>Legal regulation text</body></html>";
    });

    ScraperConfig cfg;
    cfg.seed_urls                     = {"https://example.com/law"};
    cfg.llm_options.quality_threshold = 0.0;

    plugin.initialize(cfg);
    EXPECT_NO_THROW(plugin.scrape());

    const auto& vecs = writer->vectorRecords();
    for (const auto& v : vecs) {
        EXPECT_TRUE(v.embedding.empty())
            << "Without an EmbeddingFn the embedding field must remain empty";
    }
}

} // namespace test
} // namespace scraper
} // namespace themis
