> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Scraper Module

All notable changes to the Scraper module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Per-source retry logic with exponential backoff (Target: v1.1.0, Q3 2026)
- Structured per-run event log to `std::ostream` sink (Target: v1.1.0, Q3 2026)
- `ScraperRunStats` extended with P50/P95/P99 per-document latency (Target: v1.1.0, Q3 2026)
- Content-hash deduplication across runs (Target: v1.2.0, Q4 2026)
- Incremental crawl via `If-Modified-Since` / `ETag` (Target: v1.2.0, Q4 2026)
- Sitemap-driven crawl for `GovSearchStyle::SITEMAP` sources (Target: v1.2.0, Q4 2026)
- Structured data extraction (JSON-LD / microdata / Open Graph) (Target: v1.3.0, Q1 2027)
- CSS selector-guided text extraction (Target: v1.3.0, Q1 2027)

## [1.0.0] — 2026-05-13

### Added
- `IScraperPlugin` interface and `ScraperPlugin` production implementation
  (`scraper_plugin.h/.cpp`) with full agentic loop:
  seed collection → page fetch → form discovery → result pagination →
  document processing → provenance-stamped write.
- `ScraperConfig` (`scraper_config.h/.cpp`):
  - `loadFromFile(path)` and `loadFromYaml(yaml_str)` — YAML-based configuration
  - `CrawlOptions` — `max_depth`, `max_pages`, `request_delay_ms`, `render_mode`,
    `respect_robots`
  - `SearchOptions` — `queries`, `max_result_pages`
  - `ApiOptions` — `endpoints`, `max_pages`
  - `LlmOptions` — `quality_threshold` (default 0.65)
  - `GapContext` — keyword set + description for gap-targeted scoring
  - `UrlPolicy` — whitelist/blacklist prefix and glob matching; SSRF guard
    (non-http/https schemes rejected)
- `ScraperRenderMode` enum — `STATIC`, `JS_RENDERED`, `API_JSON`, `API_GRAPHQL`
- `IScraperApiClient` / `HttpScraperApiClient` (`scraper_api_client.h/.cpp`):
  - `fetchAll(endpoint, options)` — REST and GraphQL JSON page fetch with
    page-, cursor-, and offset-based pagination
- `IScraperSearchEngine` / `HtmlSearchEngine` (`scraper_search_engine.h/.cpp`):
  - `discoverForms(html, base_url)` — form discovery via pugixml
  - `parseResults(html, base_url)` — result list parsing (JSON-LD, CSS heuristic, fallback)
- `IScraperLLMEvaluator` / `ScraperLLMEvaluator` (`scraper_llm_evaluator.h/.cpp`):
  - `evaluate(text, gap_context)` — LLM quality/relevance scoring
  - `isLlmAvailable()` — fallback detection
  - Heuristic keyword-frequency fallback when LLM is unavailable
- `IScraperMetadataWriter` / `InMemoryScraperMetadataWriter` (`scraper_metadata_writer.h/.cpp`):
  - `ScraperRecordBuilder` — builds relational, graph, and vector records with
    unconditional provenance fields:
    `is_scraper_ingested`, `ingestion_source_type`, `ingestion_plugin_version`
- `IScraperJSRenderer` / `SubprocessJSRenderer` (`scraper_js_renderer.h/.cpp`):
  - `render(url)` — headless browser rendering via external subprocess
  - `isAvailable()` — subprocess binary check
- `GovSourceCatalog` (`gov_source_catalog.h/.cpp`):
  - Built-in catalog: 8 Bund portals, 16 Bundesländer portals, 5 EU portals
  - `loadFromFile(path)` — YAML overlay for custom/extended sources
  - Filter methods: `getAll()`, `getBySourceId()`, `getEnabled()`
- `ScraperRunStats` — run counters:
  `visited`, `forms`, `api_pages`, `scraped`, `accepted`, `discarded`,
  `written`, `write_errors`, `elapsed_ms`
- Compile-time feature flags: `THEMIS_ENABLE_CURL`, `THEMIS_ENABLE_PUGIXML`,
  `THEMIS_ENABLE_LLM`
- Test doubles: `InMemoryLLMEvaluator`, `InMemorySearchEngine`,
  `InMemoryJSRenderer`, `InMemoryScraperApiClient`
- Dependency injection via `ScraperPlugin::set*()` methods
- `CMakeLists.txt` for in-tree build registration
- Module documentation: `README.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`,
  `PERFORMANCE_EXPECTATIONS.md`
- Unit tests for all components:
  `test_scraper_plugin.cpp`, `test_scraper_config.cpp`,
  `test_scraper_api_client.cpp`, `test_scraper_search_engine.cpp`,
  `test_scraper_llm_evaluator.cpp`, `test_scraper_metadata_writer.cpp`,
  `test_gov_source_catalog.cpp`
