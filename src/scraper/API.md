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

# Scraper API Contract

**Purpose and audience:** Integration reference for C++ callers of `themis::scraper`, based on public declarations under `include/scraper/`.

## Entry point and lifecycle

Include `scraper/scraper_plugin.h` and construct `ScraperPlugin` or an `IScraperPlugin` implementation. The supported lifecycle is:

1. Build a `ScraperConfig` (directly or through `loadFromFile()` / `loadFromYaml()` when yaml-cpp is enabled).
2. Call `initialize(config)`.
3. Call synchronous `scrape()` and retain the returned `ScraperRunStats`.
4. Read `getResults()` for accepted and discarded documents.
5. Call `reset()` before reusing the instance for a new run.

`scrape()` before initialization throws `std::runtime_error` in the current implementation. `initialize()` currently marks the plugin initialized after storing the config and does not validate all field combinations. Configuration parse errors are exceptions; without yaml-cpp, both YAML loader methods throw.

## Configuration and policy

`ScraperConfig` groups `GapContext`, `CrawlOptions`, `SearchOptions`, `ApiOptions`, `LlmOptions`, and `GovSourcesOptions`, plus `seed_urls`, `whitelist`, and `blacklist`. Explicit search queries take precedence; otherwise gap keywords are used. `api_options` is parsed by the YAML loader but is not consumed by the current `ScraperPlugin` API path. Catalog-backed REST sources construct endpoint settings from their `GovDataSource`; other API calls use `ApiEndpointConfig` defaults. The separate `HttpScraperApiClient` supports `ApiEndpointConfig::body_template` substitutions `{{QUERY}}`, `{{PAGE}}`, and `{{CURSOR}}` when callers configure and invoke that client directly.

`UrlPolicy` permits only `http://` and `https://`. A blacklist match always rejects; an empty whitelist permits any remaining URL, and a non-empty whitelist requires a prefix or the supported leading/trailing `*` match. Patterns are string-based, not a parsed hostname/ IP policy. See [TRUST.md](./TRUST.md) before exposing this API to untrusted callers.

The plugin applies this policy to explicit seeds and HTML search-result URLs, but selected catalog seed URLs are added without a plugin-side policy check; the exact API endpoint is exempted during API-result filtering. Redirect destinations are also not rechecked.

The config contains fields (`respect_robots`, `same_domain_only`, `max_depth`, and crawl `max_pages`) that are currently parsed but not applied by the main plugin loop. `request_delay_ms` is used in the HTML search path, not in API pagination. Do not use these settings as a security boundary or assume they limit the crawl. HTML result traversal uses `max_result_pages`; the API client's `ApiEndpointConfig::max_pages` limits pagination only when configured on the endpoint passed to that client.

Other parsed fields not consumed by the current plugin path include `SearchOptions::results_per_page`, `LlmOptions::model_path`, `LlmOptions::temperature`, and `ScraperConfig::api_options`. The evaluator uses the default LLM plugin and a fixed request temperature when LLM inference is available.

## Extension interfaces

| Interface | Contract |
|---|---|
| `IScraperSearchEngine` | Discovers forms, parses result pages, and builds search URLs/bodies. Empty discovery/results are valid outcomes. |
| `IScraperApiClient` | `fetchAll(cfg, query)` returns parsed results across bounded pagination; network errors and empty/invalid result batches terminate pagination. |
| `IScraperJSRenderer` | `render(request)` returns HTML on success or an error result; it is not required for static mode. |
| `IScraperLLMEvaluator` | Scores text against gap context and quality threshold; result includes separate quality and relevance values. |
| `IScraperMetadataWriter` | Writes relational, graph, and vector records and exposes `flush()`. Partial layer status is represented by `WriteResult`. |

`ScraperPlugin` exposes setters for these dependencies, an injectable simple HTTP function, and an optional burst controller (disabled by default). The default writer is in-memory; durable persistence requires an explicitly supplied writer.

## Results and failure behavior

`ScrapedDocument` contains source URL, title, raw/extracted content when produced by the HTML path, evaluator scores, discard state, document ID, and scraper provenance. `ScraperRunStats` reports visited/skipped URLs, API/search activity, accepted/discarded/written counts, write errors, and elapsed time. Some counters are path-specific: `docs_accepted` means evaluation passed, even if persistence then fails. A failed `flush()` result is not included in the stats.

The vector-record builder currently leaves `embedding` empty; see [EMBEDDING.md](./EMBEDDING.md). The API is synchronous and exposes no cancellation or per-source pause method. `js_timeout_ms` is passed as a renderer argument; the subprocess adapter itself does not enforce a wall-clock kill timeout. The `scraper_render_contract.h` classifier is not called by `ScraperPlugin`.

Canonical declarations: [`scraper_plugin.h`](../../include/scraper/scraper_plugin.h), [`scraper_config.h`](../../include/scraper/scraper_config.h), and [MODULE_GUIDE.md](./MODULE_GUIDE.md).

For scoring semantics and evaluation methodology, see [MODELING.md](./MODELING.md) and [RESEARCH.md](./RESEARCH.md).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
