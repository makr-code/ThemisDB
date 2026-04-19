<!-- Status: current | validated: 2026-04-19 -->

# Utils — Header Audit

**Last Audit:** 2026-04-19
**Status:** ✅ Pass
**Auditor:** Automated + Manual Review

## Summary

| Metric | Value |
|---|---|
| Total public headers (flat) | ~90 |
| Subdirectories | 2 (`geo/`, `memory/`) |
| Security-relevant headers | 10 |
| Headers with complete declarations | All reviewed headers |
| Critical findings | 0 |

## Header Files Audited

| File | Exported Symbols | Notes |
|---|---|---|
| `audit_logger.h` | `AuditLogger` | No secrets in log API; rate-limiting recommended |
| `bloom_filter.h` | `BloomFilter<T>` | Probabilistic; document false-positive rate |
| `checksum_utils.h` | checksum functions | Non-cryptographic checksums; do not use for auth |
| `clock.h` | `Clock`, `MonotonicClock` | Mockable; suitable for testing |
| `concurrent_cache.h` | `ConcurrentCache<K,V>` | Thread-safe; verify eviction policy |
| `error_registry.h` | `ErrorRegistry` | Centralized; avoid leaking internals in public errors |
| `expected.h` | `Expected<T,E>` | Result type; consistent error propagation |
| `hkdf_cache.h` | `HkdfCache` | Derived keys cached; review cache lifetime |
| `hkdf_helper.h` | `HkdfHelper` | HKDF-SHA256; verify salt entropy |
| `input_validator.h` | `InputValidator` | Sanitizes inputs; verify injection coverage |
| `lek_manager.h` | `LekManager` | LEK lifecycle; verify zeroization on destroy |
| `logger.h` / `logger_impl.h` | `Logger`, `LoggerImpl` | Ensure PII redaction via `pii_redacting_sink.h` |
| `memory_utils.h` | memory helpers | Verify `explicit_bzero` usage for sensitive buffers |
| `ner_detection_engine.h` | `NerDetectionEngine` | NER model loading; verify model path validation |
| `openssl_deleter.h` | `OpenSslDeleter` | RAII; prevents OpenSSL memory leaks |
| `pii_detection_engine.h` | `PiiDetectionEngine` | PII orchestrator; verify all entity types covered |
| `pii_detector.h` | `PiiDetector` | Pattern matching; verify regex DoS resistance |
| `pii_pseudonymizer.h` | `PiiPseudonymizer` | Deterministic pseudonymization; verify reversibility controls |
| `pii_redacting_sink.h` | `PiiRedactingSink` | Redacts before log emission; verify sink ordering |
| `rate_limiter.h` | `RateLimiter` | Token bucket; verify clock dependency |
| `safe_access.h` | `safe_at`, `safe_deref` | Bounds checking; no UB on out-of-range |
| `safe_arithmetic.h` | `safe_add`, `safe_mul` | Overflow-safe; verify all numeric paths |
| `safe_cast.h` | `safe_cast<T>` | Checked cast; throws/returns error on overflow |
| `simd_distance.h` | `SimdDistance` | SIMD + scalar fallback; verify alignment requirements |
| `thread_safety.h` | `ReadWriteLock` | RAII guards; verify no deadlock patterns |
| `tracing.h` | `Tracer`, `Span` | OTel-compatible; ensure span context propagation |
| `uuid.h` | `Uuid` | v4 UUID; verify CSPRNG source |
| `zstd_codec.h` | `ZstdCodec` | ZSTD interface; verify decompression size limits |
| `batch_operation_manager.h` | `BatchOperationManager` | Batch operation management and chunking |
| `capability_auto_generator.h` | `CapabilityAutoGenerator` | Capability auto-generation utilities |
| `compression_metrics.h` | `CompressionMetrics` | Compression ratio metrics |
| `consistent_hash.h` | `ConsistentHash` | Consistent hashing for distributed routing |
| `cron_parser.h` | `CronParser` | Cron expression parser |
| `cursor.h` | `Cursor` | Cursor abstraction for paginated results |
| `file_utils.h` | file utility functions | File I/O utility functions |
| `grpc_channel_pool.h` | `GrpcChannelPool` | gRPC channel pool management |
| `hash_util.h` | hash utility functions | Hash utility functions |
| `http_client_pool.h` | `HttpClientPool` | HTTP client connection pool |
| `json_helpers.h` | JSON helper functions | JSON parsing and serialization helpers |
| `lossless_vector_compression.h` | `LosslessVectorCompression` | Lossless vector compression codec |
| `lossless_vector_integration.h` | `LosslessVectorIntegration` | Lossless vector compression integration |
| `lz4_codec.h` | `Lz4Codec` | LZ4 compression codec |
| `normalizer.h` | `Normalizer` | Text and data normalization utilities |
| `pki_client.h` | `PkiClient` | PKI client for certificate management |
| `pointer_utils.h` | pointer utility functions | Safe pointer utilities |
| `regex_detection_engine.h` | `RegexDetectionEngine` | Regex-based pattern detection engine |
| `retention_manager.h` | `RetentionManager` | Data retention policy manager |
| `saga_logger.h` | `SagaLogger` | SAGA transaction logger |
| `self_awareness.h` | `SelfAwareness` | System self-monitoring and introspection |
| `serialization.h` | serialization utilities | Generic serialization/deserialization |
| `stemmer.h` | `Stemmer` | Text stemming utilities |
| `stopwords.h` | `Stopwords` | Stopword list management |
| `string_utils.h` | string utility functions | String manipulation utilities |
| `thread_pool_manager.h` | `ThreadPoolManager` | Thread pool lifecycle manager |
| `timestamp_utils.h` | timestamp utility functions | Timestamp conversion and formatting utilities |
| `type_conversion.h` | type conversion utilities | Type conversion utilities |
| `unaligned_access.h` | unaligned access helpers | Safe unaligned memory access helpers |
| `update_checker.h` | `UpdateChecker` | Update availability checker |
| `utils_adapters.h` | adapter utilities | Utility adapter patterns |
| `utils_interfaces.h` | utility interfaces | Utility interface abstractions |

## Findings

- **FINDING-UTILS-01 (Info):** `pii_detector.h` uses regex patterns; recommend audit for ReDoS-susceptible patterns.
- **FINDING-UTILS-02 (Info):** `uuid.h` must source randomness from a CSPRNG (e.g., `/dev/urandom` or OpenSSL); verify in implementation.
- **FINDING-UTILS-03 (Info):** `zstd_codec.h` should enforce a maximum decompression output size to prevent decompression bomb attacks.
- No critical or high findings. Next audit recommended after v1.6.0 release.
