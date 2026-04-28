> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-06-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · include/analytics/FUTURE_ENHANCEMENTS.md -->

# Analytics Module - Future Enhancements

**Version:** 1.8.0
**Status:** 📋 Active
**Last Updated:** 2026-06-09
**Module Path:** `src/analytics/`

---

## Scope

The analytics module provides the full pipeline from raw event ingestion to insight delivery:
OLAP aggregation (SUM/AVG/MIN/MAX/STDDEV/PERCENTILE over columnar data), streaming window
operators (tumbling, sliding, session, hop), a Complex Event Processing (CEP) engine with
NFA-based pattern matching, incremental materialized view maintenance (IVM), time-series
forecasting (ARIMA/Yule–Walker/Holt–Winters), multi-algorithm anomaly detection
(Isolation Forest, Z-Score, LOF, ensemble), AutoML model training and serving, process
mining, NLP analysis, Arrow/Parquet export, Arrow Flight RPC, and distributed shard-based
aggregation.  Twelve `.cpp` implementation files are covered below; all
identified issues reference exact file names and function names.

---

## Design Constraints

- `[x]` `std::lock_guard` / `std::unique_lock` must **never** be held across user callbacks, network I/O, or O(N²) computation – all identified cases resolved (CEP timerLoop, StreamingAnomalyDetector, ModelServingEngine, MLServingEngine, IncrementalView)
- `[x]` AVX-512 and ARM NEON kernel results must be bit-identical (tolerance ≤ 1 ULP) to the scalar baseline on the same input dataset
- `[ ]` Streaming aggregation peak memory must not exceed 512 MB per active window; enforced via compile-time configurable hard cap
- `[x]` IVM delta-application latency must be ≤ 50 ms for batches ≤ 10 000 rows; `applyChanges()` must not hold its exclusive lock for the full batch
- `[x]` `ExporterFactory::createExporter(format)` must return a format-specific exporter, not the universal `StubAnalyticsExporter` for every format
- `[ ]` Windows platform build stubs in `olap.cpp` and `process_mining.cpp` must be replaced by real cross-platform implementations before v2.0.0
- `[x]` All background loops (`expiryLoop`, `timerLoop`, `workerLoop`, `metricsLoop`) honour stop signals via condition variables — `CEPEngine::metricsLoop()` uses `metrics_cv_.wait_for` with stop predicate
- `[ ]` No dynamic memory allocation inside SIMD hot loops; intermediate buffers must be pre-allocated in `Impl` structs

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ExporterFactory::createExporter(format)` → `IFormatExporter` | Export pipeline | Must dispatch to Arrow IPC / Parquet / Feather exporter, not always `StubAnalyticsExporter` |
| `IncrementalView::applyChanges(batch)` | Storage CDC pipeline | Needs batch-split to bound lock-hold duration |
| `StreamingAnomalyDetector::process(point)` | Real-time alerting | Must perform training outside `mu_` lock |
| `ModelServingEngine::predict(name, version, point)` | Query executor | Inference must run outside the registry shared-lock |
| `CEPEngine::timerLoop()` | CEP runtime | Window callbacks must be dispatched after lock release |
| `DistributedAnalyticsSharding::getHealthyShardCount()` | Health dashboard | Network I/O must not run under `mutex_` |
| `LLMProcessAnalyzer::Impl::putInCache(key, response)` | LLM integration | ✅ Fixed v1.8.0: O(N) eviction replaced with O(1) LRU (doubly-linked list + hash map); SHA256 cache key; max_cache_entries in LLMConfig |
| `AutoMLModel::KNNRegressorModel::predictOneReg(x)` | AutoML serving | Stub `return 0.0` must be replaced with real k-NN regression |
| `OLAPEngine` (Windows) | Cross-platform build | Full implementation needed; current stub emits warnings and returns empty results |
| `ProcessMining` (Windows) | Cross-platform build | Stub returns `Status::Error` for every operation |

---

## Planned Features

### 1 · ExporterFactory Stub Replacement
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/analytics_export.cpp` lines 728–734

`ExporterFactory::createExporter(ExportFormat)` and `createDefaultExporter()` both return
`std::make_unique<StubAnalyticsExporter>()` unconditionally.  The comment on line 728 reads
*"For now, return stub exporter for all formats – In the future, this would return
format-specific exporters"*.  The `StubAnalyticsExporter` class itself (line 203) delegates
to `exportToFileArrow()` only when `THEMIS_HAS_ARROW` is set, and for all three Arrow
formats falls through to a `NOT_SUPPORTED` status when Arrow is absent, but the factory
never instantiates any specialised class regardless.

**Implementation Notes:**
- `[x]` Introduce `ArrowIPCExporter`, `ParquetExporter`, and `FeatherExporter` classes that wrap the existing `exportToFileArrow()` logic – remove dead `StubAnalyticsExporter` wrapper
- `[x]` Rename `StubAnalyticsExporter` to `JSONCSVExporter` to reflect its actual capability scope
- `[x]` `createExporter(ExportFormat)` must switch on `format` and return the correct concrete type; formats unavailable without Arrow must return `std::unexpected` / throw `std::runtime_error` with a clear message instead of silently returning the fallback
- `[x]` Add unit test that asserts `createExporter(ExportFormat::FMT_ARROW_PARQUET)` returns a non-stub type when `THEMIS_HAS_ARROW` is defined
- `[x]` Suppress the `6 Stubs` annotation in the file header once all stubs are promoted to real implementations

**Performance Targets:**
- Parquet export of 1 M rows: ≤ 2 s wall time with snappy compression on a single core
- CSV export of 1 M rows: ≤ 500 ms (streaming write, no full in-memory serialization)

---

