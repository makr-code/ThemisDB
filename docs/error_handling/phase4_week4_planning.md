# Phase 4 Week 4 Planning: Query Engine Migration

**Target:** Query Engine nullptr migrations  
**Priority:** P0 - CRITICAL (User-facing code)  
**Effort Estimate:** 2-3 weeks  
**Status:** 🟡 PLANNING

---

## 🎯 Objective

Migrate Query Engine from nullptr returns to `Result<T*>` pattern. Query engine is critical user-facing code that processes AQL queries, performs statistical aggregations, and evaluates window functions.

---

## 📊 Current State Analysis

### Inventory Results

**Total nullptr Returns Found:** 28 (revised from initial estimate of 35-40)

**Breakdown by File:**

| File | nullptr Count | Complexity | Priority |
|------|---------------|------------|----------|
| `statistical_aggregator.cpp` | 10 | Medium | High |
| `cte_subquery.cpp` | 6 | High | High |
| `aql_translator.cpp` | 3 | Medium | Medium |
| `window_evaluator.cpp` | 2 | Medium | Medium |
| `let_evaluator.cpp` | 1 | Low | Low |
| Other query files | 6 | Various | Various |

### Key Files to Migrate

1. **statistical_aggregator.cpp** (10 nullptr sites)
   - Functions: percentile, variance, stddev, correlation, covariance, IQR, zscore
   - Pattern: Statistical functions returning nullptr on invalid input
   - Error codes needed: ERR_QUERY_INVALID_INPUT, ERR_QUERY_INSUFFICIENT_DATA

2. **cte_subquery.cpp** (6 nullptr sites)
   - CTE (Common Table Expression) handling
   - Subquery evaluation
   - Pattern: Parser/evaluator returning nullptr on parse/validation errors
   - Error codes needed: ERR_QUERY_CTE_CYCLE_DETECTED, ERR_QUERY_INVALID_SYNTAX

3. **aql_translator.cpp** (3 nullptr sites)
   - AQL to internal representation translation
   - Expression translation
   - Error codes available: ERR_QUERY_PARSE_FAILED, ERR_QUERY_INVALID_SYNTAX

4. **window_evaluator.cpp** (2 nullptr sites)
   - Window function evaluation
   - Frame specification parsing
   - Error codes needed: ERR_QUERY_INVALID_WINDOW_SPEC

5. **let_evaluator.cpp** (1 nullptr site)
   - LET clause evaluation
   - Variable binding
   - Error codes available: ERR_QUERY_EXECUTION_FAILED

---

## 🔧 Migration Strategy

### Phase 1: Statistical Aggregator (Week 4, Days 1-2)

**Target:** `statistical_aggregator.cpp` - 10 functions

**Functions to Migrate:**
1. `percentile()` - Returns nullptr for invalid percentile value
2. `variance()` - Returns nullptr for < 2 values
3. `stddev()` - Returns nullptr for invalid data
4. `correlation()` - Returns nullptr for mismatched arrays
5. `covariance()` - Returns nullptr for mismatched arrays
6. `iqr()` - Returns nullptr for < 4 values
7. `zscore()` - Returns nullptr for invalid data

**Pattern:**
```cpp
// Before
AggregateResult* StatisticalAggregator::percentile(const std::vector<double>& values, double p) {
    if (p < 0.0 || p > 1.0) {
        return nullptr;  // No error context
    }
    // ...
}

// After
Result<AggregateResult*> StatisticalAggregator::percentile(const std::vector<double>& values, double p) {
    if (p < 0.0 || p > 1.0) {
        return Err<AggregateResult*>(
            errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
            fmt::format("Percentile must be between 0 and 1, got: {}", p)
        );
    }
    // ...
    return Ok(result);
}
```

**Error Codes to Add:**
- `ERR_QUERY_INVALID_INPUT` (6200) - Invalid input parameters
- `ERR_QUERY_INSUFFICIENT_DATA` (6201) - Not enough data for statistical function

---

### Phase 2: CTE & Subquery (Week 4, Days 3-5)

**Target:** `cte_subquery.cpp` - 6 nullptr returns

**Key Functions:**
- `parseCTE()` - Parse Common Table Expression
- `evaluateSubquery()` - Evaluate subquery
- `detectCycle()` - Check for circular references
- `validateCTE()` - Validate CTE structure

