> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Query Module - Future Enhancements

- AQL execution engine covering relational, document, graph, vector, spatial, and timeseries models with 100+ built-in functions
- Cost-based query optimizer with adaptive learning from runtime statistics and per-query cost model feedback
- Query caching at three levels: exact-match, semantic-similarity (embedding-based), and materialized CTE
- UDF registration API supporting C++ and (future) WASM/Python runtimes
- Distributed query federation across heterogeneous ThemisDB cluster nodes with cost-based pruning
- JIT compilation path for frequently executed AQL sub-expressions (LLVM IR backend)
- SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE passthrough) for client migration
- Per-query resource limits: max rows, max memory bytes, wall-clock timeout

## Design Constraints

- [ ] `AQLParser` instances are NOT thread-safe; each thread must own its own instance or use mutex protection
- [ ] Optimizer plan generation must complete in < 5 ms for queries touching ≤ 10 collections
- [ ] Query cache lookups (exact match) must have < 1 ms latency at p99 under 10,000 concurrent clients
- [ ] UDF registration must not require server restart; hot-load via dynamic linking
- [ ] Distributed federation must honor per-cluster configurable timeouts; partial results must be surfaced, not silently dropped
- [ ] JIT-compiled expressions must produce results identical (bit-for-bit) to the interpreter path
- [ ] Resource limits (max rows, memory, timeout) must be enforced without data corruption on cancellation
- [ ] SQL compatibility layer must return a structured parse error for unsupported SQL features (no silent wrong results)

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `AQLParser::parse(query)` | HTTP API handler, SDK, test harness | Returns `ASTNode`; throws `AQLParseException` on syntax error |
| `QueryOptimizer::optimize(ast)` | Execution engine | Returns `QueryPlan` with cost estimate; < 5 ms for ≤ 10 collections |
| `QueryExecutor::execute(plan, ctx)` | HTTP handler, federation layer | Streams results via `ResultIterator`; honours resource limits in `ctx` |
| `QueryCache::lookup(key, mode)` | Execution engine pre-check | Modes: EXACT, SEMANTIC, CTE; returns `CacheHit` or `std::nullopt` |
| `UDFRegistry::register_fn(name, fn)` | Admin API, plugin loader | Thread-safe; hot-reload without restart |
| `CrossClusterFederator::execute(plan)` | Distributed query coordinator | Parallel `std::async`; injectable `HttpPostFn` for testing |
| `JITCompiler::compile(expr)` | Optimizer hot-path | LLVM IR backend; falls back to interpreter on compilation failure |
| `QueryContext::set_limits(rows, mem, timeout)` | Per-request middleware | Enforced via cooperative cancellation checkpoints |

## Planned Features

### `QueryOptimizer`: Wire Real MetadataShard, Prometheus, and Statistics
**Priority:** High
**Target Version:** v1.6.0

`query_optimizer.cpp` has 3 explicit TODOs all marked `(v1.5.1)`:
- Line 507: "Replace with actual `MetadataShard` integration" — optimizer uses a hardcoded fallback instead of real schema statistics.
- Line 536: "Replace with actual `PrometheusMetrics` integration" — optimizer emits no metrics; query plan selection quality is invisible.
- Line 575: "Use actual statistics and histograms" — cardinality estimates are hardcoded constants, degrading join order selection.

**Implementation Notes:**
- `[x]` **Line 507**: Inject a `MetadataShard*` (or `StatisticsCollector*` from `src/metadata/statistics_collector.cpp`) into `QueryOptimizer` constructor; replace the hardcoded fallback with `StatisticsCollector::getCardinality(collection, field)`.
- `[x]` **Line 536**: Inject a `MetricsCollector*`; emit `query.optimizer.plan_selected`, `query.optimizer.rewrite_count`, and `query.optimizer.cost_estimate` counters on each `optimize()` call.
- `[x]` **Line 575**: Use `StatisticsCollector::getHistogram(collection, field)` equi-height histograms for selectivity estimation in join cost model.
- `[x]` Add unit tests: verify that optimizer chooses index scan over full scan when selectivity < 10 % and statistics are present; verify Prometheus counters increment on each plan selection.

**Performance Targets:**
- Optimizer `optimize()` latency: ≤ 5 ms for queries with ≤ 10 joins using real statistics.

---

### `QueryFederation`: Real Shard Determination Logic
**Priority:** High
**Target Version:** v1.6.0

`query_federation.cpp` line 348: "TODO: Implement actual shard determination logic". All federated queries currently default to broadcasting to all shards, making federation performance O(N shards) regardless of the query's key range.

**Implementation Notes:**
- `[ ]` Implement shard key routing: use `ShardingManager::getShardsForKeyRange(collection, min_key, max_key)` to route range queries to only the relevant shards.
- `[ ]` For point lookups, route to the single shard owning the key via `ShardingManager::getShardForKey(collection, key)`.
- `[ ]` Retain broadcast for queries without a shard key predicate (full-collection scans); log a `WARN` when broadcasting to > 10 shards.
- `[ ]` Add unit tests: 3-shard setup, point lookup routes to 1 shard; range query routes to 2 shards; full scan broadcasts to all 3.

---

### `CTESubquery`: Replace Phase 1 Stub
**Priority:** Medium
**Target Version:** v1.7.0

`cte_subquery.cpp` line 334 has: "Phase 1 stub: treat as scalar subquery; real behavior handled elsewhere". Correlated subqueries and EXISTS subqueries may be incorrectly evaluated as scalar, producing wrong results.

**Implementation Notes:**
- `[x]` Implement correlated subquery evaluation: detect outer references in the subquery AST; evaluate subquery once per outer row with the correlated bindings.
- `[x]` Implement `EXISTS`/`NOT EXISTS` short-circuit: stop iterating the subquery result as soon as one matching row is found.
- `[x]` Add regression tests for correlated subqueries with outer reference in WHERE clause.

---

<a id="query-compilation--jit"></a>
### Query Compilation & JIT
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0)

Just-In-Time compilation of frequently executed queries to native code for 5-10x performance improvement.

**Delivered:**
- `QueryCompiler` (`include/query/query_compiler.h` + `src/query/query_compiler.cpp`): hot-path JIT compiler with call-count tracking, `hot_threshold`-based specialisation promotion, cold/hot path dispatch, invalidation API, and statistics counters.
- Integrated into `AQLRunner::executeAql()` conjunctive query path: every conjunctive AQL query is tracked by `QueryCompiler`; once `hot_threshold` (default 100) executions of the same query are observed it is promoted to the compiled specialisation.
- `VectorizedExecutionEngine` (`include/query/vectorized_execution.h` + `src/query/vectorized_execution.cpp`): SIMD-accelerated column-store batch processing with filter, project, aggregate (Sum/Count/Avg/Min/Max + group-by), sort, configurable batch size, and execution statistics.
- 30 focused unit tests in `tests/test_vectorized_execution.cpp` registered as `VectorizedExecutionFocusedTests` CMake target.
- CI: `.github/workflows/02-feature-modules_adaptive-query_query-vectorized-execution-ci.yml`.

