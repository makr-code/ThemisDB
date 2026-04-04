### Context

This issue implements the roadmap item '`MetricsCollector`: Upgrade to `shared_mutex`' for the observability domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `MetricsCollector`: Upgrade to `shared_mutex` for Metric Read Path

### Goal

Deliver the scoped changes for `MetricsCollector`: Upgrade to `shared_mutex` in src/observability/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `MetricsCollector`: Upgrade to `shared_mutex` for Metric Read Path
**Priority:** Medium
**Target Version:** v1.8.0

`metrics_collector.cpp` uses `std::lock_guard<std::mutex>` (exclusive) for all read operations (`getMetric`, `scrapePrometheus`, `getAllMetrics` — lines 240, 251, 256). Under high-frequency Prometheus scraping, all metric reads serialize with each other. The `dropped_series_` atomic (line 261) is already correctly using an atomic; this pattern should be extended to the remaining read paths.

**Implementation Notes:**
- `[ ]` Replace `std::mutex mutex_` with `std::shared_mutex` in `MetricsCollector`.
- `[ ]` Upgrade `getMetric`, `scrapePrometheus`, `getAllMetrics`, `getSeriesCount` to `std::shared_lock`.
- `[ ]` Keep `record`, `increment`, `setGauge`, `observeHistogram`, `reset` on `std::unique_lock`.
- `[ ]` Add a TSAN-enabled stress test: 16 Prometheus scrape threads + 8 metric write threads concurrently.

**Performance Targets:**
- `scrapePrometheus()` throughput under 16 concurrent scrapers: ≥ 3× improvement vs. exclusive-mutex baseline.

---

### Acceptance Criteria

- [ ] Replace `std::mutex mutex_` with `std::shared_mutex` in `MetricsCollector`.
- [ ] Upgrade `getMetric`, `scrapePrometheus`, `getAllMetrics`, `getSeriesCount` to `std::shared_lock`.
- [ ] Keep `record`, `increment`, `setGauge`, `observeHistogram`, `reset` on `std::unique_lock`.
- [ ] Add a TSAN-enabled stress test: 16 Prometheus scrape threads + 8 metric write threads concurrently.
- [ ] `scrapePrometheus()` throughput under 16 concurrent scrapers: ≥ 3× improvement vs. exclusive-mutex baseline.

### Relationships

- Roadmap row: #191 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/observability/FUTURE_ENHANCEMENTS.md#metricscollecter-upgrade-to-shared_mutex-for-metric-read-path
- Source key: roadmap:191:observability:v1.8.0:metricscollecter-upgrade-to-shared-mutex-for-metric-read-path

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:191:observability:v1.8.0:metricscollecter-upgrade-to-shared-mutex-for-metric-read-path -->
<!-- roadmap-ref: row=191;module=observability;target=v1.8.0 -->
<!-- roadmap-detail: src/observability/FUTURE_ENHANCEMENTS.md#metricscollecter-upgrade-to-shared_mutex-for-metric-read-path -->
