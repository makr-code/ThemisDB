# ThemisDB Query Module

**Status:** PRODUCTION_CANDIDATE  
**Phase:** 6 (Documentation & Acceptance) — ✅ COMPLETE  
**Last Updated:** 2026-08-10  
**Owner:** Query Engine Team

---

## Module Purpose

The Query module provides ThemisDB's AQL (Advanced Query Language) query engine, featuring a cost-based optimizer, multi-model execution pipeline, and comprehensive caching infrastructure. AQL is based on ArangoDB's query language but significantly extended to support multiple query paradigms including relational, document, graph, vector, spatial, and timeseries models. It translates AQL queries into optimized execution plans and executes them across all data models with hybrid query support. AQL mutations Phase 1-5 complete.

### About AQL

**AQL (Advanced Query Language)** is ThemisDB's multi-paradigm query language inspired by ArangoDB's AQL but extended with:
- **Multi-Model Support**: Seamlessly query relational tables, document collections, property graphs, vector embeddings, geospatial data, and timeseries
- **Hybrid Queries**: Combine vector similarity with geospatial filters, fulltext search with graph traversal, and more
- **SQL-Like Syntax**: Familiar declarative syntax with FOR-FILTER-SORT-RETURN pattern
- **Advanced Features**: Window functions, CTEs, recursive queries, subqueries, LLM integration
- **100+ Functions**: String, math, array, date, geo (ST_*), vector, graph, document, JSON, AI/ML, process mining, ethics, and more

**Compared to ArangoDB AQL:**
- ✅ Maintains core AQL syntax (FOR, FILTER, SORT, LIMIT, RETURN, LET, COLLECT)
- ✅ Compatible graph traversal syntax with enhanced spatial constraints
- ➕ Extended with vector similarity functions (SIMILARITY, COSINE_DISTANCE, L2_DISTANCE)
- ➕ Enhanced geospatial support (ST_* functions for complex spatial queries)
- ➕ LLM integration (LLM INFER, LLM RAG, LLM EMBED)
- ➕ Timeseries-specific syntax and window functions
- ➕ Advanced process mining and ethics functions
- ➕ Query federation across distributed databases

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `aql_parser.cpp` / `aql_parser_json.cpp` | AQL and JSON query syntax parsing; AST generation |
| `aql_translator.cpp` | AST → internal query representation |
| `query_optimizer.cpp` / `adaptive_optimizer.cpp` | Logical and cost-based plan optimization |
| `optimizer_cost_model.cpp` | Cardinality and selectivity estimation |
| `query_engine.cpp` | Physical execution (scan, filter, sort, aggregate, join) |
| `aql_runner.cpp` | Top-level query dispatch (AQL, SQL, RLS, limits, cancellation) |
| `sql_parser.cpp` | SQL SELECT/INSERT/UPDATE/DELETE → AQL compatibility layer |
| `sparql_parser.cpp` | SPARQL → AQL translation for RDF/knowledge-graph queries |
| `query_cache.cpp` / `query_cache_manager.cpp` | Exact-match query result cache |
| `semantic_cache.cpp` | Embedding-based similarity cache |
| `cte_cache.cpp` / `cte_subquery.cpp` / `materialized_cte.cpp` | CTE evaluation and caching |
| `cross_cluster_federation.cpp` | Distributed federated AQL across cluster endpoints |
| `query_federation.cpp` | Internal shard-level query federation |
| `vectorized_execution.cpp` | Column-store batch execution (SIMD via analytics layer) |
| `runtime_reoptimizer.cpp` | Adaptive re-optimization from runtime statistics |
| `query_plan_visualizer.cpp` | EXPLAIN / EXPLAIN ANALYZE plan output |
| `query_canceller.cpp` | Cooperative query cancellation via request ID |
| `result_stream.cpp` / `result_type_annotation.cpp` | Result streaming, pagination, type inference |
| `window_evaluator.cpp` / `statistical_aggregator.cpp` | Window and statistical aggregate functions |
| `let_evaluator.cpp` | LET variable binding evaluation |
| `workload_cache_strategy.cpp` | Adaptive workload-based cache eviction |
| `functions/` | 100+ AQL built-in functions (math, string, geo, vector, graph, AI, …) |

## Source Verification Evidence

- Parser and optimizer surfaces verified against `aql_parser.cpp`,
  `query_optimizer.cpp`, `adaptive_optimizer.cpp`, and `optimizer_cost_model.cpp`.
- Distributed/federated execution surfaces verified against
  `query_federation.cpp` and `cross_cluster_federation.cpp`.
- Runtime safety and cancellation references verified against
  `query_engine.cpp`, `query_canceller.cpp`, and `continuous_query_engine.cpp`.

## Scope

**In Scope:**
- AQL syntax parsing and AST generation
- Cost-based query optimization with adaptive learning
- Multi-model query execution (relational, document, graph, vector, spatial)
- Hybrid queries (vector+geo, fulltext+geo, graph+spatial)
- Query result streaming and pagination
- Query caching (exact, semantic, CTE)
- Expression evaluation and function registry (25+ categories)
- Subquery and CTE (Common Table Expression) support
- Graph traversal (BFS/DFS, shortest path, recursive queries)
- Window functions and statistical aggregation
- Distributed query federation

