> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Utils Module — Public Headers

**Module Path:** `include/utils/`
**Implementation:** `../../src/utils/`
**Namespace:** `themis::utils` (geospatial types: `themis::geo`)
**Status:** 🟢 Production-Ready

Cross-cutting utility headers used throughout ThemisDB.
All headers are `#pragma once` guarded and contain no implementation code.

## Purpose

The `utils` module provides shared infrastructure consumed by every other ThemisDB module:
structured logging, tamper-evident audit logging, PII detection and pseudonymisation,
HKDF key derivation, LEK (Local Encryption Key) management, distributed tracing,
compression codecs, text processing, geospatial helpers, pagination cursors, serialisation,
safe arithmetic, and a collection of data-structure and concurrency primitives.

## Header Surface (Public API)

### Logging and Observability

| Header | Main API | Purpose |
|---|---|---|
| `logger.h` | `Logger`, `LogLevel`, `LogMetrics` | Structured spdlog-backed logging facade; per-level metrics counters |
| `logger_impl.h` | `LoggerImpl` | Logger back-end (sink wiring); internal use only |
| `audit_logger.h` | `AuditLogger`, `SecurityEventType` | Tamper-evident append-only audit trail; JSON events + hash chain |
| `saga_logger.h` | `SagaLogger`, `SAGALogCompactor`, `SAGALogReplayer` | SAGA step WAL logging, compaction, and incomplete-transaction replay |
| `tracing.h` | `Tracer`, `Span`, `SamplingStrategy` | OpenTelemetry-compatible distributed tracing; no-op when collector unreachable |

### PII Detection and Privacy

| Header | Main API | Purpose |
|---|---|---|
| `pii_detector.h` | `PIIDetector` | Plugin-based, runtime-reloadable PII detection orchestrator |
| `pii_detection_engine.h` | `PIIDetectionEngine`, `IPIIDetectionEngine` | PII detection engine interface and orchestration |
| `pii_pseudonymizer.h` | `PIIPseudonymizer` | Deterministic HMAC-based PII pseudonymisation |
| `pii_redacting_sink.h` | `PIIRedactingSink` | spdlog sink that redacts PII inline before writing |
| `regex_detection_engine.h` | `RegexDetectionEngine` | Regex-based pattern detection (email, phone, SSN, IBAN, IP, URL) |
| `ner_detection_engine.h` | `NERDetectionEngine` | MITIE/ONNX-backed Named-Entity Recognition engine |

### Cryptographic Helpers

| Header | Main API | Purpose |
|---|---|---|
| `hkdf_helper.h` | `HkdfHelper` | HKDF-SHA-256 key derivation per RFC 5869 |
| `hkdf_cache.h` | `HkdfCache` | Thread-safe bounded LRU cache for derived HKDF key material (TTL: 300 s, capacity: 1 000 entries) |
| `lek_manager.h` | `LEKManager` | Local Encryption Key (AES-256 DEK) management and automated rotation |
| `pki_client.h` | `PKIClient` | Certificate issuance and verification client; degrades gracefully when CA unreachable |
| `openssl_deleter.h` | `OpenSSLDeleter` | RAII deleters for OpenSSL objects (`BIO`, `EVP_PKEY`, etc.) |
| `checksum_utils.h` | `crc32()`, `sha256()` | CRC-32 and SHA-256 data integrity checksums |
| `hash_util.h` | `hashBytes()`, `hashString()` | General-purpose MurmurHash / FNV-1a utilities |
| `uuid.h` | `generateUUID()`, `generate_uuid_v7()` | RFC 4122 UUIDv4 and RFC 9562 UUIDv7 (monotonic, thread-safe) |

### Compression and Serialisation

| Header | Main API | Purpose |
|---|---|---|
| `zstd_codec.h` | `ZstdCodec`, `zstd_compress_stream()`, `zstd_decompress_stream()` | Zstandard compress/decompress; streaming API with 4 GB DoS guard |
| `lz4_codec.h` | `LZ4Codec` | LZ4 block compress/decompress |
| `serialization.h` | `serialize()`, `deserialize()` | Generic binary serialisation |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio and throughput tracking |
| `lossless_vector_compression.h` | `LosslessVectorCompressor` | Lossless float-vector compression (bit-packing) |
| `lossless_vector_integration.h` | `LosslessVectorIntegration` | Integration helpers for lossless vector compression |

