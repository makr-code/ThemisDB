# Security - Scraper Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the scraper module focuses on safe URL/policy-constrained fetching, deterministic parser/evaluator behavior, explicit fallback transparency, and provenance-safe write enforcement.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe/unapproved source traversal | URL policy and bounded crawl/fetch controls |
| malformed content leading to silent parser failure | explicit parse outcome handling |
| opaque LLM scoring behavior drift | explicit fallback and run-stat visibility |
| provenance tampering in ingestion records | mandatory provenance field construction in writer paths |

## Implemented Security Controls

- policy-constrained crawling/fetching behavior is validation-gated.
- parser/evaluator failures are explicit and non-silent.
- fallback behavior remains explicit for evaluation paths.
- metadata writer enforces provenance field construction.

## Security Follow-ups

- expand malformed input and hostile content edge-case regressions.
- tighten diagnostics taxonomy for fetch/parse/evaluate incident classes.
- deepen stress coverage for sustained multi-source crawl workloads.

## Sourcecode Verification (Module: scraper/security)

- Verified files:
  - src/scraper/scraper_config.cpp
  - src/scraper/scraper_api_client.cpp
  - src/scraper/scraper_search_engine.cpp
  - src/scraper/scraper_llm_evaluator.cpp
  - src/scraper/scraper_metadata_writer.cpp
- Verified controls:
  - bounded URL/fetch policy behavior
  - deterministic parse/evaluate fallback signaling
  - provenance-safe write-path enforcement