**Out of Scope:**
- Data storage and persistence (handled by storage module)
- Index structures and maintenance (handled by index module)
- Network protocol handling (handled by server module)
- Authentication decisions are provided by caller context; query execution enforces collection access checks when configured

## Key Components

> **Note on Parser Implementations:** The primary query parser is `aql_parser.cpp`
> (and `aql_parser_json.cpp` for JSON query objects). A separate SQL compatibility
> parser (`sql_parser.cpp`) translates basic SELECT/INSERT/UPDATE/DELETE into AQL.
> A SPARQL parser (`sparql_parser.cpp`) handles RDF / knowledge-graph queries.

### AQL Parser
**Location:** `aql_parser.cpp`, `../include/query/aql_parser.h`

Converts AQL query strings into Abstract Syntax Trees (AST) for execution.

**Features:**
- **AQL Syntax Support**: FOR, FILTER, SORT, LIMIT, RETURN, LET, COLLECT, WITH
- **Expression Types**: Binary/unary operators, literals, field access, function calls, arrays, objects
- **Specialized Expressions**:
  - `SimilarityCall`: Vector similarity search
  - `ProximityCall`: Geospatial proximity queries
  - `SubqueryExpr`: Nested subqueries
  - `AnyExpr`, `AllExpr`: Quantifier expressions (ANY/ALL)
- **AST Node Types**: QueryNode, ForNode, FilterNode, SortNode, LimitNode, ReturnNode, LetNode, CollectNode, WithNode
- **Error Recovery**: Detailed parse error messages with line/column information

**Thread Safety:**
- `AQLParser` is stateless; public parse methods build local tokenizer/parser state and can be called concurrently
- Parsed AST nodes are immutable and thread-safe for reading

**Usage Example:**
```cpp
#include "query/aql_parser.h"

// Parse a query
AQLParser parser;
std::string query = R"(
  FOR user IN users
    FILTER user.age > 30 AND user.city == 'Seattle'
    SORT user.name ASC
    LIMIT 10
    RETURN user
)";

auto result = parser.parse(query);
if (!result) {
  std::cerr << "Parse error: " << result.error() << std::endl;
  return;
}

auto ast = result.value();
// AST is ready for translation and execution
```

**Supported AQL Features:**
- Single and multi-collection FOR loops
- Complex boolean expressions (AND, OR, NOT)
- Arithmetic operators (+, -, *, /, %)
- Comparison operators (==, !=, <, >, <=, >=)
- IN operator for membership tests
- String, number, boolean, null literals
- Array and object literals
- Function calls with arbitrary arguments
- Field access with dot notation (user.address.city)

### Query Optimizer
**Location:** `query_optimizer.cpp`, `../include/query/query_optimizer.h`

Cost-based optimizer that reorders predicates and selects execution strategies for minimal resource usage.

**Features:**
- **Cost-Based Optimization**: Cardinality estimation and selectivity-based predicate ordering
- **Traditional Plan**: `chooseOrderForAndQuery()` - uses selectivity statistics
- **NLP-Enhanced Plan**: `chooseOrderForAndQueryWithNLP()` - incorporates semantic hints
- **Adaptive Optimization**: `adaptive_optimizer.cpp` - runtime statistics collection and adjustment
- **Hybrid Query Plans**:
  - Vector+Geo: SpatialThenVector vs VectorThenSpatial strategies
  - Fulltext+Geo: FulltextFirst vs SpatialFirst strategies
  - Graph Traversal: Cost estimation with branching factor and spatial constraints
- **Distributed Optimization**: Partition pruning, shard selection, parallelism tuning
- **Cost Model**: Histogram-based cardinality estimation (`optimizer_cost_model.cpp`)

**Thread Safety:**
- Read-safe: Statistics can be read concurrently
- Write-safe: Statistics updates use internal locking
- Optimizer instances are NOT thread-safe (create per-query)

**Configuration Example:**
```cpp
#include "query/query_optimizer.h"

QueryOptimizer optimizer;

// Configure cardinality estimation
optimizer.setEstimatedCardinality("user.age > 30", 1000);
optimizer.setEstimatedCardinality("user.city == 'Seattle'", 500);

// Optimize AND query
std::vector<Predicate> predicates = {
  Predicate::parse("user.age > 30"),
  Predicate::parse("user.city == 'Seattle'")
};

auto optimized_order = optimizer.chooseOrderForAndQuery(predicates);
// Executes predicates in order of increasing cardinality
```

**Optimization Strategies:**
- **Selectivity-Based**: Execute most selective predicates first
- **Index Awareness**: Prefer indexed predicates when available
- **Cost Model Integration**: Use histogram statistics for accurate estimates
- **Adaptive Learning**: Adjust estimates based on actual execution statistics

**Performance Characteristics:**
- Optimization overhead: 0.1-5ms for simple queries
- Complex queries (10+ predicates): 5-50ms optimization time
- Adaptive adjustment: 10-20% accuracy improvement over time
- Cost model evaluation: ~100μs per predicate

### Query Engine
**Location:** `query_engine.cpp`, `../include/query/query_engine.h`

