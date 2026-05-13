> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Utils Module

**Module Path:** `src/utils/`
**Namespace:** `themis::utils` (geospatial types: `themis::geo`)
**Status:** 🟢 Production-Ready (v1.5.0)

---

## Module Purpose

The Utils module provides shared infrastructure and cross-cutting utility components used by every other ThemisDB module. It covers: structured and audit logging, PII detection and pseudonymisation, HKDF key derivation, LEK management, distributed tracing, ZSTD/LZ4 compression, text processing (normalisation, stemming, stop-words), serialisation, geospatial helpers, pagination cursors, concurrency primitives, safe arithmetic, and network connection pools.

**In scope:** Audit logger, cursor/pagination, HKDF key derivation, LEK manager, structured logger, text normalizer, PII detection and pseudonymization, PKI client, retention manager, SAGA logger, serialization helpers, stemmer/stopwords, distributed tracing, ZSTD/LZ4 codec, geospatial utilities, bloom filter, consistent hash ring, rate limiter, timestamp utilities.

**Out of scope:** Business logic, module-specific data models, high-level orchestration.

---

## Public API Entry Points

The implementation in `src/utils/` backs the public headers in
[`../../include/utils/README.md`](../../include/utils/README.md):

- **Logging / Audit:** `logger.h`, `audit_logger.h`, `saga_logger.h`, `tracing.h`
- **PII / Privacy:** `pii_detector.h`, `pii_detection_engine.h`, `pii_pseudonymizer.h`, `pii_redacting_sink.h`, `regex_detection_engine.h`, `ner_detection_engine.h`
- **Crypto / Keys:** `hkdf_helper.h`, `hkdf_cache.h`, `lek_manager.h`, `pki_client.h`, `checksum_utils.h`, `hash_util.h`, `uuid.h`
- **Compression / Serialisation:** `zstd_codec.h`, `lz4_codec.h`, `serialization.h`, `compression_metrics.h`, `lossless_vector_compression.h`
- **Text Processing:** `normalizer.h`, `stemmer.h`, `stopwords.h`, `string_utils.h`
- **Data Structures / Concurrency:** `bloom_filter.h`, `concurrent_cache.h`, `consistent_hash.h`, `rate_limiter.h`, `thread_pool_manager.h`, `memory/pool_allocator.h`
- **Scheduling / Lifecycle:** `cursor.h`, `cron_parser.h`, `retention_manager.h`, `clock.h`, `timestamp_utils.h`
- **Geo:** `geo/ewkb.h`, `geo/validator.h`, `geometric_distances.h`, `simd_distance.h`
- **Error Handling / Safety:** `expected.h`, `error_registry.h`, `safe_access.h`, `safe_arithmetic.h`, `safe_cast.h`, `pointer_utils.h`

---

## Main Implementation Components (`src/utils/`)

