# Sprint 9: Quick Reference Guide & Implementation Runbook

**Sprint:** 9 - Concurrency & Data Race Prevention  
**Timeline:** 2026-08-11 to 2026-08-24 (Weeks 33-34)  
**Target Release:** v1.5.0 (2026-08-31)  
**Status:** Ready for Kickoff  
**Documentation Last Updated:** 2026-07-27

---

## Quick Start: Day 1 Preparation

### Environment Setup (Day 1 Morning)

```bash
# 1. Update repository
cd /path/to/ThemisDB
git fetch origin
git checkout develop
git pull

# 2. Create feature branch
git checkout -b develop/phase1-4-sprint9-concurrency
git push -u origin develop/phase1-4-sprint9-concurrency

# 3. Review Sprint 9 documentation
cat ai_working/SPRINT_9_CONCURRENCY_KICKOFF.md
cat ai_working/SPRINT_9_GAP_CATALOG.md
cat ai_working/SPRINT_9_TEAM_ASSIGNMENTS.md

# 4. Familiarize with test infrastructure
ls -la tests/security/test_concurrency_*.cpp
cat tools/run_tsan_verification.py
```

### First Test Build (Day 1 Afternoon)

```bash
# 1. Build without TSAN (baseline)
cmake --preset linux-release -B build-release
cmake --build build-release --parallel 16

# 2. Run existing tests to verify baseline
ctest --test-dir build-release --output-on-failure

# 3. Build with TSAN (concurrency safety)
python3 tools/run_tsan_verification.py --repo-root . --parallel 16
```

### First Gap Analysis (End of Day 1)

```bash
# 1. Pick your first gap from SPRINT_9_TEAM_ASSIGNMENTS.md
# 2. Read the detailed gap description in SPRINT_9_GAP_CATALOG.md
# 3. Review the remediation pattern in SPRINT_9_CONCURRENCY_KICKOFF.md
# 4. Locate the file:
cd src/<module>/<file>.cpp

# 5. Create a branch for this gap:
git checkout -b fix/s9-nnn-gap-name
```

---

## Gap-by-Gap Implementation Guide

### How to Fix a Simple Gap (S9-016, S9-017, S9-020)

**Example: S9-016 - Connection Pool Resize Race**

```bash
# File: src/network/connection_pool.cpp
# Function: addConnection()
# Issue: Unsynchronized vector push_back
```

**Steps:**

1. **Analyze current code:**
   ```cpp
   class ConnectionPool {
     std::vector<Connection> connections;  // NOT protected
     
     void addConnection(const Connection& conn) {
       connections.push_back(conn);  // RACE!
     }
   };
   ```

2. **Add mutex:**
   ```cpp
   class ConnectionPool {
     std::mutex pool_mutex_;
     std::vector<Connection> connections;
     
     void addConnection(const Connection& conn) {
       std::lock_guard<std::mutex> lock(pool_mutex_);
       connections.push_back(conn);  // Safe
     }
   };
   ```

3. **Add Doxygen documentation:**
   ```cpp
   /**
    * @brief Add connection to pool (thread-safe)
    * @param conn Connection to add
    * @thread_safety SAFE - mutex-protected
    * @note Implementation fix for S9-016 (Sprint 9)
    */
   void addConnection(const Connection& conn);
   ```

4. **Add test case:**
   - Refer to: `tests/security/test_concurrency_data_race_1.cpp` (template)
   - Create: `tests/security/test_concurrency_container_1.cpp`
   - Test: Multiple threads adding connections concurrently

5. **Verify:**
   ```bash
   # Build with TSAN
   python3 tools/run_tsan_verification.py --skip-build
   
   # Run specific test
   ctest --test-dir build-tsan -R "ConcurrentUpdates" --verbose
   
   # Check for TSAN findings
   grep -i "data race" build-tsan/SPRINT_9_TSAN_REPORT_*.md
   ```

6. **Commit:**
   ```bash
   git add -A
   git commit -m "fix(S9-016): Add mutex protection to connection pool"
   git push
   ```

---

### How to Fix a Medium Gap (S9-001, S9-002, S9-003, ...)

**Example: S9-001 - Partition State Data Race**

**Steps:**

1. **Identify the race condition:**
   - Multiple threads updating `partition_state_` vector concurrently
   - Torn writes possible during reallocation

