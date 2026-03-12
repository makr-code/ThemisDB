### Context

This issue implements the roadmap item 'Predictive Prefetcher: ML-Based Access Pattern Model' for the cache domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Predictive Prefetcher: ML-Based Access Pattern Model

### Goal

Deliver the scoped changes for Predictive Prefetcher: ML-Based Access Pattern Model in src/cache/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Predictive Prefetcher: ML-Based Access Pattern Model
**Priority:** Medium
**Target Version:** v1.8.0

`predictive_prefetcher.cpp` uses a simple frequency counter over a fixed candidate window (`config_.max_predictions`) to predict next accesses. There is no sequential-access pattern detection or time-of-day awareness. The model is not persistent across restarts.

**Implementation Notes:**
- `[ ]` Replace frequency counter with a Markov chain transition matrix (order-1) keyed by the last `N` accessed fingerprints; serialize/deserialize the matrix to RocksDB under prefix `prefetch_model::`.
- `[ ]` Add time-of-day bucketing (24 one-hour buckets) so prefetch probability is weighted by historical access at the current hour.
- `[ ]` Emit `cache.prefetch.hit_rate` and `cache.prefetch.overhead_bytes` metrics via `MetricsCollector` to evaluate model effectiveness in production.
- `[ ]` Add a prefetcher A/B test toggle: route 50 % of tenants to Markov model vs. frequency baseline; compare hit-rate improvement.

**Performance Targets:**
- Prefetch prediction latency: ≤ 100 µs per call.
- Prefetch overhead (bytes fetched but never hit): ≤ 10 % of total prefetch volume.

---

### Acceptance Criteria

- [ ] Replace frequency counter with a Markov chain transition matrix (order-1) keyed by the last `N` accessed fingerprints; serialize/deserialize the matrix to RocksDB under prefix `prefetch_model::`.
- [ ] Add time-of-day bucketing (24 one-hour buckets) so prefetch probability is weighted by historical access at the current hour.
- [ ] Emit `cache.prefetch.hit_rate` and `cache.prefetch.overhead_bytes` metrics via `MetricsCollector` to evaluate model effectiveness in production.
- [ ] Add a prefetcher A/B test toggle: route 50 % of tenants to Markov model vs. frequency baseline; compare hit-rate improvement.
- [ ] Prefetch prediction latency: ≤ 100 µs per call.
- [ ] Prefetch overhead (bytes fetched but never hit): ≤ 10 % of total prefetch volume.

### Relationships

- Roadmap row: #157 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cache/FUTURE_ENHANCEMENTS.md#predictive-prefetcher-ml-based-access-pattern-model
- Source key: roadmap:157:cache:v1.8.0:predictive-prefetcher-ml-based-access-pattern-model

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:157:cache:v1.8.0:predictive-prefetcher-ml-based-access-pattern-model -->
<!-- roadmap-ref: row=157;module=cache;target=v1.8.0 -->
<!-- roadmap-detail: src/cache/FUTURE_ENHANCEMENTS.md#predictive-prefetcher-ml-based-access-pattern-model -->