### Text Processing

| Header | Main API | Purpose |
|---|---|---|
| `normalizer.h` | `Normalizer` | Unicode/umlaut text and numeric normalisation |
| `stemmer.h` | `Stemmer` | Porter / Snowball language stemming |
| `stopwords.h` | `Stopwords` | Stop-word filter for search text processing |
| `string_utils.h` | `trim()`, `split()`, `toLower()` | String manipulation utilities |

### Data Structures and Concurrency Primitives

| Header | Main API | Purpose |
|---|---|---|
| `bloom_filter.h` | `BloomFilter` | Probabilistic membership filter (double-hashing, `std::shared_mutex`) |
| `concurrent_cache.h` | `ConcurrentCache` | Thread-safe generic LRU cache |
| `consistent_hash.h` | `ConsistentHash`, `ConsistentHashRing` | FNV-1a 64-bit consistent hashing ring with virtual nodes for sharding |
| `rate_limiter.h` | `RateLimiter` | Token-bucket rate limiter; `try_acquire` (non-blocking) and `acquire` (blocking) |
| `batch_operation_manager.h` | `BatchOperationManager` | Batched async operation dispatch |
| `thread_pool_manager.h` | `ThreadPoolManager` | Named thread pool lifecycle management |
| `thread_safety.h` | `ThreadSafetyAnnotations` | Clang thread-safety annotations (`GUARDED_BY`, etc.) |
| `memory/pool_allocator.h` | `PoolAllocator` | Fixed-size memory pool allocator |

### Network Clients and Connection Pools

| Header | Main API | Purpose |
|---|---|---|
| `grpc_channel_pool.h` | `GrpcChannelPool` | gRPC channel pooling with health-check awareness |
| `http_client_pool.h` | `HttpClientPool` | HTTP connection pool for reuse across requests |
| `retry_policy.h` | `RetryPolicy` | Configurable exponential-backoff retry policy |

### Lifecycle and Scheduling

| Header | Main API | Purpose |
|---|---|---|
| `cursor.h` | `Cursor` | Opaque pagination cursor abstraction |
| `cron_parser.h` | `CronParser` | Cron-expression parsing and next-trigger computation |
| `retention_manager.h` | `RetentionManager` | Data retention policy enforcement and expiry |
| `update_checker.h` | `UpdateChecker` | GitHub release update polling |
| `clock.h` | `IClock`, `SystemClock` | Mockable wall-clock abstraction |
| `timestamp_utils.h` | `nowMs()`, `formatTimestamp()`, `formatDuration()` | ISO 8601 / RFC 3339 timestamp formatting and parsing; Unix-ms helpers |

### Geospatial (sub-directory `geo/`)

| Header | Main API | Purpose |
|---|---|---|
| `geo/ewkb.h` | `GeometryType`, EWKB encode/decode | PostGIS Extended Well-Known Binary geometry encoding |
| `geo/validator.h` | `GeoValidator` | Geometry validity and bounds checking |
| `geometric_distances.h` | `haversine()`, `euclidean()` | Geospatial distance computations |
| `simd_distance.h` | `simdCosine()`, `simdL2()` | SIMD-accelerated cosine and L2 vector distance |

### Error Handling, Safety, and Misc