### 2 · Lock Held Across User Callbacks in `CEPEngine::timerLoop()`
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/cep_engine.cpp` lines 1071–1095

`WindowManager::timerLoop()` acquires `windows_mutex_` (line 1079) and then immediately
calls `callback_(w.events, w.start, …)` for every open GLOBAL window (line 1082–1084).
User-supplied callbacks are arbitrary code and can perform I/O, database writes, or other
blocking work.  While `windows_mutex_` is held, no other thread can add events, close
windows, or read window state.

**Implementation Notes:**
- `[x]` In `timerLoop()`, snapshot the callbacks and their arguments under the lock (copy event vectors and timestamps), release `windows_mutex_`, then invoke callbacks on the snapshot — identical to the copy-and-dispatch idiom
- `[x]` Introduce a `WindowCallbackBatch` value type that carries `(events_copy, start, now)` to make snapshots cheap via move semantics
- `[x]` Apply the same pattern to `closeWindow()` callers that invoke user callbacks while holding partition locks in `cep_engine.cpp` lines 428–440
- `[x]` `metricsLoop()` (line 2403) uses bare `std::this_thread::sleep_for(config_.metrics_interval)` — replace with a `condition_variable::wait_for` so the thread wakes immediately on `running_ = false`; current implementation can delay engine shutdown by one full `metrics_interval`
- `[x]` Add a regression test that calls `CEPEngine::stop()` and asserts it returns within 100 ms regardless of `metrics_interval` value

**Performance Targets:**
- `CEPEngine::stop()` must return within ≤ 100 ms on all background threads

---

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
- `[x]` Extract a private `snapshotWindow()` helper that copies the deque under a brief lock scope and returns a `std::vector<DataPoint>` — lock is released before calling `train()` or `predict()`
- `[x]` Gate retrain (`retrain_on_window`) behind an `std::atomic<bool> retraining_` flag and schedule training on a dedicated background thread using `std::async(std::launch::async, …)` to keep `process()` non-blocking
- `[x]` `detector_.predict(point)` is stateless once trained — hold only a `std::shared_lock<std::shared_mutex>` during prediction and upgrade to `unique_lock` only when `isTrained()` state changes
- `[x]` `getAnomalies()` (line 1080) and `getStats()` (line 1085/1090) each take their own `lock_guard` — these are read-only accessors; use `shared_lock` for them
- `[x]` Add a concurrency stress test: 8 producer threads calling `process()` at 100 kHz; assert P99 latency ≤ 1 ms with no deadlocks

**Performance Targets:**
- `process()` lock-hold duration: ≤ 50 µs (deque copy only; training async)
- Training throughput: IsolationForest on 1 000-point window ≤ 10 ms

---

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
- `[x]` Restructure `predict()` to: (1) take `shared_lock` for a brief pointer/ref capture of `*it->second`, (2) release `shared_lock`, (3) run `e.model.predictOne(point)` outside any registry lock, (4) take only `e.health_mu` for the health-metric update
- `[x]` Use a `std::shared_ptr<Entry>` inside the registry so callers can retain a reference-counted handle after releasing the registry lock — eliminates the use-after-free risk from concurrent `unregisterModel()`
- `[x]` Apply the same pattern to `predictBatch()` (line 244), `explain()` (line 283), and `evaluate()` (line 379) which exhibit the same lock-held-during-compute pattern
- `[x]` Add a benchmark: 16 concurrent `predict()` callers on the same model; assert throughput ≥ 10 000 predictions/s per core

**Performance Targets:**
- Registry lock-hold per prediction: ≤ 5 µs (pointer capture only)
- Inference throughput (decision tree depth=10): ≥ 500 000 predictions/s on 8 cores

---

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
- `[x]` Replace the double-lock pattern with a single lock acquisition that obtains a `shared_ptr<OrtSession>` reference (or equivalent), then releases the mutex before calling ONNX `Run()`
- `[x]` Move the session map from `std::map<string, unique_ptr<Session>>` to `std::map<string, shared_ptr<Session>>` so per-model handles can be retained outside the map lock
- `[x]` Per-model `std::shared_mutex` (or `std::atomic<bool> loading_`) to serialize concurrent loads of the same model without blocking unrelated models
- `[x]` Add test: two threads simultaneously infer on two different models; assert neither blocks the other

**Performance Targets:**
- Lock-hold per inference call: ≤ 5 µs (handle capture only)
- Two independent-model inferences: must proceed concurrently with no serialization

---

### 6 · `IncrementalView::applyChanges()` — Exclusive Lock for Entire Batch
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/incremental_view.cpp` lines 325–400

`applyChanges(const std::vector<ChangeRecord>& changes)` acquires `unique_lock lk(rw_mutex_)`
at line 325 and holds it for the entire iteration over `changes`, which may contain
thousands of records.  Concurrent readers (`query()` at line 371 uses `shared_lock`) are
blocked for the full batch duration, violating the 50 ms IVM constraint when batches exceed
a few hundred rows under load.

`applyChange()` (single-record path, line 284) exhibits the same pattern: the unique lock
spans `passesBaseFilters()`, `applyRow()`, and `pruneEmptyGroup()`, all of which involve
`unordered_map` lookups and string parsing.

**Implementation Notes:**
- `[x]` In `applyChanges()`, process changes in micro-batches of ≤ 256 rows: acquire `unique_lock`, apply micro-batch, release, yield with `std::this_thread::yield()`, repeat — readers can slip in between micro-batches
- `[x]` Pre-compute `passesBaseFilters()` outside the write lock using a read-only snapshot of `def_` (immutable after construction); only `applyRow()` and `pruneEmptyGroup()` need the exclusive lock
- `[x]` Add a read-latency regression test: background writer calls `applyChanges(10 000 rows)` while a reader thread calls `query()` in a tight loop; assert reader P99 ≤ 10 ms

**Performance Targets:**
- Reader P99 latency during a 10 000-row batch apply: ≤ 10 ms
- `applyChanges()` throughput: ≥ 200 000 rows/s

---

### 7 · `LLMProcessAnalyzer` — O(N) Cache Eviction Under Lock
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/llm_process_analyzer.cpp` lines 93–115, 515–530

**7a – O(N) eviction:** `putInCache()` (line 93) holds `cache_mutex` and scans all 1 000
entries linearly to find the one with the earliest expiry (lines 105–112).  Under high LLM
call rates this becomes a serialization bottleneck.  The hard-coded limit `1000` (line 105)
is not configurable from `LLMConfig`.

**7b – Expensive cache-key serialization:** `getCacheKey()` (line 515) calls
`request.process_trace.dump()` which serializes the full `nlohmann::json` object to a string
on every call — even for cache hits.  For large process traces (hundreds of events) this can
take several milliseconds in the hot request path.

**Implementation Notes:**
- `[x]` Replace `std::unordered_map<string, CacheEntry>` + manual linear eviction with an `LRUCache<string, nlohmann::json>` backed by a doubly-linked list and hash map, giving O(1) get/put/evict — the pattern already proposed in the OLAP section above, or a simple `boost::compute::detail::lru_cache` adapter
- `[x]` Expose `max_cache_entries` in `LLMConfig` (default 1 000) so operators can tune without recompiling
- `[x]` In `getCacheKey()`, compute a SHA256 digest of `request.process_trace.dump()` rather than embedding the full dump string in the key — the key comparison and hash-map lookup are O(1) for fixed-size SHA256 digests; key building is still O(trace_size) once per request, but large JSON blobs no longer live inside the map keys
- `[x]` Add a microbenchmark: `putInCache()` with 1 000 existing entries must complete in ≤ 1 µs

**Performance Targets:**
- `putInCache()` / `getFromCache()`: O(1) amortised, ≤ 1 µs P99 under 16 concurrent callers
- `getCacheKey()` for a 500-event trace: ≤ 50 µs (hash-based, not JSON dump)

---

### 8 · `DistributedAnalyticsSharding::getHealthyShardCount()` — Network I/O Under Lock
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/distributed_analytics.cpp` lines 317–325

