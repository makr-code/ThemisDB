> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Scraper Module

All notable changes to the scraper module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added
- `include/scraper/scraper_diagnostics.h`: unified diagnostics layer (Phase 2+3) — ScraperDiagnosticEvent, IScraperDiagnosticSink, ListeningScraperDiagnosticSink, faultClassOf(), defaultSeverityOf(), makeDiagnosticEvent().
- `include/scraper/scraper_burst_controller.h`: BurstCrawlController token-bucket limiter for multi-source crawl burst hardening (Q4 2026).
- `include/scraper/scraper_render_contract.h`: enforceRenderTimeout() and isPaginationLimitReached() contract helpers (Q4 2026).
- `include/scraper/scraper_run_summary.h`: ScraperRunSummary and ScraperRunSummaryCollector for operator-facing triage (Q4 2026).
- Tests SCR-01..SCR-28: focused contract-hardening, Phase 2+3 diagnostics, burst, render/pagination, and run-summary test suites.
- Benchmarks GATE-SCR-01..06 and PIPE-01..04: release gates and pipeline-depth benchmarks.

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from extraction and processor benchmark suites.

## [2.1.x] - 2026

### Added
- scraper fetch/extraction/evaluation/write hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded source catalog, render modes, and metadata/provenance support surfaces.

## [1.x] - 2024-2025

### Added
- foundational scraper ingestion plugin infrastructure.