| Header | Main API | Purpose |
|---|---|---|
| `expected.h` | `tl::expected<T,E>` | Result / expected-value type (tl::expected or built-in fallback) |
| `error_registry.h` | `ErrorRegistry` | Centralised error code registry |
| `safe_access.h` | `safeGet()`, `safeAt()` | Bounds-checked container access |
| `safe_arithmetic.h` | `safeAdd()`, `safeMul()` | Overflow-safe arithmetic |
| `safe_cast.h` | `safeCast<T>()` | Checked numeric casts (throws on narrowing) |
| `pointer_utils.h` | `notNull()`, `downcast()` | Non-null assertion and safe `dynamic_cast` |
| `unaligned_access.h` | `readUnaligned<T>()` | Safe unaligned memory reads (memcpy-based) |
| `memory_utils.h` | `alignedAlloc()`, `secureZero()` | Aligned allocation and cryptographic-safe memory zeroing |
| `file_utils.h` | `readFile()`, `writeFile()` | Portable file I/O helpers |
| `json_helpers.h` | `parseJson()`, `serializeJson()` | nlohmann/json serialisation wrappers |
| `type_conversion.h` | `toProto()`, `fromProto()` | Protobuf type conversion helpers |
| `input_validator.h` | `InputValidator` | Input sanitisation and constraint validation |
| `utils_interfaces.h` | `IHasher`, `ISerializer` | Abstract utility interfaces |
| `utils_adapters.h` | `UtilsAdapters` | Adapter helpers bridging utils interfaces |
| `capability_auto_generator.h` | `CapabilityAutoGenerator` | Runtime capability discovery and YAML persistence |
| `self_awareness.h` | `SelfAwareness` | Runtime self-diagnostics and health signals |

## Configuration Options (Public API)

| API | Key Options / Parameters |
|---|---|
| `RateLimiter(rate_per_second, burst_size)` | `rate_per_second` — token refill rate; `burst_size` — maximum token capacity |
| `HkdfCache` | Capacity: 1 000 entries (LRU); TTL: 300 s per entry; sharded mutex for low contention |
| `BloomFilter(n, p)` | `n` — expected element count; `p` — false-positive probability |
| `ConsistentHashRing` | Number of virtual nodes per physical node (controls distribution uniformity) |
| `ZstdCodec` | Compression level 1–22; streaming `max_output_bytes` DoS guard (default 4 GB) |
| `Tracer` / `SamplingStrategy` | `ALWAYS_ON`, `ALWAYS_OFF`, `PROBABILITY` (rate in [0,1]), `PARENT_BASED`, `ADAPTIVE` |
| `PIIDetector` | YAML-driven engine selection: `RegexDetectionEngine` (default), `NERDetectionEngine` (optional), `EmbeddingDetectionEngine` (optional) |
| `ThreadPoolManager` | Named pools with configurable thread counts |
| `RetryPolicy` | Max retries, base delay, multiplier, jitter |
| `LEKManager` | Rotation schedule interval; AES-256 DEK per series ID |

## Runtime Behavior, Failure Cases, and Limits

- **Logger / AuditLogger**: Log writes are thread-safe. `AuditLogger` maintains a tamper-evident hash chain (SHA-256); chain verification is available via `AuditLogVerifier`. Writing to a full or unavailable sink logs a warning and continues — audit events are never silently dropped.
- **HkdfCache**: On cache miss, key material is derived fresh and inserted. On `purge_by_ikm_hash()`, all entries derived from the given IKM are invalidated with `OPENSSL_cleanse`. Raw IKM is never stored.
- **PIIDetector**: Falls back to safe defaults (regex-only) when YAML config is missing or malformed. All engines are thread-safe for concurrent `scan()` calls.
- **ZstdCodec streaming**: `zstd_decompress_stream()` raises an error if the decompressed output exceeds `max_output_bytes` (default 4 GB) to guard against decompression bombs.
- **RateLimiter**: `acquire(n)` blocks indefinitely until tokens are available. `try_acquire(n)` returns `false` immediately if tokens are insufficient. `set_rate()` is thread-safe.
- **ConsistentHashRing**: `getNodes(key, n)` returns up to `n` distinct physical nodes for replication; returns fewer if the ring has fewer than `n` nodes.
- **Tracer / Span**: When the OpenTelemetry collector is unreachable, all span operations are no-ops — no exception is thrown and no data is buffered.
- **PKIClient**: Certificate verification falls back to a stub implementation when the CA endpoint is unreachable; the fallback is logged at WARN level.
- **UUIDv7**: `generate_uuid_v7()` guarantees monotonicity within the same millisecond via a thread-local sequence counter (max 2^18 per ms). MT19937-64 provides the random component.
- **`safeCast<T>()`**: Throws `std::overflow_error` on narrowing conversion failures.
- **`safeAt()` / `safeGet()`**: Throws `std::out_of_range` on out-of-bounds access.