`getHealthyShardCount()` acquires `mutex_` (line 317) and calls
`e.executor->isHealthy()` for every shard entry (line 321).  `ShardQueryExecutor::isHealthy()`
is a virtual call on a remote executor abstraction — in production implementations this
involves a network ping or gRPC health-check.  Holding `mutex_` for the entire health-check
sweep blocks `addShard()`, `removeShard()`, `getShardIds()`, and the scatter-gather
`executeOnAllShards()` for the full network round-trip multiplied by the shard count.

**Implementation Notes:**
- `[x]` Introduced `ShardEntry::cached_healthy` (`shared_ptr<atomic<bool>>`) updated by a background health-monitor thread; `getHealthyShardCount()` reads the cached value under the lock (< 1 µs) instead of doing live checks
- `[x]` Background health monitor runs at a configurable `health_check_interval` (default 5 s); uses its own dedicated mutex so it does not contend with the main `mutex_`
- `[x]` Exposed `getHealthyShardCountAsync() → std::future<size_t>` for callers that explicitly want live health data without blocking the shard registry
- `[x]` Test added: simulate one shard health check that takes 500 ms; assert `addShard()` completes within 5 ms during the health check

**Performance Targets:**
- `getHealthyShardCount()` (cached path): ≤ 2 µs
- Health monitor cycle for 64 shards: ≤ 5 s wall time with per-shard 1 s timeout

---

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
- `[x]` Add an in-flight-request set (`std::unordered_set<std::pair<int64_t,int64_t>>`) so the second caller for the same range waits on a `condition_variable` rather than re-computing
- `[x]` Pass `from_sequence` and `to_sequence` as bounds to `changefeed_.listEvents()` when the `Changefeed::ListOptions` struct supports it — avoids materializing the entire event log
- `[x]` Replace raw `listEvents(…); filter in loop` pattern with a binary-search or indexed range query when the changefeed is backed by a sorted store
- `[x]` `evictOldCacheEntries()` (called while holding `cache_mutex_` at line 217) performs an unguarded iteration — apply the same copy-evict-then-lock pattern to keep lock duration short

**Performance Targets:**
- `computeDiff()` cache-miss path for a 1 M-event log, range [N-1000, N]: ≤ 50 ms
- Stampede prevention: second concurrent caller for the same range must wait ≤ 5 ms

---

### 10 · `automl.cpp` — `KNNRegressorModel::predictOneReg()` Stub
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0)
**Files:** `src/analytics/automl.cpp`

`KNNModel::predictOneReg()` is fully implemented as a weighted inverse-distance mean of the
`k` nearest neighbours' target values (weight = 1/d² where d² is the squared L2 distance;
threshold on d² > 1e-15 before applying weight, else w = 1e15).
The `neighbors()` private helper uses squared L2 distance with `std::nth_element` for O(n)
nearest-neighbour selection.

**Implementation Notes:**
- `[x]` Implement `predictOneReg()` as the weighted mean of the `k_` nearest neighbours' target values, using the existing `neighbors()` helper in `KNNModel`
- `[x]` Unit test added: train a KNN model on `y = 2x` with 100 training points; `predictOneReg({5.0})` returns a value within ±0.5 of 10.0 (`KNNRegressionTest.PredictOneRegLinearRelation`)
- `[x]` Opt-in performance test added: `KNNRegressionTest.PredictOneRegPerformance` (enabled via `THEMIS_RUN_PERF_TESTS=1`)

**Performance Targets:**
- `predictOneReg()` for k=5 on a 10 000-sample training set: ≤ 1 ms

---

### 11 · `CEPEngine::computePercentile()` — Pass-by-Value Copy in Hot Path
**Priority:** Medium
**Target Version:** v1.8.0 ✅ Resolved
**Files:** `src/analytics/cep_engine.cpp` line 140

```cpp
double computePercentile(std::vector<double> vals, double p) {
```

The function signature takes `vals` by value, forcing a full heap copy of the event-window
data on every call.  For a 10 000-event window this allocates and copies 80 KB per
percentile computation.  This function is called from `AggregationWindow::computeValue()`
in the hot event-processing path.

**Implementation Notes:**
- `[x]` Change signature to `computePercentile(const std::vector<double>& vals, double p)` — pass-by-const-ref; internal scratch copy is made once inside the shared utility
- `[x]` The same pattern in `streaming_window.cpp` (`calcPercentile`) is also fixed — both now delegate to `themis::analytics::detail::computePercentile` defined in `include/analytics/detail/stats.h`
- `[x]` `include/analytics/detail/stats.h` added with `computePercentile(const std::vector<double>&, double)` and `computePercentile(std::span<const double>, double)` overloads

**Performance Targets:**
- Copy elimination: ≥ 50 % reduction in heap allocations on the CEP event-processing hot path

---

### 12 · Windows Platform Stubs — `olap.cpp` and `process_mining.cpp`
**Priority:** Medium
**Target Version:** v2.0.0
**Files:** `src/analytics/olap.cpp` lines 53–100; `src/analytics/process_mining.cpp` lines 24–end
**Status:** ✅ Completed (v2.0.0)

`olap.cpp` previously compiled an entire no-op `OLAPEngine` on `_WIN32`.  The whole-class
stub has been removed: the full cross-platform implementation is now active on all platforms
(SIMD intrinsics remain guarded per-instruction via `#if defined(__AVX512F__)` etc.).
`process_mining.cpp` Windows stub remains gated behind the opt-in flag
`THEMIS_PROCESS_MINING_WINDOWS_STUB` and now emits `spdlog::error` for every call.
Arrow-absent export stubs emit `spdlog::warn` instead of silently returning `false`.

**Implementation Notes:**
- `[x]` Audit `OLAPEngine` for Windows-specific blockers — no POSIX `mmap`/`pread` calls
  were found; SIMD intrinsics are already guarded by their own `#if defined(__AVX512F__)`
  / `#if defined(__AVX2__)` / `#if defined(__ARM_NEON)` blocks.  The whole-class
  `#if defined(_WIN32)` stub has been removed entirely.
- `[x]` CMake CI job for Windows (MSVC 2022 + vcpkg) added at
  `.github/workflows/02-feature-modules_analytics_windows-olap-ci.yml`; builds
  `test_olap_lru_cache_focused` and runs it via CTest on `windows-latest`.
  A pre-build static audit step verifies that no whole-class `_WIN32` stub is
  re-introduced and that the `Stubs:` counter in the file header is ≤ 2.
