# Utils Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.5.0 – Comprehensive shared utilities library. Logging, audit trail, PII detection, text processing, compression, tracing, key derivation, encryption key management, serialization, and geospatial helpers are all production-ready. Phase 2 and Phase 3 features (streaming PII, sampled logger, HKDF TTL cache, SAGA compaction, tamper-evident hash-chain audit writer, Bloom filter, consistent hashing, rate limiter, timestamp utils) are now complete.

## Completed ✅
- [x] Logger – structured logging with ILogger interface
- [x] AuditLogger – tamper-evident audit trail generation
- [x] SAGALogger – SAGA transaction event logging
- [x] Cursor / pagination helpers
- [x] HKDF key derivation helper
- [x] LEKManager – Local Encryption Key management
- [x] Normalizer – text normalization
- [x] PII detector and pseudonymization
- [x] PKI client for certificate management
- [x] RetentionManager – data lifecycle helper
- [x] Serialization utilities
- [x] Stemmer – text stemming for search
- [x] Stop-word filtering
- [x] Tracing – distributed trace span management
- [x] ZSTDCodec – Zstd compression/decompression
- [x] Geospatial utilities
- [x] PII detection model upgrade to ML-based NER (replacing regex patterns) (Target: Q2 2026) (Issue: #2491)
- [x] LEK rotation automation without manual intervention (Target: Q3 2026) (Issue: #2346)
- [x] Streaming PII pipeline – `PIIStreamScanner` / `PIIStreamPseudonymizer` (chunked scan, lookahead buffer, deterministic HMAC pseudonyms)
- [x] SampledLogger – per-call-site token-bucket sampling decorator; suppression counter
- [x] HKDFCache TTL eviction – bounded LRU (1 000 entries), per-entry TTL (300 s), `OPENSSL_cleanse`, sharded mutex, `purge_by_ikm_hash()`
- [x] SAGALogCompactor / SAGALogReplayer – WAL compaction (atomic rename) and incomplete-transaction replay
- [x] BloomFilter – probabilistic membership with double-hashing, `std::shared_mutex`
- [x] ConsistentHashRing – FNV-1a 64-bit, virtual nodes, `getNodes(key, n)` for replication
- [x] RateLimiter – token-bucket; `try_acquire` / `acquire` / `set_rate`; `std::condition_variable`
- [x] TimestampUtils – ISO 8601 / RFC 3339 parse + format (ms, timezone offsets), `formatDuration`, Unix-ms helpers
- [x] HashChainAuditWriter – standalone tamper-evident audit writer (SHA-256 chain, persisted head, HKDF-seedable genesis)
- [x] AuditLogVerifier – standalone chain replay verifier; detects first tampered or missing link
- [x] **UUID v7** — `generate_uuid_v7()` in `include/utils/uuid.h` (Issue: #4582) (2026-04-12)
  - RFC 9562 compliant: 48-bit Unix-ms timestamp + 18-bit monotonic seq (thread_local, `std::mutex`, 0x3FFFFU mask) + 56-bit random (thread_local MT19937-64)
  - Monotonicity guaranteed within the same millisecond via `seq_state` bump
  - 20 focused tests (UV7-01…UV7-20) in `tests/test_uuid_v7.cpp`
- [x] **Streaming ZSTD** — `zstd_compress_stream` + `zstd_decompress_stream` in `zstd_codec.h/cpp` (Issue: #4583) (2026-04-12)
  - `ZSTD_CStream`/`ZSTD_DStream` with `Source: std::function<pair<const uint8_t*,size_t>()>` + `Sink: std::function<bool(...)>`
  - `max_output_bytes` DoS-guard (default = `MAX_DECOMPRESSED_SIZE` = 4 GB); raises error on overflow
  - 10 focused tests (ZS-01…ZS-10) in `tests/test_zstd_compression_security.cpp`

## In Progress 🚧
- [?] Structured log query API (search logs like data) (Target: Q2 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] LZ4 codec as faster alternative to Zstd for hot-path data (`include/utils/lz4_codec.h` + `src/utils/lz4_codec.cpp`)

### Long-term (6-12 months)
- [?] Multi-language stemmer support (German, French, Spanish)
- [?] Geospatial index helper (H3 / S2 cell encoding)
- [?] Log aggregation sink (ship to Elasticsearch / Loki)
- [?] Differential privacy utilities for analytics exports
- [?] Cryptographic utility consolidation (move scattered crypto helpers here)

## Implementation Phases

### Phase 1: Core Utility Library (Status: Completed ✅)
- [x] AuditLogger – tamper-evident audit trail with structured event records
- [x] HKDF helper – HMAC-based key derivation (RFC 5869)
- [x] LEKManager – Local Encryption Key lifecycle management
- [x] Logger / ILogger – structured logging with pluggable sinks
- [x] Normalizer – Unicode-aware text normalization
- [x] PII detector and pseudonymization (regex-based entity recognition)
- [x] PKI client – certificate request, retrieval, and renewal
- [x] RetentionManager – data lifecycle expiry helper
- [x] SAGALogger – SAGA transaction event log writer
- [x] Serialization utilities (MessagePack / JSON round-trip)
- [x] Stemmer – Porter stemmer for search index construction
- [x] Tracing – OpenTelemetry-compatible distributed span management
- [x] ZSTDCodec – Zstd compression / decompression wrapper
- [x] Cursor / pagination helpers for consistent result-set traversal
- [x] Geospatial utilities (bounding-box and distance helpers)

### Phase 2: Streaming PII & High-Throughput Logging (Status: Completed ✅)
- [x] Streaming PII pipeline for large documents (`PIIStreamScanner` – chunked scan, lookahead buffer, cross-boundary span merging)
- [x] Sampled logger for high-throughput paths (`SampledLogger` – per-call-site token buckets, per-level coin-flip, atomic suppression counter)
- [x] PII detection model upgrade to ML-based NER (replacing regex patterns) (Target: Q2 2026)
- [x] LEK rotation automation without manual intervention (Target: Q3 2026)

### Phase 3: Tamper-Evidence & Compaction (Status: Completed ✅)
- [x] Tamper-evident audit hash chain – `HashChainAuditWriter` (SHA-256 chain, persisted head) + `AuditLogVerifier::verify_chain()` (standalone chain replay verifier)
- [x] HKDF cache with TTL-based eviction – bounded LRU, `purge_by_ikm_hash()`, `OPENSSL_cleanse`, sharded mutex
- [x] SAGA log compaction – `SAGALogCompactor::compact()` archives completed sagas atomically; `SAGALogReplayer::replay_incomplete()` for disaster recovery
- [?] Geospatial utility hardening: H3 / S2 cell encoding, polygon containment
- [?] Cryptographic utility consolidation (centralise scattered crypto helpers in utils)

## Production Readiness Checklist
- [x] Unit tests coverage > 80%
  - Phase 3 test files: `test_hash_chain_audit.cpp`, `test_pii_stream_scanner.cpp`, `test_sampled_logger.cpp`, `test_saga_compactor.cpp`
  - Focused targets: `HashChainAuditFocusedTests`, `PIIStreamScannerFocusedTests`, `SampledLoggerFocusedTests`, `SAGACompactorFocusedTests`
  - Additional targets: `TimestampUtilsFocusedTests`, `UtilsRateLimiterFocusedTests`, `UtilsStandaloneFocusedTests`
- [x] Build-system wiring complete — duplicate source entries removed from `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`; all focused test targets registered in `tests/CMakeLists.txt`
- [x] Integration tests
  - Audit log integrity: `HashChainAuditFocusedTests`
  - PII redaction: `PIIStreamScannerFocusedTests`
  - SAGA replay: `SAGACompactorFocusedTests`
- [?] Performance benchmarks (compression throughput, stemmer latency)
- [?] Security audit (PII detector false-negative rate, LEK key material handling)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- PII detection uses a plugin-based engine architecture combining regex patterns (structured PII) and rule-based NER (person names, organizations, locations); ML-model-based NER with external frameworks remains an optional future enhancement.
- Geospatial utilities are basic helpers; complex spatial operations are in the index module.
- Stop-word lists are English-only by default; multi-language support is planned.

## Breaking Changes
- ILogger interface is stable from v1.x; new optional log levels are additive.
- ZSTD codec API is stable; compression level defaults may be tuned in v1.5.0.
- Tracing span API follows OpenTelemetry conventions; stable from v1.x.
