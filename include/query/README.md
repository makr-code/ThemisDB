> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Query Module Headers

## Module Purpose

The Query module provides ThemisDB's comprehensive query execution layer, featuring the AQL (Advanced Query Language) parser, query optimizer, execution engine, caching system, and federation capabilities. AQL is a multi-paradigm query language based on ArangoDB's AQL but extended to support relational, document, graph, vector, spatial, and timeseries models. This module handles all aspects of query processing from parsing to result delivery with support for hybrid queries and distributed execution.

### About AQL (Advanced Query Language)

**AQL** is ThemisDB's declarative query language inspired by ArangoDB's AQL (ArrangoQL) but significantly extended for multi-model database capabilities:

**Core Philosophy:**
- **Declarative Syntax**: SQL-like readability with FOR-FILTER-SORT-RETURN pattern
- **Multi-Paradigm**: Single query language for all data models (relational, document, graph, vector, spatial, timeseries)
- **Composable**: Mix and match query patterns for complex hybrid queries
- **Extensible**: 100+ built-in functions with plugin architecture for custom functions

**Key Differentiators from ArangoDB AQL:**
- ✅ **Compatible Core**: Maintains ArangoDB AQL syntax for common operations
- ➕ **Vector Search**: Native SIMILARITY(), COSINE_DISTANCE(), L2_DISTANCE() functions
- ➕ **Advanced Geospatial**: Full ST_* function suite (PostGIS-compatible)
- ➕ **LLM Integration**: LLM INFER, LLM RAG, LLM EMBED commands
- ➕ **Timeseries**: Specialized window functions and retention policies
- ➕ **Process Mining**: Event log analysis, conformance checking, variant detection
- ➕ **Ethics Functions**: Bias detection, fairness scoring, transparency analysis
- ➕ **Federation**: Query across distributed databases and shards

**Query Examples:**

```aql
-- Relational query (SQL-like)
FOR user IN users
  FILTER user.age > 30 AND user.city == "Seattle"
  SORT user.name ASC
  LIMIT 10
  RETURN user

-- Graph traversal (compatible with ArangoDB)
FOR vertex, edge, path IN 1..5 OUTBOUND "users/alice" GRAPH "social"
  FILTER vertex.verified == true
  RETURN {vertex: vertex, distance: LENGTH(path)}

-- Vector similarity (ThemisDB extension)
FOR doc IN documents
  LET score = SIMILARITY(doc.embedding, @queryVector)
  FILTER score > 0.8
  SORT score DESC
  LIMIT 10
  RETURN {doc: doc, score: score}

-- Hybrid vector+geo query (ThemisDB extension)
FOR place IN places
  LET vecScore = SIMILARITY(place.embedding, @queryVector)
  FILTER vecScore > 0.7
  FILTER ST_DWithin(place.location, ST_Point(@lon, @lat), 1000)
  SORT vecScore DESC
  LIMIT 20
  RETURN place

-- LLM integration (ThemisDB extension)
LLM RAG 'What are the top rated restaurants near me?'
  SEARCH IN restaurants
  FILTER ST_DWithin(location, ST_Point(@userLon, @userLat), 5000)
  TOP 5
  MODEL 'llama-3-8b'
```

## Scope

**In Scope:**
- AQL parsing and AST construction
- Query optimization (cost-based and adaptive)
- Query execution engine (single-node and federated)
- Result streaming and pagination
- Query result caching (exact and semantic)
- Window functions and statistical aggregations
- CTEs (Common Table Expressions) and subqueries
- Function registry (100+ built-in functions)
- Cross-model query support (relational, document, graph, vector, timeseries)
- Hybrid query execution (vector+geo, fulltext+spatial, graph+geo)

**Out of Scope:**
- Data persistence (handled by storage module)
- Index management (handled by index module)
- Network protocols and wire format (handled by server module)
- Authentication and authorization (handled by auth module)

## Key Components

### Query Engine
**Location:** `query_engine.h`, `../../src/query/query_engine.cpp`

Central query execution engine supporting multi-model queries and hybrid workloads.

**Features:**
- **Multi-Model Queries**: Relational, document, graph, vector, timeseries
- **Hybrid Query Support**: Vector+Geo, Fulltext+Spatial, Graph+Geo
- **Dependency Injection**: Pluggable storage, index, and evaluation interfaces
- **Transaction Integration**: MVCC-aware query execution
- **Predicate Pushdown**: Filter pushdown to storage and index layers
- **Join Support**: Nested loop, hash join, merge join
- **Graph Traversal**: BFS/DFS with depth limits and spatial constraints

**Thread Safety:**
- Read-safe: Multiple concurrent query executions
- Write-safe: Transaction isolation via storage layer
- NOT thread-local: Single QueryEngine instance serves multiple threads

**Configuration Example:**
```cpp
// Dependency injection setup
auto storage = std::make_shared<StorageEngine>(config);
auto index_mgr = std::make_shared<IndexManager>(storage);
auto evaluator = std::make_shared<ExpressionEvaluator>();

QueryEngine engine(storage, index_mgr, evaluator);

// Execute relational query
ConjunctiveQuery query;
query.table_name = "users";
query.predicates = {{"age", ">", "30"}, {"city", "==", "Seattle"}};
auto result = engine.executeConjunctiveQuery(query);

// Execute graph traversal
RecursivePathQuery graph_query;
graph_query.start_node = "user_123";
graph_query.end_node = "user_456";
graph_query.edge_type = "FOLLOWS";
graph_query.max_depth = 5;
auto paths = engine.executeRecursivePath(graph_query);

// Execute vector+geo hybrid query
VectorGeoQuery hybrid_query;
hybrid_query.table = "places";
hybrid_query.query_vector = {0.1f, 0.2f, ...};
hybrid_query.k = 10;
hybrid_query.spatial_filter = ST_Within(location, bbox);
auto results = engine.executeVectorGeoQuery(hybrid_query);
```

