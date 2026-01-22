---
name: Phase 3 - Migrate Query Engine to Result<T>
about: Migrate Query Engine methods from legacy error patterns to Result<T>
title: '[Phase 3] Migrate Query Engine to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'query-engine', 'high-priority']
assignees: ''
---

## 📋 Overview

Migrate Query Engine methods from legacy error patterns to `Result<T>` for better error messages and diagnostics during query execution.

**Current Status:** 0% complete  
**Target:** 50+ methods  
**Priority:** 🔴 **HIGH** (user-facing, high traffic)

## 🎯 Goals

- Provide detailed error messages for query failures
- Enable better query debugging and optimization
- Type-safe error propagation through query execution pipeline

## 🔨 Scope

### Query Planning & Optimization
- [ ] Query parser methods
- [ ] Query planner methods
- [ ] Query optimizer methods
- [ ] Cost estimator methods

### Query Execution
- [ ] Executor initialization
- [ ] Plan node execution methods
- [ ] Iterator creation methods
- [ ] Result set building

### Query Validation
- [ ] Schema validation
- [ ] Type checking
- [ ] Permission checking
- [ ] Resource limit checking

### Expression Evaluation
- [ ] Expression evaluator methods
- [ ] Function call resolution
- [ ] Type coercion methods

## 📝 Implementation Strategy

### 1. Audit Current Patterns

```bash
# Find query engine files
find src/query -name "*.cpp" | while read f; do
    echo "=== $f ==="
    grep -n "return nullptr\|return std::nullopt\|return false" "$f" | head -5
done
```

### 2. Migration Priority

**High Priority (Week 1-2):**
1. Query execution entry points
2. Plan node executors
3. Error-prone operations (parsing, validation)

**Medium Priority (Week 3-4):**
4. Optimization passes
5. Cost estimation
6. Statistics gathering

**Lower Priority (Week 5-6):**
7. Utility functions
8. Helper methods
9. Debug/trace functions

### 3. Error Code Usage

New error codes to use:
- `ERR_QUERY_SYNTAX_ERROR` - Query parsing failed
- `ERR_QUERY_VALIDATION_FAILED` - Validation error
- `ERR_QUERY_EXECUTION_FAILED` - Runtime execution error
- `ERR_QUERY_TIMEOUT` - Query exceeded time limit
- `ERR_QUERY_RESOURCE_LIMIT` - Resource limit exceeded
- `ERR_SCHEMA_VALIDATION_FAILED` - Schema mismatch
- `ERR_API_INVALID_REQUEST` - Invalid query request

## 📋 Implementation Checklist

### Phase 1: Query Parsing (Week 1)
- [ ] `parseQuery()` - Return `Result<QueryAST>`
- [ ] `validateSyntax()` - Return `Result<void>`
- [ ] `parseExpression()` - Return `Result<Expression>`
- [ ] Update all parser error paths

### Phase 2: Query Planning (Week 2)
- [ ] `createPlan()` - Return `Result<QueryPlan>`
- [ ] `optimizePlan()` - Return `Result<QueryPlan>`
- [ ] `validatePlan()` - Return `Result<void>`
- [ ] Update planner error paths

### Phase 3: Query Execution (Week 3-4)
- [ ] `execute()` - Return `Result<ResultSet>`
- [ ] `executeNode()` - Return `Result<...>`
- [ ] `createIterator()` - Return `Result<Iterator*>`
- [ ] Update executor error paths

### Phase 4: Expression Evaluation (Week 4-5)
- [ ] `evaluate()` - Return `Result<Value>`
- [ ] `resolveFunction()` - Return `Result<Function*>`
- [ ] `coerceType()` - Return `Result<Value>`

### Phase 5: Testing & Integration (Week 5-6)
- [ ] Update all query engine tests
- [ ] Integration tests with API layer
- [ ] Performance regression tests
- [ ] Error message quality tests

## 🧪 Testing Requirements

### Unit Tests
- [ ] Test each error code path
- [ ] Test error message formatting
- [ ] Test error propagation through pipeline

### Integration Tests
```cpp
TEST(QueryEngineTest, SyntaxErrorWithDetail) {
    auto result = engine.parseQuery("SELECT * FORM users");  // typo: FORM
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_QUERY_SYNTAX_ERROR);
    EXPECT_THAT(result.error().message(), HasSubstr("FORM"));
    EXPECT_THAT(result.error().message(), HasSubstr("Did you mean FROM?"));
}
```

### Performance Tests
- [ ] Verify no performance regression (< 5% overhead)
- [ ] Benchmark query execution with error handling
- [ ] Profile error path performance

## 📚 Documentation Updates

- [ ] Update query engine documentation
- [ ] Document error codes for query operations
- [ ] Add troubleshooting guide for common query errors
- [ ] Update examples with error handling

### Error Message Examples

**Before:**
```
Query failed
```

**After:**
```
Query syntax error at line 3, column 15: Unexpected token 'FORM'. 
Did you mean 'FROM'? 
Expected: SELECT ... FROM table_name
```

## 🎯 Success Criteria

- [ ] All query engine methods use `Result<T>`
- [ ] Detailed error messages with line/column info
- [ ] All tests pass
- [ ] Performance impact < 5%
- [ ] Code review approved
- [ ] Documentation complete

## 📊 Progress Tracking

**Expected Effort:** 5-6 weeks  
**Priority:** 🔴 High

### Weekly Milestones
- [ ] Week 1: Parser (20% of methods)
- [ ] Week 2: Planner (20% of methods)
- [ ] Week 3-4: Executor (40% of methods)
- [ ] Week 5: Expression evaluation (15% of methods)
- [ ] Week 6: Testing & polish (5% of methods)

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **API Layer Issue:** (depends on this for error propagation)
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md

## 💡 Notes

- **High Traffic:** Query engine is most frequently used component
- **Error Quality:** Focus on helpful, actionable error messages
- **Performance:** Critical path - monitor performance carefully
- **Testing:** Comprehensive testing essential due to complexity
- **Dependencies:** May need to coordinate with API layer migration
