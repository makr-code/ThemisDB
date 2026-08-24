# Implementation Summary: Patch Ordering Enforcement (UPD-IMPL-003)

**Date:** 2026-08-18
**Priority:** CRITICAL (Wave A Blocker)
**Task Reference:** UPD-IMPL-003
**Status:** ✅ COMPLETE

## Problem Statement

Delta patches were applied in manifest order without enforcing ordering constraints. When concurrent patches affect files with dependencies:
- Patches could be applied in an undefined order
- Concurrent patches could conflict if applied to dependent files
- No mechanism existed to enforce dependency ordering

## Solution Overview

Implemented comprehensive patch ordering enforcement with:
1. **Dependency Declaration:** FileDelta extended with `depends_on` and `apply_order` fields
2. **Manifest Ordering Control:** DeltaManifest extended with `enforce_order` and `implicit_dependencies`
3. **Topological Sort:** Kahn's algorithm implementation for deterministic patch ordering
4. **Circular Dependency Detection:** DFS-based cycle detection (error code 7402)
5. **Dependency Validation:** Missing dependency detection (error code 7404)
6. **Backward Compatibility:** Default behavior unchanged when `enforce_order=false`

## Changes Made

### 1. File: `include/updates/delta_update_engine.h`

**FileDelta Structure Extensions:**
- Added `std::vector<std::string> depends_on` - paths this file depends on
- Added `uint32_t apply_order` - explicit ordering hint (0=default/no constraint)
- Updated documentation with ordering semantics

**DeltaManifest Structure Extensions:**
- Added `bool enforce_order` - flag to enable strict ordering (default: false)
- Added `std::vector<std::string> implicit_dependencies` - global dependency hints
- Updated documentation with error codes and semantics

**DeltaUpdateEngine Private Methods:**
```cpp
std::vector<FileDelta> computeApplyOrder(const DeltaManifest& manifest);
bool validateDependencies(const DeltaManifest& manifest);
bool hasCircularDependency(const std::vector<FileDelta>& deltas);
```

**Updated applyDelta Documentation:**
- Added section explaining ordering enforcement behavior
- Documented error codes 7402, 7404

### 2. File: `src/updates/delta_update_engine.cpp`

**Includes Added:**
- `<queue>` - for BFS in topological sort
- `<unordered_set>` - for O(1) dependency lookup

**JSON Serialization Updates:**
- `FileDelta::toJson()` - serialize `depends_on` and `apply_order` (omit defaults for backward compat)
- `FileDelta::fromJson()` - deserialize ordering fields with defaults
- `DeltaManifest::toJson()` - serialize `enforce_order` and `implicit_dependencies`
- `DeltaManifest::fromJson()` - deserialize ordering fields

**New Implementation: validateDependencies()**
```
Algorithm:
1. Build set of all available patch paths in manifest
2. For each patch and each dependency (explicit + implicit):
   - Check that dependency exists in available paths
   - Log error 7404 if missing
   - Return false on any missing dependency
3. Return true if all dependencies valid
```

**New Implementation: hasCircularDependency()**
```
Algorithm: Kahn's Algorithm (DFS variant)
1. Build adjacency list and in-degree map from dependencies
2. Initialize queue with nodes having in-degree 0
3. Process queue, decrementing in-degrees
4. If processed count != total nodes, cycle detected
5. Log error 7402 on cycle detection
6. Return true if cycle found, false otherwise
```

**New Implementation: computeApplyOrder()**
```
Algorithm: Kahn's Algorithm with Stable Sorting
1. Call validateDependencies() - return empty on error (7404)
2. Call hasCircularDependency() - return empty on error (7402)
3. Build adjacency list and in-degree map
4. Add both explicit and implicit dependencies to graph
5. Perform topological sort with Kahn's algorithm
6. Sort ready nodes by apply_order for determinism
7. Return ordered list of FileDelta
```

**Updated applyDelta() Method:**
```
New Logic:
1. If manifest.enforce_order is true:
   - Call computeApplyOrder()
   - If returns empty and manifest.deltas not empty:
     - Log error
     - Return result.success=false
2. Apply patches in computed order (or manifest order if !enforce_order)
3. Rest of logic unchanged
```

### 3. File: `tests/test_delta_patch_ordering.cpp` (NEW)

Created comprehensive test suite covering:

**Test 1-3: JSON Serialization**
- `JsonSerializationWithDependencies` - verify depends_on and apply_order serialized
- `JsonSerializationBackwardCompatibility` - verify old JSON deserializes correctly
- `DeltaManifestJsonOrderingFields` - verify manifest ordering fields