- `[x]` `exportToParquet()` / `exportCollectionToParquet()` now emit `spdlog::warn(...)` when Arrow is not compiled in (`olap.cpp` `#else` block); `throwArrowUnavailable()` in `analytics_export.cpp` also emits `spdlog::warn` before throwing
- `[x]` `ProcessMining` Windows stub now calls `spdlog::error(...)` before returning `Status::Error` — operators see a log entry when the capability is absent
- `[x]` `olap.cpp` file-header `Stubs:` counter updated from 4 → 2 (only the two
  Arrow-absent export stubs remain); Windows CI workflow enforces this limit ≤ 2.

**Performance Targets:**
- Full `OLAPEngine::execute()` on Windows: feature-parity with Linux for non-SIMD code paths

---

### 13 · `streaming_window.cpp` — 8 Open TODOs + Hard-coded Poll Intervals
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/streaming_window.cpp` (header reports `TODOs: 8`)

The file header (line 14) self-reports 8 open TODOs and scores 85/100 for quality.  Two
concrete structural issues are observable:

**13a – Hard-coded expiry poll intervals:**
- `SessionWindow::expiryLoop()` line 792: `expiry_cv_.wait_for(lk, std::chrono::milliseconds(200), …)` — 200 ms is hard-coded
- `WindowManager::timerLoop()` line 1073: `timer_cv_.wait_for(lk, std::chrono::milliseconds(500), …)` — 500 ms is hard-coded

These intervals control session-gap detection resolution and GLOBAL-window emission
latency, respectively.  Operators with sub-second SLAs cannot tune them without
recompiling.

**13b – `timerLoop()` holds `windows_mutex_` while calling user `callback_`:**
Lines 1079–1085 lock `windows_mutex_`, iterate windows, and invoke `callback_(w.events, …)`
inside the lock — the same pattern described in section 2 for CEP, but in the streaming
window layer.

**Implementation Notes:**
- `[x]` Add `session_expiry_check_interval_ms` and `global_window_emit_interval_ms` fields to `WindowConfig` (default 200 ms and 500 ms respectively) — pass them to `wait_for` in `expiryLoop()` and `timerLoop()` instead of literals
- `[x]` In `timerLoop()`, collect `(events_copy, start, now)` snapshots into a local vector under `windows_mutex_`, release the lock, then call all callbacks on the snapshot (already implemented in cep_engine.cpp — snapshot-then-dispatch pattern present before this change; marked complete)
- `[x]` Identify and document all 8 open TODOs in a `KNOWN_ISSUES.md` or inline comments so they are trackable in code review; the file-header counter is not sufficient
- `[x]` Add a test asserting that `SessionWindow` emits a result within `gap + expiry_check_interval_ms + 50 ms` of the last event — validates the configurable interval end-to-end

**Performance Targets:**
- Session expiry detection latency: `gap + config.session_expiry_check_interval_ms ± 20 ms`

---

### 14 · SIMD Vectorization — AVX-512 and ARM NEON
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/olap.cpp`, `src/analytics/columnar_execution.cpp`, `src/analytics/forecasting.cpp`

The existing SIMD acceleration covers AVX2 for aggregation kernels in `olap.cpp` and the
Yule–Walker autocovariance loop in `forecasting.cpp`.  AVX-512 (2× AVX2 width for double)
and ARM NEON (Cortex-A78 and Apple Silicon) paths are absent.

**Implementation Notes:**
- `[x]` Add `#ifdef __AVX512F__` path in `olap.cpp` `vectorizedSum/Avg/Min/Max` — process 8 `double` per cycle vs AVX2's 4; use `_mm512_reduce_add_pd` for horizontal reduction
- `[x]` Add `#ifdef __ARM_NEON` path with `float64x2_t` NEON intrinsics for `ColumnAggregator` in `columnar_execution.cpp` — ARM builds currently fall back to scalar
- `[x]` Gate all SIMD paths behind runtime CPUID checks (`__builtin_cpu_supports("avx512f")`) when the binary must run on heterogeneous hardware
- `[x]` Extend `forecasting.cpp` Yule–Walker AVX2 inner loop to AVX-512 (8 doubles/cycle) for the `acov0_avx2` function already scaffolded in the existing doc
- `[x]` ARM NEON and AVX2 results must produce bit-identical output (within 1 ULP) to the scalar baseline — add a parity assertion in the CI test suite

**Performance Targets:**
- AVX-512 SUM over 10 M doubles: ≥ 2× throughput vs AVX2 baseline
- ARM NEON aggregation throughput: ≥ 4 GB/s on Cortex-A78

---

