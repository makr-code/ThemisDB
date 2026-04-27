> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Utils Module - Future Enhancements

This document covers planned enhancements to ThemisDB's shared utilities subsystem, which provides foundational cross-cutting infrastructure consumed by all other modules: audit logging (`audit_logger.cpp`), cursor/pagination (`cursor.cpp`), HKDF key derivation (`hkdf_helper.cpp`, `hkdf_cache.cpp`), LEK (Local Encryption Key) management (`lek_manager.cpp`), structured logging (`logger.cpp`), data normalisation (`normalizer.cpp`), PII detection and pseudonymisation (`pii_detection_engine.cpp`, `pii_detector.cpp`, `pii_pseudonymizer.cpp`), PKI client (`pki_client.cpp`), retention manager (`retention_manager.cpp`), SAGA logging (`saga_logger.cpp`), serialisation (`serialization.cpp`), text processing (`stemmer.cpp`, `stopwords.cpp`), distributed tracing (`tracing.cpp`), ZSTD codec (`zstd_codec.cpp`), and geospatial utilities (`geo/`). The module is Beta-stage and is the primary dependency of all other ThemisDB modules.

## Design Constraints

- All utilities must be header-safe for inclusion in multiple translation units; shared state must be explicitly managed through thread-safe singleton or injected instances.
- The PII detection and pseudonymisation pipeline must operate in a streaming fashion to handle arbitrarily large documents without loading them fully into memory.
- Key derivation via `hkdf_helper.cpp` must use HKDF-SHA-256 per RFC 5869; raw symmetric keys must never appear in log output, audit records, or error messages.
- Utilities must have zero mandatory external network dependencies at runtime; `pki_client.cpp` and `tracing.cpp` degrade gracefully when their upstream services are unreachable.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AuditLogger::log(event)` | All modules requiring compliance audit trail | Structured JSON; append-only; tamper-evident |
| `PIIDetectionEngine::scan(text)` | `prompt_engineering`, `training`, ingestion | Returns span list with entity type and confidence |
| `PIIPseudonymizer::pseudonymize(text, policy)` | Same as PII detection consumers | Deterministic per tenant; reversible under admin key |
| `HKDFHelper::derive_key(ikm, info, length)` | `timeseries`, `sharding`, `security` | Must not cache raw IKM beyond a single derive call |
| `LEKManager::get_current_dek(series_id)` | `timeseries`, storage module | Returns AES-256 DEK; rotates on schedule |
| `ZSTDCodec::compress() / decompress()` | `sharding`, `timeseries`, exporters | Streaming API; supports preset levels 1–22 |
| `SAGALogger::log_step(txn_id, step, state)` | `sharding`, transaction module, training | Write-ahead log for SAGA compensation recovery |
| `Tracing::start_span(name, parent)` | All modules | OpenTelemetry-compatible; no-op when collector unreachable |

## Planned Features

### `CapabilityAutoGenerator`: Persist Schedule and Document Count State
**Priority:** Medium
**Target Version:** v1.8.0

`capability_auto_generator.cpp` has 3 TODOs:
- Line 193: "Check last update time and compare with schedule interval" — schedule checking is a no-op; updates run every invocation regardless of interval.
- Line 373: "Store previous document count somewhere" — document count delta cannot be computed without persistence.
- Line 435: "Implement YAML serialization and file writing" — generated capabilities are not persisted to disk.

**Implementation Notes:**
- `[x]` Use a small RocksDB key (`utils_capgen_state`) to persist `last_run_timestamp` and `last_document_count`; load on construction.
- `[x]` At line 193: compare `now - last_run_timestamp` against `config_.schedule_interval_s`; skip regeneration if within interval.
- `[x]` At line 373: persist the current `document_count` to the state key after each successful run.
- `[x]` At line 435: serialize the generated `CapabilitySet` to YAML using `yaml-cpp` and atomically write via `ConfigPathResolver::resolveWritable()`.

---

### `PKIClient`: Replace Fallback Stub Verification
**Priority:** High
**Target Version:** v1.8.0

`pki_client.cpp` has 2 stub fallback paths:
- Line 456: "Fallback: stub behavior (base64 of hash)" — certificate issuance falls back to a non-standard base64 encoding instead of real PKCS#10 / X.509 certificate.
- Line 575: "Fallback stub verification: compare base64(hash) equality" — TLS certificate verification falls back to comparing base64 hashes instead of validating the certificate chain.

**Implementation Notes:**
- `[x]` Line 456: implement real PKCS#10 CSR generation and submission using OpenSSL `X509_REQ_*` API; only fall back when ACME/internal CA is not configured.
- `[x]` Line 575: implement real X.509 chain verification using `X509_verify_cert()` with the configured trust store; never fall back to hash comparison for production traffic.
- `[x]` Add explicit `#ifdef THEMIS_TEST_MODE` guard around the stub paths so they cannot be used in production builds.

