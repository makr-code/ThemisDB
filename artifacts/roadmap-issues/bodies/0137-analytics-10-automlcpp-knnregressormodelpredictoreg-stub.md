### Context

This issue implements the roadmap item '`automl.cpp` — `KNNRegressorModel::predictOneReg()` Stub' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 10 · `automl.cpp` — `KNNRegressorModel::predictOneReg()` Stub

### Goal

Deliver the scoped changes for `automl.cpp` — `KNNRegressorModel::predictOneReg()` Stub in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 10 · `automl.cpp` — `KNNRegressorModel::predictOneReg()` Stub
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/automl.cpp` line 833

```cpp
double predictOneReg(const std::vector<double>&) const override { return 0.0; }
```

`KNNRegressorModel` silently returns `0.0` for all regression predictions.  The classifier
path (`predictOneClass()`) correctly implements k-NN lookup; only the regression counterpart
is missing.  Any AutoML pipeline that trains a k-NN model on a regression task will produce
silent zero predictions without any warning.

**Implementation Notes:**
- `[ ]` Implement `predictOneReg()` as the mean of the `k_` nearest neighbours' target values, reusing the existing distance-computation logic from the classifier path (`knn()` helper in `anomaly_detection.cpp` or inline equivalent)
- `[ ]` Add a guard in `AutoML::train()` that logs `spdlog::warn("KNNRegressorModel::predictOneReg: stub returning 0.0 – regression not yet implemented")` as an interim measure until the fix is deployed
- `[ ]` Add a unit test: train a KNN model on `y = 2x` with 100 training points; `predictOneReg({5.0})` must return a value within ±0.5 of 10.0

**Performance Targets:**
- `predictOneReg()` for k=5 on a 10 000-sample training set: ≤ 1 ms

---

### Acceptance Criteria

- [ ] Implement `predictOneReg()` as the mean of the `k_` nearest neighbours' target values, reusing the existing distance-computation logic from the classifier path (`knn()` helper in `anomaly_detection.cpp` or inline equivalent)
- [ ] Add a guard in `AutoML::train()` that logs `spdlog::warn("KNNRegressorModel::predictOneReg: stub returning 0.0 – regression not yet implemented")` as an interim measure until the fix is deployed
- [ ] Add a unit test: train a KNN model on `y = 2x` with 100 training points; `predictOneReg({5.0})` must return a value within ±0.5 of 10.0
- [ ] `predictOneReg()` for k=5 on a 10 000-sample training set: ≤ 1 ms

### Relationships

- Roadmap row: #137 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#10--automlcpp--knnregressormodelpredictoreg-stub
- Source key: roadmap:137:analytics:v1.8.0:10-automlcpp-knnregressormodelpredictoreg-stub

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:137:analytics:v1.8.0:10-automlcpp-knnregressormodelpredictoreg-stub -->
<!-- roadmap-ref: row=137;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#10--automlcpp--knnregressormodelpredictoreg-stub -->
