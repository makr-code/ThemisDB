# Voice Module Architecture — Session Lifecycle & Teardown Safety
## Wave A Block 2 Implementation

**Status:** Production Ready  
**Date:** 2026-08-18  
**Maturity:** 🟢 PRODUCTION-READY

---

## 1. Session Lifecycle State Machine

### 1.1 State Transitions (Frozen for Wave A Block 2)

```
┌─────────────────────────────────────────────────────────────┐
│           VOICE SESSION LIFECYCLE STATE MACHINE             │
│              Wave A Block 2: Teardown Safety                │
└─────────────────────────────────────────────────────────────┘

                    ┌──────────────┐
                    │    ACTIVE    │  (User interacting)
                    └──────────────┘
                       ↓         ↓
                  (idle)   (timeout)
                       ↓         ↓
    ┌──────────────┐    ┌──────────────┐
    │     IDLE     │    │   EXPIRED    │  (No activity / timeout)
    └──────────────┘    └──────────────┘
         ↓ (touch)           ↓
    (back to ACTIVE)    (cleanup)
                             ↓
                        ┌──────────────┐
                        │   CLOSING    │  (Teardown in progress)
                        └──────────────┘  (Wave A Block 2)
                             ↓
                        (timeout guard)
                             ↓
                        ┌──────────────┐
                        │  TERMINATED  │  (Resources released)
                        └──────────────┘
```

### 1.2 State Descriptions

| State | Duration | Behavior | Trigger |
|-------|----------|----------|---------|
| **ACTIVE** | Variable | User actively interacting | `createSession()` |
| **IDLE** | ≤ idle_timeout_ms | No activity; waiting for user | idle_timeout exceeded |
| **EXPIRED** | Final | Session exceeded absolute timeout | max_session_duration exceeded |
| **CLOSING** | ≤ closing_grace_period_ms | Graceful shutdown in progress | `terminateSession()` called |
| **TERMINATED** | Final | All resources released; no references | CLOSING timeout or cleanup complete |

### 1.3 Timeout Configuration

```c++
struct SessionTimeoutConfig {
    // Activity-based timeouts
    int64_t idle_timeout_ms = 5 * 60 * 1000;           // 5 min (default)
    int64_t max_session_duration_ms = 30 * 60 * 1000;  // 30 min (default)
    
    // Cleanup/expiration
    int64_t cleanup_interval_ms = 30 * 1000;           // 30 sec (default)
    
    // Wave A Block 2: Teardown safety
    int64_t teardown_timeout_ms = 5 * 1000;            // 5 sec (default)
    int64_t closing_grace_period_ms = 100;             // 100 ms (default)
    
    // Feature flags
    bool auto_expire = true;
    bool enable_timeout_guards = true;                 // Wave A Block 2
};
```

---

## 2. Multi-Session Teardown Safety

### 2.1 Teardown Phases

```
Phase 1: Initiation
├─ Caller invokes terminateSession(session_id)
├─ Session transitions ACTIVE/IDLE/EXPIRED → CLOSING
├─ Audit event logged: "terminateSession" action
└─ Teardown start time recorded in teardown_tracker_

Phase 2: Grace Period (Closing)
├─ Deadline = now + closing_grace_period_ms
├─ Allow cleanup operations (connection flush, etc.)
├─ Timeout guard: if elapsed > teardown_timeout_ms → FAIL-CLOSED
└─ On error: force TERMINATED state

Phase 3: Finalization
├─ Transition CLOSING → TERMINATED
├─ Call finalizeSessionTeardownLocked() to:
│  ├─ Remove session from active_cache_
│  ├─ Call backend.remove(session_id)
│  └─ Erase teardown tracking info
├─ No dangling references remain
└─ Audit event: session fully cleaned up
```

### 2.2 Dangling Reference Elimination

**Guarantee:** After `terminateSession()` returns, the session_id has zero references.

**Implementation:**
1. Session removed from `active_cache_` (in-memory)
2. Session removed from `backend_` (persistent storage)
3. Teardown tracking info cleared
4. All reference counters released (RAII-protected)

**Verification:**
- `getSession(session_id)` returns `std::nullopt` after termination
- `isDoubleCloseAttempt(session_id)` returns true after termination
- No `USE_AFTER_FREE` possible (strict typed access)

### 2.3 Timeout Guard Mechanism

```
START TEARDOWN
     ↓
  START_TIME = now()
     ↓
[Phase 1: Initiate CLOSING]
  elapsed = now() - START_TIME
  if (elapsed > teardown_timeout_ms) → FAIL-CLOSED
     ↓
[Phase 2: Grace Period]
  remaining = teardown_timeout_ms - elapsed
  sleep(min(remaining, closing_grace_period_ms))
     ↓
[Phase 3: Finalize]
  elapsed = now() - START_TIME
  if (elapsed > teardown_timeout_ms) → FORCE TERMINATE
     ↓
TRANSITION → TERMINATED
     ↓
END TEARDOWN
```

**Fail-Closed Behavior:**
- On timeout: Force transition to TERMINATED
- Log error with error_code 6608 (Teardown timeout)
- Release all resources (best-effort cleanup)
- Return false to caller (teardown failed/timed out)

### 2.4 Concurrent Teardown Safety

**Thread Safety Guarantee:** Multiple threads calling `terminateSession()` on different sessions are safe.

