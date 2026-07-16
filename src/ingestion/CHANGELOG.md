> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Ingestion Module

All notable changes to the ingestion module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified ingestion benchmark symbols from ingestion throughput, quality-judge, extraction, and timeseries-ingestion suites.

## [1.5.1] - 2026-03-21

### Added
- focused test and CI coverage for ingestion LLM adapter pathways.

## [1.5.0] - 2026-03-12

### Added
- legal extraction, lineage tracking, and expanded ingestion observability/admin surfaces.

### Changed
- connector-scoped rate limiting and checkpoint/retry tuning behaviors.

### Fixed
- connector stability issues in CDC, crawler, and object-storage paths.

## [1.4.0] - 2025-09-01

### Added
- distributed coordinator and expanded connector surface area.

### Changed
- connector backend upgrades and migration behavior.

### Fixed
- ingestion connector robustness issues in crawler/API/filesystem scenarios.

## [1.3.0] - 2025-03-01

### Added
- Kafka ingestion, quarantine back-off, and parser support expansion.

### Fixed
- dataset connector token-expiry handling issue.

## [1.2.0] - 2024-09-01

### Added
- HuggingFace and generic API connector support.

## [1.1.0] - 2024-04-01

### Added
- filesystem ingestion, rate limiting, and checkpoint foundations.

## [1.0.0] - 2024-01-01

### Added
- foundational ingestion manager/coordinator scaffolding and basic connectors.