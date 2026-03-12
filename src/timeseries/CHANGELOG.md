<!-- Status: current | validated: 2026-03-12 -->
# Changelog — Timeseries Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- Gorilla compression algorithm for floating-point time series values
- Continuous aggregation with configurable windows (tumbling, sliding, session)
- Retention policies with automatic TTL-based eviction
- Auto-batching write buffer for high-frequency single-point inserts
- Downsampling pipelines (LTTB, min/max/avg)
- Gap-fill interpolation for sparse time series

## [1.0.0] — 2024-01-01
### Added
- Time-ordered key-value storage with nanosecond precision timestamps
- Range queries by time interval
- Basic aggregation (count, sum, avg, min, max)
