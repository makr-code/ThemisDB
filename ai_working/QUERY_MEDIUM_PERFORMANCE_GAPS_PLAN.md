# Query Module MEDIUM Severity Performance Gaps - Implementation Plan

**Status**: In Progress  
**Date**: 2026-08-16  
**Branch**: develop  
**Baseline Metrics**: 4106 MEDIUM gaps across 4 categories

---

## Executive Summary

This document tracks the systematic remediation of **MEDIUM severity performance and stability gaps** in the ThemisDB query module:

1. **String Concatenation Loops** (61 instances): O(n²) string building
2. **Copy Overhead** (35 instances): Unnecessary object copies
3. **Exception Handling Gaps** (67 instances): Bare/generic catches without proper error context
4. **Other Issues** (23+28+55 instances): O(n²) algorithms, uninitialized access, unchecked results

---

## Phase 1: String Concatenation Loops (61 instances → <5)

### Objective
Replace all O(n²) string concatenation patterns with O(n) alternatives using:
- `std::string::reserve()` + `+=` operator (most common case)
- `std::stringstream` (complex builds, many small appends)
- `fmt::format()` (complex formatting)

### Files Identified
- `query_plan_visualizer.cpp`: String escaping in loop (good candidate, already has reserve())
- `aql_translator.cpp`: Field name extraction
- `cypher_parser.cpp`: Token/query processing
- `sql_parser.cpp`: SQL parsing and formatting

### Approach
1. Audit each file for loop-based `str += ...` patterns
2. Move string initialization outside loops
3. Pre-calculate final size where possible
4. Apply `.reserve()` before loops
5. Add micro-benchmarks validating 5-10x improvement

### Status: NOT STARTED

---

## Phase 2: Copy Overhead (35 instances → <10)

### Objective
Eliminate unnecessary object copies through:
- Pass-by-const-reference for read-only parameters
- Move semantics for returned objects (RVO + std::move)
- Container operations using `std::move_iterator`

### Categories to Address
- Function parameters: vectors, strings, complex objects
- Return values: Apply move or RVO
- Container insertions: Use `std::move` instead of copy
- Temporary materialization: Avoid copies in intermediate steps

### Approach
1. Audit function signatures for expensive-to-copy types
2. Change `const std::vector<T>&` for parameters that aren't modified
3. Apply `std::move()` to returns and container operations
4. Verify compilation and performance with profiling

### Status: NOT STARTED

---

## Phase 3: Exception Handling Gaps (67 instances → <20)

### Objective
Replace bare `catch(...)` and `catch(std::exception&)` with:
- Specific exception type handling
- Proper error logging with context
- Stack trace preservation
- Error re-throw with enriched diagnostics

### Gap Breakdown
- `catch_all_swallow`: 21 instances (bare catch that discards errors)
- `generic_catch`: 21 instances (catches std::exception without context)
- `uncaught_exception`: 25 instances (implicit/unchecked exception propagation)

### Files to Fix
- `aql_parser.cpp`: Multiple catch(...) blocks (lines 657, 709, 765, 970, 983, 1314, 1324, 2224)
- `aql_runner.cpp`: Exception handling gaps
- `cypher_parser.cpp`: Multiple parser exception handlers
- `cte_cache.cpp`: Bare catch blocks
- `sql_parser.cpp`: Parser exception handling

### Approach
1. Replace bare `catch(...)` with specific exception types
2. Add logging: `LOG(ERROR) << "Context: " << e.what();`
3. Re-throw or wrap in application-specific exceptions
4. Add test coverage for exception paths
5. Ensure strong exception guarantee where applicable

### Status: NOT STARTED

---

## Phase 4: Other MEDIUM Issues

### 4a: O(n²) Algorithms (23 instances)
- Identify nested loop patterns with repeated operations
- Replace with hash maps, sets, or sorted containers
- Target: nested query optimization, duplicates detection

### 4b: Uninitialized Access (28 instances)
- Initialize all variables at declaration
- Use meaningful default values
- Add assertions for invariants

### 4c: Unchecked Results (55 instances)
- Check all function return values
- Propagate errors up the call stack
- Use `Result<T>` or `std::optional<T>` consistently

### Status: NOT STARTED

---

## Implementation Checklist

### Pre-Implementation
- [x] Identify all issue locations and categorize
- [ ] Set up micro-benchmarks for string ops
- [ ] Set up profiling infrastructure

### Phase 1: String Concatenation (Target: 2.5 hours)
- [ ] Audit aql_translator.cpp
- [ ] Audit query_plan_visualizer.cpp
- [ ] Audit cypher_parser.cpp
- [ ] Audit sql_parser.cpp
- [ ] Add benchmark tests
- [ ] Verify 5-10x improvement

### Phase 2: Copy Overhead (Target: 2.5 hours)
- [ ] Audit all function signatures in query module
- [ ] Apply const-reference changes
- [ ] Apply move semantics
- [ ] Compile and profile
- [ ] Verify no performance regression

### Phase 3: Exception Handling (Target: 2 hours)
- [ ] Fix aql_parser.cpp catches
- [ ] Fix cypher_parser.cpp catches
- [ ] Fix cte_cache.cpp catches
- [ ] Add logging throughout
- [ ] Add test coverage for exception paths

### Phase 4: Other Issues (Target: 2 hours)
- [ ] Fix O(n²) algorithms
- [ ] Initialize all variables
- [ ] Check all return values

### Quality Gates
- [ ] All query tests pass (`ctest -L query`)
- [ ] Benchmark regression gates hold
- [ ] Static analysis <200 combined MEDIUM gaps (from 4106)
- [ ] Code review approval

---

## Testing Strategy

### Unit Tests
- String building correctness (output identical)
- Copy optimization (no observable behavior change)
- Exception handling (proper error propagation)

### Performance Tests
- String concatenation: 5-10x improvement
- Copy overhead: Measurable GC/allocation reduction
- Query execution time: 5-10% overall improvement

### Integration Tests
- All existing query tests pass
- Benchmark regression gates pass
- No new exceptions or crashes

---

## Rollback Plan

Each phase is independently committable and revertible:
1. Phase 1 commits tagged `fix/string-concat-[commit-hash]`
2. Phase 2 commits tagged `fix/copy-overhead-[commit-hash]`
3. Phase 3 commits tagged `fix/exception-handling-[commit-hash]`
4. Phase 4 commits tagged `fix/other-medium-[commit-hash]`

All phases preserve API contracts and only change internal implementation.

---

## Success Criteria

**Performance**:
- String concatenation: 5-10x improvement (measured)
- Overall query latency: 5-10% improvement (gate pass)

**Stability**:
- Zero new exceptions
- Zero new crashes
- All tests green

**Code Quality**:
- MEDIUM gaps: 4106 → <700 (80% reduction minimum)
- Exception handling: 67 → <20 (70% reduction)

---

## Dependencies

- C++17 compiler with full std::move support
- Existing fmt library for formatting
- Existing test infrastructure (ctest)
- Existing logging infrastructure

---

## Next Steps

1. Review and approve this plan
2. Begin Phase 1 (string concatenation)
3. Set up performance benchmarks
4. Execute phases sequentially
5. Document improvements in CHANGELOG
