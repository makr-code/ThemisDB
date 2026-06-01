> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/timeseries/FUTURE_ENHANCEMENTS.md -->

# Timeseries Module — Public Header Future Enhancements

**Module Path:** `include/timeseries/`
**Canonical implementation enhancements:** [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/timeseries/`. Runtime chunk lifecycle, compaction, WAL, and benchmark work remain tracked in:

→ [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Hypertable and storage headers must not expose internal chunk layout to callers.
- `[x]` Compression headers must keep codec internals opaque; selection logic is behind `CompressionSelector`.
- `[x]` Encrypted-chunk headers must integrate key management with `include/security/` without leaking key material.
- `[x]` Continuous-aggregate headers must define bounded refresh-lag and staleness contracts.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `Hypertable` ingestion / partition API | `hypertable.h` | Ingest pipelines, query layer | ✅ Stable |
| `GorillaEncoder` / `GorillaDecoder` | `gorilla.h` | Chunk storage internals | ✅ Stable |
| `ContinuousAggregate` refresh API | `continuous_agg.h` | Dashboard and monitoring consumers | ✅ Stable |
| `PrometheusRemoteWrite` ingest API | `prometheus_remote_write.h` | Observability integrations | ✅ Stable |
| `EncryptedChunkStore` read/write | `encrypted_chunk_store.h` | Compliance storage backends | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document continuous-aggregate refresh-lag bounds and staleness guarantees uniformly.
- Clarify adaptive-flush back-pressure contract and overflow semantics for high-throughput ingest.
- Add SIMD-feature availability guards and scalar fallback notes to `gorilla_simd.h`.

### Medium-Term (Q4 2026)

- Introduce `timeseries_policy.h` combining retention, downsampling, and compression into a single declarative policy contract.
- Expose benchmark-reference throughput targets for Prometheus remote-write and Gorilla encoding hot paths.
- Add deprecation annotations for any legacy chunk-storage APIs replaced by encrypted-chunk backends.

### Long-Term

- Unify streaming-cursor and continuous-aggregate consumption behind a shared time-series event-stream contract.
- Provide extension hooks for embedders to inject custom compression codecs alongside the built-in Gorilla/SIMD pipeline.
- Add temporal-aware query-explain output via `query_optimizer.h` to surface chunk pruning decisions to consumers.
