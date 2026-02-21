# Timeseries Module - Future Enhancements

## Scope

This document covers planned enhancements to ThemisDB's time series storage subsystem, which provides append-optimised storage via `tsstore.h`/`tsstore.cpp`, Gorilla delta-of-delta compression (`gorilla.cpp`), continuous aggregation (`continuous_agg.cpp`), configurable retention management (`retention.cpp`), hypertable partitioning (`hypertable.cpp`), and the `TSAutoBuffer` (`ts_auto_buffer.cpp`) for auto-batching high-frequency single-point inserts. The module is in Beta state and requires improved query performance, tighter integration with the downsampling pipeline, and hardened compression paths before GA.

## Design Constraints

- The `tsstore` write path must sustain ≥500k data points per second per node on commodity NVMe hardware without exceeding 10% CPU overhead.
- Gorilla compression must be transparent to callers; `tsstore.h` consumers must not need to decompress chunks manually.
- Retention policies executed by `retention.cpp` must be atomic at the chunk boundary — partial chunk deletion is not permitted.
- `TSAutoBuffer` must not buffer data for longer than its configured flush interval even under backpressure from the storage layer; overdue flushes must emit a metrics alert via `timeseries_metrics.cpp`.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `TSStore::insert_batch(points)` | `ts_auto_buffer.cpp`, ingestion module | Atomic batch; returns sequence number |
| `TSStore::scan(series_id, start, end)` | `query_optimizer.cpp`, analytics module | Returns compressed chunks; caller decodes |
| `ContinuousAgg::refresh(agg_id)` | `aggregate_scheduler.cpp`, `aggregate_scheduler_helper.cpp` | Incremental refresh from watermark |
| `RetentionManager::apply_policies()` | `retention.cpp`, scheduler module | Chunk-granular; must be idempotent |
| `Hypertable::partition(time_column)` | `hypertable.cpp` | Configures time-dimension chunk interval |
| `GorillaCoder::encode() / decode()` | `gorilla.cpp`, `tsstore.cpp` | In-place chunk compression/decompression |

## Planned Features

### [ ] Vectorised Gorilla Chunk Decoder with SIMD
**Priority:** High
**Target Version:** v0.9.0

Rewrite the `gorilla.cpp` decode path to use SIMD intrinsics (AVX2 on x86-64, NEON on ARM) for delta-of-delta reconstruction, dramatically increasing scan throughput for range queries over long time windows.

**Implementation Notes:**
- Add `gorilla_simd.cpp` alongside `gorilla.cpp` with AVX2 and NEON implementations selected via CMake feature detection; `gorilla.cpp` dispatches at runtime via CPUID check.
- `utils/simd_distance.cpp` already contains AVX2 helper patterns; reuse the lane-shuffle utilities for bit-unpacking the Gorilla XOR stream.
- `query_optimizer.cpp` should hint the expected decode width (float32 vs. float64) to the decoder to allow width-specific vectorisation paths.
- Benchmark with the existing `benchmarks/` harness; compare against the scalar baseline from `gorilla.cpp`.

**Performance Targets:**
- Gorilla decode throughput: >2 GB/s of decoded data per core (up from ~400 MB/s scalar).
- Range scan over 1M points (float64): <50 ms P99 including chunk fetch from `tsstore.cpp`.

---

### [ ] Incremental Continuous Aggregation with Watermark Pushdown
**Priority:** High
**Target Version:** v0.9.0

Extend `continuous_agg.cpp` to support watermark-based incremental refresh so that only newly ingested data since the last refresh is re-aggregated. The watermark is tracked per aggregate in the metadata layer and pushed down to `tsstore.cpp` scan predicates to skip already-processed chunks.

**Implementation Notes:**
- Add a `ContinuousAggWatermark` table to the metadata store; `continuous_agg.cpp::refresh()` reads the watermark, scans only `[watermark, now)` in `tsstore.cpp`, and advances the watermark atomically after a successful aggregate write.
- `aggregate_scheduler.cpp` must persist per-aggregate state including watermark to survive node restarts; use the WAL path from `tsstore.cpp` for durability.
- `aggregate_scheduler_helper.cpp` should expose a `backfill_range(agg_id, start, end)` method for manual recovery from gaps in watermark history.
- Emit aggregate refresh latency and lag metrics from `timeseries_metrics.cpp` tagged with `agg_id`.

**Performance Targets:**
- Incremental refresh overhead: <500 ms per aggregate per 1-minute interval under 100k inserts/s ingest rate.
- Watermark write amplification: <1.5× (aggregate write bytes / raw data bytes processed).

---

### [ ] Multi-Tier Downsampling Pipeline
**Priority:** Medium
**Target Version:** v0.10.0

Implement a configurable multi-tier downsampling pipeline (raw → 1 min → 1 hour → 1 day) integrated with `continuous_agg.cpp` and governed by `retention.cpp` policies. Each tier is stored in its own `hypertable.cpp` partition with tier-specific Gorilla compression settings.

