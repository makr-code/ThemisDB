### Context

This issue implements the roadmap item 'SLO Monitor: Latency Percentile Tracking' for the cache domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: SLO Monitor: Latency Percentile Tracking

### Goal

Deliver the scoped changes for SLO Monitor: Latency Percentile Tracking in src/cache/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### SLO Monitor: Latency Percentile Tracking
**Priority:** Medium
**Target Version:** v1.8.0

`cache_hit_rate_slo_monitor.cpp` monitors only hit-rate thresholds (`config_.warning_threshold`, `config_.critical_threshold`). It does not track cache operation latency (p50/p99). Latency regressions (e.g. L3 compaction slowing down `get()`) are invisible.

**Implementation Notes:**
- `[ ]` Add a rolling HDRHistogram (or `utils/hdr_histogram.h` if available) to `CacheHitRateSloMonitor`; record latency per tier (L1/L2/L3) on each `get()` call.
- `[ ]` Add `CacheSloConfig::p99_warn_ms` and `p99_critical_ms` thresholds; fire Alertmanager alerts when exceeded, similar to the existing hit-rate alert path.
- `[ ]` Expose `p50_latency_ms`, `p95_latency_ms`, `p99_latency_ms` in the `/v1/admin/cache/stats` response.

---

### Acceptance Criteria

- [ ] Add a rolling HDRHistogram (or `utils/hdr_histogram.h` if available) to `CacheHitRateSloMonitor`; record latency per tier (L1/L2/L3) on each `get()` call.
- [ ] Add `CacheSloConfig::p99_warn_ms` and `p99_critical_ms` thresholds; fire Alertmanager alerts when exceeded, similar to the existing hit-rate alert path.
- [ ] Expose `p50_latency_ms`, `p95_latency_ms`, `p99_latency_ms` in the `/v1/admin/cache/stats` response.

### Relationships

- Roadmap row: #158 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cache/FUTURE_ENHANCEMENTS.md#slo-monitor-latency-percentile-tracking
- Source key: roadmap:158:cache:v1.8.0:slo-monitor-latency-percentile-tracking

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:158:cache:v1.8.0:slo-monitor-latency-percentile-tracking -->
<!-- roadmap-ref: row=158;module=cache;target=v1.8.0 -->
<!-- roadmap-detail: src/cache/FUTURE_ENHANCEMENTS.md#slo-monitor-latency-percentile-tracking -->
