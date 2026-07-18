> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-07-18 -->
<!-- Agentic status sync: module issue #5618 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · AUDIT.md -->

# Changelog - API Module

All notable changes to the API module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit existing benchmark symbols from current API benchmark source.
- Agentic module status review refreshed all `src/api/*.md` docs to the 2026-07-18 validation pass and added `graphql_aql_resolver.cpp` to source-verification sets where applicable.

## [2.0.0] - 2026-04

### Fixed
- GraphQL variable substitution behavior hardened in module execution path.

## [1.9.1] - 2026-04-07

### Fixed
- API and gRPC hardening fixes for validation and reporting paths.

## [1.9.0] - 2026-03-25

### Added
- gRPC wiring and GraphQL WebSocket stability/hardening additions.

## [1.7.0] - 2026-03-09

### Added
- GraphQL WebSocket transport, gRPC surface, and async/tenant/API-gateway related additions.

## [1.6.0] - 2026-02-01

### Added
- GraphQL layer, streaming endpoints, tracing middleware, and OTLP exporter additions.

## [1.0.0] - 2024-01-01

### Added
- initial API HTTP/TLS/auth integration foundations.