> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Query Module — Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-05-31
**Module Path:** `src/query/`

---

## 1. Overview

The Query module provides ThemisDB's AQL (Advanced Query Language) engine. It parses AQL
statements into ASTs, optimizes them through a cost-based planner, and executes multi-model
query plans across relational, document, graph, vector, spatial, and time-series data models.

AQL is based on ArangoDB's AQL but significantly extended with vector similarity functions,
LLM integration commands, geospatial ST_* functions, timeseries windowing, and distributed
query federation.

---

## 2. Design Principles

- **Multi-Model Unification** – a single AQL statement can mix vector search, graph
  traversal, geospatial filters, and relational projections; the execution engine handles
  heterogeneous operator pipelines.
- **Cost-Based Optimization** – the optimizer uses statistics from the metadata module
  to choose execution strategies (join algorithms, index selection, push-down predicates).
- **Adaptive Optimization** – `adaptive_optimizer.cpp` adjusts the cost model based on
  actual execution statistics.
- **Multi-Level Caching** – exact result cache, semantic cache (near-duplicate queries),
  CTE cache, and workload-based cache strategy.
- **Federation** – `query_federation.cpp` enables queries that span multiple ThemisDB
  instances or external data sources.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `aql_parser.cpp` | AQL → AST (FOR/FILTER/SORT/LIMIT/RETURN/LET/COLLECT/WITH) |
| `aql_parser_json.cpp` | JSON query object → AST |
| `aql_translator.cpp` | AST → logical plan |
| `query_optimizer.cpp` | Cost-based logical plan optimization |
| `optimizer_cost_model.cpp` | Cost model: selectivity, cardinality, I/O estimates |
| `adaptive_optimizer.cpp` | Runtime feedback → cost model updates |
| `query_engine.cpp` | Physical execution: operator pipeline |
| `aql_runner.cpp` | Top-level query execution orchestrator |
| `cte_subquery.cpp` / `materialized_cte.cpp` / `cte_cache.cpp` | CTE evaluation and caching |
| `let_evaluator.cpp` | LET variable evaluation |
| `window_evaluator.cpp` | Window functions (RANK, LAG, LEAD, etc.) |
| `statistical_aggregator.cpp` | Statistical aggregation functions |
| `result_stream.cpp` | Result streaming and pagination |
| `result_type_annotation.cpp` | Result type inference |
| `query_cache.cpp` / `query_cache_manager.cpp` | Exact query result cache |
| `semantic_cache.cpp` | Semantic similarity-based cache |
| `workload_cache_strategy.cpp` | Adaptive cache eviction strategy |
| `query_plan_visualizer.cpp` | Human-readable query plan output |
| `query_federation.cpp` | Distributed query federation |
| `sql_parser.cpp` | SQL → AQL translation (basic compatibility layer) |
| `functions/` | 100+ AQL function implementations |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                 AQL Query String (from client)                   │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     AQL Parser                                   │
│   tokenize → parse → build AST                                  │
└──────────────────────────┬──────────────────────────────────────┘
                           │ AST
┌──────────────────────────▼──────────────────────────────────────┐
│                    Query Optimizer                               │
│  logical → cost-based rewrite → physical plan                   │
│  adaptive_optimizer: update cost model from execution stats     │
└──────────────────────────┬──────────────────────────────────────┘
                           │ physical plan
┌──────────────────────────▼──────────────────────────────────────┐
│                  Query Engine (Execution)                        │
│                                                                  │
│  FOR → scan/index lookup                                         │
│  FILTER → predicate evaluation                                   │
│  SORT → external sort / top-k                                    │
│  COLLECT → hash aggregate                                        │
│  RETURN → projection + result_stream                            │
│  Window / CTE / Subquery operators                              │
└──────────────────────────┬──────────────────────────────────────┘
                           │
          ┌────────────────┴───────────────────┐
          │                                    │
   src/index/ (lookups)              src/storage/ (scans)
