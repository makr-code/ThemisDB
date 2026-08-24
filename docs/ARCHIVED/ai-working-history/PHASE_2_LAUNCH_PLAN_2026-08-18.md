# Phase 2: Wave B Stabilization — Launch Plan

**Timestamp**: 2026-08-18 06:45 UTC  
**Status**: 🟢 **READY TO LAUNCH**  
**Target Completion**: 2026-08-20 (2 days)  

---

## Phase 2: Server Optimization (Wave B Stabilization)

### Overview

Batch 1 of Phase 2 targets the **Server module optimization** to achieve **+5-15% throughput improvement** on high-concurrency query paths.

**Scope**: 34 CRITICAL gaps across 4 API handler files  
**Implementation Pattern**: Pre-allocation + buffer reservation + connection pool hardening  
**Expected Outcome**: Measurable throughput gains on benchmarks

### Target Files & Gap Breakdown

#### 1. src/server/query_api_handler.cpp (15 gaps)
**Problem Area**: Query execution pipeline bottleneck  
**Key Gaps**:
- Reserve query result buffer before execution (avoid reallocations)
- Pre-allocate result row collection
- Batch query preparation
- Connection pool state caching
- Request/response buffer reuse

**Implementation Pattern**:
```cpp
// Before: Dynamic allocation on each query
std::vector<Row> results;  // allocates/reallocates multiple times

// After: Pre-allocated + reserved
std::vector<Row> results;
results.reserve(EXPECTED_ROWS);  // Single allocation
// + Connection pool state cached
// + Request/response buffers reused
```

**Expected Impact**: 
- Reduced allocations: 20+ → 2-3 per query
- Lower latency variance (P95/P99 improvement)
- Throughput: +5-8%

#### 2. src/server/http2_session.cpp (9 gaps)
**Problem Area**: HTTP/2 stream management  
**Key Gaps**:
- Pre-allocate stream state table
- Buffer pooling for frame assembly
- Window size pre-calculation
- Flow control state caching
- Connection reuse pool

**Implementation Pattern**:
```cpp
// Before: Dynamic allocation per stream
std::unordered_map<uint32_t, Stream> streams;

// After: Pre-allocated + pooled
std::vector<Stream> stream_pool;  // fixed size
std::queue<Stream*> available_streams;  // reuse queue
```

**Expected Impact**:
- Stream creation: O(n) allocations → O(1)
- Connection keepalive efficiency
- Throughput: +3-5%

#### 3. src/server/rope_api_handler.cpp (4 gaps)
**Problem Area**: Rope data structure serialization  
**Key Gaps**:
- Pre-allocate rope output buffer
- Batch serialization operations
- Memory pooling for intermediate buffers
- Cache rope structural metadata

**Implementation Pattern**:
```cpp
// Before: No pre-allocation
std::string output = serializeRope(rope);  // many allocations

// After: Pre-allocated + pooled
Buffer buffer = buffer_pool_.acquire(EXPECTED_SIZE);
serializeRope(rope, buffer);  // single allocation
buffer_pool_.release(buffer);
```

**Expected Impact**:
- Serialization allocations: 10+ → 1-2
- Rope traversal overhead reduced
- Throughput: +2-3%

#### 4. src/server/export_api_handler.cpp (4 gaps)
**Problem Area**: Data export pipeline  
**Key Gaps**:
- Pre-allocate export buffer
- Batch row packing
- Memory pooling for export formats
- Streaming output optimization

**Implementation Pattern**:
```cpp
// Before: Unbounded buffer growth
std::vector<uint8_t> export_data;
for (auto& row : results) {
  export_data += formatRow(row);  // reallocates many times
}

// After: Pre-allocated + streamed
Buffer export_buf = buffer_pool_.acquire(MAX_EXPORT_SIZE);
for (auto& row : results) {
  export_buf.append(formatRow(row));  // single allocation
}
export_buf.flush();
```

**Expected Impact**:
- Export allocations: 50+ → 1-2
- Memory pressure reduced
- Throughput: +2-3%

### Implementation Requirements

