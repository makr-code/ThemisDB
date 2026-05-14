> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../include/scraper/README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB Scraper Module

**Version:** 1.0.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-05-13
**Module Path:** `src/scraper/`
**Namespace:** `themis::scraper`

---

## Module Purpose

The `scraper` module provides ThemisDB's agentic web-scraper plugin for automated
ingestion of publicly available legal and government data. It covers the full
pipeline from HTTP/JS page fetching, HTML search-form discovery, JSON REST/GraphQL
API crawling, and LLM-based quality evaluation, through to multi-model persistence
(relational, property graph, vector).

Key design goals:
- **Agentic loop** — seed expansion, form submission, pagination, and evaluation
  are driven autonomously per run.
- **Gap-aware quality scoring** — every document is scored against a `GapContext`
  (keyword set + description) to prioritize relevant content.
- **Multi-render support** — static HTTP, JS-rendered (headless browser subprocess),
  JSON REST, and GraphQL API endpoints.
- **Dependency injection** — all external dependencies (HTTP, HTML engine, LLM,
  DB writer, JS renderer) are injected via interfaces, enabling full unit-test
  coverage without network or DB access.
- **Provenance guarantee** — every written record carries mandatory `is_scraper_ingested`,
  `ingestion_source_type`, and `ingestion_plugin_version` fields set by
  `ScraperRecordBuilder`.

---

## Main Components

| File | Role |
|---|---|
| `scraper_plugin.h/.cpp` | `IScraperPlugin` + `ScraperPlugin` — top-level agentic orchestration |
| `scraper_config.h/.cpp` | `ScraperConfig`, `GapContext`, `CrawlOptions`, `UrlPolicy` — config loading and URL policy |
| `scraper_api_client.h/.cpp` | `IScraperApiClient` / `HttpScraperApiClient` — REST/GraphQL JSON page fetch with pagination |
| `scraper_search_engine.h/.cpp` | `IScraperSearchEngine` / `HtmlSearchEngine` — form discovery and result parsing (pugixml) |
| `scraper_llm_evaluator.h/.cpp` | `IScraperLLMEvaluator` / `ScraperLLMEvaluator` — LLM quality + gap-relevance scoring with heuristic fallback |
| `scraper_metadata_writer.h/.cpp` | `IScraperMetadataWriter` / `InMemoryScraperMetadataWriter` + `ScraperRecordBuilder` — persistence layer for relational, graph, and vector records |
| `scraper_js_renderer.h/.cpp` | `IScraperJSRenderer` / `SubprocessJSRenderer` — headless browser rendering via subprocess |
| `gov_source_catalog.h/.cpp` | `GovSourceCatalog` — built-in registry of Bund/Länder/EU portals |
| `CMakeLists.txt` | Module build rules |
| `PERFORMANCE_EXPECTATIONS.md` | Performance targets and benchmark mapping |

---

## Public API & Entry Points

Public API overview: [`../../include/scraper/README.md`](../../include/scraper/README.md)

Primary entry header: [`../../include/scraper/scraper_plugin.h`](../../include/scraper/scraper_plugin.h)

---

## Agentic Scraper Loop

For each run, `ScraperPlugin::scrape()` executes the following loop:

```
1. collectSeeds()
   ├── Explicit seed_urls from config
   └── Gov source catalog (Bund/Länder/EU) if enabled in gov_sources

2. For each (seed_url, gov_source_id):
   a. fetchPage(seed_url)   ← STATIC, JS_RENDERED, or API_JSON/GRAPHQL
   b. UrlPolicy::isAllowed() check
   c. runSearchLoop(seed_url, page_html, ...)
   │   ├── discoverForms() → SearchForm list
   │   ├── For each form × query:
   │   │   ├── buildSearchUrl() + fetchPage()
   │   │   └── For each result in SearchResultPage (paginated):
   │   │       └── processDocument(result.url, ...)
   │   └── Follow SearchResultPage::next_page_url up to max_result_pages
   └── runApiLoop() [when render_mode == API_JSON / API_GRAPHQL]
       └── IScraperApiClient::fetchAll() → ApiResult list
           └── processDocument() for each result

3. processDocument(url, html, source_name, gov_source_id, type, date):
   ├── extractText(html)
   ├── IScraperLLMEvaluator::evaluate() → EvaluationResult
   ├── If below threshold → mark discarded, skip write
   └── ScraperRecordBuilder::build*() → IScraperMetadataWriter::write()
```

---

## Configuration Options

All config options are documented in
[`../../include/scraper/README.md`](../../include/scraper/README.md) (Configuration — YAML Format section).

Key defaults:

| Option | Default | Notes |
|---|---|---|
| `crawl_options.max_depth` | `3` | Max link-follow depth per seed |
| `crawl_options.max_pages` | `500` | Hard cap on total fetched pages |
| `crawl_options.request_delay_ms` | `250` | Polite delay between requests |
| `crawl_options.render_mode` | `STATIC` | libcurl HTTP GET + HTML parsing |
| `crawl_options.respect_robots` | `true` | `robots.txt` is consulted |
| `search_options.max_result_pages` | `10` | Pagination limit per search query |
| `api_options.max_pages` | `20` | Pagination limit per API endpoint |
| `llm_options.quality_threshold` | `0.65` | Documents below this score are discarded |

---

## Error Handling and Edge Cases

- **Network errors** per URL (timeout, DNS failure, non-2xx response) are caught
  and logged; the scraper continues with remaining seeds without aborting.
- **LLM evaluation failure** silently falls back to the heuristic keyword-frequency
  scorer. The run completes normally; `ScraperLLMEvaluator::isLlmAvailable()` can
  be used to detect the fallback.
