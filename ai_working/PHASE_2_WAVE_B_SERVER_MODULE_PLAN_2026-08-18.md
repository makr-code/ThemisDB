# Phase 2: Wave B Stabilization - Server Module Hardening

**Date**: 2026-08-18 T07:48Z  
**Status**: 🔄 PREPARATION (Launching after Wave A completes)  
**Target Completion**: 2026-08-20  
**Expected Duration**: 2-3 days

---

## Objective

Complete Server module Phase 2 hardening by addressing 34 remaining gaps across connection pool, buffer management, and API efficiency. Target: +5-15% throughput on high-concurrency paths.

---

## Module Status Dashboard

| Module | Phase | Gaps Remaining | Focus Areas | Status |
|--------|-------|----------|----------|--------|
| Search | Complete | - | Documentation review | ✅ DONE |
| Sharding | Complete | - | Phase C gate unlock | ✅ DONE |
| Replication | Complete | - | Deployment readiness | ✅ DONE |
| **Server** | Phase 2 | 34 | Connection pool, buffers | 🔄 LAUNCHING |

---

## Server Phase 2: Gap Breakdown & Implementation Plan

### 1. Query API Handler (15 gaps)
**File**: src/server/query_api_handler.cpp  
**Primary Issue**: Copy overhead in connection pool management

#### Gap Details
| Gap ID | Type | Severity | Location | Fix Pattern |
|--------|------|----------|----------|-------------|
| S-001 | copy_overhead | HIGH | connection_pool_acquire() | Pre-allocate connections, avoid repeated allocations |
| S-002 | buffer_resize | HIGH | response_buffer | Reserve buffer capacity upfront |
| S-003 | string_concat | MEDIUM | query_response_format() | Use std::ostringstream or pre-sized buffer |
| S-004 | vector_realloc | MEDIUM | batch_query_results | Use reserve() before push_back() |
| S-005 | map_lookup_repeated | MEDIUM | connection_state_cache | Cache lookup results |
| S-006 | iterator_invalidation | HIGH | result_iteration | Safe iterator patterns |
| S-007 | resource_leak | HIGH | error_paths | RAII cleanup on exceptions |
| S-008 | connection_timeout | HIGH | pool_wait() | Add timeout with fallback |
| S-009-S-015 | Various | MEDIUM | Buffer management | Batch improvements |

#### Implementation Pattern

```cpp
// BEFORE: Inefficient pool management
Status QueryAPIHandler::handleQuery(const QueryRequest& req) {
    auto conn = connection_pool_.acquire();  // Repeated allocation
    if (!conn) return Status::RESOURCE_EXHAUSTED;
    
    std::string response;
    for (const auto& result : results) {
        response += formatResult(result);  // String reallocation each iteration
    }
    return sendResponse(conn, response);
}

// AFTER: Optimized with pre-allocation
Status QueryAPIHandler::handleQuery(const QueryRequest& req) {
    // Get pre-allocated connection from pool
    auto conn = connection_pool_.acquire(std::chrono::seconds(5));
    if (!conn) return Status::RESOURCE_EXHAUSTED;
    
    // Reserve buffer capacity upfront (estimate + buffer)
    response_buffer_.reserve(estimated_response_size * 1.2);
    response_buffer_.clear();
    
    // Build response efficiently
    for (const auto& result : results) {
        formatResult(result, response_buffer_);  // Append to pre-sized buffer
    }
    
    // Use RAII for automatic cleanup
    auto scoped_conn = std::make_unique<ConnectionGuard>(std::move(conn));
    return sendResponse(*scoped_conn, response_buffer_);
}
```

