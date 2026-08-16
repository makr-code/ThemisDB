# Agent 1: Index Module Phase 2 A-2 (Iterator Invalidation)

**Duration:** 1.5 hours | **Scope:** 8 gaps | **Target:** ASan/UBSan clean

## Gap Details

### Iterator Invalidation Pattern
- **Root Cause:** Partition removal invalidates iterators held by partition manager or query executor
- **Risk:** Use-after-free, dangling iterator, crash on dereference
- **Fix Pattern:** Use stable partition ID mapping, rebuild iterators post-removal, or use stable_partition pattern

### Affected Gap Categories (from gap_scan_index.json)
1. `partition_manager_iterator_invalidation_01..04` (4 gaps)
   - File: `src/index/partition_manager.cpp`
   - Method: `RemovePartition()`, `RebuildPartitions()`, `CompactPartitions()`
   - Fix: Add `iterator_epoch` counter, invalidate iterators post-removal

2. `vector_index_iterator_invalidation_01..04` (4 gaps)
   - File: `src/index/vector_index_manager.cpp`
   - Method: `RemoveVectorIndex()`, `UpdateVectorIndex()`, iterator cleanup
   - Fix: Use stable handle-based access instead of raw iterators

## Implementation Tasks

### Task 1: Design Fix Patterns (15 min)

**Pattern 1: Stable Handle-Based Iterators**
```cpp
// Instead of raw iterator:
class PartitionHandle {
  uint64_t partition_id_;
  uint64_t epoch_;  // Version counter for invalidation
  
  bool isValid() const { return epoch_ == manager_->currentEpoch(partition_id_); }
};

// In PartitionManager:
void RemovePartition(uint32_t id) {
  partitions_.erase(id);
  partition_epoch_[id]++;  // Invalidate all handles
}
```

**Pattern 2: Iterator Rebuild After Removal**
```cpp
// In VectorIndexManager::UpdateVectorIndex():
auto iter = indices_.find(index_id);
if (iter == indices_.end()) throw std::runtime_error("index not found");

// Work with iterator, but verify it's still valid
std::vector<IndexEntry> snapshot(iter->second.begin(), iter->second.end());
// Rebuild after mutation
for (const auto& entry : snapshot) {
  // Re-find by ID, not by iterator
  auto new_iter = indices_.find(entry.id);
  if (new_iter != indices_.end()) {
    // Process
  }
}
```

### Task 2: Implement Fixes (30 min)

**Files to edit:**
- `src/index/partition_manager.cpp`
- `src/index/vector_index_manager.cpp`
- `include/index/partition_manager.h`

**Gap A-2-01 to A-2-08 Implementation:**

1. Add epoch counter to PartitionManager
2. Add handle-based access pattern in VectorIndexManager
3. Add bounds checking before iterator dereference
4. Remove raw iterator use in iterator-invalidation-risk contexts
5. Add RAII guards for partition lifetime management
6. Document iterator lifetime in API comments
7. Add null/validity checks in destruction paths
8. Add fallback to stable ID-based lookup

### Task 3: Write Tests (20 min)

**File:** `tests/index/test_index_phase2_a2_iterator_safety.cpp`

```cpp
// Test: partition removal invalidates iterators
TEST(IndexPhase2A2, PartitionRemovalInvalidatesHandles) {
  PartitionManager pm;
  auto h1 = pm.AddPartition("p1");
  EXPECT_TRUE(h1.isValid());
  
  pm.RemovePartition(h1.id());
  EXPECT_FALSE(h1.isValid());
}

// Test: vector index iterator safety
TEST(IndexPhase2A2, VectorIndexIteratorRebuild) {
  VectorIndexManager vim;
  auto idx1 = vim.CreateIndex("idx1");
  auto iter = vim.GetPartitions(idx1);  // Get stable handles
  
  vim.UpdateVectorIndex(idx1);  // May invalidate
  
  // Verify we can still access by ID
  auto new_iter = vim.GetPartitions(idx1);
  EXPECT_EQ(new_iter.size(), iter.size());
}
```

8 focused test cases covering:
- Handle validity after removal
- Stable ID-based rebuild
- Concurrent iterator access
- Destructor safety during partition removal
- Exception safety in iterator invalidation paths
- RAII guard correctness
- Fallback access patterns
- Edge cases (empty partitions, rapid add/remove)

### Task 4: Validation (15 min)

**Local validation:**
```bash
cmake --preset linux-debug -DSANITIZER=asan
cmake --build --preset linux-debug-build
ctest --preset linux-debug -R "test_index_phase2_a2" -V
```

**CI validation:**
- ASan: 0 alerts
- UBSan: 0 alerts
- Tests: 8/8 passing

### Task 5: Commit

**Message:**
```
PHASE2: Index A-2 Iterator Invalidation (8 gaps) — Handle-based access + epoch validation

- Add PartitionHandle with epoch-based validity tracking
- Implement stable ID-based lookup fallback in VectorIndexManager
- Add RAII guards for partition lifetime management
- Remove unsafe raw iterator dereference patterns
- Add 8 focused iterator safety tests
- ASan/UBSan: 0 alerts, 8/8 tests passing
```

## Exit Criteria

- [x] All 8 gaps addressed with production logic (no stubs)
- [x] 8 focused test cases, 100% passing
- [x] ASan/UBSan output: 0 new alerts
- [x] Doxygen-compliant API comments
- [x] No build regressions

## Success Timeline

- 0:00-0:15: Pattern design
- 0:15-0:45: Implementation (8 fixes)
- 0:45-1:05: Tests (8 cases)
- 1:05-1:20: Validation
- 1:20-1:30: Commit + final checks

**Target completion:** 1.5 hours ✅
