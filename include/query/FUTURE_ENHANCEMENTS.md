# Query Module - Future Enhancements

## Scope

- API-level enhancements to `include/query/` headers — public C++ interfaces for query execution, planning, and optimization
- JIT compilation hint API: `QueryEngine::Config::enable_jit` and `jit_threshold`; per-query opt-in via `QueryOptions`
- Adaptive execution hook: `AQEHook` callback invoked on plan re-optimization with actual vs. estimated cardinality
- Distributed plan interface: `DistributedQueryCoordinator` requiring explicit `Coordinator` reference; no global state
- UDF registration API: `QueryEngine::registerUDF()` with sandboxed execution policy
- Plan visualization API: `QueryPlan::toDOT()`, `toJSON()`, `toHTML()` for debugging and tooling integration

## Design Constraints

- [ ] JIT API is opt-in per query via `QueryOptions::enable_jit`; default is interpreted execution
- [ ] UDF registration is thread-safe; concurrent `registerUDF()` calls are serialized internally
- [ ] Distributed plan API requires explicit `Coordinator` reference passed to `DistributedQueryCoordinator`; no implicit global state
- [ ] `AQEHook` callbacks must be non-blocking and complete within 1 ms; long operations are disallowed
- [ ] Plan visualization APIs are read-only and do not affect query execution state
- [ ] All `QueryEngine` public methods return `Result<T>`; no exception propagation across the public API

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `QueryOptions` | All query execution callers | Carries per-query JIT, AQE, and resource limit flags |
| `AQEHook` | `QueryEngine` internals, monitoring | Callback type; invoked on adaptive re-plan |
| `DistributedQueryCoordinator` | Cluster query path | Requires explicit `RaftCluster&` constructor arg |
| `QueryEngine::registerUDF` | Application code, plugin system | Thread-safe; sandbox enforced at registration |
| `QueryPlan` | Debugging tools, explain output | `toDOT()` / `toJSON()` / `toHTML()` read-only methods |

## Planned Features

### Query Compilation (JIT)
**Priority:** High
**Target Version:** v1.7.0

Just-in-time compilation of frequently executed queries to native code.

**Features:**
- Query plan → LLVM IR generation
- Native code compilation and caching
- Automatic compilation threshold (after N executions)
- Runtime profiling and recompilation

**Expected Performance:**
- 5-10x speedup for CPU-intensive queries
- Especially beneficial for:
  - Complex expressions
  - Tight loops over large datasets
  - Arithmetic-heavy computations

**API:**
```cpp
QueryEngine::Config config;
config.enable_jit = true;
config.jit_threshold = 10;  // Compile after 10 executions
config.jit_optimization_level = 2;  // -O2 equivalent

QueryEngine engine(storage, index_mgr, evaluator, config);

// First 10 executions: interpreted
// 11+ executions: JIT-compiled native code
auto result = engine.execute(query);
```

**Compilation Pipeline:**
```
AQL Query
    ↓
Query Plan
    ↓
LLVM IR Generation
    ↓
Optimization Passes
    ↓
Native Code
    ↓
Cache (disk/memory)
    ↓
Execute
```

---

### Adaptive Query Execution (AQE)
**Priority:** High
**Target Version:** v1.7.0

Runtime query plan adjustment based on actual execution statistics.

**Features:**
- Dynamic join reordering during execution
- Automatic broadcast join conversion
- Adaptive partition pruning
- Runtime filter pushdown
- Dynamic degree of parallelism adjustment

**Example:**
```
Initial Plan:
  HashJoin(users, orders)  [estimated: 1M rows from users]
    ↓
Runtime Statistics:
  users actual rows: 100K (10x less than estimated)
    ↓
Adapt Plan:
  Switch to BroadcastJoin(users, orders)  [more efficient for small left side]
```

**Configuration:**
```cpp
QueryEngine::Config config;
config.enable_aqe = true;
config.aqe_reoptimization_threshold = 2.0;  // Reoptimize if estimate is 2x off
config.aqe_broadcast_threshold = 10 * 1024 * 1024;  // 10MB

// Automatic plan adaptation during execution
auto result = engine.execute(query, config);
```

---