#### Pre-allocation Strategy
```cpp
class ConnectionPool {
  private:
    static constexpr size_t INITIAL_POOL_SIZE = 32;
    static constexpr size_t MAX_POOL_SIZE = 256;
    std::vector<Connection> available_connections_;
    
  public:
    ConnectionPool(size_t initial_size = INITIAL_POOL_SIZE) {
        available_connections_.reserve(MAX_POOL_SIZE);
        for (size_t i = 0; i < initial_size; ++i) {
            available_connections_.emplace_back();
        }
    }
    
    std::optional<Connection> acquire(std::chrono::milliseconds timeout) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        if (available_connections_.empty()) {
            // Check if we can create new connection
            if (total_connections_ < MAX_POOL_SIZE) {
                total_connections_++;
                return Connection::create();
            }
            
            // Wait for available connection with timeout
            return wait_for_available(timeout);
        }
        
        auto conn = std::move(available_connections_.back());
        available_connections_.pop_back();
        return conn;
    }
};
```

---

### 2. HTTP/2 Session (9 gaps)
**File**: src/server/http2_session.cpp  
**Primary Issue**: Stream buffer management inefficiency

#### Gap Details
| Gap ID | Type | Severity | Location | Fix Pattern |
|--------|------|----------|----------|-------------|
| H-001 | stream_buffer_realloc | HIGH | stream_data_append() | Reserve buffer capacity |
| H-002 | stream_cache_miss | MEDIUM | stream_lookup() | Cache stream state |
| H-003 | frame_overhead | HIGH | frame_serialize() | Optimize serialization |
| H-004 | connection_timeout | HIGH | stream_wait() | Add timeout |
| H-005-H-009 | Various | MEDIUM | Buffer efficiency | Batch optimizations |

#### Implementation Pattern
```cpp
// RAII stream buffer management
class HTTP2StreamBuffer {
  private:
    std::vector<uint8_t> buffer_;
    static constexpr size_t INITIAL_CAPACITY = 4096;
    
  public:
    HTTP2StreamBuffer() {
        buffer_.reserve(INITIAL_CAPACITY);
    }
    
    void append(const uint8_t* data, size_t len) {
        // Efficient append with pre-allocated capacity
        if (buffer_.size() + len > buffer_.capacity()) {
            // Grow capacity by 50% to amortize reallocations
            buffer_.reserve(buffer_.capacity() * 1.5 + len);
        }
        buffer_.insert(buffer_.end(), data, data + len);
    }
    
    void clear() { buffer_.clear(); }
};
```

---

### 3. Rope API Handler (4 gaps)
**File**: src/server/rope_api_handler.cpp  
**Primary Issue**: Rope protocol serialization optimization

#### Gaps: R-001 through R-004
- Rope frame serialization overhead
- Compression efficiency
- Buffer management

---

### 4. Export API Handler (4 gaps)
**File**: src/server/export_api_handler.cpp  
**Primary Issue**: Large export buffer management

#### Gaps: E-001 through E-004
- Export file buffering
- Memory efficiency on large exports
- Streaming optimization

---

## Implementation Strategy

### Phase 2 Execution Plan

**Step 1: Pre-allocation & Connection Pool (Days 1-2)**
- Implement connection pool pre-allocation
- Add buffer.reserve() calls throughout
- Measure performance before/after

**Step 2: Timeout Safety (Day 2)**
- Add timeout to all pool operations
- Add fallback handling
- Verify no deadlocks

**Step 3: RAII Cleanup (Day 2-3)**
- Wrap connections in ConnectionGuard RAII
- Verify exception safety
- Test error paths

**Step 4: Performance Validation (Day 3)**
- Benchmark pre-optimization vs post-optimization
- Target: +5-15% throughput improvement
- Verify no regressions in latency

### Agent Dispatch (After Wave A)

**Agent 4: server-phase2-hardening**
- **Type**: themisdb-implementer
- **Files**: query_api_handler, http2_session, rope_api_handler, export_api_handler
- **Expected Deliverables**:
  - 4 commits (pre-allocation, timeouts, RAII, tests)
  - tests/test_server_phase2_focused.cpp (10+ tests)
  - Performance benchmark showing +5-15% throughput
  - Zero compilation errors/warnings

---

## Testing Strategy

### Performance Benchmarks

