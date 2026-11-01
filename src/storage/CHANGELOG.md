---
Author: "ThemisDB Contributors"
Created: "2026-07-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "release-versioning"
Canonical_References:
  - "../../CHANGELOG.md"
  - "README.md"
  - "AUDIT.md"
Related_Issue: "#7383"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | updated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Storage Module

All notable changes to the storage module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Audited scoped storage Markdown against current headers, implementations, tests, and CMake registration; replaced unsupported end-to-end durability/performance assertions with component boundaries and explicit evidence limits (#7383).
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from storage-performance and user-storage-mount benchmark suites.
- `SecuritySignatureManager` now fails closed when constructed without RocksDB unless the caller explicitly opts into the in-memory fallback for tests/ephemeral workflows.
- `BackupManager` now uses real manifest-driven S3/GCS/Azure transport for cloud backup upload/restore instead of the previous local-mirror-only remote stub path.

## [2.1.x] - 2026

### Added
- storage durability, recovery, and maintenance hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded storage orchestration, blob/tiering, and operational observability surfaces.

## [1.x] - 2024-2025

### Added
- foundational persistence, MVCC/WAL, and backup-recovery infrastructure.