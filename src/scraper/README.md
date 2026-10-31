# ThemisDB Scraper Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

## Module Purpose

The scraper module provides a synchronous C++17 ingestion pipeline over configured seed URLs and selected catalog sources. It supports static/JS retrieval, HTML search-form discovery, JSON REST/GraphQL client paths, document evaluation, and relational/graph/vector record construction through an injected writer. The default writer is in-memory; durable storage is supplied by an integration.

## Data Flow

1. `ScraperPlugin::initialize()` stores configuration, resets run state, optionally constructs the configured JS renderer, and attempts a source-catalog overlay. Overlay-load errors are swallowed; initialization otherwise returns `true` without comprehensive config validation.
2. `scrape()` collects explicit and selected catalog seeds. `UrlPolicy` filters explicit seeds but the current loop does not apply it to catalog seeds; an injected `BurstCrawlController`, when present, gates each seed.
3. API-mode seeds go to `IScraperApiClient::fetchAll()`. HTML/JS seeds are fetched/rendered; with search enabled the plugin uses the first discovered form and effective queries, parses results, then fetches each allowed result. With search disabled it processes the seed page directly.
4. HTML text or flattened API text is evaluated. Accepted documents are converted to relational, graph, and vector records and passed to `IScraperMetadataWriter`; the writer is flushed after the run.

The pipeline does not emit the structured events declared in `scraper_diagnostics.h`; diagnostics, run summaries, and event sinks are standalone integration primitives. `scraper_robots.*` and `scraper_sitemap.*` are also standalone helpers, not invoked by `ScraperPlugin`. Their `.cpp` files are omitted from both the `scraper_plugin` source list and the scoped test CMake registration, so the normal plugin archive does not provide their symbols and scoped tests do not cover them.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| scraper_plugin.cpp | top-level scrape orchestration lifecycle |
| scraper_config.cpp | scraper configuration and policy parsing behavior |
| scraper_api_client.cpp | API/HTTP fetch and pagination behavior |
| scraper_search_engine.cpp | form discovery and result extraction behavior |
| scraper_js_renderer.cpp | JS-rendered fetch support behavior |
| scraper_llm_evaluator.cpp | quality/relevance scoring and fallback behavior |
| scraper_metadata_writer.cpp | persistence-ready record and provenance write behavior |
| gov_source_catalog.cpp | catalog-backed seed/source registry behavior |

## Scope

In scope:
- seed discovery, fetch, parse, evaluate, and write pipeline behavior
- static/JS/API render and crawl loops
- provenance-safe metadata generation and write behavior

Out of scope:
- external website/API ownership outside scraper contracts
- global storage/query internals owned by other modules
- non-scraper ingestion domain logic

## Runtime Behavior and Limits

- `UrlPolicy` allows only HTTP(S), then applies blacklist-before-whitelist string-pattern checks. Explicit seed URLs and HTML result URLs are checked; selected catalog seed URLs are added without a plugin-side policy check, and the exact API endpoint is exempted when API results are checked. An empty whitelist allows all HTTP(S) URLs; this is not a complete network-egress/SSRF boundary.
- `search_options.max_result_pages` bounds HTML result-page iteration; API pagination is bounded by the endpoint configuration used by the API client.
- `crawl_options.max_depth`, `max_pages`, `respect_robots`, and `same_domain_only` are parsed but not enforced by the main plugin loop. `request_delay_ms` is used in the HTML search path, not by API pagination.
- `api_options`, `search_options.results_per_page`, `llm_options.model_path`, and `llm_options.temperature` are parsed but not consumed by the current plugin path. API endpoint settings come from the selected catalog source or the API client's defaults; the evaluator uses a default LLM plugin and fixed request temperature.
- Without yaml-cpp, YAML loaders throw. Without libcurl, the built-in HTTP fetch returns empty content; without pugixml, form discovery/result-list parsing returns empty results (direct text extraction has a fallback tag stripper). LLM inference uses a keyword/length heuristic when unavailable or failing.
- `js_timeout_ms` is passed to the renderer command; the subprocess adapter does not independently terminate a renderer that hangs. The `scraper_render_contract.h` timeout helper is not wired into the plugin.
- `reset()` clears results and counters but does not de-initialize the plugin. Call `initialize()` with the next config to reset state and configure another run.

## Sourcecode Verification (Module: scraper/readme)

- Verified files:
  - src/scraper/scraper_plugin.cpp
  - src/scraper/scraper_config.cpp
  - src/scraper/scraper_api_client.cpp
  - src/scraper/scraper_search_engine.cpp
  - src/scraper/scraper_js_renderer.cpp
  - src/scraper/scraper_llm_evaluator.cpp
  - src/scraper/scraper_metadata_writer.cpp
  - src/scraper/gov_source_catalog.cpp
- Verified behavior surfaces:
  - crawl/fetch/search/evaluate/write pipeline and catalog seeding behavior
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md

## Build and Tests

`src/scraper/CMakeLists.txt` is a CMake project that defines the static `scraper_plugin` target (C++17) from an explicit source list. That target omits `scraper_robots.cpp` and `scraper_sitemap.cpp`; do not assume their symbols are linkable through the normal plugin archive. A direct module configure/build is:

```bash
cmake -S src/scraper -B build/scraper
cmake --build build/scraper --target scraper_plugin
```

yaml-cpp, pugixml, and libcurl are discovered optionally; the nlohmann_json package is optional but the header must be available from that package or the configured repository fallback. `THEMIS_ENABLE_LLM` enables the LLM evaluator path. This module command does not build the test targets.

Focused tests are registered from `tests/scraper/CMakeLists.txt`: each `test_scraper_*.cpp` becomes a `module_scraper_<filename>_focused` target and a `<filename>_scraper_FocusedTests` CTest entry labeled `scraper`. See [MODULE_GUIDE.md](./MODULE_GUIDE.md) for focused regression guidance.

The test CMake file neither adds these helper implementations nor has scoped tests for `RobotsTxtCache` or `SitemapCrawler`. TODO: register the implementations and focused tests, or define/document a separate supported build target.