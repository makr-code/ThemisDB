# Phase 2C Implementation: Iterator Invalidation Fixes (3 gaps)

**Phase:** 2C (Weeks 2-3, Aug 22 – Sep 5, parallel to Phase 2A/2B)  
**Agent Type:** `themisdb-implementer` (coding + build/test)  
**Scope:** 3 CRITICAL iterator_invalidation gaps  
**Blocker:** Depends on Phase 2A CRITICAL completion (gate: P2A tests 100% PASS before P2C launch)  
**Target Artifact:** `IMPORTERS_PHASE2C_ITERATOR_INVALIDATION_FIXES_COMPLETE.md`

---

## Iterator Invalidation Problem

**Pattern:** Container modified during iteration (erase, insert) invalidates iterators; leads to crashes or undefined behavior.

**Files & Gaps:**

### 1. mdm_engine.cpp (1 CRITICAL iterator_invalidation gap)
**Problem:** Entity map iteration during update

**Gap Line:** 2567 — Loop over entity_map_ and erase entries

```cpp
// BEFORE (WRONG):
void MasterDataEngine::UpdateEntityStatus(const std::vector<std::string>& entity_ids) {
    for (auto it = entity_map_.begin(); it != entity_map_.end(); ++it) {
        if (ShouldUpdateStatus(*it)) {
            entity_map_.erase(it);  // ITERATOR INVALIDATED!
            // Next iteration: undefined behavior
        }
    }
}

// AFTER (CORRECT):
void MasterDataEngine::UpdateEntityStatus(const std::vector<std::string>& entity_ids) {
    auto it = entity_map_.begin();
    while (it != entity_map_.end()) {
        if (ShouldUpdateStatus(*it)) {
            it = entity_map_.erase(it);  // erase() returns iterator to next element
        } else {
            ++it;
        }
    }
}

// ALTERNATIVE (cleaner):
std::vector<std::string> to_remove;
for (const auto& [id, entity] : entity_map_) {
    if (ShouldUpdateStatus(id, entity)) {
        to_remove.push_back(id);
    }
}
for (const auto& id : to_remove) {
    entity_map_.erase(id);
}
```

**Impact:** Crash during entity status updates; can cause data loss.

**Tests to Add:** IMPI-2C-MD-01 (concurrent entity updates, verify no crash, correct final state)

---

### 2. deterministic_matcher.cpp (1 CRITICAL iterator_invalidation gap)
**Problem:** Match set modification during iteration

**Gap Line:** 1456 — Loop over match_candidates_ and insert/erase

```cpp
// BEFORE (WRONG):
void DeterministicMatcher::PruneWeakMatches(double threshold) {
    for (auto match : match_candidates_) {  // Range-based for (copy each element!)
        if (match.confidence < threshold) {
            match_candidates_.erase(match);  // ITERATOR INVALIDATED!
        }
    }
}

// AFTER (CORRECT):
void DeterministicMatcher::PruneWeakMatches(double threshold) {
    auto it = match_candidates_.begin();
    while (it != match_candidates_.end()) {
        if (it->confidence < threshold) {
            it = match_candidates_.erase(it);  // Returns iterator to next
        } else {
            ++it;
        }
    }
}

// ALTERNATIVE (cleaner):
std::vector<Match> keep;
for (const auto& match : match_candidates_) {
    if (match.confidence >= threshold) {
        keep.push_back(match);
    }
}
match_candidates_ = std::move(keep);
```

**Impact:** Weak matches not properly pruned; incorrect matching results.

**Tests to Add:** IMPI-2C-DM-01 (prune threshold, verify correct matches remain)

---

### 3. data_quality.cpp (1 CRITICAL iterator_invalidation gap)
**Problem:** Quality metrics map iteration during cleanup

**Gap Line:** 3245 — Loop over quality_metrics_ and erase stale entries

```cpp
// BEFORE (WRONG):
void DataQualityChecker::CleanupStaleMetrics(time_t cutoff) {
    for (auto& [field, metrics] : quality_metrics_) {  // Structured binding
        if (metrics.last_update < cutoff) {
            quality_metrics_.erase(field);  // ITERATOR INVALIDATED!
        }
    }
}

// AFTER (CORRECT):
void DataQualityChecker::CleanupStaleMetrics(time_t cutoff) {
    auto it = quality_metrics_.begin();
    while (it != quality_metrics_.end()) {
        if (it->second.last_update < cutoff) {
            it = quality_metrics_.erase(it);  // erase returns next iterator
        } else {
            ++it;
        }
    }
}

// ALTERNATIVE (cleaner):
std::vector<std::string> to_remove;
for (const auto& [field, metrics] : quality_metrics_) {
    if (metrics.last_update < cutoff) {
        to_remove.push_back(field);
    }
}
for (const auto& field : to_remove) {
    quality_metrics_.erase(field);
}
```

**Impact:** Stale metrics not cleaned; memory accumulates.

**Tests to Add:** IMPI-2C-DQ-01 (cleanup with various cutoffs, verify only stale removed)

---

## Implementation Strategy

### Week 2 (Day 1-3): Iterator Invalidation Fixes

**Priority Order (Single-File Fixes):**

