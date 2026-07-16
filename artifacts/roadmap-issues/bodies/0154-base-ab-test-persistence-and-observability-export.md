### Context

This issue implements the roadmap item 'A/B Test Persistence and Observability Export' for the base domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.3.0.

Primary detail section: A/B Test Persistence and Observability Export

### Goal

Deliver the scoped changes for A/B Test Persistence and Observability Export in src/base/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### A/B Test Persistence and Observability Export
**Priority:** Medium
**Target Version:** v1.3.0

`ab_test_manager.cpp` stores `ABVariantMetrics` exclusively in memory (in `tests_` map). All metrics are lost on server restart. There is also no export to the observability stack (MetricsCollector / OpenTelemetry).

**Implementation Notes:**
- `[ ]` Persist `ABTestConfig` and `ABVariantMetrics` snapshots to RocksDB using key prefix `ab_test::` via the `StorageEngine` interface; reload on `ABTestManager::start()`.
- `[ ]` Emit per-variant counters (`ab_test.<test_id>.<variant>.requests`, `.conversions`, `.latency_p99`) to `MetricsCollector` on every `recordEvent()` call without holding the `tests_` mutex.
- `[ ]` Add `ABTestManager::exportMetricsSnapshot()` returning a `std::vector<ABTestMetricRow>` for admin API consumption.
- `[ ]` Add a Bayesian Thompson Sampling auto-stop: when posterior probability that treatment beats control exceeds a configurable threshold (default 0.95), mark the test as concluded and route all traffic to the winner.

**Performance Targets:**
- `recordEvent()` (hot path): ≤ 2 µs with metrics emission; no mutex held during MetricsCollector call.

---

### Acceptance Criteria

- [ ] Persist `ABTestConfig` and `ABVariantMetrics` snapshots to RocksDB using key prefix `ab_test::` via the `StorageEngine` interface; reload on `ABTestManager::start()`.
- [ ] Emit per-variant counters (`ab_test.<test_id>.<variant>.requests`, `.conversions`, `.latency_p99`) to `MetricsCollector` on every `recordEvent()` call without holding the `tests_` mutex.
- [ ] Add `ABTestManager::exportMetricsSnapshot()` returning a `std::vector<ABTestMetricRow>` for admin API consumption.
- [ ] Add a Bayesian Thompson Sampling auto-stop: when posterior probability that treatment beats control exceeds a configurable threshold (default 0.95), mark the test as concluded and route all traffic to the winner.
- [ ] `recordEvent()` (hot path): ≤ 2 µs with metrics emission; no mutex held during MetricsCollector call.

### Relationships

- Roadmap row: #154 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/base/FUTURE_ENHANCEMENTS.md#ab-test-persistence-and-observability-export
- Source key: roadmap:154:base:v1.3.0:ab-test-persistence-and-observability-export

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:154:base:v1.3.0:ab-test-persistence-and-observability-export -->
<!-- roadmap-ref: row=154;module=base;target=v1.3.0 -->
<!-- roadmap-detail: src/base/FUTURE_ENHANCEMENTS.md#ab-test-persistence-and-observability-export -->
