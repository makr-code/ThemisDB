# Graph Module Headers - Future Enhancements

## Scope

- API-level enhancements to `include/graph/` public C++ headers
- Parallel traversal API exposing work-stealing executor interface to callers
- Adaptive plan cache interface keyed by structural query hash
- Graph algorithm selection API via `IGraphAlgorithm` plug-in interface
- Subgraph query interface with depth-bounded extraction
- GPU-accelerated graph executor API (compile-time optional via `THEMIS_ENABLE_GPU`)
- Approximate algorithm API with configurable error bounds

## Design Constraints

- [ ] New graph algorithms plugged in exclusively via `IGraphAlgorithm` interface; no direct class coupling
- [ ] All traversal callbacks declared `noexcept`; exceptions must not escape traversal loops
- [ ] Plan cache keyed by structural hash of query graph; hash collision handled by equality check
- [ ] `maxDepth` parameter required on all traversal APIs; unbounded traversal not permitted
- [ ] GPU executor API guarded by `THEMIS_ENABLE_GPU` compile-time feature flag
- [ ] All `Result<T>` return paths propagate error codes without exposing internal state

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IGraphAlgorithm` | `ParallelGraphExecutor`, `GPUGraphExecutor` | Plug-in point for new algorithms |
| `ParallelGraphExecutor` | Query engine, benchmarks | Work-stealing parallel BFS/DFS |
| `AdaptiveCostModel` | Query planner | Structural-hash-keyed plan cache |
| `GraphQueryRewriter` | Query optimizer | Rule-based query rewriting |
| `ApproximateGraphAlgorithms` | Analytics layer | Error-bounded approximate queries |
| `DistributedGraphManager` | Cluster coordinator | Cross-partition graph queries |

## Planned Header Additions

### graph_parallel_executor.h
**Priority:** High
**Target Version:** v1.7.0

Header for parallel graph execution capabilities.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

class ParallelGraphExecutor {
public:
    struct Config {
        size_t num_threads = 8;
        size_t batch_size = 1000;
        bool enable_work_stealing = true;
        size_t queue_capacity = 10000;
    };

    explicit ParallelGraphExecutor(
        GraphIndexManager& graph_mgr,
        const Config& config = {}
    );

    // Parallel BFS with work-stealing
    Result<std::vector<std::string>> parallelBFS(
        std::string_view start,
        int max_depth,
        const QueryConstraints& constraints
    );

    // Parallel pattern matching
    Result<std::vector<PatternMatch>> parallelPatternMatch(
        const PatternQuery& pattern,
        const QueryConstraints& constraints
    );

    // Parallel subgraph extraction
    Result<Subgraph> parallelExtractSubgraph(
        std::string_view center,
        int radius,
        size_t max_nodes
    );

    // Thread pool management
    void setNumThreads(size_t num_threads);
    size_t getNumThreads() const;

    // Performance metrics
    struct ParallelStats {
        size_t threads_used;
        double parallel_efficiency;  // 0.0-1.0
        size_t work_steals;
        double load_balance_factor;  // 0.0-1.0 (1.0 = perfect)
    };

    ParallelStats getLastExecutionStats() const;
};

} // namespace graph
} // namespace themis
```

**Benefits:**
- 40-60% faster execution for large graphs
- Better CPU utilization
- Scalable to many cores

---

### graph_cost_model.h
**Priority:** High
**Target Version:** v1.7.0

Header for adaptive cost model with learning capabilities.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

class AdaptiveCostModel {
public:
    struct ModelConfig {
        double learning_rate = 0.1;
        size_t history_size = 1000;
        double decay_factor = 0.95;
        bool enable_auto_calibration = true;
    };

    explicit AdaptiveCostModel(const ModelConfig& config = {});

    // Cost estimation with confidence
    struct CostEstimate {
        double estimated_cost;
        double confidence;  // 0.0-1.0
        std::string algorithm_name;
    };

    CostEstimate estimateCost(
        TraversalAlgorithm algorithm,
        const GraphStatistics& stats,
        const QueryConstraints& constraints
    ) const;

    // Update model with actual execution
    void recordExecution(
        TraversalAlgorithm algorithm,
        const GraphStatistics& stats,
        const QueryConstraints& constraints,
        double actual_time_ms,
        size_t nodes_explored
    );

    // Model persistence
    Result<nlohmann::json> exportModel() const;
    Result<void> importModel(const nlohmann::json& model);

