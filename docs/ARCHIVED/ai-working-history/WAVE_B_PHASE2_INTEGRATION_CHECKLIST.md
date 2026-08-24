# Wave B Server Phase 2 — Implementation Integration Checklist

**Project**: ThemisDB v2.4.0 GA - Server Performance Hardening  
**Date Created**: 2026-08-18  
**Status**: 🟢 READY FOR INTEGRATION  

---

## Phase 1: Review & Approval (Code Review)

### Code Review Checklist
- [ ] **performance_helpers.h**
  - [ ] Read line 1-100 (header guard, includes, namespace)
  - [ ] Review GenericConnectionPool class (line 40-150)
  - [ ] Review ConnectionGuard class (line 150-200)
  - [ ] Review PreallocatedBuffer class (line 200-350)
  - [ ] Review HTTP2StreamBuffer class (line 350-500)
  - [ ] Review ExportStreamingBuffer class (line 500-600)
  - [ ] Review RopeFrameSerializationCache class (line 600-630)
  - [ ] Verify no undefined behavior or unsafe patterns
  - [ ] Check const-correctness
  - [ ] Verify RAII patterns
  - [ ] Approve for merge

- [ ] **test_server_phase2_focused.cpp**
  - [ ] Review test structure (15 tests total)
  - [ ] Verify GoogleTest integration
  - [ ] Check benchmark definitions
  - [ ] Verify all gaps are tested
  - [ ] Check for memory leaks in tests
  - [ ] Approve for merge

- [ ] **Integration Examples** (PHASE2_INTEGRATION_EXAMPLES.md)
  - [ ] Review Query API integration pattern
  - [ ] Review HTTP/2 integration pattern
  - [ ] Review Rope API integration pattern
  - [ ] Review Export API integration pattern
  - [ ] Verify no breaking API changes
  - [ ] Approve approach

### Approval Sign-Off
- [ ] Architecture Review: Approved
- [ ] Security Review: Approved
- [ ] Performance Review: Approved
- [ ] Ready for Integration: YES

---

## Phase 2: Handler Integration (Development)

### 2.1 Query API Handler (query_api_handler.cpp)

#### Preparation
- [ ] Backup original file: `cp query_api_handler.cpp query_api_handler.cpp.backup`
- [ ] Create feature branch: `git checkout -b feat/phase2-query-optimization`
- [ ] Review current handleQuery() method signature

#### Integration Steps
- [ ] Add include: `#include "server/performance_helpers.h"`
- [ ] Add using statements in class: `using Pool = perf::GenericConnectionPool<Connection>;`
- [ ] In QueryAPIHandler constructor:
  - [ ] Initialize connection pool: `connection_pool_ = std::make_unique<Pool>(32, 256);`
- [ ] In handleQuery() method:
  - [ ] Replace connection allocation with: `pool_->acquire(std::chrono::seconds(5))`
  - [ ] Wrap with ConnectionGuard for RAII cleanup
  - [ ] Test error handling path (timeout case)
- [ ] In formatQueryResponse() method:
  - [ ] Replace string concatenation with PreallocatedBuffer
  - [ ] Use buffer.append() instead of +=
  - [ ] Add capacity estimation before appends
- [ ] In handleBatchQueries() method:
  - [ ] Add all_results.reserve() before loop
  - [ ] Use ConnectionGuard for each connection
  - [ ] Verify exception safety

#### Compilation Check
```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --build build --target themis_core -j8 2>&1 | head -50
```
- [ ] No compilation errors
- [ ] No compilation warnings related to changes

#### Quick Sanity Test
- [ ] Unit test for query handler compiles
- [ ] Basic query execution works
- [ ] Batch query execution works
- [ ] Timeout handling works

### 2.2 HTTP/2 Session Manager (http2_session.cpp)

#### Preparation
- [ ] Backup original: `cp http2_session.cpp http2_session.cpp.backup`
- [ ] Create feature branch: `git checkout -b feat/phase2-http2-optimization`

#### Integration Steps
- [ ] Add include: `#include "server/performance_helpers.h"`
- [ ] In Http2Session class:
  - [ ] Add member: `std::unordered_map<uint32_t, perf::HTTP2StreamBuffer> stream_buffers_;`
  - [ ] Add member: `mutable std::shared_mutex streams_mutex_;`
