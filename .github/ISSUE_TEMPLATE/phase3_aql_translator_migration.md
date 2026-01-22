---
name: Phase 3 - AQL Translator Migration
about: Migrate AQL Translator error handling to Result<T> pattern
title: '[Phase 4] Query Engine Migration - Phase 3: AQL Translator'
labels: ['P0-critical', 'enhancement', 'error-handling', 'query-engine', 'phase-4']
assignees: ''
---

## 📋 Module: AQL Translator

**Priority:** P0 (Critical)  
**Estimated Effort:** 2-3 weeks  
**Complexity:** VERY HIGH  
**Dependencies:** Phase 1-2 must be merged

## 🎯 Objective

Migrate AQL Translator error handling from legacy Status returns to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Migration Points: 96

**File:** `src/query/aql_translator.cpp` (1409 lines)

**Breakdown:**
- [ ] Parse functions (30 Status returns)
  - Query parsing
  - Expression parsing
  - Filter parsing
- [ ] Validation functions (40 Status returns)
  - Syntax validation
  - Semantic validation
  - Type checking
- [ ] Transformation functions (26 Status returns)
  - AST transformation
  - Query optimization
  - Code generation

## 📚 Resources

**Foundation Documentation:**
- Phase 1-2 Completion: See merged PR
- Migration Pattern: `docs/error_handling/phase4_query_engine_migration_example.md`
- Roadmap: `MIGRATION_ROADMAP.md`
- Code Review: `CODE_REVIEW.md`

**Error Codes to Use:**
- `ERR_QUERY_PARSE_FAILED` (6100) - Parse errors
- `ERR_QUERY_INVALID_SYNTAX` (6101) - Syntax errors
- `ERR_QUERY_EXECUTION_FAILED` (6102) - General failures
- `ERR_QUERY_TYPE_MISMATCH` (6106) - Type errors

**Error Codes to Add (if needed):**
- Consider adding specific parse error codes for different AQL constructs

## 🔧 Implementation Steps

### Week 1: Parse Functions (30 points)
- [ ] Day 1-2: Migrate query parsing functions
  - [ ] `parseQuery()` and related functions
  - [ ] Error code selection and context messages
- [ ] Day 3-4: Migrate expression parsing functions
  - [ ] Expression evaluators
  - [ ] Operator parsing
- [ ] Day 5: Migrate filter parsing functions
  - [ ] Filter expressions
  - [ ] Predicate handling
- [ ] Build verification and syntax validation

### Week 2: Validation Functions (40 points)
- [ ] Day 1-2: Migrate syntax validation functions
  - [ ] Grammar validation
  - [ ] Keyword validation
- [ ] Day 3-4: Migrate semantic validation functions
  - [ ] Variable scope checking
  - [ ] Function existence validation
- [ ] Day 5: Migrate type checking functions
  - [ ] Type inference
  - [ ] Type compatibility checking
- [ ] Build verification

### Week 3: Transformation Functions (26 points)
- [ ] Day 1-2: Migrate AST transformation functions
  - [ ] Tree rewriting
  - [ ] Node transformation
- [ ] Day 3: Migrate query optimization functions
  - [ ] Optimization rules
  - [ ] Cost estimation
- [ ] Day 4-5: Migrate code generation functions
  - [ ] Query plan generation
  - [ ] Execution plan building
- [ ] Build verification

### Week 3: Testing & Call Sites
- [ ] Update call sites across query engine
  - [ ] Search for all callers: `grep -r "AQLTranslator::" src/`
  - [ ] Update Result<T> usage patterns
- [ ] Add unit tests for parse error scenarios
  - [ ] Invalid syntax tests
  - [ ] Type mismatch tests
  - [ ] Complex query tests
- [ ] Build and test verification
- [ ] Performance validation (ensure <5% regression)

## 📝 Migration Pattern

```cpp
// Before
Status parseQuery(const std::string& query, Query& out) {
    if (query.empty()) {
        return Status::Error("Empty query");
    }
    // ...
    return Status::OK();
}

// After
Result<Query> parseQuery(const std::string& query) {
    if (query.empty()) {
        return Err<Query>(
            ErrorCode::ERR_QUERY_PARSE_FAILED,
            "Cannot parse empty query string"
        );
    }
    // ...
    return Ok(std::move(parsed_query));
}
```

## ✅ Acceptance Criteria

- [ ] All 96 AQL Translator functions migrated to `Result<T>` pattern
- [ ] All call sites updated to use Result<T>
- [ ] All unit tests passing
- [ ] Parse error tests added for major scenarios
- [ ] No performance regression >5%
- [ ] Code review approved
- [ ] Documentation updated

## 🚧 Known Challenges

1. **Large Codebase** - 1409 lines with complex parsing logic
2. **Many Dependencies** - Called from multiple places
3. **Complex Error Scenarios** - Many different parse error types
4. **Call Site Updates** - Extensive updates needed across codebase

## 📋 Checklist

- [ ] Read Phase 1-2 completion docs
- [ ] Review migration pattern examples
- [ ] Plan incremental approach (parse → validate → transform)
- [ ] Create feature branch from develop
- [ ] Implement migrations in order
- [ ] Run tests after each group
- [ ] Update call sites
- [ ] Add new tests
- [ ] Run full test suite
- [ ] Performance benchmark
- [ ] Code review
- [ ] Update documentation

## 🔗 Related Issues

- Related to: [Phase 4] Query Engine Migration (parent issue)
- Depends on: Phase 1-2 (Statistical Aggregator + CTE Subquery)
- Blocks: Phase 4 (Query Engine Core)

## 📊 Progress Tracking

**Total:** 96 migration points  
**Completed:** 0 / 96 (0%)

Update this issue with progress as you complete sections.
