# Sprint 9: Concurrency & Data Race Prevention - Kickoff Plan

**Status:** 🚀 Scheduled  
**Date:** 2026-07-27 (Planning Phase)  
**Target Start:** 2026-08-11 (Week 33)  
**Target Completion:** 2026-08-24 (Week 34)  
**Scope:** 20 concurrency gaps remediation  
**Release:** v1.5.0 (2026-08-31)

---

## 1. Gap Analysis Summary

### Overview
- **Total Gaps:** 20 concurrency-related violations
- **Severity:** CRITICAL (Data races, deadlocks, synchronization failures)
- **Categories:**
  - Type A: Data Races (Unsynchronized variable/container access) - ~8 gaps
  - Type B: Lost Wakeup Patterns (Condition variable misuse) - ~5 gaps
  - Type C: Double-Checked Locking Violations - ~4 gaps
  - Type D: Unsynchronized Container Access - ~3 gaps

### Scanner Source
- **From:** Phase 1-4 Gap Remediation Initiative (Sprint 9 batch)
- **Detection:** CWE-366 (Data Race), CWE-760 (Use of One-Way Hash with Predictable Salt)
- **Baseline:** 1,236 → 1,131 gaps (Sprints 5-8 complete), 20 remaining
- **Final Closure:** Achieves 95% gap closure rate (1,151/1,236 gaps)

### Affected Modules (Priority Order)
1. **sharding** - Partition lock synchronization, 2PC coordinator races - 6 gaps
2. **replication** - WAL entry synchronization, multi-replica races - 5 gaps
3. **transaction** - Transaction coordinator lock ordering - 4 gaps
4. **network** - Connection pool synchronization - 3 gaps
5. **cache** - Cache coherence during concurrent eviction - 2 gaps

---

## 2. Approach & Deliverables

### 2.1 SafeConcurrency Library Design

**Location:** `include/security/safe_concurrency.h` + `src/security/safe_concurrency.cpp`

**Components:**

```cpp
namespace themis::security {

// Core RAII-based synchronization wrapper
template<typename Mutex>
class LockGuard {
  // Enhanced lock_guard with deadlock detection
  // Validates lock ordering per module configuration
  // Automatic lock timeout and recovery
};

// Data race detector
class DataRaceDetector {
  // Tracks memory access patterns
  // Detects unsynchronized concurrent access
  // Logs race condition signature for ThreadSanitizer
};

// Condition variable safety wrapper
template<typename Predicate>
class SafeCondition {
  // Wraps std::condition_variable
  // Enforces predicate verification before wait
  // Prevents lost wakeups via spurious wake robustness
};

// Lock ordering enforcer
class LockOrderingValidator {
  // Enforces canonical lock order (module-specific)
  // Detects potential deadlock cycles
  // Validates lock acquisition order consistency
};

// Atomic operation verifier
template<typename T>
class AtomicVerifier {
  // Validates atomic operation memory ordering
  // Ensures acquire-release semantics where needed
  // Flags unnecessary memory barriers
};

} // namespace themis::security
```

**Reference:** `include/security/safe_iterator.h` (510 lines, 44 tests)

---

### 2.2 Remediation Pattern Templates

#### Pattern A: Data Race (Unsynchronized Access)

**Before:**
```cpp
// sharding_coordinator.h
class ShardingCoordinator {
  std::vector<ShardInfo> shard_info;  // NOT protected by mutex
  
  void updateShardStatus(int shard_id, Status status) {
    shard_info[shard_id].status = status;  // DATA RACE!
  }
};
```

**After:**
```cpp
// sharding_coordinator.h
class ShardingCoordinator {
  std::mutex shard_mutex;
  std::vector<ShardInfo> shard_info;  // Protected by mutex
  
  void updateShardStatus(int shard_id, Status status) {
    std::lock_guard<std::mutex> lock(shard_mutex);
    shard_info[shard_id].status = status;  // Safe
  }
};
```

**Test Case:**
```cpp
// tests/security/test_concurrency_data_race_1.cpp
TEST(SafeConcurrency, DataRaceDetection_ShardUpdate) {
  ShardingCoordinator coord;
  std::vector<std::thread> workers;
  
  // Spawn workers that race on shard status update
  for (int i = 0; i < 10; ++i) {
    workers.push_back(std::thread([&coord, i]() {
      for (int j = 0; j < 100; ++j) {
        coord.updateShardStatus(0, Status::ACTIVE);
      }
    }));
  }
  
  for (auto& w : workers) w.join();
  
  // Verify no use-after-update or torn reads
  EXPECT_EQ(coord.getShardStatus(0), Status::ACTIVE);
}
```

#### Pattern B: Lost Wakeup Pattern

