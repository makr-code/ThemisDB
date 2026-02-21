# Query Module - Future Enhancements

## Planned Features

### Query Compilation & JIT
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

### Columnar Execution Engine
**Priority:** High  
**Target Version:** v1.7.0

Vectorized columnar execution for analytical queries, inspired by DuckDB and ClickHouse.

**Features:**
- Columnar data layout in memory
- Vectorized operators (1024 tuples/batch)
- Late materialization
- Columnar compression (dictionary, RLE, bit-packing)
- Adaptive row/columnar switching

**Architecture:**
```cpp
class ColumnarExecutionEngine {
public:
    struct ColumnBatch {
        std::vector<std::shared_ptr<Column>> columns;
        size_t row_count;
        SelectionVector selection;  // Filtered rows
    };
    
    // Execute query in columnar mode
    Result<std::vector<ColumnBatch>> executeColumnar(
        const QueryPlan& plan,
        const ExecutionContext& ctx);
    
    // Operators work on batches
    ColumnBatch filterBatch(const ColumnBatch& input, 
                           const Expression& predicate);
    ColumnBatch projectBatch(const ColumnBatch& input,
                            const std::vector<Expression>& projections);
    ColumnBatch aggregateBatch(const ColumnBatch& input,
                              const AggregateSpec& spec);
};

// Example: Vectorized filter
void filterColumn(const int64_t* input, int64_t threshold,
                 SelectionVector& output, size_t count) {
    // SIMD comparison (8 values at once)
    __m256i thresh_vec = _mm256_set1_epi64x(threshold);
    for (size_t i = 0; i < count; i += 4) {
        __m256i vals = _mm256_loadu_si256((__m256i*)(input + i));
        __m256i cmp = _mm256_cmpgt_epi64(vals, thresh_vec);
        int mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
        // Write selected indices
        if (mask & 1) output.push_back(i);
        if (mask & 2) output.push_back(i + 1);
        if (mask & 4) output.push_back(i + 2);
        if (mask & 8) output.push_back(i + 3);
    }
}
```

**Use Cases:**
- OLAP queries (aggregations, scans)
- Large table joins
- GROUP BY with high cardinality
- Window functions

**Performance Targets:**
- 10-50x faster scans
- 5-20x faster aggregations
- 3-10x faster joins

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
- See also: top-level `roadmap.md` for cross-module roadmap context

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