## Usage Snippets

```cpp
// Structured logging
#include "utils/logger.h"
auto log = themis::utils::Logger::get("my_module");
log->info("Processing record id={}", record_id);
```

```cpp
// Audit event
#include "utils/audit_logger.h"
themis::utils::AuditLogger audit("/var/log/themis/audit.jsonl");
audit.log(themis::utils::SecurityEventType::LOGIN_SUCCESS, user_id, metadata);
```

```cpp
// PII detection and pseudonymisation
#include "utils/pii_detector.h"
#include "utils/pii_pseudonymizer.h"
themis::utils::PIIDetector detector("/etc/themis/pii_config.yaml");
auto spans = detector.scan(raw_text);
themis::utils::PIIPseudonymizer pseudo(tenant_hmac_key);
std::string redacted = pseudo.pseudonymize(raw_text, spans);
```

```cpp
// HKDF key derivation (result never cached with raw IKM)
#include "utils/hkdf_helper.h"
auto key = themis::utils::HkdfHelper::derive_key(ikm, "encryption-v1", 32);
```

```cpp
// Distributed tracing
#include "utils/tracing.h"
auto span = themis::Tracer::start_span("index_write", parent_ctx);
// ... do work ...
span->end();
```

```cpp
// Zstd streaming compression
#include "utils/zstd_codec.h"
themis::utils::ZstdCodec codec(/*level=*/3);
auto compressed = codec.compress(input_bytes);
auto decompressed = codec.decompress(compressed);
```

```cpp
// Token-bucket rate limiting
#include "utils/rate_limiter.h"
themis::utils::RateLimiter limiter(/*rate=*/1000.0, /*burst=*/200.0);
if (!limiter.try_acquire(1)) {
    return tl::make_unexpected("rate limit exceeded");
}
```

```cpp
// UUID v7 (time-ordered, monotonic)
#include "utils/uuid.h"
std::string id = themis::utils::generate_uuid_v7();
```

## Troubleshooting

| Symptom | Likely Cause | Recommended Check |
|---|---|---|
| `HkdfCache` misses are high | TTL too short or high IKM churn | Increase TTL or reduce unique IKM count; verify `purge_by_ikm_hash()` is not called too aggressively |
| PII detection returns empty spans | YAML config load failure or engine not registered | Check logs for YAML parse warnings; ensure engine library (MITIE/ONNX) is available |
| `AuditLogVerifier` reports tampered chain | Log file was externally modified or chain head is stale | Replay from known-good genesis; investigate file-system access to the audit log |
| `Tracer::start_span` does nothing | Collector unreachable and `ALWAYS_ON` sampling | Verify OpenTelemetry collector endpoint and `THEMIS_ENABLE_TRACING` compile flag |
| `ZstdCodec::decompress` throws | Input exceeds `max_output_bytes` | Increase limit or validate input origin; do not disable guard in production |
| `safeCast<T>()` throws `overflow_error` | Value out of target type range | Validate input range before casting or use a wider target type |
| `PKIClient` uses stub verification | CA endpoint unreachable | Check CA URL configuration in ThemisDB config and network connectivity |
| `generate_uuid_v7()` sequence wraps | > 2^18 UUIDs generated within the same millisecond | Throttle UUID generation or verify clock resolution |

## Related Documentation

- [Implementation Overview (`src/utils/README.md`)](../../src/utils/README.md)
- [Architecture (`src/utils/ARCHITECTURE.md`)](../../src/utils/ARCHITECTURE.md)
- [Roadmap (`src/utils/ROADMAP.md`)](../../src/utils/ROADMAP.md)
- [Future Enhancements (`src/utils/FUTURE_ENHANCEMENTS.md`)](../../src/utils/FUTURE_ENHANCEMENTS.md)
- [Security Notes (`src/utils/SECURITY.md`)](../../src/utils/SECURITY.md)
- [Changelog (`src/utils/CHANGELOG.md`)](../../src/utils/CHANGELOG.md)
- [German Overview (`docs/de/utils/README.md`)](../../docs/de/utils/README.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

