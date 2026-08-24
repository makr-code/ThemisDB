# Voice Module Phase 1 - Session Lifecycle Hardening Kickoff

**Phase**: Phase 1 (Session & Streaming Hardening)  
**Timeline**: Sep 1-30, 2026  
**Primary Agent**: themisdb-implementer  
**Secondary Agent**: themisdb-reviewer (parallel error-handling)  
**Target**: voice_session_manager.{cpp,h} state machine formalization  

---

## Quick Start (5 min read)

### What We're Building
A **formal session state machine** with atomic transitions, bounded chunk validation, and comprehensive diagnostics emission. Target: production-grade session lifecycle under concurrent load.

### Key Milestones
- **Sep 1-10**: State machine formalization + implementation
- **Sep 11-20**: Atomic transitions + concurrency hardening
- **Sep 21-30**: Bounded chunk validation + VSM-01..08 tests

### Success Criteria (Go/No-Go Gate Sep 30)
- ✅ All VSM-01..VSM-08 tests passing
- ✅ Build: `module_voice_test_session_manager_focused_focused` green
- ✅ Code coverage ≥ 85%
- ✅ Zero race conditions under stress test (VSM-07)
- ✅ Diagnostics emitted on every state change
- ✅ Error codes [8000-8010] reserved and used

---

## 1. Component Overview

### 1.1 Current State

**File**: `/home/runner/work/ThemisDB/ThemisDB/src/voice/voice_session_manager.cpp`  
**Header**: `/home/runner/work/ThemisDB/ThemisDB/include/voice/voice_session_manager.h`  
**Current LOC**: 333 (implementation file)  
**Test Files**:
- `tests/voice/test_voice_session_manager_focused.cpp` (3,117 LOC)
- `tests/voice/test_voice_session_manager_createSession_focused.cpp` (3,798 LOC)

**Current Functionality**:
- Session lifecycle management
- State transitions (basic)
- Isolation guarantees
- Some error handling (to be hardened)

**Gaps to Address** (from Phase 0 findings):
- Session state-machine NOT formally defined (Gap: 1 critical finding)
- State-transition guards incomplete (Gap: session state-race under high load)
- Bounded chunk validation missing
- Diagnostics emission incomplete

### 1.2 Architecture & Dependencies

```
voice_session_manager.cpp
├─ Uses: std::mutex, std::lock_guard (synchronization)
├─ Uses: spdlog (logging/diagnostics)
├─ Uses: ErrorContext (error structures)
├─ Exposes: VoiceSession interface (public API)
├─ Tests: test_voice_session_manager_focused.cpp (VSM-01..08)
└─ Integrates: voice_assistant (orchestration)
```

**Key Classes** (from include/voice/voice_session_manager.h):
- `VoiceSession` - main session interface
- `SessionState` - enum/state definition (to be formalized)
- `SessionRegistry` - session tracking
- `SessionLifecycleManager` - orchestrator

---

## 2. Phase 1 Implementation Tasks

### Task 1.1: Formalize State Machine (Sep 1-10)

**Objective**: Define explicit session states and valid transitions.

**States** (to formalize):
```cpp
enum class SessionState {
    CREATED = 0,      // Session created, awaiting activation
    ACTIVE = 1,       // Session active, processing audio
    CLOSING = 2,      // Session closing (graceful shutdown)
    CLOSED = 3        // Session closed (final state)
};

// Valid transitions (to enforce):
// CREATED → ACTIVE
// ACTIVE → CLOSING
// CLOSING → CLOSED
// (no other transitions allowed; invalid → fail-closed)
```

**Implementation Pattern**:
1. Add `SessionState` enum to voice_session_manager.h
2. Add state transition matrix (2D bool array or switch-based validator)
3. Implement `canTransition(from_state, to_state) → bool`
4. Add `onStateChange(from, to)` hook for diagnostics
5. Document preconditions/postconditions for each state

**Tests to Create** (VSM-01..04):
- **VSM-01**: State enum values correct
- **VSM-02**: canTransition() validates all valid transitions
- **VSM-03**: canTransition() rejects all invalid transitions
- **VSM-04**: onStateChange() diagnostics emitted

**Build & Test**:
```bash
ctest -R "VSM-01|VSM-02|VSM-03|VSM-04" --verbose
```

### Task 1.2: Implement Atomic Transitions (Sep 11-20)

**Objective**: Prevent race conditions; ensure thread-safe state changes.