**Features:**
- Hot query detection (>100 executions, configurable via `QueryCompiler::Config::hot_threshold`)
- Type-specialized compiled specialisations with fallback to interpreter on error
- Vectorized execution (SIMD batch ops via `VectorizedExecutionEngine`)
- Expression tree optimization (O0–O3 levels via `QueryCompiler::Config::opt_level`)

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


<a id="columnar-execution-engine-delivered-v170"></a> <!-- explicit anchor for cross-refs -->
### Columnar Execution Engine (Delivered v1.7.0)
**Priority:** High
**Target Version:** v1.7.0

Vectorized columnar execution for analytical queries, inspired by DuckDB and ClickHouse. The query facade (`query::VectorizedExecutionEngine`) converts JSON rows to columnar layout and delegates to the analytics pipeline (`analytics::ColumnarExecutionEngine`) for SelectionVector-based late materialization.

> Note: This delivered feature is retained here for traceability; its completion is tracked in `src/query/ROADMAP.md` (Phase 4: "Vectorized execution engine", completed).

### Scope
- Provide columnar in-memory layout and vectorized operators (Filter, Project, Aggregate, Sort) with 1,024 tuple batches.
- Preserve JSON-facing query APIs by converting to/from `ColumnBatch` via `VectorizedExecutionEngine`.
- Enable columnar compression paths (dictionary, RLE, bit-packing) via `storage::ColumnarFormatManager` for analytical workloads.
- Allow adaptive row/columnar switching by keeping row-materialized return path while executing operators in columnar mode.

### Design Constraints
- Default batch size 1,024; configurable via `VectorizedExecutionEngine::Config` / `ColumnarExecutionEngine::Config`.
- Late materialization via `SelectionVector` to avoid copying until required; projection is zero-copy (shared columns).
- Deterministic output ordering for Sort; aggregation materializes dense batches.
- Null handling must match JSON semantics (missing field → null) across conversions.

### Required Interfaces
- `query::VectorizedExecutionEngine` (`include/query/vectorized_execution.h`, `src/query/vectorized_execution.cpp`)
- `analytics::ColumnarExecutionEngine` and operators (`include/analytics/columnar_execution.h`, `src/analytics/columnar_execution.cpp`)
- `storage::ColumnarFormatManager` for on-disk compressed column segments (`include/storage/columnar_format.h`, `src/storage/columnar_format.cpp`)

### Implementation Notes
- `jsonToColumnBatch` infers column types, builds `ColumnBatch`, and preserves field order; `columnBatchToJson` materializes selections back to JSON.
- Operators use SIMD-friendly contiguous buffers (`Column` per type) and late materialization; selection vectors are intersected for multi-predicate filters.
- Columnar compression options (dictionary, RLE, bit-packing, frame-of-reference) are available for column segments and validated via dedicated codecs.
- Adaptive row/column behavior: row-mode callers receive JSON rows while internal pipeline remains columnar; small batches keep overhead bounded.

### Test Strategy
- Columnar operator coverage: `tests/analytics/test_columnar_execution.cpp` (SelectionVector, Column, operators, pipeline).
- Query facade coverage: `tests/test_vectorized_execution.cpp` (filter/project/aggregate/sort, stats, limits, mixed types).
- Compression and columnar format coverage: `tests/test_columnar_format.cpp` (dictionary, RLE, bit-packing, frame-of-reference, manager integration).

### Performance Targets
- Batching: 1,024 tuples default; configurable for cache fitting.
- Expected speedups vs row-wise execution: 5–10× for aggregations and 3–5× for scans (aligned with roadmap benchmarks), batch=1,024; higher when compression applies to scan-heavy workloads.

### Security / Reliability
- Null bitmap maintained per column; selection vectors bounds-checked via `std::vector<uint32_t>` accessors.
- JSON conversion treats missing fields as null to avoid undefined values; row count preserved through projections.
- Configurable memory soft limit via `ColumnarExecutionEngine::Config::max_memory_bytes` to avoid unbounded allocations during large batches.

---

### Adaptive Join Strategies
**Priority:** High
**Target Version:** v1.7.0

Intelligent join algorithm selection based on data characteristics and runtime statistics.

**Join Algorithms:**
```cpp
enum JoinAlgorithm {
    HASH_JOIN,           // Build hash table on smaller side
    MERGE_JOIN,          // Sorted inputs, O(n+m) merge
    NESTED_LOOP_JOIN,    // Small left side (<1000 rows)
    INDEX_NESTED_LOOP,   // Right side has index
    BROADCAST_JOIN,      // Distributed: broadcast small table
    SHUFFLE_JOIN,        // Distributed: repartition both sides
    GRACE_HASH_JOIN      // Partitioned hash join (out-of-core)
};

class AdaptiveJoinExecutor {
public:
    // Choose best join algorithm at runtime
    JoinResult executeJoin(
        const JoinSpec& spec,
        const Table& left,
        const Table& right,
        const RuntimeStats& stats);

private:
    JoinAlgorithm selectAlgorithm(
        size_t left_rows, size_t right_rows,
        bool left_sorted, bool right_sorted,
        bool has_index, size_t memory_budget);
};

// Cost model
double estimateJoinCost(JoinAlgorithm algo,
                       size_t left_rows,
                       size_t right_rows) {
    switch (algo) {
        case HASH_JOIN:
            return left_rows + right_rows;  // Build + probe
        case MERGE_JOIN:
            return left_rows + right_rows +  // Scan both
                   (left_sorted ? 0 : left_rows * log(left_rows)) +
                   (right_sorted ? 0 : right_rows * log(right_rows));
        case NESTED_LOOP_JOIN:
            return left_rows * right_rows;  // Quadratic
        default:
            return std::numeric_limits<double>::max();
    }
}
```

**Adaptive Selection Criteria:**
- **Hash Join**: Default for large equi-joins
- **Merge Join**: Both inputs sorted on join key
- **Nested Loop**: Left side <1000 rows
- **Index Nested Loop**: Right has index, left <10K rows
- **Grace Hash**: Memory budget exceeded

---

