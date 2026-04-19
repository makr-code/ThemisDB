> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Utils Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/utils/`

---

## 1. Overview

The Utils module provides shared utility components used across all ThemisDB modules:
audit logging, PII detection and pseudonymization, cron expression parsing, HKDF key
derivation, text normalization, stemming, zstd compression, distributed tracing, SIMD
distance computation, gRPC channel pooling, HTTP client pooling, license gating, and more.

---

## 2. Design Principles

- **No Business Logic** – utils components are pure utilities with no knowledge of
  ThemisDB domain objects or module-specific semantics.
- **Thread-Safe by Default** – all utils components are safe for concurrent invocation
  unless explicitly documented otherwise.
- **Composable** – components are independent; no intra-utils dependencies where avoidable.
- **Zero-Allocation Hot Paths** – SIMD distance computation and compression use
  stack-local buffers to avoid heap allocation on the hot path.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `audit_logger.cpp` | Structured audit trail with tamper-evident chain |
| `pii_detector.cpp` | PII detection (email, phone, SSN, credit card) |
| `pii_detection_engine.cpp` | Detection engine: regex + NER |
| `pii_pseudonymizer.cpp` | Pseudonymization and masking |
| `ner_detection_engine.cpp` | Named entity recognition for PII |
| `regex_detection_engine.cpp` | Regex-based PII pattern matching |
| `cron_parser.cpp` | Full cron expression parser (6-field, @-specials, TZ-aware) |
| `hkdf_helper.cpp` / `hkdf_cache.cpp` | HKDF key derivation (RFC 5869) + caching |
| `lek_manager.cpp` | Local Encryption Key lifecycle management |
| `logger.cpp` | Structured logger (wraps spdlog) |
| `tracing.cpp` | Distributed trace context helpers |
| `zstd_codec.cpp` (inferred) | ZSTD compression/decompression |
| `compression_metrics.cpp` | Compression ratio and throughput metrics |
| `cursor.cpp` | Pagination cursor serialization/deserialization |
| `serialization.cpp` | JSON/MessagePack serialization helpers |
| `normalizer.cpp` | Text normalization (unicode, whitespace, case) |
| `stemmer.cpp` | Text stemmer (Porter, Snowball) |
| `stopwords.cpp` | Stop word list management |
| `simd_distance.cpp` | SIMD-accelerated L2/cosine distance |
| `input_validator.cpp` | Input validation (length, charset, injection patterns) |
| `grpc_channel_pool.cpp` | gRPC channel pool for efficient re-use |
| `http_client_pool.cpp` | HTTP client connection pool |
| `thread_pool_manager.cpp` | Shared thread pool manager |
| `file_utils.cpp` | Filesystem utilities (safe read/write, atomic rename) |
| `error_registry.cpp` | Structured error code registry |
| ~~`build_info.cpp`~~ | Migrated to `src/themis/build_info.cpp` (v1.7.0) |
| ~~`license_info.cpp`~~ | Migrated to `src/themis/license_info.cpp` (v1.7.0) |
| `runtime_license_gate.cpp` | Per-feature license gate at runtime |
| `capability_auto_generator.cpp` | Automatic capability manifest generation |
| `saga_logger.cpp` | SAGA step logging for distributed transactions |
| `retention_manager.cpp` | Generic retention policy enforcement |
| `self_awareness.cpp` | Runtime introspection (loaded modules, resource usage) |
| `geo/` | Geospatial utility functions |
| `memory/` | Memory utilities (aligned alloc, pool, secure zero) |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    All ThemisDB Modules                         │
│   audit_logger.log(...) / pii_detector.scan(text) / etc.        │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     Utils Module                                 │
│  (pure utilities, no domain logic, no module dependencies)      │
│                                                                  │
│  audit_logger │ pii_detector │ cron_parser │ hkdf_helper        │
│  compression  │ cursor       │ serializer  │ simd_distance      │
│  stemmer      │ normalizer   │ tracing     │ input_validator     │
│  grpc_pool    │ http_pool    │ thread_pool │ file_utils          │
│  logger       │ lek_manager  │ saga_logger │ retention_manager   │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 PII Detection Pipeline

```
pii_detector.scan(text: "Contact: john@example.com, +1-555-1234")
    │
    ├─ regex_detection_engine: email pattern → "john@example.com" (confidence: 0.99)
    ├─ regex_detection_engine: phone pattern → "+1-555-1234" (confidence: 0.95)
    ├─ ner_detection_engine: NER → "John" (PERSON, confidence: 0.87)
    │
    └─ return [{field: "email", value: "john@example.com"},
               {field: "phone", value: "+1-555-1234"},
               {field: "name", value: "John"}]
```

### 4.2 SIMD Distance Computation

```
simd_distance.L2(vec_a[1536], vec_b[1536])
    │
    ├─ check CPU feature: AVX-512 → 512-bit SIMD path
    │   or AVX2 → 256-bit SIMD path
    │   or fallback → scalar loop
    │
    └─ return L2 distance (float)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Used by** | All modules | Shared utilities |
| **Provides** | `src/scheduler/` | `cron_parser.cpp` |
| **Provides** | `src/security/` | `hkdf_helper.cpp`, `lek_manager.cpp` |
| **Provides** | `src/governance/` | `audit_logger.cpp` |
| **Provides** | `src/index/` | `simd_distance.cpp` |
| **Provides** | `src/query/` | `cursor.cpp`, `input_validator.cpp` |

---

## 6. Threading & Concurrency Model

- All utils components are designed to be thread-safe.
- `audit_logger` uses a lock-free append queue.
- `pii_detection_engine` is stateless; safe for concurrent invocation.
- `hkdf_cache` uses a read-write lock.
- `simd_distance` is fully reentrant; uses only stack-local buffers.
- `grpc_channel_pool` and `http_client_pool` use internal pool mutexes.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| SIMD distance | AVX-512/AVX2 acceleration for L2/cosine on hot vector search path |
| HKDF cache | Derived keys cached; avoids HMAC recomputation per access |
| Audit logger | Lock-free queue; audit writes never block the caller |
| Connection pools | gRPC/HTTP connection reuse eliminates handshake overhead |

---

## 8. Security Considerations

- `hkdf_helper.cpp` uses OpenSSL's HMAC for FIPS-compliant key derivation.
- `lek_manager.cpp` stores derived keys in `mlock`-protected memory.
- `audit_logger.cpp` appends entries to an append-only log; no modification.
- `input_validator.cpp` rejects injection patterns (SQL, AQL, command injection).
- `pii_pseudonymizer.cpp` uses deterministic pseudonymization (HMAC-SHA256 of PII value
  with a tenant-specific key) for referential integrity.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `utils.audit_logger.path` | "logs/audit.log" | Audit log file path |
| `utils.pii.enabled` | true | Enable PII detection |
| `utils.compression.level` | 3 | zstd compression level |
| `utils.thread_pool.size` | cpu_count | Shared thread pool size |
| `utils.hkdf_cache.size` | 1024 | HKDF key cache capacity |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Audit log write failure | Log to stderr; do not crash |
| PII detection error | Return empty results; log error |
| SIMD detection failure | Fall back to scalar implementation |
| Connection pool exhausted | Return error to caller; do not wait indefinitely |

---

## 11. Known Limitations & Future Work

- Streaming PII detection pipeline for large documents is in progress.
- Tamper-evident audit chain (Merkle tree log) is planned.
- WASM-based sandboxed PII detection for untrusted content is planned.

---

## 12. References

- `src/utils/README.md` — module overview
- `docs/src/utils/` — per-utility documentation
- `ARCHITECTURE.md` (root) — full system architecture
