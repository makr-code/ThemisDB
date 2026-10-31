---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Scraper Implementation

**Purpose and audience:** Source map and runtime invariants for developers changing `src/scraper`.

## Runtime sequence

`ScraperPlugin::initialize()` stores the config, clears result/stat state, conditionally creates a subprocess JS renderer, and attempts a custom source-catalog overlay. Catalog-load exceptions are swallowed and the built-in catalog remains in use. The current implementation then marks the plugin initialized; it does not perform comprehensive semantic validation of every config field.

`scrape()` is synchronous and throws `std::runtime_error` if called before initialization. It collects allowed explicit/catalog seeds, optionally consumes one token per seed from an injected `BurstCrawlController`, selects the API or HTML/JS path, processes returned documents, flushes the writer, and returns run statistics. The call holds the plugin mutex across the run, including fetch/render work; do not invoke lifecycle methods concurrently or assume cancellation is available.

For HTML, the path is fetch/render → optional first discovered search form and configured/effective queries → result parsing and URL filtering → per-document fetch → text extraction → evaluation → record construction/write. Setting `search_options.enabled` false selects direct processing of each seed page. The API path calls `IScraperApiClient::fetchAll()` and applies evaluation and record writing to returned results. `api_options` is parsed but not used to configure this path; catalog-backed endpoint settings come from `GovDataSource`, while other endpoint settings use `ApiEndpointConfig` defaults. `search_options.results_per_page`, `llm_options.model_path`, and `llm_options.temperature` are also parsed but unused by the current plugin path. The evaluator selects the default LLM plugin and fixes the request temperature at 0.1. The plugin does not apply `request_delay_ms` to API pagination.

`reset()` clears results and counters but leaves the plugin initialized and retains its configuration and injected dependencies. `initialize()` stores a configuration and resets run state; use it when selecting a new config.

## Source ownership

| Responsibility | Implementation | Contract/evidence |
|---|---|---|
| YAML/config and URL policy | `scraper_config.cpp` | [`scraper_config.h`](../../include/scraper/scraper_config.h) |
| Orchestration and text extraction | `scraper_plugin.cpp` | [`scraper_plugin.h`](../../include/scraper/scraper_plugin.h) |
| JSON/REST pagination | `scraper_api_client.cpp` | [`scraper_api_client.h`](../../include/scraper/scraper_api_client.h) |
| HTML form/result heuristics | `scraper_search_engine.cpp` | [`scraper_search_engine.h`](../../include/scraper/scraper_search_engine.h) |
| JS process adapter | `scraper_js_renderer.cpp` | [`scraper_js_renderer.h`](../../include/scraper/scraper_js_renderer.h) |
| LLM/heuristic score | `scraper_llm_evaluator.cpp` | [`scraper_llm_evaluator.h`](../../include/scraper/scraper_llm_evaluator.h) |
| Relational/graph/vector record builders | `scraper_metadata_writer.cpp` | [`scraper_metadata_writer.h`](../../include/scraper/scraper_metadata_writer.h) |
| Built-in and overlaid sources | `gov_source_catalog.cpp` | [`gov_source_catalog.h`](../../include/scraper/gov_source_catalog.h) |

`scraper_robots.*` and `scraper_sitemap.*` provide helper declarations and implementations, but neither `.cpp` file is listed in `src/scraper/CMakeLists.txt` and neither is added by `tests/scraper/CMakeLists.txt`. Their symbols are therefore not supplied by the normal `scraper_plugin` target, and the scoped scraper tests do not cover them. TODO: add the sources to a build target and focused tests, or document and support a separate target before treating these helpers as generally linkable.

The current `ScraperPlugin` orchestration does not call these helpers. Likewise, `max_depth`, crawl `max_pages`, `respect_robots`, and `same_domain_only` are parsed config fields but are not enforced by the main orchestration path. `request_delay_ms` is used only in the HTML search path. Do not treat these fields as active runtime protections; `UrlPolicy`, configured HTML result-page limits, and API-client endpoint pagination limits are separate controls.

## Failure and data invariants

- A failed static fetch yields an empty body; a failed JS render does not produce a document. The plugin proceeds to other seeds.
- Search discovery can return no forms or no result items; this is a valid empty outcome, not proof that the source contains no relevant data.
- HTML text shorter than 50 characters is skipped before evaluation.
- API pagination stops on fetch exception, empty body, empty parsed batch, absent cursor, or configured page cap. A parser mismatch can therefore look like an empty successful result.
- Accepted documents are written with relational, graph, and vector record forms. Record builders set scraper provenance fields; the vector builder does not create embeddings.
- A failed individual writer call increments `write_errors` and does not abort the run. `flush()` is called at the end, but its boolean result is not reflected in returned statistics.

## Build and regression strategy

`src/scraper/CMakeLists.txt` builds the C++17 static target `scraper_plugin` from its explicit `SCRAPER_SOURCES` list. That list omits `scraper_robots.cpp` and `scraper_sitemap.cpp`; their headers do not make their implementation symbols available through this archive. yaml-cpp, pugixml, and libcurl are optional: without yaml-cpp YAML loads throw; without pugixml HTML form discovery/result-list parsing is empty (direct text extraction has a fallback tag stripper); without libcurl the built-in HTTP client returns empty content. nlohmann_json is discovered optionally, with a repository-header fallback. `THEMIS_ENABLE_LLM` enables the LLM evaluator path; otherwise the heuristic evaluator is used.

Use the focused sources in [`tests/scraper/`](../../tests/scraper/) for contract, plugin, rendering/pagination, burst, and run-summary regressions. The test CMake file does not add the robots/sitemap implementations, and no scoped test references those helper types. Add build registration and focused tests before relying on them; do not rely on live web sources for deterministic unit coverage.

Related: [API.md](./API.md), [MODULE_GUIDE.md](./MODULE_GUIDE.md), [TRUST.md](./TRUST.md).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