Main execution hub that processes optimized queries against storage.

**Features:**
- **Predicate Execution**:
  - AND queries: `executeAndKeys()`, `executeAndEntities()` with sequential ordering
  - OR queries: `executeOrKeys()`, `executeOrEntities()` with optional fallback
- **Graph Traversal**:
  - `executeRecursivePathQuery()`: Multi-hop path queries with depth limits
  - `executeGeneralTraversal()`: BFS/DFS with filtering
- **Hybrid Multi-Model Queries**:
  - `executeVectorGeoQuery()`: Vector similarity + spatial constraints
  - `executeFilteredVectorSearch()`: Attribute pre/post-filtering
  - `executeRadiusVectorSearch()`: Epsilon-based neighbor search
  - `executeContentSearch()`: Fulltext + metadata filtering
  - `executeContentGeoQuery()`: Fulltext + spatial hybrid
- **Advanced Features**:
  - `executeJoin()`: Multi-FOR with LET/FILTER
  - `executeGroupBy()`: COLLECT/AGGREGATE operations
  - `executeCTEs()`: Common Table Expression support
- **Expression Evaluation**: Boolean evaluation with context binding

**Thread Safety:**
- Engine instances are thread-safe for concurrent query execution
- Uses read locks for concurrent access to shared state
- Each query gets isolated execution context

**Usage Example:**
```cpp
#include "query/query_engine.h"

QueryEngine engine(storage, index_manager);

// Execute a simple AND query
std::vector<Predicate> predicates = {
  Predicate::fieldEquals("age", 30),
  Predicate::fieldGreaterThan("salary", 50000)
};

auto result = engine.executeAndEntities("users", predicates);
for (const auto& entity : result.value()) {
  std::cout << entity.toJson() << std::endl;
}

// Execute a vector+geo hybrid query
VectorGeoQuery vgq;
vgq.collection = "places";
vgq.vector_field = "embedding";
vgq.query_vector = {0.1, 0.2, 0.3, ...};
vgq.k = 10;
vgq.center_lat = 47.6062;
vgq.center_lon = -122.3321;
vgq.radius_km = 5.0;

auto places = engine.executeVectorGeoQuery(vgq);
```

**Execution Strategies:**
- **Short-Circuit Evaluation**: Stop early when result set becomes empty
- **Lazy Evaluation**: Stream results without materializing full result set
- **Batch Processing**: Process entities in batches for efficiency
- **Cache Integration**: Automatic cache lookups and updates

**Performance Characteristics:**
- Simple queries (1-2 predicates): 1-10ms
- Complex queries (5-10 predicates): 10-100ms
- Graph traversal (depth 3-5): 50-500ms
- Hybrid queries (vector+geo): 10-50ms (depends on k and radius)

### Query Cache System
**Location:** `query_cache.cpp`, `query_cache_manager.cpp`, `semantic_cache.cpp`, `cte_cache.cpp`

Multi-tiered caching infrastructure for query results.

#### Query Cache (Exact Match)
**Features:**
- Exact string-based result caching with TTL
- LRU eviction policy
- Hit rate tracking and statistics
- Thread-safe concurrent access

**Configuration:**
```cpp
QueryCache cache;
cache.setMaxSize(1000);  // 1000 entries
cache.setDefaultTTL(std::chrono::seconds(300));  // 5 minutes

// Cache a result
std::string query = "FOR u IN users FILTER u.age > 30 RETURN u";
nlohmann::json result = execute_query(query);
cache.put(query, result);

// Retrieve from cache
if (auto cached = cache.get(query)) {
  return *cached;  // Cache hit
}
```

#### Semantic Cache (Similarity-Based)
**Features:**
- Embedding-based query similarity matching
- Configurable similarity threshold (default: 0.90)
- Matches semantically similar queries ("age > 30" ≈ "age >= 31")
- LRU eviction with TTL

**Usage:**
```cpp
SemanticCache semantic_cache(embedding_model);
semantic_cache.setSimilarityThreshold(0.90);

// Similar queries match
std::string query1 = "FOR u IN users FILTER u.age > 30 RETURN u";
std::string query2 = "FOR u IN users FILTER u.age >= 31 RETURN u";
// May hit semantic cache if similarity > 0.90
```

#### CTE Cache (Temporary Results)
**Features:**
- Memory-managed CTE result storage
- Automatic spill-to-disk (100MB default threshold)
- Transparent disk/memory access
- Reference counting and cleanup

**Configuration:**
```cpp
CTECache cte_cache;
cte_cache.setMemoryThreshold(100 * 1024 * 1024);  // 100MB
cte_cache.setTempDirectory("/tmp/themisdb_cte");

// Store CTE result
std::string cte_name = "temp_users";
std::vector<Entity> cte_result = execute_subquery(...);
cte_cache.put(cte_name, cte_result);

// Access CTE (transparent disk/memory)
auto cte_data = cte_cache.get(cte_name);
```

### Result Streaming
**Location:** `result_stream.cpp`, `../include/query/result_stream.h`

Lazy result evaluation with batching, pagination, and backpressure handling.

