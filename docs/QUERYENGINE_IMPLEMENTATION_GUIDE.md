# QueryEngine Implementation Migration Guide

## Overview

This guide provides detailed instructions for migrating QueryEngine method implementations from `std::pair<Status, T>` to `Result<T>`.

## Implementation Pattern

### Step-by-Step Migration

#### 1. Update Method Signature

**Before:**
```cpp
std::pair<QueryEngine::Status, std::vector<std::string>>
QueryEngine::executeAndKeys(const ConjunctiveQuery& q) const {
    // implementation
}
```

**After:**
```cpp
Result<std::vector<std::string>>
QueryEngine::executeAndKeys(const ConjunctiveQuery& q) const {
    // implementation
}
```

#### 2. Replace Error Returns

**Pattern 1: Simple Error Return**

**Before:**
```cpp
if (q.table.empty()) {
    return {Status::Error("table darf nicht leer sein"), {}};
}
```

**After:**
```cpp
if (q.table.empty()) {
    return Err<std::vector<std::string>>(
        errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
        "Table name cannot be empty"
    );
}
```

**Pattern 2: Propagate Error from Dependency**

**Before:**
```cpp
auto [st, results] = secIdx_->scanFulltext(table, column, query);
if (!st.ok) {
    return {Status::Error(st.message), {}};
}
```

**After (if SecondaryIndexManager still uses Status):**
```cpp
auto [st, results] = secIdx_->scanFulltext(table, column, query);
if (!st.ok) {
    return Err<std::vector<std::string>>(
        errors::ErrorCode::ERR_INDEX_NOT_FOUND,
        fmt::format("Fulltext scan failed: {}", st.message)
    );
}
```

**After (if SecondaryIndexManager migrated to Result):**
```cpp
auto result = secIdx_->scanFulltext(table, column, query);
if (!result) {
    return Err<std::vector<std::string>>(
        result.error().code(),
        fmt::format("Fulltext scan failed: {}", result.error().context())
    );
}
auto results = *result;
```

#### 3. Replace Success Returns

**Before:**
```cpp
return {Status::OK(), std::move(keys)};
```

**After:**
```cpp
return Ok(std::move(keys));
```

#### 4. Update Structured Bindings (Internal Calls)

**Before:**
```cpp
auto [status, keys] = executeAndKeysRangeAware_(query);
if (!status.ok) {
    return {status, {}};
}
// use keys
```

**After:**
```cpp
auto result = executeAndKeysRangeAware_(query);
if (!result) {
    return Err<T>(result.error().code(), result.error().context());
}
auto keys = *result;
// use keys
```

### Complete Example Migration

#### Before:
```cpp
std::pair<QueryEngine::Status, std::vector<std::string>>
QueryEngine::executeAndKeys(const ConjunctiveQuery& q) const {
    if (q.table.empty()) {
        return {Status::Error("executeAndKeys: table darf nicht leer sein"), {}};
    }
    
    if (!secIdx_) {
        return {Status::Error("SecondaryIndexManager nicht verfügbar"), {}};
    }
    
    // Execute query
    auto [st, results] = secIdx_->scan(q.table, q.predicates);
    if (!st.ok) {
        return {Status::Error(st.message), {}};
    }
    
    return {Status::OK(), std::move(results)};
}
```

#### After:
```cpp
Result<std::vector<std::string>>
QueryEngine::executeAndKeys(const ConjunctiveQuery& q) const {
    if (q.table.empty()) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
            "Table name cannot be empty"
        );
    }
    
    if (!secIdx_) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_INDEX_NOT_FOUND,
            "SecondaryIndexManager not initialized"
        );
    }
    
    // Execute query (assuming secIdx still uses Status)
    auto [st, results] = secIdx_->scan(q.table, q.predicates);
    if (!st.ok) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            fmt::format("Index scan failed: {}", st.message)
        );
    }
    
    return Ok(std::move(results));
}
```

## ErrorCode Selection Guide

Choose appropriate ErrorCodes based on the error type:

| Error Scenario | ErrorCode | Context Example |
|---------------|-----------|-----------------|
| Empty/invalid table name | `ERR_QUERY_INVALID_INPUT` | "Table name cannot be empty" |
| Missing index manager | `ERR_INDEX_NOT_FOUND` | "SecondaryIndexManager not initialized" |
| Missing graph index | `ERR_INDEX_NOT_FOUND` | "GraphIndexManager not available" |
| Query execution failure | `ERR_QUERY_EXECUTION_FAILED` | "Index scan failed: {reason}" |
| Query timeout | `ERR_QUERY_TIMEOUT` | "Query exceeded time limit" |
| Invalid syntax | `ERR_QUERY_INVALID_SYNTAX` | "Invalid query syntax: {details}" |
| Parse failure | `ERR_QUERY_PARSE_FAILED` | "Failed to parse query: {reason}" |