#### Pre-Requirements (Verify Before Starting)
- [ ] Understand connection pool architecture (src/server/connection_pool.h)
- [ ] Review buffer pooling patterns from GPU batch (reusable for Server)
- [ ] Identify EXPECTED_ROWS, MAX_BUFFER_SIZE constants per file
- [ ] Verify no breaking changes to public APIs

#### Implementation Order
1. **query_api_handler.cpp** (15 gaps) — foundation for server optimization
2. **http2_session.cpp** (9 gaps) — connection/stream management
3. **rope_api_handler.cpp** (4 gaps) — serialization optimization
4. **export_api_handler.cpp** (4 gaps) — export pipeline optimization

#### Acceptance Criteria

For each file:
- [ ] All identified gaps fixed (allocation counts verified)
- [ ] No public API breaking changes
- [ ] Backward compatible (no behavior change)
- [ ] Performance improvement measurable (benchmark gates)
- [ ] Thread-safe (no new race conditions)
- [ ] Exception-safe (RAII patterns maintained)
- [ ] Zero compiler warnings
- [ ] Test coverage for new code paths

### Test Strategy

**Unit Tests** (per-file):
- Buffer pre-allocation verification
- Pool exhaustion handling (graceful degradation)
- Memory reuse correctness
- Concurrent access patterns
- Performance regression checks

**Integration Tests**:
- End-to-end query → export pipeline
- Connection pool stress (high concurrency)
- Buffer pool resource limits
- Benchmark baseline measurements

**Performance Verification**:
- Throughput measurement (queries/sec)
- P95/P99 latency tracking
- Memory allocation reduction
- CPU cache efficiency (if profiling available)

### Detailed Gap Mapping

#### query_api_handler.cpp (15 gaps)

| Gap | File | Line | Issue | Fix |
|-----|------|------|-------|-----|
| 1 | query_api_handler.cpp | ~45 | Missing result buffer pre-allocation | reserve(EXPECTED_ROWS) |
| 2 | query_api_handler.cpp | ~120 | Dynamic row vector growth | Pre-allocate fixed size |
| 3 | query_api_handler.cpp | ~180 | Query plan cache misses | Cache execution plans |
| 4 | query_api_handler.cpp | ~230 | Repeated column metadata allocation | Pool metadata buffers |
| 5 | query_api_handler.cpp | ~280 | Request object creation overhead | Reuse request objects |
| 6-15 | query_api_handler.cpp | ~300-500 | Connection pool state thrashing | Pre-cache + optimize lookups |

#### http2_session.cpp (9 gaps)

| Gap | File | Line | Issue | Fix |
|-----|------|------|-------|-----|
| 1 | http2_session.cpp | ~80 | Stream creation overhead | Pre-allocate stream pool |
| 2 | http2_session.cpp | ~150 | Frame buffer allocation | Reuse frame buffers |
| 3 | http2_session.cpp | ~200 | Window size recalculation | Cache window size |
| 4 | http2_session.cpp | ~250 | Flow control state updates | Batch state updates |
| 5 | http2_session.cpp | ~300 | Connection reuse inefficiency | Pool connections |
| 6-9 | http2_session.cpp | ~350-450 | Stream state management thrashing | Optimize state tracking |

#### rope_api_handler.cpp (4 gaps)

| Gap | File | Line | Issue | Fix |
|-----|------|------|-------|-----|
| 1 | rope_api_handler.cpp | ~60 | Missing output buffer allocation | Pre-allocate buffer |
| 2 | rope_api_handler.cpp | ~120 | Serialization intermediate buffers | Pool intermediate buffers |
| 3 | rope_api_handler.cpp | ~180 | Rope structure traversal overhead | Cache structural metadata |
| 4 | rope_api_handler.cpp | ~240 | Batch serialization inefficiency | Batch rope operations |

#### export_api_handler.cpp (4 gaps)

| Gap | File | Line | Issue | Fix |
|-----|------|------|-------|-----|
| 1 | export_api_handler.cpp | ~70 | Unbounded export buffer growth | Pre-allocate + stream |
| 2 | export_api_handler.cpp | ~140 | Row packing overhead | Batch row packing |
| 3 | export_api_handler.cpp | ~200 | Format-specific buffer allocation | Pool format buffers |
| 4 | export_api_handler.cpp | ~280 | Output stream inefficiency | Optimize streaming |

