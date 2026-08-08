# BATCH 5: Unified Error Handling & Diagnostics Specification

## Overview

**Target Duration:** 2 weeks  
**Priority:** HIGH (depends on BATCH 1-4 complete)  
**Depends On:** BATCH 1, 2, 3, 4 (all prior work)  
**Scope:** Cross-component unified diagnostics  
**Total Findings:** Cross-component (not isolated to single file)  
**Quality Level:** Production readiness for operators  

---

## Current State Analysis

**Problem:** Components emit errors independently; operators have no unified view
- Backend errors logged separately
- Key derivation errors not linked
- Scheduler diagnostics incomplete
- Tier orchestration errors silent

**Impact:**
- Hard to debug multi-component failures
- No end-to-end tracing across mount→derive→backend→schedule
- Operators can't correlate events by correlation ID
- Missing observability metrics

**Goal:** Unified error handling + operator-facing diagnostics

---

## Implementation Plan

### PHASE 5.1: Standardized Fail-Safe Behavior (HIGH)

#### Objective
Define fail-safe semantics so operators know exactly what happened when something goes wrong.

#### Fail-Safe Principle
**"No silent failures. All errors explicit."**

Whenever an operation fails:
1. Return explicit error code (not undefined behavior)
2. Include context (what, why, how to fix)
3. Emit diagnostic event (for observability)
4. No automatic fallback (operator decides next step)

#### Fail-Safe Contracts Per Component

**1. Backend (gocryptfs_backend.cpp)**

```cpp
namespace backend_contracts {
    // Mount operation fail-safe contract:
    enum class MountError {
        // Timeout - explicit: operation took too long
        kTimeoutWritingKey,      // Key delivery to gocryptfs timed out
        kTimeoutReadingOutput,   // Reading gocryptfs output timed out
        
        // Resource - explicit: missing resources
        kCannotCreatePipe,       // Cannot create pipe for key delivery
        kCannotFork,             // Cannot fork gocryptfs process
        kCannotAllocateBuffer,   // Cannot allocate buffer for I/O
        
        // Validation - explicit: bad input
        kInvalidMountPath,       // Mount path fails validation
        kInvalidKeyMaterial,     // Key material invalid
        kInvalidGocryptfsArgs,   // Command arguments failed validation
        
        // Gocryptfs - explicit: gocryptfs exited with error
        kGocryptfsExitFailure,   // gocryptfs exited non-zero
        kGocryptfsSignal,        // gocryptfs killed by signal
        
        // System - explicit: system resource limit
        kMaxOpenFiles,           // System ran out of file descriptors
        kMaxProcesses,           // System ran out of processes
        kInsufficientMemory,     // System out of memory
    };
    
    // NO "kUnknownError" - all errors must be explicit
    // NO "kTryFallback" - no automatic retries
    // NO "kSilentIgnore" - all errors surfaced
}

// Fail-safe mount:
Result<MountHandle> mount(
    const MountConfig& config,
    string_view correlation_id
) {
    // Check preconditions
    if (config.mount_path.empty()) {
        return error(backend_contracts::MountError::kInvalidMountPath);
    }
    if (config.key_material.empty()) {
        return error(backend_contracts::MountError::kInvalidKeyMaterial);
    }
    
    // Create pipe (explicit error if fails)
    auto pipe = PipeGuard::create();
    if (!pipe.isValid()) {
        return error(backend_contracts::MountError::kCannotCreatePipe);
    }
    
    // Fork gocryptfs (explicit error if fails)
    pid_t pid = fork();
    if (pid < 0) {
        return error(backend_contracts::MountError::kCannotFork);
    }
    
    // ... more explicit checks ...
    
    // If any step fails, return specific error, no fallback
    return Result<MountHandle>::ok(handle);
}
```

**2. Key Derivation (key_derivation_service.cpp)**