### Distributed Query Planning with Raft Coordination
**Priority:** High
**Target Version:** v1.8.0

Raft-based coordination for distributed query execution across clusters.

**Features:**
- Distributed query coordinator (Raft leader)
- Consensus-based query routing decisions
- Automatic failover on node failures
- Distributed transaction coordination
- Cross-datacenter query federation

**Architecture:**
```
Query Coordinator (Raft Leader)
    ↓
Decompose Query
    ↓
┌─────────┬─────────┬─────────┐
│ Shard 1 │ Shard 2 │ Shard 3 │
└─────────┴─────────┴─────────┘
    ↓          ↓          ↓
Consensus on execution status
    ↓
Merge Results
    ↓
Return to Client
```

**API:**
```cpp
DistributedQueryCoordinator coordinator(raft_cluster);

// Automatic failover and coordination
auto result = coordinator.executeFederatedQuery(query);

// If coordinator fails, Raft elects new leader automatically
```

---

### GPU-Accelerated Query Execution
**Priority:** Medium
**Target Version:** v1.8.0

Offload compute-intensive query operations to GPU for massive parallelism.

**Supported Operations:**
- Vector similarity search (FAISS GPU backend)
- Aggregations (SUM, AVG, COUNT)
- Filtering on large datasets
- Sort operations
- Matrix operations for analytical queries

**Expected Speedup:**
- Vector search: 10-100x faster
- Aggregations: 5-20x faster (depends on data size)
- Filters: 3-10x faster for arithmetic-heavy predicates

**API:**
```cpp
QueryEngine::Config config;
config.enable_gpu = true;
config.gpu_memory_limit = 4 * 1024 * 1024 * 1024;  // 4GB
config.gpu_device_id = 0;
config.gpu_batch_size = 10000;

// Automatically offload eligible operations to GPU
auto result = engine.execute(query, config);
```

**Eligible Query Patterns:**
```sql
-- Vector similarity (GPU-accelerated)
FOR doc IN embeddings
  FILTER SIMILARITY(doc.vector, @query_vec, 100)
  RETURN doc

-- Aggregations over large datasets (GPU-accelerated)
FOR sale IN sales
  COLLECT product = sale.product_id
  AGGREGATE total = SUM(sale.amount)
  RETURN {product, total}

-- Arithmetic-heavy filters (GPU-accelerated)
FOR doc IN measurements
  FILTER doc.value * 1.5 + doc.offset > 100
  RETURN doc
```

---

### Query Result Prefetching
**Priority:** Medium
**Target Version:** v1.7.0

Predictive prefetching of query results based on access patterns.

**Features:**
- Machine learning-based query prediction
- Automatic prefetch of likely next queries
- Session-aware prediction (per-user patterns)
- Time-of-day and workload-aware prefetching

**Prediction Models:**
- **Markov Model**: Predict next query based on query sequence
- **Time-Series Model**: Predict queries based on time patterns
- **User Model**: Predict queries based on user behavior

**Example:**
```
User executes: "FOR u IN users FILTER u.city == 'Seattle' RETURN u"
    ↓
Prediction: Likely next query: "FOR o IN orders FILTER o.user_id IN @user_ids RETURN o"
    ↓
Prefetch: Execute predicted query in background
    ↓
Cache result
    ↓
User executes predicted query → Cache hit!
```

**Configuration:**
```cpp
QueryPrefetcher::Config config;
config.enable_prefetch = true;
config.prediction_model = PredictionModel::MARKOV;
config.confidence_threshold = 0.7;  // Only prefetch if >70% confident
config.max_concurrent_prefetch = 3;

QueryPrefetcher prefetcher(cache, config);
prefetcher.recordQuery(query);  // Learn from executed queries
```

---

### Automatic Index Recommendation
**Priority:** High
**Target Version:** v1.7.0

Analyze query workload and recommend optimal indexes.

**Features:**
- Workload analysis over time window
- Cost-benefit analysis for proposed indexes
- Index size and maintenance cost estimation
- Automatic index creation (opt-in)
- Index usage tracking and cleanup

