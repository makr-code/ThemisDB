### Context

This issue implements the roadmap item '`CTESubquery`: Replace Phase 1 Stub' for the query domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: `CTESubquery`: Replace Phase 1 Stub

### Goal

Deliver the scoped changes for `CTESubquery`: Replace Phase 1 Stub in src/query/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### `CTESubquery`: Replace Phase 1 Stub
**Priority:** Medium
**Target Version:** v1.7.0

`cte_subquery.cpp` line 334 has: "Phase 1 stub: treat as scalar subquery; real behavior handled elsewhere". Correlated subqueries and EXISTS subqueries may be incorrectly evaluated as scalar, producing wrong results.

**Implementation Notes:**
- `[ ]` Implement correlated subquery evaluation: detect outer references in the subquery AST; evaluate subquery once per outer row with the correlated bindings.
- `[ ]` Implement `EXISTS`/`NOT EXISTS` short-circuit: stop iterating the subquery result as soon as one matching row is found.
- `[ ]` Add regression tests for correlated subqueries with outer reference in WHERE clause.

---


**Priority:** High  
**Target Version:** v1.8.0

Just-In-Time compilation of frequently executed queries to native code for 5-10x performance improvement.

**Features:**
- LLVM-based code generation
- Hot query detection (>100 executions)
- Type-specialized implementations
- Expression tree optimization
- Vectorized execution (SIMD)

**Architecture:**
```cpp
class QueryCompiler {
public:
    struct CompilerConfig {
        size_t hot_threshold = 100;           // Executions before compilation
        bool enable_simd = true;              // Use SIMD instructions
        bool enable_prefetch = true;          // Software prefetch
        OptimizationLevel opt_level = O3;     // LLVM optimization level
    };
    
    // Compile a parsed query to native code
    CompiledQuery compile(const ParsedQuery& query, 
                         const Schema& schema,
                         CompilerConfig config = {});
    
    // Execute compiled query
    Result<QueryResult> execute(const CompiledQuery& compiled,
                               const QueryParams& params);
};

// Example usage
QueryCompiler compiler;
auto compiled = compiler.compile(parsed_query, schema);

// 5-10x faster execution
for (int i = 0; i < 1000000; i++) {
    auto result = compiler.execute(compiled, params);
}
```

**Performance Targets:**
- Simple filters: 10x speedup
- Aggregations: 5x speedup
- Joins: 3-5x speedup
- Compilation time: <100ms

**Implementation Notes:**
- Cache compiled code to disk
- Version compatibility tracking
- Fallback to interpreted execution on compilation errors

---

### Acceptance Criteria

- [ ] Implement correlated subquery evaluation: detect outer references in the subquery AST; evaluate subquery once per outer row with the correlated bindings.
- [ ] Implement `EXISTS`/`NOT EXISTS` short-circuit: stop iterating the subquery result as soon as one matching row is found.
- [ ] Add regression tests for correlated subqueries with outer reference in WHERE clause.
- [ ] LLVM-based code generation
- [ ] Hot query detection (>100 executions)
- [ ] Type-specialized implementations
- [ ] Expression tree optimization
- [ ] Vectorized execution (SIMD)
- [ ] Simple filters: 10x speedup
- [ ] Aggregations: 5x speedup
- [ ] Joins: 3-5x speedup
- [ ] Compilation time: <100ms
- [ ] Cache compiled code to disk
- [ ] Version compatibility tracking
- [ ] Fallback to interpreted execution on compilation errors

### Relationships

- Roadmap row: #194 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/query/FUTURE_ENHANCEMENTS.md#ctesubquery-replace-phase-1-stub
- Source key: roadmap:194:query:v1.7.0:ctesubquery-replace-phase-1-stub

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:194:query:v1.7.0:ctesubquery-replace-phase-1-stub -->
<!-- roadmap-ref: row=194;module=query;target=v1.7.0 -->
<!-- roadmap-detail: src/query/FUTURE_ENHANCEMENTS.md#ctesubquery-replace-phase-1-stub -->