**Pattern**:
```cpp
class VoiceSessionManager {
    std::mutex session_state_lock_;
    
    Status transitionState(SessionId id, SessionState target) {
        std::lock_guard<std::mutex> lock(session_state_lock_);
        
        auto session = findSession(id);
        if (!session) return Status(VOICE_SESSION_NOT_FOUND, "");
        
        SessionState current = session->state();
        if (!canTransition(current, target)) {
            return Status(VOICE_SESSION_INVALID_TRANSITION, 
                         "Cannot transition from " + stateToString(current) + 
                         " to " + stateToString(target));
        }
        
        session->setState(target);
        onStateChange(current, target);  // Emit diagnostics
        return Status::OK();
    }
};
```

**Locking Strategy**:
- ✅ Use `std::lock_guard<std::mutex>` (RAII pattern)
- ✅ Lock held only during state assignment (minimize critical section)
- ✅ All state reads/writes go through protected methods
- ✅ No nested locking (prevent deadlocks)

**Concurrency Tests** (VSM-05..07):
- **VSM-05**: Single session, sequential transitions (baseline)
- **VSM-06**: Multiple sessions, concurrent transitions (no interference)
- **VSM-07**: Stress test: 50 sessions × 100 transitions each (race detection via thread sanitizer)

**Build & Test**:
```bash
# Compile with ThreadSanitizer to detect races
export CXXFLAGS="-fsanitize=thread -g"
cmake --preset develop
cmake --build --preset develop --target module_voice_test_session_manager_focused
ctest -R "VSM-05|VSM-06|VSM-07" --verbose
```

### Task 1.3: Bounded Chunk Validation (Sep 15-25)

**Objective**: Prevent unbounded queue growth; enforce max chunk sizes.

**Constants** (add to voice_session_manager.h):
```cpp
static constexpr size_t MAX_AUDIO_CHUNK_BYTES = 65536;  // 64 KB per chunk
static constexpr size_t MAX_SESSION_QUEUE_SIZE = 1000;  // max chunks queued
static constexpr size_t MAX_BUFFER_GROWTH_FACTOR = 2.0; // queue overflow threshold
```

**Validation Logic**:
```cpp
Status VoiceSessionManager::validateAndQueueChunk(SessionId id, const AudioChunk& chunk) {
    std::lock_guard<std::mutex> lock(session_state_lock_);
    
    auto session = findSession(id);
    if (!session || session->state() != SessionState::ACTIVE) {
        return Status(VOICE_SESSION_INACTIVE, "");
    }
    
    // Validate chunk size
    if (chunk.size() > MAX_AUDIO_CHUNK_BYTES) {
        return Status(VOICE_CHUNK_TOO_LARGE, 
                     "Chunk size " + std::to_string(chunk.size()) + 
                     " exceeds max " + std::to_string(MAX_AUDIO_CHUNK_BYTES));
    }
    
    // Check queue capacity
    if (session->queueSize() >= MAX_SESSION_QUEUE_SIZE) {
        // Drop chunk (fail-closed) and emit diagnostic
        diagnostic_emitter_->emit(DiagnosticLevel::WARNING, 
                                  VOICE_QUEUE_FULL, 
                                  "Session queue full; dropping chunk");
        return Status(VOICE_QUEUE_FULL, "");
    }
    
    session->queueChunk(chunk);
    return Status::OK();
}
```

**Tests** (VSM-02, VSM-04, VSM-08):
- **VSM-02** (revised): Oversized chunks rejected
- **VSM-04** (revised): Queue overflow triggers diagnostic
- **VSM-08**: Long-running session (1000+ chunks); queue stays bounded

**Build & Test**:
```bash
ctest -R "VSM-02|VSM-04|VSM-08" --verbose
```

### Task 1.4: Diagnostics & Error Emission (Sep 20-30)

**Objective**: Comprehensive error reporting and state-change tracking.

**Error Codes** (reserve [8000-8010]):
```cpp
// Include in include/voice/voice_session_manager.h
#define VOICE_SESSION_INVALID_TRANSITION 8001
#define VOICE_SESSION_NOT_FOUND 8002
#define VOICE_CHUNK_TOO_LARGE 8003
#define VOICE_QUEUE_FULL 8004
#define VOICE_SESSION_INACTIVE 8005
#define VOICE_SESSION_LOCK_TIMEOUT 8006
#define VOICE_SESSION_RESOURCE_EXHAUSTED 8007
#define VOICE_SESSION_STATE_CORRUPTED 8008
#define VOICE_SESSION_CLEANUP_FAILED 8009
#define VOICE_SESSION_TRANSITION_UNKNOWN 8010
```