**Recommendation Engine:**
```cpp
IndexRecommender recommender;

// Analyze queries over time window
recommender.recordQuery(query);

// Get recommendations after sufficient data
auto recommendations = recommender.getRecommendations();
for (const auto& rec : recommendations) {
    std::cout << "Table: " << rec.table << std::endl;
    std::cout << "Columns: " << rec.columns << std::endl;
    std::cout << "Type: " << rec.index_type << std::endl;
    std::cout << "Expected speedup: " << rec.expected_speedup << "x" << std::endl;
    std::cout << "Estimated size: " << rec.estimated_size_mb << "MB" << std::endl;
    std::cout << "ROI: " << rec.roi << std::endl;
}

// Automatically create recommended indexes
recommender.applyRecommendations(index_mgr);
```

**Recommendation Report:**
```
Index Recommendations (based on 10,000 queries over 7 days):

1. users.city_status_idx (B-tree)
   - Columns: city, status
   - Queries affected: 2,500 (25%)
   - Expected speedup: 15x
   - Size: 45MB
   - Maintenance cost: Low
   - ROI: High

2. orders.user_id_date_idx (Composite)
   - Columns: user_id, date
   - Queries affected: 1,800 (18%)
   - Expected speedup: 8x
   - Size: 120MB
   - Maintenance cost: Medium
   - ROI: High

3. embeddings.vector_idx (HNSW)
   - Column: vector
   - Queries affected: 500 (5%)
   - Expected speedup: 50x
   - Size: 2GB
   - Maintenance cost: High
   - ROI: Medium
```

---

### Recursive CTE Support
**Priority:** Medium
**Target Version:** v1.7.0

Full recursive Common Table Expression support for hierarchical queries.

**Features:**
- Recursive WITH clauses
- Cycle detection and prevention
- Maximum recursion depth limits
- Breadth-first and depth-first traversal

**Syntax:**
```sql
WITH RECURSIVE employee_hierarchy AS (
  -- Base case
  FOR e IN employees
    FILTER e.id == @manager_id
    RETURN {
      id: e.id,
      name: e.name,
      level: 0,
      path: [e.id]
    }

  UNION ALL

  -- Recursive case
  FOR eh IN employee_hierarchy
    FOR e IN employees
      FILTER e.manager_id == eh.id
      FILTER e.id NOT IN eh.path  // Cycle detection
      FILTER eh.level < 10  // Max depth
      RETURN {
        id: e.id,
        name: e.name,
        level: eh.level + 1,
        path: APPEND(eh.path, e.id)
      }
)
FOR emp IN employee_hierarchy
  SORT emp.level, emp.name
  RETURN emp
```

**Use Cases:**
- Organization charts
- Bill of materials (BOM)
- Graph traversal
- Hierarchical data structures

---

### Query Materialized Views
**Priority:** Medium
**Target Version:** v1.8.0

Automatic materialized view creation and maintenance for expensive queries.

**Features:**
- Automatic view creation for repeated expensive queries
- Incremental view refresh
- Dependency tracking (invalidate on source data changes)
- Automatic view pruning (remove unused views)

**Configuration:**
```cpp
MaterializedViewManager::Config config;
config.auto_create_threshold_ms = 1000;  // Materialize queries >1s
config.min_executions = 5;  // After 5 executions
config.refresh_strategy = RefreshStrategy::INCREMENTAL;
config.max_views = 100;

MaterializedViewManager view_mgr(storage, config);

// Automatically creates materialized views
auto result = view_mgr.execute(expensive_query);

// Subsequent executions use materialized view
auto cached_result = view_mgr.execute(expensive_query);  // Fast!
```

**Refresh Strategies:**
- **Immediate**: Refresh on every source data change
- **Deferred**: Refresh on next query access
- **Scheduled**: Refresh at fixed intervals
- **Incremental**: Only update changed portions

---

### Query Plan Visualization
**Priority:** Low
**Target Version:** v1.7.0

Visual query plan representation for debugging and optimization.

**Features:**
- Graphical query plan rendering
- Interactive plan exploration
- Cost annotations on operators
- Actual vs estimated cardinality comparison
- Critical path highlighting

**Output Formats:**
- **DOT/Graphviz**: For diagram generation
- **JSON**: For programmatic analysis
- **HTML**: For interactive web visualization

