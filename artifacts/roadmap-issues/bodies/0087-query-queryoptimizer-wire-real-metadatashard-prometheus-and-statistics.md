### Context

This issue implements the roadmap item '`QueryOptimizer`: Wire Real MetadataShard, Prometheus, and Statistics' for the query domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: `QueryOptimizer`: Wire Real MetadataShard, Prometheus, and Statistics

### Goal

Deliver the scoped changes for `QueryOptimizer`: Wire Real MetadataShard, Prometheus, and Statistics in src/query/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### `QueryOptimizer`: Wire Real MetadataShard, Prometheus, and Statistics
**Priority:** High
**Target Version:** v1.6.0

`query_optimizer.cpp` has 3 explicit TODOs all marked `(v1.5.1)`:
- Line 507: "Replace with actual `MetadataShard` integration" — optimizer uses a hardcoded fallback instead of real schema statistics.
- Line 536: "Replace with actual `PrometheusMetrics` integration" — optimizer emits no metrics; query plan selection quality is invisible.
- Line 575: "Use actual statistics and histograms" — cardinality estimates are hardcoded constants, degrading join order selection.

**Implementation Notes:**
- `[ ]` **Line 507**: Inject a `MetadataShard*` (or `StatisticsCollector*` from `src/metadata/statistics_collector.cpp`) into `QueryOptimizer` constructor; replace the hardcoded fallback with `StatisticsCollector::getCardinality(collection, field)`.
- `[ ]` **Line 536**: Inject a `MetricsCollector*`; emit `query.optimizer.plan_selected`, `query.optimizer.rewrite_count`, and `query.optimizer.cost_estimate` counters on each `optimize()` call.
- `[ ]` **Line 575**: Use `StatisticsCollector::getHistogram(collection, field)` equi-height histograms for selectivity estimation in join cost model.
- `[ ]` Add unit tests: verify that optimizer chooses index scan over full scan when selectivity < 10 % and statistics are present; verify Prometheus counters increment on each plan selection.

**Performance Targets:**
- Optimizer `optimize()` latency: ≤ 5 ms for queries with ≤ 10 joins using real statistics.

---

### Acceptance Criteria

- [ ] Line 507: "Replace with actual `MetadataShard` integration" — optimizer uses a hardcoded fallback instead of real schema statistics.
- [ ] Line 536: "Replace with actual `PrometheusMetrics` integration" — optimizer emits no metrics; query plan selection quality is invisible.
- [ ] Line 575: "Use actual statistics and histograms" — cardinality estimates are hardcoded constants, degrading join order selection.
- [ ] **Line 507**: Inject a `MetadataShard*` (or `StatisticsCollector*` from `src/metadata/statistics_collector.cpp`) into `QueryOptimizer` constructor; replace the hardcoded fallback with `StatisticsCollector::getCardinality(collection, field)`.
- [ ] **Line 536**: Inject a `MetricsCollector*`; emit `query.optimizer.plan_selected`, `query.optimizer.rewrite_count`, and `query.optimizer.cost_estimate` counters on each `optimize()` call.
- [ ] **Line 575**: Use `StatisticsCollector::getHistogram(collection, field)` equi-height histograms for selectivity estimation in join cost model.
- [ ] Add unit tests: verify that optimizer chooses index scan over full scan when selectivity < 10 % and statistics are present; verify Prometheus counters increment on each plan selection.
- [ ] Optimizer `optimize()` latency: ≤ 5 ms for queries with ≤ 10 joins using real statistics.

### Relationships

- Roadmap row: #87 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#queryoptimizer-wire-real-metadatashard-prometheus-and-statistics
- Source key: roadmap:87:query:v1.6.0:queryoptimizer-wire-real-metadatashard-prometheus-and-statistics

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:87:query:v1.6.0:queryoptimizer-wire-real-metadatashard-prometheus-and-statistics -->
<!-- roadmap-ref: row=87;module=query;target=v1.6.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#queryoptimizer-wire-real-metadatashard-prometheus-and-statistics -->