**Performance Characteristics:**
- Simple queries: 0.1-1ms (with indexes)
- Complex queries: 10-100ms (joins, aggregations)
- Graph traversals: 10-500ms (depends on depth, branching factor)
- Vector queries: 5-50ms (with HNSW index)
- Hybrid queries: 20-200ms (depends on selectivity)

### AQL Parser
**Location:** `aql_parser.h`, `../../src/query/aql_parser.cpp`

Recursive descent parser for AQL (Advanced Query Language) with comprehensive syntax support.

**Syntax Features (v1.5.0+):**
```
Basic Query:
  FOR doc IN users
    FILTER doc.age > 30 AND doc.city == "Seattle"
    SORT doc.name ASC
    LIMIT 10, 100
    RETURN doc

LET Expressions:
  FOR doc IN users
    LET age_category = doc.age < 18 ? "minor" : "adult"
    LET full_name = CONCAT(doc.firstName, " ", doc.lastName)
    RETURN {name: full_name, category: age_category}

Window Functions:
  FOR sale IN sales
    SORT sale.date ASC
    WINDOW w AS (PARTITION BY sale.product_id ORDER BY sale.date ROWS BETWEEN 2 PRECEDING AND CURRENT ROW)
    LET moving_avg = AVG(sale.amount) OVER w
    RETURN {product: sale.product_id, avg: moving_avg}

CTEs (Common Table Expressions):
  WITH high_value_users AS (
    FOR u IN users
      FILTER u.total_spent > 1000
      RETURN u
  )
  FOR user IN high_value_users
    FOR order IN orders
      FILTER order.user_id == user._id
      RETURN {user: user.name, order: order.id}

Subqueries:
  FOR user IN users
    LET order_count = (
      FOR order IN orders
        FILTER order.user_id == user._id
        RETURN 1
      | COUNT
    )
    FILTER order_count > 5
    RETURN {user: user.name, orders: order_count}

Array Operations:
  FOR doc IN products
    FILTER doc.tags ANY == "electronics"
    FILTER doc.reviews ALL >= 4.0
    RETURN doc

Vector Similarity:
  FOR doc IN embeddings
    FILTER SIMILARITY(doc.vector, @query_vec, 10)
    RETURN doc

Geospatial:
  FOR place IN locations
    FILTER ST_Within(place.geometry, @bbox)
    FILTER ST_Distance(place.geometry, @point) < 1000
    RETURN place

Graph Traversal:
  FOR v, e, p IN 1..5 OUTBOUND "users/123" edges
    FILTER p.edges[*].type ALL == "FRIEND"
    RETURN {vertex: v, path: p}
```

**AST Node Types:**
- **Query Nodes**: ForNode, FilterNode, SortNode, LimitNode, ReturnNode
- **Expression Nodes**: BinaryOp, UnaryOp, FunctionCall, Literal, Variable
- **Advanced Nodes**: LetNode, WithNode, CollectNode, SubqueryExpr
- **Specialized**: ArrayLiteral, ObjectConstruct, FieldAccess

**Parser API:**
```cpp
AQLParser parser;

// Parse query string
auto ast_result = parser.parse("FOR doc IN users FILTER doc.age > 30 RETURN doc");
if (!ast_result) {
    std::cerr << "Parse error: " << ast_result.error().message() << std::endl;
    return;
}

auto ast = ast_result.value();

// Inspect AST
std::cout << "Query type: " << ast->type << std::endl;
std::cout << "Collection: " << ast->for_node->collection << std::endl;

// Convert to JSON for debugging
nlohmann::json ast_json = parser.astToJson(ast);
std::cout << ast_json.dump(2) << std::endl;
```

**Error Handling:**
```cpp
auto result = parser.parse(query_string);
if (!result) {
    auto err = result.error();
    std::cerr << "Error at line " << err.line
              << ", col " << err.column << ": "
              << err.message() << std::endl;
    // Syntax errors include position for IDE integration
}
```

**Thread Safety:** Parser instances are NOT thread-safe. Use one parser per thread or synchronize access.

### Query Optimizer
**Location:** `query_optimizer.h`, `../../src/query/query_optimizer.cpp`

Cost-based query optimizer with cardinality estimation and join ordering.

**Optimization Strategies:**
- **Predicate Ordering**: Order AND predicates by selectivity (most selective first)
- **Index Selection**: Choose optimal index for query predicates
- **Join Ordering**: Reorder joins to minimize intermediate result size
- **Pushdown Optimization**: Push filters and projections to storage layer
- **Cardinality Estimation**: Sample-based cardinality estimation for predicates
- **Cost Modeling**: CPU, I/O, and memory cost estimation

**Cost Models:**

**Conjunctive Query Optimization:**
```cpp
QueryOptimizer optimizer(index_manager);

ConjunctiveQuery query;
query.table_name = "users";
query.predicates = {
    {"age", ">", "30"},      // Estimated selectivity: 0.4
    {"city", "==", "Seattle"}, // Estimated selectivity: 0.05
    {"status", "==", "active"} // Estimated selectivity: 0.7
};

// Choose optimal predicate ordering
auto plan = optimizer.chooseOrderForAndQuery(query, 1000);
// Result: city, age, status (most selective first)

// Execute with optimized plan
auto results = optimizer.executeOptimizedEntities(engine, query, plan);
```