**API:**
```cpp
auto plan = optimizer.generatePlan(query);

// Export as DOT for Graphviz
std::string dot = plan.toDOT();
// Render: dot -Tpng plan.dot -o plan.png

// Export as JSON
nlohmann::json json = plan.toJSON();

// Export as interactive HTML
std::string html = plan.toHTML();
// Open in browser for interactive exploration
```

**Visualization Example:**
```
┌─────────────────┐
│  Return         │  Cost: 10, Rows: 100
└────────┬────────┘
         │
┌────────▼────────┐
│  Filter         │  Cost: 100, Rows: 100, Selectivity: 0.1
│  (age > 30)     │  ⚠ Estimated: 1000 rows (10x off!)
└────────┬────────┘
         │
┌────────▼────────┐
│  Index Scan     │  Cost: 200, Rows: 1000
│  (city == 'SEA')│  Index: city_idx
└─────────────────┘
```

---

### ~~Query Timeout and Resource Limits~~ ✅ Implemented (v0.0.33)

Per-query resource limits to prevent runaway queries.

**Implemented in:**
- `include/query/query_resource_limits.h` — `QueryResourceLimits` struct and `QueryResourceGuard` RAII class
- `src/query/aql_runner.cpp` — `executeAqlWithLimits()` free function
- `tests/test_query_resource_limits.cpp` — unit and integration tests

**Implemented features:**
- Query timeout (wall-clock time via `timeout_ms`)
- Memory limit (serialised JSON result size via `max_memory_bytes`)
- Result set size limit (via `max_rows`)

**Usage:**
```cpp
#include "query/aql_runner.h"
#include "query/query_resource_limits.h"

QueryResourceLimits limits;
limits.max_rows         = 1000;             // max result rows  (0 = unlimited)
limits.max_memory_bytes = 4 * 1024 * 1024;  // 4 MB result size (0 = unlimited)
limits.timeout_ms       = 5000;             // 5 s wall-clock   (0 = unlimited)

auto result = executeAqlWithLimits(aql, engine, limits);
if (!result) {
    if (result.error().code() == errors::ErrorCode::ERR_QUERY_TIMEOUT) {
        std::cerr << "Query exceeded time limit" << std::endl;
    } else if (result.error().code() == errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED) {
        std::cerr << "Query exceeded row or memory limit" << std::endl;
    }
}
```

**Future extensions (not yet implemented):**
- CPU time limits
- I/O limits (bytes read)
- Per-user limit profiles

---

### Query Sampling
**Priority:** Low
**Target Version:** v1.8.0

Execute queries on data samples for approximate results with low latency.

**Features:**
- Reservoir sampling for large datasets
- Statistical confidence intervals
- Approximate aggregations (COUNT, SUM, AVG)
- Sample-based query planning

**Syntax:**
```sql
-- Approximate count (fast!)
FOR doc IN users SAMPLE 0.1  -- 10% sample
  RETURN 1
| COUNT * 10  -- Scale up

-- Approximate average
FOR doc IN sales SAMPLE 0.05  -- 5% sample
  RETURN doc.amount
| AVG  -- Representative sample

-- With confidence interval
FOR doc IN measurements SAMPLE 0.01
  RETURN {
    avg: AVG(doc.value),
    confidence_95: CONFIDENCE_INTERVAL(doc.value, 0.95)
  }
```

**API:**
```cpp
QueryEngine::SamplingConfig config;
config.sample_rate = 0.1;  // 10% sample
config.min_sample_size = 1000;  // At least 1000 rows
config.max_sample_size = 100000;  // At most 100K rows
config.provide_confidence_interval = true;

auto result = engine.executeSampled(query, config);
```

---

### Multi-Dialect Query Support
**Priority:** Low
**Target Version:** v1.9.0

Support for multiple query languages beyond AQL.

**Planned Dialects:**
- **SQL**: Standard SQL-92/SQL-99 support
- **GraphQL**: Graph query language
- **Cypher**: Neo4j-compatible graph queries
- **MQL**: MongoDB-compatible queries
- **Gremlin**: Apache TinkerPop graph traversal

