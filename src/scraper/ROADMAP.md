> **Roadmap-Hinweis:** Checkbox-Status: `[ ]` offen, `[~]` in Bearbeitung, `[x]` erledigt, `[I]` Issue, `[P]` PR, `[?]` blockiert, `[!]` unklar.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · FUTURE_ENHANCEMENTS.md · ../../include/scraper/README.md -->

# Scraper Module Roadmap

## Current Status

v1.0.0 — `ScraperPlugin` with the full agentic loop is production-ready.
All core components (config, URL policy, HTML search-engine, LLM evaluator with
heuristic fallback, JS renderer, REST/GraphQL API client, government source
catalog, provenance-stamped metadata writer) are implemented, tested, and
integrated into the ThemisDB plugin system.

---

## Completed ✅

### v1.0.0 — Initial Release

- [x] `IScraperPlugin` interface and `ScraperPlugin` production implementation
- [x] Agentic loop: seed collection → page fetch → form discovery → result pagination → document processing
- [x] `ScraperConfig` YAML loading (`loadFromFile`, `loadFromYaml`) and defaults
- [x] `GapContext` propagation to the LLM evaluator for gap-targeted scoring
- [x] `UrlPolicy` with whitelist/blacklist prefix and glob matching; SSRF guard (http/https only)
- [x] `ScraperRenderMode` — `STATIC`, `JS_RENDERED`, `API_JSON`, `API_GRAPHQL`
- [x] `HtmlSearchEngine` (pugixml) — form discovery, result list parsing (JSON-LD, CSS heuristic, fallback), URL building
- [x] `HttpScraperApiClient` (libcurl) — page-, cursor-, and offset-based pagination; JSON array extraction
- [x] `ScraperLLMEvaluator` — LLM-backed quality/relevance scoring with keyword-frequency fallback
- [x] `SubprocessJSRenderer` — headless browser rendering via external subprocess; `isAvailable()` check
- [x] `GovSourceCatalog` — 8 Bund portals + 16 Bundesländer + 5 EU portals; YAML overlay support
- [x] `ScraperRecordBuilder` — builds relational, graph, and vector records with mandatory provenance fields
- [x] `InMemoryScraperMetadataWriter` — in-memory writer for tests with full inspection API
- [x] `InMemoryLLMEvaluator`, `InMemorySearchEngine`, `InMemoryJSRenderer`, `InMemoryScraperApiClient` — test doubles for all dependencies
- [x] Dependency injection via `ScraperPlugin::set*()` methods
- [x] `ScraperRunStats` counters (visited, forms, api pages, scraped, accepted, discarded, written, write errors, elapsed_ms)
- [x] Compile-time feature flags: `THEMIS_ENABLE_CURL`, `THEMIS_ENABLE_PUGIXML`, `THEMIS_ENABLE_LLM`
- [x] Unit tests for all components (scraper_plugin, scraper_config, api_client, search_engine, llm_evaluator, metadata_writer, gov_source_catalog)

---

## In Progress [~]

*(none — all v1.0.0 items are complete)*

---

## Planned Features

### v1.1.0 — Resilience and Observability

- [ ] Per-source retry logic with exponential backoff and jitter (Target: Q3 2026)
  - Inputs: configurable `max_retries` (default 3), `retry_delay_ms` (default 1000)
  - Errors: log attempt count, final failure counted in `write_errors` equivalent
  - Tests: unit — inject transient HTTP failure, verify retry and eventual success
- [ ] Structured per-run event log emitted to a `std::ostream` sink (Target: Q3 2026)
  - Format: JSON Lines, one event per line; keys: `ts`, `event`, `url`, `source`, `detail`
  - Activation: `ScraperPlugin::setLogSink(std::ostream*)` before `initialize()`
  - Tests: unit — verify all key events appear in log output
- [ ] `ScraperRunStats` extended with P50/P95/P99 per-document latency (Target: Q3 2026)
  - Inputs: `std::vector<long>` latency samples collected in `processDocument()`
  - Perf: latency sampling must not add more than 1 % overhead to the hot path
  - Tests: unit — inject N documents, verify percentile correctness

### v1.2.0 — Incremental Crawl and Deduplication

- [ ] Content-hash deduplication across runs via persistent seen-URL store (Target: Q4 2026)
  - Interface: `ISeenUrlStore` with `contains(url)` / `mark(url)` methods
  - Production implementation: bloom filter or SQLite-backed persistent store
  - Tests: unit — re-running same seeds produces 0 new writes
- [ ] Incremental crawl — only re-fetch pages modified since last run (Target: Q4 2026)
  - Mechanism: `If-Modified-Since` / `ETag` HTTP headers stored per URL
  - Config key: `crawl_options.incremental: true`
  - Tests: unit — inject 304 response, verify page is skipped
- [ ] Sitemap-driven crawl for `GovSearchStyle::SITEMAP` sources (Target: Q4 2026)
  - Inputs: `GovDataSource::sitemap_url`; parse `<loc>` and `<lastmod>` entries
  - Constraints: ignore entries with `<changefreq>never</changefreq>` in incremental mode
  - Tests: unit — inject sitemap XML, verify URL list matches expected

### v1.3.0 — Advanced Content Extraction

- [ ] Structured data extraction from JSON-LD / microdata / Open Graph meta tags (Target: Q1 2027)
  - Outputs: populate `ScrapedDocument::metadata` with extracted keys
  - Constraints: fallback to full-text extraction if structured data absent
  - Tests: unit — inject known JSON-LD snippet, verify metadata fields populated
- [ ] CSS selector-guided text extraction for known site templates (Target: Q1 2027)
  - Config: `crawl_options.content_selector` — CSS hint for the primary content container
  - Constraints: selector must be validated at `initialize()` time; invalid selector = warning, use full-text
  - Tests: unit — inject multi-section HTML, verify only targeted section extracted

---

## Production Readiness Checklist

- [x] All interfaces injectable (testable without network/DB)
- [x] SSRF guard in `UrlPolicy` (non-http/https schemes blocked)
- [x] Provenance fields unconditionally set in `ScraperRecordBuilder`
- [x] Compile-time feature isolation (curl/pugixml/LLM optional)
- [x] `robots.txt` respected by default (`respect_robots=true`)
- [x] Polite request delay enforced (`request_delay_ms=250` default)
- [x] Error isolation: per-URL failures do not abort the run
- [x] Unit tests for all components
- [ ] Integration test against a live gov portal (Target: Q3 2026)
- [ ] Prometheus metrics export for run stats (Target: Q3 2026)
- [ ] Retry logic for transient network failures (Target: Q3 2026)

---

## Known Issues & Limitations

- `robots.txt` parsing is a best-effort URL prefix match; full RFC 9309 compliance
  (crawl-delay, wildcard patterns) is not yet implemented.
- `HtmlSearchEngine` result parsing heuristics may miss non-standard result list
  containers on less common portals; `result_list_selector` can be used as a workaround.
- `ScraperLLMEvaluator` heuristic fallback uses keyword frequency only and may
  produce inaccurate scores for highly technical or niche-domain text.
- The built-in gov source catalog is static; sources requiring authentication
  (`GovDataSource::requires_auth = true`) are present in the catalog but not
  automatically activated until credential injection is implemented (v1.3.0).
- JS rendering requires an external renderer script/binary; none is bundled.
  A compatible Puppeteer/Playwright script must be supplied separately.

---

## Breaking Changes

- None in v1.0.0. Future breaking changes will be documented here with migration guidance.
