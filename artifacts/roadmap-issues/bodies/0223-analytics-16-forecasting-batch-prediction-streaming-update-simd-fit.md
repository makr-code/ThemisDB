### Context

This issue implements the roadmap item 'Forecasting: Batch Prediction, Streaming Update, SIMD Fit' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: 16 · Forecasting: Batch Prediction, Streaming Update, SIMD Fit

### Goal

Deliver the scoped changes for Forecasting: Batch Prediction, Streaming Update, SIMD Fit in src/analytics/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### 16 · Forecasting: Batch Prediction, Streaming Update, SIMD Fit
**Priority:** Medium
**Target Version:** v1.9.0
**Files:** `src/analytics/forecasting.cpp`, `include/analytics/forecasting.h`

The forecasting engine supports `fit()` + `predict(steps)` but lacks the following
capabilities needed for production deployments.

**Implementation Notes:**
- `[ ]` Add `predictBatch(const std::vector<TimeSeries>& batch, int steps) → std::vector<std::vector<ForecastPoint>>` to amortise model-state copies across independent series — existing `predict()` re-copies internal state on every call
- `[ ]` Add `update(double new_value)` for O(1) one-step incremental absorption of a new observation without full `fit()` rerun — update only the ETS level/trend/seasonal components
- `[ ]` Auto-tune (HES `auto_tune=true`) grid search over alpha/beta/gamma is single-threaded — parallelize with `std::async` or OpenMP; 9-point grid on 500-sample series currently takes up to 50 ms single-threaded
- `[ ]` Cache last `fit()` result indexed by `(xxHash(training_data), config_hash)` so repeated fits on unchanged data are O(1) hash lookups
- `[ ]` Extend the existing AVX2 Yule–Walker scaffold to a compiled-in AVX-512 path (see section 14)

**Performance Targets:**
- `predictBatch()` for 1 000 series × 30 steps: ≤ 50 ms on a single core
- `update(new_value)`: O(1), ≤ 10 µs per call
- Auto-tune grid (9 α, n=500): ≤ 5 ms with parallel search

---

### Acceptance Criteria

- [ ] Add `predictBatch(const std::vector<TimeSeries>& batch, int steps) → std::vector<std::vector<ForecastPoint>>` to amortise model-state copies across independent series — existing `predict()` re-copies internal state on every call
- [ ] Add `update(double new_value)` for O(1) one-step incremental absorption of a new observation without full `fit()` rerun — update only the ETS level/trend/seasonal components
- [ ] Auto-tune (HES `auto_tune=true`) grid search over alpha/beta/gamma is single-threaded — parallelize with `std::async` or OpenMP; 9-point grid on 500-sample series currently takes up to 50 ms single-threaded
- [ ] Cache last `fit()` result indexed by `(xxHash(training_data), config_hash)` so repeated fits on unchanged data are O(1) hash lookups
- [ ] Extend the existing AVX2 Yule–Walker scaffold to a compiled-in AVX-512 path (see section 14)
- [ ] `predictBatch()` for 1 000 series × 30 steps: ≤ 50 ms on a single core
- [ ] `update(new_value)`: O(1), ≤ 10 µs per call
- [ ] Auto-tune grid (9 α, n=500): ≤ 5 ms with parallel search

### Relationships

- Roadmap row: #223 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#16--forecasting-batch-prediction-streaming-update-simd-fit
- Source key: roadmap:223:analytics:v1.9.0:16-forecasting-batch-prediction-streaming-update-simd-fit

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:223:analytics:v1.9.0:16-forecasting-batch-prediction-streaming-update-simd-fit -->
<!-- roadmap-ref: row=223;module=analytics;target=v1.9.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#16--forecasting-batch-prediction-streaming-update-simd-fit -->