**Diagnostic Emitter Integration**:
```cpp
class VoiceSessionManager {
    std::shared_ptr<DiagnosticEmitter> diagnostic_emitter_;
    
    void onStateChange(SessionState from, SessionState to) {
        diagnostic_emitter_->emit(
            DiagnosticLevel::INFO,
            VOICE_SESSION_STATE_CHANGED,  // Use common code
            "Session state: " + stateToString(from) + " → " + stateToString(to)
        );
    }
    
    void onError(int error_code, const std::string& message) {
        diagnostic_emitter_->emit(
            DiagnosticLevel::ERROR,
            error_code,
            message
        );
    }
};
```

**Tests** (VSM-03, VSM-08):
- **VSM-03** (revised): Error code emitted on invalid transition
- **VSM-08** (revised): All state changes logged; trace reconstructable

**Build & Test**:
```bash
ctest -R "VSM-03|VSM-08" --verbose --output-on-failure
```

---

## 3. Test Plan: VSM-01..VSM-08

### Test Matrix

| ID | Name | Category | Assertion | Success Criteria |
|---|---|---|---|---|
| **VSM-01** | State enum values | Unit | Enum values in [0,3] range | Enum defined, values correct |
| **VSM-02** | Chunk validation | Unit | Oversized chunks rejected | Returns VOICE_CHUNK_TOO_LARGE |
| **VSM-03** | Invalid transition rejection | Unit | Invalid transitions fail | canTransition() returns false |
| **VSM-04** | Diagnostics on error | Unit | Diagnostic emitted on error | Listener receives event |
| **VSM-05** | Sequential transitions | Unit | Valid sequence succeeds | 4-state journey OK |
| **VSM-06** | Concurrent sessions | Concurrency | 10 sessions ≠ interfere | Each session isolated |
| **VSM-07** | Stress test (race detection) | Stress | 50 × 100 transitions, no races | ThreadSanitizer clean |
| **VSM-08** | Long-running session | Endurance | 1000+ chunks, queue stays bounded | Memory + CPU stable |

### Test Implementation Pattern

```cpp
// In tests/voice/test_voice_session_manager_focused.cpp

#include <gtest/gtest.h>
#include "voice/voice_session_manager.h"

class VoiceSessionManagerTests : public ::testing::Test {
protected:
    VoiceSessionManager manager_;
    std::shared_ptr<MockDiagnosticEmitter> emitter_;
    
    void SetUp() override {
        emitter_ = std::make_shared<MockDiagnosticEmitter>();
        manager_.setDiagnosticEmitter(emitter_);
    }
};

// VSM-01: State enum
TEST_F(VoiceSessionManagerTests, VSM01_StateEnumValuesCorrect) {
    EXPECT_EQ(static_cast<int>(SessionState::CREATED), 0);
    EXPECT_EQ(static_cast<int>(SessionState::ACTIVE), 1);
    EXPECT_EQ(static_cast<int>(SessionState::CLOSING), 2);
    EXPECT_EQ(static_cast<int>(SessionState::CLOSED), 3);
}

// VSM-02: Chunk validation
TEST_F(VoiceSessionManagerTests, VSM02_OversizedChunksRejected) {
    auto session_id = manager_.createSession();
    manager_.transitionState(session_id, SessionState::ACTIVE);
    
    AudioChunk large_chunk(MAX_AUDIO_CHUNK_BYTES + 1);
    auto status = manager_.validateAndQueueChunk(session_id, large_chunk);
    
    EXPECT_EQ(status.error_code(), VOICE_CHUNK_TOO_LARGE);
}

// VSM-03: Invalid transition rejection
TEST_F(VoiceSessionManagerTests, VSM03_InvalidTransitionsRejected) {
    EXPECT_FALSE(manager_.canTransition(SessionState::CREATED, SessionState::CLOSED));
    EXPECT_FALSE(manager_.canTransition(SessionState::CLOSED, SessionState::ACTIVE));
    EXPECT_FALSE(manager_.canTransition(SessionState::ACTIVE, SessionState::CREATED));
}

// VSM-04: Diagnostics emitted
TEST_F(VoiceSessionManagerTests, VSM04_DiagnosticsOnError) {
    auto session_id = manager_.createSession();
    manager_.transitionState(session_id, SessionState::ACTIVE);
    
    AudioChunk oversized(MAX_AUDIO_CHUNK_BYTES + 1);
    manager_.validateAndQueueChunk(session_id, oversized);
    
    EXPECT_CALL(*emitter_, emit).Times(::testing::AtLeast(1));
    EXPECT_TRUE(emitter_->hasEventWithCode(VOICE_CHUNK_TOO_LARGE));
}

// ... VSM-05..08 follow similar patterns
```

