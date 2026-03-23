<!-- Status: current | validated: 2026-03-22 -->

# Security — include/timeseries/

Scope: security properties as expressed through the **public API**.
Implementation-level details: `../../src/timeseries/SECURITY.md`.

---

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Chunk data exfiltration from storage | Confidentiality loss | AES-256-GCM per chunk (`encrypted_chunk_store.h`); keys never in plaintext headers |
| GCM nonce/IV reuse | Catastrophic confidentiality failure | `EncryptedChunkStore` enforces IV uniqueness via atomic counter + random salt; reuse is a hard error |
| Stale key material after rotation | Long-lived ciphertext compromise | `KeyRotationManager` re-encrypts in-place; old key zeroed post-rotation |
| Unauthorised write to chunk store | Data integrity violation | `ChunkKey` required on every write; no unauthenticated overload exists |
| Unbounded buffer growth / DoS | OOM crash | `TSAutoBufferAdaptive` enforces `max_capacity`; full writes return `BufferFullError` |
| Prometheus SSRF via crafted endpoint | Metrics exfiltration | `RemoteWriteConfig` endpoint validated at construction; no runtime URL substitution |
| Malformed Gorilla SIMD bitstream | Memory corruption / UB | `GorillaSIMDDecoder` validates frame header and length before decode; returns `DecodeError` |
| Retention policy bypass | Unbounded storage growth | `RetentionExecutor` enforces time-based and size-based limits; override requires elevated privilege |
| Aggregate refresh poisoning | Incorrect aggregates served | `ContinuousAggregate` uses monotonic epoch fence; stale materialisation is rejected |
| Error message information leakage | Key ID / path disclosure | All public error types use opaque error codes; human-readable messages omit key material |

---

## Security Controls

### Encryption at Rest
- **Algorithm:** AES-256-GCM (AEAD), per-chunk granularity (default 64 MiB)
- **IV policy:** 96-bit IV; lower 64 bits are a monotonically-incrementing counter, upper 32 bits are a random salt per `EncryptedChunkStore` instance
- **Authentication:** 128-bit GCM tag verified on every read; tampered chunks raise `IntegrityError`

### Key Rotation
- Online rotation via `KeyRotationManager::rotate()` — no downtime
- Old key material zeroed in memory after successful re-encryption
- Key version stored in chunk metadata; `TSStore` routes decryption automatically

### Input Validation
- `GorillaDecoder` / `GorillaSIMDDecoder` validate frame magic, version, and length before processing
- `QueryOptimizer` rejects null-column-reference predicates before plan construction
- `PrometheusRemoteWriter` validates protobuf schema on ingest; malformed series are dropped

### Memory Safety
- All variable-length APIs use `std::span` or explicit-length caller-supplied buffers
- `TSAutoBufferAdaptive` bounds capacity with `std::numeric_limits` overflow guards

---

## Known Limitations

1. **No field-level encryption** — encryption is chunk-granular; finer granularity planned for v2.0.
2. **Key material in process memory** — HSM integration on the roadmap (Target: Q4 2026).
3. **Prometheus TLS** — TLS delegated to caller's HTTP client; `RemoteWriteConfig` does not enforce TLS by default in development builds. Production deployments **must** configure TLS.
4. **SIMD decoder alignment** — AVX2 paths use unaligned 256-bit loads; input buffers must be at least 32 bytes larger than the declared frame length on strict-alignment platforms.
5. **Retention grace period** — data in the grace period is soft-deleted; hard purge requires explicit `RetentionExecutor::purge_immediate()`.

---

> Vulnerability reports: `/SECURITY.md` at repository root.
> Implementation details: `../../src/timeseries/SECURITY.md`
