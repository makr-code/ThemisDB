### Context

This issue implements the roadmap item 'Memory Pool Allocator for Hot Analytics Paths' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 15 · Memory Pool Allocator for Hot Analytics Paths

### Goal

Deliver the scoped changes for Memory Pool Allocator for Hot Analytics Paths in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 15 · Memory Pool Allocator for Hot Analytics Paths
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/olap.cpp`, `src/analytics/columnar_execution.cpp`, `src/analytics/cep_engine.cpp`

Repeated `std::vector` construction/destruction for intermediate aggregation buffers (group
key maps, scratch arrays in `ColumnarAggregator::execute()`, `CEPEngine::workerLoop()`
event copies) causes frequent heap allocations in the hot path.

**Implementation Notes:**
- `[ ]` Introduce `AnalyticsMemoryPool` (arena allocator, initial size 64 MB) in `src/analytics/detail/memory_pool.h` with `allocate(size, align)` and `reset()` — no individual free, reset per query
- `[ ]` Wire pool into `OLAPEngine::Impl` and `ColumnarAggregator` so intermediate group-key strings and `AggState` maps allocate from the pool; `pool_.reset()` at the start of each `execute()` call
- `[ ]` For `CEPEngine`, use a lock-free ring buffer (SPSC if single producer, MPSC if multi) for the event queue rather than `std::queue<std::pair<string,Event>>` — eliminates per-event `std::string` copy for the stream_id
- `[ ]` Ensure the pool is not shared across threads; each `OLAPEngine::Impl` thread gets its own pool or uses thread-local storage

**Performance Targets:**
- Allocation overhead in `OLAPEngine::execute()`: ≤ 5 % of total query time (currently estimated 15–30 % for GROUP BY with many groups)

---

### Acceptance Criteria

- [ ] Introduce `AnalyticsMemoryPool` (arena allocator, initial size 64 MB) in `src/analytics/detail/memory_pool.h` with `allocate(size, align)` and `reset()` — no individual free, reset per query
- [ ] Wire pool into `OLAPEngine::Impl` and `ColumnarAggregator` so intermediate group-key strings and `AggState` maps allocate from the pool; `pool_.reset()` at the start of each `execute()` call
- [ ] For `CEPEngine`, use a lock-free ring buffer (SPSC if single producer, MPSC if multi) for the event queue rather than `std::queue<std::pair<string,Event>>` — eliminates per-event `std::string` copy for the stream_id
- [ ] Ensure the pool is not shared across threads; each `OLAPEngine::Impl` thread gets its own pool or uses thread-local storage
- [ ] Allocation overhead in `OLAPEngine::execute()`: ≤ 5 % of total query time (currently estimated 15–30 % for GROUP BY with many groups)

### Relationships

- Roadmap row: #47 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#15--memory-pool-allocator-for-hot-analytics-paths
- Source key: roadmap:47:analytics:v1.8.0:15-memory-pool-allocator-for-hot-analytics-paths

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:47:analytics:v1.8.0:15-memory-pool-allocator-for-hot-analytics-paths -->
<!-- roadmap-ref: row=47;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#15--memory-pool-allocator-for-hot-analytics-paths -->
