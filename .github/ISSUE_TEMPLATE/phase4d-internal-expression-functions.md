---
name: Phase 4D - Internal Expression Functions Migration
about: Migrate internal expression evaluation functions to Result<T> pattern
title: '[Phase 4D] Migrate Internal Expression Functions to Result<T>'
labels: enhancement, error-handling, query-engine, P2
assignees: ''
---

## 📋 Module: Internal Expression Functions

**Priority:** P2 (Medium)  
**Estimated Effort:** 3-4 days  
**Complexity:** HIGH  
**Dependencies:** Phase 4A-4C must be merged

## 🎯 Objective

Complete the remaining 25% of the Query Engine Error Handling Migration by migrating internal expression evaluation functions to the unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Migration Points: ~15 (remaining of 62 total)

**Components:**
- [ ] Internal Expression Evaluation (~8 points)
  - `qe_evalExpr()` internals (~300 lines)
  - `qe_evalFunction()` dispatcher (~200 lines)
  - Recursive evaluation logic
  - Expression type checking

- [ ] Spatial Functions (~5 points)
  - ST_Point, ST_AsGeoJSON
  - ST_Distance, ST_Buffer
  - ST_Intersects, ST_Contains
  - ST_Within, ST_DWithin
  - ~15 spatial functions total

- [ ] Type Conversion Functions (~2 points)
  - `qe_toNumber()` error handling
  - `qe_toBool()` error handling
  - Type validation and conversion

## 📚 Background

Phase 4A-4C completed 75% of the migration (47 of 62 points) by migrating all major public APIs:
- ✅ Phase 4A: Expression Evaluator public API (`evaluateExpression`)
- ✅ Phase 4B: Join Operations (`executeJoin`, `executeGroupBy`)
- ✅ Phase 4C: Query Planner (`executeOptimizedKeys`, `executeOptimizedEntities`)

Phase 4D focuses on the internal implementation details of expression evaluation.

## 🔧 Implementation Steps

### Day 1: Analysis and Planning
- [ ] Review current `qe_evalExpr()` implementation (~300 lines)
- [ ] Identify all function calls that need Result<T>
- [ ] Map dependencies between functions
- [ ] Create migration order (bottom-up approach)

### Day 2: Core Function Migration
- [ ] Migrate `qe_toNumber()` to `Result<double>`
- [ ] Migrate `qe_toBool()` to `Result<bool>`
- [ ] Update all call sites for these helper functions
- [ ] Run unit tests

### Day 3: Expression Evaluation
- [ ] Migrate `qe_evalExpr()` to return `Result<nlohmann::json>`
- [ ] Update recursive calls
- [ ] Handle binary operators (AND, OR, EQ, NE, LT, GT, etc.)
- [ ] Handle unary operators (NOT, MINUS)
- [ ] Run integration tests

### Day 4: Function Dispatcher
- [ ] Migrate `qe_evalFunction()` to return `Result<nlohmann::json>`
- [ ] Update string functions (LENGTH, CONCAT, SUBSTRING, UPPER, LOWER)
- [ ] Update math functions (ABS, CEIL, FLOOR, ROUND, MIN, MAX)
- [ ] Run function tests

### Day 5: Spatial Functions & Testing
- [ ] Migrate ST_* spatial functions (~15 functions)
- [ ] Update error handling for geometry parsing
- [ ] Run spatial query tests
- [ ] Performance benchmarking
- [ ] Update documentation

## 📝 Error Codes to Use

- `ERR_QUERY_EXECUTION_FAILED` (6102) - General execution failures
- `ERR_QUERY_TYPE_MISMATCH` (6106) - Type conversion errors
- `ERR_QUERY_INVALID_ARGUMENT` (6103) - Invalid function arguments

## 🎨 Migration Pattern

**Before:**
```cpp
static nlohmann::json qe_evalExpr(const Expression& expr, const Context& ctx) {
    if (!expr) return nlohmann::json(nullptr);
    
    switch (expr->getType()) {
        case BinaryOp: {
            auto left = qe_evalExpr(expr->left, ctx);
            auto right = qe_evalExpr(expr->right, ctx);
            // Process...
            return result;
        }
    }
    throw std::runtime_error("Invalid expression");
}
```

**After:**
```cpp
static Result<nlohmann::json> qe_evalExpr(const Expression& expr, const Context& ctx) {
    if (!expr) return Ok(nlohmann::json(nullptr));
    
    switch (expr->getType()) {
        case BinaryOp: {
            auto left_result = qe_evalExpr(expr->left, ctx);
            if (!left_result) return left_result; // Propagate error
            
            auto right_result = qe_evalExpr(expr->right, ctx);
            if (!right_result) return right_result; // Propagate error
            
            // Process...
            return Ok(result);
        }
    }
    return Err<nlohmann::json>(
        ERR_QUERY_EXECUTION_FAILED,
        "Invalid expression type"
    );
}
```

## ⚠️ Risks and Considerations

### High Complexity
- **Risk:** Deep recursion in expression evaluation
- **Mitigation:** Migrate bottom-up, test incrementally

### Performance Critical
- **Risk:** Expression evaluation is in hot path
- **Mitigation:** Benchmark before/after, optimize if needed

### Large Codebase Impact
- **Risk:** ~500 lines of interconnected code
- **Mitigation:** Use compiler to find all call sites, systematic updates

### Spatial Function Complexity
- **Risk:** Geometry parsing failures need proper handling
- **Mitigation:** Add comprehensive error messages, test with invalid geometries

## ✅ Acceptance Criteria

- [ ] All internal expression functions return Result<T>
- [ ] All spatial ST_* functions return Result<T>
- [ ] No std::runtime_error throws in expression evaluation
- [ ] All helper functions (qe_toNumber, qe_toBool) return Result<T>
- [ ] Performance impact < 2% on benchmark queries
- [ ] All existing tests pass
- [ ] New error scenario tests added
- [ ] Documentation updated

## 📊 Testing Requirements

### Unit Tests
- [ ] Test type conversion errors (string to number, etc.)
- [ ] Test division by zero in expressions
- [ ] Test invalid function arguments
- [ ] Test spatial function errors (invalid geometries)
- [ ] Test recursive expression errors

### Integration Tests
- [ ] Run full query engine test suite
- [ ] Test complex nested expressions
- [ ] Test spatial queries with invalid data
- [ ] Test error propagation through joins

### Performance Tests
- [ ] Benchmark simple expressions (baseline)
- [ ] Benchmark complex nested expressions
- [ ] Benchmark spatial functions
- [ ] Compare with pre-migration baseline

## 📚 Resources

- **Previous Phases:** See `PHASE4A_EXPRESSION_EVALUATOR_COMPLETE.md`, `PHASE4B_JOIN_OPERATIONS_COMPLETE.md`, `PHASE4C_QUERY_PLANNER_COMPLETE.md`
- **Final Summary:** See `PHASE4_FINAL_SUMMARY.md`
- **Error Infrastructure:** `include/utils/expected.h`, `include/utils/error_registry.h`
- **Migration Pattern:** Follow patterns from Phase 4A-4C

## 🔗 Related Issues

- Depends on: Phases 4A-4C merge
- Blocks: None (optional enhancement)
- Related to: Error Handling Migration Roadmap

## 💡 Notes

- This is an **optional enhancement** - Phase 4A-4C already provides 75% coverage
- Can be split into smaller sub-tasks if needed
- Consider performance impact carefully given hot path nature
- Maintain backward compatibility where possible
