### Context

This issue implements the roadmap item 'Parallel Execution of `translateBatchNLToAQL()`' for the aql domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: 7 · Parallel Execution of `translateBatchNLToAQL()`

### Goal

Deliver the scoped changes for Parallel Execution of `translateBatchNLToAQL()` in src/aql/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### 7 · Parallel Execution of `translateBatchNLToAQL()`
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `llm_aql_handler.cpp:translateBatchNLToAQL()` (lines 1172–1188) processes each request sequentially in a `for` loop. Each call to `translateNLToAQL()` performs a synchronous LLM inference (potentially 1–30 seconds). A batch of 10 independent translation requests therefore takes 10× the single-request latency. There is no parallelism despite each request being completely independent.

**Implementation Notes:**
- `[ ]` Replace the sequential loop with `std::transform` over a `std::vector<std::future<BatchNLToAQLResult>>` created via `std::async(std::launch::async, ...)`; collect futures in a second pass
- `[ ]` Respect a `max_concurrent_requests` limit (default: `std::thread::hardware_concurrency()`) to avoid exhausting the LLM backend thread pool; implement with a semaphore (`std::counting_semaphore`, C++20) or a bounded thread pool
- `[ ]` Propagate per-request cancellation: if one request in the batch throws a non-retryable exception, do not cancel others (current sequential behaviour accidentally provides this; parallel version must preserve it)
- `[ ]` Add a `translateBatchNLToAQLAsync()` overload that returns `std::future<std::vector<BatchNLToAQLResult>>`
- `[ ]` Benchmark: 10 independent requests with a mock LLM (each 50 ms) should complete in ≤ 150 ms wall-time when concurrency ≥ 4

---

### Acceptance Criteria

- [ ] Replace the sequential loop with `std::transform` over a `std::vector<std::future<BatchNLToAQLResult>>` created via `std::async(std::launch::async, ...)`; collect futures in a second pass
- [ ] Respect a `max_concurrent_requests` limit (default: `std::thread::hardware_concurrency()`) to avoid exhausting the LLM backend thread pool; implement with a semaphore (`std::counting_semaphore`, C++20) or a bounded thread pool
- [ ] Propagate per-request cancellation: if one request in the batch throws a non-retryable exception, do not cancel others (current sequential behaviour accidentally provides this; parallel version must preserve it)
- [ ] Add a `translateBatchNLToAQLAsync()` overload that returns `std::future<std::vector<BatchNLToAQLResult>>`
- [ ] Benchmark: 10 independent requests with a mock LLM (each 50 ms) should complete in ≤ 150 ms wall-time when concurrency ≥ 4

### Relationships

- Roadmap row: #147 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#7--parallel-execution-of-translatebatchnltoaql
- Source key: roadmap:147:aql:v1.7.0:7-parallel-execution-of-translatebatchnltoaql

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:147:aql:v1.7.0:7-parallel-execution-of-translatebatchnltoaql -->
<!-- roadmap-ref: row=147;module=aql;target=v1.7.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#7--parallel-execution-of-translatebatchnltoaql -->
