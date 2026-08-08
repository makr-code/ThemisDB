# BATCH 4: Multi-Tier Orchestration Hardening Specification

## Overview

**Target Duration:** 3 weeks  
**Priority:** MEDIUM (depends on BATCH 1-3 stabilizing)  
**Depends On:** BATCH 1, 2, 3 (all prior work)  
**Affected Component:** src/user_storage_encrypted/multi_level_storage.cpp (1121 LOC)  
**Total Findings:** 45 (3 critical + 22 high + 20 medium)  
**Quality Level:** Blocking production readiness (largest surface area)  

---

## Current State Analysis

**File:** src/user_storage_encrypted/multi_level_storage.cpp (1121 LOC)  
**Responsibilities:**
- Manages 3-tier storage hierarchy: HOT (SSD/fast), WARM (HDD/medium), COLD (S3/slow)
- Implements tier-aware caching and eviction policies
- Handles tier transition logic and data migration
- Manages per-tier error recovery and quarantine

**Critical Issues (3):**
1. HOT tier failure cascades to WARM tier (should be isolated)
2. WARM tier failure affects COLD tier lookups (should be independent)
3. Uninitialized tier state on startup (affects initial tier selection)

**High-Severity Issues (22):**
- Resource leaks in exception paths (14 findings)
  - malloc/new without cleanup on errors
  - Unclosed file handles in tier I/O failures
  - Lock leaks when exceptions occur
- Command injection in tier-specific operations (3 findings)
  - S3 CLI arguments not sanitized
  - Path concatenation in tier management
- Smart pointer misuse (5 findings)
  - Use-after-move on shared_ptr
  - Missing move semantics in tier transitions

**Medium-Severity Issues (20):**
- String concatenation in loops (6 findings)
- Expensive copy operations (8 findings)
- Missing const correctness (6 findings)

---

## Implementation Plan

### PHASE 4.1: Tier Isolation & Failure Boundaries (CRITICAL)

#### Objective
Ensure HOT/WARM/COLD failures don't cascade. Each tier failure is local and recoverable.

#### Requirements

**1. Define Per-Tier Contracts**

Each tier must have explicit:
- Success criteria (operation completed, data persisted)
- Failure modes (timeout, unavailable, data corruption)
- Fallback behavior (what happens when tier fails)

Example:
```cpp
namespace tier_contracts {
    struct HotTierContract {
        // Success: Data written to SSD within 50ms
        // Failure: Timeout after 100ms, fall through to WARM
        // Guarantee: HOT failure doesn't affect WARM operations
        
        // If HOT is unavailable:
        // - Reads go directly to WARM/COLD
        // - Writes go to WARM instead
        // - HOT is quarantined for 30s before retry
    };
    
    struct WarmTierContract {
        // Success: Data written to HDD within 500ms
        // Failure: Timeout after 1s, fall through to COLD
        // Guarantee: WARM failure doesn't affect COLD operations
        
        // If WARM is unavailable:
        // - Reads go directly to COLD
        // - Writes go to COLD + fail-open warning
        // - WARM is quarantined for 60s before retry
    };
    
    struct ColdTierContract {
        // Success: Data written to S3 within 5s
        // Failure: Timeout after 10s, return explicit error
        // Guarantee: COLD failures don't affect data integrity
        
        // If COLD is unavailable:
        // - Write fails with explicit error (no silent drop)
        // - Operator intervention required for archive
        // - COLD is marked unavailable until manual recovery
    };
}
```

**2. Implement Tier State Machine**

```cpp
enum class TierState {
    kHealthy,        // Tier operating normally
    kDegraded,       // Tier operating but slow
    kQuarantined,    // Tier temporarily disabled (retry pending)
    kUnavailable,    // Tier permanently failed (requires intervention)
};

class MultiLevelStorage {
private:
    struct TierStatus {
        TierState state = TierState::kHealthy;
        uint64_t failure_count = 0;
        std::chrono::steady_clock::time_point last_failure_time;
        std::string quarantine_reason;
        
        bool isHealthy() const { return state == TierState::kHealthy; }
        bool isQuarantined() const { return state == TierState::kQuarantined; }
        bool shouldRetry() const {
            // Retry after quarantine period
            auto age = std::chrono::steady_clock::now() - last_failure_time;
            return age > std::chrono::seconds(30);
        }
    };
    
    TierStatus hot_status_;
    TierStatus warm_status_;
    TierStatus cold_status_;
};
```

**3. Implement Failure Boundaries**

