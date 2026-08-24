# Network Batch 4.1 — Phase C Implementation Analysis

**Status:** Analysis Complete - Ready for Implementation  
**Date:** 2026-08-15  
**Agent:** themisdb-implementer  

---

## Executive Summary

Phase C requires adding timeout enforcement to 4 blocking I/O operations to prevent deadlocks. This document outlines the implementation strategy for each fix (R09-R11, R16) based on code inspection and the Phase C specification.

---

## Implementation Strategy per Fix

### R09: wire_protocol_zero_copy.cpp:112 — Timeout on I/O Write

**Location:** ZeroCopyFrameBuilder::writeTo() and related functions  
**Blocking Operations Identified:**
- `::write(fd, ...)` (line 109) - header-only write
- `::writev(fd, iov, 2)` (line 119) - scatter-gather write  
- `::WSASend()` (line 104) - Windows equivalent

**Timeout Implementation Strategy:**
1. Add optional timeout parameter to writeTo() method:
   ```cpp
   ssize_t writeTo(int fd, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const noexcept;
   ```
2. Before I/O operations, set socket to non-blocking mode
3. Use poll/select with timeout to detect when socket is ready
4. Return -1 with EAGAIN/EWOULDBLOCK on timeout
5. Log timeout occurrence at DEBUG level

**Key Considerations:**
- Socket already used for I/O, so non-blocking mode change is safe
- Use `fcntl(fd, F_SETFL, O_NONBLOCK)` on Unix; `ioctlsocket()` on Windows
- Restore blocking mode after operation or use RAII guard
- Handle EINTR and retry logic

**Acceptance Criteria:**
- ✅ Timeout parameter added
- ✅ Non-blocking I/O with timeout enforcement  
- ✅ Error returned cleanly on timeout
- ✅ No resource leaks

---

### R10: service_mesh.cpp:243 — Timeout on Accept Connection

**Location:** ServiceMeshIntegration::acceptLoop() or ::start()  
**Blocking Operations Identified:**
- `acceptor_->accept(socket, ec)` (line 211) - accept connection in loop

**Timeout Implementation Strategy:**
1. Add timeout to accept loop using deadline-based approach:
   ```cpp
   // In acceptLoop():
   std::optional<std::chrono::steady_clock::time_point> deadline;
   if (accept_timeout) {
       deadline = std::chrono::steady_clock::now() + *accept_timeout;
   }
   ```
2. Check deadline before each accept:
   ```cpp
   if (deadline && std::chrono::steady_clock::now() > *deadline) {
       // Timeout occurred
       THEMIS_WARN("Accept loop timeout");
       break;
   }
   ```
3. Use Boost.Asio's async timeout or set socket timeout:
   ```cpp
   acceptor_->set_option(net::ip::tcp::no_delay(false));
   // Or implement with std::condition_variable::wait_for()
   ```

**Key Considerations:**
- Service mesh probe is low-throughput (health checks only)
- Timeout should be conservative (e.g., 30 seconds)
- Don't spam logs - only log timeouts at WARN level
- Ensure thread-safe shutdown

**Acceptance Criteria:**
- ✅ Accept operation has timeout enforcement
- ✅ Deadline-based checks in loop
- ✅ Graceful timeout handling
- ✅ No deadlocks in accept loop

---

### R11: wire_protocol_zero_copy.cpp:160 — Timeout on Sendfile Write

**Location:** ZeroCopyFrameBuilder::writeToWithSendfile()  
**Blocking Operations Identified:**
- `::sendfile(socket_fd, payload_fd, &off, remaining)` (line 162) - zero-copy file transfer
- `::write(socket_fd, tmp.data(), ...)` (line 174) - fallback write

**Timeout Implementation Strategy:**
1. Add timeout parameter to writeToWithSendfile():
   ```cpp
   ssize_t writeToWithSendfile(int socket_fd, int payload_fd, off_t payload_offset,
                               size_t sendfile_threshold,
                               std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const noexcept;
   ```
2. Track elapsed time in sendfile loop:
   ```cpp
   auto start = std::chrono::steady_clock::now();
   while (remaining > 0) {
       if (std::chrono::steady_clock::now() - start > timeout) {
           THEMIS_WARN("Sendfile timeout after {} ms", timeout.count());
           return sf_written > 0 ? hdr_written + sf_written : -1;
       }
       // ... sendfile() call ...
   }
   ```
3. Implement loop timeout with deadline checks
4. Return partial write count if timeout occurs

**Key Considerations:**
- Sendfile is zero-copy, so partial writes are acceptable
- Timeout prevents indefinite blocking on large files
- Coordinate timeout value with R09 for consistency
- Log timeouts at DEBUG level (frequent operation)

**Acceptance Criteria:**
- ✅ Timeout parameter added and enforced
- ✅ Loop exit on timeout with partial write
- ✅ No buffer corruption on timeout
- ✅ Timeout value matches R09

---

### R16: wire_protocol_performance.cpp:232 — Timeout on Pool Acquire (Perf-Critical)