**Vector+Geo Cost Model:**
```cpp
QueryOptimizer::VectorGeoCostInput input;
input.hasVectorIndex = true;
input.hasSpatialIndex = true;
input.bboxRatio = 0.1;  // Spatial filter selects 10% of data
input.k = 10;
input.vectorDim = 768;

auto cost_result = QueryOptimizer::chooseVectorGeoPlan(input);
if (cost_result.plan == VectorGeoPlan::SpatialThenVector) {
    // Apply spatial filter first, then vector search
} else {
    // Apply vector search first, then spatial filter
}
```

**NLP-Enhanced Optimization (v1.5.0+):**
```cpp
// Combines traditional cost-based optimization with NLP semantic analysis
auto plan = optimizer.chooseOrderForAndQueryWithNLP(
    query,
    "Find all active users in Seattle over 30 years old",
    1000
);

// Plan includes semantic hints:
// - nlp_complexity: 0.3 (simple query)
// - nlp_suggested_indexes: ["city_idx", "age_idx"]
// - nlp_hints: {"semantic_type": "demographic_filter"}
```

**Thread Safety:** Optimizer is thread-safe for concurrent query optimization.

### Adaptive Optimizer
**Location:** `adaptive_optimizer.h`, `../../src/query/adaptive_optimizer.cpp`

Machine learning-based adaptive query optimization using execution feedback.

**Features:**
- **Execution History**: Track actual vs estimated cardinalities
- **Adaptive Adjustment**: Adjust cost model based on historical accuracy
- **Cardinality Feedback**: Learn from execution statistics
- **Per-Query Learning**: Separate models for different query patterns
- **Operator-Level Stats**: Track statistics per query operator

**Usage:**
```cpp
AdaptiveQueryStats stats;

// After each query execution
AdaptiveQueryStats::QueryExecution exec;
exec.query_hash = computeQueryHash(query);
exec.estimated_rows = 1000;
exec.actual_rows = 50;  // Overestimated by 20x
exec.execution_time_ms = 25.5;
stats.recordExecution(exec);

// Get adaptive adjustment factor
double adjustment = stats.getAdaptiveAdjustmentFactor(query_hash);
// Returns 0.05 (1/20) to correct future estimates

// Check for consistent misestimation
if (stats.hasCardinalityMisestimation(query_hash, 2.0)) {
    // Optimizer is consistently off by >2x
    // Consider rebuilding statistics or adjusting model
}
```

**Statistics Pruning:**
```cpp
// Retain last 24 hours of statistics
stats.pruneOldStats(std::chrono::hours(24));
```

**Thread Safety:** All methods are thread-safe with internal locking.

### Query Cache
**Location:** `query_cache.h`, `../../src/query/query_cache.cpp`

High-performance query result caching with multiple eviction policies.

**Features:**
- **Eviction Policies**: LRU (Least Recently Used), LFU (Least Frequently Used)
- **TTL Support**: Time-based expiration
- **Dependency Tracking**: Invalidate cache on data changes
- **Memory Management**: Memory-aware eviction with pressure thresholds
- **Fingerprinting**: SHA256-based deterministic cache keys
- **Statistics**: Hit rate, eviction metrics, memory usage

**Configuration:**
```cpp
QueryCache::Config config;
config.max_entries = 10000;
config.max_memory_bytes = 100 * 1024 * 1024;  // 100MB
config.max_entry_size = 10 * 1024 * 1024;     // 10MB per entry
config.eviction_policy = QueryCache::EvictionPolicy::LRU;
config.default_ttl = std::chrono::seconds(3600);  // 1 hour
config.enable_memory_pressure_eviction = true;

QueryCache cache(config);
```

**Usage:**
```cpp
// Cache query result
std::string query = "FOR doc IN users FILTER doc.age > 30 RETURN doc";
nlohmann::json params = {{"min_age", 30}};
nlohmann::json result = execute_query(query);

cache.put(query, params, result,
          std::chrono::seconds(300),  // TTL: 5 minutes
          {"users"});                 // Dependencies

// Lookup cached result
auto cached = cache.get(query, params);
if (cached) {
    return cached.value();  // Cache hit
} else {
    // Cache miss - execute query
}

// Invalidate cache when data changes
cache.invalidate({"users"});  // Invalidate all queries on "users" table

// Get cache statistics
auto stats = cache.getStatistics();
std::cout << "Hit rate: " << stats.hit_rate << "%" << std::endl;
std::cout << "Memory usage: " << stats.memory_usage_bytes << " bytes" << std::endl;
```

**Performance Targets:**
- Cache lookup: <1ms
- Cache hit rate: >60% for repeated queries
- Memory overhead: <10% of cached data

**Thread Safety:** All operations are thread-safe with fine-grained locking.

### Semantic Cache
**Location:** `semantic_cache.h`, `../../src/query/semantic_cache.cpp`

AI-powered semantic query cache using embedding similarity for cache hits.

**Features:**
- **Semantic Matching**: Find similar queries using embeddings
- **Exact + Similarity**: Two-tier lookup (exact → similarity)
- **Configurable Threshold**: Tune similarity threshold (0.0-1.0)
- **Vector Index**: Fast similarity search via HNSW index
- **LRU Eviction**: Memory-efficient cache management

**How It Works:**
```
Query: "FIND users WHERE age > 30"
         ↓
   Generate Embedding (128-dim vector)
         ↓
   Check Exact Match
         ↓ (miss)
   Similarity Search (threshold=0.85)
         ↓
   Match: "FIND users WHERE age >= 31" (similarity=0.92)
         ↓
   Return Cached Result
```

**Configuration:**
```cpp
SemanticQueryCache::Config config;
config.max_entries = 1000;
config.similarity_threshold = 0.85f;  // 85% similarity required
config.embedding_dim = 128;
config.ttl = std::chrono::seconds(3600);
config.enable_exact_match = true;
config.enable_similarity_match = true;

SemanticQueryCache cache(config);
```

