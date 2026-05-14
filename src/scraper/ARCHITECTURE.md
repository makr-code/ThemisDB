> **Status:** 2026-05-13 — Architecture reflects actual source and headers.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../include/scraper/README.md -->

# Architecture — Scraper Module

## Overview

The `scraper` module provides ThemisDB's agentic web-scraper plugin for automated
ingestion of publicly available legal and government data.  It is structured as a
layered pipeline with full dependency injection, enabling complete unit-test
coverage without network or database access.

## Module Position in ThemisDB

```
┌──────────────────────────────────────────────────────────────────┐
│ ThemisDB Plugin System (src/plugins/)                             │
│                                                                  │
│  ScraperPlugin  ◄── this module (top-level orchestrator)         │
│   ├── collectSeeds()         GovSourceCatalog + config seeds     │
│   ├── fetchPage()            IScraperApiClient / IScraperJSRenderer│
│   ├── runSearchLoop()        IScraperSearchEngine (form discovery)│
│   ├── runApiLoop()           IScraperApiClient (REST/GraphQL)     │
│   └── processDocument()      IScraperLLMEvaluator                │
│                               └── IScraperMetadataWriter         │
└──────────────────────────────────────────────────────────────────┘
          │                       │                    │
          ▼                       ▼                    ▼
  External HTTP/JS           LLM Service         ThemisDB Storage
  (libcurl, subprocess)  (REST/local model)  (relational/graph/vector)
```

## Component Hierarchy

```
themis::scraper
├── IScraperPlugin / ScraperPlugin       (scraper_plugin.h/.cpp)
│   ├── initialize(ScraperConfig)        → bool
│   ├── scrape()                         → ScraperRunStats
│   ├── set*()                           dependency injection setters
│   └── ScraperRunStats                  counters + elapsed_ms
│
├── ScraperConfig                        (scraper_config.h/.cpp)
│   ├── loadFromFile(path)               → ScraperConfig
│   ├── loadFromYaml(yaml_str)           → ScraperConfig
│   ├── CrawlOptions                     max_depth/pages, render_mode, delay
│   ├── SearchOptions                    queries, max_result_pages
│   ├── ApiOptions                       endpoints, max_pages
│   ├── LlmOptions                       quality_threshold
│   ├── GapContext                       keywords + description for scoring
│   └── UrlPolicy                        whitelist/blacklist/SSRF guard
│
├── IScraperApiClient / HttpScraperApiClient  (scraper_api_client.h/.cpp)
│   └── fetchAll(endpoint, options)      → Result<vector<ApiResult>>
│       pagination: page / cursor / offset
│
├── IScraperSearchEngine / HtmlSearchEngine   (scraper_search_engine.h/.cpp)
│   ├── discoverForms(html, base_url)    → vector<SearchForm>
│   └── parseResults(html, base_url)     → SearchResultPage
│       (JSON-LD, CSS heuristic, fallback)
│
├── IScraperLLMEvaluator / ScraperLLMEvaluator  (scraper_llm_evaluator.h/.cpp)
│   ├── evaluate(text, gap_context)      → EvaluationResult
│   ├── isLlmAvailable()                 → bool
│   └── [heuristic fallback: keyword frequency scorer]
│
├── IScraperMetadataWriter / InMemoryScraperMetadataWriter  (scraper_metadata_writer.h/.cpp)
│   ├── ScraperRecordBuilder             builds relational/graph/vector records
│   └── write(record)                   → Result<void>
│       provenance: is_scraper_ingested, ingestion_source_type, ingestion_plugin_version
│
├── IScraperJSRenderer / SubprocessJSRenderer  (scraper_js_renderer.h/.cpp)
│   ├── render(url)                      → JsRenderResult
│   └── isAvailable()                    → bool
│
└── GovSourceCatalog                     (gov_source_catalog.h/.cpp)
    ├── getAll()                         → vector<GovDataSource>
    ├── getBySourceId(id)                → optional<GovDataSource>
    ├── getEnabled(gov_sources_config)   → vector<GovDataSource>
    └── loadFromFile(path)               YAML overlay support
        [Built-in: 8 Bund + 16 Bundesländer + 5 EU portals]
```

## Agentic Loop Data Flow

```
ScraperPlugin::scrape()
        │
        ▼ 1. collectSeeds()
┌───────────────────────┐
│ seed_urls (config)    │
│ GovSourceCatalog      │──→ List<(url, gov_source_id)>
└───────────────────────┘
        │
        ▼ 2. For each seed:
fetchPage(url, render_mode)
  ├── STATIC       → libcurl HTTP GET
  ├── JS_RENDERED  → SubprocessJSRenderer (headless browser subprocess)
  ├── API_JSON     → HttpScraperApiClient (page/cursor/offset pagination)
  └── API_GRAPHQL  → HttpScraperApiClient (GraphQL pagination)
        │
        ▼ 3. runSearchLoop() [STATIC / JS_RENDERED]
discoverForms(html)  →  List<SearchForm>
  └── For each form × query:
        buildSearchUrl() + fetchPage()
        parseResults(html)  →  SearchResultPage
          └── processDocument() per result
                │
                ▼ 4. processDocument(url, html)
        extractText(html)
                │
                ▼
        IScraperLLMEvaluator::evaluate()
          ├── LLM path (if available)
          └── Heuristic fallback (keyword frequency)
                │ score >= threshold?
                ▼
        ScraperRecordBuilder::build*()
                │
                ▼
        IScraperMetadataWriter::write()  →  storage (relational/graph/vector)
```

## Dependency Direction

```
scraper/ → utils/         (logging — permitted)
scraper/ → [libcurl]      (HTTP fetch — compile-time flag THEMIS_ENABLE_CURL)
scraper/ → [pugixml]      (HTML parsing — compile-time flag THEMIS_ENABLE_PUGIXML)
scraper/ → [LLM service]  (evaluation — compile-time flag THEMIS_ENABLE_LLM)
plugins/ → scraper/       (ScraperPlugin registered in plugin system — permitted)
```

## Compile-Time Feature Isolation

| Flag | Enabled behaviour | Disabled fallback |
|------|------------------|-------------------|
| `THEMIS_ENABLE_CURL` | libcurl HTTP fetch | Empty string returned for all fetches |
| `THEMIS_ENABLE_PUGIXML` | HTML form/result parsing | Empty results |
| `THEMIS_ENABLE_LLM` | LLM evaluation path | Heuristic keyword fallback always used |

## Interfaces

- **Public API:** `include/scraper/` — all `I*` interfaces and concrete types
- **Test doubles:** `InMemoryLLMEvaluator`, `InMemorySearchEngine`,
  `InMemoryJSRenderer`, `InMemoryScraperApiClient` — full mock suite
- **Provenance contract:** `ScraperRecordBuilder` must unconditionally set
  `is_scraper_ingested`, `ingestion_source_type`, and `ingestion_plugin_version`
  on every record it builds.

## See Also

- Implementation overview: [`README.md`](./README.md)
- Public API: [`../../include/scraper/README.md`](../../include/scraper/README.md)
- Roadmap: [`ROADMAP.md`](./ROADMAP.md)
- Future Enhancements: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Security: [`SECURITY.md`](./SECURITY.md)
- Performance: [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
