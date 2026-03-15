<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Query Module Roadmap

## Current Status
v1.x – Production-grade AQL query engine with cost-based optimizer, multi-model execution, and comprehensive caching. AQL covers relational, document, graph, vector, spatial, and timeseries models with 100+ built-in functions. SQL dialect compatibility layer, query plan visualization (EXPLAIN / ANALYZE), incremental CTE materialization, adaptive re-optimization, multi-statement transactions, and cross-cluster federation are all implemented.

## Completed ✅
- [x] AQL parser with full AST generation (FOR, FILTER, SORT, LIMIT, RETURN, LET, COLLECT, WITH)
- [x] Expression types: binary/unary ops, literals, field access, function calls, arrays, objects
- [x] SimilarityCall (vector search), ProximityCall (geospatial), SubqueryExpr, AnyExpr/AllExpr
- [x] Cost-based query optimizer with adaptive learning
- [x] Multi-model execution (relational, document, graph, vector, spatial)
- [x] Hybrid queries (vector + geo, fulltext + geo, graph + spatial)
- [x] Query result streaming and pagination
- [x] Query caching (exact, semantic, CTE)
- [x] Expression evaluator and function registry (25+ categories, 100+ functions)
- [x] Graph traversal (BFS/DFS, shortest path, recursive)
- [x] Window functions and statistical aggregation
- [x] Distributed query federation
- [x] LLM integration (LLM INFER, LLM RAG, LLM EMBED)
- [x] Process mining and ethics functions
- [x] JSON query support (`aql_parser_json.cpp`)
- [x] Parallel scan for large collection full-table queries (Issue: #2432)
- [x] SQL dialect compatibility layer – `sql_parser.cpp` (SELECT/INSERT/UPDATE/DELETE passthrough) (Issue: #2236)
- [x] Query plan visualization API – `query_plan_visualizer.cpp` (EXPLAIN / EXPLAIN ANALYZE) (PR: #2075)
- [x] Incremental view maintenance for materialized CTEs – `materialized_cte.cpp` (Issue: #1431)
- [x] Query result type annotations – `result_type_annotation.cpp` (Issue: #1432)
- [x] Adaptive query re-optimization on runtime statistics – `adaptive_optimizer.cpp`, `runtime_reoptimizer.cpp` (Issue: #2232)
- [x] Multi-statement transaction AQL (BEGIN/COMMIT in query) – `aql_parser.cpp` (Issue: #2435)
- [x] `QueryExpressionEvaluator` – delegates to AQL parser + `evaluateCondition()`; `get_expression_type()` returns `"AQL"`
- [x] Query plan caching – `plan_cache.h/cpp` (fingerprinting, parameterized reuse, table/stats invalidation, 24h TTL) (Issue: #196)
- [x] Query compilation (JIT) – `query_compiler.h/cpp` (hot-path specialisation, FNV-1a fingerprinting, fallback to interpreter) (Issue: #89)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [P] Query result type annotations for client SDK code generation (Issue: #1432)
- [x] Query cancellation via request ID (Issue: #2431)

### Long-term (6-12 months)
- [x] Vectorized execution engine (column-store style batch processing) (Issue: #2434)
- [P] Adaptive query re-optimization on runtime statistics (Issue: #2232)
- [P] Multi-statement transaction AQL (BEGIN/COMMIT in query) (Issue: #2435, PR: #2608)

## Implementation Phases

### Phase 1: Core AQL Engine (Status: Completed ✅)
- [x] AQL parser with full AST generation (`aql_parser.cpp`, `aql_parser_json.cpp`)
- [x] Expression types: binary/unary ops, literals, field access, function calls, SimilarityCall, ProximityCall, SubqueryExpr
- [x] Cost-based query optimizer with adaptive learning
- [x] Multi-model execution (relational, document, graph, vector, spatial)
- [x] Hybrid queries (vector + geo, fulltext + geo, graph + spatial)
- [x] Query result streaming and pagination
- [x] Query caching (exact, semantic, CTE)
- [x] Expression evaluator and function registry (25+ categories, 100+ functions)
- [x] Graph traversal (BFS/DFS, shortest path, recursive)
- [x] Window functions and statistical aggregation
- [x] Distributed query federation
- [x] LLM integration (LLM INFER, LLM RAG, LLM EMBED)
- [x] Process mining and ethics functions

### Phase 2: SQL Compatibility & Plan Visualization (Status: Completed ✅)
- [x] SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE passthrough) → `sql_parser.cpp`
- [x] Query plan visualization API (EXPLAIN / EXPLAIN ANALYZE) → `query_plan_visualizer.cpp`
- [x] Incremental view maintenance for materialized CTEs → `materialized_cte.cpp`

### Phase 3: Resource Management & UDF (Status: In Progress 🚧)
- [x] Query result type annotations for client SDK code generation → `result_type_annotation.cpp`
- [x] Per-query resource limits (max rows, max memory, timeout)
- [x] Query cancellation via request ID
- [x] Parallel scan for large collection full-table queries
- [x] User-defined functions (UDF) registration API

### Phase 4: Vectorized Execution & Cross-Cluster Federation (Status: In Progress 🚧)
- [x] Vectorized execution engine (column-store style batch processing)
  - Implemented in `include/query/vectorized_execution.h` (`VectorizedExecutionEngine`, `VectorizedQueryPlan`, `VectorizedPredicate`, `VectorizedAggregation`)
    and `src/query/vectorized_execution.cpp`
  - Delegates to `analytics/columnar_execution.h` for SIMD batch processing with `SelectionVector` late-materialization
  - Accepts `std::vector<nlohmann::json>` rows, converts to columnar layout, then materializes results back to JSON
- [P] Adaptive query re-optimization on runtime statistics (Issue: #2232)
- [x] Cross-cluster federated AQL with cost estimation (Issue: #2233)
  - Implemented in `include/query/cross_cluster_federation.h` (`ClusterEndpoint`, `ClusterCostEstimate`, `CrossClusterFederator`)
    and `src/query/cross_cluster_federation.cpp`
  - Connects to independent ThemisDB instances via their HTTP `/query/aql` endpoint
  - Cost model: `total_cost = (estimated_rows × 0.001) + (network_latency_ms × 1.0)`
  - Optional cost-based pruning: exclude clusters whose cost exceeds a configurable multiple of the cheapest cluster
  - Parallel execution via `std::async`; injectable `HttpPostFn` test double for unit tests
  - Tests: `tests/test_cross_cluster_federation.cpp` (28 unit tests)
- [x] Multi-statement transaction AQL (BEGIN/COMMIT in query)
- [x] SPARQL compatibility for RDF / knowledge-graph queries

## Production Readiness Checklist
- [x] Unit tests coverage > 80%
- [x] Integration tests (multi-model queries, optimizer plan correctness)
- [?] Performance benchmarks (QPS, optimizer overhead, cache hit rate)
- [?] Security audit (AQL injection prevention, resource exhaustion)
- [x] Documentation complete
- [x] API stability guaranteed

## Known Issues & Limitations
- AQLParser instances are NOT thread-safe; create per-thread or protect with a mutex.
- `query_canceller.cpp`, `query_federation.cpp` were missing from `cmake/CMakeLists.txt` (fixed: 2026-03-10); `materialized_cte.cpp`, `sparql_parser.cpp`, `vectorized_execution.cpp`, `query_canceller.cpp`, `query_federation.cpp` were missing from `cmake/ModularBuild.cmake` (fixed: 2026-03-10).
- SQL dialect support covers SELECT/INSERT/UPDATE/DELETE (basic syntax only); the following
  SQL features are **not** currently supported by the transpiler and will return a parse error:
  - JOINs (INNER JOIN, LEFT JOIN, etc.)
  - Subqueries in SELECT / WHERE
  - GROUP BY / HAVING
  - Window functions (OVER, PARTITION BY)
  - DDL statements (CREATE TABLE, ALTER TABLE, DROP TABLE, TRUNCATE)
  - MERGE / UPSERT
- Distributed federation cost estimation is approximate.
- **[Fixed – March 2026]** Build system audit: `query_cache_manager.cpp`, `functions/lora_functions.cpp`,
  and `functions/process_mining_functions.cpp` were absent from `cmake/CMakeLists.txt`;
  `materialized_cte.cpp`, `vectorized_execution.cpp`, `sparql_parser.cpp`, and
  `functions/fulltext_functions.cpp` were absent from `cmake/ModularBuild.cmake` THEMIS_QUERY_SOURCES;
  a duplicate `result_stream.cpp` entry in the graph section of ModularBuild.cmake was removed.
  `query_compiler.cpp` (JIT, Issue #89) added to both `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`.
  All 38 `src/query/**/*.cpp` files are now registered in both build files.

## Breaking Changes
- AQL syntax is stable from v1.x; new keywords are additive.
- Function registry signatures are frozen for 100+ existing functions; new functions only.
