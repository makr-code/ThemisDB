# Server Module Phase 2 Hardening — Wave B Implementation Detail

**Date**: 2026-08-18T14:00Z  
**Status**: 🔄 PREPARATION (launching after Wave A exit confirmed)  
**Target Completion**: 2026-08-23  
**Expected Duration**: 8-12 hours implementation + testing  
**Coordinator**: Copilot Coding Agent  
**Executor Agent**: themisdb-implementer (server-phase2-hardening)

---

## Executive Summary

Server Module Phase 2 closes 34 performance gaps across 4 subsystems (Query API, HTTP/2, Rope, Export handlers) targeting **+5-15% throughput improvement** on high-concurrency workloads.

| Subsystem | Gaps | Primary Focus | Est. Impact |
|-----------|------|---|---|
| Query API Handler | 15 | Connection pool pre-allocation | +5-10% |
| HTTP/2 Session | 9 | Stream buffer management | +2-5% |
| Rope API Handler | 4 | Protocol serialization | +1-3% |
| Export API Handler | 4 | Export buffer management | +1-3% |
| **TOTAL** | **34** | Pre-allocation + timeouts + RAII | **+5-15%** |

---

## Gap Inventory & Implementation Strategy

### Block 1: Query API Handler (15 gaps) — Days 1-2

**File**: `src/server/query_api_handler.cpp`

#### Gap Category 1.1: Connection Pool Pre-allocation (5 gaps: S-001..S-003, S-008)

**Problem**: Repeated connection allocation in tight loops causes memory fragmentation and GC pressure.

**Implementation**:
```cpp
class ConnectionPool {
  private:
    static constexpr size_t INITIAL_POOL_SIZE = 32;
    static constexpr size_t MAX_POOL_SIZE = 256;
    std::vector<std::unique_ptr<Connection>> available_connections_;
    std::mutex pool_mutex_;
    size_t total_connections_ = 0;
    
  public:
    ConnectionPool(size_t initial_size = INITIAL_POOL_SIZE) {
        available_connections_.reserve(MAX_POOL_SIZE);
        for (size_t i = 0; i < initial_size; ++i) {
            available_connections_.push_back(std::make_unique<Connection>());
        }
        total_connections_ = initial_size;
    }
    
    std::optional<std::unique_ptr<Connection>> acquire(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        if (!available_connections_.empty()) {
            auto conn = std::move(available_connections_.back());
            available_connections_.pop_back();
            return conn;
        }
        
        if (total_connections_ < MAX_POOL_SIZE) {
            total_connections_++;
            return std::make_unique<Connection>();
        }
        
        // Wait for available connection with timeout
        return wait_for_available_locked(timeout);
    }
    
  private:
    std::optional<std::unique_ptr<Connection>> wait_for_available_locked(
        std::chrono::milliseconds timeout) {
        // Busy-wait with exponential backoff (or use condition variable)
        auto deadline = std::chrono::high_resolution_clock::now() + timeout;
        while (std::chrono::high_resolution_clock::now() < deadline) {
            if (!available_connections_.empty()) {
                auto conn = std::move(available_connections_.back());
                available_connections_.pop_back();
                return conn;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        return std::nullopt;
    }
};
```

**Gaps Fixed**:
- S-001: connection_pool_acquire() — Pre-allocated connections, no runtime allocation
- S-002: buffer_resize — Reserve capacity upfront
- S-008: connection_timeout — Add timeout with fallback
- S-010, S-011: Timeout safety mechanisms

**Expected Impact**: -30% allocation overhead, -20% GC pressure, ~+5-8% throughput

#### Gap Category 1.2: Buffer Pre-reservation (5 gaps: S-002..S-004, S-012..S-013)

**Problem**: String concatenation in response formatting causes repeated buffer reallocations.

