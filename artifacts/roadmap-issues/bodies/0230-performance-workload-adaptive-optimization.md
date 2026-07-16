### Context

This issue implements the roadmap item 'Workload-Adaptive Optimization' for the performance domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: Workload-Adaptive Optimization

### Goal

Deliver the scoped changes for Workload-Adaptive Optimization in src/performance/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Workload-Adaptive Optimization
**Priority:** Medium  
**Target Version:** v1.9.0  
**Research Basis:** "Adaptive Execution" (SIGMOD'19)

Automatically adjust optimization strategies based on runtime workload characteristics.

**Features:**
- **Workload Classification**: OLTP, OLAP, mixed, graph, vector
- **Dynamic Strategy Selection**: Choose optimal algorithms per workload
- **Resource Reallocation**: Adjust memory, threads, cache based on load
- **Performance Feedback**: Continuously monitor and adapt
- **Predictive Scaling**: Anticipate workload changes

**Architecture:**
```cpp
class WorkloadAdaptiveOptimizer {
public:
    enum class WorkloadType {
        OLTP,           // High-concurrency, short transactions
        OLAP,           // Complex analytical queries
        MIXED,          // Both OLTP and OLAP
        GRAPH,          // Graph traversal and analytics
        VECTOR,         // Vector similarity search
        TIMESERIES,     // Time-series queries
        UNKNOWN
    };
    
    struct WorkloadProfile {
        WorkloadType type;
        double read_write_ratio;
        double avg_query_complexity;
        size_t avg_result_size;
        size_t concurrent_queries;
        std::vector<std::string> hot_tables;
    };
    
    struct OptimizationStrategy {
        bool enable_jit_compilation;
        bool enable_parallel_execution;
        size_t thread_pool_size;
        size_t cache_size_mb;
        std::string join_algorithm;  // "hash", "sort-merge", "nested-loop"
        std::string index_type;      // "btree", "hash", "brin"
    };
    
    // Classify current workload
    WorkloadProfile classify_workload() const;
    
    // Get optimal strategy for workload
    OptimizationStrategy get_strategy(const WorkloadProfile& profile) const;
    
    // Apply strategy
    void apply_strategy(const OptimizationStrategy& strategy);
    
    // Automatic adaptation (runs in background)
    void enable_auto_adapt(std::chrono::seconds interval = 60s);
    void disable_auto_adapt();
};

// Example usage
WorkloadAdaptiveOptimizer optimizer;

// Manual adaptation
auto profile = optimizer.classify_workload();
auto strategy = optimizer.get_strategy(profile);
optimizer.apply_strategy(strategy);

// Automatic adaptation
optimizer.enable_auto_adapt(30s);  // Adapt every 30 seconds

// Monitor adaptation
optimizer.set_callback([](const WorkloadProfile& old_profile,
                          const WorkloadProfile& new_profile,
                          const OptimizationStrategy& strategy) {
    LOG(INFO) << "Workload changed: " << old_profile.type 
              << " -> " << new_profile.type;
    LOG(INFO) << "Applied strategy: threads=" << strategy.thread_pool_size
              << " cache_mb=" << strategy.cache_size_mb;
});
```

**Performance Targets:**
- **Adaptation latency**: <5 seconds
- **Overhead**: <1% CPU for monitoring
- **Improvement**: +20-50% vs. static configuration

---

### Acceptance Criteria

- [ ] **Workload Classification**: OLTP, OLAP, mixed, graph, vector
- [ ] **Dynamic Strategy Selection**: Choose optimal algorithms per workload
- [ ] **Resource Reallocation**: Adjust memory, threads, cache based on load
- [ ] **Performance Feedback**: Continuously monitor and adapt
- [ ] **Predictive Scaling**: Anticipate workload changes
- [ ] **Adaptation latency**: <5 seconds
- [ ] **Overhead**: <1% CPU for monitoring
- [ ] **Improvement**: +20-50% vs. static configuration

### Relationships

- Roadmap row: #230 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/performance/FUTURE_ENHANCEMENTS.md#workload-adaptive-optimization
- Source key: roadmap:230:performance:v1.9.0:workload-adaptive-optimization

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:230:performance:v1.9.0:workload-adaptive-optimization -->
<!-- roadmap-ref: row=230;module=performance;target=v1.9.0 -->
<!-- roadmap-detail: src/performance/FUTURE_ENHANCEMENTS.md#workload-adaptive-optimization -->
