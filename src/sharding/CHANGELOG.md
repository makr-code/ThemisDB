<!-- Status: current | validated: 2026-03-12 -->
# Changelog — Sharding Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- Raft-based consensus for shard coordination and leader election
- Shard repair engine with automatic rebalancing
- Cross-shard query routing with scatter-gather execution
- Consistent hashing ring with virtual nodes
- Shard split and merge operations with zero-downtime
- Per-shard metrics and health monitoring via MetricsCollector

## [1.0.0] — 2024-01-01
### Added
- Horizontal sharding with range and hash partitioning
- Shard metadata registry
- Cross-shard transaction coordination