| File | Responsibility | Notable Runtime Behavior |
|---|---|---|
| `logger.cpp` | Structured spdlog-backed logging facade; per-level metrics counters | Thread-safe; sink errors are suppressed to avoid cascading failures |
| `audit_logger.cpp` | Tamper-evident append-only audit trail; JSON events + SHA-256 hash chain | Events are never silently dropped; chain head is persisted for verifier |
| `saga_logger.cpp` | SAGA step WAL logging | Write-ahead log; `SAGALogCompactor` uses atomic rename for safe compaction |
| `tracing.cpp` | OpenTelemetry-compatible distributed trace span management | No-op when collector unreachable; no buffering or exception on collector loss |
| `pii_detector.cpp` | Plugin-based PII detection orchestrator (regex + optional NER + embeddings) | Falls back to regex-only when YAML config missing or malformed |
| `pii_detection_engine.cpp` | PII detection engine orchestration | Engines are loaded/unloaded via YAML; thread-safe for concurrent `scan()` |
| `pii_pseudonymizer.cpp` | Deterministic HMAC-based PII pseudonymisation | Deterministic per-tenant; reversible under admin key |
| `pii_stream_scanner.cpp` | Streaming PII scanner with lookahead buffer | Handles arbitrarily large documents without full in-memory load |
| `regex_detection_engine.cpp` | Regex-based pattern detection | Covers email, phone, SSN, credit cards, IBAN, IP, URL |
| `ner_detection_engine.cpp` | MITIE/ONNX-backed Named-Entity Recognition | Optional; higher overhead than regex; detects person/location/org |
| `hkdf_helper.cpp` | HKDF-SHA-256 key derivation per RFC 5869 | Raw IKM never cached or logged; single-call derive contract |
| `hkdf_cache.cpp` | Bounded LRU cache for derived HKDF key material | Capacity: 1 000 entries; TTL: 300 s; `OPENSSL_cleanse` on eviction; sharded mutex |
| `lek_manager.cpp` | Local Encryption Key (AES-256 DEK) management | Returns DEK per series ID; automated rotation; rotation schedule configurable |
| `pki_client.cpp` | Certificate issuance and verification | Degrades gracefully (stub fallback + WARN log) when CA unreachable |
| `checksum_utils.cpp` | CRC-32 and SHA-256 checksums | Stateless; no allocation for typical input sizes |
| `zstd_codec.cpp` | Zstandard compress/decompress; streaming API | `zstd_decompress_stream` enforces `max_output_bytes` (default 4 GB) DoS guard |
| `lz4_codec.cpp` | LZ4 block compress/decompress | Stateless; no streaming variant |
| `serialization.cpp` | Generic binary serialisation | Schema-version field for forward compatibility |
| `normalizer.cpp` | Unicode/umlaut text and numeric normalisation | Stateless utility; handles German umlaut expansion |
| `stemmer.cpp` | Porter / Snowball language stemming | Stateless; language selected at construction |
| `stopwords.cpp` | Stop-word filter for search pipelines | Built-in EN/DE word lists; custom lists injectable |
| `bloom_filter.cpp` | Probabilistic membership filter | Double-hashing; `std::shared_mutex`; not persisted across restarts |
| `consistent_hash.cpp` | FNV-1a 64-bit consistent hash ring | Virtual nodes; `getNodes(key, n)` for replication factor |
| `rate_limiter.cpp` | Token-bucket rate limiter | `acquire` blocks; `try_acquire` non-blocking; `set_rate` is thread-safe |
| `cursor.cpp` | Opaque pagination cursor abstraction | Serialisable; safe across RPC boundaries |
| `cron_parser.cpp` | Cron-expression parsing and next-trigger computation | Standard 5-field cron syntax |
| `retention_manager.cpp` | Data retention policy enforcement and expiry | Policy-driven; expiry events emitted to audit log |
| `timestamp_utils.cpp` | ISO 8601 / RFC 3339 parse and format | Handles millisecond precision and timezone offsets |
| `simd_distance.cpp` | SIMD-accelerated cosine and L2 vector distance | Falls back to scalar path when SIMD unavailable |
| `thread_pool_manager.cpp` | Named thread pool lifecycle management | Pools are created on first access; shutdown is cooperative |
| `error_registry.cpp` | Centralised error code registry | Thread-safe registration; duplicate codes logged at WARN |
| `audit_logger.cpp` (chain) | Hash-chain audit writer + chain verifier | `HashChainAuditWriter` / `AuditLogVerifier` — detects first tampered link |
| `capability_auto_generator.cpp` | Runtime capability discovery and YAML persistence | Uses RocksDB state key; persists `last_run_timestamp` and document count |
| `geo/ewkb.cpp` | PostGIS EWKB geometry encoding/decoding | Supports Point, LineString, Polygon, Multi* and 3D variants |
| `build_info.cpp` | Build version and compile-time info helpers | Stateless |
| `boost_throw_exception.cpp` | Boost throw_exception override | Converts Boost exceptions to `std::runtime_error` |
| `sampled_logger.cpp` | Per-call-site token-bucket sampling decorator | Suppression counter available for metrics |
| `utils_adapters.cpp` | Adapter helpers bridging utils interfaces | Thin wrappers; no state |

---

## Configuration Options (Implementation-Relevant)