---


### [x] Streaming PII Scanner for Large Documents
**Priority:** High
**Target Version:** v0.9.0

Refactor `pii_detection_engine.cpp` and `pii_pseudonymizer.cpp` to operate on a chunked streaming interface so that large legal documents (>100 MB) can be processed without full in-memory buffering. Entity spans that straddle chunk boundaries must be detected and merged correctly.

**Implementation Notes:**
- `[x]` Define a `PIIStreamScanner` class in `pii_detection_engine.cpp` with `scan_chunk(chunk, is_last)` → `PIISpanList`; internally maintains a lookahead buffer sized to the maximum entity length (configurable, default 256 bytes) to handle cross-boundary spans.
- `[x]` `pii_pseudonymizer.cpp` adds a companion `PIIStreamPseudonymizer::process_chunk()` that applies replacements using the span offsets from `PIIStreamScanner`; replacements are deterministic per `(entity_text, tenant_id)` using HMAC-SHA-256 keyed with the tenant pseudonymisation key from `lek_manager.cpp`.
- `[x]` The `regex_detection_engine.cpp` must also support chunk-boundary-aware matching; cross-chunk regex detection uses a sliding window overlap equal to `max_pattern_length` (implemented via `IPIIDetectionEngine::maxPatternLength()` and `RegexDetectionEngine::maxPatternLength()`, used by `PIIStreamScanner` constructor to auto-derive `lookahead_bytes`).
- `[x]` Add a throughput benchmark in `benchmarks/` measuring end-to-end scan+pseudonymise throughput on 100 MB synthetic legal text (`bench_pii_stream_scanner.cpp`).

**Performance Targets:**
- Streaming PII scan throughput: >100 MB/s per core for English legal text.
- Memory footprint during streaming scan of 1 GB document: <10 MB.

---

### [~] Tamper-Evident Audit Log with Hash Chain
**Priority:** High
**Target Version:** v0.9.0

Extend `audit_logger.cpp` to link audit entries into a cryptographic hash chain (each entry includes the SHA-256 hash of the previous entry). This makes offline tampering detectable without requiring a trusted external log service.