```cpp
namespace derivation_contracts {
    enum class DerivationError {
        // Validation
        kInvalidPassword,        // Password fails validation
        kInvalidTenure,          // User tenure invalid
        kInvalidNonce,           // Nonce incorrect size
        kInvalidRounds,          // Derivation rounds out of bounds
        
        // Resource
        kCannotAllocateMemory,   // Heap allocation failed
        kCannotAccessRandom,     // Cannot read /dev/urandom
        
        // Operation
        kArgon2idFailed,         // Argon2id derivation returned error
        kHkdfFailed,             // HKDF derivation returned error
        
        // Timeout (shouldn't happen in derivation, but be explicit)
        kDerivationTimeout,      // Derivation took too long
    };
}

// Fail-safe derivation:
Result<DerivedKey> derive(
    string_view password,
    const UserTenure& tenure,
    string_view correlation_id
) {
    // Validate all inputs explicitly
    if (password.empty() || password.size() > kMaxPasswordLength) {
        return error(derivation_contracts::DerivationError::kInvalidPassword);
    }
    if (tenure.created_at > std::chrono::system_clock::now()) {
        return error(derivation_contracts::DerivationError::kInvalidTenure);
    }
    // ... more validation ...
    
    // If any step fails, return explicit error
    // NO "try using default parameters" or "use weak derivation" fallback
    return Result<DerivedKey>::ok(key);
}
```

**3. Scheduler (key_rotation_scheduler.cpp)**

```cpp
namespace scheduler_contracts {
    enum class ScheduleError {
        // Scheduling
        kCannotScheduleRotation, // Unable to add to rotation queue
        kMaxRotationsScheduled,  // Too many rotations queued
        
        // Callback execution
        kCallbackFailed,         // Callback returned error (after retries)
        kCallbackTimeout,        // Callback execution timed out
        kCallbackInvalid,        // Callback function invalid/null
        
        // Rotation
        kRotationAlreadyInProgress,  // Another rotation running
        kRotationInterrupted,    // Rotation stopped mid-operation
    };
}

// Fail-safe rotation:
Result<void> rotateKeyForUser(
    string_view user_id,
    const KeyRotationPolicy& policy,
    string_view correlation_id
) {
    // Check state
    if (isRotationInProgress(user_id)) {
        return error(scheduler_contracts::ScheduleError::kRotationAlreadyInProgress);
    }
    
    // Execute with retry logic (but explicit fallback after max retries)
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        auto result = executeRotationCallback(...);
        if (result.isOk()) {
            return ok();  // Success
        }
        
        if (attempt < kMaxAttempts - 1) {
            // Retry with backoff
            scheduleRetry(...);
            return ok();  // Scheduled for retry, not failed yet
        }
    }
    
    // After max retries, return explicit error
    return error(scheduler_contracts::ScheduleError::kCallbackFailed);
}
```

**4. Tier Orchestration (multi_level_storage.cpp)**

```cpp
namespace orchestration_contracts {
    enum class OrchestrationError {
        // Tier selection
        kNoHealthyTier,          // All tiers unavailable
        kTierUnavailable,        // Requested tier not available
        
        // Tier state
        kTierQuarantined,        // Tier in quarantine (will retry)
        kTierFailedPermanently,  // Tier failed, manual intervention needed
        
        // Migration
        kMigrationFailed,        // Data migration between tiers failed
        kMigrationTimeout,       // Migration took too long
        
        // Quota
        kQuotaExceeded,          // User storage quota exceeded
        kQuotaEnforcementFailed, // Cannot enforce quota
    };
}

// Fail-safe tier selection:
Result<TierName> selectTierForWrite(
    string_view user_id,
    size_t data_size,
    string_view correlation_id
) {
    // Check if HOT available
    if (hot_status_.isHealthy()) {
        if (hot_quota_available(user_id, data_size)) {
            return ok(TierName::kHot);
        } else {
            return error(orchestration_contracts::OrchestrationError::kQuotaExceeded);
        }
    }
    
    // HOT unavailable, check WARM
    if (warm_status_.isHealthy()) {
        if (warm_quota_available(user_id, data_size)) {
            return ok(TierName::kWarm);
        } else {
            return error(orchestration_contracts::OrchestrationError::kQuotaExceeded);
        }
    }
    
    // WARM unavailable, check COLD
    if (cold_status_.isHealthy()) {
        if (cold_quota_available(user_id, data_size)) {
            return ok(TierName::kCold);
        } else {
            return error(orchestration_contracts::OrchestrationError::kQuotaExceeded);
        }
    }
    
    // All tiers unavailable
    return error(orchestration_contracts::OrchestrationError::kNoHealthyTier);
}
```

#### Implementation Requirements

1. **Define error enums** for all components
   - No generic "kError" or "kUnknown"
   - Each error explains what happened
   - Operators can act on error code

2. **Add error context struct**
   - Error code + message
   - Correlation ID
   - Timestamp
   - Component name
   - Stack trace (if applicable)

3. **Implement fail-safe guards**
   - No automatic retries (explicit scheduling only)
   - No silent fallbacks
   - All errors explicit and actionable

#### Test Strategy

