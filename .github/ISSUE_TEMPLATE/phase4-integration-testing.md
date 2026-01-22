---
name: Integration Testing for Phase 4 Migration
about: Comprehensive testing of Phase 4A-4C error handling migration
title: '[Testing] Phase 4 Migration - Integration & Performance Tests'
labels: testing, query-engine, error-handling, P1
assignees: ''
---

## 📋 Module: Integration Testing

**Priority:** P1 (High)  
**Estimated Effort:** 2-3 days  
**Complexity:** MEDIUM  
**Dependencies:** Phase 4A-4C must be merged

## 🎯 Objective

Validate the Phase 4 Query Engine Error Handling Migration (Phases 4A-4C) through comprehensive integration testing and performance benchmarking.

## 📊 Scope

### Testing Coverage

**Phase 4A: Expression Evaluator**
- [ ] Expression evaluation with valid data
- [ ] Expression evaluation with invalid data (type errors)
- [ ] Aggregation error handling (SUM, AVG, MIN, MAX)
- [ ] Sort comparator error handling
- [ ] Group key evaluation errors

**Phase 4B: Join Operations**
- [ ] Join execution with valid data
- [ ] Join execution with missing FOR clauses
- [ ] Group-by with missing COLLECT clause
- [ ] Subquery error propagation (scalar, IN, EXISTS)
- [ ] CTE join error handling

**Phase 4C: Query Planner**
- [ ] Optimized key execution
- [ ] Optimized entity execution
- [ ] Optimizer error conversion
- [ ] HTTP API error responses

## 🔧 Test Environment Setup

### Prerequisites
- [ ] RocksDB environment configured
- [ ] Full build from source
- [ ] Test database with sample data
- [ ] Benchmark query suite

### Build Commands
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DTHEMIS_BUILD_TESTS=ON
cmake --build . --parallel
```

### Test Data Setup
```bash
# Create test database
./themisdb --create-db test_phase4

