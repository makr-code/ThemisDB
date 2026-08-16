> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Query Module — Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-07-13
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

### 8.1 Parser Safety Hardening (Phase 1, Q3 2026)

**Expression Depth Bounds**:
- Recursion depth limited to `kMaxExprDepth = 500` to prevent stack exhaustion
- Graph traversal depth bounded: min ≥ 0, max ≤ configurable limit (default 100)
- Test coverage: `test_query_parser_edge_cases.cpp` § Tests 13–16 (nesting scenarios)

**Malformed Input Handling**:
- Empty or whitespace-only queries rejected with parse error (Tests 1–2)
- Unclosed strings, parentheses, brackets, braces detected and reported (Tests 5–8)
- Invalid tokens and operator sequences fail gracefully without crash (Tests 3, 22)
- Duplicate variable bindings detected and rejected (Test 11)
- All numeric conversions (`stoll`/`stod`) wrapped in try-catch to prevent exception-based DoS (Tests 9–10)

**Mutation Safety Validation** (`AqlSafetyValidator`):
- NUL-character injection blocked (Test 24)
- DML mutations (INSERT, UPDATE, REMOVE, DELETE, REPLACE, UPSERT) detected and flagged for read-only contexts (Tests 25–31)
- DDL mutations (DROP, TRUNCATE, CREATE COLLECTION) similarly blocked (Test 31)
- READ queries pass validation (Test 32)

### 8.2 Access Validation Consistency (Phase 1, Q3 2026)

**Three-Stage Access Control Flow**:

1. **Parser Stage** (Phase 2 Agent 1, Enhanced): 
   - Collection names extracted from AST with scope validation
   - `ParserScopeContext` tracks registered collections per scope level
   - Scope boundaries enforced: prevents cross-collection access at parse time
   - No SQL injection possible (AST round-trip semantics)
   - Supports nested scopes for complex queries (CTEs, subqueries)
   
2. **Execution Stage**: All `executeAndKeys*` entry points invoke `collection_access_checker_` callback:
   - Caller provides: access predicate function + caller ID
   - QueryEngine enforces: denies execution on `ERR_QUERY_ACCESS_DENIED` if check fails
   - Scope: AND queries, OR queries, range queries, spatial queries, federation scatter
   
3. **Federation Stage**: Remote cluster enforces its own access checks; result merge respects `max_result_size_bytes` limit

**Parser Stage Scope Validation (Phase 2 Agent 1)**:

The parser now implements collection-level scope validation via:
- `ParserScopeContext`: Maintains a registry of valid collections per scope level
- `registerCollection(name)`: Adds a collection to the current scope
- `isCollectionInScope(name)`: Checks if a collection is valid in the current scope
- `validateCollectionAccess(name, context)`: Validates and returns detailed errors
- `pushScope()` / `popScope()`: Manage nested scope levels for subqueries and CTEs

**Collection Name Extraction Points**:
- `aql_parser.cpp::parseForClause()` — FOR variable IN collection (line ~885)
- `aql_parser.cpp::expectCollectionName()` — mutation statements (line ~1598)
- `continuous_query_planner.cpp::compile()` — source collection validation (line ~84)
- `continuous_query_planner.cpp::evaluate()` — runtime scope check (line ~23)

**Scope Mismatch Fixes (Phase 2 Agent 1)**:
- continuous_query_planner.cpp:24 (CRITICAL) — Added runtime scope validation in evaluate()
- aql_parser.cpp:178 (HIGH) — Enhanced readNumber() context with scope tracking
- aql_parser.cpp:234 (HIGH) — Enhanced expectCollectionName() with scope registration
- Parser now registers all encountered collections and validates scope boundaries

**Entry Points Verified**:
- `executeAndKeys(ConjunctiveQuery)` — line ~323
- `executeAndKeysWithScores(ConjunctiveQuery)` — line ~733
- `executeAndEntities(ConjunctiveQuery)` — line ~844
- `executeOrKeys(DisjunctiveQuery)` — line ~1013
- `executeRangeQuery(...)` — respects access callback
- `ContinuousQueryPlanner::compile()` — validates source collection scope
- `ContinuousPlan::evaluate()` — enforces collection scope at runtime

See `src/query/ACCESS_VALIDATION_CHECKLIST.md` for detailed cross-reference matrix.