    // Model inspection
    double getModelAccuracy() const;
    size_t getTrainingSamples() const;
    std::map<TraversalAlgorithm, double> getAlgorithmWeights() const;
};

} // namespace graph
} // namespace themis
```

**Benefits:**
- Self-tuning cost estimates
- Improved plan selection over time
- Reduced manual tuning

---

### graph_distributed.h
**Priority:** Medium
**Target Version:** v1.8.0

Header for distributed graph query execution.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

enum class PartitionStrategy {
    HASH,           // Hash-based partitioning
    RANGE,          // Range partitioning
    GEOGRAPHIC,     // Geographic partitioning
    COMMUNITY,      // Community-based partitioning
    HYBRID          // Hybrid strategy
};

enum class ConsistencyLevel {
    STRONG,         // Strong consistency
    EVENTUAL,       // Eventual consistency
    BOUNDED_STALENESS  // Bounded staleness
};

class DistributedGraphManager {
public:
    struct Config {
        PartitionStrategy partitioning;
        size_t num_partitions;
        size_t replication_factor = 3;
        ConsistencyLevel consistency = ConsistencyLevel::EVENTUAL;
        bool enable_cross_partition_cache = true;
    };

    DistributedGraphManager(
        ClusterManager& cluster,
        const Config& config
    );

    // Distributed shortest path
    Result<PathResult> shortestPath(
        std::string_view start,
        std::string_view target,
        const QueryConstraints& constraints
    );

    // Distributed BFS
    Result<std::vector<std::string>> distributedBFS(
        std::string_view start,
        int max_depth,
        const QueryConstraints& constraints
    );

    // Distributed PageRank
    Result<std::map<std::string, double>> distributedPageRank(
        double damping = 0.85,
        int max_iterations = 100,
        double tolerance = 1e-6
    );

    // Partition management
    Result<size_t> getPartitionForNode(std::string_view node_id) const;
    Result<std::vector<std::string>> getNodesInPartition(size_t partition) const;

    // Cross-partition statistics
    struct DistributedStats {
        size_t total_partitions;
        size_t cross_partition_edges;
        double partition_balance;  // 0.0-1.0
        size_t replication_overhead;
    };

    DistributedStats getStats() const;
};

} // namespace graph
} // namespace themis
```

**Benefits:**
- Scale to billion-edge graphs
- Geographic distribution
- High availability

---

### graph_gpu_executor.h
**Priority:** Medium
**Target Version:** v1.9.0

Header for GPU-accelerated graph processing.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

class GPUGraphExecutor {
public:
    struct Config {
        int device_id = 0;
        size_t memory_limit_mb = 4096;
        bool enable_pinned_memory = true;
        bool enable_unified_memory = false;
    };

    explicit GPUGraphExecutor(
        GraphIndexManager& graph_mgr,
        const Config& config = {}
    );

    // Check GPU availability
    static bool isGPUAvailable();
    static std::vector<GPUInfo> getAvailableGPUs();

    // GPU-accelerated BFS
    Result<std::vector<std::string>> gpuBFS(
        std::string_view start,
        int max_depth
    );

    // GPU-accelerated PageRank
    Result<std::map<std::string, double>> gpuPageRank(
        double damping = 0.85,
        int max_iterations = 100
    );

    // GPU-accelerated betweenness
    Result<std::map<std::string, double>> gpuBetweenness();

    // Hybrid CPU-GPU execution
    Result<PathResult> hybridShortestPath(
        std::string_view start,
        std::string_view target,
        double gpu_threshold = 0.7  // Use GPU if graph > 70% of GPU memory
    );

    // GPU memory management
    void transferToGPU();
    void releaseGPUMemory();
    size_t getGPUMemoryUsed() const;
    size_t getGPUMemoryAvailable() const;
};

struct GPUInfo {
    int device_id;
    std::string name;
    size_t total_memory_mb;
    size_t available_memory_mb;
    int compute_capability_major;
    int compute_capability_minor;
};

} // namespace graph
} // namespace themis
```

**Benefits:**
- 10-100x speedup for large graphs
- Real-time analytics
- Reduced costs

---

### graph_constraints_advanced.h
**Priority:** Medium
**Target Version:** v1.7.0

Extended constraint types for advanced path finding.

**Planned Additions:**
```cpp
namespace themis {
namespace graph {

// Extend PathConstraints class
class PathConstraints {
public:
    // ... existing methods ...