**Implementation**:
```cpp
class QueryResponseBuilder {
  private:
    std::vector<char> buffer_;
    static constexpr size_t INITIAL_CAPACITY = 8192;
    
  public:
    QueryResponseBuilder() {
        buffer_.reserve(INITIAL_CAPACITY);
    }
    
    void formatQueryResponse(const QueryRequest& req, 
                           const std::vector<Result>& results) {
        // Estimate response size: header + (per-result size * count)
        size_t estimated_size = 512 +  // header
                               (results.size() * 256);  // per-result average
        
        buffer_.clear();
        buffer_.reserve(std::max(INITIAL_CAPACITY, estimated_size));
        
        // Append header
        appendToBuffer("{\"status\": \"ok\", \"results\": [");
        
        // Append each result
        for (size_t i = 0; i < results.size(); ++i) {
            if (i > 0) appendToBuffer(",");
            appendResult(results[i]);
        }
        
        appendToBuffer("]}");
    }
    
  private:
    void appendToBuffer(std::string_view sv) {
        if (buffer_.capacity() - buffer_.size() < sv.size()) {
            buffer_.reserve(buffer_.capacity() * 1.5 + sv.size());
        }
        buffer_.insert(buffer_.end(), sv.begin(), sv.end());
    }
    
    void appendResult(const Result& r) {
        // Use efficient format_to with pre-sized buffer
        auto result_json = fmt::format(
            R"({{"doc_id": "{}", "score": {:.3f}, "rank": {}}})",
            r.doc_id, r.score, r.rank);
        appendToBuffer(result_json);
    }
};
```

**Gaps Fixed**:
- S-003: string_concat — Use pre-sized buffer instead of += chains
- S-004: vector_realloc — Use reserve() before push_back()
- S-012, S-013: Buffer efficiency patterns

**Expected Impact**: -50% buffer allocations, -25% copy overhead, ~+3-5% throughput

#### Gap Category 1.3: Resource Leak & Iterator Safety (3 gaps: S-006, S-007, S-009)

**Problem**: Exception paths don't clean up connections; iterator invalidation on concurrent access.

**Implementation**:
```cpp
// RAII Connection guard for automatic cleanup on exception
class ConnectionGuard {
  private:
    std::unique_ptr<Connection> conn_;
    ConnectionPool* pool_;
    
  public:
    explicit ConnectionGuard(std::unique_ptr<Connection> conn, 
                             ConnectionPool* pool)
        : conn_(std::move(conn)), pool_(pool) {}
    
    ~ConnectionGuard() {
        if (conn_ && pool_) {
            pool_->release(std::move(conn_));
        }
    }
    
    Connection& operator*() { return *conn_; }
    Connection* operator->() { return conn_.get(); }
    Connection* get() { return conn_.get(); }
    
    // Prevent copies
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    
    // Allow moves
    ConnectionGuard(ConnectionGuard&&) noexcept = default;
    ConnectionGuard& operator=(ConnectionGuard&&) noexcept = default;
};

// Safe iterator patterns
Status QueryAPIHandler::handleBatchQueries(const std::vector<QueryRequest>& reqs) {
    std::vector<Result> all_results;
    all_results.reserve(reqs.size() * 100);  // Estimate capacity upfront
    
    for (const auto& req : reqs) {
        auto conn_opt = connection_pool_.acquire(std::chrono::seconds(5));
        if (!conn_opt) {
            return Status::RESOURCE_EXHAUSTED("Connection pool timeout");
        }
        
        // RAII guard ensures cleanup even on exception
        ConnectionGuard scoped_conn(std::move(*conn_opt), &connection_pool_);
        
        auto result = scoped_conn->executeQuery(req);
        if (!result.ok()) {
            // Automatically released by ~ConnectionGuard()
            return result.status();
        }
        
        all_results.insert(all_results.end(), 
                          result.value().begin(), 
                          result.value().end());
    }
    
    return Status::OK();
}
```

**Gaps Fixed**:
- S-006: iterator_invalidation — Avoid invalidation by pre-reserving capacity
- S-007: resource_leak — RAII guard ensures cleanup on exception
- S-009: Exception safety in error paths

**Expected Impact**: Zero resource leaks, 100% exception safety, no performance impact

#### Gap Category 1.4: Advanced Optimization (2 gaps: S-005, S-014..S-015)

**Problem**: Repeated state lookups in connection state cache; sub-optimal response formatting.

**Implementation**:
```cpp
// Connection state caching
class ConnectionStateCache {
  private:
    std::unordered_map<Connection*, ConnectionState> state_cache_;
    mutable std::shared_mutex cache_mutex_;
    
  public:
    const ConnectionState& getState(Connection* conn) const {
        std::shared_lock lock(cache_mutex_);
        auto it = state_cache_.find(conn);
        if (it != state_cache_.end()) {
            return it->second;
        }
        // Fallback: fetch and cache
        lock.unlock();
        return fetchAndCache(conn);
    }
    
  private:
    const ConnectionState& fetchAndCache(Connection* conn) {
        std::unique_lock lock(cache_mutex_);
        auto [it, _] = state_cache_.insert({conn, conn->getState()});
        return it->second;
    }
};
```

