# Utils Module Roadmap

## Current Status
v1.x – Comprehensive shared utilities library. Logging, audit trail, PII detection, text processing, compression, tracing, key derivation, encryption key management, serialization, and geospatial helpers are all production-ready.

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

## In Progress 🚧
- [ ] PII detection model upgrade to ML-based NER (replacing regex patterns) (Target: Q2 2026)
- [ ] Structured log query API (search logs like data) (Target: Q2 2026)
- [ ] LEK rotation automation without manual intervention (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] LZ4 codec as faster alternative to Zstd for hot-path data
- [ ] Consistent hashing utility for distributed key routing
- [ ] Bloom filter utility for probabilistic membership checks
- [ ] Rate limiter utility (token bucket, shared across modules)
- [ ] ISO 8601 / RFC 3339 timestamp parsing and formatting helpers

### Long-term (6-12 months)
- [ ] Multi-language stemmer support (German, French, Spanish)
- [ ] Geospatial index helper (H3 / S2 cell encoding)
- [ ] Log aggregation sink (ship to Elasticsearch / Loki)
- [ ] Differential privacy utilities for analytics exports
- [ ] Cryptographic utility consolidation (move scattered crypto helpers here)

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (audit log integrity, PII redaction correctness, ZSTD round-trip)
- [ ] Performance benchmarks (compression throughput, stemmer latency)
- [ ] Security audit (PII detector false-negative rate, LEK key material handling)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- PII detection relies on regex patterns; false-negative rate is higher than ML-based approaches.
- Geospatial utilities are basic helpers; complex spatial operations are in the index module.
- Stop-word lists are English-only by default; multi-language support is planned.

## Breaking Changes
- ILogger interface is stable from v1.x; new optional log levels are additive.
- ZSTD codec API is stable; compression level defaults may be tuned in v1.5.0.
- Tracing span API follows OpenTelemetry conventions; stable from v1.x.