**Before:**
```cpp
// replication_manager.h
class ReplicationManager {
  std::condition_variable cv;
  std::vector<WALEntry> pending_entries;
  
  void waitForReplication() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock);  // LOST WAKEUP: predicate not checked!
  }
  
  void notifyReplicated() {
    std::lock_guard<std::mutex> lock(mutex);
    // Signal sent but consumer might have already passed the check
    cv.notify_one();
  }
};
```

**After:**
```cpp
// replication_manager.h
class ReplicationManager {
  std::condition_variable cv;
  std::vector<WALEntry> pending_entries;
  std::atomic<bool> all_replicated{false};
  
  void waitForReplication() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this]() { return all_replicated.load(); });
  }
  
  void notifyReplicated() {
    std::lock_guard<std::mutex> lock(mutex);
    all_replicated.store(true);
    cv.notify_one();
  }
};
```

**Test Case:**
```cpp
// tests/security/test_concurrency_lost_wakeup_1.cpp
TEST(SafeConcurrency, LostWakeupPrevention_ReplicationWait) {
  ReplicationManager rm;
  std::atomic<int> completed{0};
  
  std::thread waiter([&rm, &completed]() {
    rm.waitForReplication();
    completed++;
  });
  
  std::thread notifier([&rm]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    rm.notifyReplicated();
  });
  
  waiter.join();
  notifier.join();
  
  EXPECT_EQ(completed, 1);
}
```

#### Pattern C: Double-Checked Locking Violation

**Before:**
```cpp
// transaction_coordinator.h
class TransactionCoordinator {
  Participant* participant = nullptr;
  
  Participant* getParticipant() {
    if (participant == nullptr) {  // First check WITHOUT lock
      std::lock_guard<std::mutex> lock(mutex);
      if (participant == nullptr) {
        participant = new Participant();  // RACE: write not visible to other threads
      }
    }
    return participant;
  }
};
```

**After:**
```cpp
// transaction_coordinator.h
class TransactionCoordinator {
  std::atomic<Participant*> participant{nullptr};
  std::mutex init_mutex;
  
  Participant* getParticipant() {
    Participant* p = participant.load(std::memory_order_acquire);
    if (p == nullptr) {
      std::lock_guard<std::mutex> lock(init_mutex);
      p = participant.load(std::memory_order_relaxed);
      if (p == nullptr) {
        p = new Participant();
        participant.store(p, std::memory_order_release);
      }
    }
    return p;
  }
};
```

**Test Case:**
```cpp
// tests/security/test_concurrency_dcl_1.cpp
TEST(SafeConcurrency, DoubleCheckedLocking_ParticipantInit) {
  TransactionCoordinator coord;
  std::vector<std::thread> workers;
  std::set<Participant*> observed_ptrs;
  std::mutex obs_mutex;
  
  for (int i = 0; i < 10; ++i) {
    workers.push_back(std::thread([&]() {
      Participant* p = coord.getParticipant();
      {
        std::lock_guard<std::mutex> lock(obs_mutex);
        observed_ptrs.insert(p);
      }
    }));
  }
  
  for (auto& w : workers) w.join();
  
  // All threads must see the same Participant instance
  EXPECT_EQ(observed_ptrs.size(), 1);
}
```

#### Pattern D: Unsynchronized Container Access

**Before:**
```cpp
// network_connection_pool.h
class ConnectionPool {
  std::vector<Connection> connections;  // NOT protected
  
  void addConnection(const Connection& conn) {
    connections.push_back(conn);  // DATA RACE on resize/reallocation
  }
};
```

**After:**
```cpp
// network_connection_pool.h
class ConnectionPool {
  std::mutex pool_mutex;
  std::vector<Connection> connections;  // Protected by mutex
  
  void addConnection(const Connection& conn) {
    std::lock_guard<std::mutex> lock(pool_mutex);
    connections.push_back(conn);  // Safe
  }
};
```

**Test Case:**
```cpp
// tests/security/test_concurrency_container_1.cpp
TEST(SafeConcurrency, UnsynchronizedContainerAccess_PoolResize) {
  ConnectionPool pool;
  std::vector<std::thread> workers;
  
  for (int i = 0; i < 20; ++i) {
    workers.push_back(std::thread([&pool, i]() {
      for (int j = 0; j < 50; ++j) {
        Connection conn;
        conn.id = i * 50 + j;
        pool.addConnection(conn);
      }
    }));
  }
  
  for (auto& w : workers) w.join();
  
  EXPECT_EQ(pool.getConnectionCount(), 1000);
}
```

---

## 3. Concurrency Test Suite Design

### 3.1 Test Harness Structure

**Location:** `tests/security/test_concurrency_*.cpp`

