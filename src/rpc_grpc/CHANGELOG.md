> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - RPC gRPC Module

All notable changes to the rpc_grpc module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- TLS reload now drains and restarts the active listener with prevalidated
  credentials, attempts rollback on replacement-start failure, and reports
  failure if rollback cannot restore service. The operation has downtime.
- Runtime send/receive limits now use shared 100 MiB API-contract constants;
  documentation no longer claims a service-count cap that is not implemented.
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from bench_wal_apply_grpc coverage.

## [2.1.x] - 2026

### Added
- gRPC plugin lifecycle, credential handling, and observability hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded RPC transport plugin and stream support surfaces.

## [1.x] - 2024-2025

### Added
- foundational gRPC plugin integration infrastructure.