    // Temporal constraints
    void addTemporalConstraint(
        int64_t start_time_ms,
        int64_t end_time_ms,
        TemporalMode mode = TemporalMode::VALID_DURING
    );

    // Weight constraints
    void addMaxWeight(double max_weight);
    void addMinWeight(double min_weight);

    // Resource constraints
    void addResourceCapacity(
        std::string_view resource_name,
        double capacity
    );

    // Geo-fence constraints
    void addGeoFence(
        double center_lat,
        double center_lon,
        double radius_km,
        GeoFenceMode mode = GeoFenceMode::MUST_STAY_INSIDE
    );

    // Semantic constraints
    void addSemanticRule(
        const Ontology& ontology,
        std::string_view rule
    );

    // Probability constraints (for uncertain graphs)
    void addMinProbability(double min_prob);
};

enum class TemporalMode {
    VALID_AT,       // Valid at specific time
    VALID_DURING,   // Valid during time range
    VALID_BEFORE,   // Valid before time
    VALID_AFTER     // Valid after time
};

enum class GeoFenceMode {
    MUST_STAY_INSIDE,      // All nodes inside fence
    MUST_PASS_THROUGH,     // At least one node inside
    MUST_NOT_ENTER         // No nodes inside
};

struct ResourceConstraint {
    std::string resource_name;
    double capacity;
    std::function<double(const std::string&)> usage_fn;
};

} // namespace graph
} // namespace themis
```

---

### graph_query_rewriter.h
**Priority:** Medium
**Target Version:** v1.8.0

Header for automatic query rewriting and optimization.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

class GraphQueryRewriter {
public:
    enum class RewriteRule {
        PREDICATE_PUSHDOWN,        // Push predicates into traversal
        COMMON_SUBEXPRESSION,      // Eliminate common subexpressions
        JOIN_REORDERING,           // Reorder joins for efficiency
        MATERIALIZED_VIEW,         // Use materialized views
        QUERY_DECOMPOSITION,       // Decompose for parallelism
        PRUNE_EARLY                // Early pruning of branches
    };

    struct RewriteConfig {
        std::set<RewriteRule> enabled_rules;
        bool aggressive_optimization = false;
        double rewrite_time_limit_ms = 100.0;
    };

    explicit GraphQueryRewriter(const RewriteConfig& config = {});

    // Rewrite query for better performance
    Result<GraphQuery> rewrite(const GraphQuery& original) const;

    // Explain rewrite transformations
    std::string explainRewrites(
        const GraphQuery& original,
        const GraphQuery& rewritten
    ) const;

    // Estimate improvement
    double estimateSpeedup(
        const GraphQuery& original,
        const GraphQuery& rewritten
    ) const;

    // Add custom rewrite rule
    void addCustomRule(
        std::string_view name,
        std::function<Result<GraphQuery>(const GraphQuery&)> rule
    );
};

} // namespace graph
} // namespace themis
```

---

### graph_approximate.h
**Priority:** Low
**Target Version:** v2.0.0

Header for approximate graph algorithms.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

enum class ApproximationMode {
    EXACT,      // Exact computation (no approximation)
    FAST,       // Fast approximation (5-10% error)
    FASTER,     // Faster approximation (10-20% error)
    FASTEST     // Fastest approximation (20-30% error)
};

class ApproximateGraphAlgorithms {
public:
    struct Config {
        ApproximationMode mode = ApproximationMode::FAST;
        double error_tolerance = 0.05;  // 5% error
        size_t sample_size = 1000;
        bool compute_error_bounds = true;
    };

    explicit ApproximateGraphAlgorithms(
        GraphIndexManager& graph_mgr,
        const Config& config = {}
    );

    // Approximate shortest path
    struct ApproximatePathResult {
        std::vector<std::string> path;
        double cost;
        double error_bound;      // Upper bound on approximation error
        double confidence;       // Confidence in result
    };

    Result<ApproximatePathResult> approximateShortestPath(
        std::string_view start,
        std::string_view target
    );

    // Approximate PageRank
    Result<std::map<std::string, double>> approximatePageRank(
        int max_iterations = 10
    );

    // Approximate reachability
    Result<bool> approximateReachability(
        std::string_view start,
        std::string_view target,
        double confidence = 0.95
    );

