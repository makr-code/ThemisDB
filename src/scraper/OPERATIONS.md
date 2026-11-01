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

# Scraper Operations

**Purpose and audience:** Deployment and routine-operation notes for the team embedding the scraper library in a service or job runner.

## Runtime and dependencies

`src/scraper/CMakeLists.txt` builds a C++17 static library named `scraper_plugin`. Optional dependencies change behavior, not just performance:

| Dependency/flag | Operational consequence when absent |
|---|---|
| yaml-cpp / `THEMIS_ENABLE_YAML` | YAML configuration loaders throw instead of parsing. |
| libcurl / `THEMIS_ENABLE_CURL` | Built-in HTTP fetch returns empty content; API client returns empty results. |
| pugixml / `THEMIS_ENABLE_PUGIXML` | HTML form discovery and result-list parsing return empty results. |
| `THEMIS_ENABLE_LLM` plus an available LLM plugin | Otherwise the evaluator uses its keyword/length heuristic. |
| Configured renderer executable | JS mode can produce no HTML if the renderer is unavailable or fails. |

The default metadata writer is in-memory; persistent service deployments must inject a writer backed by the intended storage layers. See [IMPLEMENTATION.md](./IMPLEMENTATION.md).

The `scraper_render_contract.h` timeout classifier and the diagnostic/run-summary types are standalone helpers. The plugin does not enforce renderer process timeouts through that helper or emit structured diagnostic events.

## Safe operating envelope

- Start with a small, approved seed list and narrowly scoped allowlist; increase source coverage only after checking output, discard decisions, write results, and source policy.
- Configure HTML search delay and HTML/API pagination limits for the source contract. `request_delay_ms` is not applied to API pagination, and API pagination uses the `ApiEndpointConfig` passed to the client—not the parsed `ScraperConfig::api_options`. Respect upstream terms, rate limits, and access controls; apply API throttling in the integration if required.
- Keep credentials in the environment variable referenced by a catalog source's `api_key_env`; the adapter reads it into an Authorization bearer header. Do not store credentials in source control or result metadata.
- Enable JS rendering only when an approved renderer executable is deployed and its subprocess behavior is understood.
- Retain the run statistics and the config/build revision with each job. The module does not export a built-in Prometheus endpoint or durable run history.

## Observability and recovery boundaries

The synchronous `scrape()` result is the module's built-in aggregate view. It includes activity and write counters but no per-source failure-rate metrics. `include/scraper/scraper_diagnostics.h` defines structured event/sink types, but the current `ScraperPlugin` implementation does not emit them. A deployment needing metrics or structured fault events must wire that instrumentation at its integration boundary.

There is no module-level source pause, cancellation, retry queue, or record-retraction API. Stop new runs in the caller, update the next-run configuration, and recover failed writes through the storage owner. The final `flush()` result is currently not surfaced in `ScraperRunStats`; verify persisted records independently.

## Configuration limitations to verify

The parser accepts `respect_robots`, `same_domain_only`, `max_depth`, and crawl `max_pages`, but the main `ScraperPlugin` path does not apply those fields. `api_options`, `search_options.results_per_page`, `llm_options.model_path`, and `llm_options.temperature` are also parsed but unused; the evaluator selects the default LLM plugin with a fixed temperature. `scraper_robots.*` and `scraper_sitemap.*` are independent helpers, not automatic crawl policies. Also, catalog seed URLs are not checked by the plugin's `UrlPolicy`; the exact API endpoint is exempted from result filtering. `js_timeout_ms` is passed to the renderer command but the subprocess adapter does not independently terminate a hung child. Until orchestration changes are verified, enforce source boundaries, timeouts, and rate controls in the caller/network boundary.

For incident symptoms and safe recovery steps see [RUNBOOK.md](./RUNBOOK.md). For trust boundaries and security risks see [TRUST.md](./TRUST.md).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
