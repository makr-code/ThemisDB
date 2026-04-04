# Query Engine Result<> Migration Plan

## Overview
This document outlines the plan to migrate the Query Engine API from `std::pair<Status, T>` pattern to `Result<T>` pattern and update all affected test files.

## Current State
- Query Engine uses `std::pair<QueryEngine::Status, T>` for all query methods
- Status struct has `.ok` boolean and `.message` string
- ~11 test files use Status-based assertions
- ~20+ Query Engine methods need migration

## Migration Strategy

### Phase 1: Add Result<> Variants (Dual API Approach)
Add new Result<>-based methods alongside existing Status-based methods to maintain backward compatibility:

**Pattern:**
```cpp
// OLD (keep for backward compatibility)
std::pair<Status, std::vector<T>> executeAndKeys(const Query& q) const;

// NEW (add Result<> variant)
Result<std::vector<T>> executeAndKeysResult(const Query& q) const;
```

**Benefits:**
- No breaking changes
- Gradual migration possible
- Tests can be updated incrementally
- Fallback if issues arise

### Phase 2: Update Test Files
Update test files one by one to use new Result<> methods:

1. test_query_engine.cpp (136 lines)
2. test_query_engine_join.cpp (140 lines)
3. test_query_engine_range.cpp (92 lines)
4. test_query_or.cpp (221 lines)
5. test_recursive_ctes.cpp (355 lines)
6. test_cte_cache.cpp (304 lines)
7. test_timerange_query.cpp (6908 bytes)
8. test_recursive_path_query.cpp (6469 bytes)
9. test_http_query_range.cpp (8875 bytes)
10. test_query_optimizer_vector_geo.cpp (2108 bytes)
11. test_query_engine_di.cpp (already uses interfaces)

### Phase 3: Deprecate Old API (Future)
After all tests pass and migration is validated:
- Mark old methods as [[deprecated]]
- Provide clear migration path
- Eventually remove in future version

## Implementation Steps

### Step 1: Add include for Result<>
```cpp
#include "utils/expected.h"  // For Result<T>
```

### Step 2: Implement Result<> variants
For each method:
```cpp
Result<std::vector<std::string>> executeAndKeysResult(const ConjunctiveQuery& q) const {
    auto [st, keys] = executeAndKeys(q);  // Call existing method
    if (!st.ok) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            st.message
        );
    }
    return Ok(std::move(keys));
}
```

### Step 3: Update test pattern
```cpp
// OLD
auto [st, keys] = engine.executeAndKeys(q);
ASSERT_TRUE(st.ok) << st.message;
ASSERT_EQ(keys.size(), 1u);

// NEW
auto result = engine.executeAndKeysResult(q);
ASSERT_TRUE(result.has_value()) << result.error().message();
ASSERT_EQ(result->size(), 1u);
```

## Error Code Mapping

| Scenario | ErrorCode |
|----------|-----------|
| Index not found | ERR_INDEX_NOT_FOUND |
| Query parse failed | ERR_QUERY_PARSE_FAILED |
| Execution failed | ERR_QUERY_EXECUTION_FAILED |
| Timeout | ERR_QUERY_TIMEOUT |
| Resource exhaustion | ERR_RESOURCE_EXHAUSTED |

## Testing Strategy

1. Add Result<> methods WITHOUT removing old ones
2. Update one test file at a time
3. Run tests after each file update
4. Validate no regressions
5. Keep old API until all tests pass

## Rollback Plan

If issues arise:
- Old API remains functional
- Tests can use either pattern
- Git revert to any point
- No production impact

## Timeline

- Step 1-2: Add Result<> variants (~2-3 hours)
- Step 3: Update test files (~4-6 hours)
- Validation & fixes (~2-3 hours)
- **Total**: ~1 day of focused work

## Success Criteria

✅ All Query Engine methods have Result<> variants  
✅ All 11 test files updated to use Result<>  
✅ All tests pass  
✅ No breaking changes to existing code  
✅ Clear migration path documented  
✅ Performance maintained (<5% regression)

## Notes

- This approach ensures ZERO breaking changes
- Old API can coexist with new API
- Migration is gradual and safe
- Tests validate both correctness and performance

---

**Status**: PLANNED  
**Approach**: Dual API (backward compatible)  
**Risk**: LOW (old API remains functional)
