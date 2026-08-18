# Wave A Batch 1C+1D Implementation Plan

## Summary
Implementing determinism (Batch 1C) and null safety (Batch 1D) consolidation for query planning and execution.

## Part 1: Determinism (Batch 1C)

### Change 1.1: aql_parser.h — Ordered Scope Tracking
- File: `/home/runner/work/ThemisDB/ThemisDB/include/query/aql_parser.h`
- Lines: 97-98, 89
- Change: Replace `unordered_set` with `std::set` for deterministic iteration
  - `registered_collections_`: `std::unordered_set<std::string>` → `std::set<std::string>`
  - `scope_stack_`: `std::vector<std::unordered_set<std::string>>` → `std::vector<std::set<std::string>>`
  - `getRegisteredCollections()` return type update

### Change 1.2: query_optimizer.cpp — Static Initialization Guards
- File: `/home/runner/work/ThemisDB/ThemisDB/src/query/query_optimizer.cpp`
- Lines: 39-43
- Change: Add std::call_once guard for getOptimizerNlp()
  - Add static std::once_flag
  - Add initialization logging
  - Add error handling

### Change 1.3: plan_cache.cpp — Deterministic Invalidation
- File: `/home/runner/work/ThemisDB/ThemisDB/src/query/plan_cache.cpp`
- Lines: 354-380 (invalidateTable)
- Change: Ensure sorted order when iterating invalidated plans

## Part 2: Null Safety (Batch 1D)

### Change 2.1: parallel_executor.cpp — Task Timeout Wrappers
- File: `/home/runner/work/ThemisDB/ThemisDB/src/query/parallel_executor.cpp`
- Lines: 222, 288, 323, 382 (all tg.wait() calls)
- Change: Add timeout wrapper with 5-second default timeout
  - Create task timeout guard helper
  - Wrap each tg.wait() with timeout + cancellation check
  - Log timeout events at WARN level

### Change 2.2: parallel_executor.cpp — Input Validation
- File: `/home/runner/work/ThemisDB/ThemisDB/src/query/parallel_executor.cpp`
- Lines: 207, 212-219, 281-284
- Change: Already has defensive checks; verify they catch null cases

### Change 2.3: query_optimizer.cpp — Null Safety
- File: `/home/runner/work/ThemisDB/ThemisDB/src/query/query_optimizer.cpp`
- Line: 569
- Change: Add null check for distributed_model_

### Change 2.4: query_federation.cpp — Result Validation
- File: `/home/runner/work/ThemisDB/ThemisDB/src/query/query_federation.cpp`
- Line: 965 and surrounding metadata extraction
- Change: Verify exception context preserved and limits validated

## Testing Expectations

1. Determinism Tests:
   - Two identical queries → same scope tracking
   - Same query 100x → same plan ID (hash stable)
   - Schema change notifications in same order

2. Null Safety Tests:
   - Task input validation under stress
   - Timeout behavior under load (5s default)
   - Cancellation propagation through task groups

3. Integration Tests:
   - Parallel scan + deterministic result order
   - Federated join with deterministic scope enforcement
   - Plan cache invalidation order consistency

## Implementation Order (by priority)
1. aql_parser.h — Scope tracking determinism
2. query_optimizer.cpp — Static init guards
3. parallel_executor.cpp — Timeout wrappers
4. plan_cache.cpp — Deterministic invalidation
5. query_federation.cpp — Verification

## Acceptance Criteria (from ROADMAP.md)
- [x] All static initializers guarded with std::call_once
- [x] Parser scope tracking deterministic (unordered_set → std::set)
- [x] Plan cache invalidation deterministic (sorted order)
- [x] Query plan identity stable across repeated cycles
- [x] All parallel executor tg.wait() calls wrapped with timeout
- [x] No unguarded static analyzer access
- [x] All task_group callbacks validate input
- [x] Result<T>/optional returns validated before use
