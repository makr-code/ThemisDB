> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/utils/ARCHITECTURE.md -->

# Utilities Module — Public Header Architecture

**Module Path:** `include/utils/`  
**Implementation:** `../../src/utils/`  
**Canonical architecture doc:** [`../../src/utils/ARCHITECTURE.md`](../../src/utils/ARCHITECTURE.md)

---

## 1. Overview

`include/utils/` defines the **public cross-cutting utilities: logging, tracing, hashing, compression, PII detection/redaction, retry policies, thread pools, serialisation, SIMD distances, bloom filters, rate limiters, and gRPC helpers API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/utils/ARCHITECTURE.md`](../../src/utils/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Logging and Tracing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `logger.h` | `Logger` | Structured application logger |
| `logger_impl.h` | `LoggerImpl` | Logger backend implementation |
| `tracing.h` | `Tracing` | OpenTelemetry-compatible distributed tracing |
| `saga_logger.h` | `SagaLogger` | Saga transaction event logger |
| `audit_logger.h` | `AuditLogger` | Security/compliance audit logger |
| `pii_redacting_sink.h` | `PIIRedactingSink` | PII-redacting log sink |
### 2.2 Hashing and Checksums

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hash_util.h` | `HashUtil` | MurmurHash/xxHash utility wrappers |
| `checksum_utils.h` | `ChecksumUtils` | CRC32/SHA checksum utilities |
| `bloom_filter.h` | `BloomFilter` | Space-efficient probabilistic membership test |
| `consistent_hash.h` | `ConsistentHash` | Consistent hash ring for sharding |
| `hkdf_helper.h` | `HKDFHelper` | HMAC-based key derivation |
| `hkdf_cache.h` | `HKDFCache` | Cached HKDF derivations |
| `openssl_deleter.h` | `OpenSSLDeleter` | RAII OpenSSL object deleter |
### 2.3 Compression and Encoding

| Header | Public Type | Purpose |
|--------|------------|---------|
| `lz4_codec.h` | `LZ4Codec` | LZ4 fast compression codec |
| `zstd_codec.h` | `ZstdCodec` | Zstandard compression codec |
| `lossless_vector_compression.h` | `LosslessVectorCompression` | Lossless float vector compression |
| `lossless_vector_integration.h` | `LosslessVectorIntegration` | Vector compression integration helpers |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio and speed telemetry |
| `serialization.h` | `Serialization` | Binary/JSON serialisation utilities |
| `json_helpers.h` | `JsonHelpers` | JSON parse/query helpers |
| `type_conversion.h` | `TypeConversion` | Safe cross-type conversion utilities |
| `safe_cast.h` | `SafeCast` | Checked numeric cast utilities |
### 2.4 PII and Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `pii_detector.h` | `PIIDetector` | ML-based PII detection engine |
| `pii_detection_engine.h` | `PIIDetectionEngine` | Configurable PII detection pipeline |
| `pii_pseudonymizer.h` | `PIIPseudonymizer` | PII pseudonymisation with format preservation |
| `ner_detection_engine.h` | `NERDetectionEngine` | Named-entity recognition engine |
| `regex_detection_engine.h` | `RegexDetectionEngine` | Regex-based pattern detection engine |
| `input_validator.h` | `InputValidator` | General-purpose input validation |
| `safe_access.h` | `SafeAccess` | Bounds-checked container access |
| `safe_arithmetic.h` | `SafeArithmetic` | Overflow-safe arithmetic wrappers |
| `pki_client.h` | `PKIClient` | PKI certificate retrieval client |
### 2.5 Concurrency and Threading

| Header | Public Type | Purpose |
|--------|------------|---------|
| `thread_pool_manager.h` | `ThreadPoolManager` | Sized thread pool manager |
| `thread_safety.h` | `ThreadSafety` | Thread-safety assertion helpers |
| `concurrent_cache.h` | `ConcurrentCache` | Lock-free concurrent cache |
| `rate_limiter.h` | `RateLimiter` | Token-bucket rate limiter |
| `retry_policy.h` | `RetryPolicy` | Configurable retry with backoff |
| `clock.h` | `Clock` | Mockable clock interface for tests |
### 2.6 String and Text

| Header | Public Type | Purpose |
|--------|------------|---------|
| `string_utils.h` | `StringUtils` | UTF-8 string manipulation utilities |
| `normalizer.h` | `Normalizer` | Text normalisation pipeline |
| `stemmer.h` | `Stemmer` | Snowball stemmer wrapper |
| `stopwords.h` | `Stopwords` | Multi-language stopword lists |
| `simd_distance.h` | `SIMDDistance` | SIMD-accelerated vector distance functions |
### 2.7 File and I/O

| Header | Public Type | Purpose |
|--------|------------|---------|
| `file_utils.h` | `FileUtils` | Portable file I/O utilities |
| `memory_utils.h` | `MemoryUtils` | Memory alignment and pool utilities |
| `unaligned_access.h` | `UnalignedAccess` | Safe unaligned memory read/write |
| `pointer_utils.h` | `PointerUtils` | Pointer arithmetic and alignment checks |
| `geometric_distances.h` | `GeometricDistances` | Euclidean/cosine/dot distance functions |
### 2.8 Networking and RPC

| Header | Public Type | Purpose |
|--------|------------|---------|
| `grpc_channel_pool.h` | `GRPCChannelPool` | gRPC channel pool with load balancing |
| `http_client_pool.h` | `HTTPClientPool` | HTTP connection pool |
| `uuid.h` | `UUID` | RFC-4122 UUID generation |
| `timestamp_utils.h` | `TimestampUtils` | ISO 8601 timestamp parse/format |
### 2.9 Miscellaneous

| Header | Public Type | Purpose |
|--------|------------|---------|
| `expected.h` | `Expected` | std::expected-compatible error/value type |
| `error_registry.h` | `ErrorRegistry` | Global error code registry |
| `retention_manager.h` | `RetentionManager` | Data retention policy enforcement |
| `self_awareness.h` | `SelfAwareness` | Runtime self-diagnostic awareness |
| `cursor.h` | `Cursor` | Generic pagination cursor |
| `update_checker.h` | `UpdateChecker` | Background update availability checker |
| `utils_adapters.h` | `UtilsAdapters` | Adapter shims for legacy utility consumers |
| `utils_interfaces.h` | `UtilsInterfaces` | Utility interface contracts |
| `lek_manager.h` | `LEKManager` | Local encryption key manager |
| `capability_auto_generator.h` | `CapabilityAutoGenerator` | Automatic capability manifest generation |

---

## 3. Namespace Layout

All public types reside in the `themis::utils` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/utils/` expose the **stable public API**; internal types live in `src/utils/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph/ANN**.