**Test 4-5: Dependency Validation**
- `CircularDependencyDetection` - verify circular deps detected (7402)
- `MissingDependencyDetection` - verify missing deps detected (7404)

**Test 6-7: Topological Sort**
- `TopologicalSortSimpleChain` - linear dependency chain
- `TopologicalSortDiamondDependency` - diamond dependency pattern

**Test 8-9: Advanced Features**
- `ApplyOrderHintsDeterminism` - verify apply_order controls ordering
- `ImplicitDependencies` - verify global dependencies applied

**Test 10-11: Compatibility & Round-Trip**
- `BackwardCompatibilityNoOrdering` - verify enforce_order=false ignores deps
- `JsonRoundTripComplexOrdering` - full serialization round-trip test

## Error Codes

| Code | Scenario | Location | Action |
|------|----------|----------|--------|
| 7402 | Circular dependency detected in patch ordering | hasCircularDependency() | Return empty list, fail update |
| 7404 | Dependency file missing in manifest | validateDependencies() | Return empty list, fail update |
| 7403 | Reserved for future runtime ordering violations | N/A | Not emitted in current impl |

## Backward Compatibility

✅ **Maintained:** When `enforce_order=false` (default), patches apply in manifest order
- New fields `depends_on`, `apply_order`, `enforce_order`, `implicit_dependencies` are optional
- JSON serialization omits default values to minimize manifest size
- Old manifests deserialize correctly with default values

## Key Design Decisions

1. **Topological Sort (Kahn's Algorithm):**
   - Deterministic due to stable sorting by apply_order
   - O(V+E) complexity where V=patches, E=dependencies
   - No recursion (iterative BFS) - stack-safe for large graphs

2. **Dependency Validation Timing:**
   - Performed eagerly before topological sort
   - Allows early error reporting
   - Prevents processing invalid manifests

3. **Implicit Dependencies:**
   - Applied to all patches unless already in explicit depends_on
   - Useful for common dependencies (e.g., config files)
   - De-duplicated automatically

4. **Apply Order Hints:**
   - Lower values apply first (natural ordering)
   - Only used for tie-breaking when no dependencies
   - Enables deterministic ordering of independent patches

5. **JSON Format:**
   - Conditional serialization of new fields
   - Backward compatible with older readers
   - Minimal size impact

## Testing Coverage

- [x] JSON serialization with new fields
- [x] JSON deserialization with backward compatibility
- [x] Circular dependency detection
- [x] Missing dependency detection
- [x] Topological sort correctness (chain, diamond patterns)
- [x] Apply order determinism
- [x] Implicit dependencies
- [x] Backward compatibility (enforce_order=false)
- [x] Complex round-trip serialization

## Production Requirements

✅ **Verification Checklist:**
- [x] All new methods have comprehensive documentation
- [x] Error codes in range [7400-7499] as specified
- [x] Backward compatible JSON serialization
- [x] Topological sort guaranteed to be deterministic
- [x] No circular dependency can pass validation
- [x] All dependencies are validated before application
- [x] Follows ThemisDB C++ conventions
- [x] Code follows SPDLOG error logging pattern
- [x] Private methods only (no public API changes except struct fields)

## Files Modified

1. `include/updates/delta_update_engine.h` - structures and method declarations
2. `src/updates/delta_update_engine.cpp` - implementations and JSON updates
3. `tests/test_delta_patch_ordering.cpp` - new comprehensive test suite

## Risk Assessment

**Low Risk:**
- Ordering feature is opt-in (enforce_order=false by default)
- No changes to existing patch application logic
- All new code is isolated in private methods
- Comprehensive validation prevents invalid manifests

**Mitigations:**
- Circular dependency detection before topological sort
- Early dependency validation with clear error messages
- Backward compatible JSON format
- Extensive test coverage for ordering logic

## Next Steps (Wave A Continuation)

1. **Manual Testing:** Integration tests with actual patch application
2. **Performance Benchmarking:** Measure topological sort overhead
3. **Documentation:** Add usage examples to DeveloperGuide/
4. **CI Integration:** Enable ordering validation in release pipeline
5. **Related Implementations:**
   - UPD-IMPL-004: Update coordination atomicity
   - UPD-IMPL-006: Idempotent rollback (may benefit from ordering info)

## References

- **Module Gap:** `src/updates/MODULE_GAPS_BATCH5.md § UPD-IMPL-003`
- **Architecture:** `ARCHITECTURE.md § Updates Module`
- **Wave A Roadmap:** `ROADMAP.md § Wave A CRITICAL Gaps`
- **Test Location:** `tests/test_delta_patch_ordering.cpp`
