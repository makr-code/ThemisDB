# BATCH 4.1 PHASE F — INVESTIGATION REPORT
## Connection Leak Fixes (R17-R19)

**Status:** INVESTIGATION REQUIRED  
**Date:** 2026-08-15 18:15 UTC  
**Agent:** Copilot SWE  

---

## Summary

Phase F implementation is ready to proceed, but **clarification needed** on R17-R18-R19 line number references. The specified lines (288-290) in raft_load_balancer.cpp do not contain obvious connection leak code, suggesting either:
1. Line numbers are placeholders/approximate references
2. Leak fixes are for NEW code paths that need defensive patterns
3. Actual leak locations differ from spec locations

---

## Analysis

### R17-R19: Specified Locations

**File:** `src/network/raft_load_balancer.cpp`  
**Lines:** 288, 289, 290  

**Current Code (Lines 280-300):**
```cpp
280. void RaftLoadBalancer::setHealthCheckFn(std::function<bool(const Backend &)> fn) {
281.     std::lock_guard<std::mutex> lk(shutdown_mutex_);
282.     health_check_fn_ = std::move(fn);
283. }
284. 
285. // =============================================================================
286. // Internal – routing helpers  (all called with backends_mutex_ held)
287. // =============================================================================
288. 
289. RaftLoadBalancer::Backend *RaftLoadBalancer::findBackend(const std::string &address) {
290.     for (const auto &b : backends_) {
291.         if (b->address == address)
292.             return b.get();
293.     }
294.     return nullptr;
295. }
296. 
297. std::vector<RaftLoadBalancer::Backend *> RaftLoadBalancer::healthyBackends() const {
298.     std::vector<Backend *> result;
299.     // Prefer local datacenter if configured
300.     if (config_.prefer_local_datacenter && !config_.datacenter.empty()) {
```

**Analysis:**
- Line 288: Empty line (comment section divider)
- Line 289: Method declaration `findBackend()`
- Line 290: Loop start in `findBackend()`
- These lines contain NO database connection operations
- No obvious resource leak patterns present

---

### Current Connection-Related Code

**File Inspection Results:**

1. **Connection Tracking (Lines 225-238):**
   ```cpp
   void RaftLoadBalancer::onConnectionOpened(const std::string &address) {
       std::lock_guard<std::mutex> lk(backends_mutex_);
       auto *b = findBackend(address);
       if (b)
           b->active_connections.fetch_add(1, std::memory_order_relaxed);
   }

   void RaftLoadBalancer::onConnectionClosed(const std::string &address) {
       std::lock_guard<std::mutex> lk(backends_mutex_);
       auto *b = findBackend(address);
       if (b && b->active_connections.load(std::memory_order_relaxed) > 0) {
           b->active_connections.fetch_sub(1, std::memory_order_relaxed);
       }
   }
   ```
   - These are METRICS ONLY (counting active connections)
   - Do not actually open/close sockets or database connections
   - No leak pattern here

2. **Health Check Method (Lines 409-450):**
   ```cpp
   bool RaftLoadBalancer::defaultHealthCheck(const Backend & /*backend*/) {
       // In a real implementation this would open a TCP connection to
       // backend.address and send a ThemisDB ping frame.  For unit-test and
       // embedded environments we return true (always healthy).
       return true;
   }

   void RaftLoadBalancer::runHealthChecks() {
       std::function<bool(const Backend &)> check_fn;
       {
           std::lock_guard<std::mutex> lk(shutdown_mutex_);
           check_fn = health_check_fn_ ? health_check_fn_ : &RaftLoadBalancer::defaultHealthCheck;
       }
       // ... health check iteration ...
   }
   ```
   - Health check is STUBBED ("In a real implementation...")
   - NO actual connection operations yet
   - Could be a future leak source IF implemented

3. **Backend Management (Lines 130-160):**
   ```cpp
   void RaftLoadBalancer::addBackend(const std::string &address, double weight, const std::string &datacenter) {
       std::lock_guard<std::mutex> lk(backends_mutex_);
       auto backend = std::make_unique<Backend>();
       backend->address = address;
       // ...
   }

   void RaftLoadBalancer::removeBackend(const std::string &address) {
       std::lock_guard<std::mutex> lk(backends_mutex_);
       backends_.erase(
           std::remove_if(backends_.begin(), backends_.end(),
                          [&address](const std::unique_ptr<Backend> &b) { return b->address == address; }),
           backends_.end());
   }
   ```
   - Uses std::unique_ptr (RAII-safe)
   - Backend cleanup is properly handled by unique_ptr dtor
   - No leak risk here

---

## File Checklist Status

| Check | Result | Notes |
|-------|--------|-------|
| Database connection APIs present? | NO | File is about Raft load balancing, not database connections |
| Socket open/close calls? | NO | File is about load balancer strategy, not socket ops |
| Resource pool management? | PARTIAL | Tracks active_connections counter, not actual resource pools |
| RAII wrappers present? | YES | std::unique_ptr for backends |
| onConnectionOpened/Closed methods? | YES | But these are metrics-only callbacks |
| Potential leak patterns? | NO | No obvious resource leaks found |

---

## Possible Interpretations

### Interpretation 1: Line Numbers are Approximate
The spec lines 288-290 might point to the **vicinity** where leak safeguards should be added:
- Add defensive checks in `findBackend()` to ensure connection cleanup on error
- Add RAII guards in `runHealthChecks()` if health checks actually open connections
- Add guards in backend removal to ensure active connections are cleaned up

