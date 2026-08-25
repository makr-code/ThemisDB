# Wave A Batch A-8: Voice & GPU Module Hardening Guide

**Status**: Planning document for Week 3 implementation  
**Target**: Aug 28-31, 2026  
**Estimated Effort**: 22 hours (10 hrs Voice, 12 hrs GPU)

---

## VOICE MODULE BATCH A-8 IMPLEMENTATION

### Current Status
- **Completion**: 45%
- **Blocking Issues**: 13 CRITICAL (missing liveness, anti-spoof, audit)
- **Production Readiness**: ❌ NOT READY (security features incomplete)

### IMPLEMENTATION TASKS (Priority Order)

#### 1. LIVENESS DETECTION ENGINE (NEW - 0% → 100%)
**File**: `src/voice/voice_liveness_detector.cpp` + `include/voice/voice_liveness_detector.h`  
**Priority**: CRITICAL (security gate)

**Requirements**:
- Challenge-response: Server sends random challenge, client echoes it
- Anti-playback: Verify challenge contains timestamp (freshness)
- Anti-replay: Track used challenges, reject duplicates
- Timeout: 5 second challenge window (reject stale responses)

**Implementation**:
```cpp
class VoiceLivenessDetector {
public:
    struct Challenge {
        uint64_t id;               // Unique challenge ID
        std::string text;          // Random phrase (e.g., "echo seven thousand two hundred")
        int64_t issued_at_ms;      // Timestamp
        bool verified = false;
    };
    
    Result<Challenge> issueChallenge(const std::string& user_id);
    Result<> verifyResponse(const std::string& user_id, const Challenge& challenge, 
                            const std::string& audio_response);
    
private:
    std::map<uint64_t, Challenge> active_challenges_;
    std::set<uint64_t> verified_challenges_;  // Recently verified
    
    Result<std::string> speechToTextChallenge(const std::string& audio);
};
```

**Tests**:
- Test: Valid challenge-response passes
- Test: Stale response (>5s old) rejected
- Test: Replay attack (same response twice) rejected
- Test: Invalid response text rejected

---

#### 2. ANTI-SPOOF VERIFICATION ENGINE (NEW - 0% → 100%)
**File**: `src/voice/voice_anti_spoof_engine.cpp` + `include/voice/voice_anti_spoof_engine.h`  
**Priority**: CRITICAL (security gate)

**Requirements**:
- Audio Freshness: Detect synthetic/recorded audio vs. live stream
- Speaker Verification: Verify speaker voice matches known baseline
- Noise Analysis: Detect environmental consistency (background noise pattern)

**Implementation**:
```cpp
class VoiceAntiSpoofEngine {
public:
    struct SpoofAnalysis {
        double audio_freshness_score;  // 0.0-1.0 (1.0 = live)
        double speaker_match_score;    // 0.0-1.0 (1.0 = exact match)
        double noise_consistency_score; // 0.0-1.0 (1.0 = consistent)
        bool is_likely_spoofed;        // Composite verdict
        std::string reason;            // Why verdict reached
    };
    
    Result<SpoofAnalysis> analyzeSpoofRisk(
        const std::string& audio_data,
        const std::string& speaker_baseline);
    
private:
    Result<> analyzeAudioFreshness(const std::string& audio, double& score);
    Result<> analyzeSpeakerMatch(const std::string& audio, 
                                  const std::string& baseline, double& score);
    Result<> analyzeNoisePattern(const std::string& audio, double& score);
};
```

**Detection Methods**:
- Spectral Analysis: Check for digital artifacts (synthesis)
- Noise Characteristics: Real audio has environmental noise; synthetic is too clean
- Speaker Embedding: Use speaker recognition model to verify identity

**Tests**:
- Test: Live audio passes (high freshness score)
- Test: Recorded audio detected as spoofed
- Test: AI-synthesized audio detected as spoofed
- Test: Speaker voice mismatch detected

---

#### 3. FAIL-CLOSED SESSION LIFECYCLE (UPDATE - 50% → 100%)
**File**: `src/voice/voice_session_manager.cpp`  
**Priority**: HIGH (operational safety)

**Requirements**:
- Fail-Closed Default: Deny access unless explicitly authorized
- Timeout: 5 min idle timeout, 30 min absolute session timeout
- Clean Teardown: Close all resources on session end (no leaks)

