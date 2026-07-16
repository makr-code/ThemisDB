### Context

This issue implements the roadmap item '`MLServingEngine::infer()` — TOCTOU Session Load + Full-Inference Lock' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 5 · `MLServingEngine::infer()` — TOCTOU Session Load + Full-Inference Lock

### Goal

Deliver the scoped changes for `MLServingEngine::infer()` — TOCTOU Session Load + Full-Inference Lock in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 5 · `MLServingEngine::infer()` — TOCTOU Session Load + Full-Inference Lock
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/ml_serving.cpp` lines 175–210

Two separate issues:

**5a – TOCTOU session check:** lines 178–188 take `sessions_mutex`, check whether the
session exists and call `loadSession()`, then release. Lines 190–200 immediately re-acquire
the same mutex and call `sessions.at(req.model_name)`. Between the two lock acquisitions
another thread can have evicted the session, causing `sessions.at()` to throw.

**5b – ONNX inference under global mutex:** lines 190–210 hold `sessions_mutex` for the
entire ONNX `Run()` call, serializing all model inferences regardless of which model is
targeted.

**Implementation Notes:**
- `[ ]` Replace the double-lock pattern with a single lock acquisition that obtains a `shared_ptr<OrtSession>` reference (or equivalent), then releases the mutex before calling ONNX `Run()`
- `[ ]` Move the session map from `std::unordered_map<string, unique_ptr<Session>>` to `std::unordered_map<string, shared_ptr<Session>>` so per-model handles can be retained outside the map lock
- `[ ]` Per-model `std::shared_mutex` (or `std::atomic<bool> loading_`) to serialize concurrent loads of the same model without blocking unrelated models
- `[ ]` Add test: two threads simultaneously infer on two different models; assert neither blocks the other

**Performance Targets:**
- Lock-hold per inference call: ≤ 5 µs (handle capture only)
- Two independent-model inferences: must proceed concurrently with no serialization

---

### Acceptance Criteria

- [ ] Replace the double-lock pattern with a single lock acquisition that obtains a `shared_ptr<OrtSession>` reference (or equivalent), then releases the mutex before calling ONNX `Run()`
- [ ] Move the session map from `std::unordered_map<string, unique_ptr<Session>>` to `std::unordered_map<string, shared_ptr<Session>>` so per-model handles can be retained outside the map lock
- [ ] Per-model `std::shared_mutex` (or `std::atomic<bool> loading_`) to serialize concurrent loads of the same model without blocking unrelated models
- [ ] Add test: two threads simultaneously infer on two different models; assert neither blocks the other
- [ ] Lock-hold per inference call: ≤ 5 µs (handle capture only)
- [ ] Two independent-model inferences: must proceed concurrently with no serialization

### Relationships

- Roadmap row: #44 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#5--mlservingengineinfer--toctou-session-load--full-inference-lock
- Source key: roadmap:44:analytics:v1.8.0:5-mlservingengineinfer-toctou-session-load-full-inference-lock

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:44:analytics:v1.8.0:5-mlservingengineinfer-toctou-session-load-full-inference-lock -->
<!-- roadmap-ref: row=44;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#5--mlservingengineinfer--toctou-session-load--full-inference-lock -->
