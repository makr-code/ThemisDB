# Next Steps - Error Handling Migration Phase 3

## Current Status

### Completed (Phase 1 & 2)
- ✅ IStorageEngine interface migration (4 methods)
- ✅ StorageEngine implementation (4 methods)
- ✅ IQueryEngine interface migration (4 methods)
- ✅ QueryEngine method signatures (18 methods)
- ✅ Comprehensive documentation (4 guides)
- ✅ Test coverage (7 tests)

**Total: 54 methods using Result<T>**

## Phase 3: Implementation Migration

### Objective
Update QueryEngine `.cpp` implementations to match the new `Result<T>` signatures.

### Scope
- **File**: `src/query/query_engine.cpp` (175KB, 15 method implementations)
- **Methods**: 18 QueryEngine methods need implementation updates
- **Pattern**: `std::pair<Status, T>` → `Result<T>`

### Implementation Strategy

#### Approach: Incremental Migration
Given the size and complexity, we'll use an incremental approach:

1. **Phase 3a**: Core query methods (5 methods)
   - `executeAndKeys()`
   - `executeAndEntities()`
   - `executeOrKeys()`
   - `executeOrEntities()`
   - `executeAndKeysWithScores()`

2. **Phase 3b**: Fallback methods (4 methods)
   - `executeAndKeysWithFallback()`
   - `executeAndEntitiesWithFallback()`
   - `executeOrKeysWithFallback()`
   - `executeOrEntitiesWithFallback()`

3. **Phase 3c**: Graph methods (2 methods)
   - `executeRecursivePathQuery()`
   - `executeGeneralTraversal()`

4. **Phase 3d**: Hybrid search (5 methods)
   - `executeVectorGeoQuery()`
   - `executeContentGeoQuery()`
   - `executeFilteredVectorSearch()`
   - `executeRadiusVectorSearch()`
   - `executeContentSearch()`

5. **Phase 3e**: Private helpers (2 methods)
   - `executeAndKeysRangeAware_()`
   - `executeAndEntitiesRangeAware_()`

### Key Challenges

#### 1. Dependency on SecondaryIndexManager Status
SecondaryIndexManager still returns `std::pair<Status, T>`, not `Result<T>`.

**Solution**: Convert Status errors to ErrorCodes when propagating:
```cpp
auto [st, results] = secIdx_->scan(...);
if (!st.ok) {
    return Err<T>(ErrorCode::ERR_QUERY_EXECUTION_FAILED, st.message);
}
```

#### 2. Internal Method Calls
QueryEngine methods call each other (e.g., `executeAndKeys` calls `executeAndKeysRangeAware_`).

**Solution**: Update structured bindings to Result checking:
```cpp
// Old: auto [status, keys] = executeAndKeysRangeAware_(q);
// New: auto result = executeAndKeysRangeAware_(q);
```

#### 3. Complex Error Paths
Methods like `executeRecursivePathQuery` have 10+ error return points.

**Solution**: Migrate systematically, one error path at a time.

### Migration Pattern

See `docs/QUERYENGINE_IMPLEMENTATION_GUIDE.md` for detailed patterns.

**Quick Reference**:
```cpp
// Error return
return {Status::Error("msg"), {}};
→ return Err<T>(ErrorCode::ERR_QUERY_..., "msg");

// Success return
return {Status::OK(), value};
→ return Ok(value);

// Structured binding
auto [st, result] = method();
if (!st.ok) return {st, {}};
→ auto result = method();
if (!result) return Err<T>(result.error().code(), ...);
```

### ErrorCode Mapping

| Scenario | ErrorCode |
|----------|-----------|
| Empty table name | `ERR_QUERY_INVALID_INPUT` |
| Missing index manager | `ERR_INDEX_NOT_FOUND` |
| Query execution failure | `ERR_QUERY_EXECUTION_FAILED` |
| Query timeout | `ERR_QUERY_TIMEOUT` |
| Invalid syntax | `ERR_QUERY_INVALID_SYNTAX` |

## Phase 4: Update Callers

### Scope
Update code that calls QueryEngine methods to use `Result<T>` instead of `std::pair<Status, T>`.

### Files to Update
- AQL Translator (`src/query/aql_translator.cpp`)
- Query handlers in server
- API endpoints
- Tests

### Pattern
```cpp
// Old
auto [status, results] = engine.executeAndKeys(query);
if (!status.ok) {
    handleError(status.message);
    return;
}

// New
auto result = engine.executeAndKeys(query);
if (!result) {
    handleError(result.error().message());
    return;
}
auto results = *result;
```

## Phase 5: Testing

### Test Strategy
1. **Unit Tests**: Verify each migrated method
2. **Error Path Tests**: Test each ErrorCode scenario
3. **Integration Tests**: End-to-end query execution
4. **Regression Tests**: Ensure no behavioral changes

### Test Coverage Goals
- All error paths tested
- All success paths tested
- ErrorCode validation
- Error message quality

## Timeline Estimate

| Phase | Effort | Risk |
|-------|--------|------|
| 3a: Core methods | Medium | Low |
| 3b: Fallback methods | Medium | Low |
| 3c: Graph methods | High | Medium |
| 3d: Hybrid search | High | Medium |
| 3e: Private helpers | Low | Low |
| Phase 4: Callers | High | Medium |
| Phase 5: Testing | Medium | Low |

## Risk Mitigation

### Risks
1. **Breaking changes**: Callers need updates
2. **Complex dependencies**: SecondaryIndexManager uses Status
3. **Large codebase**: Many call sites to update

### Mitigations
1. ✅ Keep deprecated Status temporarily for transition
2. ✅ Provide helper converters (fromBoolStatus, etc.)
3. ✅ Comprehensive documentation and examples
4. ✅ Incremental migration with testing at each step

## Success Criteria

- [ ] All 18 QueryEngine implementations migrated
- [ ] All callers updated to use Result<T>
- [ ] All tests passing
- [ ] No deprecation warnings
- [ ] Code review approved
- [ ] Documentation updated

## Documentation

### Created
1. ✅ `ERROR_HANDLING_MIGRATION.md` - General migration guide
2. ✅ `ERROR_HANDLING_MIGRATION_SUMMARY.md` - Status report
3. ✅ `QUERYENGINE_MIGRATION_PROGRESS.md` - Phase 2 tracking
4. ✅ `QUERYENGINE_IMPLEMENTATION_GUIDE.md` - Implementation patterns
5. ✅ `NEXT_STEPS.md` - This document

### To Update
- [ ] README.md (if needed)
- [ ] CHANGELOG.md
- [ ] Migration summary with Phase 3 status

## Immediate Next Actions

1. **Start Phase 3a**: Migrate core query methods
   - Begin with `executeAndKeys()` as reference implementation
   - Test thoroughly
   - Use as template for remaining methods

2. **Validate approach**: 
   - Ensure pattern works with existing callers
   - Verify ErrorCode selection is appropriate
   - Confirm no performance regression

3. **Continue incrementally**:
   - One phase at a time
   - Test after each method migration
   - Update documentation as we go

## Notes

- **Minimal Changes**: We're updating implementations to match already-changed signatures
- **Backward Compatibility**: Status struct marked deprecated but still available
- **Zero Overhead**: Result<T> has same performance as std::pair<Status, T>
- **Type Safety**: Compiler now enforces error checking

## References

- Implementation guide: `docs/QUERYENGINE_IMPLEMENTATION_GUIDE.md`
- ErrorCode registry: `include/utils/error_registry.h`
- Result<T> type: `include/utils/expected.h`
