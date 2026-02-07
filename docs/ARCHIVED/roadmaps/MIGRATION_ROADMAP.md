# Query Engine Error Handling Migration - Roadmap

## 📊 Overall Progress

**Total Migration Points:** 265  
**Completed:** 35 (13.2%)  
**Remaining:** 230 (86.8%)

---

## ✅ Completed Phases

### Phase 1: Error Code Addition & Statistical Aggregator ✅
**Completion Date:** 2026-01-20  
**Migration Points:** 10

**Accomplishments:**
- Added 4 query error codes (6104-6107)
- Migrated 9 functions in StatisticalAggregator
- Updated ~15 tests
- Created comprehensive migration documentation

**Files:**
- `include/utils/error_registry.h`
- `src/utils/error_registry.cpp`
- `include/query/statistical_aggregator.h`
- `src/query/statistical_aggregator.cpp`
- `tests/test_statistical_aggregations.cpp`

**Documentation:**
- `docs/error_handling/phase4_query_engine_migration_example.md`
- `PHASE1_COMPLETE_SUMMARY.md`

---

### Phase 2: CTE Subquery ✅
**Completion Date:** 2026-01-20  
**Migration Points:** 25

**Accomplishments:**
- Migrated CTEEvaluator (10 error sites)
- Migrated SubqueryEvaluator (15 error sites)
- Specialized error codes for cycle detection, resource exhaustion
- Context-rich error messages

**Files:**
- `include/query/cte_subquery.h`
- `src/query/cte_subquery.cpp`

**Documentation:**
- `PHASE2_COMPLETE_SUMMARY.md`

---

## ⏳ Remaining Phases

### Phase 3: AQL Translator
**Priority:** P0 - CRITICAL  
**Estimated Effort:** 2-3 weeks  
**Complexity:** VERY HIGH  
**Migration Points:** 96

**Scope:**
- Parse functions (30 Status returns)
- Validation functions (40 Status returns)
- Transformation functions (26 Status returns)
- Update call sites across query engine
- Add unit tests for parse error scenarios

**Files to Migrate:**
- `src/query/aql_translator.cpp` (1409 lines)
- `include/query/aql_translator.h`

**Error Codes to Use:**
- `ERR_QUERY_PARSE_FAILED` (6100)
- `ERR_QUERY_INVALID_SYNTAX` (6101)
- `ERR_QUERY_TYPE_MISMATCH` (6106)

**Challenges:**
- Large codebase (1409 lines)
- Complex parsing logic
- Many call sites to update
- Need comprehensive parse error tests

---

### Phase 4: Query Engine Core
**Priority:** P0 - CRITICAL  
**Estimated Effort:** 2 weeks  
**Complexity:** VERY HIGH  
**Migration Points:** 72 (5 nullptr + 67 Status)

**Scope:**
- Query initialization (5 nullptr + 20 Status)
- Query execution pipeline (30 Status)
- Result handling (17 Status)
- Update call sites
- Add unit tests for execution failures

**Files to Migrate:**
- `src/query/query_engine.cpp` (3727 lines)
- `include/query/query_engine.h`

**Error Codes to Use:**
- `ERR_QUERY_EXECUTION_FAILED` (6102)
- `ERR_QUERY_TIMEOUT` (6103)
- `ERR_QUERY_RESOURCE_EXHAUSTED` (6107)

**Challenges:**
- Very large codebase (3727 lines)
- Critical execution path
- High test coverage required
- Performance-sensitive code

---

### Phase 5: Expression Evaluator
**Priority:** P1 - HIGH  
**Estimated Effort:** 1 week  
**Complexity:** MEDIUM  
**Migration Points:** ~30

**Scope:**
- Expression evaluation functions
- Type checking logic
- Operator implementations
- Update call sites
- Add unit tests

**Files to Migrate:**
- `src/query/expression_evaluator.cpp` (estimate)
- `include/query/expression_evaluator.h`

**Error Codes to Use:**
- `ERR_QUERY_TYPE_MISMATCH` (6106)
- `ERR_QUERY_EXECUTION_FAILED` (6102)

---

### Phase 6: Join Operations
**Priority:** P1 - HIGH  
**Estimated Effort:** 1 week  
**Complexity:** MEDIUM  
**Migration Points:** ~20

**Scope:**
- Join execution functions
- Join optimization
- Update call sites
- Add unit tests

**Files to Migrate:**
- Join-related functions in `query_engine.cpp`
- Separate join files (if any)

**Error Codes to Use:**
- `ERR_QUERY_EXECUTION_FAILED` (6102)
- `ERR_QUERY_RESOURCE_EXHAUSTED` (6107)

---

### Phase 7: Query Planner & Optimizer
**Priority:** P2 - MEDIUM  
**Estimated Effort:** 1 week  
**Complexity:** MEDIUM  
**Migration Points:** ~12

**Scope:**
- Query planning functions
- Optimization rules
- Update call sites
- Add unit tests

**Files to Migrate:**
- `src/query/query_optimizer.cpp`
- Related planning code

**Error Codes to Use:**
- `ERR_QUERY_EXECUTION_FAILED` (6102)

---

### Phase 8: Testing & Validation
**Priority:** P0 - CRITICAL  
**Estimated Effort:** 1 week  
**Complexity:** MEDIUM