**Usage:**
```cpp
std::string query = "FOR doc IN users FILTER doc.age > 30 RETURN doc";
nlohmann::json result = execute_query(query);

// Cache result (automatically generates embedding)
cache.put(query, result.dump());

// Lookup - exact match
auto lookup1 = cache.get("FOR doc IN users FILTER doc.age > 30 RETURN doc");
// Returns: {found: true, exact_match: true, similarity: 1.0}

// Lookup - semantic match
auto lookup2 = cache.get("FOR doc IN users FILTER doc.age >= 31 RETURN doc");
// Returns: {found: true, exact_match: false, similarity: 0.92}

// Lookup - no match
auto lookup3 = cache.get("FOR doc IN products FILTER doc.price < 100 RETURN doc");
// Returns: {found: false}
```

**Embedding Generation:**
- **Simple Hashing**: Fast but less accurate (default)
- **Sentence Encoding**: Accurate but slower (requires model)
- **Feature Extraction**: Parse query and extract features

**Thread Safety:** Thread-safe with internal locking.

### Workload Cache Strategy
**Location:** `workload_cache_strategy.h`, `../../src/query/workload_cache_strategy.cpp`

Intelligent cache strategy selection based on workload patterns.

**Features:**
- **Workload Detection**: OLTP vs OLAP vs Mixed
- **Dynamic Tuning**: Adjust cache size based on workload
- **Policy Selection**: Choose LRU/LFU based on access patterns
- **Predictive Caching**: Pre-cache likely queries
- **Admission Control**: Only cache queries meeting criteria

**Workload Types:**
```cpp
enum class WorkloadType {
    OLTP,      // Transactional: Small queries, high concurrency, point lookups
    OLAP,      // Analytical: Large queries, low concurrency, scans
    MIXED,     // Hybrid workload
    UNKNOWN    // Not yet determined
};
```

**Strategy Selection:**
```cpp
WorkloadCacheStrategy strategy;

// Analyze workload
strategy.recordQuery(query_type, result_size, execution_time);

// Get recommended strategy
auto recommendation = strategy.getRecommendedStrategy();
if (recommendation.workload == WorkloadType::OLTP) {
    // Use LRU with large cache, small TTL
    cache_config.eviction_policy = EvictionPolicy::LRU;
    cache_config.max_entries = 50000;
    cache_config.default_ttl = std::chrono::seconds(60);
} else if (recommendation.workload == WorkloadType::OLAP) {
    // Use LFU with smaller cache, longer TTL
    cache_config.eviction_policy = EvictionPolicy::LFU;
    cache_config.max_entries = 1000;
    cache_config.default_ttl = std::chrono::seconds(3600);
}

// Apply dynamic tuning
strategy.applyRecommendedSettings(cache);
```

**Thread Safety:** Thread-safe workload analysis.

### Result Stream
**Location:** `result_stream.h`, `../../src/query/result_stream.cpp`

Streaming query result delivery with pagination and backpressure handling.

**Features:**
- **Lazy Evaluation**: Results generated on demand
- **Pagination**: Offset/limit, cursor-based, keyset pagination
- **Backpressure**: Prevent memory overflow on large result sets
- **Batching**: Configurable batch sizes for network efficiency
- **Streaming**: Iterator-based result consumption

**Pagination Strategies:**

**Offset/Limit (Simple but inefficient):**
```cpp
// Page 1: LIMIT 0, 100
// Page 2: LIMIT 100, 100
// Page 3: LIMIT 200, 100
// Problem: Skipping rows is expensive for large offsets
```

**Cursor-Based (Efficient):**
```cpp
// Page 1: WHERE id > '0' LIMIT 100
// Page 2: WHERE id > 'last_id_from_page1' LIMIT 100
// Page 3: WHERE id > 'last_id_from_page2' LIMIT 100
```

**Keyset Pagination (Most efficient):**
```cpp
// Page 1: ORDER BY created_at, id LIMIT 100
// Page 2: WHERE (created_at, id) > (last_created_at, last_id) ORDER BY created_at, id LIMIT 100
```

**Usage:**
```cpp
StreamConfig config;
config.batch_size = 100;
config.max_buffer_size = 1000;
config.enable_backpressure = true;

// Create result iterator
auto iterator = engine.executeStreaming(query, config);

// Consume results in batches
while (iterator->hasNext()) {
    auto batch_result = iterator->nextBatch(100);
    if (!batch_result) {
        std::cerr << "Error: " << batch_result.error().message() << std::endl;
        break;
    }

    auto batch = batch_result.value();
    for (const auto& item : batch.items) {
        process(item);
    }

    if (batch.is_last_batch) break;

    // Use cursor for next request
    PaginationCursor cursor = batch.cursor;
}

// Alternative: Consume one item at a time
while (iterator->hasNext()) {
    auto item_result = iterator->next();
    if (item_result) {
        process(item_result.value());
    }
}
```

**Thread Safety:** ResultIterator instances are NOT thread-safe. Use one iterator per thread.

### Query Federation
**Location:** `query_federation.h`, `../../src/query/query_federation.cpp`

Distributed query execution across multiple shards/nodes.

**Features:**
- **Query Decomposition**: Split query into per-shard sub-queries
- **Parallel Execution**: Execute sub-queries concurrently
- **Result Merging**: Merge and deduplicate results
- **Aggregate Pushdown**: Push aggregations to shards
- **JOIN Optimization**: Broadcast join, shuffle join
- **Cost-Based Routing**: Route queries to optimal shards

**Execution Strategies:**

**Scatter-Gather:**
```cpp
// Send same query to all shards, merge results
QueryFederation federation(shard_router);
auto result = federation.executeScatterGather(query);
```