- **JS rendering timeout** produces `JsRenderResult{success=false}` for the
  affected page. The URL is logged and skipped; the run continues.
- **Write failures** are counted in `ScraperRunStats::write_errors` but do not
  abort the run. The `flush()` return value should be checked after the run.
- **UrlPolicy SSRF guard** silently rejects URLs with non-http/https schemes.
  The URL is counted as visited but no network request is made.
- **Config parse errors** from `loadFromFile()` / `loadFromYaml()` throw
  `std::runtime_error` immediately; no partial state is applied.
- **Uninitialized plugin** — calling `scrape()` before `initialize()` throws
  `std::logic_error`.

---

## Compile-Time Feature Flags

| Flag | Effect |
|---|---|
| `THEMIS_ENABLE_CURL` | Enables libcurl HTTP fetch in `HttpScraperApiClient` and `ScraperPlugin`; without it, HTTP fetches return empty strings |
| `THEMIS_ENABLE_PUGIXML` | Enables HTML parsing in `HtmlSearchEngine`; without it, form discovery and result parsing return empty results |
| `THEMIS_ENABLE_LLM` | Enables the LLM evaluation path; without it, `ScraperLLMEvaluator` always uses the heuristic fallback |

---

## Government Source Catalog

`GovSourceCatalog` ships with a built-in catalog of:
- **8 Bund (Federal) portals** — e.g. `gesetze_im_internet`, `bundesanzeiger`, `bgbl`
- **16 Bundesländer state law portals** — one per German state
- **5 EU portals** — EUR-Lex, CURIA, europarl.europa.eu, ec.europa.eu, publications.europa.eu

Sources are activated via `gov_sources.bund_enabled`, `bundeslaender_enabled`,
`eu_enabled`, or explicit `source_ids`. A custom YAML overlay can extend or
override built-in entries via `GovSourceCatalog::loadFromFile()`.

---

## Provenance Fields

Every record written by this plugin — relational, graph, and vector — carries
these three mandatory fields, set unconditionally by `ScraperRecordBuilder`:

| Field | Value |
|---|---|
| `is_scraper_ingested` | Always `true` |
| `ingestion_source_type` | Always `"SCRAPER"` |
| `ingestion_plugin_version` | Semver of the plugin build (default `"1.0.0"`) |

These fields must not be cleared or overridden by any downstream consumer.
They are the authoritative audit trail for automated web-scraping ingestion.

---

## Performance Targets

See [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) for full benchmark mapping.

| Target | Goal |
|---|---|
| Text extraction throughput | ≥ 50 MB/s for standard documents |
| End-to-end extraction P95 | ≤ 50 ms per document |
| JS renderer overhead | ≤ 20 % over the non-JS path |
| Metadata write path P99 | ≤ 15 ms |
| Throughput regression | ≤ 10 % vs. baseline under load |

---

## Tests

| Test file | Scope |
|---|---|
| `tests/test_scraper_plugin.cpp` | `ScraperPlugin` full agentic loop with mock dependencies |
| `tests/test_scraper_config.cpp` | `ScraperConfig` YAML loading, `UrlPolicy`, `effectiveSearchQueries()` |
| `tests/test_scraper_api_client.cpp` | `HttpScraperApiClient` pagination (page/cursor/offset), JSON parsing |
| `tests/test_scraper_search_engine.cpp` | `HtmlSearchEngine` form discovery, result parsing, URL building |
| `tests/test_scraper_llm_evaluator.cpp` | `ScraperLLMEvaluator` heuristic path + mock LLM path |
| `tests/test_scraper_metadata_writer.cpp` | `ScraperRecordBuilder` provenance fields, `InMemoryScraperMetadataWriter` |
| `tests/test_gov_source_catalog.cpp` | `GovSourceCatalog` built-in sources, YAML overlay, filter methods |

Run with:

```bash
ctest --preset linux-release --output-on-failure -j 1 --timeout 60 -R "scraper"
```

---

## Installation

For in-tree builds both include roots must be on the include path:

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}          # include/
    ${THEMISDB_SOURCE_DIR}/src       # src/ (for source-local headers)
)
```

Optionally enable backend features at configure time:

```cmake
target_compile_definitions(your_target PRIVATE
    THEMIS_ENABLE_CURL     # link libcurl for HTTP fetch
    THEMIS_ENABLE_PUGIXML  # link pugixml for HTML parsing
    THEMIS_ENABLE_LLM      # enable LLM evaluator path
)
```

---

## Usage

See [`../../include/scraper/README.md`](../../include/scraper/README.md) for full usage examples
including dependency injection for tests.

Quick start:

```cpp
#include "scraper/scraper_plugin.h"
#include "scraper/scraper_config.h"

using namespace themis::scraper;

ScraperConfig cfg = ScraperConfig::loadFromFile("/etc/themis/scraper.yaml");
ScraperPlugin plugin;

if (!plugin.initialize(cfg)) {
    throw std::runtime_error("Scraper initialization failed");
}

ScraperRunStats stats = plugin.scrape();
```

---

## See Also

- [`../../include/scraper/README.md`](../../include/scraper/README.md) — public header API reference
- [`ROADMAP.md`](./ROADMAP.md) — phased delivery status
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — planned feature work
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — component hierarchy and data flow
- [`AUDIT.md`](./AUDIT.md) — audit findings and compliance status
- [`SECURITY.md`](./SECURITY.md) — threat model and security controls
- [`CHANGELOG.md`](./CHANGELOG.md) — version history
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — performance targets and benchmarks
- [`../../docs/de/scraper/README.md`](../../docs/de/scraper/README.md) — German secondary overview
