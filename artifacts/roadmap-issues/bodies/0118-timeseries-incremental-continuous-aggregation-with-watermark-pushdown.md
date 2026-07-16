### Context

This issue implements the roadmap item 'Incremental Continuous Aggregation with Watermark Pushdown' for the timeseries domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: [ ] Incremental Continuous Aggregation with Watermark Pushdown

### Goal

Deliver the scoped changes for Incremental Continuous Aggregation with Watermark Pushdown in src/timeseries/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Add a `ContinuousAggWatermark` table to the metadata store; `continuous_agg.cpp::refresh()` reads the watermark, scans only `[watermark, now)` in `tsstore.cpp`, and advances the watermark atomically after a successful aggregate write.
- [ ] `aggregate_scheduler.cpp` must persist per-aggregate state including watermark to survive node restarts; use the WAL path from `tsstore.cpp` for durability.
- [ ] `aggregate_scheduler_helper.cpp` should expose a `backfill_range(agg_id, start, end)` method for manual recovery from gaps in watermark history.
- [ ] Emit aggregate refresh latency and lag metrics from `timeseries_metrics.cpp` tagged with `agg_id`.
- [ ] Incremental refresh overhead: <500 ms per aggregate per 1-minute interval under 100k inserts/s ingest rate.
- [ ] Watermark write amplification: <1.5× (aggregate write bytes / raw data bytes processed).

### Relationships

- Roadmap row: #118 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/timeseries/FUTURE_ENHANCEMENTS.md#-incremental-continuous-aggregation-with-watermark-pushdown
- Source key: roadmap:118:timeseries:v1.6.0:incremental-continuous-aggregation-with-watermark-pushdown

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:118:timeseries:v1.6.0:incremental-continuous-aggregation-with-watermark-pushdown -->
<!-- roadmap-ref: row=118;module=timeseries;target=v1.6.0 -->
<!-- roadmap-detail: src/timeseries/FUTURE_ENHANCEMENTS.md#-incremental-continuous-aggregation-with-watermark-pushdown -->
