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

## In Progress 🚧
- [I] SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE passthrough) (Target: Q2 2026) (Issue: #1421)
- [I] Query plan visualization API (EXPLAIN / EXPLAIN ANALYZE) (Target: Q2 2026) (Issue: #1430)
- [I] Incremental view maintenance for materialized CTEs (Target: Q3 2026) (Issue: #1431)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Query result type annotations for client SDK code generation (Issue: #1432)
- [ ] Per-query resource limits (max rows, max memory, timeout)
- [ ] Query cancellation via request ID
- [ ] Parallel scan for large collection full-table queries
- [ ] User-defined functions (UDF) registration API

### Long-term (6-12 months)
- [ ] Vectorized execution engine (column-store style batch processing)
- [ ] Adaptive query re-optimization on runtime statistics
- [ ] Cross-cluster federated AQL with cost estimation
- [ ] Multi-statement transaction AQL (BEGIN/COMMIT in query)
- [ ] SPARQL compatibility for RDF / knowledge-graph queries

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
- [~] SQL dialect compatibility layer (SELECT/INSERT/UPDATE/DELETE passthrough)
- [~] Query plan visualization API (EXPLAIN / EXPLAIN ANALYZE)
- [~] Incremental view maintenance for materialized CTEs

### Phase 3: Resource Management & UDF (Status: Planned 📋)
- [ ] Query result type annotations for client SDK code generation
- [ ] Per-query resource limits (max rows, max memory, timeout)
- [ ] Query cancellation via request ID
- [ ] Parallel scan for large collection full-table queries
- [ ] User-defined functions (UDF) registration API

### Phase 4: Vectorized Execution & Cross-Cluster Federation (Status: Planned 📋)
- [ ] Vectorized execution engine (column-store style batch processing)
- [ ] Adaptive query re-optimization on runtime statistics
- [ ] Cross-cluster federated AQL with cost estimation
- [ ] Multi-statement transaction AQL (BEGIN/COMMIT in query)
- [ ] SPARQL compatibility for RDF / knowledge-graph queries

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (multi-model queries, optimizer plan correctness)
- [ ] Performance benchmarks (QPS, optimizer overhead, cache hit rate)
- [ ] Security audit (AQL injection prevention, resource exhaustion)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- AQLParser instances are NOT thread-safe; create per-thread or protect with a mutex.
- SQL dialect support is not yet implemented; see `FUTURE_ENHANCEMENTS.md`.
- Distributed federation cost estimation is approximate.

## Breaking Changes
- AQL syntax is stable from v1.x; new keywords are additive.
- Function registry signatures are frozen for 100+ existing functions; new functions only.