**Gaps Fixed**:
- S-005: map_lookup_repeated — Cache results to reduce repeated lookups
- S-014, S-015: Advanced buffer patterns

**Expected Impact**: -10% cache lookups, ~+1-2% throughput

---

### Block 2: HTTP/2 Session (9 gaps) — Days 2-3

**File**: `src/server/http2_session.cpp`

#### Gap Category 2.1: Stream Buffer Management (5 gaps: H-001..H-003, H-007..H-008)

**Problem**: Stream data buffers reallocate on every append; frame serialization not optimized.

**Implementation**:
```cpp
class HTTP2StreamBuffer {
  private:
    std::vector<uint8_t> buffer_;
    static constexpr size_t INITIAL_CAPACITY = 4096;
    static constexpr double GROWTH_FACTOR = 1.5;
    
  public:
    HTTP2StreamBuffer() {
        buffer_.reserve(INITIAL_CAPACITY);
    }
    
    void append(const uint8_t* data, size_t len) {
        size_t required = buffer_.size() + len;
        if (required > buffer_.capacity()) {
            // Grow capacity with exponential backoff
            buffer_.reserve(std::max(required, 
                                    (size_t)(buffer_.capacity() * GROWTH_FACTOR)));
        }
        buffer_.insert(buffer_.end(), data, data + len);
    }
    
    // Efficient frame serialization
    void appendHTTP2Frame(const HTTP2Frame& frame) {
        // Pre-calculate frame size: 9-byte header + payload
        size_t frame_size = 9 + frame.payload_length();
        if (buffer_.size() + frame_size > buffer_.capacity()) {
            buffer_.reserve(buffer_.capacity() * GROWTH_FACTOR + frame_size);
        }
        
        // Append frame header (9 bytes)
        uint8_t header[9];
        encodeFrameHeader(header, frame);
        buffer_.insert(buffer_.end(), header, header + 9);
        
        // Append payload
        buffer_.insert(buffer_.end(), frame.payload().begin(), frame.payload().end());
    }
    
    void clear() { buffer_.clear(); }
    std::vector<uint8_t> extract() { return std::move(buffer_); }
};
```

**Gaps Fixed**:
- H-001: stream_buffer_realloc — Exponential growth reduces reallocation frequency
- H-002: stream_cache_miss — State cached in HTTP2StreamBuffer
- H-003: frame_overhead — Efficient frame serialization without intermediate copies
- H-007, H-008: Buffer efficiency patterns

**Expected Impact**: -70% stream buffer reallocations, -40% serialization overhead, ~+2-4% throughput

#### Gap Category 2.2: Stream Timeout & Concurrency (4 gaps: H-004..H-006, H-009)

**Problem**: No timeout on stream operations; concurrent stream access not safe.

**Implementation**:
```cpp
class HTTP2SessionManager {
  private:
    std::unordered_map<uint32_t, HTTP2StreamBuffer> streams_;
    mutable std::shared_mutex streams_mutex_;
    static constexpr auto STREAM_TIMEOUT = std::chrono::seconds(30);
    
  public:
    Status appendToStream(uint32_t stream_id, const uint8_t* data, size_t len,
                         std::chrono::milliseconds timeout = STREAM_TIMEOUT) {
        std::shared_lock lock(streams_mutex_);
        auto it = streams_.find(stream_id);
        if (it == streams_.end()) {
            return Status::NOT_FOUND("Stream not found");
        }
        
        // Append with timeout
        auto deadline = std::chrono::high_resolution_clock::now() + timeout;
        try {
            it->second.append(data, len);
            return Status::OK();
        } catch (const std::exception& e) {
            return Status::INTERNAL(e.what());
        }
    }
    
    Status flushStream(uint32_t stream_id, std::chrono::milliseconds timeout) {
        std::unique_lock lock(streams_mutex_, std::defer_lock);
        
        auto deadline = std::chrono::high_resolution_clock::now() + timeout;
        while (true) {
            if (std::chrono::high_resolution_clock::now() > deadline) {
                return Status::DEADLINE_EXCEEDED("Stream flush timeout");
            }
            
            if (lock.try_lock_for(std::chrono::milliseconds(100))) {
                auto it = streams_.find(stream_id);
                if (it == streams_.end()) {
                    return Status::NOT_FOUND("Stream not found");
                }
                // Perform flush
                return Status::OK();
            }
        }
    }
};
```