2. **Choose synchronization strategy:**
   - Option A: Mutex (simplest, most common)
   - Option B: Atomic (for simple types only)
   - Option C: RCU pattern (if many readers, few writers)
   - **Decision:** Mutex (matches existing patterns in module)

3. **Implement fix:**
   ```cpp
   // Header file
   class PartitionManager {
    private:
     mutable std::mutex state_mutex_;
     std::vector<PartitionState> partition_state_;
     
    public:
     void updatePartitionState(int id, State state) {
       std::lock_guard<std::mutex> lock(state_mutex_);
       partition_state_[id].state = state;
     }
   };
   ```

4. **Document lock hierarchy:**
   ```cpp
   // In header file comments:
   /**
    * Lock Hierarchy for Sharding Module:
    * Level 1 (Outermost): ShardingCoordinator::shard_mutex_
    * Level 2:              PartitionManager::state_mutex_
    * Level 3 (Innermost):  Partition::partition_mutex_
    *
    * RULE: Always acquire in order Level 1 → Level 2 → Level 3
    * Acquiring out of order risks deadlock.
    */
   ```

5. **Add test case:**
   ```cpp
   // tests/security/test_concurrency_data_race_1.cpp
   TEST_F(DataRaceTest_PartitionState, ConcurrentUpdates_AllStatesCorrect) {
     // Test template already provided in file
     // Add specific assertions for this gap
   }
   ```

6. **Verify with TSAN:**
   ```bash
   # Full run with ThreadSanitizer
   python3 tools/run_tsan_verification.py
   
   # Expected output: "NO FINDINGS - Concurrency tests clean"
   ```

7. **Submit for code review:**
   ```bash
   git commit -m "fix(S9-001): Add mutex to partition state synchronization

   - Protected partition_state_ vector with state_mutex_
   - All updates now acquire lock before modification
   - Documented lock ordering: Level 1/2
   - Added ConcurrentUpdates test case
   - ThreadSanitizer verified clean"
   
   git push
   ```

---

### How to Fix a Complex Gap (S9-004, S9-005, S9-010, S9-011, S9-014, S9-015)

**Example: S9-004 - Transaction Coordinator Lock Ordering**

**Steps:**

1. **Understand the deadlock pattern:**
   ```
   Thread A: acquire(coord_lock) → acquire(part1_lock) → acquire(part2_lock)
   Thread B: acquire(coord_lock) → acquire(part2_lock) → acquire(part1_lock)
            → DEADLOCK (circular wait)
   ```

2. **Design canonical lock order:**
   ```cpp
   // Transaction Coordinator Lock Hierarchy
   // Level 1: TransactionCoordinator::coord_lock_
   // Level 2: Participant::participant_lock_  (ORDERED by participant_id)
   //
   // RULE: Participants acquired in ID order (1 < 2 < 3 < ...)
   //       Never skip participant IDs (e.g., 1 → 3)
   ```

3. **Implement with ordering validation:**
   ```cpp
   void TransactionCoordinator::acquireLocks() {
     std::lock_guard<std::mutex> coord_lock(coord_lock_);
     
     // Sort participants by ID first
     std::sort(participants_.begin(), participants_.end(),
       [](const auto& a, const auto& b) { return a->id() < b->id(); });
     
     // Acquire in sorted order (prevents circular waits)
     for (auto& part : participants_) {
       std::lock_guard<std::mutex> part_lock(part->lock());
       part->setState(State::LOCKED);
     }
   }
   ```

4. **Add ThreadSanitizer annotation (if available):**
   ```cpp
   #ifdef THREAD_SANITIZER
   AnnotateExpectRaceRelease("coord_lock", &coord_lock_);
   AnnotateBenignRaceBegin();
   // Safely ordered code here
   AnnotateBenignRaceEnd("safe because ordered");
   #endif
   ```

5. **Document exhaustively:**
   ```cpp
   /**
    * @thread_safety SAFE when called sequentially
    * 
    * Lock Ordering Guarantees:
    * - Coordinator lock acquired first (global synchronization)
    * - Participant locks acquired in participant ID order
    * - No participant ID skipping (prevents circular waits)
    * 
    * Deadlock Prevention:
    * - Participants sorted by ID before locking
    * - Canonical order: Coord → Part[1] → Part[2] → Part[3]...
    * - ThreadSanitizer lock-order-inversion detection enabled
    * 
    * Implementation Note (Sprint 9, S9-004):
    * Fixed circular wait deadlock by enforcing participant ID ordering.
    * See ai_working/SPRINT_9_TRANSACTION_LOCK_ORDERING.md for hierarchy.
    */
   ```