- [ ] In createStream() method:
  - [ ] Create HTTP2StreamBuffer with 4KB initial capacity
  - [ ] Store in stream_buffers_ map
  - [ ] Lock with shared_mutex
- [ ] In appendToStream() method:
  - [ ] Use shared_lock for read access
  - [ ] Call stream_buffer.append(data, len)
  - [ ] Update last_access_time via timeout tracking
- [ ] In sendHTTP2Frame() method:
  - [ ] Replace individual appends with appendFrame()
  - [ ] Pre-calculate frame size (9 + payload_len)
  - [ ] Use unique_lock for write access
- [ ] In flushStream() method:
  - [ ] Add timeout parameter
  - [ ] Use try_lock_for() with backoff
  - [ ] Return DEADLINE_EXCEEDED on timeout
- [ ] Clean up idle streams based on getIdleDuration()

#### Compilation Check
```bash
cmake --build build --target themis_core -j8 2>&1 | head -50
```
- [ ] No compilation errors
- [ ] No HTTP/2 test failures

#### HTTP/2 Functionality Test
- [ ] Single stream works
- [ ] Multiple concurrent streams work
- [ ] Stream timeout works
- [ ] Frame serialization works

### 2.3 Rope API Handler (rope_api_handler.cpp)

#### Preparation
- [ ] Backup original: `cp rope_api_handler.cpp rope_api_handler.cpp.backup`
- [ ] Create feature branch: `git checkout -b feat/phase2-rope-optimization`

#### Integration Steps
- [ ] Add include: `#include "server/performance_helpers.h"`
- [ ] In RopeApiHandler class:
  - [ ] Add member: `perf::PreallocatedBuffer rope_buffer_;` (4KB init)
  - [ ] Add member: `perf::RopeFrameSerializationCache frame_cache_;`
  - [ ] Initialize in constructor: `rope_buffer_(4096)`
- [ ] In handleRopeSend() method:
  - [ ] Clear buffer at start: `rope_buffer_.clear()`
  - [ ] For each frame:
    - [ ] Check cache first: `frame_cache_.get(cache_key)`
    - [ ] If cache hit, append cached data
    - [ ] If cache miss, serialize and cache: `frame_cache_.put(key, serialized)`
  - [ ] Send entire buffer in one operation
  - [ ] Add timeout to send operation
- [ ] In handleRopeReceive() method:
  - [ ] Add timeout parameter
  - [ ] Return DEADLINE_EXCEEDED on timeout

#### Compilation Check
```bash
cmake --build build --target themis_core -j8 2>&1 | head -50
```
- [ ] No compilation errors
- [ ] Rope protocol tests pass

#### Rope Functionality Test
- [ ] Frame serialization works
- [ ] Cache hit increases performance
- [ ] Timeout works
- [ ] Batch operations work

### 2.4 Export API Handler (export_api_handler.cpp)

#### Preparation
- [ ] Backup original: `cp export_api_handler.cpp export_api_handler.cpp.backup`
- [ ] Create feature branch: `git checkout -b feat/phase2-export-optimization`

#### Integration Steps
- [ ] Add include: `#include "server/performance_helpers.h"`
- [ ] In ExportApiHandler class:
  - [ ] Add member: `std::unique_ptr<perf::ExportStreamingBuffer> export_buffer_;`
  - [ ] In constructor, initialize:
    ```cpp
    auto write_fn = [this](const std::vector<uint8_t>& chunk) -> bool {
        return writeExportChunk(chunk);
    };
    export_buffer_ = std::make_unique<perf::ExportStreamingBuffer>(
        write_fn,
        10485760  // 10MB estimated
    );
    ```
- [ ] In handleExportJsonl() method:
  - [ ] Call export_buffer_->flush() at start
  - [ ] For each record:
    - [ ] Serialize to JSONL
    - [ ] Call export_buffer_->write(data, len)
    - [ ] If returns false (backpressure), sleep 10ms and retry
  - [ ] Call export_buffer_->flush(EXPORT_TIMEOUT) at end
  - [ ] Return error if flush timeout
- [ ] In other export methods, apply same pattern

#### Compilation Check
```bash
cmake --build build --target themis_core -j8 2>&1 | head -50
```
- [ ] No compilation errors
- [ ] Export tests pass

#### Export Functionality Test
- [ ] Small export works
- [ ] Large export works
- [ ] Backpressure activates correctly
- [ ] Timeout works
- [ ] Memory usage reduced

### 2.5 CMakeLists.txt Update