**Synchronization:**
- Each session protected by `manager_mutex_`
- Lock held only during state transition (short-lived)
- No deadlock possible (single lock per manager)
- `terminateAllSessions()` serializes operations

**Race Condition Prevention:**
1. **Double-Close:** Detected by `isDoubleCloseAttempt()` before proceeding
2. **Use-After-Free:** Session already removed from cache; can't reuse
3. **Partial Cleanup:** All-or-nothing semantic via RAII + lock

---

## 3. Reverse-Dependency Cleanup Order

### 3.1 Cleanup Sequence (Sessions → Authenticator → Storage)

```
┌──────────────────────────────────────────┐
│ Call terminateAllSessions()              │
└──────────────────────────────────────────┘
         ↓
[Collect all active sessions]
         ↓
For each session:
  ├─ Transition ACTIVE/IDLE → CLOSING → TERMINATED
  ├─ Remove from active_cache_ (memory)
  ├─ Signal authenticator to cleanup user context (if needed)
  └─ Call backend.remove(session_id) (persistent)
         ↓
[All sessions terminated, references cleared]
         ↓
[Authenticator cleanup happens organically:]
  ├─ No sessions reference authenticator data
  ├─ Authenticator state remains (profiles, etc.)
  └─ Explicit cleanup via separate call if needed
         ↓
[Storage cleanup:]
  ├─ Session data removed from backend
  ├─ Audit logs remain (immutable)
  └─ No data leaks
```

### 3.2 Cleanup Order Invariant

**Invariant:** Session cleanup must complete before authenticator/storage cleanup.

**Verification in Tests:**
1. Create sessions tied to users/devices
2. Call `terminateAllSessions()`
3. Assert `getSessionsForUser(user_id).empty()`
4. Assert `backend_.listActiveSessions().empty()`

---

## 4. Observable Metrics

### 4.1 Teardown Diagnostics

```c++
json VoiceSessionManager::getSessionTeardownStatus(const std::string& session_id)
→ {
    "session_id": "sess-abc123",
    "timestamp_ms": 1692360000000,
    "state": "CLOSING" | "TERMINATED" | "UNKNOWN",
    "teardown_start_ms": 1692359999000,
    "elapsed_ms": 1000,
    "pre_closing_state": "ACTIVE",
    "is_tearing_down": true|false,
    "error_code": 0 | 6608  // Error on timeout
}
```

### 4.2 Session Analytics

```c++
SessionAnalytics VoiceSessionManager::getAnalytics()
→ {
    total_sessions: 150,
    active_sessions: 45,
    expired_sessions: 5,
    avg_session_duration_ms: 123456.0,
    avg_turns_per_session: 8.5,
    sessions_by_device: { "device-001": 15, ... },
    sessions_by_language: { "en": 100, "fr": 50, ... }
}
```

---

## 5. Error Codes

| Code | Meaning | Recovery |
|------|---------|----------|
| 6600 | Session creation failed | Retry after clearing cache |
| 6601 | Session not found | Already terminated or never existed |
| 6602 | Session timeout/expiration | Session expired; create new one |
| 6603 | Invalid state transition | Check state machine; wrong operation |
| 6604 | Resource limit exceeded | Too many concurrent sessions |
| 6605 | User ID validation failed | Provide valid non-empty user_id |
| 6608 | **Teardown timeout exceeded (Wave A Block 2)** | Force terminate; check system load |

---

## 6. Production Requirements

### 6.1 Build Flags

```bash
# Wave A Block 2: Enable teardown safety
cmake -DWITH_VOICE_TEARDOWN_SAFETY=ON

# Enable audit logging (mandatory)
cmake -DWITH_VOICE_AUDIT_LOGGING=ON
```

### 6.2 Runtime Configuration

```json
{
    "voice": {
        "session": {
            "teardown_timeout_ms": 5000,
            "closing_grace_period_ms": 100,
            "idle_timeout_ms": 300000,
            "max_session_duration_ms": 1800000
        },
        "audit": {
            "enabled": true,
            "file_path": "/var/log/voice/audit.log",
            "retention_days": 90,
            "rotation_size_mb": 100,
            "enable_syslog": true,
            "syslog_facility": "LOCAL1"
        }
    }
}
```

---

## 7. Testing & Verification

### 7.1 Wave A Block 2 Test Suite

**Multi-Session Teardown Tests:** ≥8 tests
- Single session teardown (baseline)
- Concurrent 10-session teardown
- Concurrent 100-session teardown (stress)
- Timeout enforcement (≤5s)
- Dangling reference elimination
- Reverse-dependency cleanup
- Abort signal cleanup
- Network failure cleanup

**Audit Logging Tests:** ≥6 tests
- authenticate() event logged
- authorize() event logged
- createSession() event logged
- terminateSession() event logged
- Audit persistence (file write)
- Log rotation & retention

**Resource Leak Detection:**
- Valgrind: `--leak-check=full`
- AddressSanitizer: `-fsanitize=address`
- Concurrent teardown under load (100+ iterations)

### 7.2 Acceptance Criteria

- ✅ Zero dangling session references post-teardown
- ✅ Zero unresolved TODO/STUB in teardown paths
- ✅ All security functions emit audit logs
- ✅ Concurrent teardown (≥10 sessions) → no crashes
- ✅ Audit logs survive process restart
- ✅ Fail-closed on timeout/error
- ✅ Resource leak verification (Valgrind clean)
- ✅ All tests passing

---

**End of Architecture Documentation**