```cpp
TEST(FailSafeBehavior, BackendMountAllErrorsExplicit) {
    // Test that every error path returns explicit error code
    // No path returns undefined/unknown error
    
    // Simulate failures at each point
    for (auto failure_mode : {kPipeFails, kForkFails, kKeyDeliveryFails, ...}) {
        auto result = backend->mount(config);
        EXPECT_FALSE(result.isOk());
        EXPECT_TRUE(result.error() != MountError::kUnknown);
    }
}

TEST(FailSafeBehavior, DerivationNoSilentFallbacks) {
    // Invalid input must return error, never fallback to weak derivation
    EXPECT_FALSE(derive_with_empty_password().isOk());
    EXPECT_FALSE(derive_with_invalid_rounds().isOk());
    EXPECT_FALSE(derive_with_short_nonce().isOk());
    // Not "all errors return same error" - each should be specific
}
```

---

### PHASE 5.2: Unified Diagnostics via DiagnosticEmitter (HIGH)

#### Objective
Implement cross-component diagnostic event system so operators can observe and debug issues.

#### DiagnosticEmitter Pattern

Already used in Updates module (verified in memories). Replicate here:

```cpp
// Include existing diagnostic infrastructure
#include "updates/updates_diagnostic_emitter.h"
#include "updates/updates_diagnostics.h"

namespace user_storage {

/**
 * @class DiagnosticEmitter
 * @brief Thread-safe listener pattern for diagnostic events
 * 
 * Allows backend, key derivation, scheduler, and orchestration to emit
 * structured diagnostic events that operators can observe and react to.
 */
class DiagnosticEmitter {
public:
    // Listener interface
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void onDiagnostic(const DiagnosticEvent& event) noexcept = 0;
    };
    
    // Register listener
    void addListener(std::shared_ptr<Listener> listener);
    void removeListener(std::shared_ptr<Listener> listener);
    
    // Emit event (thread-safe broadcast)
    void emit(const DiagnosticEvent& event) noexcept;
    
private:
    std::vector<std::shared_ptr<Listener>> listeners_;
    mutable std::mutex listeners_mutex_;
};

/**
 * @struct DiagnosticEvent
 * @brief Structured diagnostic event
 */
struct DiagnosticEvent {
    // Identification
    std::string event_type;              // "mount_start", "rotation_failed", etc.
    std::string correlation_id;          // Link events across components
    
    // Timing
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds duration;  // For operation events
    
    // Context
    std::string component;               // "backend", "key_derivation", "scheduler", "orchestration"
    std::string user_id;                 // Affected user
    std::string mount_path;              // Affected mount
    
    // Status
    enum class Status { kSuccess, kFailure, kWarning, kInfo };
    Status status;
    
    // Error details
    std::string error_code;              // MountError::kTimeoutWritingKey, etc.
    std::string error_message;           // Human-readable message
    
    // Additional context
    std::map<std::string, std::string> metadata;  // Key-value pairs
    std::optional<std::string> stack_trace;       // If error includes trace
    
    // Serialization
    std::string toJson() const;
    static DiagnosticEvent fromJson(string_view json);
};

}  // namespace user_storage
```

#### Emit Patterns

**1. Backend Events**

```cpp
void GocryptfsBackend::mount(...) {
    auto correlation_id = generateCorrelationId();
    
    DiagnosticEvent start_event;
    start_event.event_type = "mount_start";
    start_event.correlation_id = correlation_id;
    start_event.component = "backend";
    start_event.timestamp = std::chrono::system_clock::now();
    diagnostics_->emit(start_event);
    
    auto timer = std::chrono::steady_clock::now();
    
    // Attempt mount...
    auto result = mountGocryptfs(...);
    
    DiagnosticEvent end_event;
    end_event.event_type = result.isOk() ? "mount_complete" : "mount_failed";
    end_event.correlation_id = correlation_id;
    end_event.component = "backend";
    end_event.duration = elapsed(timer);
    
    if (!result.isOk()) {
        end_event.status = DiagnosticEvent::Status::kFailure;
        end_event.error_code = getErrorCodeName(result.error());
        end_event.error_message = result.error().message();
    }
    
    diagnostics_->emit(end_event);
    
    return result;
}
```

**2. Key Derivation Events**