# Load sample data
./themisdb --import-data tests/data/sample_queries.json
```

## 📝 Test Cases

### 1. Expression Evaluation Tests

#### Test 1.1: Valid Expressions
```sql
-- Should succeed
FOR doc IN users
RETURN {
    fullName: CONCAT(doc.firstName, " ", doc.lastName),
    age: doc.age,
    isAdult: doc.age >= 18
}
```

**Expected:** All expressions evaluate successfully

#### Test 1.2: Type Mismatch Errors
```sql
-- Should handle gracefully
FOR doc IN users
RETURN {
    sum: doc.name + doc.age  -- String + Number
}
```

**Expected:** Error logged, document skipped or null returned

#### Test 1.3: Division by Zero
```sql
-- Should handle gracefully
FOR doc IN metrics
LET ratio = doc.value / doc.count
RETURN ratio
```

**Expected:** Error logged for documents with count=0

### 2. Join Operation Tests

#### Test 2.1: Valid Join
```sql
FOR u IN users
FOR o IN orders
FILTER u.id == o.userId
RETURN {user: u.name, order: o.id}
```

**Expected:** Join executes successfully

#### Test 2.2: Empty FOR Clause
```sql
-- Invalid query
RETURN {test: "value"}
```

**Expected:** Error returned with message "No FOR clauses provided"

#### Test 2.3: Subquery Error Propagation
```sql
FOR doc IN collection
FILTER doc.value IN (
    FOR sub IN invalid_collection
    RETURN sub.id
)
RETURN doc
```

**Expected:** Subquery error propagated to parent query

### 3. Query Optimizer Tests

#### Test 3.1: Optimized Query Execution
```sql
FOR doc IN users
FILTER doc.city == "Berlin"
FILTER doc.age > 18
RETURN doc
```

**Expected:** Optimizer chooses correct index order

#### Test 3.2: Optimizer Error Handling
```sql
-- Query with non-existent index
FOR doc IN users
FILTER doc.nonIndexedColumn == "value"
RETURN doc
```

**Expected:** Falls back to sequential scan with proper error logging

### 4. Error Message Quality Tests

#### Test 4.1: Expression Error Messages
- [ ] Verify error messages include expression context
- [ ] Verify error messages include variable bindings
- [ ] Verify error codes are correct

#### Test 4.2: Join Error Messages
- [ ] Verify error messages include join context
- [ ] Verify error messages include collection names
- [ ] Verify full error propagation chain

## 🚀 Performance Benchmarks

### Benchmark Suite

#### Benchmark 1: Simple Queries (Baseline)
```sql
FOR doc IN users FILTER doc.id == "123" RETURN doc
```
**Target:** < 1% overhead vs. pre-migration

#### Benchmark 2: Complex Expressions
```sql
FOR doc IN users
RETURN {
    score: (doc.metric1 + doc.metric2) / 2,
    category: doc.category == "A" ? "Premium" : "Standard",
    valid: doc.age > 18 AND doc.verified == true
}
```
**Target:** < 2% overhead vs. pre-migration

#### Benchmark 3: Multi-way Joins
```sql
FOR u IN users
FOR o IN orders
FOR p IN products
FILTER u.id == o.userId AND o.productId == p.id
RETURN {user: u.name, product: p.name}
```
**Target:** < 1% overhead vs. pre-migration

#### Benchmark 4: Aggregations
```sql
FOR doc IN metrics
COLLECT category = doc.category
AGGREGATE sum = SUM(doc.value), avg = AVG(doc.value)
RETURN {category, sum, avg}
```
**Target:** < 2% overhead vs. pre-migration

### Performance Metrics to Collect
- [ ] Query execution time (p50, p95, p99)
- [ ] Memory usage
- [ ] CPU usage
- [ ] Error path overhead
- [ ] Success path overhead

## 🔍 Error Path Testing

### Error Scenarios
1. [ ] Type conversion errors in expressions
2. [ ] Missing collections in joins
3. [ ] Invalid index access
4. [ ] Resource exhaustion (large joins)
5. [ ] Malformed spatial queries
6. [ ] Invalid aggregation arguments

### Error Propagation
- [ ] Verify errors propagate correctly through expression tree
- [ ] Verify errors propagate correctly through join pipeline
- [ ] Verify errors propagate correctly through subqueries
- [ ] Verify HTTP API returns correct error codes

## 📊 Test Execution

### Unit Tests
```bash
cd build
ctest -R QueryEngine --verbose
```

### Integration Tests
```bash
cd build
ctest -R Integration_QueryEngine --verbose
```

### Performance Benchmarks
```bash
cd build
./benchmarks/query_benchmark --benchmark_filter=Expression
./benchmarks/query_benchmark --benchmark_filter=Join
./benchmarks/query_benchmark --benchmark_filter=Optimizer
```

## ✅ Acceptance Criteria

### Functionality
- [ ] All existing query engine tests pass
- [ ] New error scenario tests pass
- [ ] Error messages are descriptive and actionable
- [ ] Error codes are correct and consistent

### Performance
- [ ] Success path overhead < 1%
- [ ] Error path overhead acceptable (< 10%)
- [ ] No memory leaks detected
- [ ] No performance regressions in benchmarks

### Quality
- [ ] Code coverage maintained or improved
- [ ] No new compiler warnings
- [ ] No static analysis issues
- [ ] Documentation updated

## 📚 Resources

- **Test Infrastructure:** `tests/query/test_query_engine.cpp`
- **Benchmark Suite:** `benchmarks/query_benchmark.cpp`
- **Sample Queries:** `tests/data/sample_queries.json`
- **Migration Docs:** `PHASE4_FINAL_SUMMARY.md`

## 🔗 Related Issues

- Depends on: Phase 4A-4C merge
- Related to: Phase 4D (if performance issues found)

## 💡 Notes

- Run tests in both Debug and Release modes
- Test with AddressSanitizer (ASAN) for memory issues
- Test with ThreadSanitizer (TSAN) for concurrency issues
- Compare results with pre-migration baseline
- Document any performance anomalies
