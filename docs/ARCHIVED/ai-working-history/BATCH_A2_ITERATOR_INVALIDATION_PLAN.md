# Batch A-2 Remediation Plan: iterator_invalidation

**Date:** 2026-08-15  
**Status:** 🟡 IN PROGRESS  
**Target Files:** 6-8 files with iterator invalidation risks  

## Identified Gap Locations

### Primary Hot Spots (from MODULE_GAPS.md top-20)
1. `src/index/vector_index.cpp:80` - CRITICAL
2. `src/index/multi_vector_search.cpp:224` - CRITICAL  
3. `src/index/gpu_memory_oversubscription.cpp:230` - CRITICAL
4. `src/index/graph_index.cpp:244, 247, 248` - CRITICAL (3x)
5. `src/index/edge_types.cpp:364` - CRITICAL
6. `src/index/multi_vector_search.cpp:406` - CRITICAL

## Common Iterator Invalidation Patterns

### Pattern 1: Vector Resize During Iteration
```cpp
// ❌ UNSAFE
std::vector<int> vec = {1, 2, 3};
for (auto it = vec.begin(); it != vec.end(); ++it) {
    vec.push_back(*it * 2);  // INVALIDATES ITERATORS!
}

// ✅ SAFE - Use index-based access
std::vector<int> vec = {1, 2, 3};
size_t original_size = vec.size();
for (size_t i = 0; i < original_size; ++i) {
    vec.push_back(vec[i] * 2);  // No iterator invalidation
}
```

### Pattern 2: Erase During Iteration
```cpp
// ❌ UNSAFE
std::vector<int> vec = {1, 2, 3, 4, 5};
for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (*it % 2 == 0) {
        vec.erase(it);  // INVALIDATES ITERATOR!
    }
}

// ✅ SAFE - Collect indices first
std::vector<int> vec = {1, 2, 3, 4, 5};
std::vector<size_t> indices_to_erase;
for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i] % 2 == 0) {
        indices_to_erase.push_back(i);
    }
}
// Erase in reverse order to maintain indices
for (auto it = indices_to_erase.rbegin(); it != indices_to_erase.rend(); ++it) {
    vec.erase(vec.begin() + *it);
}

// ✅ ALTERNATIVE - Use erase-remove idiom
vec.erase(std::remove_if(vec.begin(), vec.end(), 
          [](int x) { return x % 2 == 0; }), vec.end());
```

### Pattern 3: Map Iterator Invalidation
```cpp
// ❌ UNSAFE - Only erase invalidates
std::map<int, int> m = {{1, 10}, {2, 20}};
for (auto it = m.begin(); it != m.end(); ++it) {
    if (it->second > 15) {
        m.erase(it);  // INVALIDATES IT and all iterators to erased element
    }
}

// ✅ SAFE - Post-increment pattern
std::map<int, int> m = {{1, 10}, {2, 20}};
for (auto it = m.begin(); it != m.end(); ) {
    if (it->second > 15) {
        it = m.erase(it);  // erase returns next iterator
    } else {
        ++it;
    }
}
```

## Files Flagged for Review

### 1. vector_index.cpp:80
**Issue:** Iterator/index-based access patterns in ID mapping  
**Solution:** Already using index-based access; ensure no concurrent modification

### 2. multi_vector_search.cpp:224, 406
**Issue:** Vector collection and score fusion operations  
**Solution:** Review for concurrent mutations; cache sizes early

### 3. gpu_memory_oversubscription.cpp:230
**Issue:** LRU list management with iterator storage in map  
**Pattern:** `lru_map[key] = lru_list.iterator`  
**Solution:** Ensure thread-safe updates; validate iterator validity

### 4. graph_index.cpp:244, 247, 248
**Issue:** Multiple operations in edge/node processing  
**Solution:** Review container mutations in graph operations

### 5. edge_types.cpp:364
**Issue:** Type registry lookups  
**Solution:** Ensure registry is not modified during iteration

## Remediation Strategy

### Immediate Actions (Required for this batch)
1. **Cache sizes before loops** - Capture container sizes before iteration
   ```cpp
   size_t original_size = container.size();
   for (size_t i = 0; i < original_size; ++i) {
       // Safe to add/remove beyond original_size
   }
   ```

2. **Use index-based access** - Replace iterator loops with index loops where possible
   ```cpp
   // Before: for (auto it = ...; it != ...; ++it)
   // After:  for (size_t i = 0; i < size; ++i)
   ```

3. **Defer mutations** - Collect operations, then apply them
   ```cpp
   std::vector<size_t> indices_to_remove;
   for (size_t i = 0; i < vec.size(); ++i) {
       if (should_remove(vec[i])) {
           indices_to_remove.push_back(i);
       }
   }
   // Apply removals
   ```

4. **Thread-safe access** - Add mutexes for concurrent access paths
   ```cpp
   std::lock_guard<std::mutex> lock(container_mutex);
   for (...) { /* iteration */ }
   ```

## Validation

### ASan/MSan Checks
```bash
ctest -L index --output-on-failure -j 1 -E "perf"
```

### Manual Code Review Checklist
- [ ] No iterator stored across container mutations
- [ ] Index-based loops used for potentially modified containers
- [ ] Size captured before loop for growing containers
- [ ] Erase operations use post-increment pattern for maps/sets

## Status Tracking
- [ ] vector_index.cpp:80 - Review & Fix
- [ ] multi_vector_search.cpp:224 - Review & Fix
- [ ] multi_vector_search.cpp:406 - Review & Fix
- [ ] gpu_memory_oversubscription.cpp:230 - Review & Fix
- [ ] graph_index.cpp:244 - Review & Fix
- [ ] graph_index.cpp:247 - Review & Fix
- [ ] graph_index.cpp:248 - Review & Fix
- [ ] edge_types.cpp:364 - Review & Fix

---