HOT tier failure → automatically downgrade to WARM (no cascade):
```cpp
Result<std::vector<uint8_t>> readFromHot(string_view key) {
    auto result = hot_tier_->read(key);
    if (!result.isOk()) {
        // HOT failed, DON'T try to be clever
        // Just return error and let caller decide (usually downgrade to WARM)
        hot_status_.state = TierState::kQuarantined;
        hot_status_.failure_count++;
        return error("HOT tier unavailable");
    }
    hot_status_.state = TierState::kHealthy;
    return result;
}

Result<std::vector<uint8_t>> read(string_view key) {
    // Try HOT first (fast path)
    if (hot_status_.isHealthy()) {
        auto result = readFromHot(key);
        if (result.isOk()) return result;
        // HOT failed, continue to WARM
    }
    
    // HOT unavailable or failed, try WARM (medium path)
    if (warm_status_.isHealthy()) {
        auto result = readFromWarm(key);
        if (result.isOk()) return result;
        // WARM failed, continue to COLD
    }
    
    // WARM unavailable or failed, try COLD (slow path)
    if (cold_status_.isHealthy()) {
        return readFromCold(key);  // Let COLD error propagate
    }
    
    return error("All tiers unavailable");
}
```

**4. Test Tier Isolation**

Each test simulates failure of one tier and verifies others work:

```cpp
TEST(MultiLevelStorageIsolation, HotFailureDoesNotAffectWarm) {
    // Setup: All tiers healthy
    auto storage = MultiLevelStorage::create();
    storage->write("key1", data1);
    
    // Simulate HOT tier failure
    mock_hot_->simulateFailure();
    
    // Verify: WARM still works
    auto result = storage->read("key1");
    EXPECT_TRUE(result.isOk());
    
    // Verify: HOT is quarantined, not WARM
    EXPECT_TRUE(storage->isTierQuarantined(TierName::kHot));
    EXPECT_FALSE(storage->isTierQuarantined(TierName::kWarm));
}

TEST(MultiLevelStorageIsolation, WarmFailureDoesNotAffectCold) {
    // Simulate WARM failure
    mock_warm_->simulateFailure();
    
    // Verify: COLD still accessible
    auto result = storage->readFromCold("archive_key");
    EXPECT_TRUE(result.isOk());
    
    // Verify: WARM quarantined, COLD healthy
    EXPECT_TRUE(storage->isTierQuarantined(TierName::kWarm));
    EXPECT_FALSE(storage->isTierQuarantined(TierName::kCold));
}

TEST(MultiLevelStorageIsolation, TierRecovery) {
    // Simulate HOT failure
    mock_hot_->simulateFailure();
    EXPECT_TRUE(storage->isTierQuarantined(TierName::kHot));
    
    // Advance time past quarantine period
    mockClock->advanceSeconds(35);
    
    // HOT recovers (mock stops failing)
    mock_hot_->stopSimulatingFailure();
    
    // Next HOT access should retry successfully
    auto result = storage->write("new_key", new_data);
    EXPECT_TRUE(result.isOk());
}
```

#### Deliverables
- Per-tier contract documentation (3 contracts)
- Tier state machine implementation (TierState enum + TierStatus struct)
- Failure boundary tests (6-8 test cases)
- Quarantine and recovery logic

---

### PHASE 4.2: Resource Leak Fixes (HIGH)

#### Objective
Ensure all exceptions during tier I/O operations don't leak resources.

#### Issues to Fix

**1. malloc/new without cleanup (8 locations)**
- Allocate buffer for tier I/O → exception before free → leak
- Solution: Use std::unique_ptr or std::vector

**2. File handle leaks (4 locations)**
- Open file for tier migration → exception before close → leak
- Solution: Use std::ifstream/std::ofstream (RAII)

**3. Lock leaks (2 locations)**
- Acquire lock for tier transition → exception before unlock → leak
- Solution: Use std::lock_guard (already best practice)

#### Example Pattern: Before & After

```cpp
// BEFORE (leaks on exception):
Result<void> migrateHotToWarm(string_view key, const std::vector<uint8_t>& data) {
    uint8_t* buffer = malloc(data.size() + 100);  // +100 for metadata
    int fd = open(temp_file, O_WRONLY | O_CREAT);
    
    if (!encryptData(data, buffer)) {
        free(buffer);
        close(fd);
        return error("Encryption failed");  // Leaks if encryptData throws
    }
    
    if (write(fd, buffer, data.size()) < 0) {
        free(buffer);
        close(fd);
        return error("Write failed");  // Leaks if write throws
    }
    
    free(buffer);
    close(fd);
    return ok();  // Leak if any step above throws (not just returns error)
}

// AFTER (exception-safe):
Result<void> migrateHotToWarm(string_view key, const std::vector<uint8_t>& data) {
    auto buffer = std::make_unique<uint8_t[]>(data.size() + 100);
    std::ofstream out(temp_file, std::ios::binary);
    if (!out.is_open()) {
        return error("Cannot open temp file");  // buffer auto-deleted
    }
    
    if (!encryptData(data, buffer.get())) {
        return error("Encryption failed");  // buffer auto-deleted, file closed
    }
    
    out.write(reinterpret_cast<char*>(buffer.get()), data.size() + 100);
    if (!out) {
        return error("Write failed");  // buffer auto-deleted, file closed
    }
    
    out.close();  // Explicit close (but destructor will also close)
    return ok();  // buffer auto-deleted on scope exit
}
```