### Build & Test Command

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset develop -DCMAKE_BUILD_TYPE=Release
cmake --build --preset develop --target module_voice_test_session_manager_focused --parallel 16

# Run all VSM tests
ctest --preset develop -R "module_voice_test_session_manager_focused" \
      --verbose --output-on-failure --timeout 120
```

---

## 4. Code Style & Conventions

### Modern C++ Practices (Mandatory)

- ✅ Use `std::lock_guard<std::mutex>` for RAII locking (no manual unlock)
- ✅ Use `std::shared_ptr` for diagnostic_emitter (shared ownership)
- ✅ Use `auto` for type inference where type is obvious
- ✅ Use `constexpr` for compile-time constants (MAX_AUDIO_CHUNK_BYTES, etc.)
- ✅ Use range-based for loops for container iteration
- ✅ Prefer nullptr over NULL or 0
- ✅ Use `const` correctness on all method parameters

### Error Handling Pattern

```cpp
// Prefer explicit Status return over exceptions
Status transitionState(SessionId id, SessionState target) {
    std::lock_guard<std::mutex> lock(session_state_lock_);
    
    auto session = findSession(id);
    if (!session) {
        onError(VOICE_SESSION_NOT_FOUND, "Session ID not found");
        return Status(VOICE_SESSION_NOT_FOUND, "Session ID not found");
    }
    
    // ... more validation
    return Status::OK();
}
```

### Documentation Pattern (Doxygen)

```cpp
/**
 * @brief Attempt to transition session to target state.
 * 
 * Validates state transition via @ref canTransition(), applies atomic lock,
 * updates state, and emits diagnostics via registered listener.
 * 
 * @param id Session ID
 * @param target Target state (ACTIVE, CLOSING, CLOSED)
 * @return Status::OK() on success; error code + message on failure
 * 
 * @pre Session exists and is in a valid state
 * @post State updated if transition valid; diagnostics emitted
 * 
 * @throws Never (all errors returned via Status)
 * 
 * Thread-safe: protected by session_state_lock_
 */