1. **mdm_engine.cpp (Day 1, 1 gap)**
   - Find loop at line 2567
   - Apply erase-during-iteration pattern: `it = map.erase(it)`
   - OR apply two-pass pattern (collect + erase)
   - Verify UpdateEntityStatus behavior correct
   - Tests: IMPI-2C-MD-01

2. **deterministic_matcher.cpp (Day 2, 1 gap)**
   - Find loop at line 1456
   - Fix range-based for → while loop with erase handling
   - Verify PruneWeakMatches logic
   - Tests: IMPI-2C-DM-01

3. **data_quality.cpp (Day 3, 1 gap)**
   - Find loop at line 3245
   - Fix structured binding loop → while loop with erase handling
   - Verify CleanupStaleMetrics behavior
   - Tests: IMPI-2C-DQ-01

### Build & Test Cycle

**Per File:**
1. Locate iterator invalidation loop
2. Choose pattern:
   - Pattern A: `it = container.erase(it)` (erase returns next iterator)
   - Pattern B: Two-pass (collect IDs to remove, then erase separately)
3. Verify no iterator dereference after erase
4. Add test case (100+ iterations, multiple removals)
5. Build + test

**Verification Commands:**
```bash
cmake --build --preset community-release-allow-missing-rocksdb --target module_importers_mdm_engine_focused
ctest --preset community-release-allow-missing-rocksdb -R "importers_mdm_engine_focused" --output-on-failure

# Under ASAN/UBSan to catch any remaining invalidation:
ASAN_OPTIONS=detect_container_overflow=1 ctest -R "importers_mdm_engine_focused"
```

---

## Test Coverage (3 focused tests: IMPI-2C-*)

| File | Gap Count | Test Cases | Scenario |
|------|-----------|-----------|----------|
| mdm_engine | 1 | IMPI-2C-MD-01 | Entity status update with concurrent removes |
| deterministic_matcher | 1 | IMPI-2C-DM-01 | Prune weak matches below threshold |
| data_quality | 1 | IMPI-2C-DQ-01 | Cleanup stale metrics by timestamp |

**Test Requirements:**
- Each test: 100+ iterations of modify-during-iteration scenario
- Verification: No crash, correct final container state
- UBSan/ASAN: Detects 0 container overflow/use-after-free

---

## Acceptance Criteria (Phase 2C Exit Gate)

✅ **All 3 iterator_invalidation CRITICAL gaps fixed:**
- [ ] mdm_engine.cpp: 1/1 gap (erase-during-iteration pattern)
- [ ] deterministic_matcher.cpp: 1/1 gap (while loop + erase)
- [ ] data_quality.cpp: 1/1 gap (two-pass or while loop)

✅ **Testing & Verification:**
- [ ] All 3 focused tests (IMPI-2C-*) PASS
- [ ] UBSan/ASAN detect 0 container overflow or use-after-free
- [ ] Container final state correct (all expected elements present)

✅ **Code Quality:**
- [ ] No range-based for loops modifying container
- [ ] Proper iterator handling: `it = container.erase(it)` pattern OR two-pass
- [ ] Comments explain iterator safety fix

✅ **Git Commit:**
- Message: `IMPORTERS-P2C-ITERATOR-INVALIDATION: Fix 3 CRITICAL iterator_invalidation gaps`
- All 3 files modified in single commit
- 3 focused tests added

---

## Blockers & Risks

| Risk | Mitigation |
|------|-----------|
| Phase 2A not complete | WAIT: Phase 2A exit gate must pass before launching Phase 2C |
| Range-based for with structured binding | Replace with explicit while loop + iterator/pair access |
| Multiple removals per iteration | Use Pattern B (two-pass) if logic is complex |
| Different container types | std::map/unordered_map: erase() returns iterator; std::vector: erase() returns iterator |

---

## Iterator Invalidation Patterns (Reference)

### Pattern A: Erase During Forward Iteration
```cpp
for (auto it = container.begin(); it != container.end(); ) {
    if (condition(*it)) {
        it = container.erase(it);  // erase() returns iterator to next element
    } else {
        ++it;  // Manually increment if not erasing
    }
}
```

### Pattern B: Two-Pass (Collect + Remove)
```cpp
std::vector<Key> keys_to_remove;
for (const auto& [key, value] : container) {
    if (condition(key, value)) {
        keys_to_remove.push_back(key);
    }
}
for (const auto& key : keys_to_remove) {
    container.erase(key);
}
```

### Avoid: Range-Based For with Modifications
```cpp
// WRONG - iterator invalidated:
for (auto& [key, value] : container) {
    if (should_remove) {
        container.erase(key);  // CRASH!
    }
}
```

---

## Related Documentation

- Phase 1 Triage: `ai_working/IMPORTERS_PHASE1_GAP_TRIAGE.md` (Iterator Invalidation section)
- Master Coordination: `ai_working/IMPORTERS_GAP_CLOSURE_COORDINATION.md`
- C++ Best Practices: `.github/instructions/cpp-best-practices.instructions.md` (STL best practices)

---

## Success Indicators

✅ **Phase 2C is successful when:**
- All 3 iterator_invalidation gaps fixed with proper erase pattern
- 100% of tests (IMPI-2C-01..03) PASS
- UBSan/ASAN detects 0 issues
- Code review approved (iterator patterns validated)
- All Phase 2 (A+B+C) exit gates met → Ready for Phase 3 dispatch
