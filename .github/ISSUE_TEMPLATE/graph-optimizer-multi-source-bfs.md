---
name: ⚡ Performance: Graph Query Optimizer - Multi-Source BFS
about: Implement parallel BFS from multiple source vertices for improved performance
title: "[GRAPH-OPTIMIZER] Multi-Source Parallel BFS"
labels: priority:P2, type:performance, area:graph, effort:medium, phase:optimization
assignees: ''
---

## ⚡ Performance Enhancement - Graph Query Optimizer

**Current Status:** Single-source BFS only, no multi-source support  
**Priority:** P2 (Medium)  
**Effort:** 2-3 weeks  
**Target Version:** v1.5.0  
**Related Files:**
- `include/graph/graph_query_optimizer.h`
- `src/graph/graph_query_optimizer.cpp`
- `tests/test_graph_query_optimizer.cpp`
- `benchmarks/bench_graph_query_optimizer.cpp`

---

## 📋 Problem Description

Current BFS implementation supports only single source vertex traversal. Many graph queries benefit from multi-source BFS:
- **Community detection:** Find all vertices within k hops of multiple seeds
- **Influence propagation:** Track spread from multiple initial nodes
- **Reachability queries:** Check if any of multiple sources reaches target
- **Batch operations:** Process multiple BFS queries simultaneously

**Performance Impact:** N separate BFS calls vs 1 multi-source BFS = ~N×(setup overhead + redundant exploration)

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **Multi-Source BFS API**
  
  ```cpp
  // Execute BFS from multiple sources simultaneously
  Result<std::unordered_map<std::string, std::vector<std::string>>> executeMultiSourceBFS(
      const std::vector<std::string>& source_vertices,
      int max_depth,
      const QueryConstraints& constraints = {},
      ExecutionStats* stats = nullptr
  );
  // Returns: map of source -> reachable vertices
  
  // Find vertices reachable from ANY source
  Result<std::vector<std::string>> executeUnifiedBFS(
      const std::vector<std::string>& source_vertices,
      int max_depth,
      const QueryConstraints& constraints = {},
      ExecutionStats* stats = nullptr
  );
  ```

- [ ] **Distance Tracking**
  
  Track which source reached each vertex first and at what distance:
  
  ```cpp
  struct MultiSourceResult {
      std::unordered_map<std::string, std::string> vertex_to_source;  // vertex -> closest source
      std::unordered_map<std::string, int> vertex_to_distance;        // vertex -> distance from source
      std::unordered_map<std::string, std::vector<std::string>> source_to_vertices;  // source -> reachable
  };
  
  Result<MultiSourceResult> executeMultiSourceBFSDetailed(
      const std::vector<std::string>& source_vertices,
      int max_depth,
      const QueryConstraints& constraints = {}
  );
  ```

- [ ] **Parallel Execution**
  
  Process multiple sources in parallel using thread pool:
  - Shared visited set with atomic operations
  - Lock-free frontier queues per thread
  - Work stealing for load balancing

- [ ] **Early Termination**
  
  Stop when all sources reach specific targets:
  
  ```cpp
  // Find shortest paths from any source to any target
  Result<std::vector<GraphIndexManager::PathResult>> executeMultiSourceToTargets(
      const std::vector<std::string>& sources,
      const std::vector<std::string>& targets,
      bool stop_on_first_reach = false
  );
  ```

### Nice to Have (P3)

- [ ] **Source Priority/Weights**
  
  ```cpp
  struct SourceConfig {
      std::string vertex;
      double priority = 1.0;     // Higher priority explored first
      int max_depth_override = -1;  // Per-source depth limit
  };
  
  Result<MultiSourceResult> executeWeightedMultiSourceBFS(
      const std::vector<SourceConfig>& sources,
      int default_max_depth
  );
  ```

- [ ] **Frontier Merging**
  
  Detect when frontiers from different sources meet:
  - Useful for finding meeting points
  - Can enable multi-source bidirectional search

- [ ] **Progressive Results**
  
  Stream results as frontiers expand:
  
  ```cpp
  using ProgressCallback = std::function<void(
      const std::string& source,
      int current_depth,
      const std::vector<std::string>& new_vertices
  )>;
  
  Result<MultiSourceResult> executeMultiSourceBFSProgressive(
      const std::vector<std::string>& sources,
      int max_depth,
      ProgressCallback callback
  );
  ```

---

## 📐 Technical Design

### Algorithm

**Sequential Multi-Source (baseline):**
```
visited = {}
source_map = {}
distance_map = {}

for each source in sources:
    queue = [(source, 0)]
    
    while queue not empty:
        vertex, depth = queue.pop()
        
        if vertex in visited:
            continue
        
        visited.add(vertex)
        source_map[vertex] = source
        distance_map[vertex] = depth
        
        if depth < max_depth:
            for neighbor in get_neighbors(vertex):
                if neighbor not in visited:
                    queue.push((neighbor, depth + 1))
```