**Features:**
- **Pagination Strategies**:
  - `OFFSET_LIMIT`: Traditional offset/limit pagination
  - `CURSOR_BASED`: Opaque cursor for stateless pagination
  - `KEYSET`: Keyset pagination for stable ordering
- **Lazy Evaluation**: Results generated on-demand
- **Batch Processing**: Configurable batch size (default: 100 rows)
- **Backpressure Handling**: Slows query execution when consumer is slow
- **Statistics Tracking**: Rows returned, batch count, execution time

**Usage Example:**
```cpp
#include "query/result_stream.h"

ResultStream stream(query_engine, query);
stream.setBatchSize(100);
stream.setPaginationStrategy(PaginationStrategy::CURSOR_BASED);

// Stream results
while (stream.hasMore()) {
  auto batch = stream.nextBatch();
  for (const auto& row : batch) {
    process(row);
  }
}

// Get statistics
auto stats = stream.getStatistics();
std::cout << "Rows returned: " << stats.rows_returned << std::endl;
std::cout << "Execution time: " << stats.execution_time_ms << "ms" << std::endl;
```

### Function Registry
**Location:** `../include/query/functions/*.h`, `functions/*.cpp`

25+ specialized function categories for domain-specific operations.

**Function Categories:**
- **AI/ML**: `ai_ml_functions.h` - Inference, embeddings, model management
- **Graph**: `graph_functions.h`, `graph_extensions.h` - Traversal, shortest path, centrality
- **Geospatial**: `geo_functions.h`, `crs_functions.h` - ST_Distance, ST_Within, coordinate transforms
- **Vector**: `vector_functions.h` - Cosine similarity, L2 distance, dot product
- **Fulltext**: `fulltext_functions.h` - Text search, tokenization, ranking
- **JSON**: `json_path_functions.h` - JSONPath queries, manipulation
- **Temporal**: `date_functions.h`, `holiday_provider.h` - Date arithmetic, formatting, holiday detection
- **Domain-Specific**:
  - `process_mining_functions.h` - Process discovery, conformance checking
  - `lora_functions.h` - LoRA adapter management
  - `ethics_functions.h` - Bias detection, fairness metrics
- **Security**: `security_functions.h`, `retention_functions.h` - Encryption, access control, data retention
- **Data Manipulation**: `string_functions.h`, `math_functions.h`, `array_functions.h`, `collection_functions.h`, `document_functions.h`, `file_functions.h`, `relational_functions.h`

**Usage in AQL:**
```aql
// Geospatial function
FOR place IN places
  FILTER ST_Distance(place.location, [47.6062, -122.3321]) < 5000
  RETURN place

// Vector similarity
FOR doc IN documents
  FILTER COSINE_SIMILARITY(doc.embedding, @query_vector) > 0.9
  SORT COSINE_SIMILARITY(doc.embedding, @query_vector) DESC
  LIMIT 10
  RETURN doc

// Graph traversal
FOR v, e, p IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
  RETURN p

// Process mining
FOR event IN process_events
  LET trace = PROCESS_TRACE(event.case_id, 'process_model')
  FILTER CONFORMANCE_CHECK(trace) < 0.5
  RETURN {case_id: event.case_id, conformance: CONFORMANCE_CHECK(trace)}
```

### Advanced Features

#### Window Functions
**Location:** `window_evaluator.cpp`, `../include/query/window_evaluator.h`

Analytical window functions for advanced aggregation.

**Supported Functions:**
- `ROW_NUMBER()`: Sequential row numbering
- `RANK()`: Rank with gaps for ties
- `DENSE_RANK()`: Rank without gaps
- `NTILE(n)`: Distribute rows into n buckets
- Aggregate window functions: `SUM()`, `AVG()`, `MIN()`, `MAX()`, `COUNT()`

**Example:**
```aql
FOR sale IN sales
  WINDOW w AS (
    PARTITION BY sale.region
    ORDER BY sale.amount DESC
  )
  RETURN {
    region: sale.region,
    amount: sale.amount,
    rank: RANK() OVER w,
    running_total: SUM(sale.amount) OVER w
  }
```

#### Statistical Aggregation
**Location:** `statistical_aggregator.cpp`, `../include/query/statistical_aggregator.h`

Advanced statistical functions for COLLECT/GROUP BY operations.

**Functions:**
- Basic: `COUNT()`, `SUM()`, `AVG()`, `MIN()`, `MAX()`
- Statistical: `STDDEV()`, `VARIANCE()`, `MEDIAN()`, `PERCENTILE()`
- Distributional: `MODE()`, `RANGE()`, `IQR()`

#### Query Federation
**Location:** `query_federation.cpp`, `../include/query/query_federation.h`

Distributed query execution coordination across multiple ThemisDB nodes.

**Features:**
- Partition-aware query routing
- Parallel execution across shards
- Result merging and aggregation
- Cross-shard joins and unions

## Architecture

### Query Execution Pipeline

```
Query String
     ↓
[AQL Parser] ────────> AST (Abstract Syntax Tree)
     ↓
[AQL Translator] ────> Internal Query Representation
     ↓
[Query Optimizer] ───> Optimized Execution Plan
     ↓                   (with predicate ordering, index selection)
[Cache Lookup] ──────> [Query Cache / Semantic Cache]
     ↓ (miss)
[Query Engine] ──────> Storage / Index Managers
     ↓
[Result Stream] ─────> Paginated Results
     ↓
[Cache Store] ───────> Update caches
     ↓
Return Results
```

