# Scraper Plugin – Changelog

All notable changes to this plugin will be documented in this file.

---

## [1.0.0] – 2026-04-02

### Added

- **`scraper_config.h/cpp`** – `ScraperConfig` (YAML-based configuration),
  `UrlPolicy` (whitelist / blacklist with glob support, SSRF prevention),
  `GapContext`, `CrawlOptions`, `SearchOptions`, `ApiOptions`, `LlmOptions`,
  `GovSourcesOptions`, and `ScraperRenderMode` (STATIC, JS_RENDERED, API_JSON, API_GRAPHQL).

- **`gov_source_catalog.h/cpp`** – `GovSourceCatalog` with built-in entries for:
  - 8 German federal (Bund) sources: Gesetze im Internet, Rechtsprechung im
    Internet, Bundesanzeiger, Bundestag DIP (REST API), Bundesrat, OpenJur,
    dejure.org, Bundesverfassungsgericht
  - All 16 German state (Bundesland) law portals (Baden-Württemberg through
    Thüringen)
  - 5 EU sources: EUR-Lex (HTML + SPARQL), CURIA, European Parliament
    (REST), EU Publications Office

- **`scraper_search_engine.h/cpp`** – `HtmlSearchEngine` (pugixml-backed
  HTML form discovery, openjur.de-style result-list parsing, pagination
  detection); `InMemorySearchEngine` mock.

- **`scraper_js_renderer.h/cpp`** – `SubprocessJSRenderer` (launches
  external headless browser via subprocess/popen, compatible with
  Puppeteer / Playwright); `InMemoryJSRenderer` mock.

- **`scraper_api_client.h/cpp`** – `HttpScraperApiClient` (libcurl-backed
  JSON REST API crawler with page / cursor / offset pagination); 
  `InMemoryScraperApiClient` mock.

- **`scraper_llm_evaluator.h/cpp`** – `ScraperLLMEvaluator` (LLM-based
  gap-relevance and quality scoring via `LLMPluginManager::generate()` under
  `THEMIS_ENABLE_LLM`; heuristic keyword-density fallback always available);
  `InMemoryLLMEvaluator` mock.

- **`scraper_metadata_writer.h/cpp`** – `InMemoryScraperMetadataWriter`
  (stores relational records, graph nodes/edges, vector records for testing);
  `ScraperRecordBuilder` (builds all three record types from a scraped
  document).

- **`scraper_plugin.h/cpp`** – `ScraperPlugin` agentic loop: gov-catalog
  seed expansion → HTML/JS/API fetch → search-form submission → result-list
  pagination → LLM evaluation → metadata write. Supports dependency injection
  for all sub-components.

- **`tests/test_scraper_plugin.cpp`** – 40 unit tests (`ScraperPluginFocusedTests`):
  UrlPolicy (6), Config (5), GovCatalog (5), SearchEngine (5), ApiClient (4),
  LLMEvaluator (4), MetadataWriter (4), Plugin integration (7).

- **`config/scraper_urls.yaml`** – Example configuration targeting
  openjur.de, Gesetze im Internet, and Rechtsprechung im Internet.

- **`config/gov_sources.yaml`** – Catalog overlay template.

- **`CMakeLists.txt`** – Standalone build with optional yaml-cpp, pugixml,
  libcurl, nlohmann_json, and LLM integration flags.

- **`.github/workflows/02-feature-modules_scraper-plugin-ci.yml`** – CI for
  Ubuntu 22.04 (GCC 12 + Clang 15) and Ubuntu 24.04 (GCC 13).