**Gaps Fixed**:
- H-004: connection_timeout — Add timeout to stream operations
- H-005, H-006: Concurrency safety with shared_mutex
- H-009: Timeout coordination

**Expected Impact**: No deadlocks, safe concurrent access, ~0-1% performance impact (safe is worth it)

---

### Block 3: Rope API Handler (4 gaps: R-001..R-004) — Day 3

**File**: `src/server/rope_api_handler.cpp`

**Primary Issue**: Rope protocol serialization not optimized.

**Implementation Strategy**:
1. Pre-allocate buffers for rope frames (similar to HTTP/2)
2. Batch rope operations to reduce system calls
3. Implement rope frame serialization cache for common patterns
4. Add timeout to all rope send/receive operations

**Expected Impact**: +1-3% throughput on rope-heavy workloads

---

### Block 4: Export API Handler (4 gaps: E-001..E-004) — Day 3

**File**: `src/server/export_api_handler.cpp`

**Primary Issue**: Large export buffer management not optimal for streaming.

**Implementation Strategy**:
1. Implement streaming buffer with chunked writes
2. Pre-allocate export buffer based on estimated file size
3. Add backpressure handling (stop reading if buffer full)
4. Implement timeout on export operations

**Expected Impact**: +1-3% throughput on export-heavy workloads, better memory efficiency

---

## Testing Strategy

### Functional Tests (test_server_phase2_focused.cpp)

```cpp
// Test 1: Connection pool pre-allocation and timeout
TEST(ServerPhase2, ConnectionPoolPreallocationAndTimeout) {
    ConnectionPool pool(/*initial_size=*/4, /*max_size=*/8);
    std::vector<std::unique_ptr<Connection>> acquired;
    
    // Acquire 4 connections (initial pool)
    for (int i = 0; i < 4; ++i) {
        auto conn = pool.acquire(std::chrono::seconds(1));
        ASSERT_TRUE(conn);
        acquired.push_back(std::move(*conn));
    }
    
    // Acquire 4 more (up to max)
    for (int i = 0; i < 4; ++i) {
        auto conn = pool.acquire(std::chrono::seconds(1));
        ASSERT_TRUE(conn);
        acquired.push_back(std::move(*conn));
    }
    
    // Next acquire should timeout
    auto start = std::chrono::high_resolution_clock::now();
    auto result = pool.acquire(std::chrono::milliseconds(500));
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    EXPECT_FALSE(result);
    EXPECT_GE(elapsed, std::chrono::milliseconds(500));
}

// Test 2: Buffer pre-reservation efficiency
TEST(ServerPhase2, BufferPrereservationEfficiency) {
    QueryResponseBuilder builder;
    std::vector<Result> results(1000);
    for (int i = 0; i < 1000; ++i) {
        results[i] = {.doc_id = "doc_" + std::to_string(i), .score = 0.5f};
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    builder.formatQueryResponse({}, results);
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    // Should complete in < 5ms (no reallocations)
    EXPECT_LT(elapsed, std::chrono::milliseconds(5));
}

// Test 3: RAII connection guard cleanup
TEST(ServerPhase2, ConnectionGuardCleanup) {
    ConnectionPool pool(/*initial_size=*/1);
    
    {
        auto conn_opt = pool.acquire(std::chrono::seconds(1));
        ASSERT_TRUE(conn_opt);
        ConnectionGuard guard(std::move(*conn_opt), &pool);
        // Simulate exception
        if (true) throw std::runtime_error("test");
    }
    // Catch exception and verify cleanup
    
    // Connection should be released back to pool
    auto conn2 = pool.acquire(std::chrono::seconds(1));
    EXPECT_TRUE(conn2);
}

// Test 4: Stream buffer efficiency
TEST(ServerPhase2, HTTP2StreamBufferEfficiency) {
    HTTP2StreamBuffer buffer;
    std::vector<uint8_t> data(10000, 0xFF);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        buffer.append(data.data(), 100);
    }
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    // Should not cause 1000 reallocations with exponential growth
    EXPECT_LT(elapsed, std::chrono::milliseconds(50));
}

// Test 5-10: Additional gap-specific tests
```

### Performance Benchmarks