**API:**
```cpp
QueryEngine engine(storage, index_mgr, evaluator);

// Execute SQL
auto sql_result = engine.executeSQL(
    "SELECT * FROM users WHERE age > 30"
);

// Execute Cypher
auto cypher_result = engine.executeCypher(
    "MATCH (u:User)-[:FRIEND]->(f) WHERE u.name = 'Alice' RETURN f"
);

// Execute MQL (MongoDB)
auto mql_result = engine.executeMQL(
    R"({"collection": "users", "filter": {"age": {"$gt": 30}}})"
);
```

**Translation Layer:**
```
SQL/Cypher/MQL/GraphQL
        ↓
Internal AST (AQL-based)
        ↓
Query Optimizer
        ↓
Execution Engine
```

---

## API Extensions

### Streaming Aggregations
**Priority:** High
**Target Version:** v1.7.0

Incremental aggregation computation for streaming results.

**Features:**
- Windowed aggregations over streams
- Tumbling, sliding, and session windows
- Real-time aggregation updates
- Low-latency streaming pipelines

**API:**
```cpp
StreamAggregator aggregator;

// Define tumbling window (5 minutes)
WindowSpec window;
window.type = WindowType::TUMBLING;
window.duration = std::chrono::minutes(5);

// Define aggregation
AggregationSpec agg;
agg.function = AggFunction::SUM;
agg.field = "amount";

// Process stream
auto stream = data_source.stream();
while (auto item = stream.next()) {
    aggregator.add(item, window, agg);

    // Get current window results
    if (auto result = aggregator.getCurrentWindow()) {
        process(result.value());
    }
}
```

---

### Query Profiling API
**Priority:** Medium
**Target Version:** v1.7.0

Detailed profiling information for query optimization.

**Features:**
- Per-operator timing
- Memory allocation tracking
- I/O statistics
- Cache hit/miss rates
- Lock contention metrics

**API:**
```cpp
QueryProfiler profiler;

// Execute with profiling
auto result = profiler.execute(engine, query);

// Get profile data
QueryProfile profile = profiler.getProfile();

// Print profile report
std::cout << "Total time: " << profile.total_time_ms << "ms" << std::endl;
std::cout << "Operators:" << std::endl;
for (const auto& op : profile.operators) {
    std::cout << "  " << op.name << ": "
              << op.time_ms << "ms, "
              << op.rows_processed << " rows, "
              << op.memory_mb << "MB" << std::endl;
}

// Export as JSON for visualization
nlohmann::json json = profile.toJSON();
```

**Profile Output:**
```json
{
  "query": "FOR doc IN users FILTER doc.age > 30 RETURN doc",
  "total_time_ms": 125.5,
  "operators": [
    {
      "type": "IndexScan",
      "time_ms": 100.2,
      "rows_processed": 10000,
      "memory_mb": 15.5,
      "cache_hits": 8500,
      "cache_misses": 1500
    },
    {
      "type": "Filter",
      "time_ms": 20.1,
      "rows_processed": 10000,
      "rows_output": 3000,
      "selectivity": 0.3
    },
    {
      "type": "Return",
      "time_ms": 5.2,
      "rows_processed": 3000
    }
  ],
  "memory_peak_mb": 25.8,
  "io_bytes_read": 52428800
}
```

---

### Query Hints
**Priority:** Medium
**Target Version:** v1.7.0

Explicit optimizer hints for fine-tuning query execution.

**Hint Types:**
- **Index Hints**: Force specific index usage
- **Join Hints**: Force join type/order
- **Parallelism Hints**: Control degree of parallelism
- **Memory Hints**: Memory budget hints

**Syntax:**
```sql
-- Force index usage
FOR doc IN users
  USE INDEX city_idx
  FILTER doc.city == "Seattle"
  RETURN doc

-- Force join type
FOR u IN users
  FOR o IN orders
    USE HASH JOIN
    FILTER o.user_id == u.id
    RETURN {user: u, order: o}

-- Control parallelism
FOR doc IN large_collection
  USE PARALLEL 8
  FILTER doc.value > 100
  RETURN doc

-- Memory budget hint
FOR doc IN huge_collection
  USE MEMORY 1GB
  SORT doc.score DESC
  LIMIT 100
  RETURN doc
```

