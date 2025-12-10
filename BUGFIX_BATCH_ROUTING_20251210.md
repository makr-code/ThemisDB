# Bug Fix: /entities/batch Routing Issue

## Date
2025-12-10

## Problem
The `/entities/batch` endpoint returned 404 errors despite being implemented in the code.

## Root Cause
In `src/server/http_server.cpp`, the route classification logic had a critical ordering bug:

```cpp
// BEFORE (BROKEN):
// Parametrized entity by key
if (target.rfind("/entities/", 0) == 0) {  // ← Matches "/entities/batch"!
    if (method == http::verb::get) return Route::EntitiesGet;
    if (method == http::verb::put) return Route::EntitiesPut;
    if (method == http::verb::delete_) return Route::EntitiesDelete;
    return Route::NotFound;  // ← POST /entities/batch falls through here!
}

if (target == "/entities" && method == http::verb::post) return Route::EntitiesPost;
if (target == "/entities/batch" && method == http::verb::post) return Route::EntitiesBatchPost;  // ← Never reached!
```

The prefix matcher `/entities/` caught `/entities/batch` before the exact route check, causing all batch requests to return 404.

## Solution
Move exact-match routes BEFORE prefix matching:

```cpp
// AFTER (FIXED):
// Exact matches FIRST
if (target == "/entities" && method == http::verb::post) return Route::EntitiesPost;
if (target == "/entities/batch" && method == http::verb::post) return Route::EntitiesBatchPost;

// THEN parametrized routes
if (target.rfind("/entities/", 0) == 0) {
    if (method == http::verb::get) return Route::EntitiesGet;
    if (method == http::verb::put) return Route::EntitiesPut;
    if (method == http::verb::delete_) return Route::EntitiesDelete;
    return Route::NotFound;
}
```

## Impact
**Before Fix:**
- Bulk operations: 100% failure rate (0/10,000 success)
- Stress test: 0% success (0/100,000 operations)
- Throughput: 0 ops/s for batch operations

**After Fix (expected):**
- Bulk operations should work correctly
- Stress test should complete successfully
- Throughput should increase 10-100x

## Testing
1. Test batch endpoint availability:
   ```bash
   curl -X POST http://localhost:8765/entities/batch \
     -H "Content-Type: application/json" \
     -d '{"operations":[{"op":"put","key":"test:1","blob":"{}"}]}'
   ```

2. Run CRUD benchmarks:
   ```bash
   python benchmarks/comprehensive_crud_benchmark.py --host localhost --port 8765
   ```

## Files Changed
- `src/server/http_server.cpp` (lines 1000-1016)

## Related Issues
- CRUD Benchmark failures reported in `CRUD_BENCHMARK_ANALYSIS_20251210.md`
- Bulk insert 0% success rate
- Stress test 100% failure rate