**Implementation Notes:**
- Add `DownsamplingPolicy` configuration to `retention.cpp` that declares tier resolutions and retention durations; `hypertable.cpp` auto-provisions per-tier tables at policy creation time.
- `continuous_agg.cpp` executes downsampling as a watermark-driven aggregate (see incremental refresh feature above), computing min/max/avg/sum/count per downsampling window.
- Reads from `query_optimizer.cpp` must be routed to the coarsest tier that satisfies the query's time granularity; add a `TierSelector` in `query_optimizer.cpp` that compares requested resolution against available tiers.
- Retention expiry of raw data must not leave gaps in coarser tiers; `retention.cpp` must enforce that the target tier is fully populated before deleting raw chunks.

**Performance Targets:**
- Downsampling throughput: >10M raw points/s reduced to 1-min aggregates on a single node.
- Storage reduction from raw to 1-day tier: >50× for typical sensor/metric workloads.

---

### [~] TSAutoBuffer Adaptive Flush with Backpressure Signalling
**Priority:** High
**Target Version:** v0.9.0

Enhance `ts_auto_buffer.cpp` to dynamically adjust the flush batch size based on downstream `tsstore.cpp` write latency feedback, implementing a feedback-control loop that prevents buffer overruns without requiring manual tuning of the flush interval.

**Implementation Notes:**
- Add a `FlushController` class to `ts_auto_buffer.cpp` that maintains an EWMA of recent `tsstore.cpp` write latencies and scales the target batch size inversely with latency.
- If `tsstore.cpp` write latency exceeds a configurable SLO threshold (default 50 ms), `TSAutoBuffer` must emit a `ts_autobuffer_backpressure` counter via `timeseries_metrics.cpp` and block producers until the queue drains below the low-water mark.
- Ensure timer-based flush still fires at the configured maximum interval even when adaptive sizing is active, satisfying the constraint that data must not be held longer than the flush interval.
- `FlushController` state (EWMA, current batch size) must be exposed as runtime metrics for observability.

**Performance Targets:**
- Sustained single-point ingest throughput through `TSAutoBuffer`: >500k points/s per node.
- Buffer-to-storage flush latency P99: <10 ms under normal load.
- Backpressure event rate during sustained overload: <1 event/s (adaptive batching absorbs bursts).

---

### [ ] Chunk-Level Encryption at Rest
**Priority:** Medium
**Target Version:** v1.0.0

Add AES-256-GCM encryption to individual time series chunks in `tsstore.cpp` using data encryption keys derived by `utils/hkdf_helper.cpp` and managed by `utils/lek_manager.cpp`. Encryption must be transparent to the query path; chunks are decrypted on-demand during scan.

**Implementation Notes:**
- Add `EncryptedChunkStore` wrapper in `tsstore.cpp` that intercepts chunk write/read operations and applies AES-256-GCM using keys fetched from `utils/lek_manager.cpp` by series ID.
- Key rotation must re-encrypt affected chunks in a background job without blocking reads; implement in `ts_encrypted_key_rotation.cpp`.
- Gorilla-compressed data must be encrypted after compression (compress-then-encrypt) to maximise compression ratio and avoid information leakage from cipher input patterns.
- Audit every key access via `utils/audit_logger.cpp` with series ID, chunk range, and accessor identity.

**Performance Targets:**
- Encryption overhead on write path: <5% throughput reduction vs. unencrypted baseline.
- AES-256-GCM throughput per core: >1 GB/s (AES-NI assisted via OpenSSL EVP).

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >85% new code | Cover `GorillaSIMD` decode, `ContinuousAggWatermark`, `FlushController`, `TierSelector` |
| Integration | Full write → aggregate → query → retention cycle | Use realistic 1-hour dataset with 100k series |
| Performance | P99 < budgets above | Gorilla SIMD decode bench, `TSAutoBuffer` throughput under backpressure |
| Correctness | Gorilla encode/decode round-trip | Fuzz `gorilla.cpp` with property-based tests; verify lossless for float64 |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| Write throughput per node | ~200k pts/s | >500k pts/s | `TSAutoBuffer` adaptive flush benchmark |
| Gorilla decode throughput | ~400 MB/s | >2 GB/s | SIMD decoder microbenchmark |
| Range scan (1M pts, float64) | ~300 ms | <50 ms | SIMD decode + `query_optimizer.cpp` tier selection |
| Continuous agg refresh latency | ~5 s | <500 ms | Incremental watermark refresh benchmark |
| Storage compression ratio (Gorilla) | ~4× | >6× (with multi-tier downsampling) | Dataset comparison on real sensor traces |
| Chunk encryption overhead | N/A | <5% write throughput | AES-NI benchmark vs. plaintext baseline |

## Security / Reliability

- [ ] Chunk-level AES-256-GCM encryption keys must be managed exclusively through `utils/lek_manager.cpp`; hard-coded or environment-variable keys are prohibited.
- [ ] `retention.cpp` chunk deletion must be atomic at the chunk boundary and logged to `utils/audit_logger.cpp`; partially deleted chunks must be detected and repaired on startup.
- [ ] The Gorilla SIMD decoder must validate chunk magic bytes and version headers before decoding to prevent corrupt chunk data from causing undefined behaviour in the SIMD path.
- [?] Determine whether time series data containing legal event timestamps must be retained for a minimum period regardless of configured retention policy (regulatory constraint).
- [ ] `TSAutoBuffer` must not silently drop data under extreme backpressure; instead it must surface a `BUFFER_FULL` error to the caller so the ingestion layer can apply back-pressure upstream.