| Component | Configurable Parameters |
|---|---|
| `HkdfCache` | Capacity (default: 1 000 entries), TTL (default: 300 s), mutex shard count |
| `RateLimiter` | `rate_per_second`, `burst_size`; mutable via `set_rate()` |
| `BloomFilter` | Expected element count `n`, false-positive probability `p` |
| `ConsistentHashRing` | Virtual nodes per physical node |
| `ZstdCodec` | Compression level 1–22; `max_output_bytes` DoS guard |
| `Tracer` / `SamplingStrategy` | `ALWAYS_ON`, `ALWAYS_OFF`, `PROBABILITY` (rate), `PARENT_BASED`, `ADAPTIVE` |
| `PIIDetector` | YAML config path; engine list (`regex`, `ner`, `embedding`) |
| `LEKManager` | Rotation schedule interval; DEK key length |
| `RetryPolicy` | Max retries, base delay, multiplier, jitter |
| `ThreadPoolManager` | Pool name → thread count mapping |
| `SampledLogger` | Per-site sampling rate (tokens/s), burst |

---

## Runtime Flow

### 1. Audit logging with tamper-evident chain

1. Caller invokes `AuditLogger::log(event_type, actor, metadata)`.
2. Event is serialised to JSON and appended to the log file.
3. SHA-256 hash of the new entry is chained to the previous head; new head is persisted.
4. `AuditLogVerifier::verify(log_path)` replays the file and returns the first broken link (if any).

### 2. PII detection + pseudonymisation pipeline

1. `PIIDetector::scan(text)` dispatches to all enabled engines in priority order.
2. Regex engine runs first (fast, low overhead); NER and embedding engines are optional.
3. Detected spans (type, offset, length, confidence) are returned.
4. `PIIPseudonymizer::pseudonymize(text, spans, policy)` replaces each span with a deterministic HMAC-derived token.
5. For large documents, `PIIStreamScanner` operates in streaming mode with a lookahead buffer.

### 3. HKDF key derivation + caching

1. Caller invokes `HkdfHelper::derive_key(ikm, info, length)`.
2. `HkdfCache` is checked first; on hit, derived key is returned without re-derivation.
3. On miss, HKDF-SHA-256 is applied; result is inserted with TTL; `OPENSSL_cleanse` is called on eviction.
4. Raw IKM is never stored in cache or logs.

---

## Runtime Errors, Failure Cases, and Limits

- **Logger / AuditLogger**: Sink write errors are caught and logged at a lower level; audit events are not silently dropped.
- **HkdfCache**: `purge_by_ikm_hash()` is O(n) over the cache; avoid calling in hot paths.
- **PIIDetector**: Malformed YAML config causes fallback to regex-only mode; error is logged at WARN.
- **ZstdCodec**: `zstd_decompress_stream()` raises `std::runtime_error` if output exceeds `max_output_bytes`. Do not increase the limit beyond validated inputs.
- **RateLimiter**: `acquire(n)` blocks indefinitely; callers must handle cancellation externally.
- **ConsistentHashRing**: `getNodes(key, n)` returns fewer nodes than `n` when the ring is underpopulated.
- **Tracer**: All span operations are no-ops when the collector is unreachable; no exception is thrown.
- **PKIClient**: Stub fallback is activated when CA is unreachable; stub always returns `verified=true` — not for production security decisions.
- **UUIDv7**: Sequence wraps at 2^18 within the same millisecond; caller must throttle if that rate is expected.
- **`safeCast<T>()`**: Throws `std::overflow_error` on narrowing conversion failures.
- **BloomFilter**: Not persisted across process restarts; membership tests have false positives but no false negatives.

---

## Usage Patterns

### 1. Structured logging (any module)

```cpp
#include "utils/logger.h"
auto log = themis::utils::Logger::get("query_engine");
log->info("Executing query plan steps={}", plan.size());
log->warn("Slow query detected latency_ms={}", latency);
```

### 2. Tamper-evident audit trail

```cpp
#include "utils/audit_logger.h"
themis::utils::AuditLogger audit("/var/log/themis/audit.jsonl");
audit.log(themis::utils::SecurityEventType::LOGIN_SUCCESS, user_id,
          {{"ip", client_ip}, {"session", session_id}});
// Verify chain integrity
themis::utils::AuditLogVerifier verifier;
auto result = verifier.verify("/var/log/themis/audit.jsonl");
if (!result.ok) { /* report tampered link at result.first_bad_entry */ }
```

### 3. PII detection and pseudonymisation