### 8.2.1 Executor Stage Scope Enforcement (Phase 2 Agent 3, Q3 2026)

**Execution-Time Scope Isolation**:

The executor stage enforces scope boundaries when assembling and merging results across shards, materialized views, and result streams. This prevents cross-scope data leakage during:
- Federated query result merging (per-shard scope validation)
- Materialized view snapshot access (scope tagging on refresh)
- Result pagination (scope boundary checks)

**ScopeEnforcer Interface** (`src/query/scope_enforcer.h`):

Public Methods:
- `validateResultScope(result_data, expected_scope)` — Validates result belongs to expected collection/shard
- `enforceAccumulatedScopeBounds(scope_key, bytes, max)` — Prevents per-scope resource exhaustion during merge
- `validatePageScope(begin, end, total, scope)` — Ensures pagination respects scope boundaries
- `extractResultScope(result_data)` — Extracts scope metadata from JSON result
- `resetScopeAccumulation(scope_key)` — Clears accumulation tracking for new queries

**Federated Query Scope Enforcement**:

In `QueryFederation::executeFederatedRAGQuery()` (line ~230):
- Creates `ScopeEnforcer` for result validation pipeline
- Per-shard scope tracking via `scope_key = collection:shard_id`
- Accumulated byte checking prevents any single shard from exceeding resource limits
- Shard result validation before merge prevents cross-shard contamination
- All RAG documents tagged with shard_id for lineage tracking

In `QueryFederation::execute()` with PARTITION_PRUNING (line ~350):
- Shard result validation via scope enforcer
- Each shard result validated against `QueryScope{collection, shard_id, is_federated=true}`
- Scope validation logged; violations logged as warnings
- Invalid scopes do not block merge but are tracked for compliance auditing

**Materialized View Scope Isolation**:

In `MaterializedView::refresh()` (line ~175):
- Each row tagged with `_view_scope` metadata during full refresh
- Scope metadata includes: collection name, generation counter, refresh timestamp
- Tags are non-invasive (stored in separate `_view_scope` object field)
- Scope generation incremented on each full refresh to track snapshot age
- Incremental refreshes preserve existing scope tags

**Result Stream Scope Validation** (Phase 3 implementation):
- `ResultStream::fillBuffer()` will validate each batch against expected scope
- `ResultStream::next()` will check result scope before returning
- `ResultStream::batch()` will enforce pagination scope bounds
- `ResultStream::skip()` will validate skip range within total results

**Test Coverage** (Phase 2):
- `tests/query/test_query_federation_scope_safety.cpp` (160+ lines)
  - Multi-shard scope isolation tests
  - Per-shard accumulated size limit tests
  - Cross-shard contamination detection tests
  - Pagination scope validation tests
  
- `tests/query/test_materialized_view_scope_isolation.cpp` (110+ lines)
  - View scope tagging on refresh
  - Scope metadata preservation during delta operations
  - Scope consistency across multiple view accesses
  - Concurrent read scope consistency tests

### 8.2.2 Query Optimizer Scope Bounds and Validation (Phase 2 Agent 2, Q3 2026)

**Scope Mismatch Gap Closure**: HIGH-severity gaps fixed via scope bounds validation.

**Problem (gap query_optimizer.cpp:345)**:
- Query plans did not enforce result boundary limits
- Federated queries could leak results across scope boundaries
- No validation that optimizer results respect scope constraints

**Solution: Three-Layer Scope Validation**:

1. **Plan Scope Configuration** (`setScopeBounds`):
   ```cpp
   // Set scope bounds on query plan
   optimizer.setScopeBounds(plan, "tenant_123/db_orders", 
                           max_rows=50000, max_bytes=10MB,
                           enforce_federation=true);
   ```
   - Captures scope identity (database/collection/tenant)
   - Configures row and byte limits
   - Enables federation scope isolation when distributed

2. **Result Boundary Validation** (`validateResultBounds`):
   ```cpp
   // Validate result doesn't escape scope
   bool ok = optimizer.validateResultBounds(plan, actual_rows, actual_bytes);
   ```
   - Detects row overflow (actual_rows > max_result_rows)
   - Detects byte overflow (actual_bytes > max_result_bytes)
   - Emits metrics on violations
   - Logs detailed diagnostics for troubleshooting