**Pattern:**
```cpp
// Before
CTENode* CTEParser::parseCTE(const std::string& query) {
    if (hasCircularRef()) {
        return nullptr;  // Silent failure
    }
    // ...
}

// After
Result<CTENode*> CTEParser::parseCTE(const std::string& query) {
    if (hasCircularRef()) {
        return Err<CTENode*>(
            errors::ErrorCode::ERR_QUERY_CTE_CYCLE_DETECTED,
            fmt::format("Circular CTE reference detected in: {}", query)
        );
    }
    // ...
    return Ok(node);
}
```

**Error Codes to Add:**
- `ERR_QUERY_CTE_CYCLE_DETECTED` (6104) - Circular CTE reference
- `ERR_QUERY_SUBQUERY_FAILED` (6105) - Subquery evaluation failed

---

### Phase 3: Translator & Evaluators (Week 4-5, Days 6-8)

**Targets:**
- `aql_translator.cpp` (3 nullptr sites)
- `window_evaluator.cpp` (2 nullptr sites)  
- `let_evaluator.cpp` (1 nullptr site)

**Pattern:** Similar to previous - nullptr → Result<T*>

**Error Codes Available:**
- `ERR_QUERY_PARSE_FAILED` (6100)
- `ERR_QUERY_INVALID_SYNTAX` (6101)
- `ERR_QUERY_EXECUTION_FAILED` (6102)

**Error Codes to Add:**
- `ERR_QUERY_INVALID_WINDOW_SPEC` (6106) - Invalid window specification

---

## 📋 Week 4 Detailed Plan

### Day 1: Statistical Aggregator Part 1
- [ ] Migrate `percentile()` function
- [ ] Migrate `variance()` function
- [ ] Migrate `stddev()` function
- [ ] Add error codes: ERR_QUERY_INVALID_INPUT, ERR_QUERY_INSUFFICIENT_DATA
- [ ] Update call sites (if any)
- [ ] Create migration example document

### Day 2: Statistical Aggregator Part 2
- [ ] Migrate `correlation()` function
- [ ] Migrate `covariance()` function
- [ ] Migrate `iqr()` function
- [ ] Migrate `zscore()` function
- [ ] Update remaining aggregator functions
- [ ] Test all statistical functions

### Day 3: CTE Parser Part 1
- [ ] Migrate `parseCTE()` function
- [ ] Migrate `detectCycle()` function
- [ ] Add error code: ERR_QUERY_CTE_CYCLE_DETECTED
- [ ] Update call sites

### Day 4: CTE Parser Part 2
- [ ] Migrate `evaluateSubquery()` function
- [ ] Migrate `validateCTE()` function
- [ ] Add error code: ERR_QUERY_SUBQUERY_FAILED
- [ ] Update call sites
- [ ] Test CTE functionality

### Day 5: Translator
- [ ] Migrate `aql_translator.cpp` functions (3 sites)
- [ ] Update call sites
- [ ] Test translation functionality

### Day 6: Window & LET Evaluators
- [ ] Migrate `window_evaluator.cpp` functions (2 sites)
- [ ] Migrate `let_evaluator.cpp` function (1 site)
- [ ] Add error code: ERR_QUERY_INVALID_WINDOW_SPEC
- [ ] Update call sites

### Day 7: Testing & Documentation
- [ ] Update query engine tests
- [ ] Add error scenario tests
- [ ] Performance testing
- [ ] Create Week 4 completion report

### Day 8: Buffer & Handoff
- [ ] Final review
- [ ] Update documentation
- [ ] Prepare for Week 5 (LLM/LoRA)

---

## 🧪 Testing Strategy

### Unit Tests to Add

**Statistical Aggregator:**
- Test invalid percentile (< 0 or > 1)
- Test variance with < 2 values
- Test IQR with < 4 values
- Test correlation with mismatched arrays
- Verify error messages include input values

**CTE Parser:**
- Test circular CTE reference detection
- Test nested CTEs
- Test invalid CTE syntax
- Verify cycle detection error messages

**Translator:**
- Test invalid AQL syntax
- Test unsupported operations
- Verify translation error messages

### Integration Tests
- [ ] End-to-end query with statistical functions
- [ ] Complex CTE queries
- [ ] Window function queries
- [ ] Error propagation through query pipeline

---

## 📊 Success Metrics

### Coverage Targets
- [ ] All 28 nullptr returns → Result<T*>
- [ ] 7+ new error codes added
- [ ] 100% of query engine functions use Result<>
- [ ] All tests passing

### Quality Gates
- [ ] No compilation errors
- [ ] No breaking changes to external APIs
- [ ] Error context includes query details
- [ ] Performance within 5% baseline

---

