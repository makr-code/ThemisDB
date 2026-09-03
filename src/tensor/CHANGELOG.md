> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Tensor Module

All notable changes to the tensor module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from tensor fingerprint graph, tensor fingerprint, and tensor dedup benchmark suites.
- Hardened `TensorCoreStorageBridge` fail-closed behavior: backend `put()` exceptions are now converted to `ERR_STORAGE_TRANSACTION_FAILED` results, and `getRaw()` now returns `nullopt` when backend `get()` throws.

## [2.1.x] - 2026

### Added
- tensor index/bridge/graph hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded tensor bridge, fingerprint graph, and structural helper surfaces.

## [1.x] - 2024-2025

### Added
- foundational tensor index and bridge infrastructure.