```cpp
Result<DerivedKey> KeyDerivationService::derive(
    string_view password,
    const UserTenure& tenure,
    string_view correlation_id
) {
    DiagnosticEvent start_event;
    start_event.event_type = "derivation_start";
    start_event.correlation_id = correlation_id;
    start_event.component = "key_derivation";
    start_event.metadata["tenure_days"] = formatDays(tenure.age());
    diagnostics_->emit(start_event);
    
    auto timer = std::chrono::steady_clock::now();
    auto result = deriveKey(...);
    
    DiagnosticEvent end_event;
    end_event.event_type = result.isOk() ? "derivation_complete" : "derivation_failed";
    end_event.correlation_id = correlation_id;
    end_event.component = "key_derivation";
    end_event.duration = elapsed(timer);
    
    if (!result.isOk()) {
        end_event.status = DiagnosticEvent::Status::kFailure;
        end_event.error_code = getErrorCodeName(result.error());
    }
    
    diagnostics_->emit(end_event);
    return result;
}
```

**3. Scheduler Events**

```cpp
void KeyRotationScheduler::executeRotation(
    const RotationTask& task,
    string_view correlation_id
) {
    DiagnosticEvent start_event;
    start_event.event_type = "rotation_start";
    start_event.correlation_id = correlation_id;
    start_event.component = "scheduler";
    start_event.user_id = task.user_id;
    start_event.metadata["attempt"] = "0";
    diagnostics_->emit(start_event);
    
    auto timer = std::chrono::steady_clock::now();
    
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        auto result = executeCallback(task.callback);
        
        if (result.isOk()) {
            DiagnosticEvent success_event;
            success_event.event_type = "rotation_complete";
            success_event.correlation_id = correlation_id;
            success_event.component = "scheduler";
            success_event.duration = elapsed(timer);
            success_event.status = DiagnosticEvent::Status::kSuccess;
            success_event.metadata["attempts"] = std::to_string(attempt + 1);
            diagnostics_->emit(success_event);
            return;
        }
        
        if (attempt < kMaxAttempts - 1) {
            DiagnosticEvent retry_event;
            retry_event.event_type = "rotation_retry";
            retry_event.correlation_id = correlation_id;
            retry_event.component = "scheduler";
            retry_event.status = DiagnosticEvent::Status::kWarning;
            retry_event.metadata["attempt"] = std::to_string(attempt + 1);
            retry_event.metadata["retry_in_seconds"] = std::to_string(getBackoffSeconds(attempt));
            diagnostics_->emit(retry_event);
        }
    }
    
    // All retries exhausted
    DiagnosticEvent failure_event;
    failure_event.event_type = "rotation_failed";
    failure_event.correlation_id = correlation_id;
    failure_event.component = "scheduler";
    failure_event.duration = elapsed(timer);
    failure_event.status = DiagnosticEvent::Status::kFailure;
    failure_event.error_code = "kCallbackFailed";
    failure_event.metadata["attempts"] = std::to_string(kMaxAttempts);
    diagnostics_->emit(failure_event);
}
```

**4. Tier Orchestration Events**

```cpp
Result<TierName> MultiLevelStorage::selectTierForWrite(
    string_view user_id,
    size_t data_size,
    string_view correlation_id
) {
    DiagnosticEvent event;
    event.event_type = "tier_selection_start";
    event.correlation_id = correlation_id;
    event.component = "orchestration";
    event.user_id = user_id;
    event.metadata["data_size_bytes"] = std::to_string(data_size);
    diagnostics_->emit(event);
    
    // Tier selection logic...
    TierName selected = selectBestTier(...);
    
    DiagnosticEvent result_event;
    result_event.event_type = "tier_selected";
    result_event.correlation_id = correlation_id;
    result_event.component = "orchestration";
    result_event.user_id = user_id;
    result_event.metadata["selected_tier"] = tierNameToString(selected);
    result_event.status = DiagnosticEvent::Status::kInfo;
    diagnostics_->emit(result_event);
    
    return selected;
}
```

#### Integration Test: End-to-End Tracing

```cpp
TEST(UnifiedDiagnostics, EndToEndTracingWithCorrelationId) {
    // Setup diagnostic listener
    std::vector<DiagnosticEvent> events;
    auto listener = std::make_shared<TestDiagnosticListener>(events);
    diagnostics_->addListener(listener);
    
    // Mount user's encrypted storage
    auto correlation_id = "mount-test-abc123";
    auto result = storage->mount(config, correlation_id);
    EXPECT_TRUE(result.isOk());
    
    // Verify all events have same correlation ID
    std::vector<std::string> event_types;
    for (const auto& event : events) {
        EXPECT_EQ(event.correlation_id, correlation_id);
        event_types.push_back(event.event_type);
    }
    
    // Verify complete event chain:
    // mount_start → derivation_start → derivation_complete → 
    // mount_complete → rotation_start
    EXPECT_THAT(event_types, Contains("mount_start"));
    EXPECT_THAT(event_types, Contains("derivation_start"));
    EXPECT_THAT(event_types, Contains("derivation_complete"));
    EXPECT_THAT(event_types, Contains("mount_complete"));
    EXPECT_THAT(event_types, Contains("rotation_start"));
    
    // Operator can grep logs for correlation_id and see full flow
}
```