**Changes**:
```cpp
// Before: Permissive default (WRONG)
if (!isAuthenticated()) {
    // Maybe allow based on other signals
}

// After: Fail-closed (CORRECT)
if (!isAuthenticated()) {
    return error("Authorization required");  // DENY by default
}

// Timeout tracking
auto now = getTimeNowMs();
if (now - session_.last_activity_ms > 5 * 60_000) {
    closeSession("Idle timeout");
}
if (now - session_.created_at_ms > 30 * 60_000) {
    closeSession("Session max age exceeded");
}
```

**RAII Pattern for cleanup**:
```cpp
class SessionLifetimeGuard {
    VoiceSession* session_;
public:
    SessionLifetimeGuard(VoiceSession& s) : session_(&s) {}
    ~SessionLifetimeGuard() {
        if (session_) {
            session_->cleanup();  // RAII ensures cleanup even on exception
        }
    }
};
```

**Audit Logging**:
- Log: Authentication attempt (user_id, timestamp, result)
- Log: Session created/closed (duration, bytes transferred)
- Log: Liveness challenge issued/verified
- Log: Spoof detection triggered

---

#### 4. COMPREHENSIVE AUDIT LOGGING (AUDIT - 0% → 100%)
**File**: `src/voice/voice_audit_logger.cpp` + `include/voice/voice_audit_logger.h`  
**Priority**: CRITICAL (compliance)

**Critical Gaps** (3 instances in voice_assistant.cpp):
- Line 144: Authentication attempt not logged
- Line 264: Session creation not logged
- Line 659: Liveness challenge not logged

**Implementation**:
```cpp
class VoiceAuditLogger {
public:
    void logAuthenticationAttempt(
        const std::string& user_id,
        const std::string& method,  // "liveness", "password", "2fa"
        bool success,
        const std::string& reason);
    
    void logSessionLifecycle(
        const std::string& session_id,
        const std::string& event,   // "created", "closed"
        int64_t duration_ms,
        size_t bytes_transferred);
    
    void logLivenessChallenge(
        const std::string& user_id,
        const std::string& challenge_id,
        bool passed,
        const std::string& reason);
    
    void logSpoofDetection(
        const std::string& user_id,
        double spoof_score,
        const std::string& verdict);
};
```

**Audit Event Format**:
```json
{
    "timestamp": "2026-08-28T14:22:31.456Z",
    "event_type": "VOICE_AUTH_ATTEMPT",
    "user_id": "user-12345",
    "method": "liveness",
    "result": "PASS",
    "duration_ms": 2314,
    "session_id": "sess-abc123"
}
```

---

### VOICE MODULE IMPLEMENTATION CHECKLIST

- [ ] **Create VoiceLivenessDetector** (3-4 hrs)
  - [ ] Implement Challenge structure and issuance
  - [ ] Implement speechToTextChallenge() conversion
  - [ ] Implement verifyResponse() with freshness check
  - [ ] Add active challenge tracking + expiry
  - [ ] Unit tests: 5+ test cases

- [ ] **Create VoiceAntiSpoofEngine** (3-4 hrs)
  - [ ] Implement analyzeAudioFreshness() with spectral analysis
  - [ ] Implement analyzeSpeakerMatch() with embedding comparison
  - [ ] Implement analyzeNoisePattern() for synthetic detection
  - [ ] Composite SpoofAnalysis scoring algorithm
  - [ ] Unit tests: 5+ test cases

- [ ] **Update VoiceSessionManager** (2-3 hrs)
  - [ ] Replace permissive auth with fail-closed checks
  - [ ] Add idle timeout (5 min) enforcement
  - [ ] Add absolute timeout (30 min) enforcement
  - [ ] Implement SessionLifetimeGuard RAII wrapper
  - [ ] Add cleanup in exception paths

- [ ] **Create VoiceAuditLogger** (1-2 hrs)
  - [ ] Implement audit event logging infrastructure
  - [ ] Wire into authentication flows
  - [ ] Add to session lifecycle events
  - [ ] Add to liveness challenge flow
  - [ ] Add to spoof detection