### 15 · Memory Pool Allocator for Hot Analytics Paths
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/olap.cpp`, `src/analytics/columnar_execution.cpp`, `src/analytics/cep_engine.cpp`

Repeated `std::vector` construction/destruction for intermediate aggregation buffers (group
key maps, scratch arrays in `ColumnarAggregator::execute()`, `CEPEngine::workerLoop()`
event copies) causes frequent heap allocations in the hot path.

**Implementation Notes:**
- `[x]` Introduce `AnalyticsMemoryPool` (arena allocator, initial size 64 MB) in `src/analytics/detail/memory_pool.h` with `allocate(size, align)` and `reset()` — no individual free, reset per query
- `[x]` Wire pool into `OLAPEngine::Impl` and `ColumnarAggregator` so intermediate group-key strings and `AggState` maps allocate from the pool; `pool_.reset()` at the start of each `execute()` call
- `[x]` For `CEPEngine`, use a lock-free ring buffer (SPSC if single producer, MPSC if multi) for the event queue rather than `std::queue<std::pair<string,Event>>` — eliminates per-event `std::string` copy for the stream_id
- `[x]` Ensure the pool is not shared across threads; each `OLAPEngine::Impl` thread gets its own pool or uses thread-local storage

**Performance Targets:**
- Allocation overhead in `OLAPEngine::execute()`: ≤ 5 % of total query time (currently estimated 15–30 % for GROUP BY with many groups)

---

### 16 · Forecasting: Batch Prediction, Streaming Update, SIMD Fit
**Priority:** Medium
**Target Version:** v1.9.0
**Files:** `src/analytics/forecasting.cpp`, `include/analytics/forecasting.h`

The forecasting engine supports `fit()` + `predict(steps)` but lacks the following
capabilities needed for production deployments.

**Implementation Notes:**
- `[x]` Add `predictBatch(const std::vector<TimeSeries>& batch, int steps) → std::vector<std::vector<ForecastPoint>>` to amortise model-state copies across independent series — existing `predict()` re-copies internal state on every call
- `[x]` Add `update(double new_value)` for O(1) one-step incremental absorption of a new observation without full `fit()` rerun — update only the ETS level/trend/seasonal components
- `[x]` Auto-tune (HES `auto_tune=true`) grid search over alpha/beta/gamma is single-threaded — parallelize with `std::async` or OpenMP; 9-point grid on 500-sample series currently takes up to 50 ms single-threaded
- `[x]` Cache last `fit()` result indexed by `(xxHash(training_data), config_hash)` so repeated fits on unchanged data are O(1) hash lookups
- `[x]` Extend the existing AVX2 Yule–Walker scaffold to a compiled-in AVX-512 path (see section 14)

**Performance Targets:**
- `predictBatch()` for 1 000 series × 30 steps: ≤ 50 ms on a single core
- `update(new_value)`: O(1), ≤ 10 µs per call
- Auto-tune grid (9 α, n=500): ≤ 5 ms with parallel search

---

### 17 · Arrow Zero-Copy Integration and Result Cache with LRU Eviction
**Priority:** Medium
**Target Version:** v1.8.0
**Files:** `src/analytics/analytics_export.cpp`, `src/analytics/olap.cpp`, `src/analytics/arrow_export.cpp`

`analytics_export.cpp` line 341 allocates a `std::vector<uint8_t> chunk(data.begin()+offset, …)` for every chunk during Arrow IPC streaming — unnecessary copy when the source buffer is already contiguous.  The OLAP result cache in `olap.cpp` can grow unbounded (no eviction policy).

**Implementation Notes:**
- `[x]` Use `arrow::Buffer::Wrap()` or `arrow::MutableBuffer` zero-copy wrappers instead of copying bytes into `std::vector<uint8_t>` during Arrow IPC serialization in `analytics_export.cpp` line 341
- `[x]` Implement `LRUCache<std::string, OLAPResult>` (doubly-linked list + `unordered_map`, max 1 000 entries configurable) for OLAP query result caching — current implementation has no eviction
- `[x]` Cache key for OLAP must be computed from a normalized query representation (sorted dimensions, canonical filter order) so semantically equivalent queries hit the same entry
- `[x]` Add TTL-based invalidation: cached entries older than `cache_ttl_ms` (configurable, default 60 s) are evicted on next access or by a background cleanup thread

**Performance Targets:**
- Arrow IPC export copy overhead: ≤ 1 % of total export time (zero-copy path)
- OLAP cache hit rate for repeated identical queries: ≥ 80 % in typical dashboard workloads

---

## Implementation Phases

### Phase 1 — Design / API Contracts (2026 Q3)
- `[x]` Define `IFormatExporter` hierarchy and finalize `ExporterFactory` dispatch API (section 1)
- `[x]` Draft `LRUCache<K,V>` utility header in `include/analytics/detail/lru_cache.h` (sections 7, 17)
- `[x]` Define `AnalyticsMemoryPool` API (section 15)
- `[x]` Add `session_expiry_check_interval_ms` / `global_window_emit_interval_ms` to `WindowConfig` (section 13)
- `[x]` Add `max_cache_entries` to `LLMConfig` (section 7)

### Phase 2 — Core Implementations (2026 Q4)
- `[x]` Implement per-format `IAnalyticsExporter` classes; retire `StubAnalyticsExporter` (section 1)
- `[x]` Refactor `StreamingAnomalyDetector::process()` to async training (section 3)
- `[x]` Refactor `ModelServingEngine::predict()` to inference-outside-lock pattern (section 4)
- `[x]` Implement `LRUCache` in `llm_process_analyzer.cpp` (section 7)
- `[x]` Implement `KNNRegressorModel::predictOneReg()` via `KNNModel` (section 10)
- `[x]` Fix `CEPEngine::timerLoop()` callback-under-lock and `metricsLoop()` shutdown race (section 2)

### Phase 3 — Error Handling and Edge Cases (2027 Q1)
- `[x]` Add spdlog warnings to all silent Arrow/Windows `return false` stubs (section 12)
- `[x]` TOCTOU fix for `MLServingEngine::infer()` (section 5)
- `[x]` Stampede prevention for `DiffEngine::computeDiff()` (section 9)
- `[x]` `DistributedAnalyticsSharding` cached health state (section 8)
- `[x]` `IncrementalView::applyChanges()` micro-batch lock release (section 6)

### Phase 4 — Tests (2027 Q1)
- `[x]` Concurrency stress test for `StreamingAnomalyDetector` (8 threads, 100 kHz, P99 ≤ 1 ms) — `tests/analytics/test_anomaly_detection.cpp` `StreamingConcurrencyStress::EightProducersP99Latency` (run with `THEMIS_RUN_PERF_TESTS=1`)
- `[x]` OLAP cache eviction test: assert bounded memory growth under 10 000 unique queries — `tests/analytics/test_olap_lru_cache.cpp` `OLAPLRUCache::BoundedMemoryGrowthUnder10kUniqueQueries` (Linux RSS assertion + cross-platform functional variant)
- `[x]` `CEPEngine::stop()` latency test: returns within 100 ms regardless of `metrics_interval`
- `[x]` `IVM` reader-latency test: P99 ≤ 10 ms during 10 000-row batch apply
- `[x]` `KNNRegressorModel` regression accuracy test on `y = 2x`

### Phase 5 — Performance / Hardening (2027 Q2)
- `[x]` AVX-512 and ARM NEON kernels with CI parity assertions (section 14)
- `[x]` `AnalyticsMemoryPool` integration in OLAP and columnar execution (section 15)
- `[x]` `computePercentile` pass-by-value elimination (section 11)
- `[x]` Zero-copy Arrow IPC export (section 17)
- `[x]` Forecasting batch prediction and streaming update API (section 16)

### Phase 6 — Documentation and Sign-off (2027 Q2)
- `[ ]` Update `README.md` performance numbers after Phase 5 benchmarks
- `[x]` Document all resolved TODOs in `streaming_window.cpp` header (TODO #6 resolved)
- `[x]` Update `include/analytics/FUTURE_ENHANCEMENTS.md` to reflect new public API additions (v1.8.0–v1.9.0 APIs; completed feature statuses)
- `[ ]` Add Windows CI job and set stub-count CI gate to 0 for non-Windows builds (section 12)

---

## Production Readiness Checklist

- `[x]` `ExporterFactory` returns correct type for every `ExportFormat` value
- `[x]` All `std::lock_guard` scopes verified to hold ≤ 1 ms under worst-case production load
- `[x]` `CEPEngine::stop()` completes within 100 ms in all code paths
- `[ ]` `ModelServingEngine` inference throughput ≥ 10 000 predictions/s on 8 cores
- `[x]` `IncrementalView` reader P99 ≤ 10 ms under 10 000-row batch writes
- `[x]` Windows `OLAPEngine` stubs emit spdlog::error; `ProcessMining` Windows stub now logs via spdlog::error
- `[x]` `KNNRegressorModel::predictOneReg()` stub replaced with real implementation (via `KNNModel`)
- `[x]` All hard-coded poll intervals (200 ms, 500 ms, 100 ms) moved to configuration structs
- `[x]` `LLMProcessAnalyzer` cache eviction O(1)
- `[x]` SIMD parity tests passing on AVX2 + scalar; AVX-512 and NEON paths added

---

## Known Issues and Limitations

| Issue | File | Severity | Notes |
|-------|------|----------|-------|
| `ExporterFactory` always returns stub | `analytics_export.cpp:728` | High | Parquet/Feather silently unavailable without error |
| Training under `StreamingAnomalyDetector` lock | `anomaly_detection.cpp:1051` | High | O(N²) LOF train blocks all producers |
| ONNX inference under global `sessions_mutex` | `ml_serving.cpp:190` | High | Serializes all model inferences |
| Inference under registry `shared_lock` | `model_serving.cpp:206` | High | Starves writers during long inference |
| User callback under `windows_mutex_` | `cep_engine.cpp:1082` | High | Any slow callback freezes the CEP window layer |
| O(N) LLM cache eviction under lock | `llm_process_analyzer.cpp:105` | Medium | Degrades under high LLM call rates |
| Network I/O in `getHealthyShardCount()` | `distributed_analytics.cpp:321` | Medium | ✅ Fixed v1.8.0: background monitor + cached_healthy atomic |
| Cache stampede in `DiffEngine` | `diff_engine.cpp:181` | Medium | Two threads can duplicate expensive changefeed scan |
| `KNNRegressorModel::predictOneReg()` = 0.0 | `automl.cpp:833` | Medium | ✅ Fixed v1.8.0: weighted inverse-distance mean of k nearest neighbours |
| 8 unresolved TODOs | `streaming_window.cpp` | Medium | Enumerated as inline TODO(v1.8.0) comments in file header (§13 resolved) |
| Windows OLAP/ProcessMining stubs | `olap.cpp:53`, `process_mining.cpp:24` | Low | Not a blocker on Linux; silently fails on Windows |
| `computePercentile` by-value copy | `cep_engine.cpp:140` | Low | 80 KB copy per percentile on 10k-event windows |

---

## Breaking Changes

None expected through v1.9.0 — all changes are either internal refactors or additive API
extensions.  The `WindowConfig` struct additions (section 13) and `LLMConfig.max_cache_entries`
(section 7) are backwards-compatible with default values matching current hard-coded constants.

---

## Test Strategy

- **Unit tests** (≥ 90 % line coverage per file): each fix in sections 1–13 must have a corresponding isolated test in `tests/analytics/`
- **Concurrency tests**: `StreamingAnomalyDetector` (8 producers, 100 kHz), `ModelServingEngine` (16 concurrent predictors), `IncrementalView` (writer + 4 readers)
- **Regression benchmarks** (Google Benchmark): tracked for `OLAPEngine::execute`, `CEPEngine` event throughput, `IncrementalView::applyChanges`, `computePercentile` — PRs blocked on ≥ 5 % regression
- **Platform tests**: Linux x86_64 (AVX2 + AVX-512 if available), ARM64, Windows 2022 MSVC
- **Parity tests**: AVX-512 / ARM NEON vs scalar results, tolerance ≤ 1 ULP

---

## Performance Targets

| Operation | Current (estimated) | Target |
|-----------|--------------------|----|
| `ModelServingEngine::predict()` (8-core, decision tree depth 10) | ~20 000/s (lock-serialized) | ≥ 500 000/s |
| `IncrementalView::applyChanges()` reader P99 during 10k-row batch | ~500 ms | ≤ 10 ms |
| `StreamingAnomalyDetector::process()` lock-hold | ~10 ms (includes train) | ≤ 50 µs |
| `LLMProcessAnalyzer` cache put/get | O(N) | ✅ O(1) ≤ 1 µs (v1.8.0) |
| `DiffEngine::computeDiff()` cache-miss (1 M event log, range 1000) | ~500 ms | ≤ 50 ms |
| AVX-512 SUM over 10 M doubles | N/A (unimplemented) | ≥ 2× AVX2 |
| `forecasting.cpp` auto-tune (9α, n=500) | ~50 ms single-thread | ≤ 5 ms parallel |

---

## Security / Reliability

- All SIMD code paths compiled with `-fstack-protector-strong`; no pointer arithmetic on user-controlled offsets
- GPU kernel launches gated behind `GPUKernelValidator` checksum registry when GPU support is enabled
- IVM delta messages validated for schema conformance before `applyChange()` — invalid deltas rejected with `EINVAL`, never silently ignored
- Streaming aggregation enforces a configurable row-count hard cap (default 10 M rows/window) to prevent OOM via adversarial input
- `LLMProcessAnalyzer` API key sanitised from all log output; existing sanitization in `analytics_export.cpp` must be extended to cover the retry-path exception messages
- All public API functions return `Result<T>` / status codes; exceptions must not propagate across module boundaries into the query executor

---

## See Also

- **API Enhancements**: [`../../include/analytics/FUTURE_ENHANCEMENTS.md`](../../include/analytics/FUTURE_ENHANCEMENTS.md)
- **Current Implementation**: [`README.md`](./README.md)
- **Architecture**: [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- **Roadmap**: [`ROADMAP.md`](./ROADMAP.md)
- **Performance Guide**: [`../../docs/de/analytics/performance_guide.md`](../../docs/de/analytics/performance_guide.md)

---

## Expert System Engine
**Priority:** High
**Target Version:** v2.1.0
**Issue:** #PLANNED

Aufbauend auf der bestehenden CEP-Engine (`cep_engine.cpp`) mit NFA-Pattern-Matching, EPL-Parser
und Rule-Engine soll eine vollwertige **Expertensystem-Engine** entstehen.  Die CEP-Komponenten
bilden das Regelausführungssubsystem (Working Memory + Agenda + NFA-Matcher); `ExpertSystemEngine`
ergänzt sie um eine persistente Wissensbasis, Vorwärts-/Rückwärtsverkettung und eine
Erklärungskomponente.

### Scope
- `include/analytics/expert_system_engine.h` (new)
- `src/analytics/expert_system_engine.cpp` (new)
- `include/analytics/knowledge_base.h` (new)
- `src/analytics/knowledge_base.cpp` (new)
- Integration: `src/analytics/cep_engine.cpp` (Rule-Execution-Layer), `src/analytics/model_serving.cpp` (ML-Scorer)
- Integration: `src/graph/knowledge_graph_reasoner.cpp` (Wissensgraph-Fakten)

### Design Constraints
- `[ ]` Working Memory: max 10 000 aktive Fakten (Ring-Eviction bei Überschreitung)
- `[ ]` Regelwerk: max 100 Horn-Klausel-Regeln; Laden aus YAML-Datei oder programmatisch
- `[ ]` Vorwärtsverkettungs-Zyklus ≤ 50 ms für 10 000 Fakten + 100 Regeln
- `[ ]` Rückwärtsverkettung ≤ 20 ms für Tiefe ≤ 10 (Depth-Limited-Search)
- `[ ]` Erklärungsgeneration ≤ 10 ms (Proof-Trace als geordnete Regelanwendungssequenz)
- `[ ]` Thread-safety: `assertFact()` und `forwardChain()` via `std::mutex`; `explain()` read-only
- `[ ]` ML-Scorer-Integration optional (`THEMIS_ENABLE_ANALYTICS_ML_SCORER`); deterministischer
  Fallback wenn nicht aktiviert

### Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ExpertSystemEngine::assertFact(fact)` | CDC-Pipeline, AQL-Layer | Schreibt `(subject, predicate, object)` Tripel in Working Memory |
| `ExpertSystemEngine::retractFact(fact_id)` | CDC-Pipeline | Entfernt Fakt; triggert Agenda-Re-Evaluation |
| `ExpertSystemEngine::forwardChain(max_cycles)` | Scheduler, CDC-Callback | Vorwärtsverkettung bis Fixpunkt; gibt Anzahl gefeuerte Regeln zurück |
| `ExpertSystemEngine::queryGoal(goal)` | AQL-Layer | Rückwärtsverkettung; liefert `GoalResult` mit Proof-Trace |
| `ExpertSystemEngine::explain(decision_id)` | Audit-API, Explanation-Endpoint | Exportiert Proof-Trace als JSON-Array von `{rule_id, matched_facts, derived_fact}` |
| `ExpertSystemEngine::setMLScorer(ModelServingEngine*)` | Server-Startup | Registriert ML-Modell für Konfidenz-Scoring von Regelprämissen |
| `KnowledgeBase::loadRulesFromYaml(path)` | Server-Startup, Hot-Reload | Lädt Horn-Klausel-Regelwerk; validiert auf Konsistenz |
| `KnowledgeBase::assertFact(triple)` | Reasoner, CDC | Persistiert Fakt (in-memory + optional RocksDB) |
| `KnowledgeBase::getFacts(predicate)` | Reasoner | Index-Lookup nach Prädikat; O(log N) |