### Materialized Views & Incremental Maintenance
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented (Issue #195)

Pre-computed query results with automatic incremental updates.

**Features:**
- View definition and creation
- Automatic query rewriting (use view if applicable)
- Incremental maintenance on data changes
- Partial refresh strategies
- View staleness tracking

**API:**
```cpp
// Define materialized view
class MaterializedView {
public:
    struct Definition {
        std::string name;
        std::string query_aql;
        RefreshStrategy strategy;
        std::chrono::seconds staleness_tolerance{60};
    };

    enum RefreshStrategy {
        IMMEDIATE,       // Update on every base table change
        DEFERRED,        // Update on query if stale
        PERIODIC,        // Scheduled refresh
        MANUAL           // User-triggered refresh
    };

    // Create view
    static Result<void> create(const Definition& def,
                              QueryEngine& engine);

    // Refresh view
    Result<void> refresh(bool incremental = true);

    // Check if view can answer query
    static bool canRewrite(const ParsedQuery& query,
                          const MaterializedView& view);
};

// Example: Create view for frequent aggregation
MaterializedView::Definition view_def;
view_def.name = "sales_by_region";
view_def.query_aql = R"(
    FOR sale IN sales
    COLLECT region = sale.region INTO groups
    RETURN {
        region: region,
        total_sales: SUM(groups[*].sale.amount),
        avg_sale: AVG(groups[*].sale.amount),
        count: LENGTH(groups)
    }
)";
view_def.strategy = MaterializedView::DEFERRED;
view_def.staleness_tolerance = std::chrono::minutes(5);

MaterializedView::create(view_def, engine);

// Query automatically uses view if applicable
auto result = engine.execute(
    "FOR r IN sales_by_region FILTER r.region == 'EU' RETURN r"
);
// ^ Uses precomputed view, not raw sales table
```

**Incremental Maintenance:**
```cpp
// When base table changes
void onInsert(const std::string& table, const BaseEntity& entity) {
    for (auto& view : dependent_views[table]) {
        if (view->strategy == IMMEDIATE) {
            // Delta processing
            view->applyDelta(DeltaOp::INSERT, entity);
        } else if (view->strategy == DEFERRED) {
            view->markStale();
        }
    }
}

// Delta computation for common patterns
void applyAggregateDelta(const DeltaOp op,
                        const BaseEntity& entity) {
    // Example: SUM(amount) - just add/subtract the delta
    if (op == INSERT) {
        aggregate_value += entity.getField("amount").as_double();
    } else if (op == DELETE) {
        aggregate_value -= entity.getField("amount").as_double();
    }
}
```

**Performance Impact:**
- Query speedup: 10-100x (depends on aggregation complexity)
- Insert overhead: 5-20% (incremental maintenance)

---

### Query Result Streaming
**Priority:** Medium
**Target Version:** v1.7.0

Stream large query results incrementally instead of materializing entire result set.

**Features:**
- Iterator-based result consumption
- Backpressure support
- Asynchronous prefetching
- Memory-bounded execution
- Early termination support

**API:**
```cpp
class ResultStream {
public:
    // Iterator interface
    class Iterator {
    public:
        Result<bool> hasNext();
        Result<nlohmann::json> next();
        Result<void> skip(size_t count);
    };

    // Create stream from query
    static Result<ResultStream> execute(
        QueryEngine& engine,
        const ParsedQuery& query,
        const StreamConfig& config = {});

    // Async iteration
    std::future<Result<void>> forEachAsync(
        std::function<void(const nlohmann::json&)> callback,
        size_t max_concurrent = 4);

    // Backpressure control
    void pause();
    void resume();
    size_t bufferedCount() const;
};

// Example: Stream 10 million rows
auto stream = ResultStream::execute(engine, large_query);
auto it = stream->getIterator();

while (it.hasNext().value()) {
    auto row = it.next().value();
    processRow(row);  // Process one at a time

    // Memory bounded - doesn't load all 10M rows
}

// Async streaming with backpressure
stream->forEachAsync([&](const nlohmann::json& row) {
    if (outputQueue.size() > 1000) {
        stream->pause();  // Backpressure
        std::this_thread::sleep_for(100ms);
        stream->resume();
    }
    outputQueue.push(row);
});
```

**Benefits:**
- Constant memory usage (no result materialization)
- Lower latency to first result
- Better for large result sets
- Enables pipeline parallelism

---

### Parallel Query Execution (Intra-Query)
**Priority:** High
**Target Version:** v1.7.0

Parallelize single query execution across multiple CPU cores.

**Parallelization Strategies:**
```cpp
class ParallelExecutor {
public:
    struct ParallelConfig {
        size_t max_threads = std::thread::hardware_concurrency();
        size_t morsel_size = 1024;  // Rows per task
        bool enable_parallel_scan = true;
        bool enable_parallel_join = true;
        bool enable_parallel_aggregate = true;
    };

    // Parallel table scan
    Result<std::vector<BaseEntity>> parallelScan(
        const std::string& table,
        const Expression& filter,
        size_t num_threads);

    // Parallel hash join
    Result<std::vector<JoinTuple>> parallelHashJoin(
        const Table& left,
        const Table& right,
        const JoinSpec& spec,
        size_t num_threads);

    // Parallel aggregation
    Result<AggregateResult> parallelAggregate(
        const Table& input,
        const AggregateSpec& spec,
        size_t num_threads);
};

// Example: 4-way parallel scan
auto results = parallel_exec.parallelScan(
    "large_table",
    parse("field > 100"),
    4  // 4 threads
);
// Each thread processes table_size/4 rows

// Parallel hash join (partitioned)
// 1. Partition both sides by join key hash
// 2. Each thread builds hash table for its partition
// 3. Each thread probes its hash table
// 4. Merge results
auto join_result = parallel_exec.parallelHashJoin(
    left_table, right_table, join_spec, 8
);
```

**Morsel-Driven Parallelism:**
```
Table (100M rows)
    ↓
Split into morsels (1024 rows each)
    ↓
┌────────┬────────┬────────┬────────┐
│Thread 1│Thread 2│Thread 3│Thread 4│
└────────┴────────┴────────┴────────┘
    ↓        ↓        ↓        ↓
Process morsels (scan, filter, project)
    ↓        ↓        ↓        ↓
Merge results → Next operator
```

**Performance Targets:**
- Linear scaling up to 8 cores
- 70-80% efficiency at 16 cores
- Scans: 4x speedup on 4 cores
- Joins: 3x speedup on 4 cores

---

### Query Plan Caching
**Priority:** Medium
**Target Version:** v1.7.0

> **✅ Delivered — v1.7.0 (Issue #196).** This feature is retained here for traceability; its completion is tracked in `src/query/ROADMAP.md` ("Query plan caching", completed). Implementation: `include/query/plan_cache.h` and `src/query/plan_cache.cpp`. Focused tests: `tests/test_query_plan_caching.cpp` → `QueryPlanCachingFocusedTests`.

Cache optimized query plans to skip parsing and optimization on repeated queries.

**Features:**
- Plan fingerprinting (query structure + statistics)
- Parameterized plan reuse
- Plan invalidation on schema/statistics changes
- Statistics-aware plan selection

**API:**
```cpp
class PlanCache {
public:
    struct CachedPlan {
        std::string query_fingerprint;
        QueryPlan plan;
        std::vector<ParameterInfo> parameters;
        std::chrono::system_clock::time_point created_at;
        Statistics statistics_snapshot;
    };

    // Get cached plan
    Result<CachedPlan> get(const std::string& query,
                          const Statistics& current_stats);

    // Cache plan
    void put(const std::string& query,
            const QueryPlan& plan,
            const Statistics& stats);

    // Invalidate on schema change
    void invalidateTable(const std::string& table);
};

// Example: Parameterized query
std::string query_template =
    "FOR u IN users FILTER u.age > @age RETURN u";

// First execution: parse + optimize + cache
auto plan1 = engine.prepare(query_template, {{"age", 30}});
plan_cache.put(query_template, plan1, current_stats);

// Second execution: retrieve cached plan
auto cached = plan_cache.get(query_template, current_stats);
if (cached) {
    auto result = engine.execute(cached->plan, {{"age", 40}});
    // Skip parsing and optimization!
}
```

**Invalidation Strategy:**
- Schema changes: Invalidate all plans for affected tables
- Statistics drift: Invalidate if cardinality estimates change >10x
- Periodic: Refresh plans every 24 hours

---

### Subquery Optimization (Phase 3.4 Completion)
**Priority:** High
**Target Version:** v1.6.5

Complete subquery support with advanced optimization.

**Subquery Types:**
```cpp
// Scalar subquery (returns single value)
"RETURN {avg: (FOR u IN users RETURN AVG(u.age))[0]}"

// IN subquery (membership test)
"FOR u IN users FILTER u.id IN (
    FOR o IN orders FILTER o.total > 1000 RETURN o.user_id
) RETURN u"

// EXISTS subquery (existence test)
"FOR u IN users FILTER EXISTS(
    FOR o IN orders FILTER o.user_id == u.id RETURN o
) RETURN u"

// Correlated subquery
"FOR u IN users RETURN {
    name: u.name,
    order_count: (
        FOR o IN orders
        FILTER o.user_id == u.id
        RETURN 1
    ) |> LENGTH
}"
```

**Optimization Techniques:**
```cpp
class SubqueryOptimizer {
public:
    // Convert IN subquery to semi-join
    JoinPlan decorrelate(const CorrelatedSubquery& subq);

    // Hoist uncorrelated subqueries to top level
    QueryPlan hoistIndependent(const ParsedQuery& query);

    // Merge nested aggregations
    AggregateSpec mergeAggregates(
        const std::vector<AggregateExpr>& nested);

    // Pull up correlated predicates
    std::vector<Expression> pullUpFilters(
        const CorrelatedSubquery& subq);
};

// Example: IN → Semi-Join transformation
// Before:
"FOR u IN users FILTER u.id IN (SELECT user_id FROM orders) RETURN u"

// After optimization:
"FOR u IN users
 FOR o IN orders
 FILTER u.id == o.user_id
 COLLECT id = u.id INTO g
 RETURN g[0].u"
```

**Performance Impact:**
- IN subquery: 10-100x faster (avoid repeated execution)
- EXISTS: 5-50x faster (early termination)
- Correlated: 100-1000x faster (decorrelation)

---

## Performance Optimizations

### Predicate Pushdown to Storage Layer
**Priority:** High
**Target Version:** v1.7.0

Push filters directly to RocksDB iterators for early pruning.

**Current:**
```
RocksDB Iterator → Read all rows → Filter in query engine
```

**Optimized:**
```
RocksDB Iterator with filter → Only read matching rows
```

**Implementation:**
```cpp
// Custom RocksDB filter
class PredicateFilter : public rocksdb::FilterPolicy {
public:
    bool KeyMayMatch(const Slice& key, const Slice& filter) const override;
};

// Bloom filter for common predicates
auto bloom = std::make_shared<PredicateFilter>(predicates);
read_options.filter_policy = bloom;

// 10-50x fewer rows read from storage
```

**Benefits:**
- Reduce I/O by 10-100x
- Lower CPU usage (no unnecessary deserialization)
- Better for range queries

---

### Index-Only Scans (Covering Indexes)
**Priority:** Medium
**Target Version:** v1.7.0

Satisfy queries entirely from index without accessing base table.

**Example:**
```sql
-- Create index on (name, age)
CREATE INDEX idx_name_age ON users (name, age);

-- Query can be satisfied by index alone
FOR u IN users
FILTER u.name > 'A'
RETURN {name: u.name, age: u.age}
-- No need to access base table!
```

**Implementation:**
```cpp
class CoveringIndexScan {
public:
    // Check if index covers query
    bool isCovering(const SecondaryIndex& index,
                   const std::vector<std::string>& required_fields);

    // Scan index directly
    Result<std::vector<IndexEntry>> scanIndexOnly(
        const SecondaryIndex& index,
        const Expression& filter,
        const std::vector<std::string>& projections);
};
```

**Performance:**
- 5-20x faster (no table lookup)
- Lower I/O and cache pressure

---

### Late Materialization
**Priority:** Medium
**Target Version:** v1.7.0

Defer fetching full rows until after filtering and projection.

**Strategy:**
```
Traditional:
Read rows → Deserialize all fields → Filter → Project

Late Materialization:
Read keys → Filter on indexed fields → Fetch only matching rows
```

**Implementation:**
```cpp
// Phase 1: Filter on index (only keys)
auto matching_keys = index.scan(filter);

// Phase 2: Fetch only required fields for matching keys
for (const auto& key : matching_keys) {
    auto row = storage.getPartial(key, required_fields);
    results.push_back(row);
}
```

**Benefits:**
- 2-10x faster for selective queries
- Lower memory usage
- Better cache locality

---

### Adaptive Statistics Collection
**Priority:** Medium
**Target Version:** v1.7.0

Continuously update statistics for better cost estimates.

**Metrics:**
- Cardinality per table/collection
- Column value distributions (histograms)
- Index selectivity
- Correlation between columns
- Data skew detection

**API:**
```cpp
class StatisticsCollector {
public:
    // Collect statistics incrementally
    void sampleTable(const std::string& table,
                    size_t sample_size = 10000);

    // Histogram for range estimation
    Histogram buildHistogram(const std::string& table,
                            const std::string& column,
                            size_t num_buckets = 256);

    // Detect correlations
    double estimateCorrelation(const std::string& col1,
                              const std::string& col2);

    // Update on data changes
    void onInsert(const std::string& table, const BaseEntity& entity);
    void onDelete(const std::string& table, const std::string& key);
};

// Example: Use histogram for range query estimation
auto hist = stats.buildHistogram("users", "age", 100);
size_t estimated = hist.estimateRange(25, 35);  // age BETWEEN 25 AND 35
```

**Sampling Strategy:**
- Reservoir sampling for large tables
- Update on 1% of inserts/deletes
- Full refresh every 24 hours

---

### Query Rewrite Rules
**Priority:** Medium
**Target Version:** v1.7.0

Algebraic query transformations for optimization.

**Rules:**
```cpp
class QueryRewriter {
public:
    // Predicate pushdown
    QueryPlan pushFilters(const QueryPlan& plan);

    // Projection pushdown (early pruning)
    QueryPlan pushProjections(const QueryPlan& plan);

    // Join reordering
    QueryPlan reorderJoins(const QueryPlan& plan);

    // Common subexpression elimination
    QueryPlan eliminateCommonSubexpressions(const QueryPlan& plan);

    // Constant folding
    Expression foldConstants(const Expression& expr);
};

// Example: Filter pushdown
// Before:
"FOR u IN users FOR o IN orders FILTER o.user_id == u.id AND u.age > 30"

// After:
"FOR u IN users FILTER u.age > 30 FOR o IN orders FILTER o.user_id == u.id"
// Filter on users first (more selective)
```

**Impact:** 10-50% query speedup

---

## Refactoring Opportunities

### Unified Expression Evaluator
**Priority:** Medium
**Target Version:** v1.8.0

Consolidate expression evaluation logic scattered across multiple files.

**Current State:**
- `query_engine.cpp`: Expression evaluation for filters
- `let_evaluator.cpp`: LET clause evaluation
- `window_evaluator.cpp`: Window function expressions
- `statistical_aggregator.cpp`: Aggregate expressions

**Proposed:**
```cpp
class ExpressionEvaluator {
public:
    // Single entry point for all expression evaluation
    Result<Value> evaluate(
        const Expression& expr,
        const EvaluationContext& ctx);

    // Context holds variables, functions, current row
    struct EvaluationContext {
        std::unordered_map<std::string, Value> variables;
        FunctionRegistry& functions;
        const nlohmann::json* current_row;
        QueryEngine* engine;  // For subqueries
    };

    // Optimization: Compile expression to bytecode
    std::shared_ptr<CompiledExpression> compile(const Expression& expr);
    Result<Value> evaluateCompiled(
        const CompiledExpression& compiled,
        const EvaluationContext& ctx);
};
```

**Benefits:**
- Consistent behavior across all expression contexts
- Single point for optimization
- Easier to add new expression types
- Testability

---

### Plan Serialization & Debugging
**Priority:** Low
**Target Version:** v1.8.0

Serialize query plans for debugging and analysis.

**Features:**
```cpp
class QueryPlan {
public:
    // Serialize to JSON
    nlohmann::json toJSON() const;

    // Visualize as tree
    std::string toGraphviz() const;

    // Explain output (like PostgreSQL EXPLAIN)
    std::string explain(bool analyze = false) const;

    // Compare two plans
    static PlanDiff compare(const QueryPlan& p1, const QueryPlan& p2);
};

// Example: EXPLAIN output
auto plan = optimizer.optimize(query);
std::cout << plan.explain(true) << std::endl;

/*
Output:
Aggregate  (cost=1245.23 rows=100)
  -> Hash Join  (cost=1100.00 rows=5000)
      Hash Cond: (o.user_id = u.id)
      -> Seq Scan on orders o  (cost=0.00 rows=10000)
            Filter: o.total > 1000
      -> Seq Scan on users u  (cost=0.00 rows=1000)
*/
```

---

### Query Parser Refactoring
**Priority:** Low
**Target Version:** v1.8.0

Modernize AQL parser with better error messages and recovery.

**Current Issues:**
- Generic error messages ("Parse error at line 5")
- No error recovery (first error aborts)
- Manual recursive descent (hard to maintain)

**Proposed:**
```cpp
// Use parser generator (ANTLR4 or Bison)
grammar AQL;

query: forClause filterClause* returnClause;
forClause: 'FOR' IDENTIFIER 'IN' expression;
filterClause: 'FILTER' expression;
// ... rest of grammar

// Better errors
ParseError {
    size_t line;
    size_t column;
    std::string message;
    std::string suggestion;  // "Did you mean FILTER?"
    std::string snippet;     // Show problematic line
};

// Error recovery
auto result = parser.parse(query);
if (!result.errors.empty()) {
    for (const auto& err : result.errors) {
        std::cerr << "Line " << err.line << ":" << err.column
                  << " " << err.message << std::endl;
        std::cerr << err.snippet << std::endl;
        if (!err.suggestion.empty()) {
            std::cerr << "Suggestion: " << err.suggestion << std::endl;
        }
    }
}
```

---

### SQL Parser Module
**Priority:** Medium
**Target Version:** v2.0.0

Add native SQL query support as an alternative query interface alongside AQL, enabling users
familiar with standard SQL syntax to interact with ThemisDB without learning AQL.

**Motivation:**
- Lower adoption barrier for users coming from relational database backgrounds
- Unified Query Interface: support for multiple query languages (AQL, SQL, GraphQL)
- Enables integration with existing SQL-based tooling and BI connectors

**Planned Features:**
- Standard SQL SELECT/INSERT/UPDATE/DELETE syntax
- SQL-to-AQL transpiler (translate SQL queries into AQL for execution)
- Support for SQL JOINs, subqueries, GROUP BY, ORDER BY, HAVING
- Compatibility layer for common SQL dialects (PostgreSQL, MySQL)
- Plugin architecture for additional query languages (GraphQL, etc.)

**Architecture:**
```cpp
class SQLParser {
public:
    // Parse a SQL query string into an internal AST
    Result<SQLASTNode> parse(const std::string& sql_query);
};

class SQLToAQLTranspiler {
public:
    // Translate a SQL AST into an AQL query string for execution
    Result<std::string> transpile(const SQLASTNode& sql_ast);
};

// Example usage
SQLParser sql_parser;
SQLToAQLTranspiler transpiler;

auto sql_ast = sql_parser.parse("SELECT name, age FROM users WHERE age > 30 ORDER BY name");
auto aql_query = transpiler.transpile(sql_ast.value());
// aql_query: "FOR users IN users FILTER users.age > 30 SORT users.name RETURN {name: users.name, age: users.age}"
auto result = executeAql(aql_query.value(), engine);
```

**Implementation Notes:**
- Implement as a standalone module (`src/query/sql_parser.cpp`) separate from AQL parser
- Full unit and integration test coverage required before merging
- Performance and security review mandatory
- See also: top-level `ROADMAP.md` for cross-module roadmap context

---

## Known Issues

### Semantic Cache False Positives
**Status:** Open
**Severity:** Low
**Affects:** v1.5.0+

Semantic cache may return incorrect results for queries with similar syntax but different semantics.

**Example:**
```
Query 1: "FOR u IN users FILTER u.age > 30 RETURN u"
Query 2: "FOR u IN users FILTER u.age < 30 RETURN u"
```

Embeddings may be too similar, causing cache hit for Query 2 using Query 1's results.

**Workaround:**
- Increase similarity threshold (current: 0.85 → 0.95)
- Disable semantic cache for critical queries
- Use query fingerprinting instead

**Proposed Fix:**
- Embed operators as distinct tokens (>, <, ==, !=)
- Include predicate values in embedding
- Add exact predicate match verification step

---

### CTE Spill-to-Disk Performance
**Status:** Open
**Severity:** Medium
**Affects:** v1.4.0+

CTE spill-to-disk degrades performance significantly when memory pressure is high.

**Symptoms:**
- 10-100x slowdown when CTE exceeds memory budget
- Disk I/O becomes bottleneck

**Current Mitigation:**
- Increase CTE memory budget
- Use streaming queries instead of CTEs

**Proposed Fix:**
- Compressed spill files (Snappy)
- Async I/O for background spilling
- Columnar format for spill files

---

### OR Query Performance
**Status:** Open
**Severity:** Medium
**Affects:** All versions

OR queries execute disjuncts sequentially, leading to poor performance for many ORs.

**Example:**
```sql
FOR u IN users
FILTER u.city == 'Berlin' OR u.city == 'Munich' OR u.city == 'Hamburg'
RETURN u
```

Each disjunct executes independently, then results are unioned.

**Current Performance:** O(n * d) where n = rows, d = disjuncts
**Desired Performance:** O(n)

**Proposed Fix:**
- Rewrite to `FILTER u.city IN ['Berlin', 'Munich', 'Hamburg']`
- Parallel disjunct execution
- Bitmap OR for indexed predicates

---

### Query Optimizer Statistics Drift
**Status:** Open
**Severity:** Medium
**Affects:** v1.4.0+

Statistics used for cost estimation become stale over time, leading to suboptimal plans.

**Symptoms:**
- Query performance degrades over time
- Plans don't adapt to data growth

**Workaround:**
- Manual statistics refresh
- Restart database (recomputes stats)

**Proposed Fix:**
- Continuous statistics collection
- Adaptive re-optimization on misestimates

---

## Research Areas

### Machine Learning for Query Optimization
**Status:** Research
**Timeline:** 2025

Use ML models to predict optimal query plans.

**Approaches:**
- **Learned Cost Models**: Neural network predicts operator costs
- **Reinforcement Learning**: Learn join ordering from query workload
- **Cardinality Estimation**: Deep learning for selectivity prediction

**Challenges:**
- Training data collection
- Model inference latency (<10ms)
- Handling novel queries (cold start)

**References:**
- Neo: Learned Query Optimizer (Halo et al. 2019)
- Bao: Bandit Optimizer (Marcus et al. 2020)

---

### Approximate Query Processing
**Status:** Research
**Timeline:** 2025-2026

Trade accuracy for speed using sampling and sketches.

**Techniques:**
- **Online Aggregation**: Return approximate results incrementally
- **Sampling**: Process 1% of data, extrapolate results
- **Sketches**: HyperLogLog (DISTINCT), t-Digest (percentiles)

**Example:**
```sql
-- Approximate COUNT DISTINCT (1% error, 100x faster)
FOR u IN users
COLLECT AGGREGATE cnt = HLL(u.id)
RETURN cnt

-- Approximate PERCENTILE (99x faster)
FOR u IN users
COLLECT AGGREGATE p95 = TDIGEST(u.age, 0.95)
RETURN p95
```

**Use Cases:**
- Dashboards (real-time analytics)
- Data exploration
- Monitoring/alerting

---

### Adaptive Query Processing
**Status:** Research
**Timeline:** 2025

Adjust execution strategy at runtime based on actual data distribution.

**Features:**
- **Mid-Query Re-Optimization**: Switch plans if cardinality estimates are wrong
- **Adaptive Joins**: Switch join algorithm during execution
- **Progressive Optimization**: Refine plan as more data is processed

**Example:**
```
Estimated: 1000 rows from left side of join
Actual: 1,000,000 rows

Action: Switch from hash join to sort-merge join
```

---

### Federated Query Processing
**Status:** Research
**Timeline:** 2025-2026

Query across heterogeneous data sources (PostgreSQL, MongoDB, S3, APIs).

**Architecture:**
```
ThemisDB Query Engine
    ↓
┌───────────┬──────────┬─────────┬─────────┐
│PostgreSQL │ MongoDB  │   S3    │  REST   │
│ Adapter   │ Adapter  │ Adapter │ Adapter │
└───────────┴──────────┴─────────┴─────────┘
```

**Challenges:**
- Schema mapping
- Predicate pushdown per source
- Join across heterogeneous sources
- Cost estimation without statistics

---

## Migration Paths

### v1.5 → v1.6 (Columnar Engine)
**Breaking Changes:** None (opt-in)

**Migration:**
```cpp
// Enable columnar execution for analytics
QueryEngine::Config config;
config.enable_columnar = true;
config.columnar_threshold_rows = 10000;  // Use columnar for >10K rows

auto engine = std::make_unique<QueryEngine>(storage, config);
```

**Performance:** 5-20x faster aggregations and scans

---

### v1.6 → v1.7 (Parallel Execution)
**Breaking Changes:** None

**Migration:**
```cpp
// Parallel execution enabled by default
QueryEngine::Config config;
config.max_parallel_threads = 8;
config.min_parallel_rows = 10000;  // Parallel for >10K rows

auto engine = std::make_unique<QueryEngine>(storage, config);
```

**Performance:** 2-5x faster on multi-core systems

---

### v1.7 → v1.8 (JIT Compilation)
**Breaking Changes:** Requires LLVM 15+

**Migration:**
```cpp
// JIT enabled by default for hot queries
QueryEngine::Config config;
config.enable_jit = true;
config.jit_hot_threshold = 100;  // Compile after 100 executions

auto engine = std::make_unique<QueryEngine>(storage, config);
```

**Performance:** 5-10x faster for frequently executed queries

---

## Community Contributions

### Wanted: Additional AQL Functions
**Difficulty:** Easy
**Mentor:** Core team

Implement additional built-in functions for AQL.

**Examples:**
- String: `LEVENSHTEIN_DISTANCE()`, `SOUNDEX()`
- Math: `BITWISE_AND()`, `BITWISE_OR()`
- Date: `TIMEZONE_CONVERT()`, `BUSINESS_DAYS()`
- ML: `COSINE_SIMILARITY()`, `KMEANS_PREDICT()`

**Getting Started:**
1. Fork repository
2. Implement function in `src/query/functions/`
3. Add tests in `tests/query/functions/`
4. Update documentation in `docs/aql/functions/`
5. Submit PR

---

### Wanted: Query Optimizer Benchmarks
**Difficulty:** Medium
**Mentor:** Query team

Build comprehensive benchmark suite for query optimizer.

**Scope:**
- TPC-H queries
- TPC-DS queries
- Custom workloads (graph, vector, spatial)
- Cardinality estimation accuracy
- Join ordering quality

**Deliverables:**
- Benchmark harness
- Dataset generators
- Performance baselines
- Regression tests

---

### Wanted: Visual Query Builder
**Difficulty:** Medium-Hard
**Mentor:** UI team

Web-based visual query builder for AQL.

**Features:**
- Drag-and-drop query construction
- Visual joins and filters
- Live query preview
- Export to AQL text

**Tech Stack:**
- React + TypeScript
- Cytoscape.js (graph visualization)
- Monaco Editor (code editor)

---

### Wanted: Query Profiler
**Difficulty:** Hard
**Mentor:** Performance team

Low-overhead query profiler for production environments.

**Features:**
- Per-operator timing
- Memory allocation tracking
- I/O statistics
- Flame graphs
- Export to Chrome Trace format

**Similar Projects:**
- PostgreSQL auto_explain
- MySQL Performance Schema
- Oracle SQL Trace

---

## Related Documentation

- [Query Module Architecture](../README.md) - Current architecture and components
- [Storage Module](../../storage/README.md) - Underlying storage layer
- [Index Module](../../index/README.md) - Index types and usage
- [AQL Language Reference](../../../aql/README.md) - AQL syntax and semantics
- [Query Optimization Guide](../../../docs/en/query-optimization.md) - Performance tuning
- [Distributed Queries](../../../docs/en/distributed-queries.md) - Multi-node execution
- [Caching Strategies](../../../docs/en/caching.md) - Cache configuration
- [Performance Benchmarks](../../../BENCHMARK_BEST_PRACTICES.md) - Benchmark results

---

## Test Strategy

- Unit test coverage ≥ 80% for all `AQLParser`, `QueryOptimizer`, `QueryExecutor`, `QueryCache`, and `UDFRegistry` code paths
- Parser round-trip tests: parse → AST → unparse → re-parse; assert AST equality for all 100+ built-in function signatures
- Optimizer correctness tests: compare estimated vs. actual row counts on 20 synthetic datasets; assert cost error < 30%
- Cache hit-rate tests: replay 10,000 query log entries and assert exact-match cache hit rate ≥ 95% for repeated queries
- UDF hot-reload tests: register, execute, unregister, and re-register a UDF without restarting; assert zero crashes under concurrent load
- Federation timeout tests: inject a slow cluster (> 2 s response) and assert partial results are returned within the configured timeout
- Resource limit tests: execute queries that would exceed `max_rows`, `max_memory`, and `timeout`; assert clean cancellation without memory leaks
- SQL compatibility negative tests: send all unsupported SQL constructs (JOINs, subqueries, DDL) and assert structured `AQLParseException` is returned

## Performance Targets

- Query parse + optimize latency: ≤ 5 ms at p99 for queries referencing ≤ 10 collections on a 16-core host
- Query execution throughput: ≥ 10,000 simple AQL queries/s at p99 < 20 ms on a 3-node cluster with warm cache
- Exact-match cache lookup latency: ≤ 1 ms at p99 under 10,000 concurrent clients
- Semantic cache lookup latency: ≤ 10 ms at p99 (includes embedding similarity comparison)
- JIT compilation: first-compile latency ≤ 50 ms; subsequent execution ≥ 3× faster than interpreter for arithmetic-heavy expressions
- Federation plan overhead: cost estimation for a 5-cluster plan in ≤ 20 ms; parallel execution via `std::async`
- Streaming result first-chunk latency: ≤ 50 ms from query submission to first result page returned
- Vectorized execution (column batch): ≥ 5× throughput improvement vs. row-at-a-time for aggregation queries on > 1 M rows

## Security / Reliability

- AQL injection prevention: all user-supplied string literals must be parameterized or escaped; `AQLParser` must reject any attempt to embed raw AQL via string interpolation
- Resource exhaustion: `max_rows`, `max_memory`, and `timeout` limits are enforced before query execution begins and re-checked at every 1,000-row batch boundary
- UDF sandboxing: user-defined functions must run with restricted system-call access; WASM UDFs are isolated in a separate memory arena
- Distributed federation: each cluster call is authenticated via mTLS; unauthenticated federation endpoints must return HTTP 403
- Query cancellation must guarantee that all allocated memory is released and all acquired locks are freed within 100 ms of the cancellation signal
- SQL dialect transpiler must never silently produce wrong results for unsupported constructs; it must return a structured error with the unsupported feature name
- AQL parser must enforce a maximum AST depth of 256 to prevent stack-overflow from deeply nested subqueries

---

## Security Hardening Backlog (Q2 2026)

> GAP-021 + GAP-022 – identified via static analysis (2026-04-21).
> Reference: `docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md`.

### GAP-021 – Server-Side Cap for `max_frontier_size` and `max_results`

**Scope:** `src/server/query_api_handler.cpp:531–532`

### Design Constraints
- Caps must be configurable via `THEMIS_MAX_FRONTIER_SIZE` and `THEMIS_MAX_RESULTS` env vars
- Default caps: `max_frontier_size = 500,000`, `max_results = 100,000`
- Caps must be enforced **after** reading user value; user cannot exceed server cap

### Required Interfaces
```cpp
static constexpr size_t kDefaultServerMaxFrontier = 500'000;
static constexpr size_t kDefaultServerMaxResults  = 100'000;

size_t server_frontier_cap = config_.max_frontier_size > 0
    ? config_.max_frontier_size : kDefaultServerMaxFrontier;
max_frontier_size = std::min(max_frontier_size, server_frontier_cap);
max_results = std::min(max_results, server_results_cap);
```

### Test Strategy
- Unit test: request with `max_frontier_size=999999999` → capped to server limit, no OOM
- Unit test: server limit configurable via env var
- Integration test: large graph traversal completes within memory bound

### Performance Targets
- Cap enforcement: O(1), negligible overhead

---

### GAP-022 – Enforce Default Memory Limit for AQL Queries

**Scope:** `src/server/query_api_handler.cpp:537`, `src/query/aql_runner.cpp:826`

### Design Constraints
- Default memory limit: 256 MB (configurable via `THEMIS_MAX_QUERY_MEMORY_BYTES`)
- Explicit `{"max_memory_bytes": 0}` from user must be treated as "use server default",
  not "no limit"
- Admins can set the limit higher than the default (e.g., for analytical workloads)

### Required Interfaces
```cpp
static constexpr size_t kDefaultMaxQueryMemoryBytes = 256ULL * 1024 * 1024; // 256 MB
if (resource_limits.max_memory_bytes == 0) {
    resource_limits.max_memory_bytes = config_.max_query_memory_bytes > 0
        ? config_.max_query_memory_bytes : kDefaultMaxQueryMemoryBytes;
}
```

### Test Strategy
- Unit test: omit `max_memory_bytes` → 256 MB limit applied
- Unit test: `{"max_memory_bytes": 0}` → 256 MB limit applied (not unlimited)
- Unit test: `{"max_memory_bytes": 134217728}` → 128 MB limit applied
- Integration test: query producing > 256 MB result set → 413 error

### Performance Targets
- The memory check in `aql_runner.cpp` is O(1) per result batch; no measurable overhead


---

## Continuous Query Language (CQL) — Phase 8

> **Research foundation:** [CQL — Arasu, Babu & Widom (2006)](../../research/papers/arasu_cql_2006.md) · [Best Practice: Continuous Query Sliding Windows](../../research/best_practices/continuous_query_sliding_window.md)

### Scope

Extend the AQL query engine with a CQL-compliant standing query sub-system. Standing queries are registered once (`CREATE CONTINUOUS QUERY`) and evaluated continuously as new data arrives, delivering incremental results to subscribers via SSE/WebSocket push. Affected files:

- `include/query/continuous_query_engine.h` — public API
- `include/query/window_spec.h` — `WindowSpec` (time, count, tumbling)
- `include/query/continuous_query_registry.h` — `ContinuousQueryInfo`
- `src/query/continuous_query_engine.cpp` — evaluation loop
- `src/query/continuous_query_planner.cpp` — `ContinuousPlan` generation
- `src/query/continuous_query_registry.cpp` — RocksDB-backed registry
- `src/query/synopsis_store.cpp` — RocksDB ring buffer per window
- `src/query/incremental_agg.cpp` — delta-based SUM/AVG/MIN/MAX/COUNT
- `src/query/cq_watermark.cpp` — late-data detection, correction deltas
- `src/query/aql_parser.cpp` — DDL: `CREATE / DROP / SHOW CONTINUOUS QUERY`

### Design Constraints

- [ ] Bounded windows only: every continuous query must declare an upper bound on window size; unbounded stream-stream joins rejected at parse time with `UNBOUNDED_JOIN_WINDOW`
- [ ] UDF purity: UDFs called inside a continuous query body must be declared pure; impure UDFs rejected with `IMPURE_UDF_IN_CONTINUOUS_QUERY`
- [ ] Max window size defaults: 10 M tuples OR 1 GB, whichever is smaller; operator-configurable
- [ ] Late-data budget: `allowed_lateness_ms` per query (default 500 ms); events arriving after budget counted in `late_dropped_events_total` Prometheus counter
- [ ] Crash recovery: registry persisted in RocksDB; synopsis WAL enables resume within one scheduler tick after node restart
- [ ] Result queue: bounded per `(query_name, subscriber_id)`; overflow drops oldest entries and emits `SUBSCRIBER_QUEUE_OVERFLOW` warning

### Required Interfaces

```cpp
// include/query/continuous_query_engine.h
namespace themis {

enum class ResultMode { DELTA, SNAPSHOT, CHANGES };

struct WindowSpec {
    enum class Type { TIME_SLIDING, COUNT_SLIDING, TUMBLING } type;
    int64_t range_ms;    // TIME_SLIDING and TUMBLING
    int64_t slide_ms;    // TIME_SLIDING
    int64_t rows;        // COUNT_SLIDING
    int64_t slide_rows;  // COUNT_SLIDING
    std::string partition_by;  // optional, COUNT_SLIDING
};

struct ContinuousQuerySpec {
    std::string name;
    std::string source_collection;
    WindowSpec window;
    std::string aql_body;
    ResultMode result_mode;
    int64_t allowed_lateness_ms{500};
    size_t max_window_tuples{10'000'000};
    size_t max_window_bytes{1ULL << 30};  // 1 GB
};

struct ContinuousQueryInfo {
    std::string name;
    std::string source_collection;
    WindowSpec window;
    ResultMode result_mode;
    std::chrono::system_clock::time_point registered_at;
    std::chrono::system_clock::time_point last_tick_at;
    uint64_t tuples_processed{0};
    size_t result_queue_depth{0};
};

class ContinuousQueryEngine {
public:
    using ContinuousQueryHandle = std::string;
    using ResultStreamPtr = std::shared_ptr<ResultStream>;

    virtual ~ContinuousQueryEngine() = default;

    virtual ContinuousQueryHandle registerQuery(ContinuousQuerySpec spec) = 0;
    virtual void dropQuery(const std::string& name) = 0;
    virtual std::vector<ContinuousQueryInfo> listQueries() const = 0;
    virtual ResultStreamPtr subscribe(const std::string& name, ResultMode mode) = 0;
};

}  // namespace themis
```

### Implementation Notes

- **Evaluation loop**: `ContinuousQueryEngine` calls `AggregateScheduler::schedule(tick_interval_ms, callback)` per registered query; callback invokes `ContinuousQueryPlanner::evaluate(plan, synopsis_store)` which computes the delta and emits results.
- **Synopsis storage**: `SynopsisStore` wraps a RocksDB column family with key `<query_name>:<partition_key>:<event_ts_us>` and value `<serialised tuple>`. The `expire()` method deletes entries older than `window_start`.
- **Incremental aggregation**: `IncrementalAgg<Op>` maintains a running aggregate updated by `add(tuple)` and `remove(tuple)`. For `MIN`/`MAX`, falls back to full synopsis scan when the evicted tuple equals the current extremum.
- **Result delivery**: `ResultQueue` uses a bounded `std::deque` protected by `std::mutex`; `ResultStream::next()` blocks with a configurable timeout; the SSE endpoint polls `next()` in a loop, flushing each delta as an `event: data\n\n` SSE frame.
- **Cross-shard queries**: `ContinuousQueryPlanner` detects sharded sources and generates a `ScatterGatherPlan`: one sub-query per shard + a `MergeAggNode` on the coordinator. Coordinator emits the merged delta to subscribers.

### Test Strategy

- **Unit (CQ-01..CQ-20)** — `tests/test_continuous_query_engine.cpp`
  - Window tick computation, synopsis insert/expire/size enforcement
  - Incremental SUM/AVG/MIN/MAX delta correctness vs. full re-scan reference
  - Watermark advancement; late-data correction within one tick
  - DELTA/SNAPSHOT/CHANGES result mode output verification
  - Validation rejections: unbounded join, impure UDF, unknown source
- **Integration (CQI-01..CQI-05)** — `tests/integration/test_continuous_query_e2e.cpp`
  - End-to-end: register → inject events → verify SSE delta stream content
  - Multi-subscriber fan-out
  - Late event within `allowed_lateness_ms`: correction delta emitted
  - Client disconnect + reconnect: buffered deltas delivered
  - Node restart: registry reloaded; evaluation resumes within one tick

### Performance Targets

| Metric | Target | Condition |
|--------|--------|-----------|
| Throughput | ≥ 500 k tuples/s | 1 sliding time-window query, 4-core host |
| Per-tuple p99 latency | ≤ 5 ms | Ingest → window update → SSE delivery |
| Empty-window tick overhead | ≤ 1 µs | No new events in evaluation interval |
| Concurrent active queries | ≥ 1 000 | Mixed window types, single node |
| Watermark correction latency | ≤ 2 × tick_interval | Late event within `allowed_lateness_ms` |

### Security / Reliability

- AQL body inside a continuous query is subject to the same AQL injection prevention (parameterised literals, max AST depth = 256) as regular queries
- `ContinuousQueryRegistry` persists to RocksDB with WAL; no lost registrations on crash
- Result queues are bounded; slow consumers cannot cause unbounded memory growth
- `SUBSCRIBER_QUEUE_OVERFLOW` emitted as structured log warning with `query_name` and `subscriber_id` fields for operator observability

---

## QueryFederation Broadcast Join (Target: future milestone — stub removal)

**Stub:** `src/query/query_federation.cpp` broadcast-join path — returns metadata JSON only; no actual join rows.  
**Risk:** Federated broadcast joins appear to succeed but return zero results.

### Scope
- Fetch the smaller ("broadcast") table completely via `shard_router_->executeQuery()`.
- Broadcast the fetched rows to all shard executors via a new `broadcastQuery()` method.
- Collect co-located join results per shard; merge into unified result set.
- Return actual document rows (not just strategy metadata).

### Performance Targets
- Broadcast join for small table ≤ 10 000 rows: P99 ≤ 50 ms on 5-shard LAN cluster.

---

## QueryFederation Shuffle Join (Target: future milestone — stub removal)

**Stub:** `src/query/query_federation.cpp` shuffle-join path — returns metadata JSON only; no actual join rows.  
**Risk:** Federated shuffle joins appear to succeed but return zero results.

### Scope
- Repartition both sides by join key hash via `shard_router_->shufflePartition()`.
- Execute equi-joins per receiving shard; collect and merge per-shard results.
- Return actual document rows.

### Performance Targets
- Shuffle join for 1 M × 500 K rows (5 shards): P99 ≤ 2 s on GbE LAN.

---

## Query Engine Path Reconstruction (Target: future milestone — stub removal)

**Stub:** `src/query/query_engine.cpp` spatial path queries — all paths are trivial 2-hop (start→end); full BFS path reconstruction missing.  
**Risk:** Multi-hop path queries return incorrect results; intermediate nodes silently discarded; shortest-path semantics are wrong.

### Scope
- Modify `executeSpatialBFS()` to track `parent[node] = predecessor` during traversal.
- Reconstruct full path via backtracking: `node → parent[node] → … → start_node`.
- Push the full ordered path vector into `allPaths` instead of the trivial `{start, end}` pair.