6. **Create property-based test:**
   ```cpp
   TEST(Concurrency, LockOrdering_NoDeadlock_100Threads) {
     TransactionCoordinator coord;
     std::vector<std::thread> workers;
     std::atomic<int> deadlock_detected{0};
     
     // Spawn 100 threads, each trying different participant combinations
     for (int i = 0; i < 100; ++i) {
       workers.push_back(std::thread([&coord, &deadlock_detected, i]() {
         try {
           coord.acquireLocks();  // Should complete without deadlock
         } catch (const std::exception& e) {
           if (std::string(e.what()).find("deadlock") != std::string::npos) {
             deadlock_detected++;
           }
         }
       }));
     }
     
     for (auto& w : workers) w.join();
     EXPECT_EQ(deadlock_detected, 0);
   }
   ```

7. **Run extended TSAN verification:**
   ```bash
   # Extended run (10x timeout) for deadlock detection
   python3 tools/run_tsan_verification.py --verbose
   
   # Monitor for lock-order-inversion warnings
   grep -i "lock-order-inversion" build-tsan/*.log
   ```

8. **Submit with detailed PR:**
   ```bash
   git commit -m "fix(S9-004): Enforce canonical lock ordering in transaction coordinator

   Prevents deadlock by acquiring participant locks in participant ID order.
   
   Changes:
   - Sort participants by ID before acquiring locks
   - Document lock hierarchy: Coord → Part[1] → Part[2]...
   - Added exhaustive thread-safety documentation
   - Added property-based test (100 threads, no deadlock)
   - ThreadSanitizer verified clean (lock-order-inversion detection enabled)
   
   References:
   - SPRINT_9_TRANSACTION_LOCK_ORDERING.md (lock hierarchy)
   - tests/security/test_concurrency_lock_ordering_1.cpp (test case)
   - Issue #5372 (transaction coordinator deadlock)"
   
   git push
   ```

---

## Testing Workflow

### Before Submitting Each Gap Fix:

1. **Build clean (no TSAN first):**
   ```bash
   cmake --preset linux-release -B build-release
   cmake --build build-release --parallel 16 --target module_<module>_<gap>_test
   ctest --test-dir build-release -R "<gap>" --verbose
   ```

2. **Build with TSAN:**
   ```bash
   python3 tools/run_tsan_verification.py --skip-build --verbose
   ```

3. **Check for regressions:**
   ```bash
   # Run full test suite (should be fast for one gap)
   ctest --test-dir build-release -L "unit" --output-on-failure
   ```

4. **Verify performance (for every 5 gaps):**
   ```bash
   # Quick baseline check
   python3 benchmarks/wave7/report_variance_w7.py --suite quick
   ```

### Checkpoint Submission Workflow:

**Every Friday end-of-day:**

```bash
# 1. Ensure all local changes committed
git status  # Should be clean

# 2. Create checkpoint PR
git log --oneline HEAD~5..HEAD  # Review commits

# 3. Push to PR branch
git push

# 4. Create PR with template:
#    Title: "Sprint 9 Checkpoint N: Gaps S9-XXX to S9-YYY"
#    Description: 
#      - Gaps completed this checkpoint
#      - Test results (X passed, 0 failed)
#      - TSAN findings (0)
#      - Performance delta (< 2%)
#      - Blockers/issues for next week
```

---

## Common Pitfalls & How to Avoid Them

### Pitfall 1: Missing Predicate in Condition Variable

**WRONG:**
```cpp
void wait() {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock);  // ❌ Lost wakeup possible
  process();
}
```

**RIGHT:**
```cpp
void wait() {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [this]() { return condition_met_.load(); });  // ✅
  process();
}
```

### Pitfall 2: Unsafe Double-Checked Locking

**WRONG:**
```cpp
if (ptr == nullptr) {  // ❌ First check without lock
  std::lock_guard<std::mutex> lock(mutex);
  if (ptr == nullptr) {
    ptr = new T();  // Write not visible!
  }
}
```