3. **Federation Scope Isolation** (`validateFederationScopeIsolation`):
   ```cpp
   // Prevent cross-scope federation leakage
   bool isolated = optimizer.validateFederationScopeIsolation(plan, remote_scope_id);
   ```
   - Ensures local scope_id matches remote scope_id
   - Prevents distributed query merging across tenant/database boundaries
   - Blocks if isolation is enforced but scopes mismatch

**Integration Points**:
- `chooseOrderForAndQuery()`: Can optionally call setScopeBounds before returning plan
- `executeOptimizedKeys()` / `executeOptimizedEntities()`: Callers validate result bounds
- `optimizeForDistribution()`: Sets federation isolation flag for sharded queries

**Backward Compatibility**:
- Plans without scope bounds pass validation (legacy queries)
- Scope validation is opt-in via setScopeBounds
- Existing code continues to work without modification

**Test Coverage** (`test_query_optimizer_scope_bounds.cpp`):
- 25 deterministic test cases covering:
  - Scope bounds configuration with row/byte/both limits
  - Result boundary overflow detection
  - Federation scope isolation enforcement
  - Edge cases: nested scopes, multi-collection, zero rows, large limits
  - Thread safety of concurrent scope configuration

### 8.3 General Security Properties

- AQL does not support arbitrary code execution; only registered functions are callable.
- Federation transport restricts request/redirect protocols to HTTP/HTTPS and validates endpoint registration inputs.
- Continuous-query runtime bounds registry growth and injection-queue depth.

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

## 12. Source Verification Evidence

- Parser and translation references in this document were re-checked against
  `src/query/aql_parser.cpp`, `src/query/aql_parser_json.cpp`,
  `src/query/aql_translator.cpp`, and `include/query/aql_parser.h`.
- Optimizer and execution references were re-checked against
  `src/query/query_optimizer.cpp`, `src/query/adaptive_optimizer.cpp`,
  `src/query/query_engine.cpp`, and `src/query/runtime_reoptimizer.cpp`.
- Distributed/federated and cancellation references were re-checked against
  `src/query/query_federation.cpp`, `src/query/cross_cluster_federation.cpp`,
  `src/query/query_canceller.cpp`, and `src/query/continuous_query_engine.cpp`.

---

## 12. LLM Integration Points

The Query module intentionally exposes only a **read-only, public parser interface** for consumption by the LLM assistance layer (`src/aql/`). This prevents circular dependencies and keeps the query engine independent of LLM components.

### 12.1 Public APIs for LLM Layer

**See:** `src/query/AQL_LLM_INTEGRATION_CONTRACT.md` (canonical integration specification)

**Exposed Interfaces:**
- `AQLParserService` abstract class (stable interface for parser calls)
- `AQLParserServiceImpl` concrete implementation
- `ParseResult` struct (AST + diagnostics)
- `ParserDiagnostics` struct (error location, suggestions)

**One-Way Dependency:**
```
src/aql/ (LLM Integration)
    └─→ calls AQLParserService::parse() [src/query/]
    
src/query/ (Query Engine)
    └─→ NEVER imports from src/aql/
```

### 12.2 LLM Validation Pipeline

When the LLM layer generates candidate AQL strings (from natural language), it **MUST**:
1. Call `AQLParserService::parse(aql_string)` to validate syntax
2. On parse failure: attempt retry with corrective feedback (max 1 retry)
3. Return only validated AQL to the user (never unvalidated strings)
4. Emit metrics: `aql_validation_failures_total`, `aql_validation_successes_total`

**Location:** `src/aql/llm_aql_handler.cpp::validateAQLWithParser()`

### 12.3 SLA & Guarantees

- **Parser call duration:** ≤ 500ms (includes AST construction and diagnostics)
- **Timeout handling:** Convert to `ParseResult::error` if exceeded
- **Backward compatibility:** Query engine continues to work if LLM layer is unavailable

---

## 13. References

- `src/query/README.md` — module overview
- `src/query/FUTURE_ENHANCEMENTS.md` — roadmap
- `src/query/AQL_LLM_INTEGRATION_CONTRACT.md` — LLM integration specification (canonical)
- `src/aql/README.md` — LLM integration layer overview
- `docs/aql_language_guide.md` — AQL language reference
- `docs/query_optimizer.md` — optimizer internals
- `ARCHITECTURE.md` (root) — full system architecture
