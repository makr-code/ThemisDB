### Context

This issue implements the roadmap item 'Adaptive Query Compilation' for the performance domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Adaptive Query Compilation

### Goal

Deliver the scoped changes for Adaptive Query Compilation in src/performance/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Adaptive Query Compilation
**Priority:** High  
**Target Version:** v1.8.0  
**Research Basis:** "How to Architect a Query Compiler, Revisited" (SIGMOD'18)

JIT compilation of hot queries to native machine code for order-of-magnitude performance improvements.

**Features:**
- **LLVM Backend**: Generate optimized machine code
- **Hot Query Detection**: Identify frequently executed queries (>100 times)
- **Type Specialization**: Generate type-specific code paths
- **Expression Folding**: Constant propagation and dead code elimination
- **Vectorized Codegen**: SIMD instructions for batch processing
- **Adaptive Recompilation**: Recompile based on runtime statistics

**Architecture:**
```cpp
class AdaptiveQueryCompiler {
public:
    struct CompilationConfig {
        size_t hot_threshold = 100;           // Executions before JIT
        OptLevel optimization = O3;            // LLVM opt level
        bool enable_vectorization = true;      // SIMD codegen
        bool enable_prefetch = true;           // Software prefetch
        bool enable_inlining = true;           // Function inlining
        size_t compilation_timeout_ms = 100;   // Max compile time
    };
    
    struct CompiledQuery {
        using ExecuteFn = std::function<Result<QueryResult>(const QueryParams&)>;
        
        ExecuteFn execute;
        uint64_t compilation_time_us;
        uint64_t code_size_bytes;
        std::string llvm_ir;  // For debugging
        std::string assembly;  // For debugging
    };
    
    // Compile query to native code
    Result<CompiledQuery> compile(
        const ParsedQuery& query,
        const Schema& schema,
        CompilationConfig config = {});
    
    // Execute compiled query
    Result<QueryResult> execute(
        const CompiledQuery& compiled,
        const QueryParams& params);
    
    // Check if query is eligible for compilation
    bool is_compilable(const ParsedQuery& query) const;
    
    // Get compilation statistics
    struct CompilationStats {
        size_t queries_compiled;
        size_t compilation_failures;
        uint64_t total_compilation_time_us;
        uint64_t average_speedup_percent;
    };
    
    CompilationStats get_stats() const;
};

// Example usage
AdaptiveQueryCompiler compiler;

// First execution: interpreted
for (int i = 0; i < 150; i++) {
    auto result = execute_query(query);
    
    // After 100 executions, automatically compiles
    if (i == 100) {
        // Now running compiled version
        // 5-10x faster execution
    }
}

// Manual compilation
if (compiler.is_compilable(query)) {
    auto compiled = compiler.compile(query, schema, {
        .optimization = OptLevel::O3,
        .enable_vectorization = true
    });
    
    // Subsequent executions use compiled version
    auto result = compiler.execute(compiled.value(), params);
}
```

**Performance Targets:**
- **Simple filters**: 10x speedup
- **Aggregations**: 5-8x speedup
- **Joins**: 3-5x speedup
- **Complex expressions**: 8-15x speedup
- **Compilation time**: <100ms per query

**Implementation Strategy:**
1. Build LLVM IR generator for query operators
2. Implement type specialization for common types
3. Add vectorization for batch operations
4. Implement hot query detection and caching
5. Add adaptive recompilation based on cardinality

**Validation:**
- Benchmark against interpreted execution
- Verify correctness with differential testing
- Measure compilation overhead vs. execution savings

---

### Acceptance Criteria

- [ ] **LLVM Backend**: Generate optimized machine code
- [ ] **Hot Query Detection**: Identify frequently executed queries (>100 times)
- [ ] **Type Specialization**: Generate type-specific code paths
- [ ] **Expression Folding**: Constant propagation and dead code elimination
- [ ] **Vectorized Codegen**: SIMD instructions for batch processing
- [ ] **Adaptive Recompilation**: Recompile based on runtime statistics
- [ ] **Simple filters**: 10x speedup
- [ ] **Aggregations**: 5-8x speedup
- [ ] **Joins**: 3-5x speedup
- [ ] **Complex expressions**: 8-15x speedup
- [ ] **Compilation time**: <100ms per query
- [ ] Build LLVM IR generator for query operators
- [ ] Implement type specialization for common types
- [ ] Add vectorization for batch operations
- [ ] Implement hot query detection and caching
- [ ] Add adaptive recompilation based on cardinality
- [ ] Benchmark against interpreted execution
- [ ] Verify correctness with differential testing
- [ ] Measure compilation overhead vs. execution savings

### Relationships

- Roadmap row: #86 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/performance/FUTURE_ENHANCEMENTS.md#adaptive-query-compilation
- Source key: roadmap:86:performance:v1.8.0:adaptive-query-compilation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:86:performance:v1.8.0:adaptive-query-compilation -->
<!-- roadmap-ref: row=86;module=performance;target=v1.8.0 -->
<!-- roadmap-detail: src/performance/FUTURE_ENHANCEMENTS.md#adaptive-query-compilation -->