---

### PHASE 5.3: Operator-Facing Observability Metrics

#### Objective
Expose metrics that operators can monitor via Prometheus/Grafana.

#### Metrics to Expose

```cpp
namespace user_storage {

/**
 * @class StorageMetrics
 * @brief Observability metrics for encrypted storage operations
 */
class StorageMetrics {
public:
    // Mount operations
    uint64_t mount_operations_total = 0;           // Total mount attempts
    uint64_t mount_success_total = 0;              // Successful mounts
    uint64_t mount_failure_total = 0;              // Failed mounts
    
    // Mount latency (percentiles)
    double mount_latency_p50_ms = 0;               // 50th percentile
    double mount_latency_p95_ms = 0;               // 95th percentile
    double mount_latency_p99_ms = 0;               // 99th percentile
    
    // Key rotation
    uint64_t rotation_operations_total = 0;        // Total rotations
    uint64_t rotation_success_total = 0;           // Successful rotations
    uint64_t rotation_failure_total = 0;           // Failed rotations
    uint64_t rotation_retries_total = 0;           // Total retry attempts
    
    // Tier status
    struct TierMetrics {
        std::string tier_name;                     // "HOT", "WARM", "COLD"
        uint64_t operations_total = 0;             // Operations attempted
        uint64_t success_total = 0;                // Operations succeeded
        uint64_t failure_total = 0;                // Operations failed
        uint64_t quarantine_count = 0;             // Times quarantined
        std::chrono::milliseconds latency_avg;     // Average latency
        bool is_healthy = true;                    // Current health status
    };
    std::map<std::string, TierMetrics> tier_metrics;
    
    // Errors (by error code)
    struct ErrorMetrics {
        uint64_t count = 0;
        std::chrono::system_clock::time_point last_occurrence;
    };
    std::map<std::string, ErrorMetrics> error_counts;  // kTimeoutWritingKey → count
    
    // Quota
    uint64_t total_quota_bytes = 0;
    uint64_t used_quota_bytes = 0;
    double quota_utilization_percent = 0;
    
    // Serialization for Prometheus
    std::string toPrometheus() const;
};

}
```

#### Metrics Emitted

**Mount metrics:**
```
user_storage_encrypted_mount_operations_total{instance="node1"} 1250
user_storage_encrypted_mount_success_total{instance="node1"} 1245
user_storage_encrypted_mount_failure_total{instance="node1"} 5
user_storage_encrypted_mount_latency_p50_ms{instance="node1"} 42
user_storage_encrypted_mount_latency_p95_ms{instance="node1"} 128
user_storage_encrypted_mount_latency_p99_ms{instance="node1"} 255
```

**Rotation metrics:**
```
user_storage_encrypted_rotation_operations_total{instance="node1"} 125
user_storage_encrypted_rotation_success_total{instance="node1"} 123
user_storage_encrypted_rotation_failure_total{instance="node1"} 2
user_storage_encrypted_rotation_retries_total{instance="node1"} 4
```

**Tier metrics:**
```
user_storage_encrypted_tier_operations_total{tier="HOT",instance="node1"} 500
user_storage_encrypted_tier_success_total{tier="HOT",instance="node1"} 495
user_storage_encrypted_tier_failure_total{tier="HOT",instance="node1"} 5
user_storage_encrypted_tier_latency_avg_ms{tier="HOT",instance="node1"} 15

user_storage_encrypted_tier_operations_total{tier="WARM",instance="node1"} 300
user_storage_encrypted_tier_success_total{tier="WARM",instance="node1"} 300
user_storage_encrypted_tier_latency_avg_ms{tier="WARM",instance="node1"} 125

user_storage_encrypted_tier_quarantine_count{tier="HOT",instance="node1"} 2
user_storage_encrypted_tier_is_healthy{tier="HOT",instance="node1"} 1
```