- [ ] **Testing** (2 hrs)
  - [ ] Compile: No errors or warnings
  - [ ] Run existing voice tests
  - [ ] Test liveness detection end-to-end
  - [ ] Test anti-spoof with synthetic audio samples
  - [ ] Test session timeout enforcement
  - [ ] Verify audit logs for all events

---

## GPU MODULE BATCH A-8 IMPLEMENTATION

### Current Status
- **Completion**: 40%
- **Blocking Issues**: 16 CRITICAL (unchecked CUDA, memory leaks, use-after-free)
- **Production Readiness**: ❌ NOT READY (memory safety incomplete)

### IMPLEMENTATION TASKS (Priority Order)

#### 1. CUDA ERROR CHECKING WRAPPER (NEW - 0% → 100%)
**File**: `include/gpu/gpu_safe_raii.h` + `src/gpu/gpu_safe_raii.cpp`  
**Priority**: CRITICAL (memory safety gate)

**Requirements**:
- Wrap all CUDA calls with automatic error checking
- Throw exception on CUDA error (exception-safe model)
- Track resource lifecycle (alloc/dealloc)

**Implementation**:
```cpp
// Macro for safe CUDA calls
#define CUDA_CHECK(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        throw std::runtime_error(std::string("CUDA error: ") + \
            cudaGetErrorString(err)); \
    } \
} while(0)

class DeviceMemoryGuard {
    void* ptr_ = nullptr;
    size_t size_ = 0;
    
public:
    template<typename T>
    DeviceMemoryGuard(size_t count) {
        CUDA_CHECK(cudaMalloc(&ptr_, sizeof(T) * count));
        size_ = sizeof(T) * count;
    }
    
    ~DeviceMemoryGuard() {
        if (ptr_) {
            cudaFree(ptr_);  // No error checking in destructor (no-throw)
        }
    }
    
    // Delete copy, allow move
    DeviceMemoryGuard(const DeviceMemoryGuard&) = delete;
    DeviceMemoryGuard& operator=(const DeviceMemoryGuard&) = delete;
    DeviceMemoryGuard(DeviceMemoryGuard&& other) noexcept 
        : ptr_(std::exchange(other.ptr_, nullptr)), 
          size_(std::exchange(other.size_, 0)) {}
    
    T* get() { return static_cast<T*>(ptr_); }
};

class KernelTimeoutGuard {
    cudaStream_t stream_;
    std::thread timeout_thread_;
    std::atomic<bool> completed_{false};
    
public:
    KernelTimeoutGuard(cudaStream_t s, uint32_t timeout_ms);
    ~KernelTimeoutGuard();
    
    void markCompleted() { completed_.store(true); }
};
```

**Unchecked CUDA Calls to Fix** (18 instances):
- `query_accelerator.cpp` lines: 763, 766, 777, 787, 788, 825, 826, 827, 916, 917, 1100
- `memory_pool.cpp`: Allocation/deallocation without error checks
- `unified_memory.cpp`: CPU↔GPU transfers without error checks

---

#### 2. UNIFIED MEMORY CPU/GPU COORDINATION (UPDATE - 30% → 100%)
**File**: `src/gpu/unified_memory.cpp`  
**Priority**: CRITICAL (data corruption risk)

**Requirements**:
- Track ownership (CPU vs GPU has exclusive access)
- Prevent concurrent CPU/GPU access (mutual exclusion)
- Enforce synchronization points (cudaDeviceSynchronize)

**Implementation**:
```cpp
class UnifiedMemoryBuffer {
private:
    enum class Owner { UNOWNED, CPU, GPU };
    std::atomic<Owner> owner_{Owner::UNOWNED};
    void* ptr_ = nullptr;
    size_t size_ = 0;
    
public:
    Result<> acquireForCPU() {
        Owner expected = Owner::UNOWNED;
        if (owner_.compare_exchange_strong(expected, Owner::CPU)) {
            return ok();
        }
        if (expected == Owner::GPU) {
            // GPU still has access - wait for synchronization
            CUDA_CHECK(cudaDeviceSynchronize());
            owner_.store(Owner::CPU);
            return ok();
        }
        return error("Buffer ownership conflict");
    }
    
    Result<> acquireForGPU() {
        Owner expected = Owner::UNOWNED;
        if (owner_.compare_exchange_strong(expected, Owner::GPU)) {
            return ok();
        }
        if (expected == Owner::CPU) {
            // CPU has access - flush caches
            // (On NVIDIA unified memory, this is implicit)
            owner_.store(Owner::GPU);
            return ok();
        }
        return error("Buffer ownership conflict");
    }
    
    Result<> releaseOwnership() {
        owner_.store(Owner::UNOWNED);
        return ok();
    }
};
```