### Wissensrepräsentation — Regelformat (YAML)
```yaml
# Horn-Klausel-Regeln im ThemisDB-Expertensystem-Format
rules:
  - id: compliance_violation_detected
    priority: 10
    description: "Markiert einen Vorfall als Compliance-Verletzung wenn Schwellwert überschritten"
    if:
      - [?incident, type, SecurityIncident]
      - [?incident, severity, critical]
      - [?incident, affected_records, "?count > 1000"]
    then:
      - [?incident, requires_action, compliance_review]
      - [?incident, notification_level, regulatory]
    ml_confidence_threshold: 0.85   # ML-Scorer muss ≥ 0.85 bestätigen

  - id: expert_domain_inference
    priority: 5
    if:
      - [?person, authored, ?document]
      - [?document, hasKeyword, ?keyword]
      - [?keyword, inDomain, ?domain]
    then:
      - [?person, expertIn, ?domain]
```

### Implementation Notes
- `[ ]` `ExpertSystemEngine` hält einen Pointer auf `CEPEngine::RuleEngine` (nicht-owning);
  Horn-Klauseln werden als CEP-Regeln mit EPL-Syntax registriert; der NFA-Matcher dient als
  Rete-ähnliches Muster-Ausführungssubsystem
- `[ ]` Working Memory: `std::unordered_multimap<std::string, Fact>` (Prädikat → Fakten);
  Ring-Eviction via LRU-Verdrängung bei 10 000 Fakten
