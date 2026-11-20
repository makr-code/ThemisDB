---
Author: "ThemisDB Contributors"
Created: "2026-07-18"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Sharding Module

All notable changes to the sharding module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- DIS-06 / #8200: CrossShard's 2PC path delegates durable decisions and replay to
  `TwoPhaseCommitCoordinator`; non-2PC protocols and both public APIs remain.
  Configure persistence and trusted participant endpoint resolution before rollout.
- Coordinator and participant writes now require WAL configuration; BEGIN,
  decisions, votes and terminal acknowledgements enforce synchronous durability.
- Participant timeouts retain prepared locks/in-doubt state rather than selecting
  local ABORT. Recovery/readiness exposes unresolved decisions; terminal replay
  rejects contradictory decisions and preserves retries older than the WAL cache.
- Separate WAL domains/formats are unchanged. No automatic migration of unresolved
  older CrossShard records; production transport/replica/platform evidence and
  maintainer acceptance remain open (see ROADMAP.md and RUNBOOK.md).
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from sharding, shard-routing, and shard-resource benchmark suites.

## [2.1.x] - 2026

### Added
- sharding routing/transaction/repair/rebalance hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded distributed coordination, migration, and operational observability surfaces.

## [1.x] - 2024-2025

### Added
- foundational sharding and distributed partitioning infrastructure.