### Thread Safety Model

- **Parser/Translator**: NOT thread-safe, create per-query or use mutex
- **Optimizer**: NOT thread-safe, statistics reads are safe
- **Engine**: Thread-safe, concurrent query execution supported
- **Caches**: Thread-safe, concurrent reads/writes with locking
- **Result Streams**: NOT thread-safe, single consumer per stream

### Optimization Process

1. **Parse**: Query string → AST
2. **Translate**: AST → Internal query with predicates
3. **Estimate**: Calculate selectivity for each predicate using statistics
4. **Order**: Sort predicates by selectivity (most selective first)
5. **Select Strategy**: Choose execution strategy (sequential, index-based, hybrid)
6. **Execute**: Run optimized plan with short-circuit evaluation
7. **Learn**: Update statistics for adaptive optimization

## Integration Points

### Storage Module Integration

```cpp
#include "query/query_engine.h"
#include "storage/storage_engine.h"

// Inject storage dependency
auto storage = std::make_shared<StorageEngine>(rocksdb_config);
QueryEngine engine(storage, index_manager);

// Engine uses storage for:
// - Entity retrieval (get, scan, prefix scan)
// - Transaction coordination
// - Batch operations
```

### Index Module Integration

```cpp
#include "query/query_engine.h"
#include "index/index_manager.h"

// Inject index manager
auto index_mgr = std::make_shared<IndexManager>(storage);
QueryEngine engine(storage, index_mgr);

// Engine uses indexes for:
// - Vector similarity search
// - Spatial range queries
// - Graph traversal
// - Secondary index lookups
```

### LLM Integration (AQL Extensions)

```cpp
#include "query/llm_aql_handler.h"

LLMAQLHandler llm_handler(llm_engine);

// LLM-specific AQL commands
std::string query = R"(
  MODEL LOAD 'gpt-4' WITH {context_size: 8192}
  INFER 'Summarize this document' ON documents LIMIT 10
)";

auto result = llm_handler.execute(query);
```

## API Reference

### Basic Query Execution

```cpp
#include "query/aql_runner.h"

// High-level API (recommended)
std::string query = "FOR u IN users FILTER u.age > 30 RETURN u";
auto result = executeAql(query, engine);

if (result) {
  nlohmann::json data = result.value();
  std::cout << data.dump(2) << std::endl;
} else {
  std::cerr << "Query error: " << result.error() << std::endl;
}
```

### Per-Query Resource Limits

`executeAqlWithLimits()` wraps `executeAql()` and enforces configurable caps on
row count, memory, and wall-clock execution time.  A limit value of `0` means
**unlimited** for that dimension.

**Header:** `include/query/query_resource_limits.h`

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
    // Query exceeded configured timeout
  } else if (result.error().code() == errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED) {
    // Row count or memory limit exceeded
  }
}
```

**Enforcement semantics:**

| Limit | Checkpoint | Error code |
|---|---|---|
| `max_rows` | After execution – checks `results` array length | `ERR_QUERY_RESOURCE_EXHAUSTED` |
| `max_memory_bytes` | After execution – uses serialized JSON byte size as proxy | `ERR_QUERY_RESOURCE_EXHAUSTED` |
| `timeout_ms` | After execution returns – wall-clock from call entry | `ERR_QUERY_TIMEOUT` |

**Low-level helper — `QueryResourceGuard`:**

For code that produces rows iteratively (e.g. custom execution paths), use the
`QueryResourceGuard` RAII class to track violations incrementally:

```cpp
#include "query/query_resource_limits.h"

QueryResourceGuard guard(limits);

for (auto& row : rows) {
  auto violation = guard.checkRow(/*estimated_bytes=*/row.dump().size());
  if (violation != QueryResourceGuard::Violation::None) {
    // handle RowLimit / MemoryLimit / Timeout
    break;
  }
  process(row);
}
```

### Query Cancellation via Request ID

`executeAqlCancellable()` wraps `executeAql()` and registers the query under a
caller-assigned **request ID** in the process-wide `QueryCanceller` registry.
Any other thread can cancel it at any time by calling `QueryCanceller::cancel()`.

**Header:** `include/query/query_canceller.h`

```cpp
#include "query/aql_runner.h"
#include "query/query_canceller.h"

// Execute (e.g. in an HTTP worker thread):
auto result = executeAqlCancellable(aql, engine, "request-42");

// Cancel from any other thread (e.g. HTTP cancel handler):
bool was_live = QueryCanceller::instance().cancel("request-42");
```

On cancellation `executeAqlCancellable()` returns
`Err(ERR_QUERY_CANCELLED, request_id)`. The registration is automatically
cleaned up when the function returns.

### Advanced Query Execution

```cpp
// Lower-level API (for fine-grained control)
AQLParser parser;
AQLTranslator translator;
QueryOptimizer optimizer;

// Parse
auto ast = parser.parse(query).value();

// Translate
auto internal_query = translator.translate(ast);

// Optimize
auto optimized_plan = optimizer.optimize(internal_query);

