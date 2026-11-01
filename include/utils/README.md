---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Utils Module — Public Headers

**Module Path:** `include/utils/`
**Implementation:** `../../src/utils/`
**Namespace:** `themis::utils` (geospatial types: `themis::geo`)
**Status:** Component maturity is API-specific; verify the matching header and enabled build features.

Cross-cutting utility headers used throughout ThemisDB. Public declarations live
here; some headers also provide inline or template implementations.

## Purpose

The `utils` module provides shared infrastructure used across ThemisDB:
structured logging, tamper-evident audit logging, PII detection and pseudonymisation,
HKDF key derivation, LEK (Local Encryption Key) management, distributed tracing,
compression codecs, text processing, geospatial helpers, pagination cursors, serialisation,
safe arithmetic, and a collection of data-structure and concurrency primitives.

## Header Surface (Public API)

### Logging and Observability

| Header | Main API | Purpose |
|---|---|---|
| `logger.h` | `Logger`, `Logger::Level`, `LogMetrics` | Structured spdlog-backed logging facade; per-level metrics counters |
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
| `hkdf_helper.h` | `HKDFHelper` | HKDF-SHA-256 key derivation per RFC 5869 |
| `hkdf_cache.h` | `HkdfCache` | Thread-safe bounded LRU cache for derived HKDF key material (TTL: 300 s, capacity: 1 000 entries) |
| `lek_manager.h` | `LEKManager` | Local Encryption Key (AES-256 DEK) management and automated rotation |
| `pki_client.h` | `VCCPKIClient`, `PKIConfig`, `SignatureResult` | Local/REST signing and verification; optional TLS pinning and trust-store configuration |
| `openssl_deleter.h` | `OpenSSLDeleter` | RAII deleters for OpenSSL objects (`BIO`, `EVP_PKEY`, etc.) |
| `checksum_utils.h` | `calculateSHA256()`, deprecated `calculateMD5()` | File checksum helpers; MD5 is retained for legacy verification only |
| `hash_util.h` | `themis::hash::fnv1a32()`, `fnv1a64()` | FNV-1a hash functions |
| `uuid.h` | `generate_uuid_v4()`, `generate_uuid_v7()` | UUIDv4 and timestamped UUIDv7 helpers |

### Compression and Serialisation

| Header | Main API | Purpose |
|---|---|---|
| `zstd_codec.h` | `zstd_compress_safe()`, `zstd_decompress_safe()`, `ZstdStreamCompressor`, `ZstdStreamDecompressor` | Bounded buffer APIs and chunked Zstandard frame processing |
| `lz4_codec.h` | `LZ4Codec` | LZ4 block compress/decompress |
| `serialization.h` | `Serialization::Encoder`, `Serialization::Decoder` | Typed binary serialization with nesting, size, and bounds checks |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio and throughput tracking |
| `lossless_vector_compression.h` | Experimental `SparseVectorCodec`, `VarIntCodec`, `DictionaryCodec`, `AdaptiveCompressor` | Experimental lossless codecs under `themis::experimental`; disabled by default |
| `lossless_vector_integration.h` | Experimental `VectorCompressionHelper` | Experimental integration helpers under `themis::experimental` |

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
| `rate_limiter.h` | `RateLimiter` | Token bucket; non-blocking and timeout-bounded acquisition methods |
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
| `timestamp_utils.h` | `TimestampUtils::format()`, `parse()`, `now()`, `formatDuration()`, `toUnixMs()`, `fromUnixMs()` | ISO 8601 / RFC 3339 timestamp formatting and parsing; Unix-ms conversion |

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
| `safe_access.h` | `safe_get()` | Bounds-checked optional access to vector/map elements |
| `safe_arithmetic.h` | `safe_add()` and related helpers | Overflow/underflow-checked arithmetic returning optional values |
| `safe_cast.h` | `safe_cast<T>()`, `FloatBits` | Same-size bitwise conversion; not a checked numeric narrowing cast |
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
| `ZstdStreamCompressor` / `ZstdStreamDecompressor` | Chunked frame processing; use the safe buffer APIs when explicit size and expansion-ratio validation is required. |
| `Tracer` / `SamplingStrategy` | `ALWAYS_ON`, `ALWAYS_OFF`, `PROBABILITY` (rate in [0,1]), `PARENT_BASED`, `ADAPTIVE` |
| `PIIDetector` | YAML-driven engine loading; the factory instantiates `regex` and `ner`. `embedding` is advertised but not implemented and cannot be instantiated. |
| `ThreadPoolManager` | Named pools with configurable thread counts |
| `RetryPolicy` | Max retries, base delay, multiplier, jitter |
| `LEKManager` | Rotation schedule interval; AES-256 DEK per series ID |

## Runtime Behavior, Failure Cases, and Limits

