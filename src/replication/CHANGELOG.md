<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Replication Module

All notable changes documented here. Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

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