**Parallel Multi-Source (optimized):**
```
global_visited = atomic_set()
results = concurrent_map[source -> vertices]

parallel for each source in sources:
    local_queue = [(source, 0)]
    local_visited = {}
    
    while local_queue not empty:
        vertex, depth = local_queue.pop()
        
        // Atomic check-and-set
        if not global_visited.try_insert(vertex):
            continue
        
        local_visited.add(vertex)
        results[source].push_back(vertex)
        
        if depth < max_depth:
            neighbors = get_neighbors(vertex)
            for neighbor in neighbors:
                if neighbor not in global_visited:
                    local_queue.push((neighbor, depth + 1))
```

### Data Structures

```cpp
class GraphQueryOptimizer {
public:
    // New methods
    Result<MultiSourceResult> executeMultiSourceBFS(...);
    Result<std::vector<std::string>> executeUnifiedBFS(...);
    
private:
    // Parallel execution helpers
    struct ThreadLocalState {
        std::queue<std::pair<std::string, int>> frontier;
        std::vector<std::string> discovered;
        size_t nodes_explored = 0;
    };
    
    void processSourceBFS(
        const std::string& source,
        int max_depth,
        std::atomic<bool>& should_stop,
        std::unordered_set<std::string>& global_visited,
        std::mutex& visited_mutex,
        ThreadLocalState& local_state
    );
};
```

---

## ✅ Acceptance Criteria

- [ ] Multi-source BFS correctly explores from all sources
- [ ] Distance tracking accurate (matches single-source results)
- [ ] No duplicate exploration (visited set properly synchronized)
- [ ] Parallel version faster than N sequential BFS for N sources
- [ ] Works with all existing constraints (forbidden vertices, max results, etc.)
- [ ] Unit tests for correctness (compare with sequential baseline)
- [ ] Performance benchmarks showing speedup vs sequential
- [ ] Documentation with usage examples

---

## 🧪 Testing Strategy

### Unit Tests

```cpp
TEST_F(GraphQueryOptimizerTest, MultiSourceBFS_ReachesAllVertices) {
    std::vector<std::string> sources = {"A", "B", "C"};
    
    auto result = optimizer_->executeMultiSourceBFS(sources, 2);
    ASSERT_TRUE(result);
    
    const auto& source_map = result.value();
    
    // Each source should reach its neighbors
    EXPECT_GT(source_map["A"].size(), 0);
    EXPECT_GT(source_map["B"].size(), 0);
    EXPECT_GT(source_map["C"].size(), 0);
}

TEST_F(GraphQueryOptimizerTest, MultiSourceBFS_CorrectDistances) {
    auto detailed = optimizer_->executeMultiSourceBFSDetailed({"A"}, 3);
    ASSERT_TRUE(detailed);
    
    // Verify distances match single-source BFS
    auto single = optimizer_->executeBFS("A", 3);
    ASSERT_TRUE(single);
    
    // All vertices in single should be in detailed with correct distance
    // (detailed test implementation)
}
```

### Benchmarks

```cpp
BENCHMARK_F(GraphQueryOptimizerBench, MultiSourceBFS_vs_Sequential) {
    std::vector<std::string> sources;
    for (int i = 0; i < 10; ++i) {
        sources.push_back("node_" + std::to_string(i));
    }
    
    // Multi-source (should be faster)
    benchmark::DoNotOptimize(
        optimizer_->executeMultiSourceBFS(sources, 3)
    );
}
```

---

## 📊 Success Metrics

- **Speedup:** 3-5× faster than N sequential BFS for N=10 sources
- **Scalability:** Linear speedup up to number of CPU cores
- **Memory:** ≤ 1.5× memory usage vs single-source
- **Correctness:** 100% agreement with sequential baseline

---

## 🔗 Related Issues

- Depends on: Graph Query Engine Optimization (completed)
- Enables: Community detection algorithms
- Related: Parallel traversal hints (can share thread pool)

---

## 📝 Implementation Notes

### Phase 1: Sequential Multi-Source
- Basic API and data structures
- Sequential execution (no parallelism yet)
- Correctness validation

### Phase 2: Parallel Execution
- Thread pool integration
- Lock-free synchronization
- Performance optimization

### Phase 3: Advanced Features
- Source priorities and weights
- Progressive results streaming
- Frontier merging detection

---

## 🎓 References

- **Parallel BFS:** Bader, D. A., & Madduri, K. (2006). "Designing Multithreaded Algorithms for BFS"
- **Graph500 Benchmark:** BFS-based graph benchmark
- **PBFS:** Xia, Y., & Prasanna, V. K. (2009). "Parallel Breadth First Search on GPU"
