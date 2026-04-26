> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Graph Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production-Ready** — Core graph query optimization (cost-based algorithm selection, constrained path finding, traversal algorithm selection, adaptive optimization, parallel traversal, structural plan reuse) is functional. Distributed graph query execution across shards is implemented. EXPLAIN endpoint (`POST /api/v1/graph/query/explain`) for dry-run plan inspection is now available (Issue: #1816).

## Completed ✅
- [x] Graph query optimizer with cost-based algorithm selection
- [x] Constrained path finding (min/max length, required/forbidden nodes and edges)
- [x] Traversal algorithm selection: BFS, DFS, Dijkstra, A*, Bidirectional
- [x] Query plan generation with cost estimates
- [x] Query plan explanation and alternative strategy reporting
- [x] Execution statistics tracking for adaptive optimization
- [x] Query plan caching
- [x] Path validation and constraint checking
- [x] Integration with GraphIndexManager for graph operations
- [x] Integration with AQL for graph query execution
- [x] Query plan reuse across structurally similar queries
- [x] Parallel multi-source BFS/DFS for large graphs (Issue: #1808)
- [x] Adaptive cost model: EMA-based per-algorithm learning, enabled by default
- [x] Adaptive plan selection using execution feedback (cost model learning) (Issue: #1812)
- [x] Cost model calibration from real execution feedback (Issue: #2386)
- [x] Property graph schema-aware optimizer hints (Issue: #1819)
- [x] Distributed graph query execution across shards (Issue: #1826)
- [x] Incremental graph query execution on live updates (Issue: #1825)
- [x] Plan cache eviction with size and TTL controls (Issue: #1827)
- [x] Graph query result streaming for large path sets (Issue: #1822)
- [x] Integration with analytics module for graph algorithm reuse (Issue: #1821)
- [x] Parallel multi-source traversal for large fan-out queries — fan_out_threshold + intra-frontier parallelism (Issue: #1811)
- [x] Subgraph isomorphism queries (pattern matching) (Issue: #2390)
- [x] EXPLAIN HTTP endpoint (`POST /api/v1/graph/query/explain`) for all query types (Issue: #1816)
- [x] Query Rewriting for Graph Optimization (Issue: #250): `GraphQueryRewriter` with predicate pushdown, CSE, join reordering, materialized view utilisation, and query decomposition for parallelism (`include/graph/graph_query_rewriter.h`, `src/graph/graph_query_rewriter.cpp`)

## In Progress 🚧
- [~] GPU-accelerated BFS/DFS for massive graphs (`graph/gpu_traversal.cpp`, CPU fallback active; real CUDA kernels planned for THEMIS_ENABLE_CUDA)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] EXPLAIN output in AQL for graph query plans (Issue: #1816)
- [x] Scheduled Semantic Graph Edge Refresh with vector similarity scoring, temporal decay, and ACID batch updates (Issue: #FEATURE/ScheduledGraphEdgeRefresh)

### Long-term (6-12 months)
- [I] GPU-accelerated BFS/DFS for massive graphs (Issue: #1829)
- [ ] Ontologie-Integration: `OntologyManager` + semantische Pfad-Constraints (Target: Q3 2026)
  - Affected: `include/graph/ontology_manager.h`, `src/graph/ontology_manager.cpp`,
    `include/graph/path_constraints.h`, `src/graph/path_constraints.cpp`
  - Expected behavior: `PathConstraints::addSemanticConstraint(ontology, ruleset)` prüft
    Kanten- und Knotentypen gegen OWL-lite Konzepthierarchie; Violation-Liste zurückgeben
  - Errors: unbekannte Konzept-IDs → unconstrained (WARN); Parsing-Fehler → `Status::Error`
  - Tests: OM-01..OM-12 (`test_ontology_manager.cpp`) + SC-01..SC-10 (`test_path_constraints_semantic.cpp`)
  - Perf: ≤ 5 µs per edge constraint check; Ontologie-Load ≤ 100 ms für 10 000 Konzepte
  - Detail: `src/graph/FUTURE_ENHANCEMENTS.md` → Ontology-based Semantic Constraints
- [ ] Knowledge Graph Reasoning mit AI/ML + LoRA (Target: Q4 2026 – Q3 2027)
  - Affected: `include/graph/knowledge_graph_reasoner.h`, `src/graph/knowledge_graph_reasoner.cpp`,
    `include/rag/knowledge_graph_retriever.h`, `src/rag/knowledge_graph_retriever.cpp`,
    Integration mit `src/llm/multi_lora_manager.cpp`
  - Expected behavior: Horn-Klausel-Forward-Chaining + Erklärungsketten + incremental CDC-Trigger;
    LoRA-Adapter liefert Soft-Plausibility-Score pro Inferenzkante; Mustererkennung in Graphpfaden
  - Errors: Regelwiderspruch → `ConflictError`; LoRA-Adapter nicht geladen → deterministische Fallback-Regeln
  - Tests: KGR-01..KGR-20 (`test_knowledge_graph_reasoner.cpp`)
  - Perf: Forward-chaining 1 M Kanten ≤ 2 s kalt; ≤ 50 ms incremental
  - Detail: `src/graph/FUTURE_ENHANCEMENTS.md` → Knowledge Graph Reasoning with Ontology & ML/LoRA

## Implementation Phases

### Phase 1: Graph Query Optimizer Core (Status: Completed ✅)
- [x] Graph query optimizer with cost-based algorithm selection (`graph/query_optimizer.cpp`)
- [x] Constrained path finding (min/max length, required/forbidden nodes and edges)
- [x] Traversal algorithm selection: BFS, DFS, Dijkstra, A*, Bidirectional
- [x] Query plan generation with cost estimates and explanation output
- [x] Execution statistics tracking for adaptive optimization
- [x] Query plan caching (`graph/plan_cache.cpp`)
- [x] Path validation and constraint checking
- [x] Integration with GraphIndexManager for graph operations
- [x] Integration with AQL for graph query execution

### Phase 2: Parallel Traversal & Adaptive Planning (Status: Completed ✅)
- [x] Parallel multi-source BFS/DFS for large graphs (`graph/parallel_traversal.cpp`, Target: Q2 2026) (Issue: #1833)
- [x] Query plan reuse across structurally similar queries (Target: Q2 2026)
- [x] Adaptive cost model: EMA per algorithm, confidence-weighted blending into cost estimates
- [x] Advanced cost model calibration from real execution feedback (Target: Q3 2026)

### Phase 3: Pattern Matching & Distribution (Status: In Progress 🚧)
- [x] Subgraph isomorphism queries (pattern matching)
- [x] Distributed graph query execution across shards
- [x] Plan cache eviction with size and TTL controls
- [x] Temporal graph query optimization (time-ranged traversals)
- [~] GPU-accelerated BFS/DFS for massive graphs (`graph/gpu_traversal.cpp`, CPU fallback active; real CUDA kernels planned for THEMIS_ENABLE_CUDA)

### Phase 4: Scheduled Edge Refresh (Status: Completed ✅)
- [x] `ScheduledGraphEdgeRefreshEngine` class with `RefreshPolicy` config interface and background scheduler (`include/graph/scheduled_edge_refresh.h`, `src/graph/scheduled_edge_refresh.cpp`, Target: Q4 2026)
  - Affected files: `include/graph/scheduled_edge_refresh.h`, `src/graph/scheduled_edge_refresh.cpp`, `include/index/graph_index.h`, `src/index/graph_index.cpp`
  - Runtime: background thread wakes at `refresh_interval`; synchronous `triggerRefresh()` also available
  - Error handling: safety gate aborts batch; commit failure logged; invalid policy throws `std::invalid_argument`
  - Tests: `tests/graph/test_scheduled_edge_refresh.cpp` (45+ tests: unit, integration, regression, ChangeFeed, anomaly)
  - Performance target: cycle completes in O(V·K) per vertex for candidate discovery; brute-force for ≤10k nodes, ANN for larger graphs
  - Compatibility: no breaking changes to `GraphIndexManager` public API; `createWriteBatch()` is an additive method
- [x] Vector similarity scoring: cosine, dot-product, Euclidean (Target: Q4 2026)
- [x] Temporal decay factor for edge relevance scoring (exponential half-life, Target: Q4 2026)
- [x] Centrality-based edge weight (inverse log-degree dampening, Target: Q4 2026)
- [x] ACID batch transactions with rollback on safety-gate violations (`createWriteBatch()` on GraphIndexManager, Target: Q4 2026)
- [x] Audit trail for all edge mutations (in-memory ring buffer, 10k entries, Target: Q4 2026)
- [x] Anomaly detection: `removal_rate` + `anomaly_high_removal_rate` in `RefreshStats`; `anomaly_threshold_removal_rate` in `RefreshPolicy` (Target: Q4 2026)
- [x] Changefeed integration: `setChangefeed()` → `recordEvent()` per mutation with key prefix `graph_edge_refresh:` (Target: Q4 2026)
- [x] Integration tests: large graph (50+ nodes), cluster-embedding scenario, regression (stable graph), changefeed event verification (Target: Q4 2026)
- [x] Integration with acceleration module for ANN/GNN top-k candidate edges — `setANNIndex(IAnnIndex*)`, `rebuildANNIndex()`, ANN path in `discoverCandidateEdges()` when vertex count > `policy.ann_min_vertices` (Target: Q1 2027)
- [x] CEP event emission for edge mutations via `analytics/cep_engine` — `setCEPEventCallback(std::function<void(themisdb::analytics::Event)>)`, `EDGE_CREATE`/`EDGE_DELETE` events emitted after successful batch commit (Target: Q1 2027)
- [x] Bilingual documentation EN (`docs/scheduled_edge_refresh.md`) and DE (`docs/de/scheduled_edge_refresh.md`) including anomaly detection + Changefeed sections (Target: Q4 2026)

### Phase 5: Query Rewriting for Graph Optimization (Status: Completed ✅, Issue: #250)
- [x] `GraphQueryRewriter` class (`include/graph/graph_query_rewriter.h`, `src/graph/graph_query_rewriter.cpp`, Target: Q2 2026)
  - Affected files: `include/graph/graph_query_rewriter.h`, `src/graph/graph_query_rewriter.cpp`, `tests/test_graph_query_rewriter.cpp`
  - Runtime: pure JSON-plan transformer; no database I/O; thread-safe (stateless rules); fixed-point iteration (max 5 passes)
  - Rules: `PREDICATE_PUSHDOWN`/`PRUNE_EARLY` (promotes vertex filters to prune conditions for early BFS/DFS branch cutting), `COMMON_SUBEXPRESSION` (replaces duplicate traversals with LET-scoped refs), `JOIN_REORDERING` (swaps traversal_join operands by heuristic cardinality), `MATERIALIZED_VIEW` (tags sub-graph traversals for precomputed materialisation), `QUERY_DECOMPOSITION` (splits multi-start traversals into independent parallel subqueries)
  - Error handling: `addCustomRule(nullptr)` throws `std::invalid_argument`; `rewrite_time_limit_ms` provides a wall-clock escape hatch
  - Tests: `tests/test_graph_query_rewriter.cpp` (38 unit tests covering all acceptance criteria; standalone target `test_graph_query_rewriter`)
  - Performance: O(n) per rule pass where n = JSON plan nodes; total rewrite for typical AQL plan < 1 ms
  - Compatibility: additive new class; no changes to `GraphQueryOptimizer` public API
- [x] Common subexpression elimination for graph traversal plans
- [x] Predicate pushdown to graph traversal layer (prune early)
- [x] Join reordering for graph traversal patterns based on estimated selectivity
- [x] Materialized view utilisation tags for frequently accessed subgraphs
- [x] Query decomposition for multi-start traversal parallelism
- [x] `estimateSpeedup()` heuristic for pre/post rewrite plan comparison
- [x] `explainRewrites()` human-readable transformation summary
- [x] Custom rule hook via `addCustomRule()`
- [x] Selective rule enablement via `RewriteConfig::enabled_rules`
- [x] Wall-clock time budget via `RewriteConfig::rewrite_time_limit_ms`

### Phase 6: Ontologie-Integration & Semantische Constraints (Status: Planned [ ])
- [ ] `OntologyManager` — JSON/YAML-Loader, `isA()` transitive Konzepthierarchie, `allowedEdgeTypes()` (Target: Q3 2026)
  - Affected: `include/graph/ontology_manager.h`, `src/graph/ontology_manager.cpp`
  - Runtime: immutable nach `build()`; thread-safe read-only; LRU-Cache für `isA()` (1 000 Einträge)
  - Error handling: unbekannte Konzept-IDs → unconstrained (WARN); Parsing-Fehler → `Status::Error`
  - Tests: `tests/graph/test_ontology_manager.cpp` (OM-01..OM-12)
  - Perf: Load ≤ 100 ms für 10 000 Konzepte; `isA()` ≤ 5 µs inkl. Cache-Lookup
- [ ] `PathConstraints::addSemanticConstraint()` — OWL-lite Pfad-Validierung, prune-first BFS (Target: Q4 2026)
  - Affected: `include/graph/path_constraints.h`, `src/graph/path_constraints.cpp`
  - Runtime: `validateSemanticPath()` iteriert über alle Pfadkanten; Violation-Liste zurückgeben
  - Tests: `tests/graph/test_path_constraints_semantic.cpp` (SC-01..SC-10)
  - Detail: `src/graph/FUTURE_ENHANCEMENTS.md` → Ontology-based Semantic Constraints

### Phase 7: Knowledge Graph Reasoning mit AI/ML + LoRA (Status: Planned [ ])
- [ ] `KnowledgeGraphReasoner` — Horn-Klausel-Forward-Chaining + `InferenceStore` + Erklärungsketten (Target: Q4 2026)
  - Affected: `include/graph/knowledge_graph_reasoner.h`, `src/graph/knowledge_graph_reasoner.cpp`
  - Runtime: `infer(subjectId, depth)` → `InferenceChain`; `explain(factId)` → Proof-Trace als Triple-Sequenz
  - Error handling: Regelwiderspruch → `ConflictError`; Zirkelbeweis → Depth-Limit mit `CycleDetected`
  - Tests: `tests/graph/test_knowledge_graph_reasoner.cpp` (KGR-01..KGR-20)
  - Perf: 1 M Kanten kalt ≤ 2 s; incremental CDC ≤ 50 ms
- [ ] Incremental CDC-Trigger: `KnowledgeGraphReasoner::onCDCEvent()` für Forward-Chaining bei Kanten-Inserts (Target: Q1 2027)
- [ ] LoRA-Adapter-Integration: `applyLoRAScore()` — Soft-Plausibility-Scoring via `MultiLoRAManager` für Mustererkennung (Target: Q2 2027)
  - Affected: Integration mit `src/llm/multi_lora_manager.cpp`
  - Runtime: Graph-Kontext → LoRA-Adapter-Selektion → Konfidenzwert (0.0–1.0) pro Inferenzkante
  - Guard: `THEMIS_ENABLE_LLM`; deterministischer Regel-Fallback wenn LoRA nicht geladen
  - Perf: LoRA-Scoring 1 000 Kanten ≤ 500 ms
- [ ] RAG-Integration: `KnowledgeGraphRetriever` nutzt `KnowledgeGraphReasoner` für Multi-Hop-Reasoning (Target: Q3 2027)
  - Affected: `include/rag/knowledge_graph_retriever.h`, `src/rag/knowledge_graph_retriever.cpp`
  - Detail: `src/graph/FUTURE_ENHANCEMENTS.md` → Knowledge Graph Reasoning with Ontology & ML/LoRA


- [I] Unit tests coverage > 80% (Issue: #1830)
- [x] Integration tests (query optimizer, constrained path finding, AQL integration)
- [x] Performance benchmarks (traversal latency vs graph size) (Issue: #1831)
- [I] Security audit (query injection via path constraints) (Issue: #1832)
- [x] Documentation complete
- [x] API stability guaranteed for graph query optimizer and path finder

## Known Issues & Limitations
- Adaptive plan selection using execution feedback is now active; `selectAlgorithm` uses learned EMA costs when confidence > 0, falling back to static depth heuristics otherwise
- Advanced cost model calibration from real execution feedback is implemented: `calibrateFromHistory()` re-seeds EMA models from batch history and computes cost accuracy metrics (`mean_estimated_ms`, `mean_absolute_error_ms`, `cost_ratio`) when `ExecutionStats::estimated_cost_ms` is populated (automatic in all execute* methods)
- Incremental query execution is BFS-only; DFS/Dijkstra/A* incremental modes are planned
- Incremental query execution is not thread-safe (same as the optimizer itself)
- Incremental query HTTP API (`POST /graph/query/incremental`, `DELETE /graph/query/incremental/:handle`, `POST /graph/changes`) is exposed via `GraphApiHandler`; edge mutations via `POST /graph/edge` and `DELETE /graph/edge/:id` automatically notify registered queries on success
- Subgraph isomorphism (pattern matching) is implemented via `executeSubgraphIsomorphism` (VF2-style backtracking)
- Cross-shard edge following (edges whose endpoints reside on different shards) requires caller-side coordination; the current distributed query model executes intra-shard queries in parallel and returns the globally cheapest result

## Breaking Changes
- Distributed graph query introduces shard-aware plan nodes (new plan format, backward-compatible with single-node)
- Subgraph isomorphism query syntax will extend AQL graph traversal syntax (additive)

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `LocalShardGraphExecutor` – Führt Graph-Traversals lokal auf einem Shard aus; getestet in test_graph_distributed
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.

