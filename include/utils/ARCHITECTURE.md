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

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/utils/ARCHITECTURE.md -->

# Utilities Module — Public Header Architecture

**Module Path:** `include/utils/`  
**Implementation:** `../../src/utils/`  
**Canonical architecture doc:** [`../../src/utils/ARCHITECTURE.md`](../../src/utils/ARCHITECTURE.md)

---

## 1. Overview

`include/utils/` declares shared APIs for logging, tracing, hashing,
compression, PII detection/redaction, retry policies, thread pools,
serialization, SIMD distances, bloom filters, rate limiting, and connection
pools.

For runtime composition and implementation internals see:
→ [`../../src/utils/ARCHITECTURE.md`](../../src/utils/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Logging and Tracing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `logger.h` | `Logger` | Structured application logger |
| `logger_impl.h` | `LoggerImpl` | Logger backend implementation |
| `tracing.h` | `Tracer`, `Span`, `SamplingStrategy` | Optional OpenTelemetry tracing and span sampling |
| `saga_logger.h` | `SagaLogger` | Saga transaction event logger |
| `audit_logger.h` | `AuditLogger` | Security/compliance audit logger |
| `pii_redacting_sink.h` | `PIIRedactingSink` | PII-redacting log sink |
### 2.2 Hashing and Checksums

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hash_util.h` | `themis::hash::fnv1a32()`, `fnv1a64()` | FNV-1a hash functions |
| `checksum_utils.h` | `calculateSHA256()`, deprecated `calculateMD5()` | File checksum functions; MD5 is legacy-only |
| `bloom_filter.h` | `BloomFilter` | Space-efficient probabilistic membership test |
| `consistent_hash.h` | `ConsistentHash` | Consistent hash ring for sharding |
| `hkdf_helper.h` | `HKDFHelper` | HMAC-based key derivation |
| `hkdf_cache.h` | `HKDFCache` | Cached HKDF derivations |
| `openssl_deleter.h` | `OpenSSLDeleter` | RAII OpenSSL object deleter |
### 2.3 Compression and Encoding

| Header | Public Type | Purpose |
|--------|------------|---------|
| `lz4_codec.h` | `LZ4Codec` | LZ4 fast compression codec |
| `zstd_codec.h` | `zstd_*_safe()`, `ZstdStreamCompressor`, `ZstdStreamDecompressor` | Bounded buffer and streaming Zstandard APIs |
| `lossless_vector_compression.h` | `SparseVectorCodec`, `VarIntCodec`, `DictionaryCodec`, `AdaptiveCompressor` | Experimental codecs in `themis::experimental`; disabled by default |
| `lossless_vector_integration.h` | `VectorCompressionHelper` | Experimental integration helper in `themis::experimental` |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio and speed telemetry |
| `serialization.h` | `Serialization` | Binary/JSON serialisation utilities |
| `json_helpers.h` | `JsonHelpers` | JSON parse/query helpers |
| `type_conversion.h` | `TypeConversion` | Safe cross-type conversion utilities |
| `safe_cast.h` | `safe_cast<T>()`, `FloatBits` | Same-size bitwise conversions, not numeric range casts |
### 2.4 PII and Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `pii_detector.h` | `PIIDetector` | PII engine orchestration and configuration reload |
| `pii_detection_engine.h` | `IPIIDetectionEngine`, `PIIDetectionEngineFactory` | Engine interface and factory; `regex` and `ner` are instantiable |
| `pii_pseudonymizer.h` | `PIIPseudonymizer` | PII-to-UUID mapping backed by configured storage and encryption |
| `ner_detection_engine.h` | `NERDetectionEngine` | Named-entity recognition engine |
| `regex_detection_engine.h` | `RegexDetectionEngine` | Regex-based pattern detection engine |
| `input_validator.h` | `InputValidator` | General-purpose input validation |
| `safe_access.h` | `safe_get()` | Optional-returning vector/map access |
| `safe_arithmetic.h` | `safe_add()` and related helpers | Overflow/underflow-checked arithmetic |
| `pki_client.h` | `VCCPKIClient`, `PKIConfig`, `SignatureResult` | PKI signing and verification client |
### 2.5 Concurrency and Threading

| Header | Public Type | Purpose |
|--------|------------|---------|
| `thread_pool_manager.h` | `ThreadPoolManager` | Sized thread pool manager |
| `thread_safety.h` | Annotation macros | Clang thread-safety annotations |
| `concurrent_cache.h` | `ConcurrentCache` | Thread-safe concurrent cache |
| `rate_limiter.h` | `RateLimiter` | Token-bucket rate limiter |
| `retry_policy.h` | `RetryPolicy` | Configurable retry with backoff |
| `clock.h` | `IClock`, `SystemClock` | Clock interface and system implementation |
### 2.6 String and Text

| Header | Public Type | Purpose |
|--------|------------|---------|
| `string_utils.h` | String helper functions | String manipulation utilities |
| `normalizer.h` | `Normalizer` | Text normalisation pipeline |
| `stemmer.h` | `Stemmer` | Snowball stemmer wrapper |
| `stopwords.h` | `Stopwords` | Multi-language stopword lists |
| `simd_distance.h` | SIMD distance helper functions | SIMD-accelerated vector distance functions |
### 2.7 File and I/O

| Header | Public Type | Purpose |
|--------|------------|---------|
| `file_utils.h` | `FileUtils` | Portable file I/O utilities |
| `memory_utils.h` | `MemoryUtils` | Memory alignment and pool utilities |
| `unaligned_access.h` | `UnalignedAccess` | Safe unaligned memory read/write |
| `pointer_utils.h` | `PointerUtils` | Pointer arithmetic and alignment checks |
| `geometric_distances.h` | Distance helper functions | Euclidean and geospatial distance calculations |
### 2.8 Networking and RPC

| Header | Public Type | Purpose |
|--------|------------|---------|
| `grpc_channel_pool.h` | `GrpcChannelPool` | gRPC channel pool |
| `http_client_pool.h` | `HttpClientPool` | HTTP connection pool |
| `uuid.h` | `generate_uuid_v4()`, `generate_uuid_v7()` | UUID generation helpers |
| `timestamp_utils.h` | `TimestampUtils` | ISO 8601 timestamp parse/format |
### 2.9 Miscellaneous

| Header | Public Type | Purpose |
|--------|------------|---------|
| `expected.h` | `tl::expected<T, E>` | Expected-value type (third-party or in-header fallback) |
| `error_registry.h` | `ErrorRegistry` | Global error code registry |
| `retention_manager.h` | `RetentionManager` | Data retention policy enforcement |
| `self_awareness.h` | `SelfAwareness` | Runtime self-diagnostic awareness |
| `cursor.h` | `Cursor` | Generic pagination cursor |
| `update_checker.h` | `UpdateChecker` | Background update availability checker |
| `utils_adapters.h` | `UtilsAdapters` | Adapter shims for legacy utility consumers |
| `utils_interfaces.h` | Utility interface types | Streaming PII, audit, HKDF cache, logging, and pipeline contracts |
| `lek_manager.h` | `LEKManager` | Local encryption key manager |
| `capability_auto_generator.h` | `CapabilityAutoGenerator` | Automatic capability manifest generation |

---

## 3. Namespace Layout

Most APIs reside in `themis::utils`; some headers expose compatibility
namespace bridges. Geospatial APIs may use `themis::geo`. Check the specific
header for the declaration's actual namespace.

---

## 4. Contract Notes

- Headers in `include/utils/` define the consumer-facing declarations; internal implementations live in `src/utils/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- The utils module is cross-cutting infrastructure; no single domain-layer association is implied.