```cpp
// Benchmark: Connection pool efficiency
TEST(ServerPhase2Bench, ConnectionPoolPreallocationThroughput) {
    std::vector<Query> queries = generate_test_queries(1000);
    
    auto start = high_resolution_clock::now();
    for (const auto& query : queries) {
        auto result = query_handler_.handleQuery(query);
        ASSERT_OK(result);
    }
    auto elapsed = high_resolution_clock::now() - start;
    
    // Target: Handle 1000 queries in <5 seconds (200 QPS)
    ASSERT_LT(elapsed, std::chrono::seconds(5));
}

// Benchmark: Buffer efficiency
TEST(ServerPhase2Bench, BufferReserveEfficiency) {
    std::vector<Response> responses;
    
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        // With pre-reserved capacity, should be fast
        response_buffer_.clear();
        response_buffer_.reserve(estimated_size);
        formatLargeResponse(response_buffer_);
        responses.push_back(std::string(response_buffer_.begin(), response_buffer_.end()));
    }
    auto elapsed = high_resolution_clock::now() - start;
    
    // Target: Format 10000 large responses in <2 seconds
    ASSERT_LT(elapsed, std::chrono::seconds(2));
}
```

### Functional Tests
```cpp
// Test: Connection pool timeout
TEST(ServerPhase2, ConnectionPoolTimeout) {
    pool_config_.max_wait_ms = 100;
    
    // Exhaust pool
    std::vector<auto> connections;
    for (int i = 0; i < POOL_SIZE; ++i) {
        connections.push_back(pool_.acquire(std::chrono::milliseconds(1)));
    }
    
    // Next acquire should timeout
    auto start = high_resolution_clock::now();
    auto result = pool_.acquire(std::chrono::milliseconds(100));
    auto elapsed = high_resolution_clock::now() - start;
    
    EXPECT_FALSE(result);
    EXPECT_GT(elapsed, std::chrono::milliseconds(80));
    EXPECT_LT(elapsed, std::chrono::milliseconds(150));
}

// Test: Exception safety with RAII
TEST(ServerPhase2, ConnectionGuardCleanup) {
    size_t initial_available = pool_.available_count();
    
    {
        auto conn = pool_.acquire();
        ASSERT_TRUE(conn);
        // Simulate exception inside RAII scope
        throw std::runtime_error("test");
    }
    
    // Should be cleaned up automatically
    EXPECT_EQ(pool_.available_count(), initial_available);
}
```

---

## Success Criteria

| Criterion | Target | Method |
|-----------|--------|--------|
| All 34 gaps fixed | 100% | Code review + testing |
| Throughput improvement | +5-15% | Benchmark comparison |
| Latency regression | <1% | Latency benchmark |
| Test coverage | 10+ tests | Focused test suite |
| Exception safety | 100% RAII | Code review + ASan |
| Backward compatibility | Full | API compatibility test |
| Zero regressions | All existing tests pass | Full test suite run |

---

## Timeline

| Date | Task | Status |
|------|------|--------|
| 2026-08-18 | Wave A (Batch A-6,8,9) complete | 🔄 In Progress |
| 2026-08-19 | Server Phase 2 agent launch | ⏳ Queued |
| 2026-08-20 | Server Phase 2 complete | ⏳ Expected |
| 2026-08-21+ | Wave A Exit Criteria verification | ⏳ Queued |

---

## Key Risks & Mitigations

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Pool exhaustion under load | MEDIUM | Timeout with fallback, circuit breaker |
| Buffer exhaustion | HIGH | Pre-allocation + overflow detection |
| Backward compatibility break | HIGH | Extensive API testing |
| Performance regression | MEDIUM | Benchmark comparison before/after |
| Memory leaks on errors | MEDIUM | RAII + ASan verification |

---

**Document**: PHASE_2_WAVE_B_SERVER_MODULE_PLAN_2026-08-18.md  
**Generated**: 2026-08-18 T07:48Z  
**Coordinator**: Copilot Main Agent  
**Next Review**: After Wave A completion (2026-08-18 T09:30Z)