**Coordination Pattern**:
```cpp
// GPU kernel launch
{
    auto gpu_access = buffer.acquireForGPU();
    CUDA_CHECK(kernel<<<blocks, threads>>>(buffer.ptr_));
    CUDA_CHECK(cudaDeviceSynchronize());
    buffer.releaseOwnership();
}

// CPU access
{
    auto cpu_access = buffer.acquireForCPU();
    memcpy(host_ptr, buffer.ptr_, size);
    buffer.releaseOwnership();
}
```

---

#### 3. KERNEL TIMEOUT ENFORCEMENT (NEW - 0% → 100%)
**File**: `src/gpu/kernel_timeout_enforcer.cpp` + `include/gpu/kernel_timeout_enforcer.h`  
**Priority**: HIGH (prevent hangs)

**Requirements**:
- Monitor kernel execution time
- Preempt long-running kernels (10s default)
- Fallback to CPU on timeout

**Implementation**:
```cpp
class KernelTimeoutEnforcer {
public:
    struct KernelConfig {
        uint32_t timeout_ms = 10_000;  // 10 second default
        bool enable_fallback = true;   // Fall back to CPU
        cudaStream_t stream;
    };
    
    Result<> executeWithTimeout(
        const std::function<void()>& kernel_lambda,
        const KernelConfig& config);
    
private:
    void monitorKernelTimeout(
        cudaStream_t stream,
        std::promise<bool>& completion);
};
```

**Timeout Logic**:
```cpp
Result<> executeWithTimeout(...) {
    std::promise<bool> promise;
    auto future = promise.get_future();
    
    std::thread monitor_thread([this, &promise, stream, timeout] {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        // Check if kernel completed
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
            // Timeout! Try to preempt kernel
            cudaStreamDestroy(stream);  // Forcibly end stream
            promise.set_value(false);
        }
    });
    
    try {
        kernel_lambda();
        CUDA_CHECK(cudaDeviceSynchronize());
        promise.set_value(true);
    } catch (...) {
        promise.set_value(false);
        throw;
    }
}
```

---

#### 4. GPU MEMORY POOL SAFETY (UPDATE - 40% → 100%)
**File**: `src/gpu/memory_pool.cpp`  
**Priority**: HIGH (memory leak prevention)

**Requirements**:
- Track allocated blocks (prevent leaks)
- Coalesce adjacent free blocks (fragmentation control)
- Defragmentation when fragmentation > threshold

**Changes**:
```cpp
class GPUMemoryPool {
private:
    struct Block {
        void* ptr;
        size_t size;
        bool in_use;
        Block* next;
    };
    
    Block* head_ = nullptr;
    std::mutex pool_mutex_;
    size_t total_allocated_ = 0;
    size_t total_freed_ = 0;
    
public:
    Result<void*> allocate(size_t size) {
        std::unique_lock lock(pool_mutex_);
        // Find free block
        // Defragment if needed
        // Allocate
    }
    
    Result<> deallocate(void* ptr) {
        std::unique_lock lock(pool_mutex_);
        // Mark block as free
        // Coalesce adjacent free blocks
        // Update fragmentation metric
    }
    
    double getFragmentationRatio() {
        // Return (free_fragmented / total_freed) ratio
        // Target: < 10%
    }
};
```

---

### GPU MODULE IMPLEMENTATION CHECKLIST

- [ ] **Create GPU RAII Wrappers** (3-4 hrs)
  - [ ] Implement CUDA_CHECK macro
  - [ ] Implement DeviceMemoryGuard for allocation/deallocation
  - [ ] Implement KernelTimeoutGuard for kernel execution
  - [ ] Add comprehensive error messages
  - [ ] Unit tests: 5+ test cases

