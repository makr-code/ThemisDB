---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/timeseries/; src/timeseries/FUTURE_ENHANCEMENTS.md"
---

> Exact model backend not verifiable from final document text alone.

<!-- Planning notes checked against current declarations: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/timeseries/FUTURE_ENHANCEMENTS.md -->

# Timeseries Module — Public Header Future Enhancements

**Module Path:** `include/timeseries/`
**Canonical implementation enhancements:** [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/timeseries/`. Runtime chunk lifecycle, compaction, WAL, and benchmark work remain tracked in:

→ [`../../src/timeseries/FUTURE_ENHANCEMENTS.md`](../../src/timeseries/FUTURE_ENHANCEMENTS.md)

---

## Design Considerations

The following are goals for future public-contract work, not claims that the
current headers satisfy them:

- Keep internal storage layout out of caller-facing contracts where practical.
- Keep codec implementation details separate from the selector interfaces
  (`ICompressionSelector` and `HeuristicCompressionSelector`).
- Keep key material behind caller-supplied key-provider boundaries.
- State refresh and staleness behavior precisely without implying a bounded
  lag unless an implementation and test establish one.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `Hypertable` API | `hypertable.h` | Callers using hypertable operations | Declared |
| `GorillaEncoder` / `GorillaDecoder` | `gorilla.h` | Codec callers | Declared |
| `ContinuousAggMaterializationEngine` | `continuous_agg.h` | Callers registering and refreshing named aggregates | Declared |
| `PromWriteRequest` decoder | `prometheus_remote_write.h` | Callers parsing remote-write payloads | Declared |
| `EncryptedChunkStore` | `encrypted_chunk_store.h` | Callers attaching encrypted chunk storage | Declared |

“Declared” confirms that a symbol exists in the named header; it is not a
binary-compatibility, production-readiness, or runtime-integration guarantee.

---

## Planned Enhancements

### Short-Term (Q3 2026)

These target dates are historical planning labels; completion or a current
commitment is not established by this header inventory.

- Document continuous-aggregate refresh-lag bounds and staleness guarantees uniformly.
- Clarify adaptive-flush back-pressure contract and overflow semantics for high-throughput ingest.
- Add SIMD-feature availability guards and scalar fallback notes to `gorilla_simd.h`.

### Medium-Term (Q4 2026)

- Introduce `timeseries_policy.h` combining retention, downsampling, and compression into a single declarative policy contract.
- Expose benchmark-reference throughput targets for Prometheus remote-write payload decoding and Gorilla encoding hot paths.
- Add deprecation annotations for any legacy chunk-storage APIs replaced by encrypted-chunk backends.

### Long-Term

- Unify streaming-cursor and continuous-aggregate consumption behind a shared time-series event-stream contract.
- Provide extension hooks for embedders to inject custom compression codecs alongside the built-in Gorilla/SIMD pipeline.
- Add temporal-aware query-explain output via `query_optimizer.h` to surface chunk pruning decisions to consumers.
