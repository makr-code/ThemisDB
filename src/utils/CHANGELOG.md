> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Utils Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Utils catch-all hardening (Phase 24, 2026-05-19): replaced all remaining 32 `catch(...)`
  handlers with typed `catch (const std::exception&)` across 13 files; zero catch-all handlers
  remain in `src/utils/*.cpp`.

## [1.5.0] — 2026-03-12
### Added
- Streaming PII detector with configurable entity types
- Sampled structured logger (probabilistic sampling for high-throughput paths)
- HKDF TTL cache with automatic key rotation
- SAGA compaction utility for pruning completed saga logs
- Tamper-evident hash-chain audit writer (append-only ledger)
- Bloom filter for probabilistic membership queries
- Consistent hashing utility (shared with sharding module)
- Rate limiter utility (token bucket, used by multiple modules)
- Timestamp utilities (UTC, ISO 8601, HLC format converters)

## [1.0.0] — 2024-01-01
### Added
- `AuditLogger` — structured audit trail with JSON output
- `ErrorRegistry` — centralised error code registry
- PII detection (static regex patterns)
- Text processing utilities (tokenization, normalization)
- Compression helpers (zstd, lz4 wrappers)
- Distributed tracing utilities (span/trace context)
- HKDF key derivation
- Encryption key management utilities
- Serialization helpers (JSON, binary)
- Geospatial coordinate helpers
