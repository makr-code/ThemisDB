> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/scraper/ARCHITECTURE.md -->

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
| `scraper_api_client.h` | `ScraperAPIClient` | External scraping API client adapter |
| `scraper_js_renderer.h` | `ScraperJSRenderer` | Headless JS rendering for SPAs |
| `scraper_search_engine.h` | `ScraperSearchEngine` | Search-engine-based scrape orchestration |
### 2.2 Content Quality and Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `scraper_llm_evaluator.h` | `ScraperLLMEvaluator` | LLM-based scraped content quality judge |
| `scraper_metadata_writer.h` | `ScraperMetadataWriter` | Scraped content metadata persistence |
| `scraper_plugin.h` | `IScraperPlugin` | Pluggable scraper backend interface |
| `gov_source_catalog.h` | `GovSourceCatalog` | Government and open-data source catalogue |

---

## 3. Namespace Layout

All public types reside in the `themis::scraper` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/scraper/` expose the **stable public API**; internal types live in `src/scraper/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN Frontdoor**.
