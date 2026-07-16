### Context

This issue implements the roadmap item '`ModelServingEngine::predict()` — Inference Under Registry Lock' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 4 · `ModelServingEngine::predict()` — Inference Under Registry Lock

### Goal

Deliver the scoped changes for `ModelServingEngine::predict()` — Inference Under Registry Lock in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 4 · `ModelServingEngine::predict()` — Inference Under Registry Lock
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/model_serving.cpp` lines 196–230

`predict()` acquires `std::shared_lock lock(impl_->mu)` (line 200) to look up the model
entry in `impl_->registry`, then calls `e.model.predictOne(point)` (line 206) while still
holding the shared lock.  Inference is O(depth) for trees or O(k·N) for k-NN and can take
several milliseconds for large ensembles.  Although it is a shared lock, any concurrent
`registerModel()` or `unregisterModel()` caller waiting for an exclusive lock is starved for
the full inference duration.  Additionally, line 211 takes `e.health_mu` under the outer
`impl_->mu` — nested lock acquisition creates an implicit lock-order dependency.

**Implementation Notes:**
- `[ ]` Restructure `predict()` to: (1) take `shared_lock` for a brief pointer/ref capture of `*it->second`, (2) release `shared_lock`, (3) run `e.model.predictOne(point)` outside any registry lock, (4) take only `e.health_mu` for the health-metric update
- `[ ]` Use a `std::shared_ptr<Entry>` inside the registry so callers can retain a reference-counted handle after releasing the registry lock — eliminates the use-after-free risk from concurrent `unregisterModel()`
- `[ ]` Apply the same pattern to `predictBatch()` (line 244), `explain()` (line 283), and `evaluate()` (line 379) which exhibit the same lock-held-during-compute pattern
- `[ ]` Add a benchmark: 16 concurrent `predict()` callers on the same model; assert throughput ≥ 10 000 predictions/s per core

**Performance Targets:**
- Registry lock-hold per prediction: ≤ 5 µs (pointer capture only)
- Inference throughput (decision tree depth=10): ≥ 500 000 predictions/s on 8 cores

---

### Acceptance Criteria

- [ ] Restructure `predict()` to: (1) take `shared_lock` for a brief pointer/ref capture of `*it->second`, (2) release `shared_lock`, (3) run `e.model.predictOne(point)` outside any registry lock, (4) take only `e.health_mu` for the health-metric update
- [ ] Use a `std::shared_ptr<Entry>` inside the registry so callers can retain a reference-counted handle after releasing the registry lock — eliminates the use-after-free risk from concurrent `unregisterModel()`
- [ ] Apply the same pattern to `predictBatch()` (line 244), `explain()` (line 283), and `evaluate()` (line 379) which exhibit the same lock-held-during-compute pattern
- [ ] Add a benchmark: 16 concurrent `predict()` callers on the same model; assert throughput ≥ 10 000 predictions/s per core
- [ ] Registry lock-hold per prediction: ≤ 5 µs (pointer capture only)
- [ ] Inference throughput (decision tree depth=10): ≥ 500 000 predictions/s on 8 cores

### Relationships

- Roadmap row: #43 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#4--modelservingenginepredict--inference-under-registry-lock
- Source key: roadmap:43:analytics:v1.8.0:4-modelservingenginepredict-inference-under-registry-lock

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:43:analytics:v1.8.0:4-modelservingenginepredict-inference-under-registry-lock -->
<!-- roadmap-ref: row=43;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#4--modelservingenginepredict--inference-under-registry-lock -->
