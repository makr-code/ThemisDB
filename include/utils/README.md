> **Build:** `cmake --preset release && cmake --build build/release`

# Utils Module Headers

Cross-cutting utility headers used throughout ThemisDB.

## Purpose

Public interfaces and declarations for utils functionality.

## Header Reference

| Header | Key Types / Functions | Description |
|---|---|---|
| `audit_logger.h` | `AuditLogger` | Structured audit-trail logging |
| `batch_operation_manager.h` | `BatchOperationManager` | Batched async operation dispatch |
| `bloom_filter.h` | `BloomFilter` | Probabilistic membership filter |
| `capability_auto_generator.h` | `CapabilityAutoGenerator` | <!-- TODO: verify --> Runtime capability discovery |
| `checksum_utils.h` | `crc32()`, `sha256()` | Data integrity checksums |
| `clock.h` | `IClock`, `SystemClock` | Mockable wall-clock abstraction |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio / throughput stats |
| `concurrent_cache.h` | `ConcurrentCache` | Thread-safe LRU cache |
| `consistent_hash.h` | `ConsistentHash` | Consistent hashing ring for sharding |
| `cron_parser.h` | `CronParser` | Cron expression parsing and scheduling |
| `cursor.h` | `Cursor` | Pagination cursor abstraction |
| `error_registry.h` | `ErrorRegistry` | Centralised error code registry |
| `expected.h` | `Expected<T,E>` | Result / expected-value type |
| `file_utils.h` | `readFile()`, `writeFile()` | Portable file I/O helpers |
| `grpc_channel_pool.h` | `GrpcChannelPool` | gRPC channel pooling |
| `hash_util.h` | `hashBytes()`, `hashString()` | General-purpose hash utilities |
| `hkdf_cache.h` | `HkdfCache` | Cached HKDF key material |
| `hkdf_helper.h` | `HkdfHelper` | HKDF derive-key wrappers |
| `http_client_pool.h` | `HttpClientPool` | HTTP connection pooling |
| `input_validator.h` | `InputValidator` | Input sanitisation and validation |
| `json_helpers.h` | `parseJson()`, `serializeJson()` | JSON serialisation helpers |
| `lek_manager.h` | `LEKManager` | Local encryption key management |
| `logger.h` | `Logger`, `LogLevel` | Structured logging facade |
| `logger_impl.h` | `LoggerImpl` | Logger back-end implementation |
| `lossless_vector_compression.h` | `LosslessVectorCompressor` | Lossless float-vector compression |
| `lossless_vector_integration.h` | `LosslessVectorIntegration` | Integration helpers for vector compression |
| `lz4_codec.h` | `LZ4Codec` | LZ4 compress / decompress |
| `memory_utils.h` | `alignedAlloc()`, `secureZero()` | Memory allocation and zeroing |
| `ner_detection_engine.h` | `NERDetectionEngine` | Named-entity recognition engine |
| `normalizer.h` | `Normalizer` | Text and numeric normalisation |
| `openssl_deleter.h` | `OpenSSLDeleter` | RAII deleters for OpenSSL objects |
| `pii_detection_engine.h` | `PIIDetectionEngine` | PII detection orchestration |
| `pii_detector.h` | `PIIDetector` | Core PII pattern detector |
| `pii_pseudonymizer.h` | `PIIPseudonymizer` | Deterministic PII pseudonymisation |
| `pii_redacting_sink.h` | `PIIRedactingSink` | Log sink that redacts PII inline |
| `pki_client.h` | `PKIClient` | Certificate issuance / verification client |
| `pointer_utils.h` | `notNull()`, `downcast()` | Safe pointer utilities |
| `rate_limiter.h` | `RateLimiter` | Token-bucket rate limiter |
| `regex_detection_engine.h` | `RegexDetectionEngine` | Regex-based pattern detection |
| `retention_manager.h` | `RetentionManager` | Data retention policy enforcement |
| `safe_access.h` | `safeGet()`, `safeAt()` | Bounds-checked container access |
| `safe_arithmetic.h` | `safeAdd()`, `safeMul()` | Overflow-safe arithmetic |
| `safe_cast.h` | `safeCast<T>()` | Checked numeric casts |
| `saga_logger.h` | `SagaLogger` | SAGA step audit logging |
| `self_awareness.h` | `SelfAwareness` | <!-- TODO: verify --> Runtime self-diagnostics |
| `serialization.h` | `serialize()`, `deserialize()` | Generic binary serialisation |
| `simd_distance.h` | `simdCosine()`, `simdL2()` | SIMD-accelerated vector distance |
| `stemmer.h` | `Stemmer` | Language stemming (Porter / Snowball) |
| `stopwords.h` | `Stopwords` | Stop-word filter for text processing |
| `string_utils.h` | `trim()`, `split()`, `toLower()` | String manipulation utilities |
| `thread_pool_manager.h` | `ThreadPoolManager` | Named thread pool lifecycle management |
| `thread_safety.h` | `ThreadSafetyAnnotations` | Thread-safety annotations / GUARDED_BY |
| `timestamp_utils.h` | `nowMs()`, `formatTimestamp()` | Timestamp formatting and parsing |
| `tracing.h` | `Tracer`, `Span` | OpenTelemetry-compatible distributed tracing |
| `type_conversion.h` | `toProto()`, `fromProto()` | Type conversion helpers |
| `unaligned_access.h` | `readUnaligned<T>()` | Safe unaligned memory reads |
| `update_checker.h` | `UpdateChecker` | GitHub release update polling |
| `utils_adapters.h` | `UtilsAdapters` | <!-- TODO: verify --> Adapter helpers for utils interfaces |
| `utils_interfaces.h` | `IHasher`, `ISerializer` | Abstract utility interfaces |
| `uuid.h` | `generateUUID()` | RFC 4122 UUID generation |
| `zstd_codec.h` | `ZstdCodec` | Zstandard compress / decompress |

## Implementation

See `../../src/utils/` for the implementation code.

## Documentation

See `../../docs/src/utils/` for detailed module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