**Test Naming Convention:**
- `test_concurrency_data_race_N.cpp` - Data race scenarios (N=1-3)
- `test_concurrency_lost_wakeup_N.cpp` - Lost wakeup patterns (N=1-2)
- `test_concurrency_dcl_N.cpp` - Double-checked locking (N=1)
- `test_concurrency_container_N.cpp` - Container access (N=1-2)
- `test_concurrency_lock_ordering_N.cpp` - Deadlock prevention (N=1)

**Total:** 5+ test files, 10+ test cases

### 3.2 ThreadSanitizer Integration

**CMake Configuration:**
```cmake
# CMakeLists.txt additions
option(ENABLE_THREAD_SANITIZER "Enable ThreadSanitizer for concurrency testing" ON)

if(ENABLE_THREAD_SANITIZER)
  set(TSAN_FLAGS "-fsanitize=thread -fPIE")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TSAN_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TSAN_FLAGS}")
endif()
```

**CTest Label:**
```cmake
set_tests_properties(
  test_concurrency_data_race_1
  PROPERTIES
  LABELS "concurrency;thread-safety;sprint-9"
  TIMEOUT 300  # 5 minute timeout for stress tests
)
```

---

## 4. Lock Ordering Documentation

### 4.1 Module-Specific Lock Hierarchy

**Sharding Module:**
```
Level 1 (Outermost): ShardingCoordinator::shard_mutex
Level 2:             Partition::state_mutex
Level 3 (Innermost): Transaction::lock_mutex

RULE: Always acquire in order Level 1 → Level 2 → Level 3
NEVER acquire backward or skip levels (deadlock risk)
```

**Replication Module:**
```
Level 1: ReplicationManager::sync_mutex
Level 2: WAL::write_mutex
Level 3: Replica::state_mutex

RULE: Respects global timeline: no nested acquisition at same level
```

**Transaction Module:**
```
Level 1: TransactionCoordinator::coord_mutex
Level 2: Participant::state_mutex
Level 3: Lock::holder_mutex

RULE: 2PC protocol enforcement - coordinator lock must precede participant
```

### 4.2 Deadlock Prevention Checklist

For each gap fix:
- [ ] Lock ordering documented in header comments
- [ ] Mutex acquisition order matches hierarchy
- [ ] No circular wait patterns detected (manual review)
- [ ] ThreadSanitizer lock ordering verification clean
- [ ] Code comment explains why specific lock order chosen

---

## 5. Preparation Timeline

### Phase 0: Pre-Sprint Analysis (By 2026-08-04)

**Task:** Extract and catalog all 20 identified concurrency gaps

**Deliverables:**
- [ ] `SPRINT_9_GAP_CATALOG.md` - All 20 gaps listed with:
  - Gap ID and module
  - CWE classification
  - Complexity assessment (Simple/Medium/Complex)
  - False positive risk level
  - Suggested fix pattern
- [ ] `SPRINT_9_MODULE_CLASSIFICATION.md` - Gaps grouped by module
- [ ] `SPRINT_9_FALSE_POSITIVE_PATTERNS.md` - Safe-by-design patterns documented

**Effort:** 2-3 hours (analysis only, no coding)

---

### Phase 1: Semantic Analysis Tooling (By 2026-08-06)

**Task:** Set up ThreadSanitizer and code analysis infrastructure

**Deliverables:**
- [ ] ThreadSanitizer build preset in CMakePresets.json:
  ```json
  {
    "name": "linux-tsan-debug",
    "description": "Linux Debug with ThreadSanitizer",
    "inherits": "base",
    "cacheVariables": {
      "ENABLE_THREAD_SANITIZER": "ON",
      "CMAKE_BUILD_TYPE": "Debug"
    }
  }
  ```
- [ ] `tools/run_tsan_verification.sh` - Automated ThreadSanitizer verification
- [ ] `include/security/safe_concurrency.h` - SafeConcurrency library header
- [ ] `src/security/safe_concurrency.cpp` - Implementation stub

**Effort:** 3-4 hours (infrastructure setup)

---

### Phase 2: Sprint 8 Closure (By 2026-08-10)

**Task:** Archive Sprint 8 and update ROADMAP

**Deliverables:**
- [ ] `SPRINT_8_FINAL_CLOSURE_REPORT.md` - Phase 4-5 completion
- [ ] Updated `ROADMAP.md`:
  - Sprint 8 marked complete with 12 gaps fixed
  - Phase 1-4 overall progress: 91.5% → 95% (post-Sprint 9)
  - Sprint 9 marked as "Starting W33"
- [ ] Updated `CHANGELOG.md` with Sprint 8 closure note

**Effort:** 1-2 hours (documentation only)

---

