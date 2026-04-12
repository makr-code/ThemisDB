<!-- Status: current | validated: 2026-04-06 -->

# Utils — Roadmap

## Current Status
**v1.5.0** — Stable utility layer. PII pipeline, SIMD distance, lossless vector compression, and HKDF helpers are production-ready.

## Completed
- [x] PII detection/pseudonymization pipeline
- [x] SIMD-accelerated L2 and cosine distance (`simd_distance.h`)
- [x] Lossless vector compression headers
- [x] HKDF-SHA256 key derivation helpers
- [x] Safe arithmetic / cast / access utilities
- [x] Thread safety primitives and thread-pool manager
- [x] OpenTelemetry-compatible tracing (`tracing.h`)
- [x] ZSTD codec interface
- [x] Consistent hash ring
- [x] Bloom filter
- [x] Concurrent LRU cache

## Planned Features

- [ ] GPU-accelerated SIMD distance fallback path in `simd_distance.h` (Target: Q3 2026)
- [ ] Streaming PII redaction in `pii_redacting_sink.h` (chunked input) (Target: Q2 2026)
- [ ] `safe_arithmetic.h` — 128-bit integer support (Target: Q3 2026)
- [ ] `zstd_codec.h` — streaming compress/decompress API (Target: Q2 2026)
- [x] `zstd_codec.h` — streaming API: `ZstdStreamCompressor` + `ZstdStreamDecompressor` added
- [ ] `rate_limiter.h` — distributed (Redis-backed) rate limiting (Target: Q3 2026)
- [ ] `bloom_filter.h` — scalable Bloom filter (SBF) variant (Target: Q4 2026)
- [ ] `retention_manager.h` — GDPR right-to-erasure integration (Target: Q2 2026)
- [ ] Formal ReDoS audit of `regex_detection_engine.h` patterns (Target: Q2 2026)
- [ ] `uuid.h` — UUID v7 (time-ordered) support (Target: Q3 2026)
- [x] `uuid.h` — UUID v7 added: `generate_uuid_v7()` (RFC 9562, time-ordered)
- [x] LZ4 codec — `lz4_codec.h` + `lz4_codec.cpp` added (safe + legacy API)

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define `IEncryptionBackend` interface analogues for utils
- [x] Standardize `Expected<T,E>` result type across all utility APIs
- [x] Define streaming API contract for `zstd_codec.h`

### Phase 2 — Core Implementation
- [x] Streaming ZSTD compress/decompress (`ZstdStreamCompressor` + `ZstdStreamDecompressor`)
- [ ] Distributed rate limiter backend
- [x] UUID v7 implementation (`generate_uuid_v7()`, RFC 9562)

### Phase 3 — Error Handling & Edge Cases
- [ ] `zstd_codec.h`: enforce max decompression output size
- [ ] `pii_detector.h`: ReDoS-safe regex audit and replacements
- [ ] `rate_limiter.h`: handle backend (Redis) unavailability gracefully

### Phase 4 — Tests
- [ ] Property-based tests for `safe_arithmetic.h` overflow boundaries
- [ ] Fuzz testing for `pii_detector.h` regex patterns
- [ ] Load tests for `thread_pool_manager.h` under saturation

### Phase 5 — Performance / Hardening
- [ ] SIMD distance GPU path benchmarks
- [ ] HKDF cache eviction policy review
- [ ] Memory zeroization audit for `lek_manager.h`

### Phase 6 — Documentation & Sign-off
- [ ] Complete API documentation for all ~60 headers
- [ ] Security review of PII pipeline
- [ ] Update CHANGELOG with v1.6.0 entries

## Production Readiness Checklist
- [x] PII pipeline tested and in production
- [x] SIMD distance benchmarked
- [ ] Streaming ZSTD API released
- [ ] Distributed rate limiter released
- [ ] ReDoS audit completed for regex detection engine
- [ ] All headers have complete Doxygen documentation