// Execute
auto result = engine.execute(optimized_plan);

// Stream results
ResultStream stream(result);
while (stream.hasMore()) {
  auto batch = stream.nextBatch();
  process(batch);
}
```

### Hybrid Query Example

```cpp
// Vector + Geo hybrid query
std::string query = R"(
  FOR place IN places
    FILTER ST_Distance(place.location, [47.6062, -122.3321]) < 5000
    AND COSINE_SIMILARITY(place.embedding, @query_vector) > 0.9
    SORT COSINE_SIMILARITY(place.embedding, @query_vector) DESC
    LIMIT 10
    RETURN place
)";

// Bind parameters
QueryParameters params;
params.bindVector("query_vector", embedding);

auto result = executeAql(query, engine, params);
```

### Graph Traversal Example

```cpp
// Shortest path query
std::string query = R"(
  FOR v, e, p IN OUTBOUND SHORTEST_PATH
    'users/alice' TO 'users/bob'
    GRAPH 'social'
    RETURN p
)";

auto result = executeAql(query, engine);

// Recursive path query with depth limit
std::string recursive_query = R"(
  FOR v, e, p IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
    FILTER v.country == 'USA'
    RETURN p
)";

auto recursive_result = executeAql(recursive_query, engine);
```

## Dependencies

### Internal Dependencies

- **storage**: Storage engine for data persistence and transaction coordination
- **index**: Index manager for vector, spatial, graph, and secondary indexes
- **core**: Base types, configuration, metrics
- **llm** (optional): LLM engine for INFER/RAG commands

### External Dependencies

**Required:**
- None (query module has no external dependencies)

**Optional:**
- **nlohmann/json**: JSON parsing (for JSON function library)
- **Boost.Geometry**: Advanced geospatial functions

### Build Configuration

```cmake
# CMakeLists.txt
add_library(themisdb_query
  src/query/aql_parser.cpp
  src/query/aql_translator.cpp
  src/query/query_optimizer.cpp
  src/query/query_engine.cpp
  src/query/query_cache.cpp
  src/query/semantic_cache.cpp
  src/query/cte_cache.cpp
  src/query/result_stream.cpp
  src/query/aql_runner.cpp
  # ... additional files
)

target_link_libraries(themisdb_query
  PUBLIC themisdb_storage
  PUBLIC themisdb_index
  PUBLIC themisdb_core
)
```

## Performance Characteristics

### Query Execution Latency

- **Simple queries** (1-2 predicates, no joins): 1-10ms
- **Complex queries** (5-10 predicates, joins): 10-100ms
- **Graph traversal** (depth 3-5, 100-1000 edges): 50-500ms
- **Hybrid queries** (vector k=10 + spatial radius 5km): 10-50ms
- **Aggregate queries** (GROUP BY with 1M rows): 100-1000ms

### Optimization Overhead

- **Simple queries**: 0.1-1ms
- **Complex queries** (10+ predicates): 1-10ms
- **Adaptive learning update**: 0.01-0.1ms per query

### Cache Performance

- **Query cache hit**: ~1μs (saves 10-1000ms+ execution time)
- **Semantic cache hit**: ~100μs (includes embedding similarity)
- **CTE cache (in-memory)**: ~10μs per access
- **CTE cache (disk spill)**: ~5-50ms per access

### Throughput

- **Cached queries**: 50K-100K queries/sec
- **Simple queries**: 1K-10K queries/sec
- **Complex queries**: 100-1K queries/sec
- **Streaming results**: 10K-100K rows/sec

### Memory Usage

- **Parser/AST**: 1-10KB per query
- **Query cache**: ~10KB per cached query (configurable max size)
- **Semantic cache**: ~20KB per cached query (includes embeddings)
- **CTE cache**: 100MB default threshold (configurable)
- **Result stream buffer**: 1MB per active stream (configurable)

### Tuning Recommendations

1. **Cache Configuration**:
   - Set query cache size based on workload repetition (1K-100K entries)
   - Enable semantic cache for analytics workloads with similar queries
   - Increase CTE memory threshold for complex queries with large intermediate results

2. **Optimizer Tuning**:
   - Enable adaptive optimization for production workloads
   - Collect statistics on representative data samples
   - Monitor and adjust selectivity estimates periodically

3. **Streaming Configuration**:
   - Increase batch size for high-throughput scenarios (1000+ rows)
   - Use CURSOR_BASED pagination for stateless web APIs
   - Use KEYSET pagination for stable, ordered results

4. **Predicate Ordering**:
   - Ensure statistics are up-to-date for accurate selectivity estimates
   - Use EXPLAIN to verify predicate ordering in complex queries
   - Consider manual hints for queries where optimizer struggles

## Continuous Query Language (CQL) — v2.0.0

ThemisDB v2.0.0 introduces a production-grade **Continuous Query Language (CQL)** engine that evaluates standing queries continuously as new data arrives. The implementation is based on the formal semantics of Arasu, Babu & Widom (2006); see [arasu_cql_2006.md](../../research/papers/arasu_cql_2006.md).

### CQL Syntax Reference

```aql
-- Sliding time-window aggregate (10 s range, 1 s slide)
CREATE CONTINUOUS QUERY sensor_avg
  OVER sensor_readings
  WINDOW [RANGE 10 SECONDS SLIDE 1 SECOND]
  AS FOR t IN __window__
     COLLECT AGGREGATE avg_v = AVG(t.value)
     RETURN {ts: CURRENT_TIMESTAMP(), avg: avg_v}
  OUTPUT DELTA;