- [ ] Verify test registration already updated (should be automatic)
- [ ] Check if any additional dependencies needed
- [ ] Verify performance_helpers.h is in include path
- [ ] Test build completes successfully

---

## Phase 3: Testing (QA)

### 3.1 Unit Test Execution

```bash
cd /home/runner/work/ThemisDB/ThemisDB/build
ctest -L server_phase2 -V 2>&1 | tee phase2_test_output.log
```

**Expected Results**:
- [ ] All 15 tests pass
- [ ] 0 test failures
- [ ] Execution time < 30 seconds
- [ ] No memory leaks detected

**Tests to Verify**:
- [ ] ConnectionPoolTest (3 tests)
- [ ] ConnectionGuardTest (2 tests)
- [ ] BufferPreallocationTest (3 tests)
- [ ] HTTP2StreamBufferTest (2 tests)
- [ ] ExportStreamingBufferTest (2 tests)
- [ ] RopeFrameCacheTest (2 tests)
- [ ] IntegrationTest (1 test)

### 3.2 Regression Test

```bash
ctest -L server -V 2>&1 | tee full_server_test_output.log
```

**Expected Results**:
- [ ] All existing server tests pass
- [ ] No new test failures
- [ ] No performance regressions > 5%

### 3.3 Integration Test

Create simple integration test:
```cpp
// Test: Connect → Query → Disconnect cycle
for (int i = 0; i < 100; ++i) {
    auto conn = pool.acquire();
    // Execute query
    pool.release(conn);
}
```

- [ ] 100 query cycles work
- [ ] Memory stable (no leaks)
- [ ] No exceptions thrown

### 3.4 Concurrent Stress Test

```cpp
// Test: 16 threads, 1000 queries each
std::vector<std::thread> threads;
for (int i = 0; i < 16; ++i) {
    threads.push_back(std::thread([&pool]() {
        for (int j = 0; j < 1000; ++j) {
            auto conn = pool.acquire();
            // Execute query
            pool.release(conn);
        }
    }));
}
// Wait all threads
```

- [ ] All threads complete successfully
- [ ] No deadlocks
- [ ] No race conditions
- [ ] Memory stable

### 3.5 Exception Safety Test

```cpp
// Test: Exceptions in guard scope
ConnectionGuard<Connection, Pool> guard(conn, pool);
try {
    throw std::runtime_error("Test");
} catch (...) {
}
// Guard should still cleanup
```

- [ ] Connection returned to pool on exception
- [ ] No double-free
- [ ] Guard is RAII-safe

---

## Phase 4: Performance Validation (Benchmarking)

### 4.1 Baseline Measurement (Before Optimization)

```bash
# Build clean baseline
rm -rf build_baseline
cmake -B build_baseline -DCMAKE_BUILD_TYPE=Release
cd build_baseline && make -j8

# Run benchmarks WITHOUT performance_helpers integration
./module_server_test_server_phase2_focused_focused \
  --benchmark_out=baseline_metrics.json \
  --benchmark_out_format=json
```

**Record**:
- [ ] Connection pool acquisition rate baseline
- [ ] Buffer append efficiency baseline
- [ ] HTTP/2 stream operations baseline
- [ ] Export memory usage baseline
- [ ] Rope serialization performance baseline

### 4.2 Optimized Measurement (After Integration)

```bash
# Rebuild with performance_helpers integrated
cd /home/runner/work/ThemisDB/ThemisDB/build
make -j8

# Run benchmarks WITH performance_helpers
./module_server_test_server_phase2_focused_focused \
  --benchmark_out=optimized_metrics.json \
  --benchmark_out_format=json
```

**Record**:
- [ ] Connection pool acquisition rate optimized
- [ ] Buffer append efficiency optimized
- [ ] HTTP/2 stream operations optimized
- [ ] Export memory usage optimized
- [ ] Rope serialization performance optimized

### 4.3 Performance Comparison

Create comparison script:
```python
import json
with open('baseline_metrics.json') as f:
    baseline = json.load(f)
with open('optimized_metrics.json') as f:
    optimized = json.load(f)

# Calculate improvements
for bench in baseline['benchmarks']:
    name = bench['name']
    base_time = bench['cpu_time']
    opt_time = next(b['cpu_time'] for b in optimized['benchmarks'] if b['name'] == name)
    improvement = (base_time - opt_time) / base_time * 100
    print(f"{name}: {improvement:.1f}% improvement")
```