```cpp
#include "utils/pii_detector.h"
#include "utils/pii_pseudonymizer.h"
themis::utils::PIIDetector detector("/etc/themis/pii.yaml");
auto spans = detector.scan(raw_text);
themis::utils::PIIPseudonymizer pseudo(tenant_hmac_key);
std::string safe_text = pseudo.pseudonymize(raw_text, spans);
```

### 4. ZSTD streaming compression

```cpp
#include "utils/zstd_codec.h"
themis::utils::ZstdCodec codec(/*level=*/3);
// Block API
auto compressed = codec.compress(data_bytes);
auto restored   = codec.decompress(compressed);
// Streaming API
themis::utils::zstd_compress_stream(source_fn, sink_fn, /*level=*/3);
```

### 5. Rate limiting in an API handler

```cpp
#include "utils/rate_limiter.h"
static themis::utils::RateLimiter limiter(/*rate=*/500.0, /*burst=*/50.0);
if (!limiter.try_acquire(1)) {
    return tl::make_unexpected("rate limit exceeded");
}
// ... proceed ...
```

### 6. Consistent hash-ring for sharding

```cpp
#include "utils/consistent_hash.h"
themis::utils::ConsistentHashRing ring(/*virtual_nodes=*/150);
ring.addNode("shard-1");
ring.addNode("shard-2");
ring.addNode("shard-3");
auto targets = ring.getNodes(record_key, /*replication=*/2);
```

---

## Dependency Direction

```
utils/ → (no ThemisDB module dependencies)
All other modules → utils/  (permitted)
utils/ → external: spdlog, OpenSSL, nlohmann/json, ZSTD, LZ4, Boost (header-only), optional MITIE/ONNX
```

No ThemisDB module may add a runtime dependency from `utils/` back into any other ThemisDB module.

---

## Troubleshooting

| Symptom | Likely Cause | Recommended Check |
|---|---|---|
| `AuditLogVerifier` reports tampered chain | Log file externally modified or chain head stale | Investigate file-system access; replay from known-good genesis |
| PII engine falls back to regex-only | YAML config missing, malformed, or engine library absent | Check logs for YAML parse errors; verify MITIE/ONNX availability |
| `ZstdCodec::decompress` throws | Input exceeds `max_output_bytes` DoS guard | Validate input origin; do not disable guard in production |
| Tracing spans not appearing in collector | Collector unreachable or `THEMIS_ENABLE_TRACING` not set | Verify collector endpoint and compile flag |
| `RateLimiter::acquire` blocks indefinitely | Token refill rate too low for request volume | Increase `rate_per_second` or add cancellation timeout |
| High `HkdfCache` miss rate | TTL too short or high IKM churn | Tune TTL; check that `purge_by_ikm_hash` is not overcalled |
| `PKIClient` stub fallback active | CA endpoint unreachable | Check CA URL in ThemisDB config and network connectivity |
| `safeCast<T>()` throws `overflow_error` | Value out of target type range | Validate range before casting or widen the target type |

---

## See Also

- [`ARCHITECTURE.md`](ARCHITECTURE.md) — component diagram and dependency constraints
- [`ROADMAP.md`](ROADMAP.md) — delivery phases and production-readiness checklist
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — planned features and enhancements
- [`SECURITY.md`](SECURITY.md) — security baseline (key management, PII, audit)
- [`CHANGELOG.md`](CHANGELOG.md) — implementation-level changes
- [`AUDIT.md`](AUDIT.md) — audit log format and verification procedure
- [`../../include/utils/README.md`](../../include/utils/README.md) — public API documentation
- [`../../docs/de/utils/README.md`](../../docs/de/utils/README.md) — German module overview

## Scientific References

1. Josuttis, N. M. (2012). **The C++ Standard Library: A Tutorial and Reference (2nd ed.)**. Addison-Wesley. ISBN: 978-0-321-62321-8

2. Knuth, D. E. (1998). **The Art of Computer Programming, Vol. 3: Sorting and Searching (2nd ed.)**. Addison-Wesley. ISBN: 978-0-201-89685-5

3. Agner Fog. (2023). **Instruction Tables: Lists of Instruction Latencies, Throughputs and Micro-operation Breakdowns for Intel, AMD and VIA CPUs**. Technical University of Denmark. https://www.agner.org/optimize/instruction_tables.pdf

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

