> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Updates Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- Blue/green deployment support with traffic switchover (PR #3421)
- CoordinatedUpdateManager for cross-module atomic updates (PR #3422)
- Update history log with rollback support (PR #3420)
- HSM-backed SigningService for update package verification (PR #3438)
- Binary delta updates to minimize download size
- Canary rollout with configurable traffic percentage
- Dry-run migration preview before applying schema changes
- Pre-flight health checks before update execution
- Multi-node coordinated updates with quorum acknowledgement
- Notification webhooks for update lifecycle events
- Build system audit: all 21 source files registered; 10 focused test targets

## [1.0.0] — 2024-01-01
### Added
- `HotReloadEngine` for zero-downtime module updates
- Release manifest management and verification
- Schema migration framework with rollback
- Digital signature verification for update packages
- Automatic backup before applying updates