**Expected Results**:
- [ ] Connection Pool: >= 50% improvement
- [ ] Buffer Append: >= 30% improvement
- [ ] HTTP/2 Streams: >= 40% improvement
- [ ] Export Memory: >= 40% reduction
- [ ] Overall Throughput: >= 5% improvement (cumulative)

### 4.4 Performance Verification Checklist

- [ ] Baseline metrics captured
- [ ] Optimized metrics captured
- [ ] Comparison script results generated
- [ ] All metrics show expected improvement or no regression
- [ ] P99 latency regression < 1%
- [ ] Memory usage improvement >= 20%
- [ ] Generate final performance report

---

## Phase 5: Documentation & Release

### 5.1 Documentation Updates

- [ ] Update CHANGELOG.md with Phase 2 improvements
- [ ] Create v2.4.0 release notes
- [ ] Update performance documentation
- [ ] Add performance_helpers.h to API docs
- [ ] Create optimization guide for developers
- [ ] Document any configuration changes

### 5.2 Code Cleanup

- [ ] Remove all .backup files
- [ ] Remove any test/debug code
- [ ] Final code formatting pass
- [ ] Verify no temporary files included

### 5.3 Final Testing

- [ ] Run full test suite one more time: `ctest -V`
- [ ] Run ASan build: `cmake -DENABLE_ASAN=ON && make && ctest`
- [ ] Run UBSan build: `cmake -DENABLE_UBSAN=ON && make && ctest`
- [ ] Run ThreadSanitizer: `cmake -DENABLE_TSAN=ON && make && ctest`
- [ ] All tests pass

### 5.4 Code Review Final Pass

- [ ] All review comments addressed
- [ ] No outstanding issues
- [ ] Architecture reviewer sign-off
- [ ] Security reviewer sign-off
- [ ] Performance reviewer sign-off

### 5.5 Merge & Release

- [ ] Squash commits if needed
- [ ] Merge to v2.4.0-rc1 branch
- [ ] Tag as v2.4.0-rc1
- [ ] Run CI/CD pipeline
- [ ] Approve for GA release
- [ ] Merge to main/master
- [ ] Tag as v2.4.0
- [ ] Create release notes
- [ ] Publish release

---

## Rollback Procedure (If Needed)

### Quick Rollback (< 5 min)

```bash
# Step 1: Revert handler changes
git checkout query_api_handler.cpp http2_session.cpp rope_api_handler.cpp export_api_handler.cpp

# Step 2: Remove performance_helpers usage
# (Comment out #include "server/performance_helpers.h" in affected files)

# Step 3: Rebuild
cmake --build build -j8

# Step 4: Run tests
ctest -V

# Step 5: Verify no errors
echo "Rollback complete"
```

### Staged Rollback (if partial rollback needed)

- [ ] Rollback Query API only (most impactful, easiest)
- [ ] Keep HTTP/2, Rope, Export optimizations
- [ ] Verify stability
- [ ] Gradually re-enable

### Full Rollback (if major issues)

```bash
git revert <commit-hash>
git push
# Deploy from last known good state
```

---

## Success Criteria Final Validation

**Must Pass** (Go/No-Go):
- [ ] All 15 tests pass: ✅ or ❌
- [ ] No compilation errors: ✅ or ❌
- [ ] Throughput improvement >= 5%: ✅ or ❌
- [ ] Memory usage not increased: ✅ or ❌
- [ ] No deadlocks detected: ✅ or ❌

**Should Pass** (Optimization):
- [ ] Throughput improvement >= 10%: ✅ or 🟡
- [ ] P99 latency improvement >= 20%: ✅ or 🟡
- [ ] Memory usage improvement >= 20%: ✅ or 🟡
- [ ] All sanitizers pass: ✅ or 🟡

**Final Gate**: 
- [ ] All "Must Pass" items are ✅
- [ ] Ready for Production Release: YES/NO

---

## Sign-Off

**Integration Status**: 🟢 READY  
**Test Status**: 🟢 READY  
**Performance Status**: 🟢 READY  
**Release Status**: 🟢 READY  

**Can proceed to next phase**: YES

---

**Document**: WAVE_B_PHASE2_INTEGRATION_CHECKLIST.md  
**Version**: 2.4.0  
**Created**: 2026-08-18  
**Status**: 🟢 READY FOR USE