**Partition Pruning:**
```cpp
// Send query only to relevant shards
// Example: WHERE user_id = 'X' → route to shard containing user X
auto result = federation.executeWithPruning(query);
```

**Broadcast Join:**
```cpp
// Broadcast small table to all shards for local join
// Example: JOIN users (large, sharded) WITH countries (small)
QueryFederation::Config config;
config.enable_broadcast_join = true;
config.broadcast_threshold_bytes = 10 * 1024 * 1024;  // 10MB
federation.executeBroadcastJoin(query, config);
```

**Map-Reduce:**
```cpp
// Map phase on shards, reduce locally
// Example: COUNT(*) → COUNT on each shard, SUM locally
auto result = federation.executeMapReduce(query);
```

**Execution Plan:**
```cpp
QueryFederation federation(shard_router);

// Generate execution plan
auto plan = federation.planQuery(query);
std::cout << "Strategy: " << plan.strategy << std::endl;
std::cout << "Target shards: " << plan.target_shards.size() << std::endl;
std::cout << "Estimated time: " << plan.estimated_time_ms << "ms" << std::endl;

// Execute plan
auto result = federation.execute(plan);
```

**Thread Safety:** Federation operations are thread-safe.

### Window Functions
**Location:** `window_evaluator.h`, `../../src/query/window_evaluator.cpp`

SQL-style window functions for analytical queries.

**Supported Functions:**
- **Aggregate**: SUM, AVG, COUNT, MIN, MAX
- **Ranking**: ROW_NUMBER, RANK, DENSE_RANK, PERCENT_RANK
- **Analytic**: LEAD, LAG, FIRST_VALUE, LAST_VALUE, NTH_VALUE
- **Statistical**: STDDEV, VARIANCE, MEDIAN

**Window Frames:**
- **ROWS**: Physical row offsets (ROWS BETWEEN 2 PRECEDING AND CURRENT ROW)
- **RANGE**: Logical value ranges (RANGE BETWEEN INTERVAL '1' DAY PRECEDING AND CURRENT ROW)

**Usage:**
```cpp
// AQL syntax
FOR sale IN sales
  SORT sale.date ASC
  WINDOW w AS (
    PARTITION BY sale.product_id
    ORDER BY sale.date
    ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
  )
  LET moving_avg = AVG(sale.amount) OVER w
  LET prev_sale = LAG(sale.amount, 1) OVER w
  RETURN {
    product: sale.product_id,
    date: sale.date,
    amount: sale.amount,
    moving_avg: moving_avg,
    prev_sale: prev_sale
  }
```

**API Usage:**
```cpp
WindowEvaluator evaluator;

WindowEvaluator::WindowSpec spec;
spec.partition_by = {"product_id"};
spec.order_by = {{"date", SortOrder::ASC}};
spec.frame_type = FrameType::ROWS;
spec.frame_start = -2;  // 2 PRECEDING
spec.frame_end = 0;     // CURRENT ROW

// Evaluate window function
auto result = evaluator.evaluate(
    WindowFunction::AVG,
    spec,
    data,
    "amount"  // Column to aggregate
);
```

**Performance:** O(n log n) for sorting, O(n) for evaluation

**Thread Safety:** WindowEvaluator is thread-safe.

### Statistical Aggregator
**Location:** `statistical_aggregator.h`, `../../src/query/statistical_aggregator.cpp`

Advanced statistical functions for data analysis.

**Functions:**
- **Basic**: COUNT, SUM, AVG, MIN, MAX
- **Statistical**: STDDEV, VARIANCE, MEDIAN, PERCENTILE
- **Advanced**: CORR (correlation), COVAR (covariance), REGR (regression)
- **Distributional**: SKEWNESS, KURTOSIS

**Usage:**
```cpp
StatisticalAggregator aggregator;

// Basic statistics
auto avg = aggregator.average(values);
auto stddev = aggregator.stddev(values);

// Percentiles
auto median = aggregator.percentile(values, 0.5);
auto p95 = aggregator.percentile(values, 0.95);
auto p99 = aggregator.percentile(values, 0.99);

// Correlation
auto corr = aggregator.correlation(x_values, y_values);

// Regression
auto [slope, intercept] = aggregator.linearRegression(x_values, y_values);
```

**AQL Syntax:**
```
FOR doc IN measurements
  COLLECT product = doc.product_id INTO group
  RETURN {
    product: product,
    count: COUNT(group),
    avg: AVG(group[*].doc.value),
    stddev: STDDEV(group[*].doc.value),
    median: PERCENTILE(group[*].doc.value, 0.5),
    p95: PERCENTILE(group[*].doc.value, 0.95)
  }
```

**Thread Safety:** Aggregator is thread-safe for concurrent aggregations.

### CTEs and Subqueries
**Location:** `cte_subquery.h`, `../../src/query/cte_subquery.cpp`

Common Table Expressions (WITH clauses) and subquery support.

**CTE Features:**
- **Named Subqueries**: Define reusable query fragments
- **Recursive CTEs**: Support for recursive queries (planned)
- **CTE Caching**: Cache CTE results for multiple references
- **Dependency Analysis**: Detect circular dependencies

**CTE Syntax:**
```
WITH high_value_users AS (
  FOR u IN users
    FILTER u.total_spent > 1000
    RETURN u
),
recent_orders AS (
  FOR o IN orders
    FILTER o.date > DATE_SUB(NOW(), 30, 'days')
    RETURN o
)
FOR user IN high_value_users
  FOR order IN recent_orders
    FILTER order.user_id == user._id
    RETURN {user: user.name, order: order.id}
```

**Subquery Types:**