```

---

## 4. Data Flow

### 4.1 Simple Query

```
AQL: "FOR u IN users FILTER u.age > 30 RETURN u"
    │
    ├─ AqlParser: tokenize → AST {ForNode, FilterNode, ReturnNode}
    │
    ├─ QueryOptimizer:
    │       ├─ metadata.getStats("users.age") → high selectivity
    │       └─ rewrite: use B-tree index on "age" (push predicate)
    │
    ├─ QueryEngine:
    │       ├─ index lookup: age > 30 → doc IDs
    │       ├─ filter residuals
    │       └─ project RETURN fields
    │
    └─ ResultStream → paginated results to client
```

### 4.2 Hybrid Query (Vector + Geo)

```
AQL: "FOR doc IN documents
       LET score = SIMILARITY(doc.embedding, @query_vec)
       FILTER ST_Distance(doc.location, @center) < 1000
       SORT score DESC LIMIT 10 RETURN doc"
    │
    ├─ Optimizer: vector scan → geo filter (push geo predicate early)
    │
    ├─ Execution:
    │       ├─ VectorIndex.search(query_vec, k=100) → candidates
    │       ├─ geo_module.ST_Distance(candidate.location, center) < 1000 → filter
    │       └─ sort by score, limit 10
    │
    └─ results
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/index/` | Index lookups (vector, B-tree, graph, spatial) |
| **Uses** | `src/storage/` | Collection scans and document reads |
| **Uses** | `src/metadata/` | Schema and statistics for optimization |
| **Uses** | `src/analytics/` | Analytical sub-plan execution |
| **Uses** | `src/cache/` | Query result caching |
| **Uses** | `src/geo/` | ST_* function execution |
| **Uses** | `src/llm/` | LLM INFER/RAG/EMBED commands |
| **Called by** | `src/server/` | Query API handlers |

---

## 6. Threading & Concurrency Model

- `AQLParser` is stateless (`include/query/aql_parser.h`) and can be called concurrently.
- `QueryEngine` enforces collection-level access checks when `collection_access_checker_` is configured (`src/query/query_engine.cpp`).
- Continuous-query runtime bounds registry growth and injection-queue depth (`src/query/continuous_query_engine.cpp`).
- Cross-cluster federation hardens outbound transport with URL scheme validation and restricted redirect/protocol handling (`src/query/cross_cluster_federation.cpp`).

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Cost-based optimization | Uses cardinality + selectivity to choose best join/scan strategy |
| Index push-down | Predicates pushed to index scan to minimize rows read |
| Vectorized execution | SIMD-enabled operators for aggregation |
| Multi-level cache | Exact → semantic → CTE cache hierarchy |
| Streaming results | `result_stream.cpp` enables pagination without full materialization |

---

## 8. Security Considerations

- AQL does not support arbitrary code execution; only registered functions are callable.
- Parser recursion depth is bounded (`kMaxExprDepth = 500`, `kMaxTraversalDepth = 100`) to avoid stack-overflow style abuse.
- Query execution can fail closed with `ERR_QUERY_ACCESS_DENIED` when caller-provided collection access checks deny execution.
- Federation transport restricts request/redirect protocols to HTTP/HTTPS and validates endpoint registration inputs.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `query.cache.size_mb` | 256 | Exact query cache size |
| `query.cache.semantic.enabled` | true | Enable semantic cache |
| `query.optimizer.adaptive` | true | Enable adaptive optimizer |
| `query.max_result_size_mb` | 100 | Max result set size |
| `query.max_runtime_s` | 30 | Query timeout |

---

## 10. Error Handling

| Error Type | HTTP Code | Strategy |
|---|---|---|
| Parse error | 400 | Return error with line/column |
| Function not found | 400 | Return unknown function error |
| Index missing | 200 | Fall back to full scan; warn |
| Query timeout | 408 | Cancel in-flight operators; return error |
| OOM during execution | 507 | Spill to disk (planned); currently abort |

---

## 11. Known Limitations & Future Work

- SQL compatibility layer (`sql_parser.cpp`) is basic; complex SQL with window functions
  is not fully supported.
- Spill-to-disk for large intermediate results is planned.
- Additional benchmark evidence is still needed for some vectorized and federated performance envelopes.
- Some advanced optimization and distributed behaviors continue to be hardened incrementally.

---

## 12. References

- `src/query/README.md` — module overview
- `src/query/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/aql_language_guide.md` — AQL language reference
- `docs/query_optimizer.md` — optimizer internals
- `ARCHITECTURE.md` (root) — full system architecture
