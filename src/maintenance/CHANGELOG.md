> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Maintenance Module

All notable changes to the maintenance module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from scheduler, distributed coordination, index-rebuild, and TPCC proxy suites.

## [2.0.0] - 2026-04-13

### Added
- distributed lock integration and multi-tenant schedule isolation behavior.

## [1.2.0] - 2026-04-12

### Added
- explicit task dependency handling and handler registry integration.

### Changed
- scheduling and execution internals hardened for dependency and concurrency scenarios.

## [1.1.0] - 2026-04-10

### Added
- persistent schedule storage and force-run behavior.

## [1.0.0] - 2026-03-11

### Added
- baseline maintenance orchestrator, schedule lifecycle, and API integration.