-- Count window (last 500 events)
CREATE CONTINUOUS QUERY recent_events
  OVER iot_stream
  WINDOW [ROWS 500]
  AS FOR t IN __window__
     FILTER t.severity >= 3
     SORT t.event_ts DESC
     RETURN t
  OUTPUT SNAPSHOT;

-- Tumbling time-window (non-overlapping 1-minute buckets)
CREATE CONTINUOUS QUERY minute_counts
  OVER transactions
  WINDOW [TUMBLING 1 MINUTE]
  AS FOR t IN __window__
     COLLECT AGGREGATE cnt = COUNT(t)
     RETURN {minute: t.ts, count: cnt}
  OUTPUT CHANGES;

-- Drop a query
DROP CONTINUOUS QUERY sensor_avg;

-- List all active queries
SHOW CONTINUOUS QUERIES;
```

### Window Types

| Type | Syntax | Semantics |
|------|--------|-----------|
| Sliding time | `[RANGE T SLIDE S]` | Overlapping windows of duration T, advancing every S |
| Tumbling time | `[TUMBLING T]` | Non-overlapping windows of fixed duration T |
| Sliding count | `[ROWS N SLIDE K]` | Overlapping windows of N tuples, advancing K tuples |
| Count (fixed) | `[ROWS N]` | Last N tuples (shorthand for sliding count, slide=1) |

### Output Modes

| Mode | Behaviour |
|------|-----------|
| `DELTA` | Emits `(+tuple)` additions and `(-tuple)` retractions each tick |
| `SNAPSHOT` | Emits the full current window state each tick |
| `CHANGES` | Emits only additions (no retractions) |

### C++ API

```cpp
#include "query/continuous_query_engine.h"
#include "query/continuous_query_engine_impl.h"

using namespace themis::query;

// Create engine
auto engine = ContinuousQueryEngineImpl::create();

// Register a query
ContinuousQuerySpec spec;
spec.name               = "my_query";
spec.source_collection  = "sensor_data";
spec.window             = WindowSpec::slidingTime(
    std::chrono::seconds(10), std::chrono::seconds(1));
spec.aql_body           = "FOR t IN __window__ COLLECT AGGREGATE s = SUM(t.v) RETURN s";
spec.result_mode        = ResultMode::DELTA;
spec.allowed_lateness_ms = 500;

auto handle = engine->registerQuery(std::move(spec));

// Subscribe to results
auto stream = engine->subscribe("my_query", ResultMode::DELTA);
while (stream->hasMore()) {
    auto result = stream->next(std::chrono::seconds(1));
    if (result) {
        // result->payload  — JSON string
        // result->is_retract — true for retractions
    }
}

// Inject a tuple (CDC feed / test)
engine->injectTuple("sensor_data",
    R"({"ts":1714000000000000,"v":42.5})",
    1714000000000000LL);  // event_ts in µs

