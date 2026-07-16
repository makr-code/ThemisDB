> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Server Module

All notable changes to the server module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, CHANGELOG, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable server behavior.
- Performance expectations updated from mixed network/security proxy mapping to explicit benchmark symbols from bench_api_endpoints.cpp and bench_stream_protocol.cpp.

## [1.9.x] - 2026

### Added
- broader server runtime hardening across request lifecycle, protocol sessions, and reliability paths.

## [1.5.x] - 2026

### Added
- HTTP/3, MCP, extended rate-limiting, and expanded server runtime surfaces.

## [1.0.x] - 2024-2025

### Added
- foundational HTTP server, middleware, and API endpoint runtime infrastructure.