**Scalar Subquery (returns single value):**
```
FOR user IN users
  LET order_count = (
    FOR order IN orders
      FILTER order.user_id == user._id
      RETURN 1
    | COUNT
  )
  RETURN {user: user.name, orders: order_count}
```

**Array Subquery (returns array):**
```
FOR user IN users
  LET orders = (
    FOR order IN orders
      FILTER order.user_id == user._id
      RETURN order
  )
  RETURN {user: user.name, orders: orders}
```

**Exists Subquery:**
```
FOR user IN users
  FILTER EXISTS (
    FOR order IN orders
      FILTER order.user_id == user._id
      FILTER order.total > 1000
      RETURN order
  )
  RETURN user
```

**CTE Cache:**
```cpp
CTECache cache;

// Cache CTE result
std::string cte_name = "high_value_users";
nlohmann::json cte_result = execute_cte(cte_query);
cache.put(cte_name, cte_result);

// Reuse CTE result
if (auto cached = cache.get(cte_name)) {
    // Use cached result
    return cached.value();
}
```

**Thread Safety:** CTE cache is thread-safe.

### Subquery Optimizer
**Location:** `subquery_optimizer.h`, `../../src/query/subquery_optimizer.cpp`

Optimization of subqueries and CTEs.

**Optimizations:**
- **Subquery Flattening**: Convert correlated subqueries to joins
- **CTE Materialization**: Materialize CTEs when referenced multiple times
- **Subquery Pushdown**: Push subqueries to storage layer when possible
- **Exists → Semi-Join**: Convert EXISTS subqueries to semi-joins
- **NOT EXISTS → Anti-Join**: Convert NOT EXISTS to anti-joins

**Example Transformations:**

**Before (Correlated Subquery):**
```
FOR user IN users
  FILTER (
    FOR order IN orders
      FILTER order.user_id == user._id
      RETURN 1
    | COUNT
  ) > 5
  RETURN user
```

**After (Join):**
```
FOR user IN users
  FOR order IN orders
    FILTER order.user_id == user._id
    COLLECT user_id = user._id INTO groups
    FILTER COUNT(groups) > 5
    RETURN user
```

**API Usage:**
```cpp
SubqueryOptimizer optimizer;

// Optimize subquery
auto optimized_plan = optimizer.optimize(query_ast);

// Check if optimization applied
if (optimized_plan.optimization_applied) {
    std::cout << "Applied: " << optimized_plan.optimization_type << std::endl;
}
```

**Thread Safety:** Optimizer is thread-safe.

## Function Registry

### Overview
**Location:** `functions/function_registry.h`, `../../src/query/functions/`

Comprehensive function library with 100+ built-in functions across 25+ categories.

**Registration:**
```cpp
FunctionRegistry registry;

// Register custom function
registry.registerFunction(
    "MY_FUNC",
    [](const std::vector<Value>& args) -> Value {
        // Implementation
    },
    2,  // Min args
    3   // Max args (optional)
);

// Call function
auto result = registry.call("MY_FUNC", {arg1, arg2});
```

### Function Categories

#### String Functions
**Location:** `functions/string_functions.h`

**Functions:**
- `CONCAT(str1, str2, ...)` - Concatenate strings
- `SUBSTRING(str, start, length)` - Extract substring
- `UPPER(str)`, `LOWER(str)` - Case conversion
- `TRIM(str)`, `LTRIM(str)`, `RTRIM(str)` - Whitespace removal
- `REPLACE(str, search, replace)` - String replacement
- `SPLIT(str, delimiter)` - Split into array
- `CONTAINS(str, search)` - Check if string contains substring
- `REGEX_MATCH(str, pattern)` - Regular expression matching
- `REGEX_REPLACE(str, pattern, replacement)` - Regex replacement
- `LEVENSHTEIN(str1, str2)` - Edit distance
- `SOUNDEX(str)` - Phonetic encoding

#### Math Functions
**Location:** `functions/math_functions.h`

**Functions:**
- `ABS(x)`, `CEIL(x)`, `FLOOR(x)`, `ROUND(x, decimals)`
- `SQRT(x)`, `POW(x, y)`, `EXP(x)`, `LOG(x)`, `LOG10(x)`
- `SIN(x)`, `COS(x)`, `TAN(x)`, `ASIN(x)`, `ACOS(x)`, `ATAN(x)`
- `MIN(a, b, ...)`, `MAX(a, b, ...)`
- `RAND()`, `RAND_RANGE(min, max)`

#### Date/Time Functions
**Location:** `functions/date_functions.h`

**Functions:**
- `NOW()` - Current timestamp
- `DATE_ADD(date, amount, unit)` - Add time interval
- `DATE_SUB(date, amount, unit)` - Subtract time interval
- `DATE_DIFF(date1, date2, unit)` - Difference between dates
- `DATE_FORMAT(date, format)` - Format date as string
- `DATE_PARSE(str, format)` - Parse string to date
- `YEAR(date)`, `MONTH(date)`, `DAY(date)`, `HOUR(date)`, `MINUTE(date)`, `SECOND(date)`
- `DAY_OF_WEEK(date)`, `DAY_OF_YEAR(date)`, `WEEK_OF_YEAR(date)`
- `IS_HOLIDAY(date, country)` - Check if date is holiday
- `NEXT_BUSINESS_DAY(date, country)` - Next business day

#### Array Functions
**Location:** `functions/array_functions.h`

**Functions:**
- `LENGTH(array)` - Array length
- `FIRST(array)`, `LAST(array)` - First/last element
- `NTH(array, n)` - N-th element
- `PUSH(array, value)`, `POP(array)` - Add/remove elements
- `REVERSE(array)` - Reverse array
- `FLATTEN(array)` - Flatten nested arrays
- `UNION(arr1, arr2)`, `INTERSECTION(arr1, arr2)`, `DIFFERENCE(arr1, arr2)` - Set operations
- `UNIQUE(array)` - Remove duplicates
- `SORT(array)` - Sort array