- [ ] **Fix CUDA Error Checking** (4-5 hrs)
  - [ ] query_accelerator.cpp lines 763, 766, 777: Add error checks
  - [ ] query_accelerator.cpp lines 787, 788, 825, 826, 827: Fix use-after-free
  - [ ] query_accelerator.cpp lines 916, 917, 1100: Add error checks
  - [ ] memory_pool.cpp: Wrap all cudaMalloc/cudaFree calls
  - [ ] unified_memory.cpp: Wrap all cudaMemcpy calls

- [ ] **Implement Unified Memory Coordination** (2-3 hrs)
  - [ ] Track CPU/GPU ownership with atomic states
  - [ ] Enforce mutual exclusion on access
  - [ ] Add synchronization points
  - [ ] Unit tests: 3+ test cases (CPU access, GPU access, conflicts)

- [ ] **Implement Kernel Timeout Enforcement** (2-3 hrs)
  - [ ] Create KernelTimeoutEnforcer class
  - [ ] Implement timeout monitoring thread
  - [ ] Add fallback-to-CPU logic
  - [ ] Unit tests: 2+ test cases (normal completion, timeout)

- [ ] **Implement GPU Memory Pool Safety** (2-3 hrs)
  - [ ] Add block tracking (allocation metadata)
  - [ ] Implement coalescing of adjacent free blocks
  - [ ] Add fragmentation tracking metric
  - [ ] Unit tests: 3+ test cases (alloc, dealloc, defrag)

- [ ] **Testing** (2-3 hrs)
  - [ ] Compile: No errors or warnings
  - [ ] Run existing GPU tests
  - [ ] Memory safety: AddressSanitizer clean
  - [ ] Concurrent GPU/CPU access test
  - [ ] Kernel timeout with fallback test
  - [ ] Memory pool fragmentation test

---

## COMBINED IMPLEMENTATION METRICS

| Module | Files | LOC Added | Test Cases | Est. Hrs |
|--------|-------|-----------|-----------|----------|
| Voice: Liveness | 2 | 400 | 5 | 4 |
| Voice: Anti-Spoof | 2 | 500 | 5 | 4 |
| Voice: Session Mgmt | 1 | 200 | 3 | 2 |
| Voice: Audit Log | 2 | 300 | 0 | 2 |
| **Voice Total** | **7** | **1400** | **13** | **12** |
| GPU: RAII Wrappers | 2 | 350 | 5 | 4 |
| GPU: Error Checking | 3 | 150 | 0 | 4 |
| GPU: Unified Memory | 1 | 300 | 3 | 3 |
| GPU: Kernel Timeout | 2 | 250 | 2 | 2 |
| GPU: Memory Pool | 1 | 200 | 3 | 3 |
| **GPU Total** | **9** | **1250** | **13** | **16** |
| **BATCH A-8 Total** | **16** | **2650** | **26** | **28** |

---

## SUCCESS CRITERIA

All items below must be PASS before marking Batch A-8 complete:

**Voice Module**:
- ✅ Liveness detection: Live audio accepted, recorded audio rejected
- ✅ Anti-spoof: Synthetic audio detected and rejected
- ✅ Session lifecycle: Timeouts enforced, fail-closed by default
- ✅ Audit logging: All auth events logged with timestamps
- ✅ Tests: All 13 unit tests pass
- ✅ Security: No auth bypass vulnerabilities found in audit

**GPU Module**:
- ✅ CUDA error checking: All cudaMalloc/cudaFree wrapped
- ✅ Unified memory: No concurrent CPU/GPU access violations
- ✅ Kernel timeout: Long-running kernels preempted at 10s
- ✅ Memory pool: Fragmentation < 10%, no memory leaks
- ✅ Tests: All 13 unit tests pass
- ✅ Safety: AddressSanitizer and ThreadSanitizer clean

---

## Handoff for Week 3 Implementers

This document provides detailed specifications for Voice and GPU hardening. Implementation can begin as soon as Transaction/Sharding/Replication batches (A-6/A-7) are validated.

**Parallelization Strategy**:
- Engineer 1: Voice module (Liveness + Anti-Spoof)
- Engineer 2: Voice audit logging + GPU RAII wrappers
- Engineer 3: GPU unified memory coordination + timeouts + pool safety

**Expected Outcome**: All Voice and GPU modules production-ready for Wave A exit gate validation.