### Phase 3: Sprint 9 Kickoff (Starting 2026-08-11 — W33)

**Task:** Begin Sprint 9 implementation with team coordination

**Deliverables:**
- [ ] Feature branch: `develop/phase1-4-sprint9-concurrency`
- [ ] `SPRINT_9_TEAM_ASSIGNMENTS.md` - Gap-to-engineer mapping
- [ ] `SPRINT_9_WEEKLY_MILESTONES.md`:
  - Week 33 (Aug 11-17): Gaps 1-10 implementation + 3 test cases
  - Week 34 (Aug 18-24): Gaps 11-20 implementation + 2 test cases + integration testing

**Effort:** 1 hour (coordination and planning)

---

## 6. Implementation Phases (Weeks 33-34)

### Week 33: Gaps 1-10 Implementation & Testing

**Daily Cadence:**
- Day 1-2: Gap analysis and fix design per gap (1 hour/gap)
- Day 3-4: Implementation (1.5 hours/gap average)
- Day 5: Test writing and ThreadSanitizer verification

**Expected Deliverables:**
- [ ] 10 gaps fixed or documented as safe-by-design
- [ ] 3 new concurrency test cases added
- [ ] Initial ThreadSanitizer run (identify remaining issues)
- [ ] Checkpoint PR with code review feedback

### Week 34: Gaps 11-20 + Integration & Verification

**Daily Cadence:**
- Day 1-2: Gaps 11-20 implementation (1.5 hours/gap average)
- Day 3: Final 2 test cases + lock ordering documentation
- Day 4-5: Full regression test suite + ThreadSanitizer clean run

**Expected Deliverables:**
- [ ] All 20 gaps fixed or documented
- [ ] 5+ concurrency test cases all passing
- [ ] ThreadSanitizer clean on full build
- [ ] Lock ordering documented for all affected modules
- [ ] Final code review and sign-off

---

## 7. Success Criteria for Sprint 9

- ✅ All 20 data-race gaps fixed or documented as safe-by-design
- ✅ ThreadSanitizer clean on full build (zero data race warnings)
- ✅ 5+ new concurrency test cases all passing
- ✅ Lock ordering documented in affected code (comments + `SPRINT_9_LOCK_ORDERING.md`)
- ✅ Zero regressions in existing test suite (full CTest passing)
- ✅ < 2% performance delta vs v1.4.0 baseline
- ✅ Code review sign-off from maintainers
- ✅ Gap closure rate achieves 95% (1,151/1,236 gaps remediated)

---

## 8. Risk Factors & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| False positive rate > 60% | Medium | High | Semantic analysis + conservative approach; document safe patterns |
| Deadlock introduction from fixes | Low | High | ThreadSanitizer + property-based tests + lock ordering review |
| Performance regression > 2% | Medium | High | Baseline comparison + load testing + benchmark verification |
| Scope creep beyond 20 gaps | Low | Medium | Strict scope gate per EPIC_001; weekly checkpoint reviews |
| Missed synchronization bugs | Medium | Medium | Code review focus on concurrency patterns; pair programming |
| Test flakiness under TSAN | Medium | Low | Deterministic seeding (kCanonicalSeed=42); repeat tests 3x minimum |

---

## 9. Integration with GA Hardening Program

Sprint 9 completion contributes to:
- **Phase 1-4 Gap Remediation Closure** - 95% overall gap closure rate (v1.5.0 release gate)
- **Release-Critical Path Security** - Concurrency safety for sharding/replication modules
- **Operational Production Readiness** - Thread-safe distributed transaction coordination
- **v1.5.0 Release Readiness** - Final critical blocking item for v1.5.0 GA

---

## 10. Documentation Deliverables

By end of Sprint 9:
- [ ] `SPRINT_9_FINAL_COMPLETION_REPORT.md` - All 20 gaps addressed
- [ ] `CONCURRENCY_SAFETY_PATTERNS_GUIDE.md` - Best practices and examples
- [ ] `RACE_CONDITION_REMEDIATION_GUIDE.md` - How to identify and fix races
- [ ] `SPRINT_9_LOCK_ORDERING.md` - Module-specific lock hierarchies
- [ ] Updated `include/security/safe_concurrency.h` - Full SafeConcurrency library
- [ ] Updated `tests/security/test_concurrency_*.cpp` - 5+ test files with 10+ cases
- [ ] Updated `CHANGELOG.md` - v1.5.0 release notes (concurrency improvements)
- [ ] Updated `ROADMAP.md` - Phase 1-4 marked 95% complete

---

**Last Updated:** 2026-07-27  
**Next Milestone:** Pre-Sprint Analysis Phase starts 2026-07-28  
**Target Start Date:** 2026-08-11 (Week 33)  