**RIGHT:**
```cpp
std::atomic<T*> ptr{nullptr};

if (ptr.load(std::memory_order_acquire) == nullptr) {
  std::lock_guard<std::mutex> lock(mutex);
  T* expected = nullptr;
  ptr.compare_exchange_strong(expected, new T(),
    std::memory_order_release,
    std::memory_order_acquire);  // ✅
}
```

### Pitfall 3: Wrong Lock Ordering

**WRONG:**
```cpp
// Thread A
std::lock_guard<std::mutex> lock1(mutex_a);
std::lock_guard<std::mutex> lock2(mutex_b);

// Thread B
std::lock_guard<std::mutex> lock2(mutex_b);  // ❌ Opposite order!
std::lock_guard<std::mutex> lock1(mutex_a);
// → DEADLOCK
```

**RIGHT:**
```cpp
// DEFINE canonical order in code comment:
// Level 1: mutex_a
// Level 2: mutex_b

// ALL threads follow this order:
std::lock_guard<std::mutex> lock_a(mutex_a);   // ✅ L1 first
std::lock_guard<std::mutex> lock_b(mutex_b);   // ✅ L2 second
```

### Pitfall 4: Forgetting ThreadSanitizer Verification

**AFTER every gap fix:**
```bash
# Run TSAN to confirm no regressions
python3 tools/run_tsan_verification.py --skip-build

# Expected: "NO FINDINGS"
# If failures appear, debug immediately!
```

---

## Emergency Contacts & Escalation

### If ThreadSanitizer Reports a Data Race:

1. **Capture full output:**
   ```bash
   python3 tools/run_tsan_verification.py --verbose > tsan_output.txt
   ```

2. **Analyze the race trace:**
   - Location of conflicting accesses
   - Thread IDs and timing
   - Lock state (if available)

3. **Determine if true positive or false positive:**
   - True positive: Race is real, fix required
   - False positive: Safe pattern, document in SPRINT_9_FALSE_POSITIVE_PATTERNS.md

4. **If true positive:**
   - Add protective synchronization
   - Re-run TSAN to verify fix
   - Add regression test

### If Build Fails:

1. **Check compiler errors first:**
   ```bash
   cmake --build build-release 2>&1 | head -50
   ```

2. **Clean rebuild:**
   ```bash
   rm -rf build-release build-tsan
   python3 tools/run_tsan_verification.py
   ```

3. **Ask for help:**
   - Post to daily standup
   - Reference specific gap ID (S9-XXX)
   - Share error messages

---

## Key Metrics to Track

### Daily:
- [ ] Gaps completed (target: 1-1.5 per day)
- [ ] Test pass rate (target: 100%)
- [ ] TSAN findings (target: 0)

### Weekly:
- [ ] Gaps completed (target: 5-7 per week)
- [ ] Checkpoint PR status
- [ ] Performance delta (target: < 2%)
- [ ] Code review feedback turnaround (< 24h)

### Sprint:
- [ ] Total gaps: 20/20 (100%)
- [ ] Test coverage: 5+ test cases
- [ ] Lock ordering documented: 5 modules
- [ ] Release gate ready: ✅

---

## References & Resources

### Key Documentation
- **Kickoff Plan:** `ai_working/SPRINT_9_CONCURRENCY_KICKOFF.md`
- **Gap Catalog:** `ai_working/SPRINT_9_GAP_CATALOG.md` (all 20 gaps with details)
- **Team Assignments:** `ai_working/SPRINT_9_TEAM_ASSIGNMENTS.md` (schedule & checkpoints)
- **False Positive Patterns:** `SPRINT_9_GAP_CATALOG.md` (§FP Analysis Reference)

### Code References
- **SafeConcurrency Library:** `include/security/safe_concurrency.h`
- **Test Template:** `tests/security/test_concurrency_data_race_1.cpp`
- **TSAN Runner:** `tools/run_tsan_verification.py`

### External Resources
- **ThreadSanitizer Manual:** https://github.com/google/sanitizers/wiki/ThreadSanitizerManual
- **C++ Concurrency Guide:** https://en.cppreference.com/w/cpp/thread
- **Lock Ordering Best Practices:** src/transaction/LOCK_ORDERING_GUIDE.md (TBD)

---

**Sprint 9 Implementation Guide**  
**Last Updated:** 2026-07-27  
**Next Update:** 2026-08-11 (kickoff)  
**Ready for Implementation:** ✅ YES