#### JSON/Document Functions
**Location:** `functions/document_functions.h`, `functions/json_path_functions.h`

**Functions:**
- `JSON_PARSE(str)` - Parse JSON string
- `JSON_STRINGIFY(obj)` - Serialize to JSON
- `JSON_MERGE(obj1, obj2)` - Merge objects
- `JSON_PATH(obj, path)` - JSONPath query
- `JSON_EXTRACT(obj, path)` - Extract value by path
- `HAS_KEY(obj, key)` - Check if key exists
- `KEYS(obj)` - Get all keys
- `VALUES(obj)` - Get all values

#### Geospatial Functions
**Location:** `functions/geo_functions.h`, `functions/crs_functions.h`

**Functions:**
- `ST_Distance(geom1, geom2)` - Distance between geometries
- `ST_Within(geom, polygon)` - Check if geometry is within polygon
- `ST_Intersects(geom1, geom2)` - Check if geometries intersect
- `ST_Buffer(geom, distance)` - Buffer around geometry
- `ST_Area(polygon)` - Calculate polygon area
- `ST_Length(linestring)` - Calculate linestring length
- `ST_Centroid(geom)` - Calculate centroid
- `ST_Transform(geom, from_srid, to_srid)` - Coordinate transformation
- `ST_MakePoint(lon, lat)` - Create point
- `ST_MakeLine(points)` - Create linestring
- `ST_MakePolygon(rings)` - Create polygon

#### Vector Functions
**Location:** `functions/vector_functions.h`

**Functions:**
- `VECTOR_DISTANCE(vec1, vec2, metric)` - Distance (euclidean, cosine, manhattan)
- `VECTOR_SIMILARITY(vec1, vec2)` - Cosine similarity
- `VECTOR_NORMALIZE(vec)` - Normalize vector
- `VECTOR_DOT(vec1, vec2)` - Dot product
- `VECTOR_MAGNITUDE(vec)` - Vector magnitude

#### Graph Functions
**Location:** `functions/graph_functions.h`, `functions/graph_extensions.h`

**Functions:**
- `SHORTEST_PATH(start, end, edge_type)` - Shortest path between nodes
- `ALL_PATHS(start, end, max_depth)` - All paths up to max depth
- `NEIGHBORS(node, direction)` - Get neighbors
- `DEGREE(node, direction)` - Node degree
- `PAGERANK(graph)` - PageRank algorithm
- `CONNECTED_COMPONENTS(graph)` - Find connected components
- `COMMUNITY_DETECTION(graph)` - Detect communities

#### AI/ML Functions
**Location:** `functions/ai_ml_functions.h`, `functions/lora_functions.h`

**Functions:**
- `EMBED(text, model)` - Generate text embeddings
- `CLASSIFY(text, model)` - Text classification
- `SENTIMENT(text)` - Sentiment analysis
- `NER(text)` - Named entity recognition
- `SUMMARIZE(text, max_length)` - Text summarization
- `TRANSLATE(text, target_lang)` - Machine translation
- `LORA_ADAPT(base_model, lora_weights)` - Apply LoRA adaptation

#### Fulltext Functions
**Location:** `functions/fulltext_functions.h`

**Functions:**
- `MATCH(field, query)` - Fulltext search
- `PHRASE(field, phrase)` - Phrase search
- `FUZZY(field, term, distance)` - Fuzzy search
- `WILDCARD(field, pattern)` - Wildcard search
- `BOOST(field, query, boost)` - Boost query score

#### Security Functions
**Location:** `functions/security_functions.h`

**Functions:**
- `HASH(value, algorithm)` - Cryptographic hash (SHA256, SHA512, MD5)
- `HMAC(value, key, algorithm)` - HMAC signature
- `ENCRYPT(value, key)` - Encrypt value
- `DECRYPT(value, key)` - Decrypt value
- `MASK(value, mask_char)` - Mask sensitive data
- `REDACT(value, pattern)` - Redact sensitive patterns

#### Ethics Functions
**Location:** `functions/ethics_functions.h`

**Functions:**
- `BIAS_DETECT(dataset, protected_attributes)` - Detect dataset bias
- `FAIRNESS_METRICS(predictions, actuals, protected_attr)` - Calculate fairness metrics
- `ANONYMIZE(data, k)` - K-anonymization
- `DIFFERENTIAL_PRIVACY(query, epsilon)` - Apply differential privacy

#### Process Mining Functions
**Location:** `functions/process_mining_functions.h`

**Functions:**
- `DISCOVER_PROCESS(events)` - Discover process model from event log
- `CONFORMANCE_CHECK(events, model)` - Check conformance to process model
- `BOTTLENECK_ANALYSIS(events)` - Identify process bottlenecks
- `VARIANT_ANALYSIS(events)` - Analyze process variants

#### Other Function Categories
- **Collection Functions** (`collection_functions.h`) - Set operations
- **Relational Functions** (`relational_functions.h`) - SQL-style operations
- **File Functions** (`file_functions.h`) - File I/O operations
- **Retention Functions** (`retention_functions.h`) - Data retention policies
- **Holiday Provider** (`holiday_provider.h`) - Holiday calendars

## Architecture

### Query Execution Pipeline

```
AQL Query String
      ↓
Parse (AQLParser)
      ↓
AST (Abstract Syntax Tree)
      ↓
Optimize (QueryOptimizer)
      ↓
Execution Plan
      ↓
Execute (QueryEngine)
      ↓
Result Stream
      ↓
Client
```

### Component Dependencies