## 🚨 Risk Assessment

### Technical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Complex query parsing | High | Incremental migration, extensive testing |
| Breaking API changes | Critical | Careful call site analysis |
| Performance regression | Medium | Benchmark before/after |
| Statistical edge cases | Medium | Comprehensive test coverage |

### Challenges

1. **Call Site Discovery:** Query engine functions are widely used
2. **Error Context:** Need to include query details in errors
3. **Testing:** Need comprehensive query test suite
4. **Performance:** Statistical functions in hot path

---

## 📚 Error Codes Summary

### Existing Codes
- `ERR_QUERY_PARSE_FAILED` (6100) - Query parsing failed
- `ERR_QUERY_INVALID_SYNTAX` (6101) - Invalid query syntax
- `ERR_QUERY_EXECUTION_FAILED` (6102) - Query execution failed
- `ERR_QUERY_TIMEOUT` (6103) - Query timeout

### New Codes to Add
- `ERR_QUERY_CTE_CYCLE_DETECTED` (6104) - Circular CTE reference
- `ERR_QUERY_SUBQUERY_FAILED` (6105) - Subquery evaluation failed
- `ERR_QUERY_INVALID_WINDOW_SPEC` (6106) - Invalid window specification
- `ERR_QUERY_INVALID_INPUT` (6200) - Invalid function input
- `ERR_QUERY_INSUFFICIENT_DATA` (6201) - Not enough data for operation

---

## 📁 Files to Modify

### Source Files (8 files)
1. `src/query/statistical_aggregator.cpp` - 10 nullptr sites
2. `src/query/cte_subquery.cpp` - 6 nullptr sites
3. `src/query/aql_translator.cpp` - 3 nullptr sites
4. `src/query/window_evaluator.cpp` - 2 nullptr sites
5. `src/query/let_evaluator.cpp` - 1 nullptr site
6. `src/query/*.cpp` - Remaining ~6 sites

### Header Files
1. `include/query/statistical_aggregator.h`
2. `include/query/cte_subquery.h`
3. `include/query/aql_translator.h`
4. `include/query/window_evaluator.h`
5. `include/query/let_evaluator.h`

### Test Files (~12 files)
1. `tests/query/test_statistical_aggregator.cpp`
2. `tests/query/test_cte_subquery.cpp`
3. `tests/query/test_aql_translator.cpp`
4. `tests/query/test_window_evaluator.cpp`
5. Other query test files

---

## 🔗 Dependencies

### Prerequisites
- ✅ Phase 1-2 foundation complete
- ✅ Week 1-3 storage layer complete
- ✅ Error registry with query codes

### Blocking Issues
- None identified

### External Dependencies
- Query engine team coordination
- Integration test suite availability
- Performance baseline data

---

## 🎯 Definition of Done

Week 4 is complete when:

**Code:**
- [ ] All 28 nullptr returns → Result<T*>
- [ ] All new error codes added to registry
- [ ] All call sites updated
- [ ] No compilation errors or warnings

**Testing:**
- [ ] All unit tests passing
- [ ] New error scenario tests added
- [ ] Integration tests passing
- [ ] Performance benchmarks < 5% regression

**Documentation:**
- [ ] Migration examples documented
- [ ] Error codes documented
- [ ] Week 4 completion report created
- [ ] Progress tracking updated

**Validation:**
- [ ] Code review approved
- [ ] QA validation passed
- [ ] No P0/P1 bugs introduced

---

## 📅 Timeline

**Start Date:** Week 4 Day 1  
**Target Completion:** Week 4 Day 8  
**Buffer:** 2 days for issues  
**Next Phase:** Week 5-7 (LLM/LoRA migration)

---

## 📞 Communication

### Stakeholders to Notify
- Query engine team
- API consumers
- QA team
- Documentation team

### Updates Needed
- Daily progress updates
- Breaking changes communication
- Performance impact reports
- Testing status

---

## ✅ Next Steps

### Immediate Actions
1. Review this plan with team
2. Set up Week 4 branch/tracking
3. Add new error codes to registry
4. Begin statistical_aggregator.cpp migration

### Preparation
- Set up performance baseline
- Prepare test environment
- Coordinate with query engine team
- Review existing query tests

---

**Status:** 🟡 Ready to Start  
**Priority:** P0 - CRITICAL  
**Confidence:** HIGH - Pattern established from Weeks 1-3  
**Estimated Effort:** 8-10 days

---

*Plan Created: 2026-01-20*  
*Next Review: End of Day 2*  
*Owner: TBD*