// Drop
engine->dropQuery("my_query");
```

### Performance Targets

| Metric | Target | Benchmark |
|--------|--------|-----------|
| Throughput | ≥ 500 000 tuples/s | `BM_ContinuousQuery_Throughput` (CQ-PERF-01) |
| Per-tuple p99 latency | ≤ 5 ms | `BM_ContinuousQuery_TupleLatency` (CQ-PERF-01) |
| Empty-window tick overhead | ≤ 1 µs | `BM_ContinuousQuery_WindowTick` (CQ-PERF-02) |
| Concurrent active queries | ≥ 1 000 | Unit test CQ-20 |
| Watermark correction latency | ≤ 2 × tick_interval | Unit tests CQ-14/15 |

### Capacity Limits

| Parameter | Default | Override |
|-----------|---------|---------|
| `max_tuples` per synopsis | 10 000 000 | `SynopsisStore` constructor |
| `max_bytes` per synopsis | 1 GiB | `SynopsisStore` constructor |
| `allowed_lateness_ms` | 0 (strict) | `ContinuousQuerySpec::allowed_lateness_ms` |

## Known Limitations

1. **Parser Limitations**:
   - No support for subqueries in SELECT clause (only in FILTER/LET)
   - Limited support for correlated subqueries
   - No support for lateral joins (planned for v1.6.0)

2. **Optimizer Limitations**:
   - Cost model assumes uniform data distribution (can be inaccurate for skewed data)
   - No multi-query optimization (each query optimized independently)
   - Limited optimization for user-defined functions

3. **Execution Limitations**:
   - Graph traversal depth limited to 20 hops (configurable, but memory-intensive)
   - Parallel execution is limited to the parallel-scan operator (`parallel_scan.h`); intra-operator parallelism within a single plan is planned for v1.6.0
   - Join algorithm selection is adaptive; hash-join and nested-loop variants are implemented in `query_engine.cpp`

4. **Caching Limitations**:
   - Query cache uses exact string matching (whitespace-sensitive)
   - Semantic cache requires embedding model (adds latency)
   - CTE cache disk spill can be slow for very large CTEs (>1GB)

5. **Result Streaming Limitations**:
   - OFFSET_LIMIT pagination can be inefficient for large offsets
   - CURSOR_BASED pagination requires stable ordering
   - No support for bidirectional streaming (forward-only)

6. **Function Limitations**:
   - Some advanced functions require optional dependencies
   - WASM/Python UDF runtimes are planned; C++ UDFs can be registered via `UDFRegistry` in `include/query/functions/udf_registry.h`
   - Limited support for aggregate window functions

7. **Memory Constraints**:
   - Large result sets can consume significant memory
   - CTE materialization can exceed memory limits (requires disk spill)
   - Query cache eviction can cause performance variability

8. **Thread Safety**:
   - Parser/translator are not thread-safe (must create per-query or lock)
   - Result streams are single-consumer only

## Status

**Production Ready** (as of v2.0.0)

✅ **Stable Features:**
- AQL parsing and AST generation
- Cost-based query optimization
- Multi-model query execution
- Graph traversal (BFS/DFS, shortest path)
- Hybrid queries (vector+geo, fulltext+geo)
- Query caching (exact, semantic, CTE)
- Result streaming and pagination
- Expression evaluation and function registry
- Subquery and CTE support
- SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE)
- SPARQL compatibility for RDF / knowledge-graph queries
- Per-query resource limits (rows, memory, timeout)
- Query cancellation via request ID (`QueryCanceller`)
- Vectorized execution (column-store batch processing)
- Cross-cluster federated AQL
- UDF registration (`UDFRegistry`)
- Query plan visualization (EXPLAIN / EXPLAIN ANALYZE)
- Result type annotations (for SDK code generation)
- Parallel collection scans (`ParallelScanner`)
- Runtime re-optimization from execution statistics
- **Continuous Query Language (CQL)** — `ContinuousQueryEngine`, `WindowSpec`, `SynopsisStore`, `IncrementalAgg`, `CQWatermark` (v2.0.0)

⚠️ **Beta Features:**
- Adaptive query optimization (v1.5.0+)
- Semantic cache (v1.5.0+)
- Query federation (v1.5.0+)
- Window functions (v1.5.0+)

🔬 **Experimental:**
- NLP-enhanced optimization (v1.5.0+)
- Distributed query planning (v1.5.0+)

## Related Documentation

- [Storage Module](../storage/README.md) - Data persistence and transaction coordination
- [Index Module](../index/README.md) - Index structures for efficient query execution
- [Public Query Header API](../../include/query/README.md) - Public entry points and header-level API surface
- [AQL Syntax Reference](../../docs/de/aql/aql_syntax.md) - Complete AQL syntax documentation
- [Query Performance Expectations](PERFORMANCE_EXPECTATIONS.md) - Performance tuning targets and benchmark expectations
- [Implementation Summary: AQL Functions](../../docs/de/implementation/IMPLEMENTATION_SUMMARY_AQL_FUNCTIONS.md) - AQL function implementation details
- [Implementation Summary: Query Optimizer](../../docs/de/implementation/IMPLEMENTATION_SUMMARY_OPTIMIZER.md) - Query optimizer internals
- [arasu_cql_2006.md](../../research/papers/arasu_cql_2006.md) - CQL formal semantics (Arasu, Babu & Widom 2006)

*Last Updated: April 2026*
*Module Version: v2.0.0*
*Next Review: v2.1.0 Release*

## Scientific References

1. Selinger, P. G., Astrahan, M. M., Chamberlin, D. D., Lorie, R. A., & Price, T. G. (1979). **Access Path Selection in a Relational Database Management System**. *Proceedings of the 1979 ACM SIGMOD International Conference on Management of Data*, 23–34. https://doi.org/10.1145/582095.582099

2. Graefe, G., & McKenna, W. J. (1993). **The Volcano Model: An Extensible and Parallel Query Evaluation System**. *IEEE Transactions on Knowledge and Data Engineering*, 6(1), 120–135. https://doi.org/10.1109/69.273032

3. Ioannidis, Y. E. (1996). **Query Optimization**. *ACM Computing Surveys*, 28(1), 121–123. https://doi.org/10.1145/234313.234367

4. Graefe, G. (1993). **Query Evaluation Techniques for Large Databases**. *ACM Computing Surveys*, 25(2), 73–170. https://doi.org/10.1145/152610.152611

5. Leis, V., Gubichev, A., Mirchev, A., Boncz, P., Kemper, A., & Neumann, T. (2015). **How Good Are Query Optimizers, Really?** *Proceedings of the VLDB Endowment*, 9(3), 204–215. https://doi.org/10.14778/2850583.2850594

6. Arasu, A., Babu, S., & Widom, J. (2006). **The CQL Continuous Query Language: Semantic Foundations and Query Execution**. *The VLDB Journal*, 15(2), 121–142. https://doi.org/10.1007/s00778-004-0147-z — foundational reference for ThemisDB CQL engine (Phase 8); see [`research/papers/arasu_cql_2006.md`](../../research/papers/arasu_cql_2006.md)

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
