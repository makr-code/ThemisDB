# Scraper Plugin – Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.1.0 – Provenance flags on all record types; comprehensive 56-source `knowledge_sources.yaml` catalog; 60 unit tests.

## Completed ✅

### Phase 1 – Config & URL Policy
- [x] `ScraperConfig` YAML loader via yaml-cpp
- [x] `UrlPolicy` with whitelist / blacklist and glob patterns
- [x] SSRF prevention (http/https only)
- [x] `ScraperRenderMode` enum (STATIC, JS_RENDERED, API_JSON, API_GRAPHQL)
- [x] `GovSourcesOptions` selecting Bund / Bundesländer / EU groups

### Phase 2 – Government Source Catalog
- [x] 8 German federal (Bund) sources
- [x] All 16 Bundesland state law portals
- [x] 5 EU sources (EUR-Lex HTML + SPARQL, CURIA, EP, Publications Office)
- [x] YAML overlay for custom / additional sources
- [x] `GovSourceCatalog::byIds()` for explicit source selection

### Phase 3 – Scraping Core
- [x] `HtmlSearchEngine` – form discovery (pugixml)
- [x] `HtmlSearchEngine` – openjur.de-style result-list parsing
- [x] `HtmlSearchEngine` – next-page detection (rel=next, class hints, German labels)
- [x] `SubprocessJSRenderer` – React/SPA rendering via external command
- [x] `HttpScraperApiClient` – page / cursor / offset pagination
- [x] Agentic plugin loop in `ScraperPlugin::scrape()`

### Phase 4 – LLM Evaluation & Metadata
- [x] `ScraperLLMEvaluator` with LLM path (THEMIS_ENABLE_LLM) + heuristic fallback
- [x] `InMemoryScraperMetadataWriter` – relational + graph + vector records
- [x] `ScraperRecordBuilder` – doc-id (FNV-1a), ISO-8601 timestamps, edge building
- [x] Initial 40 unit tests (`ScraperPluginFocusedTests`, Groups A–H; extended to 60 total by Phase 5)

### Phase 5 – Provenance & Knowledge Catalog (v1.1.0) ✅
- [x] Provenance fields on all record types: `is_scraper_ingested`, `ingestion_source_type`, `ingestion_plugin_version` stamped unconditionally by `ScraperRecordBuilder`
- [x] `config/knowledge_sources.yaml` — comprehensive 56-source catalog (7 EU law, 3 EU data, 10 Bund, 16 Bundesland, 7 standards, 5 general knowledge, 4 scientific)
- [x] `ScraperRecordBuilder::kPluginVersion` static semver constant
- [x] 20 new unit tests (Groups I–L) covering provenance flags, catalog completeness, end-to-end propagation, and immutability

## Completed (v1.2.0) ✅

### Short-term (v1.2.0)
- [x] SPARQL query support for EUR-Lex CELLAR endpoint (Target: Q3 2026) — evidence: include/scraper/scraper_api_client.h (SparqlQueryBuilder, SparqlApiClient), src/scraper/scraper_plugin.cpp (runSparqlLoop)
- [x] Bundestag DIP API key management via environment variable `BUNDESTAG_API_KEY` (Target: Q3 2026) — evidence: src/scraper/gov_source_catalog.cpp (api_key_env), src/scraper/scraper_plugin.cpp (getenv)
- [x] Robots.txt respect in `ScraperPlugin::fetchPage()` (Target: Q3 2026) — evidence: include/scraper/scraper_robots.h (RobotsTxtCache), src/scraper/scraper_robots.cpp, src/scraper/scraper_plugin.cpp
- [x] Sitemap-driven crawl mode for `GovSearchStyle::SITEMAP` sources (Target: Q3 2026) — evidence: include/scraper/scraper_sitemap.h (SitemapCrawler), src/scraper/scraper_sitemap.cpp, src/scraper/scraper_plugin.cpp (runSitemapLoop)
- [x] Embedding generation for `ScraperVectorRecord` (Target: Q3 2026) — evidence: include/scraper/scraper_plugin.h (setEmbeddingFn/EmbeddingFn), src/scraper/scraper_plugin.cpp
- [x] 16 new focused tests (V12-01..V12-16) covering all v1.2.0 features — evidence: tests/scraper/test_scraper_v12_features_focused.cpp

## Planned Features 📋

### Medium-term (v1.3.0)
- [ ] Parallel crawl with configurable concurrency limit (Target: Q4 2026)
  - Thread pool limited to `crawl_options.max_concurrent_requests`
  - Per-domain rate limiting using token buckets