**Location:** PayloadBufferPool::acquire()  
**Blocking Operations Identified:**
- `pool_mutex_.lock()` (implicit in lock_guard) - can block if lock is held
- Buffer allocation - can block on memory exhaustion (rare)

**Timeout Implementation Strategy:**
1. Add optional timeout to acquire():
   ```cpp
   Handle acquire(std::optional<std::chrono::microseconds> timeout = std::nullopt);
   ```
2. Use try_lock_for() with timeout:
   ```cpp
   {
       std::unique_lock<std::mutex> lock(pool_mutex_);
       if (timeout) {
           // Use high-precision timeout for performance monitoring
           if (!lock.try_lock_for(*timeout)) {
               miss_count_.fetch_add(1, std::memory_order_relaxed);
               // Return error or create buffer without lock
               return Handle(std::make_unique<Buffer>(), this);
           }
       }
       // ... pool access ...
   }
   ```
3. Implement lock-free fast path for no contention case
4. Log timeout at DEBUG level only (not per-operation)

**Key Considerations:**
- **CRITICAL:** Must maintain <1% performance overhead
- Use try_lock_for() which is optimized by STL
- Timeout should be very short (microseconds, e.g., 10µs)
- Fallback to heap allocation on timeout is acceptable
- No spinning or busy-waiting

**Acceptance Criteria:**
- ✅ Timeout parameter added (optional)
- ✅ Try-lock with timeout used
- ✅ **Performance impact < 1%** (critical gate)
- ✅ Fallback to heap allocation on contention
- ✅ No deadlocks

---

## Implementation Order

1. **R09 & R11** (wire_protocol_zero_copy.cpp) - Implement together for consistency
2. **R10** (service_mesh.cpp) - Independent, simpler change
3. **R16** (wire_protocol_performance.cpp) - Performance-critical, needs careful validation

---

## Common Patterns to Implement

### Timeout Pattern 1: Non-blocking I/O with Poll
```cpp
// Set non-blocking
fcntl(fd, F_SETFL, O_NONBLOCK);

// Use poll() with timeout
struct pollfd pfd = {fd, POLLIN|POLLOUT, 0};
int poll_result = poll(&pfd, 1, timeout_ms);
if (poll_result == 0) return ETIMEDOUT;
if (poll_result < 0) return errno;

// Do I/O operation
ssize_t n = write(fd, ...);
```

### Timeout Pattern 2: Deadline-based Loop
```cpp
auto deadline = std::chrono::steady_clock::now() + timeout;
while (condition) {
    if (std::chrono::steady_clock::now() > deadline) {
        THEMIS_WARN("Operation timeout");
        return TimeoutError;
    }
    // ... operation ...
}
```

### Timeout Pattern 3: Try-lock with Timeout
```cpp
std::unique_lock<std::mutex> lock(mutex_);
if (!lock.try_lock_for(timeout)) {
    return ETIMEDOUT;
}
// ... critical section ...
```

---

## Testing Strategy

### Unit Tests
- Create test fixtures that simulate slow I/O
- Use network simulation tools (if available)
- Verify timeouts trigger within ±100ms

### ThreadSanitizer Validation
```bash
cmake --preset develop-tsan
cmake --build build-tsan --target network
ctest --preset develop-tsan -k network
```

### Performance Validation (R16 Critical)
```bash
# Baseline before changes
./benchmarks/network/perf_test --mode baseline

# After changes
./benchmarks/network/perf_test --mode with-timeouts

# Compare: expect < 1% regression
```

---

## Risks and Mitigation

| Risk | Mitigation |
|------|-----------|
| Timeout value too short → false positives | Use conservative defaults (5-30 sec), make configurable |
| Timeout value too long → not effective | Validate with slow network tests |
| Performance regression (R16) | Use try_lock_for(), profile before commit |
| Deadlock during timeout handling | Test with ThreadSanitizer, no nested locks |
| Resource leaks on timeout | Use RAII guards, test with ASan |

---

## Files to Modify

1. `src/network/wire_protocol_zero_copy.cpp`
   - Add timeout to writeTo() (R09)
   - Add timeout to writeToWithSendfile() (R11)

2. `src/network/service_mesh.cpp`
   - Add timeout to acceptLoop() or start() (R10)

3. `src/network/wire_protocol_performance.cpp`
   - Add timeout parameter to acquire() (R16)

4. Potentially: `include/network/wire_protocol_zero_copy.h`
   - Update method signatures

5. Potentially: `include/network/wire_protocol_performance.h`
   - Update method signatures

---

## Next Steps

1. Implement R09 & R11 in wire_protocol_zero_copy.cpp
2. Implement R10 in service_mesh.cpp
3. Implement R16 in wire_protocol_performance.cpp
4. Run compilation and tests
5. Validate with ThreadSanitizer
6. Measure performance impact (R16)
7. Create Phase C completion report
8. Commit all changes

---

*Generated by themisdb-implementer agent | 2026-08-15*