#### Test Strategy

Verify no leaks via RAII cleanup:

```cpp
TEST(MultiLevelStorageResourceLeaks, NoLeaksOnMigrationFailure) {
    // Use valgrind or ASan to detect leaks
    // Simulate failure at each point:
    // 1. Encryption fails → verify buffer freed, file closed
    // 2. Write fails → verify buffer freed, file closed
    // 3. Exception thrown → verify all cleaned up
    
    for (int fail_at = 0; fail_at < kNumFailurePoints; ++fail_at) {
        mock_tier_->simulateFailureAt(fail_at);
        auto result = storage->migrateHotToWarm(...);
        EXPECT_FALSE(result.isOk());
    }
    
    // ASan report: no memory leaks, all file descriptors closed
}
```

#### Deliverables
- Replace malloc/new with std::make_unique
- Replace open/close with std::ifstream/std::ofstream
- Add 6-8 RAII cleanup verification tests

---

### PHASE 4.3: Performance Optimization (MEDIUM)

#### Objective
Optimize copy operations and string concatenation in hot paths.

#### Issues to Fix

**1. String concatenation in loops (6 locations)**
- Problem: Each iteration allocates new string
- Solution: Use std::stringstream or vector::append

```cpp
// BEFORE (6 allocations in loop):
std::string paths;
for (const auto& key : keys) {
    paths = paths + key + ",";  // String reallocation each iteration
}

// AFTER (1 allocation):
std::stringstream paths;
for (const auto& key : keys) {
    paths << key << ",";  // Streaming to buffer
}
std::string result = paths.str();
```

**2. Expensive copy operations (8 locations)**
- Problem: Returning large structures by value
- Solution: Use move semantics or return references

```cpp
// BEFORE (copy in loop):
std::vector<TierData> results;
for (const auto& tier : tiers_) {
    auto data = tier.getData();  // Copies full data
    results.push_back(data);      // Another copy
}

// AFTER (move semantics):
std::vector<TierData> results;
for (const auto& tier : tiers_) {
    auto data = tier.getData();
    results.push_back(std::move(data));  // Move, no copy
}
```

#### Test Strategy

Measure before/after performance:

```cpp
BENCHMARK(MultiLevelStoragePerformance, StringConcatenation) {
    std::vector<std::string> keys = generateKeys(1000);
    for (auto _ : state) {
        std::stringstream paths;
        for (const auto& key : keys) {
            paths << key << ",";
        }
        benchmark::DoNotOptimize(paths.str());
    }
}
```

#### Deliverables
- Replace string concatenation with stringstream (6 locations)
- Add move semantics to copy-heavy operations (8 locations)
- Verify no performance regression via benchmarks

---

### PHASE 4.4: Additional Hardening

#### 1. Command Injection Prevention (3 locations)

S3 tier uses CLI commands. Sanitize all arguments.

```cpp
// Similar to BATCH 2: validate S3 paths, bucket names, credentials
```

#### 2. Smart Pointer Correctness (5 locations)

Fix use-after-move and missing move semantics.

```cpp
// Replace unsafe patterns with const correctness
// Add move constructors/assignment operators where needed
```

#### 3. Const Correctness (6 locations)

Mark read-only methods as const.

```cpp
// getTierStatus() → const
// getStorageQuota() → const
// etc.
```

---

## Quality Gates: BATCH 4 Acceptance Criteria

| Criterion | Evidence |
|-----------|----------|
| Tier isolation working (HOT/WARM/COLD) | 6-8 isolation tests pass |
| All resource leaks fixed | valgrind/ASan clean (14 locations) |
| Failure boundaries explicit | Per-tier contracts documented |
| Tier recovery working | 3-4 recovery tests pass |
| Command injection fixed (3 locations) | Argument validation + fuzzing |
| Smart pointer misuse fixed (5 locations) | Code review + tests |
| String concatenation optimized (6 locations) | Performance benchmark |
| Copy operations optimized (8 locations) | Move semantics verified |
| No performance regression | All benchmarks pass |
| All tests pass | 70+ contract + 20+ new BATCH 4 tests |

---

## Deliverables Summary

**New Files (1):**
- tests/user_storage_encrypted/test_multi_tier_orchestration_focused.cpp (350+ LOC, 18-20 tests)

**Modified Files (2):**
- src/user_storage_encrypted/multi_level_storage.cpp (add tier contracts, state machine, RAII cleanup, optimizations)
- tests/user_storage_encrypted/CMakeLists.txt (register new test target)

**Estimated Duration:** 3 weeks