- `[ ]` `KnowledgeBase` speichert Fakten als `(subject, predicate, object)` Tripel; kompatibel
  mit `KnowledgeGraphReasoner` — Fakten können bidirektional ausgetauscht werden
- `[ ]` Rückwärtsverkettung: Depth-Limited-Search mit max `depth = 10`; zirkuläre Beweise
  werden durch Visited-Set erkannt und als `CycleDetected` abgebrochen
- `[ ]` ML-Scorer-Augmentierung: `ModelServingEngine::predict()` bewertet Regelprämissen;
  Konfidenz < Threshold → Regel als "soft hint" markiert, nicht als harte Entscheidung
- `[ ]` `LoRAPatternClassifier` (s. u.) kann als ML-Scorer verwendet werden

### Test Strategy
- `tests/analytics/test_expert_system_engine.cpp` — ES-01..ES-20
  - ES-01..ES-05: `assertFact` + `forwardChain` (Vorwärtsverkettung bis Fixpunkt)
  - ES-06..ES-10: `queryGoal` Rückwärtsverkettung + Proof-Trace-Serialisierung
  - ES-11..ES-14: ML-Scorer-Integration (Mock `ModelServingEngine`)
  - ES-15..ES-17: Regelkonflikt-Erkennung + `ConflictError`
  - ES-18..ES-20: Concurrency (8 Threads, 10 000 Fakten)
- `tests/analytics/test_knowledge_base.cpp` — KB-01..KB-08
  - KB-01..KB-03: YAML-Laden + Validierung
  - KB-04..KB-05: `assertFact` / `retractFact` Konsistenz
  - KB-06..KB-08: `getFacts(predicate)` Index-Korrektheit

### Performance Targets
- `forwardChain(max=100)` auf 10 000 Fakten + 100 Regeln: ≤ 50 ms
- `queryGoal` Tiefe ≤ 10: ≤ 20 ms
- `explain(decision_id)` Proof-Trace: ≤ 10 ms
- `KnowledgeBase::loadRulesFromYaml` (100 Regeln): ≤ 50 ms

### Security / Reliability
- Regel-YAML wird gegen JSON-Schema validiert bevor Laden; ungültige Regeln → `INVALID_ARGUMENT`
- Keine Shell-Ausführung oder Dateisystemzugriff in Regelaktionen
- ML-Scorer-Konfidenzwerte werden geloggt (Audit-Trail); kein silent-override von Regelergebnissen

---

## AI/ML + LoRA Pattern Classification
**Priority:** High
**Target Version:** v2.1.0 – v2.2.0
**Issue:** #PLANNED

LoRA-fine-tuned LLM-Adapter liefern domänenspezifische Mustererkennung in Ereignisströmen,
Zeitreihendaten und Graphpfaden.  `LoRAPatternClassifier` wrapped `MultiLoRAManager` und
integriert sich in CEP-Engine, ExpertSystemEngine und KnowledgeGraphReasoner.

### Scope
- `include/analytics/lora_pattern_classifier.h` (new)
- `src/analytics/lora_pattern_classifier.cpp` (new)
- Integration: `src/llm/multi_lora_manager.cpp`, `src/analytics/cep_engine.cpp`,
  `src/analytics/model_serving.cpp`, `src/graph/knowledge_graph_reasoner.cpp`

