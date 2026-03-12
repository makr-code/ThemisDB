### Context

This issue implements the roadmap item '`DiffEngine::computeDiff()` — Cache Stampede / O(N) Changefeed Scan' for the analytics domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 9 · `DiffEngine::computeDiff()` — Cache Stampede / O(N) Changefeed Scan

### Goal

Deliver the scoped changes for `DiffEngine::computeDiff()` — Cache Stampede / O(N) Changefeed Scan in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 9 · `DiffEngine::computeDiff()` — Cache Stampede / O(N) Changefeed Scan
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/diff_engine.cpp` lines 175–220

`computeDiff()` checks the cache under `cache_mutex_` (line 181), releases the lock, then
performs a linear scan of the entire changefeed (`listEvents` with `limit=0`, line 198),
then re-acquires `cache_mutex_` to write the result (line 217).  Two concurrent callers
requesting the same diff range will both miss the cache, both perform the expensive scan,
and both write the result — a classic cache stampede.  The O(N) post-filter loop (lines
200–207) over all events then discards events outside the requested range; the changefeed
should be queried with both `from_sequence` and `to_sequence` bounds to avoid scanning the
entire log.

**Implementation Notes:**
- `[ ]` Add an in-flight-request set (`std::unordered_set<std::pair<int64_t,int64_t>>`) so the second caller for the same range waits on a `condition_variable` rather than re-computing
- `[ ]` Pass `from_sequence` and `to_sequence` as bounds to `changefeed_.listEvents()` when the `Changefeed::ListOptions` struct supports it — avoids materializing the entire event log
- `[ ]` Replace raw `listEvents(…); filter in loop` pattern with a binary-search or indexed range query when the changefeed is backed by a sorted store
- `[ ]` `evictOldCacheEntries()` (called while holding `cache_mutex_` at line 217) performs an unguarded iteration — apply the same copy-evict-then-lock pattern to keep lock duration short

**Performance Targets:**
- `computeDiff()` cache-miss path for a 1 M-event log, range [N-1000, N]: ≤ 50 ms
- Stampede prevention: second concurrent caller for the same range must wait ≤ 5 ms

---

### Acceptance Criteria

- [ ] Add an in-flight-request set (`std::unordered_set<std::pair<int64_t,int64_t>>`) so the second caller for the same range waits on a `condition_variable` rather than re-computing
- [ ] Pass `from_sequence` and `to_sequence` as bounds to `changefeed_.listEvents()` when the `Changefeed::ListOptions` struct supports it — avoids materializing the entire event log
- [ ] Replace raw `listEvents(…); filter in loop` pattern with a binary-search or indexed range query when the changefeed is backed by a sorted store
- [ ] `evictOldCacheEntries()` (called while holding `cache_mutex_` at line 217) performs an unguarded iteration — apply the same copy-evict-then-lock pattern to keep lock duration short
- [ ] `computeDiff()` cache-miss path for a 1 M-event log, range [N-1000, N]: ≤ 50 ms
- [ ] Stampede prevention: second concurrent caller for the same range must wait ≤ 5 ms

### Relationships

- Roadmap row: #136 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#9--diffenginecomputediff--cache-stampede--on-changefeed-scan
- Source key: roadmap:136:analytics:v1.8.0:9-diffenginecomputediff-cache-stampede-on-changefeed-scan

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:136:analytics:v1.8.0:9-diffenginecomputediff-cache-stampede-on-changefeed-scan -->
<!-- roadmap-ref: row=136;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#9--diffenginecomputediff--cache-stampede--on-changefeed-scan -->