**API:**
```cpp
QueryHints hints;
hints.force_index = "city_idx";
hints.join_type = JoinType::HASH;
hints.parallel_degree = 8;
hints.memory_budget_mb = 1024;

auto result = engine.execute(query, hints);
```

---

### Query Explain Plan
**Priority:** High
**Target Version:** v1.6.0

Detailed execution plan explanation without executing query.

**API:**
```cpp
auto explain = engine.explain(query);

std::cout << "Execution Plan:" << std::endl;
std::cout << explain.plan_text << std::endl;
std::cout << "Estimated cost: " << explain.estimated_cost << std::endl;
std::cout << "Estimated rows: " << explain.estimated_rows << std::endl;
std::cout << "Estimated time: " << explain.estimated_time_ms << "ms" << std::endl;

// Detailed operator breakdown
for (const auto& op : explain.operators) {
    std::cout << op.type << " - Cost: " << op.cost
              << ", Rows: " << op.estimated_rows << std::endl;
}
```

**Output Format:**
```
Execution Plan:
  Return (cost=10, rows=100, time=1ms)
    ← Filter (cost=100, rows=100, selectivity=0.1, time=10ms)
      ← IndexScan on city_idx (cost=200, rows=1000, time=100ms)

Indexes Used:
  - city_idx (selectivity: 0.1, cost: 200)

Warnings:
  - Missing index on 'age' column (potential speedup: 10x)
  - Large result set (consider LIMIT clause)

Estimated Total Cost: 310
Estimated Rows: 100
Estimated Time: 111ms
```

---

## Backward Compatibility

### API Versioning
**Target Version:** v1.7.0

Versioned public API to ensure backward compatibility.

**Versioning Strategy:**
```cpp
namespace themis::query::v1 {
    class QueryEngine { /* v1 API */ };
}

namespace themis::query::v2 {
    class QueryEngine { /* v2 API with new features */ };
}

// Compatibility layer
namespace themis::query {
    using QueryEngine = v2::QueryEngine;  // Default to latest
}
```

**Migration Path:**
```cpp
// Old code (v1.6)
#include "query/query_engine.h"
themis::query::QueryEngine engine;  // Still works in v1.7+

// New code (v1.7+)
#include "query/query_engine.h"
themis::query::v2::QueryEngine engine;  // Opt-in to v2 API
```

---

### Deprecation Policy

**Rules:**
1. Features deprecated in v1.x are removed in v2.0
2. Minimum 2 minor versions (6 months) deprecation period
3. Clear deprecation warnings in logs and documentation
4. Migration guides provided

**Deprecated Features (v1.6 → v1.7):**

| Feature | Deprecated | Removed | Replacement |
|---------|-----------|---------|-------------|
| `QueryEngine::executeSimple()` | v1.6 | v2.0 | `QueryEngine::execute()` |
| `QueryCache::Config::cache_size` | v1.6 | v2.0 | `QueryCache::Config::max_entries` |
| `AQLParser::parseOld()` | v1.5 | v1.7 | `AQLParser::parse()` |

**Deprecation Example:**
```cpp
// Deprecated API (v1.6)
[[deprecated("Use execute() instead")]]
Result<nlohmann::json> executeSimple(const std::string& query);

// Usage triggers warning:
auto result = engine.executeSimple(query);
// Warning: 'executeSimple' is deprecated: Use execute() instead

// New API (v1.7+)
Result<nlohmann::json> execute(const std::string& query, const QueryOptions& options = {});
```

---

## Migration Guides

### v1.5 → v1.6 Migration

**Breaking Changes:**
- None (fully backward compatible)

**New Features:**
- Query timeout and resource limits
- Explain plan API
- Improved error messages

**Recommended Updates:**
```cpp
// Old (v1.5)
auto result = engine.execute(query);

// New (v1.6) - Add timeout
QueryEngine::ResourceLimits limits;
limits.max_execution_time_ms = 30000;
auto result = engine.execute(query, limits);
```

---

### v1.6 → v1.7 Migration

**Breaking Changes:**
- `executeSimple()` deprecated (use `execute()`)
- `QueryCache::Config::cache_size` renamed to `max_entries`