- [ ] Incremental checkpointing – resume interrupted runs (Target: Q4 2026)
  - Store last-visited URL + page per source in persistent checkpoint store

### Long-term (v2.0.0)
- [ ] Full DB write integration (replace `InMemoryScraperMetadataWriter`) (Target: Q1 2027)
  - Relational: AQL INSERT into `scraper_documents` collection
  - Graph: create nodes/edges in property graph via graph module API
  - Vector: `IAnnIndex::build()` for embedding vectors
- [ ] Plugin sandbox enforcement (network + filesystem restrictions) (Target: Q2 2027)
- [ ] Prometheus metrics export (docs_scraped, docs_accepted, eval_scores) (Target: Q2 2027)

## Implementation Phases

### Phase 1: Design / Config (Status: Completed ✅)
- [x] ScraperConfig YAML schema
- [x] UrlPolicy with glob + SSRF guard
- [x] GovSourcesOptions

### Phase 2: Gov Catalog (Status: Completed ✅)
- [x] Built-in Bund sources (8)
- [x] Built-in Bundesland sources (16)
- [x] Built-in EU sources (5)
- [x] YAML overlay

### Phase 3: Core Scraping (Status: Completed ✅)
- [x] HTML form discovery + result-list parsing
- [x] JS renderer subprocess interface
- [x] JSON API client with pagination

### Phase 4: Evaluation & Write (Status: Completed ✅)
- [x] LLM evaluator + heuristic fallback
- [x] Metadata writer (relational, graph, vector)
- [x] Agentic loop
- [x] 40 unit tests covering Groups A–H (v1.0.0; extended by Phase 5)

### Phase 5: Provenance & Knowledge Catalog (v1.1.0) (Status: Completed ✅)
- [x] Provenance fields on all record types (`is_scraper_ingested`, `ingestion_source_type`, `ingestion_plugin_version`)
- [x] `config/knowledge_sources.yaml` — 56-source comprehensive catalog
- [x] `ScraperRecordBuilder::kPluginVersion` constant
- [x] 20 new unit tests (Groups I–L)

### Phase 6: v1.2.0 – SPARQL / robots.txt / Sitemap / Embeddings (Status: Completed ✅)
- [x] `SparqlQueryBuilder` + `SparqlApiClient` for EUR-Lex CELLAR (include/scraper/scraper_api_client.h)
- [x] `RobotsTxtCache` with parse/isAllowed/Allow-override (include/scraper/scraper_robots.h)
- [x] `SitemapCrawler` with sitemap-index recursion (include/scraper/scraper_sitemap.h)
- [x] `ScraperPlugin::setEmbeddingFn()` — embedding callback wired into all write paths
- [x] `BUNDESTAG_API_KEY` env-var read and forwarded as ******
- [x] 16 focused tests V12-01..V12-16

### Phase 7: Performance & Hardening (Status: Planned)
- [ ] Parallel crawl
- [ ] Checkpointing

### Phase 8: Full DB Integration (Status: Planned)
- [ ] Real AQL / graph / vector writes

## Production Readiness Checklist

- [x] No stub methods
- [x] Unit tests ≥ 76 covering all components (v1.2.0: +16 V12-series)
- [x] SSRF prevention in UrlPolicy
- [x] LLM guard (THEMIS_ENABLE_LLM compile flag)
- [x] Fallback when LLM unavailable
- [x] CI workflow (3 OS × 3 compiler matrix)
- [x] YAML config + gov catalog documentation
- [x] Provenance fields on all record types (v1.1.0)
- [x] Comprehensive 56-source knowledge catalog (v1.1.0)
- [x] SPARQL query support for EUR-Lex CELLAR (v1.2.0)
- [x] robots.txt enforcement (v1.2.0)
- [x] Sitemap-driven crawl mode (v1.2.0)
- [x] Embedding generation hook (v1.2.0)
- [x] BUNDESTAG_API_KEY env-var management (v1.2.0)
- [ ] Parallel crawl with rate limiting
- [ ] Incremental checkpointing
- [ ] Full DB write integration
- [ ] Prometheus metrics

## Known Issues & Limitations

- `InMemoryScraperMetadataWriter` is used by default; real DB writes require
  implementing a `ThemisDBScraperMetadataWriter` backed by the AQL/graph/vector APIs.
- JS renderer requires an external Node.js/Puppeteer script; no bundled script is included.
- The Bundestag DIP API requires a free API key (env `BUNDESTAG_API_KEY`).
- Parallel crawl is not yet implemented; all requests are sequential.
