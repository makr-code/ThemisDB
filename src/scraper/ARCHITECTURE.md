# Architecture - Scraper Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

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

## Overview

The scraper module composes seed-source selection, fetch/render, optional HTML search or API retrieval, evaluation, and metadata/provenance writing into a synchronous ingestion subsystem. Limits vary by path; several parsed crawl controls are not enforced by the main loop.

## Main Execution Planes

1. Seed and fetch plane
- seed collection from config/catalog
- static/JS/API fetch and pagination behavior

2. Parse and evaluation plane
- form/result extraction behavior
- quality/relevance scoring and fallback behavior

3. Persistence and provenance plane
- record building for downstream stores
- provenance-safe write behavior and stats visibility

## Core Contracts

| Contract | Behavior |
|---|---|
| seed/fetch contract | explicit seed/catalog selection and mode-dependent fetch behavior |
| extraction contract | explicit form/result extraction semantics |
| evaluation contract | explicit relevance/quality outcomes with fallback behavior |
| write contract | metadata/provenance construction and write signaling |

## Failure Semantics

- fetch failures can result in empty content and are not uniformly surfaced in logs or run statistics.
- search/parser empty outcomes are not necessarily distinguishable from unsupported or unmatched page structure.
- evaluation outcomes are returned; accepted documents are passed to the writer.
- individual writer failures increment `write_errors`; `flush()` failure is not represented in returned stats.

## Sourcecode Verification (Module: scraper/architecture)

- Verified files:
  - src/scraper/scraper_plugin.cpp
  - src/scraper/scraper_api_client.cpp
  - src/scraper/scraper_search_engine.cpp
  - src/scraper/scraper_llm_evaluator.cpp
  - src/scraper/scraper_metadata_writer.cpp
  - src/scraper/gov_source_catalog.cpp
- Verified architecture claims:
  - seed/fetch + parse/evaluation + persistence/provenance plane split
  - path-specific fetch/parse/evaluate/write outcomes; failures are not uniformly surfaced as diagnostics
  - module-local ownership of scraper ingestion behavior

## Diagnostics Primitives (not wired into the plugin)

The public headers provide diagnostic event/sink types, fault-classification helpers, and a run-summary collector. These are standalone integration primitives: `ScraperPlugin` has no diagnostic-sink dependency and does not emit these events. Do not interpret the declarations or associated tests as end-to-end plugin instrumentation.

`scraper_robots.h` and `scraper_sitemap.h` also declare standalone helpers. Their implementations are omitted from the `scraper_plugin` target's CMake source list and the focused scraper test registration; the normal archive does not provide their symbols. See [IMPLEMENTATION.md](./IMPLEMENTATION.md) for the build/test TODO.

### Components

| Component | File | Responsibility |
|---|---|---|
| ScraperDiagnosticEvent | include/scraper/scraper_diagnostics.h | Structured per-fault event with mandatory message, source URL, timestamp |
| IScraperDiagnosticSink | include/scraper/scraper_diagnostics.h | Thread-safe emit interface for operator sinks |
| ListeningScraperDiagnosticSink | include/scraper/scraper_diagnostics.h | Recording sink with listener callbacks for tests and metrics |
| faultClassOf() / defaultSeverityOf() | include/scraper/scraper_diagnostics.h | Error-to-fault-class routing helpers |
| BurstCrawlController | include/scraper/scraper_burst_controller.h | Optional token-bucket gate applied once per seed |
| ScraperRunSummary / ScraperRunSummaryCollector | include/scraper/scraper_run_summary.h | Aggregate counters for events explicitly sent to a listening sink; callers must provide event and run-stat inputs |

### Fault Classes

When an integration calls `faultClassOf()`, its `ScraperError` is mapped to a `ScraperFaultClass`:
- `kFetchPath` — HTTP fetch / DNS / TLS failures
- `kRenderPath` — JS renderer timeout / crash
- `kParsePath` — HTML/DOM parse errors
- `kEvaluatorPath` — LLM evaluator failures
- `kWritePath` — Metadata / relational / graph / vector write failures
- `kCrawlControl` — Pagination limits, burst throttle, source catalog issues
- `kInternal` — Unclassified errors
---

### Declared integration surface (not a wired downstream consumer)

`include/server/scraper_plugin_api_handler.h` declares an HTTP handler and labels its integration to `HttpServer` and plugin loading as pending. The scraper module's own `CMakeLists.txt` builds `scraper_plugin`; it does not define or consume `THEMIS_PLUGIN_SCRAPER`. The handler's actual target registration, route wiring, and runtime behavior are not established by the scraper module sources reviewed here.

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/server/scraper_plugin_api_handler.h` → `ScraperPluginApiHandler` declaration | Header documents proposed routes; `HttpServer` wiring is marked pending. Do not treat these as available production endpoints. |
| `plugin_host` | TODO | No scraper-module CMake registration or verified plugin-host load path is established by the sources referenced above. |
| _(tests)_ | Public `include/scraper/` headers | `tests/scraper/` contains focused test consumers; the separately declared server handler has no verified runtime wiring in this review. |