### Interpretation 2: R17-R19 Point to Methods, Not Exact Lines
Lines might be meant to indicate:
- R17: `onConnectionOpened()` / `onConnectionClosed()` pair (lines 225-238)
- R18: `findBackend()` method (line 289)
- R19: `runHealthChecks()` method (line 416)

### Interpretation 3: Leak Fixes are for FUTURE Health Check Implementation
When actual database connections are added (via health_check_fn_), ensure:
- Health check connections are closed even on error (RAII guard)
- Pool exhaustion checks in place
- Timeout prevents connection hang

### Interpretation 4: Incorrect Phase F Specification
The leak fixes might belong to a different file or module:
- `wire_protocol_zero_copy.cpp` (has socket operations, currently appears safe)
- `service_mesh.cpp` (has connection management)
- `connection_compression.cpp` (handles connections)

---

## Recommendations

### Option A: Proceed with Defensive Guards
Add RAII guards to ensure safe resource cleanup in:
1. Health check method (defensively guard any future connection code)
2. Backend selection methods (ensure error paths clean up connections)
3. Connection tracking methods (ensure metrics stay consistent)

**Effort:** ~30 minutes (defensive programming)  
**Risk:** Low (adds safeguards without changing existing behavior)  
**Benefit:** Prevents future leaks when real connection code is added

### Option B: Request Clarification
Ask user to confirm:
1. Are R17-R19 locations correct, or should they be updated?
2. Are these fixes for FUTURE code or existing code?
3. Should we focus on specific connection leak patterns?

**Effort:** 5 minutes (ask question)  
**Risk:** Low (clarification ensures correct implementation)  
**Benefit:** Guarantees we fix the right leaks

### Option C: Search for Connection Leaks Across Module
Scan all network files (*.cpp) for patterns:
- Resource acquisition without release
- Error paths that don't clean up
- Missing RAII wrappers

**Effort:** ~45 minutes (full module scan)  
**Risk:** Medium (might find unrelated issues)  
**Benefit:** Comprehensive leak detection

---

## Code Patterns Ready to Implement

### RAII Connection Guard Pattern
```cpp
// If we need to add guards in health check:
class ConnectionGuard {
public:
    explicit ConnectionGuard(DatabasePool& pool) 
        : conn_(pool.acquire()) {}
    
    ~ConnectionGuard() noexcept {
        if (conn_) conn_->close();
    }
    
    DatabaseConnection* get() { return conn_; }
    
private:
    DatabaseConnection* conn_;
    // Disallow copy
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
};
```

### Error Path Guard Pattern
```cpp
// In backend selection methods:
auto *backend = findBackend(address);
if (!backend) {
    // Connection leak guard: ensure cleanup on error
    // (placeholder for actual cleanup when connections exist)
    return {};
}
```

### Health Check with Connection Safety
```cpp
void RaftLoadBalancer::runHealthChecks() {
    // ... existing setup code ...
    
    for (Backend *b : backends_snapshot) {
        // When real connection code is added:
        // ConnectionGuard guard(db_pool);
        // auto conn = guard.get();
        // const bool ok = checkBackendHealth(b, conn);
        
        const bool ok = check_fn(*b);
        
        // Cleanup happens automatically via ConnectionGuard dtor
        // Even if exception thrown or early return
    }
}
```

---

## Current Implementation Status

**All 5 previous phases COMPLETE:**
- ✅ Phase A: Brace formatting (R01-R05)
- ✅ Phase B: Missing destructors (R06-R08)
- ✅ Phase C: Timeout enforcement (R09-R11, R16)
- ✅ Phase D: Memcpy bounds validation (R12-R13)
- ✅ Phase E: Smart pointer & exception safety (R14-R15)

**Phase F: READY FOR IMPLEMENTATION**
- Current status: Code inspection complete
- Line number clarification: PENDING
- Implementation approach: READY (defensive guards pattern)
- Risk level: LOW (safeguards, not breaking changes)

---

## Next Steps

### Immediate (Agent Decision):
1. **Use Option A** (Recommended): Add defensive RAII guards to raft_load_balancer.cpp methods that deal with connections, even if current code is only stubbed.
   - Implements safeguards for future health check implementation
   - Prevents leaks in production when real connection code is added
   - ~30 minutes work

2. **Use Option B**: Request user clarification on exact R17-R19 locations before proceeding.
   - Ensures we fix the exact leaks user intended
   - Prevents rework if locations change
   - ~5 minutes delay

3. **Use Option C**: Comprehensive leak scan across module.
   - Catches any connection leaks in other files
   - More thorough than targeted fixes
   - ~45 minutes, may find other issues

### Recommended: Use Option A + Be Ready for Clarification
- Proceed with implementing defensive connection guards in raft_load_balancer.cpp
- Document exact changes in completion report
- If user provides clarification, can pivot or add additional fixes
- Maintains timeline while ensuring safety

---

## Files to Modify (if Option A)

1. **include/network/raft_load_balancer.h**
   - Add ConnectionGuard helper class if needed
   - Document connection lifecycle expectations

2. **src/network/raft_load_balancer.cpp**
   - Add guards to `runHealthChecks()` (line 416)
   - Add guards to `findBackend()` error paths (line 289)
   - Add guards to `onConnectionOpened/Closed` pair
   - Add cleanup in `removeBackend()` (line 148)

---

**Report Status:** READY FOR DECISION  
**Recommended Action:** Proceed with Option A (add defensive guards) while being ready to implement Option B if user provides clarification.

