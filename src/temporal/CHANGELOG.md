<!-- Status: current | validated: 2026-03-12 -->
# Changelog — Temporal Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- Bitemporal data model: transaction time + valid time tracking
- `AS OF`, `FROM...TO`, `BETWEEN...AND` time-travel query syntax
- HLC-based conflict resolution for concurrent temporal writes
- Bitemporal joins across transaction-time and valid-time dimensions
- SEQUENCED and NON-SEQUENCED update semantics
- Temporal aggregations (min/max/avg over time intervals)
- Automated retention policies with configurable age-based eviction

## [1.0.0] — 2024-01-01
### Added
- Transaction-time tracking for all storage mutations
- Basic time-travel queries