**New Features:**
- JIT compilation
- Adaptive query execution
- Query profiling
- Query hints

**Migration Steps:**

**1. Update QueryCache Configuration:**
```cpp
// Old (v1.6)
QueryCache::Config config;
config.cache_size = 10000;

// New (v1.7)
QueryCache::Config config;
config.max_entries = 10000;
```

**2. Replace executeSimple():**
```cpp
// Old (v1.6)
auto result = engine.executeSimple(query);

// New (v1.7)
auto result = engine.execute(query);
```

**3. Enable New Features (Optional):**
```cpp
// Enable JIT compilation
QueryEngine::Config config;
config.enable_jit = true;
config.jit_threshold = 10;
QueryEngine engine(storage, index_mgr, evaluator, config);

// Enable adaptive execution
config.enable_aqe = true;
```

---

### AQL Syntax Evolution

**v1.5 → v1.6:**
- No syntax changes

**v1.6 → v1.7:**
- Query hints syntax added
- Sampling syntax added (SAMPLE keyword)

**v1.7 → v1.8:**
- Recursive CTE syntax
- Streaming aggregations syntax

**Syntax Compatibility Table:**

| Feature | v1.5 | v1.6 | v1.7 | v1.8 |
|---------|------|------|------|------|
| Basic FOR/FILTER/RETURN | ✅ | ✅ | ✅ | ✅ |
| LET expressions | ✅ | ✅ | ✅ | ✅ |
| Window functions | ✅ | ✅ | ✅ | ✅ |
| CTEs (non-recursive) | ✅ | ✅ | ✅ | ✅ |
| Subqueries | ✅ | ✅ | ✅ | ✅ |
| Query hints | ❌ | ❌ | ✅ | ✅ |
| Sampling (SAMPLE) | ❌ | ❌ | ✅ | ✅ |
| Recursive CTEs | ❌ | ❌ | ❌ | ✅ |
| Streaming AGG | ❌ | ❌ | ❌ | ✅ |

---

## Known Limitations & Workarounds

### Limitation #1: CTE Materialization Strategy
**Severity:** Medium
**Versions:** v1.5.x, v1.6.x

CTEs are always materialized, even when referenced only once.

**Workaround:**
```sql
-- Inefficient: CTE materialized
WITH temp AS (
  FOR doc IN users FILTER doc.age > 30 RETURN doc
)
FOR t IN temp RETURN t

-- Efficient: Inline query
FOR doc IN users FILTER doc.age > 30 RETURN doc
```

**Planned Fix:** v1.7.0 - Inline CTEs when referenced once

---

### Limitation #2: No Cross-Shard Transactions
**Severity:** High
**Versions:** v1.5.x, v1.6.x, v1.7.x

Federated queries don't support ACID transactions across shards.

**Workaround:**
- Design shard-local transactions
- Use compensation logic for multi-shard updates
- Consider eventual consistency

**Planned Fix:** v1.8.0 - Distributed transactions with 2PC

---

### Limitation #3: Limited JIT for Complex Expressions
**Severity:** Low
**Versions:** v1.7.0

JIT compilation doesn't support all expression types.

**Unsupported:**
- User-defined functions
- Complex spatial operations
- Some graph operations

**Workaround:** These operations fall back to interpreted execution

**Planned Fix:** v1.8.0 - Expand JIT coverage

---

### Limitation #4: Query Cache Invalidation Granularity
**Severity:** Medium
**Versions:** v1.5.x, v1.6.x

Cache invalidation is table-level, not row-level.

**Workaround:**
- Use shorter TTLs for frequently updated tables
- Manually invalidate cache on critical updates

**Example:**
```cpp
// Write to users table
storage->put(user_key, user_value);

// Invalidate all cached queries on users table
cache.invalidate({"users"});  // Invalidates all queries, even if user not affected
```

**Planned Fix:** v1.8.0 - Row-level cache invalidation tracking

---

## Performance Roadmap

### v1.7.0 Performance Targets
- 5-10x speedup for repeated queries (JIT)
- 2-5x speedup for complex queries (AQE)
- 50% reduction in cache memory (improved eviction)
- 90%+ cache hit rate (prefetching)