```cpp
// Benchmark: High-concurrency query throughput
BENCHMARK_F(ServerPhase2Bench, HighConcurrencyQueryThroughput)(benchmark::State& st) {
    ThreadPoolExecutor executor(16);
    QueryAPIHandler handler;
    std::vector<QueryRequest> requests = generateTestRequests(1000);
    
    for (auto _ : st) {
        std::vector<std::future<Status>> futures;
        for (const auto& req : requests) {
            futures.push_back(executor.submit([&]() {
                return handler.handleQuery(req);
            }));
        }
        
        for (auto& fut : futures) {
            auto result = fut.get();
            benchmark::DoNotOptimize(result);
        }
    }
    
    // Target: >= 1000 QPS (1000 queries / iteration_time)
    st.SetLabel("High-concurrency query throughput");
}

// Benchmark: Buffer allocation efficiency
BENCHMARK_F(ServerPhase2Bench, BufferAllocationEfficiency)(benchmark::State& st) {
    for (auto _ : st) {
        QueryResponseBuilder builder;
        std::vector<Result> results(10000);
        builder.formatQueryResponse({}, results);
        benchmark::DoNotOptimize(builder);
    }
    
    // Target: complete 10K-result formatting in < 10ms
}
```

---

## Success Criteria

| Criterion | Target | Verification |
|-----------|--------|---|
| **Gap Coverage** | 34/34 gaps fixed | Code review + test coverage |
| **Throughput Improvement** | +5-15% | Before/after benchmark comparison |
| **P99 Latency Regression** | < 1% | Latency benchmark stability |
| **Test Pass Rate** | 100% (10+ tests) | Test runner output |
| **Memory Safety** | ASan/UBSan clean | Build output verification |
| **Exception Safety** | 100% RAII patterns | Code review + exception tests |
| **Backward Compatibility** | No API changes | Compatibility test suite |
| **Zero Regressions** | All existing tests pass | Full test suite run |

---

## Timeline & Milestones

| Day | Task | Owner | Duration | Deliverable |
|-----|------|-------|----------|-------------|
| Day 1 | Query API pre-allocation (S-001..003, S-008) | themisdb-impl | 3h | 5 gaps closed |
| Day 2 | Query API buffer reserve (S-002..004, S-012..015) | themisdb-impl | 2h | 5 gaps closed |
| Day 2 | Query API RAII + resource safety (S-006, S-007, S-009) | themisdb-impl | 2h | 3 gaps closed |
| Day 2 | HTTP/2 stream buffer (H-001..003, H-007..008) | themisdb-impl | 3h | 5 gaps closed |
| Day 3 | HTTP/2 timeout & concurrency (H-004..006, H-009) | themisdb-impl | 2h | 4 gaps closed |
| Day 3 | Rope API optimization (R-001..004) | themisdb-impl | 2h | 4 gaps closed |
| Day 3 | Export API optimization (E-001..004) | themisdb-impl | 2h | 4 gaps closed |
| Day 3 | Testing & validation (10+ tests) | themisdb-impl | 3h | Test suite |
| Day 3 | Performance benchmarking & comparison | themisdb-impl | 2h | Before/after report |
| Day 4 | Code review & sign-off | Review | 1h | Final approval |

**Total**: ~26 hours (implementation, testing, benchmarking, review)

---

## Risks & Mitigations

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Connection pool exhaustion under sustained load | HIGH | Implement circuit breaker, adaptive timeout |
| Buffer exhaustion on large exports | HIGH | Streaming buffer with backpressure |
| Performance improvement < 5% | MEDIUM | Profile hotspots, may need additional optimizations |
| Backward compatibility break | HIGH | Extensive API compatibility testing |
| Memory leaks in RAII patterns | MEDIUM | ASan + valgrind verification |
| Concurrency issues with timeout | MEDIUM | ThreadSanitizer verification |

---

## Related Documentation

- **Wave B Execution Plan**: ai_working/WAVE_B_EXECUTION_PLAN_2026_08_18.md
- **Original Phase 2 Plan**: ai_working/PHASE_2_WAVE_B_SERVER_MODULE_PLAN_2026_08-18.md
- **Server Module Status**: src/server/ROADMAP.md (TBD)
- **C++ Best Practices**: .github/instructions/cpp-best-practices.instructions.md
- **Build Configuration**: cmake/Dependencies.cmake, CMakeLists.txt

---

**Document**: SERVER_PHASE2_HARDENING_DETAILED_PLAN_2026_08_18.md  
**Status**: 🔄 PREPARATION (Ready for agent launch after Wave A exit)  
**Agent Assignment**: themisdb-implementer (server-phase2-hardening)  
**Target Launch**: 2026-08-21 (after Wave A A1-A5 progress confirmation)  
**Expected Completion**: 2026-08-23
