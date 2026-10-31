---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Scraper Implementation

**Purpose and audience:** Source map and runtime invariants for developers changing `src/scraper`.

## Runtime sequence

`ScraperPlugin::initialize()` stores the config, clears result/stat state, conditionally creates a subprocess JS renderer, and attempts a custom source-catalog overlay. Catalog-load exceptions are swallowed and the built-in catalog remains in use. The current implementation then marks the plugin initialized; it does not perform comprehensive semantic validation of every config field.

`scrape()` is synchronous and throws `std::runtime_error` if called before initialization. It collects allowed explicit/catalog seeds, optionally consumes one token per seed from an injected `BurstCrawlController`, selects the API or HTML/JS path, processes returned documents, flushes the writer, and returns run statistics. The call holds the plugin mutex across the run, including fetch/render work; do not invoke lifecycle methods concurrently or assume cancellation is available.

For HTML, the path is fetch/render → optional first discovered search form and configured/effective queries → result parsing and URL filtering → per-document fetch → text extraction → evaluation → record construction/write. Setting `search_options.enabled` false selects direct processing of each seed page. The API path calls `IScraperApiClient::fetchAll()` and applies evaluation and record writing to returned results.

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

`scraper_robots.*` and `scraper_sitemap.*` provide standalone helper types. The current `ScraperPlugin` orchestration does not call them. Likewise, `max_depth`, `max_pages`, `respect_robots`, and `same_domain_only` are parsed config fields but are not enforced by the main orchestration path. Do not treat those fields as active runtime protections; `UrlPolicy`, configured search-page limits, and API-client pagination limits are separate controls.

## Failure and data invariants

- A failed static fetch yields an empty body; a failed JS render does not produce a document. The plugin proceeds to other seeds.
- Search discovery can return no forms or no result items; this is a valid empty outcome, not proof that the source contains no relevant data.
- HTML text shorter than 50 characters is skipped before evaluation.
- API pagination stops on fetch exception, empty body, empty parsed batch, absent cursor, or configured page cap. A parser mismatch can therefore look like an empty successful result.
- Accepted documents are written with relational, graph, and vector record forms. Record builders set scraper provenance fields; the vector builder does not create embeddings.
- A failed individual writer call increments `write_errors` and does not abort the run. `flush()` is called at the end, but its boolean result is not reflected in returned statistics.

## Build and regression strategy

`src/scraper/CMakeLists.txt` builds the C++17 static target `scraper_plugin`. yaml-cpp, pugixml, and libcurl are optional: without yaml-cpp YAML loads throw; without pugixml HTML form/result extraction is empty; without libcurl the built-in HTTP client returns empty content. LLM evaluation requires `THEMIS_ENABLE_LLM`; otherwise the heuristic evaluator is used.

Use the focused sources in [`tests/scraper/`](../../tests/scraper/) for contract, plugin, rendering/pagination, burst, and run-summary regressions. Add tests for changed failure branches and inspect `ScraperRunStats`; do not rely on live web sources for deterministic unit coverage.

Related: [API.md](./API.md), [MODULE_GUIDE.md](./MODULE_GUIDE.md), [TRUST.md](./TRUST.md).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