### v1.8.0 Performance Targets
- 10-100x speedup for vector queries (GPU)
- 5-20x speedup for aggregations (GPU)
- 3-10x speedup for distributed queries (Raft coordination)
- <100ms p99 latency for federated queries

### v1.9.0 Performance Targets
- Sub-millisecond query compilation (JIT cache)
- 99.9% cache hit rate (ML-based prefetching)
- Predictable query latency (resource limits)

---

## Research Directions

### Machine Learning Query Optimization
**Priority:** Research
**Timeline:** 2025+

End-to-end learned query optimizer using neural networks.

**Approach:**
- Train neural network on historical query executions
- Learn cost model from actual performance
- Replace traditional cost-based optimizer
- Continuous online learning

**Challenges:**
- Requires large training dataset
- Model interpretability
- Fallback to traditional optimizer

---

### Quantum-Inspired Query Optimization
**Priority:** Research
**Timeline:** 2026+

Explore quantum algorithms for join ordering and plan selection.

**Potential Applications:**
- Optimal join ordering (exponential search space)
- Query plan enumeration
- Cardinality estimation

**Status:** Early research, not production-ready

---

## Contributing to Query Module

### Priority Areas for Contribution

**High Priority:**
1. JIT compilation implementation
2. Adaptive query execution
3. Query timeout and resource limits
4. Index recommendation engine
5. Distributed query coordination

**Medium Priority:**
1. Query profiling API
2. Query hints
3. Recursive CTEs
4. Materialized views
5. GPU acceleration

**Low Priority:**
1. Query plan visualization
2. Query sampling
3. Multi-dialect support

### Contribution Guidelines

1. **Add tests**: Comprehensive unit and integration tests required
2. **Benchmark**: Include performance benchmarks for optimizations
3. **Document**: Update README.md and function reference
4. **Backward compatibility**: Maintain API compatibility
5. **Code review**: All PRs require 2+ approvals
6. **Performance regression**: No performance regressions without justification

For detailed guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## Test Strategy

- Unit tests for JIT API: execute query 11 times with `jit_threshold=10`; assert compiled path used on 11th execution
- Unit tests for `AQEHook`: inject a 10× cardinality mismatch; assert hook is invoked and plan switches join type
- Unit tests for `DistributedQueryCoordinator`: mock `RaftCluster`; assert no global state mutations occur
- Unit tests for `registerUDF`: register UDF from 2 concurrent threads; assert no data races under TSan
- Integration tests: run explain plan for a multi-join query; assert `toDOT()` output is valid Graphviz DOT
- Regression tests: verify all deprecated APIs still compile with expected `[[deprecated]]` warnings

## Performance Targets

- Query plan compilation (JIT) ≤ 50 ms for typical OLTP queries at `-O2` LLVM optimization level
- UDF registration ≤ 1 ms per call including sandbox policy application
- Adaptive plan switch (AQE re-optimization) ≤ 5 ms at runtime without blocking query execution
- `QueryPlan::toDOT()` serialization ≤ 10 ms for plans with up to 50 operators
- `DistributedQueryCoordinator` query decomposition overhead ≤ 20 ms per federated query
- Query explain (without execution) ≤ 5 ms for standard OLTP query shapes

## Security / Reliability

- UDF sandbox prevents file system and network access; violations result in UDF termination and `ERR_UDF_SANDBOX_VIOLATION`
- Query resource limits (`QueryResourceLimits`) are enforced at the public API level before execution begins
- Distributed coordinator requires authenticated `RaftCluster` reference; unauthenticated access returns `ERR_UNAUTHORIZED`
- JIT-compiled code is verified against an IR allowlist before native execution
- Query plan serialization (`toDOT`, `toHTML`) must not leak internal storage paths or connection credentials

---

## See Also

- [README.md](README.md) - Current query module documentation
- [Storage Module Future Enhancements](../../src/storage/FUTURE_ENHANCEMENTS.md)
- [Index Module Future Enhancements](../../src/index/FUTURE_ENHANCEMENTS.md)
- [Performance Roadmap](../../docs/PERFORMANCE_ROADMAP.md)
