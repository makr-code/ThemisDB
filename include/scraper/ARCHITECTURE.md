> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/scraper/ARCHITECTURE.md -->

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

# Scraper Module — Public Header Architecture

**Module Path:** `include/scraper/`  
**Implementation:** `../../src/scraper/`  
**Canonical architecture doc:** [`../../src/scraper/ARCHITECTURE.md`](../../src/scraper/ARCHITECTURE.md)

---

## 1. Overview

`include/scraper/` defines the **public web scraping, LLM-assisted content evaluation, JavaScript rendering, metadata writing, and government source cataloguing API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/scraper/ARCHITECTURE.md`](../../src/scraper/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Scraper Core

| Header | Public Type | Purpose |
|--------|------------|---------|
| `scraper_config.h` | `ScraperConfig` | Scraper configuration and rate-limit settings |
| `scraper_api_client.h` | `IScraperApiClient`, `HttpScraperApiClient` | JSON REST/GraphQL client interface and HTTP implementation |
| `scraper_js_renderer.h` | `IScraperJSRenderer`, `SubprocessJSRenderer` | Optional external JS renderer interface and adapter |
| `scraper_search_engine.h` | `IScraperSearchEngine`, `HtmlSearchEngine` | HTML form discovery and result parsing |
### 2.2 Content Quality and Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `scraper_llm_evaluator.h` | `IScraperLLMEvaluator`, `ScraperLLMEvaluator` | LLM-or-heuristic content evaluation |
| `scraper_metadata_writer.h` | `IScraperMetadataWriter`, `InMemoryScraperMetadataWriter`, `ScraperRecordBuilder` | Writer contract and record construction |
| `scraper_plugin.h` | `IScraperPlugin`, `ScraperPlugin` | Synchronous plugin lifecycle and orchestration |
| `gov_source_catalog.h` | `GovSourceCatalog`, `GovDataSource` | Built-in source catalog and overlays |

---

## 3. Namespace Layout

All public types reside in the `themis::scraper` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/scraper/` declare the public C++ surface; compatibility and change policy are governed by the project versioning rules, not by this page's presence.
- `ScraperPlugin` composes seed/catalog selection, fetch/render, optional HTML search or API retrieval, evaluation, and writer calls. Public helper headers for diagnostics, run summaries, renderer contracts, robots, and sitemaps are not automatically wired into that orchestration.
- `scraper_diagnostics.h` provides event/sink types, but the plugin does not emit events. `scraper_render_contract.h` helpers likewise are not called by the plugin.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