```
QueryEngine
├─ Storage Interface (data access)
├─ Index Manager (index operations)
├─ Expression Evaluator (predicate evaluation)
└─ Query Optimizer (plan optimization)

QueryOptimizer
├─ Adaptive Stats (ML-based tuning)
├─ Cost Model (plan costing)
└─ Index Manager (cardinality estimation)

AQLParser
└─ Function Registry (function validation)

QueryCache
└─ Semantic Cache (similarity matching)
```

## Performance Considerations

### Query Optimization

**Index Usage:**
- Always create indexes for frequently filtered columns
- Use composite indexes for multi-column filters
- Verify index usage with query plans

**Predicate Ordering:**
- Most selective predicates first
- Use cardinality estimation for optimal ordering

**Join Optimization:**
- Small table first for nested loop joins
- Use hash joins for equality joins
- Consider broadcast joins for distributed queries

### Caching Strategy

**When to Cache:**
- Repeated queries (cache hit rate >60%)
- Expensive queries (>100ms execution time)
- Stable data (infrequent updates)

**When NOT to Cache:**
- Real-time data requirements
- High update frequency
- Large result sets (>10MB)

### Streaming vs Materialization

**Use Streaming:**
- Large result sets (>1000 rows)
- Limited memory
- Progressive UI updates

**Use Materialization:**
- Small result sets (<100 rows)
- Multiple passes required
- Sorting/aggregation needed

## Version Compatibility

**v1.5.x (Current Stable):**
- All features documented here
- AQL v2.0 syntax
- Multi-model queries
- Hybrid query execution

**v1.4.x:**
- ⚠️ No semantic cache
- ⚠️ No adaptive optimizer
- ⚠️ Limited window functions

**v1.3.x:**
- ⚠️ No query federation
- ⚠️ No CTEs
- ⚠️ No streaming

**Migration Notes:**
- v1.3 → v1.4: Add streaming support to clients
- v1.4 → v1.5: Update AQL syntax for CTEs and window functions

## Implementation Cross-Reference

### Public API → Implementation Mapping

| Public Header | Implementation |
|--------------|----------------|
| `query_engine.h` | `../../src/query/query_engine.cpp` |
| `aql_parser.h` | `../../src/query/aql_parser.cpp` |
| `query_optimizer.h` | `../../src/query/query_optimizer.cpp` |
| `adaptive_optimizer.h` | `../../src/query/adaptive_optimizer.cpp` |
| `query_cache.h` | `../../src/query/query_cache.cpp` |
| `semantic_cache.h` | `../../src/query/semantic_cache.cpp` |
| `query_federation.h` | `../../src/query/query_federation.cpp` |
| `result_stream.h` | `../../src/query/result_stream.cpp` |
| `window_evaluator.h` | `../../src/query/window_evaluator.cpp` |
| `statistical_aggregator.h` | `../../src/query/statistical_aggregator.cpp` |
| `cte_subquery.h` | `../../src/query/cte_subquery.cpp` |
| `subquery_optimizer.h` | `../../src/query/subquery_optimizer.cpp` |
| `functions/*.h` | `../../src/query/functions/*.cpp` |

### Related Modules

- **Storage Module**: Provides data persistence and transactions
- **Index Module**: Provides secondary indexes, vector indexes, spatial indexes
- **Server Module**: Handles network protocols and query routing
- **Auth Module**: Provides query-level authorization

## Feature Maturity

✅ **Production-Ready:**
- AQL parsing and execution
- Query optimization
- Query caching
- Result streaming
- Window functions
- Function registry

⚠️ **Beta Features:**
- Adaptive optimization (learning-based)
- Semantic cache (embedding-based)
- Query federation (distributed)
- Subquery optimization

🔬 **Experimental:**
- Predictive caching
- GPU-accelerated query execution
- Query compilation (JIT)
- Automatic index recommendation

## Related Documentation

### Quick Links

- **Core Components:**
  - [AQL Parser](../../docs/query/aql_parser.md) - AQL syntax and parsing
  - [Query Optimizer](../../docs/query/query_optimizer.md) - Optimization strategies
  - [Query Engine](../../docs/query/query_engine.md) - Execution engine architecture
- **Architecture:**
  - [Query Execution Pipeline](../../docs/query/execution_pipeline.md) - End-to-end query flow
  - [Multi-Model Queries](../../docs/query/multi_model.md) - Cross-model query support
  - [Hybrid Queries](../../docs/query/hybrid_queries.md) - Vector+Geo, Fulltext+Spatial
- **Advanced Features:**
  - [Query Federation](../../docs/query/federation.md) - Distributed query execution
  - [Window Functions](../../docs/query/window_functions.md) - Analytical window functions
  - [CTEs and Subqueries](../../docs/query/ctes_subqueries.md) - Advanced query patterns
- **Performance:**
  - [Caching Strategies](../../docs/query/caching.md) - Query result caching
  - [Query Optimization Tips](../../docs/query/optimization_tips.md) - Best practices
  - [Performance Benchmarks](../../benchmarks/query/) - Query performance data
- **Function Reference:**
  - [AQL Function Reference](../../docs/query/function_reference.md) - Complete function catalog
  - [Custom Functions](../../docs/query/custom_functions.md) - Registering custom functions

## Contributing

When contributing to the query module:

1. Add comprehensive tests for new features
2. Update AQL parser for new syntax
3. Document function signatures and examples
4. Benchmark performance impact
5. Update this README for public API changes
6. Add function documentation to reference guide
7. Consider backward compatibility

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned query improvements
- [Storage Module](../../src/storage/README.md) - Data persistence layer
- [Index Module](../../src/index/README.md) - Index management
- [Server Module](../../src/server/README.md) - Network protocols

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