### Success Metrics

#### Performance Gates
- [ ] Throughput: +5-15% on high-concurrency benchmarks
- [ ] Allocation count: Reduced by 50%+ per operation
- [ ] P95/P99 latency: Improved by 10-20%
- [ ] Memory pressure: Peak memory stable or reduced

#### Quality Gates
- [ ] Zero regressions in existing tests
- [ ] Zero new compiler warnings
- [ ] All acceptance criteria met
- [ ] Code review approved
- [ ] Backward compatible verified

#### Timeline
- Start: 2026-08-18 ~08:00 UTC
- Estimated Duration: 2 days
- Expected Completion: 2026-08-20 ~12:00 UTC
- Code Review + Merge: 2026-08-20 ~18:00 UTC

---

## Phase 2 Agent Execution

### Agent Configuration

**Agent Type**: themisdb-implementer  
**Name**: server-phase-2-optimization  
**Task**: Server module optimization (34 gaps, +5-15% throughput)

**Prompt Highlights**:
- Pre-allocation strategy (constant sizes, pool patterns)
- Connection pool optimization
- HTTP/2 stream management
- Memory buffer reuse
- Performance benchmarking expectations
- Quality gates (exception safety, thread safety, backward compatibility)

### Expected Deliverables

1. **Production Code Changes** (~350 LOC)
   - query_api_handler.cpp: Pre-allocation + pool reuse
   - http2_session.cpp: Stream pool + buffer reuse
   - rope_api_handler.cpp: Serialization buffers
   - export_api_handler.cpp: Export buffer pre-allocation

2. **Test Suite** (~200 LOC)
   - Pre-allocation verification
   - Buffer pool tests
   - Concurrency stress tests
   - Performance regression tests

3. **Documentation**
   - Implementation guide (before/after patterns)
   - Performance measurement results
   - Acceptance checklist

4. **Commits** (1-2 per file or 1 comprehensive)
   - Large batches per user preference
   - Clear commit messages
   - Verified acceptance criteria in each commit

### Launch Command (for coordinator)

```
Launch themisdb-implementer with:
- Name: server-phase-2-optimization
- Description: Server module optimization (34 CRITICAL gaps, +5-15% throughput)
- Mode: sync (or background for parallel Phase 3 staging)
- Timeout: 4-6 hours (larger batch agent)
```

---

## Phase 2 Post-Completion

### Verification Steps
1. Build & compile (0 warnings)
2. Run existing test suite (100% pass)
3. Run new test suite (all deterministic)
4. Performance benchmarking (measure throughput improvement)
5. Code review (exception safety, thread safety, backward compatibility)

### Merge & Integration
- Merge to develop after all gates pass
- Update ROADMAP.md with Phase 2 completion
- Stage Wave A Exit Criteria planning

### Wave A Exit Criteria (Next: 2026-08-22)
- [ ] Voice & GPU hardening verified in production
- [ ] Server Phase 2 throughput gates met
- [ ] Chaos injection test suite for Transaction/Sharding/Replication recovery
- [ ] Performance baseline collection (p95/p99)
- [ ] Release_critical CI gating on develop

---

## Timeline Summary

```
2026-08-18
  06:45 UTC  Phase 2 Launch Plan created
  08:00 UTC  Phase 2 agent launch (server-phase-2-optimization)
  
2026-08-20
  ~12:00 UTC Phase 2 implementation complete
  ~18:00 UTC Code review + merge to develop
  
2026-08-22
  Wave A Exit Criteria planning
  Phase 3 preparation begins
  
2026-10-31
  Batch 7 critical modules ready (ethics_ai, security, audit, observability)
```

---

**Status**: 🟢 **READY TO LAUNCH**  
**Next Action**: Launch Phase 2 agent (server-phase-2-optimization)  
**Expected Delivery**: 2026-08-20 12:00 UTC  

