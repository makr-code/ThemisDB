### Context

This issue implements the roadmap item 'Lock-Free Metrics' for the core domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Lock-Free Metrics

### Goal

Deliver the scoped changes for Lock-Free Metrics in src/core/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Lock-Free Metrics
**Priority:** High  
**Target Version:** v1.6.0

Replace mutex-based counters with atomic operations.

**Implementation:**
- `std::atomic` for counters
- Lock-free ring buffer for histograms
- Thread-local aggregation with periodic flush

**Expected Improvement:** 80% reduction in metric update latency

---

### Acceptance Criteria

- [ ] `std::atomic` for counters
- [ ] Lock-free ring buffer for histograms
- [ ] Thread-local aggregation with periodic flush

### Relationships

- Roadmap row: #66 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/core/FUTURE_ENHANCEMENTS.md#lock-free-metrics
- Source key: roadmap:66:core:v1.6.0:lock-free-metrics

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:66:core:v1.6.0:lock-free-metrics -->
<!-- roadmap-ref: row=66;module=core;target=v1.6.0 -->
<!-- roadmap-detail: src/core/FUTURE_ENHANCEMENTS.md#lock-free-metrics -->