- **Logger / AuditLogger**: `AuditLogger` can maintain a SHA-256 hash chain when hash chaining is enabled; `AuditLogVerifier` verifies supported chain records. Check each method's error contract rather than assuming a failed write was persisted.
- **HKDFCache**: Cache misses derive and insert a key; TTL and capacity are configurable. The current cache key construction includes the IKM bytes in its in-memory map key. Do not assume IKM is absent from process memory; **TODO:** replace this with a non-reversible keyed/digested cache key and verify zeroization behavior.
- **PIIDetector**: YAML load failure initializes the built-in regex engine; a failed reload retains the existing engines. Engine failures in direct detector calls can yield partial findings; use `PIIStreamScanner` where its fail-closed behavior is required.
- **Zstd**: `zstd_compress_safe()` and `zstd_decompress_safe()` return explicit results and enforce the limits in `include/utils/zstd_codec.h`. The streaming classes use `compress_chunk()` / `flush()` and `decompress_chunk()` / `is_done()`; they do not expose a `max_output_bytes` constructor option.
- **RateLimiter**: `try_acquire()` is non-blocking. Use `try_acquire_for()` or `acquire_with_timeout()` for bounded waits; the public header does not expose an unbounded `acquire()` method.
- **ConsistentHashRing**: `getNodes(key, n)` returns up to `n` distinct physical nodes for replication; returns fewer if the ring has fewer than `n` nodes.
- **Tracer / Span**: Tracing behavior depends on the build configuration and sampler. See `tracing.h` for initialization, exporter, flush, and no-op behavior.
- **VCCPKIClient**: Stub paths are compiled only when `THEMIS_TEST_MODE` is defined. Configure local key/certificate or PKI service paths and explicit trust/pinning settings for production.
- **UUIDv7**: `generate_uuid_v7()` encodes the Unix timestamp in milliseconds and uses random suffix bits from a thread-local `std::mt19937_64`. **TODO:** the header comment claims same-process monotonic ordering, but the implementation generates independent random suffixes within a millisecond; do not rely on that ordering until the contract and implementation are reconciled.
- **`safe_cast<T>()`**: Performs a same-size bitwise conversion for trivially copyable types; it does not validate numeric range.
- **`safe_get()`**: Returns an optional reference; missing indices/keys produce `std::nullopt`.

## Usage Snippets

```cpp
// Structured logging
#include "utils/logger.h"
themis::utils::Logger::init("themis.log");
auto log = themis::utils::Logger::get();
log->info("Processing record id={}", record_id);
```

```cpp
// Audit event
#include "utils/audit_logger.h"
themis::utils::AuditLogger audit("/var/log/themis/audit.jsonl");
audit.logSecurityEvent(themis::utils::SecurityEventType::LOGIN_SUCCESS,
                       user_id, "session", metadata);
```

```cpp
// PII detection
#include "utils/pii_detector.h"
themis::utils::PIIDetector detector("/etc/themis/pii_config.yaml");
auto findings = detector.detectInText(raw_text);
```

```cpp
// HKDF key derivation (result never cached with raw IKM)
#include "utils/hkdf_helper.h"
std::vector<uint8_t> salt = /* caller-provided salt */;
auto key = themis::utils::HKDFHelper::derive(ikm, salt, "encryption-v1", 32);
```

```cpp
// Distributed tracing
#include "utils/tracing.h"
auto span = themis::Tracer::startSpan("index_write");
// ... do work ...
span.end();
```

```cpp
// Safe Zstd buffer compression
#include "utils/zstd_codec.h"
auto compressed = themis::utils::zstd_compress_safe(input_bytes.data(), input_bytes.size(), 3);
if (compressed) {
    auto decompressed = themis::utils::zstd_decompress_safe(*compressed);
}
```

```cpp
// Token-bucket rate limiting
#include "utils/rate_limiter.h"
themis::utils::RateLimiter limiter(/*rate_per_second=*/1000.0, /*burst_size=*/200.0);
if (!limiter.try_acquire(1)) {
    // Reject or defer the operation.
}
```

```cpp
// UUID v7 (timestamped)
#include "utils/uuid.h"
std::string id = themis::utils::generate_uuid_v7();
```

## Troubleshooting

| Symptom | Likely Cause | Recommended Check |
|---|---|---|
| `HkdfCache` misses are high | TTL too short or high IKM churn | Increase TTL or reduce unique IKM count; verify `purge_by_ikm_hash()` is not called too aggressively |
| PII detection returns no findings | No matching PII, missing configuration, or an engine failure | Check `getLastError()` and engine metadata; do not interpret a failed scan as proof that the input contains no PII |
| `AuditLogVerifier` reports a chain error | Log contents or checkpoint do not validate | Preserve the log and verifier result; investigate file-system access and the configured chain/checkpoint |
| Traces are not exported | Tracing support is disabled, no exporter is configured, or exporter initialization/flush failed | Check the build option and `Tracer` initialization/flush result in `tracing.h` |
| Safe Zstd operation returns an error | Unsupported codec, invalid input, or configured size/ratio bounds | Inspect the returned error and validate the input; do not bypass the bounds |
| `safe_get()` returns `std::nullopt` | Index/key is absent | Check the optional result before using the reference |
| PKI operation fails | Missing key/certificate, trust configuration, or service availability | Check `SignatureResult::ok`, `PKIConfig`, and the documented failure result; test stubs are not production fallback |

## Related Documentation

- [Implementation Overview (`src/utils/README.md`)](../../src/utils/README.md)
- [Architecture (`src/utils/ARCHITECTURE.md`)](../../src/utils/ARCHITECTURE.md)
- [Roadmap (`src/utils/ROADMAP.md`)](../../src/utils/ROADMAP.md)
- [Future Enhancements (`src/utils/FUTURE_ENHANCEMENTS.md`)](../../src/utils/FUTURE_ENHANCEMENTS.md)
- [Security Notes (`src/utils/SECURITY.md`)](../../src/utils/SECURITY.md)
- [Changelog (`src/utils/CHANGELOG.md`)](../../src/utils/CHANGELOG.md)
- [German Overview (`docs/de/utils/README.md`)](../../docs/de/utils/README.md)

## Build and tests

The headers are part of the repository's core target, not a separately
installed utils library. In-tree consumers link `themis_core`; tests are
registered through [`tests/utils/CMakeLists.txt`](../../tests/utils/CMakeLists.txt).
See the [test guide](../../tests/utils/PHASE_4_TEST_EXECUTION_GUIDE.md) for
focused build and CTest commands.

```cmake
target_link_libraries(your_target PRIVATE themis_core)
```
