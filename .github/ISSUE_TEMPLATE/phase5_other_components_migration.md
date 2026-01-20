---
name: Phase 5 - Other Query Components Migration
about: Migrate remaining query components to Result<T> pattern
title: '[Phase 4] Query Engine Migration - Phase 5: Other Components'
labels: ['P1-high', 'enhancement', 'error-handling', 'query-engine', 'phase-4']
assignees: ''
---

## 📋 Module: Other Query Components

**Priority:** P1 (High)  
**Estimated Effort:** 3 weeks  
**Complexity:** MEDIUM  
**Dependencies:** Phase 1-2 must be merged

## 🎯 Objective

Migrate remaining query component error handling to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Migration Points: ~62

**Components:**
- [ ] Expression Evaluator (~30 points)
  - Expression evaluation functions
  - Type checking logic
  - Operator implementations
- [ ] Join Operations (~20 points)
  - Join execution
  - Join optimization
  - Result merging
- [ ] Query Planner (~12 points)
  - Query planning
  - Optimization rules
  - Cost estimation

## 📚 Resources

**Foundation Documentation:**
- Phase 1-2 Completion: See merged PR
- Migration Pattern: `docs/error_handling/phase4_query_engine_migration_example.md`
- Roadmap: `MIGRATION_ROADMAP.md`

**Error Codes to Use:**
- `ERR_QUERY_EXECUTION_FAILED` (6102) - General failures
- `ERR_QUERY_TYPE_MISMATCH` (6106) - Type errors
- `ERR_QUERY_RESOURCE_EXHAUSTED` (6107) - Resource limits

## 🔧 Implementation Steps

### Week 1: Expression Evaluator (~30 points)
- [ ] Day 1-2: Migrate expression evaluation functions
  - [ ] Basic expression evaluation
  - [ ] Complex expression handling
- [ ] Day 3: Migrate type checking logic
  - [ ] Type inference
  - [ ] Type validation
- [ ] Day 4-5: Migrate operator implementations
  - [ ] Binary operators
  - [ ] Unary operators
  - [ ] Special operators
- [ ] Build verification and testing

### Week 2: Join Operations (~20 points)
- [ ] Day 1-2: Migrate join execution functions
  - [ ] Hash join
  - [ ] Nested loop join
  - [ ] Merge join
- [ ] Day 3: Migrate join optimization
  - [ ] Join ordering
  - [ ] Join strategy selection
- [ ] Day 4-5: Migrate result merging
  - [ ] Result combination
  - [ ] Duplicate elimination
- [ ] Build verification and testing

### Week 3: Query Planner (~12 points)
- [ ] Day 1-2: Migrate query planning functions
  - [ ] Plan generation
  - [ ] Plan validation
- [ ] Day 3: Migrate optimization rules
  - [ ] Rule application
  - [ ] Rule ordering
- [ ] Day 4-5: Update call sites and testing
  - [ ] Search for all callers
  - [ ] Update Result<T> usage
  - [ ] Integration tests
  - [ ] Performance validation

## 📝 Migration Pattern

```cpp
// Expression Evaluator Example
// Before
Status evaluateExpression(const Expression& expr, Value& out) {
    if (!expr.valid()) {
        return Status::Error("Invalid expression");
    }
    // ...
    return Status::OK();
}

// After
Result<Value> evaluateExpression(const Expression& expr) {
    if (!expr.valid()) {
        return Err<Value>(
            ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            fmt::format("Invalid expression: {}", expr.toString())
        );
    }
    // ...
    return Ok(std::move(result_value));
}
```

## ✅ Acceptance Criteria

- [ ] All ~62 component functions migrated to `Result<T>` pattern
- [ ] Expression evaluator fully migrated (~30 points)
- [ ] Join operations fully migrated (~20 points)
- [ ] Query planner fully migrated (~12 points)
- [ ] All call sites updated
- [ ] All unit tests passing
- [ ] Component-specific tests added
- [ ] No performance regression >5%
- [ ] Code review approved
- [ ] Documentation updated

## 📋 Checklist

- [ ] Read Phase 1-2 completion docs
- [ ] Review migration pattern examples
- [ ] Plan incremental approach by component
- [ ] Create feature branch from develop
- [ ] Implement migrations in order
- [ ] Run tests after each component
- [ ] Update call sites
- [ ] Add comprehensive tests
- [ ] Performance benchmark
- [ ] Code review
- [ ] Update documentation

## 🔗 Related Issues

- Related to: [Phase 4] Query Engine Migration (parent issue)
- Depends on: Phase 1-2 (Statistical Aggregator + CTE Subquery)
- Parallel with: Phase 3 (AQL Translator), Phase 4 (Query Engine Core)

## 📊 Progress Tracking

**Total:** ~62 migration points  
**Completed:** 0 / 62 (0%)

- Expression Evaluator: 0 / ~30 (0%)
- Join Operations: 0 / ~20 (0%)
- Query Planner: 0 / ~12 (0%)

Update this issue with progress as you complete sections.