### Design Constraints
- `[ ]` Batch-Klassifikation (≤ 64 Events): ≤ 100 ms
- `[ ]` Adapter-Selektion via Embedding-Ähnlichkeit: ≤ 5 ms
- `[ ]` Guard: `THEMIS_ENABLE_LLM`; AutoML-Fallback (`automl.cpp`) immer aktiv
- `[ ]` Thread-safety: `batchClassify()` reentrant; kein globaler Mutex über LoRA-Inference
- `[ ]` Mustererkennung-Metriken: Precision ≥ 0.90 (Betrug), F1 ≥ 0.88 (Zeitreihen-Anomalie)

### Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `LoRAPatternClassifier::classify(events, adapter_id)` | CEP-Engine, ExpertSystem | Klassifiziert Ereignis-Batch; gibt `PatternResult{label, confidence}` zurück |
| `LoRAPatternClassifier::selectAdapter(context)` | Intern, AQL | Wählt Adapter via Embedding-Cosine-Ähnlichkeit zur Kontext-Domäne |
| `LoRAPatternClassifier::batchClassify(event_batch)` | High-throughput-Pfad | Parallele Klassifikation via Thread-Pool; gibt geordnete `PatternResult`-Liste |
| `CEPEngine::setLoRAPatternClassifier(classifier)` | Server-Startup | Registriert Classifier; ermöglicht `PATTERN CLASSIFIED_AS` EPL-Ausdruck |
| `ExpertSystemEngine::setMLScorer(LoRAPatternClassifier*)` | Server-Startup | LoRA-Classifier als ML-Scorer für Regelprämissen |
| `KnowledgeGraphReasoner::applyLoRAScore(chain, adapter_id)` | Reasoning-Layer | Soft-Plausibility-Scoring für Inferenz-Kanten |

### EPL-Erweiterung für LoRA-Mustererkennung
```sql
-- CEP-Regel: Betrugssequenz via LoRA-Klassifikation
CREATE RULE fraud_sequence_lora
AS SELECT COUNT(*) AS event_count, FIRST(user_id) AS user
FROM STREAM events
WINDOW (TUMBLING 60s)
WHERE CLASSIFIED_AS('fraud_sequence', min_confidence=0.90)
  AND amount > 10000
PATTERN WITHIN 300s
ACTION alert(channel="fraud_ops");

-- CEP-Regel: Compliance-Verstoß mit Expertensystem-Bestätigung
CREATE RULE compliance_expert
AS SELECT *
FROM STREAM audit_events
WHERE EXPERT_SYSTEM_CONFIRMS('compliance_violation_detected', confidence>=0.85)
ACTION db_write(table="compliance_violations"), slack(channel="#legal");
```

### Implementation Notes
- `[ ]` `LoRAPatternClassifier::classify()` baut einen strukturierten Prompt aus Event-Features
  (Typ, Zeitstempel, Werte) und ruft `MultiLoRAManager::generateWithAdapter(adapter_id, prompt)`
  auf; JSON-Antwort enthält `{"label": "...", "confidence": 0.92}`
- `[ ]` `selectAdapter(context)` berechnet Cosine-Ähnlichkeit zwischen Context-Embedding
  (via `IEmbeddingProvider`) und vorregistrierten Adapter-Domänen-Embeddings; wählt Top-1
- `[ ]` `batchClassify()` spawnt Worker-Threads via `std::async`; max 4 parallele LoRA-Calls
- `[ ]` AutoML-Fallback: `MLServingClient::predict()` mit dem aktuell besten AutoML-Modell
  wenn kein LoRA-Adapter verfügbar oder `THEMIS_ENABLE_LLM=OFF`
- `[ ]` Adapter-Training: LoRA-Adapter werden über `IncrementalLoRATrainer` (Training-Modul)
  trainiert; Export via `exportWeights()` + Import via `MultiLoRAManager::loadAdapter()`
- `[ ]` Mustererkennung im Graph: `KnowledgeGraphReasoner::applyLoRAScore()` nutzt
  `LoRAPatternClassifier::classify(graph_context_events, "graph_patterns_v1")`

### Test Strategy
- `tests/analytics/test_lora_pattern_classifier.cpp` — LPC-01..LPC-15
  - LPC-01..LPC-05: Einzelereignis-Klassifikation (Mock `MultiLoRAManager`)
  - LPC-06..LPC-08: Batch-Klassifikation + Thread-Pool-Parallelismus
  - LPC-09..LPC-11: Adapter-Selektion via Cosine-Ähnlichkeit (3 Adapter, 3 Domänen)
  - LPC-12..LPC-13: CEP-Integration (`CLASSIFIED_AS` EPL-Ausdruck)
  - LPC-14..LPC-15: AutoML-Fallback wenn LoRA deaktiviert

### Performance Targets
- Batch-Klassifikation 64 Events: ≤ 100 ms (inkl. LoRA-Inference)
- Adapter-Selektion: ≤ 5 ms
- AutoML-Fallback: ≤ 20 ms pro Event

### Security / Reliability
- LoRA-Adapter-Pfade werden durch `isLoRAPathTrusted()` validiert (`multi_lora_manager.cpp`)
- Klassifikations-Outputs werden nie direkt in Datenbank-Writes ohne menschliche Bestätigung
  oder Confidence-Threshold verwendet
- Adapter-Konfidenzwerte werden im Audit-Log protokolliert

---

## Process Mining Windows Port (Target: Q4 2026)

**Stub:** `src/analytics/process_mining.cpp` — `THEMIS_PROCESS_MINING_WINDOWS_STUB` block  
**Risk:** Windows nodes in a mixed cluster cannot execute process-mining operations. All ProcessMining public methods return `Status::Error` immediately, so BPM conformance checking and Petri-net analysis are unavailable on Windows.

### Scope
- Audit all POSIX dependencies in `process_mining.cpp` and `process_mining.h`:
  - `fork()`/`exec()` — if used, replace with `CreateProcess()` or a cross-platform subprocess library.
  - `mmap()`/`mprotect()` — replace with `MapViewOfFile()` or in-memory alternatives.
  - `pread()`/`pwrite()` — replace with `ReadFile()`/`WriteFile()` with seek.
- Remove `THEMIS_PROCESS_MINING_WINDOWS_STUB` CMake option once all blockers are resolved.
- Add `test_process_mining_windows.yml` CI workflow on `windows-latest`.

### Design Constraints
- Cross-platform abstraction must not change the public API in `process_mining.h`.
- Windows build must pass the full `ProcessMiningTests` test suite (`tests/test_process_mining.cpp`).
- BPMN runtime and Petri-net evaluator must produce bit-identical results on Windows and Linux for deterministic event logs.

### Test Strategy
- Windows CI: build without `THEMIS_PROCESS_MINING_WINDOWS_STUB`; run all `ProcessMiningTests`.
- Cross-platform parity: same event log → same conformance check output on Linux and Windows.

### Security / Reliability
- Windows subprocess handling must apply the same input validation as the Linux path.
- No `PROCESS_CREATE_NO_WINDOW` races; subprocess output must be captured deterministically.