**Scope:**
- Update ~12 existing test files:
  - `tests/test_query_engine.cpp`
  - `tests/test_query_engine_di.cpp`
  - `tests/test_query_engine_join.cpp`
  - `tests/test_recursive_ctes.cpp`
  - `tests/test_cte_cache.cpp`
  - Others
- Add complex query error tests
- Add timeout scenario tests
- Add CTE cycle detection tests
- Performance benchmarking (ensure <5% regression)
- Code review and refinement
- Documentation updates

**Acceptance Criteria:**
- All tests passing
- No performance regression >5%
- All error codes tested
- Documentation complete

---

## 📅 Proposed Timeline

### Week 3-4: Phase 3 - AQL Translator
- Week 3: Parse functions (30 points)
- Week 4: Validation & transformation (66 points)

### Week 5-6: Phase 4 - Query Engine Core
- Week 5: Query initialization & execution (52 points)
- Week 6: Result handling & call sites (20 points)

### Week 7: Phase 5 - Expression Evaluator
- Expression functions (30 points)

### Week 8: Phase 6 - Join Operations
- Join functions (20 points)

### Week 9: Phase 7 - Query Planner
- Planning & optimization (12 points)

### Week 10: Phase 8 - Testing & Validation
- Test updates and validation

---

## 🎯 Success Metrics

### Coverage Targets

| Metric | Target | Current | Status |
|--------|--------|---------|--------|
| Migration Points Completed | 265 / 265 | 35 / 265 | 🟡 13.2% |
| nullptr Sites Migrated | 28 | 17 | 🟡 60.7% |
| Status Returns Migrated | 237 | 18 | 🟡 7.6% |
| Error Codes Defined | 10+ | 8 | ✅ 80% |
| Test Files Updated | ~12 | 1 | 🟡 8.3% |

### Quality Gates

- ✅ No P0/P1 bugs introduced
- 🟡 All existing tests passing (pending build)
- ✅ Error context preserved in 100% of cases
- ✅ Documentation updated
- 🟡 Team training complete (pending)

---

## 🚨 Risk Management

### Technical Risks

| Risk | Impact | Mitigation | Status |
|------|--------|------------|--------|
| AQL Translator complexity | High | Incremental migration, extensive testing | ⏳ Planned |
| Query Engine size (3727 lines) | High | Break into sub-phases | ⏳ Planned |
| Performance regression | Medium | Continuous benchmarking | ⏳ Pending |
| Breaking API changes | Critical | Backward compatibility where possible | ✅ Mitigated |
| Call site updates | High | Comprehensive search, update in batches | ⏳ Planned |

### Contingency Plans

1. **If AQL Translator takes >3 weeks:**
   - Break into smaller sub-phases
   - Focus on high-priority parse functions first
   - Defer optimization-related code

2. **If Performance Regresses >5%:**
   - Profile hot paths
   - Optimize Result<T> usage in critical sections
   - Consider conditional compilation for legacy paths

3. **If Call Site Updates are Extensive:**
   - Create compatibility shims temporarily
   - Update call sites in waves
   - Use compiler warnings to track progress

---

## 📚 Resources

### Documentation
- `docs/error_handling/phase4_migration_matrix.md` - Overall migration plan
- `docs/error_handling/phase4_query_engine_migration_example.md` - Statistical Aggregator example
- `docs/error_handling/phase4_week2_getOrCreateColumnFamily_example.md` - RocksDB example
- `PHASE1_COMPLETE_SUMMARY.md` - Phase 1 summary
- `PHASE2_COMPLETE_SUMMARY.md` - Phase 2 summary
- `MIGRATION_ROADMAP.md` - This document

### Code Infrastructure
- `include/utils/expected.h` - Result<T> implementation
- `include/utils/error_registry.h` - Error codes
- `src/utils/error_registry.cpp` - Error registration

### Error Codes Available
- `ERR_QUERY_PARSE_FAILED` (6100)
- `ERR_QUERY_INVALID_SYNTAX` (6101)
- `ERR_QUERY_EXECUTION_FAILED` (6102)
- `ERR_QUERY_TIMEOUT` (6103)
- `ERR_QUERY_CTE_CYCLE_DETECTED` (6104)
- `ERR_QUERY_AGGREGATION_FAILED` (6105)
- `ERR_QUERY_TYPE_MISMATCH` (6106)
- `ERR_QUERY_RESOURCE_EXHAUSTED` (6107)

---

## ✅ Definition of Done

Phase 4 Query Engine Migration is complete when:

**Code:**
- [ ] All 265 migration points converted to Result<T>
- [ ] All error codes properly used with context
- [ ] No compiler warnings
- [ ] Code reviews approved

**Testing:**
- [ ] All unit tests passing (100%)
- [ ] All integration tests passing (100%)
- [ ] Performance benchmarks < 5% regression
- [ ] All error codes tested
- [ ] Code coverage ≥ 85%

**Documentation:**
- [ ] Migration guide complete
- [ ] API documentation updated
- [ ] Error catalog complete
- [ ] Team training materials ready

**Deployment:**
- [ ] Staged rollout complete
- [ ] Production validation passed
- [ ] No P0/P1 bugs in 2 weeks
- [ ] Celebration! 🎉

---

**Last Updated:** 2026-01-20  
**Current Phase:** Phase 2 Complete ✅  
**Next Phase:** Phase 3 - AQL Translator (96 points)