    // Approximate community detection
    Result<std::map<std::string, int>> approximateCommunities();
};

} // namespace graph
} // namespace themis
```

---

### graph_ml.h
**Priority:** Low
**Target Version:** v2.0.0

Header for graph machine learning integration.

**Planned Classes:**
```cpp
namespace themis {
namespace graph {

enum class EmbeddingAlgorithm {
    NODE2VEC,       // Random walk-based
    DEEPWALK,       // Deep walk
    GRAPHSAGE,      // GraphSAGE
    GCN             // Graph Convolutional Network
};

class GraphEmbedding {
public:
    struct TrainingConfig {
        EmbeddingAlgorithm algorithm;
        size_t dimensions = 128;
        size_t walk_length = 80;
        size_t num_walks = 10;
        double learning_rate = 0.025;
        size_t epochs = 5;
    };

    GraphEmbedding(
        GraphIndexManager& graph_mgr,
        const TrainingConfig& config
    );

    // Train embeddings
    Result<void> train();

    // Get node embedding
    Result<std::vector<double>> getNodeEmbedding(
        std::string_view node_id
    ) const;

    // Link prediction
    struct LinkPrediction {
        std::string from_node;
        std::string to_node;
        double probability;
    };

    Result<std::vector<LinkPrediction>> predictLinks(
        std::string_view node_id,
        size_t top_k = 10
    ) const;

    // Node classification
    Result<std::string> classifyNode(
        std::string_view node_id,
        const ClassificationModel& model
    ) const;

    // Similarity search
    Result<std::vector<std::string>> findSimilarNodes(
        std::string_view node_id,
        size_t top_k = 10
    ) const;

    // Persistence
    Result<void> saveModel(std::string_view path) const;
    Result<void> loadModel(std::string_view path);
};

class GraphNeuralNetwork {
public:
    // GNN inference for graph classification, node classification, link prediction
    // Implementation TBD
};

} // namespace graph
} // namespace themis
```

---

## API Stability Guarantees

**Stable APIs (v1.5.0+):**
- `graph_query_optimizer.h`: Core API stable, may add new methods
- `path_constraints.h`: Core API stable, may add new constraint types

**Beta APIs (v1.7.0+):**
- `graph_parallel_executor.h`: API may change based on feedback
- `graph_cost_model.h`: API may change based on feedback
- `graph_constraints_advanced.h`: Extended constraints API in flux

**Experimental APIs (v2.0.0+):**
- `graph_distributed.h`: API highly experimental
- `graph_gpu_executor.h`: API highly experimental
- `graph_approximate.h`: API highly experimental
- `graph_ml.h`: API highly experimental

## Deprecation Policy

- Deprecated APIs will be marked with `[[deprecated]]` attribute
- Deprecated APIs will be maintained for at least 2 major versions
- Migration guides will be provided for all breaking changes
- Compile-time warnings will be issued for deprecated usage

---

*Last Updated: April 2026*
*Next Review: Q3 2026*

---

## Test Strategy

- Unit tests for `IGraphAlgorithm` contract: each implementation must pass a compliance suite
- Property-based tests for `AdaptiveCostModel`: cost estimates monotonically improve with more training samples
- Integration tests verifying `maxDepth` enforcement stops traversal at the correct depth
- Header-only compilation tests: every planned header must compile in isolation with no `src/` dependency
- Regression tests confirming `noexcept` traversal callbacks do not propagate exceptions to callers
- Benchmark tests measuring plan cache lookup latency against the ≤ 500 ns target

## Performance Targets

- Plan cache lookup: ≤ 500 ns per query (structural hash hit path)
- Traversal API dispatch overhead: ≤ 1 µs per hop
- `ParallelGraphExecutor` parallel efficiency: ≥ 70% on 8-core hardware for graphs > 100K nodes
- `AdaptiveCostModel::estimateCost`: ≤ 2 µs including confidence interval computation
- GPU BFS kernel launch overhead: ≤ 100 µs amortized over a batch of ≥ 10K nodes
- Approximate shortest path: ≤ 5% error at `ApproximationMode::FAST` with ≥ 10× latency reduction vs exact

## Security / Reliability

- `maxDepth` parameter validated ≥ 1 and ≤ configurable system limit at API entry; violation returned as `Error::InvalidArgument` via `Result<T>`
- No unbounded traversal APIs exposed in public headers; all traversal signatures require an explicit depth bound
- `IGraphAlgorithm` implementations are sandboxed from internal storage: no direct RocksDB access via this interface
- `DistributedGraphManager` partition metadata is read-only from the public header API
- Graph query depth limit defaults to 32; overriding to a higher value requires explicit opt-in via `Config`
- All `Result<T>` return paths propagate error codes without exposing internal engine state
