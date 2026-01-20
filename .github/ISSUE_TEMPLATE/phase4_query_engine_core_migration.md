---
name: Phase 4 - Query Engine Core Migration
about: Migrate Query Engine Core error handling to Result<T> pattern
title: '[Phase 4] Query Engine Migration - Phase 4: Query Engine Core'
labels: ['P0-critical', 'enhancement', 'error-handling', 'query-engine', 'phase-4']
assignees: ''
---

## 📋 Module: Query Engine Core

**Priority:** P0 (Critical)  
**Estimated Effort:** 2 weeks  
**Complexity:** VERY HIGH  
**Dependencies:** Phase 1-2 must be merged, Phase 3 recommended

## 🎯 Objective

Migrate Query Engine Core error handling from legacy patterns (`return nullptr`, Status structs) to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Migration Points: 72 (5 nullptr + 67 Status)

**File:** `src/query/query_engine.cpp` (3727 lines)

**Breakdown:**
- [ ] Query initialization (5 nullptr + 20 Status = 25 points)
  - Query parsing and setup
  - Resource allocation
  - Context initialization
- [ ] Query execution pipeline (30 Status = 30 points)
  - Query plan execution
  - Iterator management
  - Result streaming
- [ ] Result handling (17 Status = 17 points)
  - Result aggregation
  - Error propagation
  - Resource cleanup

## 📚 Resources

**Foundation Documentation:**
- Phase 1-2 Completion: See merged PR
- Migration Pattern: `docs/error_handling/phase4_query_engine_migration_example.md`
- Roadmap: `MIGRATION_ROADMAP.md`

**Error Codes to Use:**
- `ERR_QUERY_EXECUTION_FAILED` (6102) - General execution failures
- `ERR_QUERY_TIMEOUT` (6103) - Query timeout
- `ERR_QUERY_RESOURCE_EXHAUSTED` (6107) - Resource limits
- `ERR_QUERY_PARSE_FAILED` (6100) - Parse errors (if needed)

## 🔧 Implementation Steps

### Week 1: Query Initialization (25 points)
- [ ] Day 1-2: Migrate query setup functions (5 nullptr + 10 Status)
  - [ ] `initializeQuery()` and related
  - [ ] Context creation
  - [ ] nullptr → Result<T*> or Result<unique_ptr<T>>
- [ ] Day 3-4: Migrate resource allocation (10 Status)
  - [ ] Memory allocation
  - [ ] Index access
  - [ ] Storage access
- [ ] Day 5: Build verification and testing
  - [ ] Unit tests for initialization failures
  - [ ] Resource cleanup tests

### Week 2: Query Execution (30 points)
- [ ] Day 1-2: Migrate execution pipeline (15 Status)
  - [ ] Query plan execution
  - [ ] Operator execution
- [ ] Day 3-4: Migrate iterator management (15 Status)
  - [ ] Iterator creation
  - [ ] Iterator navigation
- [ ] Day 5: Build verification and testing
  - [ ] Execution failure tests
  - [ ] Timeout tests

### Week 2: Result Handling (17 points)
- [ ] Day 1: Migrate result aggregation (8 Status)
  - [ ] Result collection
  - [ ] Streaming results
- [ ] Day 2: Migrate error propagation (9 Status)
  - [ ] Error handling
  - [ ] Cleanup on error
- [ ] Day 3-5: Testing and call site updates
  - [ ] Update all call sites
  - [ ] Integration tests
  - [ ] Performance validation

## 📝 Migration Pattern

### Pattern 1: nullptr → Result<T*>
```cpp
// Before
QueryContext* createContext(...) {
    if (error) return nullptr;
    return new QueryContext(...);
}

// After
Result<std::unique_ptr<QueryContext>> createContext(...) {
    if (error) {
        return Err<std::unique_ptr<QueryContext>>(
            ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            fmt::format("Failed to create context: {}", error_msg)
        );
    }
    return Ok(std::make_unique<QueryContext>(...));
}
```

### Pattern 2: Status → Result<T>
```cpp
// Before
Status executeQuery(const Query& query, Results& out) {
    if (!validate(query)) {
        return Status::Error("Invalid query");
    }
    // ... execute ...
    return Status::OK();
}

// After
Result<Results> executeQuery(const Query& query) {
    if (!validate(query)) {
        return Err<Results>(
            ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Query validation failed"
        );
    }
    // ... execute ...
    return Ok(std::move(results));
}
```

## ✅ Acceptance Criteria

- [ ] All 72 query engine functions migrated to `Result<T>` pattern
- [ ] All 5 nullptr returns eliminated with proper Result<T*>
- [ ] All 67 Status returns converted to Result<T>
- [ ] All call sites updated
- [ ] All unit tests passing
- [ ] Execution failure tests added
- [ ] Timeout scenario tests added
- [ ] No performance regression >5%
- [ ] Code review approved
- [ ] Documentation updated

## 🚧 Known Challenges

1. **Very Large Codebase** - 3727 lines with critical execution path
2. **Performance Sensitive** - Must maintain zero-overhead
3. **High Test Coverage Required** - Many execution scenarios
4. **Complex State Management** - Resource allocation and cleanup
5. **Many Call Sites** - Widely used across application

## 📋 Checklist

- [ ] Read Phase 1-2 completion docs
- [ ] Review migration pattern examples
- [ ] Plan incremental approach (init → execute → results)
- [ ] Create feature branch from develop
- [ ] Implement migrations in order
- [ ] Run tests after each group
- [ ] Update call sites progressively
- [ ] Add comprehensive tests
- [ ] Performance benchmark
- [ ] Code review
- [ ] Update documentation

## 🔗 Related Issues

- Related to: [Phase 4] Query Engine Migration (parent issue)
- Depends on: Phase 1-2 (Statistical Aggregator + CTE Subquery)
- Recommended: Phase 3 (AQL Translator) completed first
- Blocks: Final validation and testing

## 📊 Progress Tracking

**Total:** 72 migration points  
**Completed:** 0 / 72 (0%)

- Query Initialization: 0 / 25 (0%)
- Query Execution: 0 / 30 (0%)
- Result Handling: 0 / 17 (0%)

Update this issue with progress as you complete sections.
