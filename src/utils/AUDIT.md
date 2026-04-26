> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-04-19 -->
# Audit Report — Utils Module
**Last Audit:** 2026-04-19 | **Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Test Coverage | ✅ Present |
| Source Files | 44 (`.cpp` in `src/utils/`) |
| Open TODOs | Low |

## Source Files Audited

- `audit_logger.cpp` — Structured audit trail with hash-chain tamper detection
- `bloom_filter.cpp` — Probabilistic membership testing
- `boost_throw_exception.cpp` — Boost exception integration shim
- `build_info.cpp` — Build metadata and version information
- `capability_auto_generator.cpp` — Auto-generates capability manifests for modules
- `checksum_utils.cpp` — CRC32, Adler32, and FNV checksum utilities
- `compression_metrics.cpp` — Metrics for compression ratio and throughput
- `consistent_hash.cpp` — Consistent hashing ring for shard routing
- `cron_parser.cpp` — Cron expression parser and next-fire-time calculator
- `cursor.cpp` — Cursor management for paginated result sets
- `error_registry.cpp` — Centralised error code registry
- `file_utils.cpp` — File I/O utilities with path sanitization
- `grpc_channel_pool.cpp` — gRPC channel pool with health checking
- `hkdf_cache.cpp` — HKDF TTL cache with key rotation
- `hkdf_helper.cpp` — HKDF key derivation helper functions
- `http_client_pool.cpp` — HTTP client connection pool
- `input_validator.cpp` — Input validation and sanitization utilities
- `lek_manager.cpp` — Local encryption key manager
- `logger.cpp` — Structured levelled logger
- `lz4_codec.cpp` — LZ4 compression/decompression codec
- `ner_detection_engine.cpp` — Named entity recognition detection engine
- `normalizer.cpp` — Unicode and text normalization utilities
- `pii_detection_engine.cpp` — PII pattern detection engine (regex + ML)
- `pii_detector.cpp` — PII entity detection (streaming + static)
- `pii_pseudonymizer.cpp` — PII pseudonymization with reversible tokens
- `pii_stream_scanner.cpp` — Streaming PII scanner for large payloads
- `pki_client.cpp` — PKI certificate management client
- `rate_limiter.cpp` — Token bucket rate limiter
- `regex_detection_engine.cpp` — Regex-based content detection engine
- `retention_manager.cpp` — Data retention policy enforcement
- `runtime_license_gate.cpp` — Runtime license feature gate enforcement
- `saga_logger.cpp` — Distributed saga transaction logger
- `sampled_logger.cpp` — Sampling-based high-throughput logger
- `self_awareness.cpp` — Module self-awareness and health reporting
- `serialization.cpp` — Binary and JSON serialization utilities
- `simd_distance.cpp` — SIMD-accelerated vector distance computations
- `stemmer.cpp` — Word stemming for search indexing
- `stopwords.cpp` — Stopword list management for text processing
- `thread_pool_manager.cpp` — Thread pool lifecycle and work-stealing scheduler
- `timestamp_utils.cpp` — UTC/ISO 8601/HLC format conversion
- `tracing.cpp` — Distributed tracing integration utilities
- `update_checker.cpp` — Module update availability checker
- `utils_adapters.cpp` — Adapter utilities for cross-module integration
- `zstd_codec.cpp` — Zstandard compression/decompression codec

## Findings
### Resolved
- Tamper-evident hash-chain audit writer implemented (v1.5.0)
- All Phase 2 and Phase 3 utilities complete
### Open
- None critical

## Compliance
- GDPR: PII detector and audit logger support compliance with Articles 30 and 32
- SOC 2: Tamper-evident audit trail satisfies audit logging requirements
