# ThemisDB Scraper Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The scraper module provides ThemisDB's scraping ingestion pipeline for seeded web/API discovery, JS/static retrieval, search/form crawling, LLM-assisted relevance/quality evaluation, and metadata/provenance writing.

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

- scrape pipeline behavior is config-bounded and explicit.
- fetch/search/evaluate/write outcomes are deterministic per configured modes.
- LLM-evaluation fallback behavior remains explicit and observable.
- provenance fields are enforced by module write paths.

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