**Error metrics:**
```
user_storage_encrypted_error_count{error_code="kTimeoutWritingKey",instance="node1"} 3
user_storage_encrypted_error_count{error_code="kCallbackFailed",instance="node1"} 2
user_storage_encrypted_error_count{error_code="kNoHealthyTier",instance="node1"} 1
user_storage_encrypted_error_last_occurrence{error_code="kTimeoutWritingKey",instance="node1"} 1691234567
```

#### Operator Runbook Template

Create runbook for common failure scenarios:

```markdown
# User Storage Encrypted - Operator Runbook

## HOT Tier Quarantined

**Symptom:**
- `user_storage_encrypted_tier_is_healthy{tier="HOT"} 0`
- Logs show: `[tier_quarantine] tier=HOT reason=<reason>`

**Diagnosis:**
- Grep logs: `grep "<correlation_id>" /var/log/themisdb`
- Check metrics: `user_storage_encrypted_tier_failure_total{tier="HOT"}`

**Action:**
1. Check HOT tier health (SSD status, disk space)
2. If recoverable: System will auto-recover after quarantine period (30s default)
3. If not: Manually trigger WARM/COLD fallback via API

## Mount Timeout

**Symptom:**
- `user_storage_encrypted_error_count{error_code="kTimeoutWritingKey"}` increasing
- Client receives: "Timeout: write to key stdin pipe blocked"

**Diagnosis:**
- Grep logs: `grep "correlation_id=<id>" /var/log/themisdb`
- Check gocryptfs status: `ps aux | grep gocryptfs`

**Action:**
1. Increase timeout in config (default 5s) if I/O is slow
2. Check system load
3. Check FUSE filesystem status

## Key Rotation Callback Failed

**Symptom:**
- `user_storage_encrypted_error_count{error_code="kCallbackFailed"}`  increasing
- Logs show: `[rotation_failed] correlation_id=<id> attempts=3`

**Diagnosis:**
- Grep logs: `grep "rotation_failed" /var/log/themisdb`
- Check callback logs: `grep "<callback_name>" <callback_logs>`

**Action:**
1. Investigate callback implementation
2. Check if callback has external dependencies (network, etc.)
3. Increase max retries if callback is flaky but eventually succeeds

## Quota Exceeded

**Symptom:**
- `user_storage_encrypted_error_count{error_code="kQuotaExceeded"}` increasing
- Mount fails with: "Quota exceeded for tier HOT"

**Diagnosis:**
- `user_storage_encrypted_quota_utilization_percent` near 100%
- Check which users are using most quota

**Action:**
1. Clean up old data or archive to external storage
2. Increase quota if legitimate usage increase
3. Implement quota warnings before hitting hard limit
```

---

## Quality Gates: BATCH 5 Acceptance Criteria

| Criterion | Evidence |
|-----------|----------|
| Fail-safe contracts defined (4 components) | Contract enums + documentation |
| All errors explicit (no silent failures) | Code review + boundary tests |
| DiagnosticEmitter implemented | Emits all operation events |
| End-to-end tracing working | Integration test passes |
| Correlation ID consistent across components | Log grep shows matching IDs |
| Metrics exposed correctly | Prometheus format verified |
| Operator runbook complete | PDF or markdown doc |
| All tests pass | 70+ contract + 20+ BATCH 5 tests |
| Performance no regression | Diagnostic logging < 1ms overhead |

---

## Deliverables Summary

**New Files (3):**
1. include/user_storage_encrypted/diagnostic_emitter.hpp (200+ LOC)
2. src/user_storage_encrypted/diagnostic_emitter.cpp (150+ LOC)
3. docs/user_storage_encrypted/OPERATOR_RUNBOOK.md (200+ LOC)
4. tests/user_storage_encrypted/test_unified_diagnostics_focused.cpp (250+ LOC, 15 tests)

**Modified Files (5):**
1. src/user_storage_encrypted/gocryptfs_backend.cpp (add diagnostics emission)
2. src/user_storage_encrypted/key_derivation_service.cpp (add diagnostics)
3. src/user_storage_encrypted/key_rotation_scheduler.cpp (add metrics)
4. src/user_storage_encrypted/multi_level_storage.cpp (add tier metrics)
5. tests/user_storage_encrypted/CMakeLists.txt (register test target)

**Updated Documentation:**
- src/user_storage_encrypted/ROADMAP.md (mark Phase 2-3 complete)
- docs/user_storage_encrypted/ERROR_TAXONOMY.md (all error codes + meanings)
- docs/user_storage_encrypted/METRICS.md (Prometheus metrics list)

**Estimated Duration:** 2 weeks
