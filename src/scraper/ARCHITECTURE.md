# Architecture - Scraper Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The scraper module composes seed-source expansion, multi-mode fetch/render, crawl/search extraction, evaluation, and metadata/provenance writing into a bounded ingestion-oriented scraping subsystem.

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
| seed/fetch contract | deterministic source expansion and bounded fetch behavior |
| extraction contract | explicit form/result extraction semantics |
| evaluation contract | explicit relevance/quality outcomes with fallback behavior |
| write contract | deterministic metadata/provenance construction and write signaling |

## Failure Semantics

- fetch failures are explicit and non-silent.
- parse/evaluation errors surface deterministic outcomes.
- write failures are observable and accounted in run stats.
- unsupported modes/inputs fail explicitly under config constraints.

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
  - explicit failure boundaries for fetch/parse/evaluate/write paths
  - module-local ownership of scraper ingestion behavior

## Diagnostics Layer

The diagnostics layer was introduced in Q3 2026 to provide unified, non-silent fault reporting across all scraper pipeline stages.

### Components

| Component | File | Responsibility |
|---|---|---|
| ScraperDiagnosticEvent | include/scraper/scraper_diagnostics.h | Structured per-fault event with mandatory message, source URL, timestamp |
| IScraperDiagnosticSink | include/scraper/scraper_diagnostics.h | Thread-safe emit interface for operator sinks |
| ListeningScraperDiagnosticSink | include/scraper/scraper_diagnostics.h | Recording sink with listener callbacks for tests and metrics |
| faultClassOf() / defaultSeverityOf() | include/scraper/scraper_diagnostics.h | Error-to-fault-class routing helpers |
| BurstCrawlController | include/scraper/scraper_burst_controller.h | Token-bucket burst limiter for multi-source crawl bursts |
| ScraperRunSummary / ScraperRunSummaryCollector | include/scraper/scraper_run_summary.h | Operator-facing aggregate triage view of a completed scrape run |

### Fault Classes

All scraper errors are routed to a `ScraperFaultClass` via `faultClassOf()`:
- `kFetchPath` — HTTP fetch / DNS / TLS failures
- `kRenderPath` — JS renderer timeout / crash
- `kParsePath` — HTML/DOM parse errors
- `kEvaluatorPath` — LLM evaluator failures
- `kWritePath` — Metadata / relational / graph / vector write failures
- `kCrawlControl` — Pagination limits, burst throttle, source catalog issues
- `kInternal` — Unclassified errors