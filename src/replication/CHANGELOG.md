> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Replication Module

All notable changes documented here. Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.7.0] — 2026-03-13
### Added
- LogicalReplicationManager with schema-aware logical slots, include/exclude filters, row predicates, DDL streaming, and data transformation hooks for cross-version replication.
- Conflict-free initial sync for logical subscribers via snapshot deduplication and persisted slot state under `<wal_directory>/logical_slots`.
- Focused logical replication tests (filters, DDL, transforms, initial sync) under the `LogicalReplicationTests` label.

## [1.5.0] — 2026-03-12
### Added
- Raft consensus with leader election and log replication
- CRDT types: FLAG_EW (Enable-Wins) and FLAG_DW (Disable-Wins)
- CoordinatedUpdateManager for cross-module atomic updates
- Read replicas with configurable consistency levels
- Snapshot-based replication for new node catch-up

## [1.0.0] — 2024-01-01
### Added
- Initial implementation