## Methods to Migrate

### High Priority (Core Query Methods)

1. ✅ `executeAndKeys()` - Already migrated in header, needs implementation
2. ✅ `executeAndEntities()` - Already migrated in header, needs implementation
3. ✅ `executeOrKeys()` - Already migrated in header, needs implementation
4. ✅ `executeOrEntities()` - Already migrated in header, needs implementation
5. ✅ `executeAndKeysWithScores()` - Already migrated in header, needs implementation

### Medium Priority (Fallback Methods)

6. ✅ `executeAndKeysWithFallback()` - Already migrated in header, needs implementation
7. ✅ `executeAndEntitiesWithFallback()` - Already migrated in header, needs implementation
8. ✅ `executeOrKeysWithFallback()` - Already migrated in header, needs implementation
9. ✅ `executeOrEntitiesWithFallback()` - Already migrated in header, needs implementation

### Medium Priority (Graph Methods)

10. ✅ `executeRecursivePathQuery()` - Already migrated in header, needs implementation
11. ✅ `executeGeneralTraversal()` - Already migrated in header, needs implementation

### Medium Priority (Hybrid Search)

12. ✅ `executeVectorGeoQuery()` - Already migrated in header, needs implementation
13. ✅ `executeContentGeoQuery()` - Already migrated in header, needs implementation
14. ✅ `executeFilteredVectorSearch()` - Already migrated in header, needs implementation
15. ✅ `executeRadiusVectorSearch()` - Already migrated in header, needs implementation
16. ✅ `executeContentSearch()` - Already migrated in header, needs implementation

### Low Priority (Private Helpers)

17. ✅ `executeAndKeysRangeAware_()` - Already migrated in header, needs implementation
18. ✅ `executeAndEntitiesRangeAware_()` - Already migrated in header, needs implementation

## Implementation Status

- **Header Signatures**: ✅ Complete (18 methods)
- **Implementation (.cpp)**: ⏳ In Progress
- **Caller Updates**: ⏳ Pending
- **Tests**: ⏳ Pending

## Challenges and Solutions

### Challenge 1: SecondaryIndexManager Still Uses Status

**Solution**: Convert Status to ErrorCode when propagating errors:

```cpp
auto [st, results] = secIdx_->scanFulltext(table, column, query);
if (!st.ok) {
    // Map Status message to appropriate ErrorCode
    return Err<T>(
        errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
        fmt::format("Fulltext scan failed: {}", st.message)
    );
}
```

### Challenge 2: Nested Calls Between QueryEngine Methods

**Solution**: Update internal calls to use Result<T>:

```cpp
// Old
auto [status, keys] = executeAndKeysRangeAware_(query);
if (!status.ok) {
    return {status, {}};
}

// New
auto result = executeAndKeysRangeAware_(query);
if (!result) {
    return Err<T>(result.error().code(), result.error().context());
}
auto keys = *result;
```

### Challenge 3: Large Methods with Multiple Error Paths

**Solution**: Migrate systematically from top to bottom, updating:
1. Method signature
2. Early validation errors
3. Dependency errors
4. Internal call errors
5. Success returns

## Testing Strategy

After migrating implementations:

1. **Unit Tests**: Verify error codes are correct
2. **Integration Tests**: Ensure callers handle Result<T> properly
3. **Error Path Tests**: Test each error condition
4. **Success Path Tests**: Verify normal operation

## Example Test

```cpp
TEST(QueryEngineTest, ExecuteAndKeysReturnsErrorForEmptyTable) {
    // Setup
    QueryEngine engine(storage, indexMgr);
    ConjunctiveQuery query;
    query.table = "";  // Invalid
    
    // Execute
    auto result = engine.executeAndKeys(query);
    
    // Verify
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
    EXPECT_THAT(result.error().message(), testing::HasSubstr("Table"));
}
```

## Migration Checklist

For each method:

- [ ] Update method signature in .cpp
- [ ] Replace `return {Status::Error(...), {}};` with `return Err<T>(...);`
- [ ] Replace `return {Status::OK(), value};` with `return Ok(value);`
- [ ] Update internal method calls from structured binding to Result checking
- [ ] Choose appropriate ErrorCodes
- [ ] Update error messages to be clear and English
- [ ] Test the migrated method
- [ ] Update callers if needed

## Next Steps

1. Migrate implementations systematically (start with core methods)
2. Update calling code to handle Result<T>
3. Add comprehensive error handling tests
4. Remove deprecated Status struct once migration complete

## Performance Notes

- `Result<T>` has zero overhead compared to `std::pair<Status, T>`
- No heap allocations for errors (stored inline)
- No exceptions thrown
- Move semantics preserved

## References

- Error codes: `include/utils/error_registry.h`
- Result type: `include/utils/expected.h`
- Migration guide: `docs/ERROR_HANDLING_MIGRATION.md`