Status transitionState(SessionId id, SessionState target);
```

---

## 5. Build & Validation Checklist

### Pre-Implementation (Sep 1)

- [ ] Read ROADMAP.md Phase 1 section
- [ ] Review ARCHITECTURE.md Section 4 (integration boundaries)
- [ ] Examine current voice_session_manager.{cpp,h}
- [ ] Review test_voice_session_manager_focused.cpp
- [ ] Understand CMake test registration (tests/voice/CMakeLists.txt)
- [ ] Set up development environment
  ```bash
  cd /home/runner/work/ThemisDB/ThemisDB
  cmake --preset develop -DCMAKE_BUILD_TYPE=Release
  cmake --build --preset develop --target themis_core --parallel 16
  ```
- [ ] Verify build succeeds: `echo $?` should be 0
- [ ] Run existing tests: `ctest --preset develop -R voice --verbose`

### Implementation (Sep 1-30)

- [ ] Task 1.1: State machine formalization (Sep 1-10)
  - [ ] Enum defined + documented
  - [ ] canTransition() implemented
  - [ ] onStateChange() hook added
  - [ ] VSM-01..04 tests passing

- [ ] Task 1.2: Atomic transitions (Sep 11-20)
  - [ ] Mutex locking pattern applied
  - [ ] Lock guards in place
  - [ ] VSM-05..07 tests passing
  - [ ] ThreadSanitizer clean (VSM-07)

- [ ] Task 1.3: Bounded chunk validation (Sep 15-25)
  - [ ] Constants defined (MAX_AUDIO_CHUNK_BYTES, MAX_SESSION_QUEUE_SIZE)
  - [ ] validateAndQueueChunk() implemented
  - [ ] VSM-02, VSM-04, VSM-08 passing

- [ ] Task 1.4: Diagnostics & error codes (Sep 20-30)
  - [ ] Error codes [8000-8010] defined
  - [ ] Diagnostic emitter integrated
  - [ ] All error paths emit diagnostics
  - [ ] VSM-03, VSM-08 passing

### Post-Implementation (Sep 30)

- [ ] All tests passing
  ```bash
  ctest --preset develop -R "module_voice_test_session_manager_focused" \
        --verbose --output-on-failure
  ```

- [ ] Code coverage ≥ 85%
  ```bash
  cmake --build --preset develop --target coverage-voice-session
  # Check coverage report
  ```

- [ ] Static analysis clean
  ```bash
  ctest --preset develop -R "voice_session.*clang-tidy" --verbose
  ```

- [ ] No ThreadSanitizer warnings
  ```bash
  export CXXFLAGS="-fsanitize=thread"
  ctest --preset develop -R "VSM-07" --verbose
  ```

- [ ] Doxygen generation clean
  ```bash
  doxygen Doxyfile
  # Check for warnings in voice_session_manager.h output
  ```

- [ ] Secret scanning clean (no API keys, passwords)
  ```bash
  runtime-tools-secret_scanning with modified files
  ```

---

## 6. Handoff Checklist (To themisdb-implementer)

**Before Starting Phase 1**:

- [ ] You have received this kickoff brief
- [ ] You have read:
  - [ ] VOICE_IMPLEMENTATION_STATUS_PHASE0.md
  - [ ] src/voice/ROADMAP.md (Phase 1 section)
  - [ ] src/voice/ARCHITECTURE.md
  - [ ] VOICE_PHASE0_RISK_ASSESSMENT.md (Section 3.1)
- [ ] You have examined:
  - [ ] src/voice/voice_session_manager.cpp (current implementation)
  - [ ] include/voice/voice_session_manager.h (current interface)
  - [ ] tests/voice/test_voice_session_manager_focused.cpp (existing tests)
- [ ] You understand:
  - [ ] The 4-state machine (CREATED→ACTIVE→CLOSING→CLOSED)
  - [ ] Locking pattern (std::lock_guard + RAII)
  - [ ] Bounded chunk validation strategy
  - [ ] Diagnostics emission pattern
  - [ ] Error code reservation [8000-8010]
- [ ] Your build environment is ready:
  - [ ] `cmake --preset develop` succeeds
  - [ ] `cmake --build --preset develop --target themis_core` succeeds
  - [ ] `ctest --preset develop -R voice --verbose` runs (baseline)
- [ ] You have scheduled:
  - [ ] Sep 1: Kick-off internal review + Task 1.1 start
  - [ ] Sep 10: Task 1.1 completion checkpoint
  - [ ] Sep 20: Task 1.2 completion checkpoint
  - [ ] Sep 25: Task 1.3 completion checkpoint
  - [ ] Sep 30: All tests green; Phase 1 go/no-go decision

---

## 7. Support & Escalation

### Questions?

- **Code Architecture**: Review ARCHITECTURE.md Sections 1-5
- **Test Patterns**: See tests/voice/*.cpp existing tests
- **CMake Build**: See tests/voice/CMakeLists.txt
- **Error Codes**: See src/voice/MODULE_GAPS.md (error taxonomy)
- **Diagnostics**: See include/updates/updates_diagnostic_emitter.h (listener pattern, similar module)

### Blockers?

- **Build Issues**: Contact task-agent for CMake/dependency resolution
- **Design Questions**: Contact themisdb-reviewer for arch review
- **Test Framework Issues**: Contact task-agent for test infrastructure support

### Daily Standup

- **Time**: Suggest 10:00 UTC daily, Sep 1-30
- **Attendees**: themisdb-implementer, themisdb-reviewer, coordination
- **Topics**: Progress on VSM-01..08, blockers, confidence level

---

## 8. Success Criteria Summary

### Phase 1 Exit Gate (Sep 30, 2026)

**MUST Have** (Go-decision blockers):
1. ✅ State machine formalized (CREATED→ACTIVE→CLOSING→CLOSED)
2. ✅ Atomic transitions locked (race-free under load)
3. ✅ **VSM-01..VSM-08 ALL PASSING** (8/8 tests)
4. ✅ Build target: `module_voice_test_session_manager_focused_focused` green
5. ✅ Error codes [8000-8010] reserved and in use
6. ✅ Diagnostics emitted on every state transition + error

**SHOULD Have** (confidence indicators):
- ✅ Code coverage ≥ 85% for voice_session_manager.cpp
- ✅ ThreadSanitizer clean (no race warnings)
- ✅ Doxygen warnings ≤ 0
- ✅ Static analysis findings ≤ 0 (critical/high)
- ✅ Bounded chunk validation enforced
- ✅ Documentation updated (in-code + Doxygen)

**Phase 1 Go-Decision Trigger**: All MUST Have + ≥80% SHOULD Have → Proceed to Phase 1.2 (Stream Teardown)

---

**Phase 1 Kickoff Date**: Sep 1, 2026  
**Phase 1 Exit Gate**: Sep 30, 2026  
**Primary Owner**: themisdb-implementer  
**Secondary Owner**: themisdb-reviewer  

Good luck! 🚀
