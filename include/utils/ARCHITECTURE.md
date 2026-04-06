<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/utils/ -->

# Utils — Public Header Architecture

## Overview
`include/utils/` is ThemisDB's general-purpose utility layer. It provides ~60 headers spanning cryptography helpers, PII detection/pseudonymization, SIMD-accelerated vector operations, safe arithmetic, threading primitives, observability, and codec interfaces. All headers are consumed by other ThemisDB modules; implementation lives in `../../src/utils/`.

## Design Principles
1. **Zero-cost abstractions** — headers expose thin interfaces; hot paths use SIMD/HKDF without virtual dispatch.
2. **Safety by default** — `safe_arithmetic.h`, `safe_cast.h`, `safe_access.h` prevent UB in common numeric and pointer operations.
3. **Privacy-first** — PII pipeline (`pii_detection_engine.h`, `pii_detector.h`, `pii_pseudonymizer.h`, `pii_redacting_sink.h`) is first-class, not an afterthought.
4. **Observability built-in** — `tracing.h`, `audit_logger.h`, `saga_logger.h` are available to all modules.
5. **Portability** — SIMD paths (`simd_distance.h`) provide scalar fallbacks; `unaligned_access.h` handles alignment portably.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `audit_logger.h` | `AuditLogger` | Structured audit event emission |
| `batch_operation_manager.h` | `BatchOperationManager` | Batched async operation scheduling |
| `bloom_filter.h` | `BloomFilter<T>` | Space-efficient probabilistic membership |
| `capability_auto_generator.h` | `CapabilityAutoGenerator` | Automatic capability token generation |
| `checksum_utils.h` | checksum helpers | CRC/hash checksum utilities |
| `clock.h` | `Clock`, `MonotonicClock` | Mockable clock abstraction |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio and latency tracking |
| `concurrent_cache.h` | `ConcurrentCache<K,V>` | Thread-safe LRU cache |
| `consistent_hash.h` | `ConsistentHashRing` | Consistent hashing for shard routing |
| `cron_parser.h` | `CronParser` | Cron expression parser/scheduler |
| `cursor.h` | `Cursor<T>` | Generic paginated result cursor |
| `error_registry.h` | `ErrorRegistry` | Centralized error code registry |
| `expected.h` | `Expected<T,E>` | Result type (error-or-value) |
| `file_utils.h` | file helpers | Path, temp file, directory utilities |
| `geo/` | geo utilities | Geospatial helper headers (subdirectory) |
| `grpc_channel_pool.h` | `GrpcChannelPool` | Pooled gRPC channel management |
| `hkdf_cache.h` | `HkdfCache` | Cached HKDF derived-key storage |
| `hkdf_helper.h` | `HkdfHelper` | HKDF-SHA256 key derivation |
| `http_client_pool.h` | `HttpClientPool` | Pooled HTTP client connections |
| `input_validator.h` | `InputValidator` | Input sanitization and validation |
| `lek_manager.h` | `LekManager` | Local Encryption Key management |
| `logger.h` | `Logger` | Structured logging interface |
| `logger_impl.h` | `LoggerImpl` | Logger implementation detail header |
| `lossless_vector_compression.h` | `LosslessVectorCompressor` | Lossless float-vector compression |
| `lossless_vector_integration.h` | integration helpers | Integration adapter for vector compression |
| `memory/` | memory utilities | Memory pool/allocator headers (subdirectory) |
| `memory_utils.h` | memory helpers | Aligned alloc, zeroization helpers |
| `ner_detection_engine.h` | `NerDetectionEngine` | Named-entity recognition for PII |
| `normalizer.h` | `Normalizer` | Text normalization (Unicode, case) |
| `openssl_deleter.h` | `OpenSslDeleter` | RAII deleters for OpenSSL types |
| `pii_detection_engine.h` | `PiiDetectionEngine` | PII detection orchestrator |
| `pii_detector.h` | `PiiDetector` | Low-level PII pattern detector |
| `pii_pseudonymizer.h` | `PiiPseudonymizer` | Deterministic PII pseudonymization |
| `pii_redacting_sink.h` | `PiiRedactingSink` | Log sink that redacts PII before emission |
| `pki_client.h` | `PkiClient` | PKI certificate retrieval client |
| `pointer_utils.h` | pointer helpers | Safe pointer arithmetic utilities |
| `rate_limiter.h` | `RateLimiter` | Token-bucket rate limiter |
| `regex_detection_engine.h` | `RegexDetectionEngine` | Regex-based PII pattern engine |
| `retention_manager.h` | `RetentionManager` | Data retention policy enforcement |
| `safe_access.h` | `safe_at`, `safe_deref` | Bounds-checked container/pointer access |
| `safe_arithmetic.h` | `safe_add`, `safe_mul`, … | Overflow-safe arithmetic operations |
| `safe_cast.h` | `safe_cast<T>` | Checked numeric type casts |
| `saga_logger.h` | `SagaLogger` | Distributed saga transaction logger |
| `self_awareness.h` | `SelfAwareness` | Runtime capability/health introspection |
| `serialization.h` | serialization helpers | Binary/JSON serialization utilities |
| `simd_distance.h` | `SimdDistance` | SIMD-accelerated L2/cosine distance |
| `stemmer.h` | `Stemmer` | Text stemming (Porter/Snowball) |
| `stopwords.h` | `Stopwords` | Stopword list management |
| `string_utils.h` | string helpers | Split, trim, format, encode utilities |
| `thread_pool_manager.h` | `ThreadPoolManager` | Configurable thread-pool lifecycle |
| `thread_safety.h` | `ReadWriteLock`, guards | Thread-safety primitives and RAII guards |
| `timestamp_utils.h` | timestamp helpers | ISO-8601 parse/format, epoch conversion |
| `tracing.h` | `Tracer`, `Span` | OpenTelemetry-compatible tracing |
| `type_conversion.h` | type conversion helpers | Safe type conversion utilities |
| `unaligned_access.h` | `load_unaligned<T>` | Portable unaligned memory reads |
| `update_checker.h` | `UpdateChecker` | Version/update availability checker |
| `utils_adapters.h` | adapter helpers | Adapter utilities for utils interfaces |
| `utils_interfaces.h` | base interfaces | Common base interfaces for utils layer |
| `uuid.h` | `Uuid` | UUID v4 generation and parsing |
| `zstd_codec.h` | `ZstdCodec` | ZSTD compress/decompress interface |

## Subsystem Groups
```
Cryptography:   hkdf_helper.h, hkdf_cache.h, openssl_deleter.h, lek_manager.h
PII Pipeline:   pii_detection_engine.h, pii_detector.h, pii_pseudonymizer.h,
                pii_redacting_sink.h, ner_detection_engine.h, regex_detection_engine.h
SIMD / Vector:  simd_distance.h, lossless_vector_compression.h, lossless_vector_integration.h
Safety:         safe_arithmetic.h, safe_cast.h, safe_access.h, pointer_utils.h
Threading:      thread_safety.h, thread_pool_manager.h, concurrent_cache.h
Observability:  tracing.h, audit_logger.h, saga_logger.h, logger.h
Codecs:         zstd_codec.h, compression_metrics.h
```

Implementation in `../../src/utils/`
