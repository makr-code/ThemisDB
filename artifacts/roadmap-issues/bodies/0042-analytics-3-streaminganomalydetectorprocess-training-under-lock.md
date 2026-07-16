### Context

This issue implements the roadmap item '`StreamingAnomalyDetector::process()` — Training Under Lock' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 3 · `StreamingAnomalyDetector::process()` — Training Under Lock

### Goal

Deliver the scoped changes for `StreamingAnomalyDetector::process()` — Training Under Lock in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 3 · `StreamingAnomalyDetector::process()` — Training Under Lock
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/anomaly_detection.cpp` lines 1035–1070

`StreamingAnomalyDetector::process()` acquires `mu_` at line 1040 and holds it for the
entire execution, including:
- Line 1051: `std::vector<DataPoint> buf(window_.begin(), window_.end())` — full deque-to-vector copy
- Line 1053: `detector_.train(buf)` — O(N·T) for IsolationForest (`N` = window size, `T` = trees), O(N²) for LOF
- Line 1063–1064: `detector_.predict(point)` — model scoring while holding the lock

Every concurrent call to `process()` (from any producer thread) blocks for the entire
training duration.

**Implementation Notes:**
- `[ ]` Extract a private `snapshotWindow()` helper that copies the deque under a brief lock scope and returns a `std::vector<DataPoint>` — lock is released before calling `train()` or `predict()`
- `[ ]` Gate retrain (`retrain_on_window`) behind an `std::atomic<bool> retraining_` flag and schedule training on a dedicated background thread using `std::async(std::launch::async, …)` to keep `process()` non-blocking
- `[ ]` `detector_.predict(point)` is stateless once trained — hold only a `std::shared_lock<std::shared_mutex>` during prediction and upgrade to `unique_lock` only when `isTrained()` state changes
- `[ ]` `getAnomalies()` (line 1080) and `getStats()` (line 1085/1090) each take their own `lock_guard` — these are read-only accessors; use `shared_lock` for them
- `[ ]` Add a concurrency stress test: 8 producer threads calling `process()` at 100 kHz; assert P99 latency ≤ 1 ms with no deadlocks

**Performance Targets:**
- `process()` lock-hold duration: ≤ 50 µs (deque copy only; training async)
- Training throughput: IsolationForest on 1 000-point window ≤ 10 ms

---

### Acceptance Criteria

- [ ] Line 1051: `std::vector<DataPoint> buf(window_.begin(), window_.end())` — full deque-to-vector copy
- [ ] Line 1053: `detector_.train(buf)` — O(N·T) for IsolationForest (`N` = window size, `T` = trees), O(N²) for LOF
- [ ] Line 1063–1064: `detector_.predict(point)` — model scoring while holding the lock
- [ ] Extract a private `snapshotWindow()` helper that copies the deque under a brief lock scope and returns a `std::vector<DataPoint>` — lock is released before calling `train()` or `predict()`
- [ ] Gate retrain (`retrain_on_window`) behind an `std::atomic<bool> retraining_` flag and schedule training on a dedicated background thread using `std::async(std::launch::async, …)` to keep `process()` non-blocking
- [ ] `detector_.predict(point)` is stateless once trained — hold only a `std::shared_lock<std::shared_mutex>` during prediction and upgrade to `unique_lock` only when `isTrained()` state changes
- [ ] `getAnomalies()` (line 1080) and `getStats()` (line 1085/1090) each take their own `lock_guard` — these are read-only accessors; use `shared_lock` for them
- [ ] Add a concurrency stress test: 8 producer threads calling `process()` at 100 kHz; assert P99 latency ≤ 1 ms with no deadlocks
- [ ] `process()` lock-hold duration: ≤ 50 µs (deque copy only; training async)
- [ ] Training throughput: IsolationForest on 1 000-point window ≤ 10 ms

### Relationships

- Roadmap row: #42 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#3--streaminganomalydetectorprocess--training-under-lock
- Source key: roadmap:42:analytics:v1.8.0:3-streaminganomalydetectorprocess-training-under-lock

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:42:analytics:v1.8.0:3-streaminganomalydetectorprocess-training-under-lock -->
<!-- roadmap-ref: row=42;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#3--streaminganomalydetectorprocess--training-under-lock -->
