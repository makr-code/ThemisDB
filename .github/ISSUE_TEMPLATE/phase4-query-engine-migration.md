---
name: Phase 4 - Query Engine Migration
about: Track query engine error handling migration to Result<T> pattern
title: '[Phase 4] Query Engine Migration'
labels: ['error-handling', 'phase-4', 'query-engine', 'refactoring']
assignees: ''
---

## 📋 Module: Query Engine

**Priority:** P0 (Critical)  
**Estimated Effort:** 2-3 weeks  
**Complexity:** Medium-High  
**Dependencies:** Phase 4 Foundation PR must be merged

## 🎯 Objective

Migrate query engine error handling from legacy patterns (`return nullptr`, custom Status structs) to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Files to Migrate

**AQL Translator** (`src/query/aql_translator.cpp`):
- [ ] 96 Status returns → `Result<T>` pattern
- [ ] Parse error handling
- [ ] Validation error propagation

**Query Engine** (`src/query/query_engine.cpp`):
- [ ] 5 nullptr returns → `Result<T*>` or `Result<unique_ptr<T>>`
- [ ] 67 Status returns → `Result<T>` pattern
- [ ] Query execution error handling
- [ ] Resource allocation failures

**CTE Subquery** (`src/query/cte_subquery.cpp`):
- [ ] 8 nullptr returns → `Result<T*>`
- [ ] 32 Status returns → `Result<T>` pattern
- [ ] Recursive query error handling
- [ ] Cycle detection errors

**Statistical Aggregator** (`src/query/statistical_aggregator.cpp`):
- [ ] 10 nullptr returns → `Result<T*>`
- [ ] Aggregation failure handling
- [ ] Type mismatch errors

**Other Query Components:**
- [ ] Expression evaluator
- [ ] Join operations
- [ ] Query planner
- [ ] Execution coordinator

**Total:** 28 nullptr sites + 237 Status returns = **265 migration points**

## 📚 Resources

**Foundation Documentation:**
- Phase 4 Migration Matrix: `docs/error_handling/phase4_migration_matrix.md`
- Migration Example: `docs/error_handling/phase4_week2_getOrCreateColumnFamily_example.md`
- Progress Summary: `docs/error_handling/phase4_progress_summary.md`

**Error Codes Available:**
- `ERR_QUERY_INVALID_SYNTAX` (5000)
- `ERR_QUERY_EXECUTION_FAILED` (5001)
- `ERR_QUERY_TIMEOUT` (5002)
- `ERR_QUERY_INVALID_PARAM` (5003)

**Error Codes to Add:**
- [ ] `ERR_QUERY_CTE_CYCLE_DETECTED` (5004)
- [ ] `ERR_QUERY_AGGREGATION_FAILED` (5005)
- [ ] `ERR_QUERY_TYPE_MISMATCH` (5006)
- [ ] `ERR_QUERY_RESOURCE_EXHAUSTED` (5007)

## 🔧 Implementation Steps

### Phase 1: Error Code Addition (Week 1 Day 1-2)
- [ ] Add new query error codes to error registry
- [ ] Register with detailed metadata, causes, solutions
- [ ] Update error documentation

### Phase 2: AQL Translator (Week 1)
- [ ] Migrate parse functions (30 Status returns)
- [ ] Migrate validation functions (40 Status returns)
- [ ] Migrate transformation functions (26 Status returns)
- [ ] Update call sites across query engine
- [ ] Add unit tests for parse error scenarios
- [ ] Build verification

### Phase 3: CTE Subquery (Week 1-2)
- [ ] Migrate CTE construction (8 nullptr + 15 Status)
- [ ] Migrate cycle detection logic
- [ ] Migrate recursive query execution (17 Status)
- [ ] Update call sites
- [ ] Add unit tests for cycle detection
- [ ] Build verification

### Phase 4: Query Engine Core (Week 2)
- [ ] Migrate query initialization (5 nullptr + 20 Status)
- [ ] Migrate query execution pipeline (30 Status)
- [ ] Migrate result handling (17 Status)
- [ ] Update call sites
- [ ] Add unit tests for execution failures
- [ ] Build verification

### Phase 5: Statistical Aggregator (Week 2-3)
- [ ] Migrate aggregation functions (10 nullptr + remaining Status)
- [ ] Migrate type checking logic
- [ ] Update call sites
- [ ] Add unit tests for aggregation failures
- [ ] Build verification

### Phase 6: Testing & Validation (Week 3)
- [ ] Update ~12 existing test files
- [ ] Add complex query error tests
- [ ] Add timeout scenario tests
- [ ] Add CTE cycle detection tests
- [ ] Performance benchmarking (ensure <5% regression)
- [ ] Code review and refinement
- [ ] Documentation updates

## ✅ Acceptance Criteria

- [ ] All 265 query engine functions migrated to `Result<T>` pattern
- [ ] All call sites updated to use Result<T> checks
- [ ] 4 new error codes added and registered
- [ ] Custom Status structs removed
- [ ] Zero build warnings or errors
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] Performance regression <5%
- [ ] Code review approved
- [ ] Documentation updated

## 📝 Migration Pattern

```cpp
// BEFORE: Status pattern
struct QueryResult {
    Status status;
    std::vector<Row> rows;
};

QueryResult executeQuery(const std::string& aql) {
    auto parse_status = parser.parse(aql);
    if (!parse_status.ok) {
        return {parse_status, {}};
    }
    // ...
    return {Status::OK(), rows};
}

// AFTER: Result<T> pattern
Result<std::vector<Row>> executeQuery(const std::string& aql) {
    auto parse_result = parser.parse(aql);
    if (!parse_result) {
        return Err<std::vector<Row>>(
            ERR_QUERY_INVALID_SYNTAX,
            fmt::format("Parse failed: {}", parse_result.error().message())
        );
    }
    // ...
    return Ok(std::move(rows));
}

// Call site update
auto result = engine.executeQuery(aql);
if (result) {
    auto& rows = *result;
    // process rows
} else {
    LOG_ERROR("Query execution failed: {}", result.error().message());
    return result.error();
}
```

## 🔗 Related Issues

- Depends on: Phase 4 Foundation PR
- Coordinates with: Storage Layer Migration (shared dependencies)
- Blocks: Schema Management Migration

## 📊 Progress Tracking

**Week 1:** ⬜⬜⬜⬜⬜ 0%  
**Week 2:** ⬜⬜⬜⬜⬜ 0%  
**Week 3:** ⬜⬜⬜⬜⬜ 0%

**Overall:** 0 of 265 functions migrated (0%)

**Breakdown:**
- AQL Translator: 0 / 96 (0%)
- Query Engine: 0 / 72 (0%)
- CTE Subquery: 0 / 40 (0%)
- Statistical Aggregator: 0 / 10 (0%)
- Others: 0 / 47 (0%)

## ⚠️ High Risk Areas

- **Performance sensitive:** Query execution path must maintain performance
- **High call site count:** AQL translator used throughout codebase
- **Complex error propagation:** Nested query execution requires careful error chaining
- **Type safety:** Aggregation type mismatches need proper error codes

## 💬 Notes

- This is the highest-impact module (P0 priority)
- Largest scope in Phase 4 (265 migration points)
- Pattern established in foundation PR applies here
- Coordinate with storage team on shared error handling
- Consider batching similar functions for efficiency

---
**Assigned to:** TBD  
**Started:** TBD  
**Target Completion:** TBD  
**Actual Completion:** TBD
