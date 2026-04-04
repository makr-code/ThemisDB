# Adaptive & Distributed Query Optimizer Documentation

## Overview

ThemisDB's query optimizer has been enhanced with adaptive query execution, distributed optimization capabilities, multi-index intersection, and NUMA-aware execution. These features enable the database to learn from past query executions, optimize for distributed/sharded environments, and efficiently utilize modern hardware.

## Table of Contents

1. [Adaptive Query Execution](#adaptive-query-execution)
2. [Distributed Query Optimization](#distributed-query-optimization)
3. [Multi-Index Optimization](#multi-index-optimization)
4. [NUMA-Aware Execution](#numa-aware-execution)
5. [HNSW Production Defaults](#hnsw-production-defaults)
6. [Usage Examples](#usage-examples)
7. [Performance Tuning](#performance-tuning)

---

## Adaptive Query Execution

### Concepts

Adaptive query execution allows the optimizer to learn from historical query executions and adjust its behavior based on actual runtime statistics. This addresses the common problem of cardinality misestimation in traditional cost-based optimizers.

### Key Components

#### AdaptiveQueryStats

Tracks historical query execution statistics:
- Estimated vs actual row counts
- Execution times
- Per-operator statistics
- Cardinality misestimation detection

**Example Usage:**

```cpp
#include "query/adaptive_optimizer.h"

themis::AdaptiveQueryStats stats;

// Record query execution
themis::AdaptiveQueryStats::QueryExecution exec;
exec.query_hash = "query_abc123";
exec.estimated_rows = 1000;
exec.actual_rows = 5000;  // 5x underestimation
exec.execution_time_ms = 25.5;
exec.timestamp = std::chrono::system_clock::now();

stats.recordExecution(exec);

// Get adaptive adjustment factor
double adjustment = stats.getAdaptiveAdjustmentFactor("query_abc123");
// adjustment will be > 1.0 to account for consistent underestimation
```

#### AdaptivePlanSelector

Selects optimal execution plans based on historical performance:

```cpp
using themis::AdaptivePlanSelector;

AdaptivePlanSelector selector;

// Define alternative plans
std::vector<AdaptivePlanSelector::PlanChoice> alternatives;

AdaptivePlanSelector::PlanChoice index_scan;
index_scan.strategy = AdaptivePlanSelector::PlanChoice::Strategy::INDEX_SCAN;
index_scan.estimated_cost = 10.0;
alternatives.push_back(index_scan);

AdaptivePlanSelector::PlanChoice table_scan;
table_scan.strategy = AdaptivePlanSelector::PlanChoice::Strategy::TABLE_SCAN;
table_scan.estimated_cost = 50.0;
alternatives.push_back(table_scan);

// Select best plan with adaptive learning
auto selected = selector.selectPlan(alternatives, "query_hash", stats);
```

#### Runtime Plan Switching

Detect when cardinality estimates are significantly off during execution:

```cpp
// Check if plan should be switched mid-execution
bool should_switch = selector.shouldSwitchPlan(
    5000,    // rows_so_far
    1000,    // estimated_total
    0.50,    // progress (50% done)
    5.0      // misestimation_threshold
);

if (should_switch) {
    auto alternative = selector.getAlternativePlan(current_plan, 5000, 1000);
    // Switch to alternative plan
}
```

### Enabling Adaptive Optimization

```cpp
#include "query/query_optimizer.h"

themis::SecondaryIndexManager index_mgr(...);
themis::QueryOptimizer optimizer(index_mgr);

// Enable adaptive optimization
optimizer.enableAdaptiveOptimization(true);

// Record executions (integrate with query engine)
optimizer.recordQueryExecution(
    query_hash,
    estimated_rows,
    actual_rows,
    execution_time_ms
);

// Optimizer will automatically use adaptive adjustments
```

---

## Distributed Query Optimization

### Concepts

Distributed query optimization extends the cost model to account for network latency, data locality, and cross-shard communication costs. It enables efficient query execution across multiple shards/partitions.

### DistributedQueryCostModel

Estimates costs for distributed query execution:

```cpp
#include "query/adaptive_optimizer.h"

using themis::DistributedQueryCostModel;

DistributedQueryCostModel model;

// Define shard information
std::vector<DistributedQueryCostModel::ShardInfo> shards;

DistributedQueryCostModel::ShardInfo shard1;
shard1.shard_id = "shard_us_west";
shard1.estimated_rows = 100000;
shard1.is_local = true;
shard1.network_latency_ms = 0.0;

DistributedQueryCostModel::ShardInfo shard2;
shard2.shard_id = "shard_eu_central";
shard2.estimated_rows = 80000;
shard2.is_local = false;
shard2.network_latency_ms = 50.0;  // 50ms cross-region latency

shards.push_back(shard1);
shards.push_back(shard2);

// Estimate total cost
double cost = model.estimateDistributedQueryCost(shards, 5000);
```

### Cross-Shard Join Optimization

The optimizer automatically selects the best join strategy:

```cpp
auto join_cost = model.estimateCrossShardJoinCost(
    left_shard,
    right_shard,
    10000,   // left_rows
    100000   // right_rows
);

// Recommended strategies:
// - "broadcast": Small table is broadcast to all nodes
// - "repartition": Both tables are repartitioned by join key
// - "semi_join": Use semi-join to reduce network transfer

std::cout << "Strategy: " << join_cost.recommended_strategy << std::endl;
std::cout << "Network cost: " << join_cost.network_cost << "ms" << std::endl;
std::cout << "Compute cost: " << join_cost.compute_cost << "ms" << std::endl;
```

### Partition Pruning

Automatically prune unnecessary partitions:

```cpp
bool should_prune = model.shouldPrunePartition(
    shard,
    10,    // total_shards
    0.001  // selectivity (0.1%)
);

if (should_prune) {
    // Skip this shard - network cost exceeds benefit
}
```

### Query Optimizer Integration

```cpp
// Generate distributed execution plan
auto dist_plan = optimizer.optimizeForDistribution(
    query,
    {"shard1", "shard2", "shard3"},
    true  // enable_partition_pruning
);

std::cout << "Shards to query: " << dist_plan.shard_ids.size() << std::endl;
std::cout << "Partition pruning: " << dist_plan.use_partition_pruning << std::endl;
std::cout << "Join strategy: " << dist_plan.join_strategy << std::endl;
std::cout << "Parallelism: " << dist_plan.recommended_parallelism << std::endl;
```

---

## Multi-Index Optimization

### Concepts

Multi-index optimization allows queries to benefit from using multiple indexes simultaneously, including index intersection for AND queries.

### MultiIndexOptimizer

```cpp
#include "query/adaptive_optimizer.h"

using themis::MultiIndexOptimizer;

MultiIndexOptimizer optimizer;

// Define available indexes
std::vector<MultiIndexOptimizer::IndexCandidate> indexes;

MultiIndexOptimizer::IndexCandidate idx1;
idx1.index_name = "idx_status";
idx1.column = "status";
idx1.estimated_selectivity = 5000;  // 5% of 100k rows
idx1.access_cost = 1.0;

MultiIndexOptimizer::IndexCandidate idx2;
idx2.index_name = "idx_category";
idx2.column = "category";
idx2.estimated_selectivity = 10000;  // 10% of 100k rows
idx2.access_cost = 1.0;

indexes.push_back(idx1);
indexes.push_back(idx2);

// Optimize multi-index access
auto plan = optimizer.optimizeMultiIndexAccess(indexes, 100000);

std::cout << "Indexes to use: ";
for (const auto& idx : plan.indexes_to_use) {
    std::cout << idx << " ";
}
std::cout << std::endl;

std::cout << "Use bitmap intersection: " << plan.use_bitmap_intersection << std::endl;
std::cout << "Estimated rows: " << plan.estimated_result_rows << std::endl;
```

### Index Intersection

For queries with multiple predicates (AND conditions), the optimizer can use index intersection:

```sql
-- Query: WHERE status = 'active' AND category = 'premium'
-- Without intersection: Use most selective index (idx_status), then filter
-- With intersection: Use both indexes, intersect row IDs, then fetch rows
```

The optimizer automatically chooses between:
1. **Single index**: Use most selective index if it's very selective (< 1%)
2. **Index intersection**: Use multiple indexes and intersect if beneficial
3. **Bitmap intersection**: Use bitmap operations for very selective multi-index access

---

## NUMA-Aware Execution

### Concepts

NUMA (Non-Uniform Memory Access) architectures have multiple memory nodes with different access latencies. NUMA-aware execution places data and threads on appropriate nodes to minimize memory access latency.

### NumaAwareOptimizer

```cpp
#include "query/adaptive_optimizer.h"

using themis::NumaAwareOptimizer;

NumaAwareOptimizer numa_optimizer;

// Check if NUMA is available
if (NumaAwareOptimizer::isNumaAvailable()) {
    size_t num_nodes = NumaAwareOptimizer::getNumaNodeCount();
    std::cout << "NUMA nodes: " << num_nodes << std::endl;
    
    // Get optimal placement for query
    auto placement = numa_optimizer.getOptimalPlacement(
        100 * 1024 * 1024,  // data_size (100 MB)
        8                    // parallelism
    );
    
    std::cout << "Preferred NUMA node: " << placement.preferred_numa_node << std::endl;
    std::cout << "CPU affinity: ";
    for (int cpu : placement.cpu_affinity) {
        std::cout << cpu << " ";
    }
    std::cout << std::endl;
    
    // Pin thread to CPU
    NumaAwareOptimizer::pinThreadToCpu(placement.cpu_affinity[0]);
}
```

### Benefits

- **Reduced latency**: Local memory access is 2-3x faster than remote
- **Better cache utilization**: Keep data and threads on same node
- **Improved scalability**: Avoid memory bus contention

---

## HNSW Production Defaults

### Concepts

HNSW (Hierarchical Navigable Small World) indexes require careful parameter tuning. The production defaults system provides:
- Dataset-size-based parameter selection
- Performance profile selection (latency vs recall trade-offs)
- Runtime adaptation based on actual performance

### HnswProductionDefaults

```cpp
#include "index/hnsw_production_defaults.h"

using namespace themis::index;

// Get recommended parameters for dataset
auto params = HnswProductionDefaults::getRecommendedParams(
    1000000,  // dataset_size
    768,      // dimension
    HnswProductionDefaults::PerformanceProfile::BALANCED
);

std::cout << "M: " << params.M << std::endl;
std::cout << "ef_construction: " << params.ef_construction << std::endl;
std::cout << "ef_search: " << params.ef_search << std::endl;
std::cout << "Use prefetch: " << params.use_prefetch << std::endl;
std::cout << "NUMA aware: " << params.numa_aware << std::endl;
```

### Performance Profiles

Three profiles are available:

1. **LATENCY_OPTIMIZED**: Minimum query latency, ~95% recall
   - Lower ef_search values
   - Best for real-time applications

2. **BALANCED**: Balanced latency/recall, ~97% recall
   - Default recommendation
   - Good for most use cases

3. **RECALL_OPTIMIZED**: Maximum recall ~99%, higher latency acceptable
   - Higher ef_search values
   - Best for accuracy-critical applications

```cpp
// Compare profiles
auto latency_opt = HnswProductionDefaults::getRecommendedParams(
    100000, 768, HnswProductionDefaults::PerformanceProfile::LATENCY_OPTIMIZED);

auto recall_opt = HnswProductionDefaults::getRecommendedParams(
    100000, 768, HnswProductionDefaults::PerformanceProfile::RECALL_OPTIMIZED);

// latency_opt.ef_search < recall_opt.ef_search
```

### Runtime Adaptation

```cpp
using themis::index::HnswRuntimeAdapter;

int current_ef = 64;
double actual_latency = 15.0;
double target_latency = 10.0;
double actual_recall = 0.93;
double target_recall = 0.95;

// Adapt ef_search based on actual performance
int new_ef = HnswRuntimeAdapter::adjustEfSearch(
    current_ef,
    actual_latency,
    target_latency,
    actual_recall,
    target_recall
);

std::cout << "Adjusted ef_search: " << current_ef << " -> " << new_ef << std::endl;
```

### Overfetch for Post-Filtering

When applying post-filters to vector search results:

```cpp
size_t k = 10;  // Want 10 results
double filter_selectivity = 0.2;  // 20% of vectors pass filter

// Get overfetch multiplier
double overfetch = HnswRuntimeAdapter::getOverfetchMultiplier(
    filter_selectivity, k);

size_t k_initial = static_cast<size_t>(k * overfetch);
// Fetch k_initial results, apply filter, get ~k final results
```

---

## Usage Examples

### Complete Adaptive Query Example

```cpp
#include "query/query_optimizer.h"
#include "query/adaptive_optimizer.h"

// Initialize optimizer with adaptive features
themis::QueryOptimizer optimizer(index_manager);
optimizer.enableAdaptiveOptimization(true);

// Execute query
auto plan = optimizer.chooseOrderForAndQuery(query);
auto results = optimizer.executeOptimizedKeys(engine, query, plan);

// Record execution for learning
optimizer.recordQueryExecution(
    query_hash,
    plan.details[0].estimatedCount,
    results.value().size(),
    execution_time_ms
);

// Future queries will benefit from this history
```

### Complete Distributed Query Example

```cpp
#include "query/query_optimizer.h"

themis::QueryOptimizer optimizer(index_manager);

// Get available shards
std::vector<std::string> shards = {"shard1", "shard2", "shard3"};

// Optimize for distributed execution
auto dist_plan = optimizer.optimizeForDistribution(
    query,
    shards,
    true  // enable partition pruning
);

// Execute on recommended shards with optimal parallelism
executeDistributedQuery(query, dist_plan);
```

---

## Performance Tuning

### Adaptive Optimization

**When to enable:**
- Long-running workloads with repeated query patterns
- Queries with complex predicates and joins
- Environments with changing data distributions

**Configuration:**
```cpp
optimizer.enableAdaptiveOptimization(true);
```

**Best practices:**
- Let the system collect at least 10-20 executions per query pattern before making decisions
- Monitor the `hasCardinalityMisestimation()` metric to identify problematic queries
- Use runtime plan switching for long-running queries (> 1 second execution time)

### Distributed Optimization

**When to use:**
- Multi-shard/multi-region deployments
- Queries spanning multiple partitions
- Cross-datacenter queries

**Best practices:**
- Measure actual network latency between shards
- Enable partition pruning for selective queries
- Use broadcast joins for small lookup tables (< 10K rows)
- Use repartition joins for large-large joins with similar sizes

### Multi-Index Optimization

**When beneficial:**
- Queries with multiple equality predicates (AND conditions)
- Each predicate has moderate selectivity (5-20%)
- Indexes exist on all predicate columns

**Best practices:**
- Create indexes on frequently queried columns
- Monitor `estimated_result_rows` vs actual to validate benefit
- Consider composite indexes for very common query patterns

### NUMA Optimization

**When to enable:**
- Systems with 2+ NUMA nodes
- Large working sets (> 1 GB)
- High parallelism (> 8 threads)

**Best practices:**
- Pin long-running queries to specific NUMA nodes
- Allocate query buffers on the same NUMA node as execution
- Monitor NUMA hit rates (`numastat` on Linux)

### HNSW Tuning

**Quick Start:**
```cpp
// For 1M vectors, 768 dimensions, balanced profile
auto params = HnswProductionDefaults::getRecommendedParams(
    1000000, 768, HnswProductionDefaults::PerformanceProfile::BALANCED);
```

**Latency-critical applications:**
```cpp
auto params = HnswProductionDefaults::getRecommendedParams(
    dataset_size, dimension, 
    HnswProductionDefaults::PerformanceProfile::LATENCY_OPTIMIZED);
```

**Accuracy-critical applications:**
```cpp
auto params = HnswProductionDefaults::getRecommendedParams(
    dataset_size, dimension,
    HnswProductionDefaults::PerformanceProfile::RECALL_OPTIMIZED);
```

**Runtime monitoring:**
```cpp
// Track actual performance
double actual_latency = measure_query_latency();
double actual_recall = measure_recall();  // Requires ground truth

// Adapt ef_search
int new_ef = HnswRuntimeAdapter::adjustEfSearch(
    current_ef, actual_latency, target_latency,
    actual_recall, target_recall);
```

---

## Performance Benchmarks

Based on ThemisDB benchmark results (BENCHMARK_RESULTS_COMPLETE_2025.md):

### Adaptive Optimization
- **Cardinality estimation accuracy**: Improved from 60% to 85% after 100 executions
- **Query plan quality**: 15-20% better execution times for complex queries
- **Runtime switching**: Prevents 10-30% performance degradation from bad estimates

### Distributed Optimization
- **Partition pruning**: Reduces shards queried by 30-70% for selective queries
- **Cross-shard joins**: 20-40% faster with optimal strategy selection
- **Network cost reduction**: 50% less data transfer with semi-join strategy

### Multi-Index Optimization
- **Index intersection**: 2-5x faster for queries with 2-3 selective predicates
- **Bitmap operations**: Efficient for selectivity < 10%

### NUMA Optimization
- **Memory latency**: 2-3x improvement for local vs remote access
- **Scalability**: 85-90% scaling efficiency on NUMA systems (vs 70-75% without)

### HNSW Defaults
- **Build time**: Optimized parameters reduce build time by 15-25%
- **Query latency**: Balanced profile achieves < 10ms P99 latency
- **Recall**: 95-97% recall with balanced profile, 99%+ with recall-optimized

---

## References

- HNSW Paper: Malkov & Yashunin (2018)
- ThemisDB Benchmark Results: docs/de/performance/BENCHMARK_RESULTS_COMPLETE_2025.md
- Adaptive Query Processing: Deshpande et al. (2007)
- NUMA-Aware Optimization: Leis et al. (2013)

---

## API Reference

See header files for complete API documentation:
- `include/query/adaptive_optimizer.h`
- `include/index/hnsw_production_defaults.h`
- `include/query/query_optimizer.h`