**Implementation Notes:**
- Add a `HashChainAuditWriter` class in `audit_logger.cpp` that maintains the running chain head in a memory-mapped file; on write, computes `entry_hash = SHA256(prev_hash || entry_json)` and appends both fields to the log record.
- Provide a `AuditLogVerifier::verify_chain(log_path)` tool (standalone binary) that replays the hash chain and reports the first tampered or missing entry.
- Chain head must be persisted to a separate `audit_chain_head.bin` file (fsync'd after each write) so verification can resume from any point without replaying the full log.
- Integrate `utils/hkdf_helper.cpp` to derive the initial chain seed from the cluster's root key so chain heads are cluster-specific and cannot be forged with a fresh chain.

**Performance Targets:**
- Audit log write throughput: >10k events/s per node (SHA-256 computed inline, no external call).
- Hash chain verification throughput: >50k entries/s.

---

### [ ] HKDF Key Derivation Cache with TTL-Based Eviction
**Priority:** Medium
**Target Version:** v0.9.0

Harden `hkdf_cache.cpp` to enforce per-entry TTL eviction and cap the maximum number of cached derived keys to prevent unbounded memory growth. Cached entries must be zeroed from memory on eviction to prevent derived key material from persisting in heap memory.

**Implementation Notes:**
- Refactor `hkdf_cache.cpp` to use a bounded LRU structure (max entries configurable, default 1000) with per-entry TTL (default 300 s); on TTL expiry or LRU eviction, call `OPENSSL_cleanse()` on the key buffer before deallocation.
- Add `HKDFCache::purge_by_ikm_hash()` to allow immediate invalidation of all entries derived from a given root key when that key is rotated in `lek_manager.cpp`.
- Cache hit/miss and eviction counts must be exported as Prometheus-compatible counters via `utils/tracing.cpp` span attributes.
- Ensure `hkdf_cache.cpp` is thread-safe under concurrent key derivation requests from the `timeseries` and `sharding` modules using a sharded mutex to reduce contention.

**Performance Targets:**
- Cache hit latency: <1 µs (pointer lookup, no crypto).
- HKDF-SHA-256 derive on cache miss: <100 µs (OpenSSL EVP HKDF).
- Memory overhead of 1000-entry cache: <1 MB.

---

### [ ] Structured Log Sampling and Rate-Limiting
**Priority:** Medium
**Target Version:** v0.10.0

Extend `logger.cpp` with per-call-site log sampling and rate-limiting to prevent high-frequency code paths from flooding the log pipeline under load. Sampling rate is configurable per log level and module, hot paths can declare their expected burst rate.

**Implementation Notes:**
- Add a `SampledLogger` decorator in `logger.cpp` that wraps the underlying `ILogger` interface; sampling is implemented with a token-bucket rate limiter (one bucket per unique `(file, line, level)` key) to avoid suppressing all instances of a message.
- Configuration is loaded from the `config` module at startup and hot-reloadable via SIGHUP handler; default sampling rates: DEBUG 1%, INFO 10%, WARN 100%, ERROR 100%.
- Suppressed log counts are emitted as a `logs_suppressed_total` counter via the observability subsystem so operators can detect unexpected sampling.
- `audit_logger.cpp` events must bypass the rate limiter entirely; compliance events must never be sampled.

**Performance Targets:**
- Logger hot path overhead with sampling enabled: <200 ns per suppressed log call.
- Rate-limiter memory overhead: <10 KB per 100 unique call sites.

---

### [ ] SAGA Logger Compaction and Replay API
**Priority:** Medium
**Target Version:** v0.9.0

Add a compaction job and a public replay API to `saga_logger.cpp` so that completed SAGA transactions are compacted out of the active WAL and compensating transactions can be replayed programmatically during disaster recovery.

**Implementation Notes:**
- Implement `SAGALogCompactor::compact(before_txn_id)` in `saga_logger.cpp` that rewrites the WAL, retaining only steps for in-flight or failed transactions; completed transactions are archived to the `timeseries` module for audit retention.
- Add `SAGALogReplayer::replay_incomplete(recovery_handler)` that scans the WAL for transactions in `COMPENSATING` state and calls the provided handler for each unconfirmed compensation step; used by `sharding/cross_shard_transaction.cpp` during node recovery.
- Compaction must be atomic (write new WAL, fsync, rename); partial compaction must not leave the WAL in a corrupted state.
- Compaction progress must be emitted as a structured log entry to `audit_logger.cpp` for the compliance audit trail.

**Performance Targets:**
- Compaction throughput: >50k SAGA step records/s.
- Replay scan of 1M-step WAL: <10 s.
- WAL write throughput: >20k step records/s under concurrent SAGA transactions from sharding module.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >85% new code | Cover `PIIStreamScanner` chunk-boundary cases, `HashChainAuditWriter`, `HKDFCache` eviction, `SampledLogger` rate-limiting |
| Integration | Cross-module key derivation flows | Verify `lek_manager.cpp` → `hkdf_helper.cpp` → `timeseries` encryption round-trip |
| Security | PII detection recall ≥95% | Test against legal-domain PII fixture dataset; verify no PII leaks in audit log |
| Reliability | Audit chain tamper detection | Inject bit-flip in log file; verify `AuditLogVerifier` detects it |
| Performance | P99 < budgets above | Streaming PII scan, audit write throughput, HKDF cache hit latency |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| PII scan throughput | ~20 MB/s | >100 MB/s | Streaming scanner microbenchmark on legal corpus |
| Audit log write throughput | ~3k events/s | >10k events/s | Hash-chain writer benchmark (SHA-256 inline) |
| HKDF derive (cache miss) | ~500 µs | <100 µs | OpenSSL EVP HKDF microbenchmark |
| HKDF cache hit latency | N/A (no cache) | <1 µs | LRU pointer-lookup microbenchmark |
| Logger hot path (suppressed, sampling) | ~800 ns | <200 ns | Flame-graph profiling of sampled logger |
| SAGA WAL write throughput | ~8k steps/s | >20k steps/s | Concurrent SAGA stress test from sharding module |
| SAGA compaction throughput | N/A | >50k records/s | Compaction microbenchmark on 10M-step WAL fixture |

## Security / Reliability

- [ ] HKDF-derived keys must be zeroed from memory (`OPENSSL_cleanse`) immediately after use and must not appear in any log output, metric label, or error message; validate this in code review and via `audit_logger.cpp` scrub checks.
- [ ] The `audit_logger.cpp` hash chain must be verified on node startup; if the chain is broken, the node must refuse to start and alert operators rather than silently accepting a potentially tampered log.
- [ ] `pii_pseudonymizer.cpp` pseudonymisation keys must be tenant-scoped and derived via `hkdf_helper.cpp` from the tenant root key; cross-tenant pseudonymisation with a shared key is architecturally prohibited.
- [ ] `pki_client.cpp` certificate validation must reject expired, revoked (via OCSP stapling), and self-signed certificates unless explicitly whitelisted in the node trust store; no `verify=false` escape hatch in production builds.
- [!] Review whether `regex_detection_engine.cpp` ReDoS exposure exists on attacker-controlled PII patterns; fuzz the engine with `fuzz/` harness before GA.
- [ ] `lek_manager.cpp` must enforce a maximum DEK age policy (default 30 days) and automatically trigger key rotation; rotation events must be logged to `audit_logger.cpp` with old and new key IDs (not key material).

---

## Security Hardening Backlog (Q3 2026)

> GAP-005 – identified via static analysis (2026-04-21).
> Reference: `docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md`.

### GAP-005 – Replace MD5 with SHA-256 in `checksum_utils.cpp`

**Scope:** `src/utils/checksum_utils.cpp:58` (`calculateMD5`)

### Design Constraints
- All existing callers of `calculateMD5()` must be migrated; the function must be renamed
  or deprecated with a clear error to prevent new callers
- Binary format of stored checksums in metadata tables will change from 32-hex-char (MD5)
  to 64-hex-char (SHA-256); migration guide required

### Required Interfaces
```cpp
// New function (replaces calculateMD5):
std::string calculateSHA256(const std::string& file_path);

// Deprecated shim (compile-time warning):
[[deprecated("Use calculateSHA256; MD5 is cryptographically broken")]]
std::string calculateMD5(const std::string& file_path);
```

### Implementation Notes
- Use `EVP_MD_CTX` + `EVP_sha256()` from OpenSSL (already a dependency):
  ```cpp
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
  // read file in chunks, EVP_DigestUpdate per chunk
  EVP_DigestFinal_ex(ctx, digest, &len);
  EVP_MD_CTX_free(ctx);
  ```
- The existing `MD5_CTX` / `MD5_Init` / `MD5_Update` / `MD5_Final` OpenSSL APIs are
  deprecated in OpenSSL 3.0 and will be removed in a future release

### Test Strategy
- Unit test: known file → SHA-256 digest matches `sha256sum` reference value
- Unit test: `calculateMD5` call → compiler deprecation warning (test via `-Werror=deprecated`)
- Migration test: update fixture checksums in all tests that use `calculateMD5`

### Performance Targets
- SHA-256 throughput (OpenSSL, AES-NI-class CPU): ≥ 500 MB/s for large files
- Overhead vs MD5: ≤ 2× (acceptable for a one-time file integrity check)

### Security / Reliability
- MD5 collision attacks are feasible with commodity hardware (< 1 hour); SHA-256 has
  no known collision attacks
- OpenSSL's EVP interface is FIPS 140-3 compliant when using the FIPS provider
