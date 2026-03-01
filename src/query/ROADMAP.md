<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Query Module Roadmap

## Current Status
v1.x – Production-grade AQL query engine with cost-based optimizer, multi-model execution, and comprehensive caching. AQL covers relational, document, graph, vector, spatial, and timeseries models with 100+ built-in functions.

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

## In Progress 🚧
- [P] SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE passthrough) (Target: Q2 2026) (Issue: #2236)
- [P] Query plan visualization API (EXPLAIN / EXPLAIN ANALYZE) (Target: Q2 2026) (PR: #2075)
- [P] Incremental view maintenance for materialized CTEs (Target: Q3 2026) (Issue: #1431)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [P] Query result type annotations for client SDK code generation (Issue: #1432)
- [x] Per-query resource limits (max rows, max memory, timeout) (Issue: #1967)
  - Implemented in `include/query/query_resource_limits.h` (`QueryResourceLimits`, `QueryResourceGuard`) and `src/query/aql_runner.cpp` (`executeAqlWithLimits`)
  - Enforces max rows, max memory bytes (serialised JSON proxy), and wall-clock timeout per query
  - Tests: `tests/test_query_resource_limits.cpp` (guard unit tests + integration tests)
- [!] Query cancellation via request ID (Issue: #2431)
- [x] User-defined functions (UDF) registration API (Issue: #2433)

### Long-term (6-12 months)
- [I] Vectorized execution engine (column-store style batch processing) (Issue: #2434)
- [P] Adaptive query re-optimization on runtime statistics (Issue: #2232)
- [x] Cross-cluster federated AQL with cost estimation (Issue: #2233)
- [P] Multi-statement transaction AQL (BEGIN/COMMIT in query) (Issue: #2435, PR: #2608)
- [I] SPARQL compatibility for RDF / knowledge-graph queries (Issue: #2235)

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

### Phase 2: SQL Compatibility & Plan Visualization (Status: In Progress 🚧)
- [P] SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE passthrough)
- [~] Query plan visualization API (EXPLAIN / EXPLAIN ANALYZE)
- [x] Incremental view maintenance for materialized CTEs

### Phase 3: Resource Management & UDF (Status: Planned 📋)
- [x] Query result type annotations for client SDK code generation
- [x] Per-query resource limits (max rows, max memory, timeout)
- [ ] Query cancellation via request ID
- [x] Parallel scan for large collection full-table queries
- [x] User-defined functions (UDF) registration API

### Phase 4: Vectorized Execution & Cross-Cluster Federation (Status: In Progress 🚧)
- [ ] Vectorized execution engine (column-store style batch processing)
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
- [ ] SPARQL compatibility for RDF / knowledge-graph queries

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (multi-model queries, optimizer plan correctness)
- [?] Performance benchmarks (QPS, optimizer overhead, cache hit rate)
- [?] Security audit (AQL injection prevention, resource exhaustion)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- AQLParser instances are NOT thread-safe; create per-thread or protect with a mutex.
- SQL dialect support covers SELECT/INSERT/UPDATE/DELETE (basic syntax only); the following
  SQL features are **not** currently supported by the transpiler and will return a parse error:
  - JOINs (INNER JOIN, LEFT JOIN, etc.)
  - Subqueries in SELECT / WHERE
  - GROUP BY / HAVING
  - Window functions (OVER, PARTITION BY)
  - DDL statements (CREATE TABLE, ALTER TABLE, DROP TABLE, TRUNCATE)
  - MERGE / UPSERT
- Distributed federation cost estimation is approximate.

## Breaking Changes
- AQL syntax is stable from v1.x; new keywords are additive.
- Function registry signatures are frozen for 100+ existing functions; new functions only.
