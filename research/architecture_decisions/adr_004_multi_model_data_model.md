# ADR-004: Native Multi-Model Data Model (Relational + Vector + Graph + Document)

**Status:** Accepted  
**Date:** 2022-07-01  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/aql/`, `src/graph/`, `src/index/`, `src/query/`  
**Related Research:** [Graph Databases (O'Reilly 2015)](../papers/graph_databases_oreilly_2015.md)

---

## Context

ThemisDB is positioned as a general-purpose database engine supporting four distinct data models in a single deployable binary:

1. **Relational (OLTP)** — row-oriented table storage with SQL-compatible query semantics.
2. **ANN Vector Search** — approximate nearest-neighbor lookup over high-dimensional float vectors (HNSW, ADR-001).
3. **Property Graph** — labeled vertices and edges with Cypher/AQL traversal queries.
4. **JSON Document** — schema-free document storage with nested field indexing.

At the time of this decision, the architecture team evaluated whether to build each model as a separate micro-service or to unify them in a single engine. The primary motivator for unification was **cross-model queries**: a ThemisDB query might ask "find documents whose author vertex is within graph distance 2 of user X, ranked by semantic vector similarity to query embedding Q" — a three-model join that is prohibitively expensive if each model lives in a separate service requiring network round-trips.

## Decision Drivers

- **Cross-model transactions:** A write that inserts a document, updates a graph edge, and adds a vector embedding must be atomic (single `WriteBatch` in RocksDB, ADR-002).
- **Cross-model joins in a single query plan:** The AQL query planner must produce a unified execution plan without inter-service calls on the critical path.
- **Unified MVCC:** All four model types must see the same consistent snapshot during a multi-model read.
- **Operational simplicity:** A single binary is easier to deploy, monitor, and scale than four separate services.
- **Index manager reuse:** B-tree, HNSW, and adjacency indexes are all registered with the same `IndexManager`; the query planner selects the right index type per predicate.
- **License / vendor independence:** Avoid runtime dependency on ArangoDB, MongoDB, or PostgreSQL binary distributions.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **Native multi-model single engine** | Atomic cross-model transactions; single MVCC; in-process cross-model joins; one binary; unified AQL | High implementation complexity; each model adds engine surface area; testing matrix multiplies |
| **Single relational engine + adapters** | SQL ecosystem familiarity; mature query optimizer | Vector search and graph traversal are second-class citizens; cross-model joins still require adapter calls with latency penalty |
| **ArangoDB-style multi-model** | Proven multi-model concept; AQL already designed for this | ArangoDB is a separate product; ThemisDB cannot embed it without license complications; no control over storage layer |
| **Separate micro-services per model** | Independent scaling; isolated failure domains; simpler per-service codebase | Cross-model joins require distributed query coordination (2+ network hops per join); no shared MVCC; complex deployment |
| **PostgreSQL + pgvector + AGE extensions** | Mature SQL; pgvector for ANN; AGE for graph | No control over storage internals; extensions share no common transaction path with the core; pgvector recall limitations; GPL-adjacent |

## Decision

**Chosen: Native multi-model in a single engine with unified AQL**

All four data models are implemented as first-class citizens within ThemisDB's single query engine:

- **Shared storage:** All model types store their data in RocksDB column families (ADR-002). Relational rows, document JSON blobs, graph adjacency lists, and HNSW graph state live under the same `WriteBatch` commit boundary.
- **Unified MVCC:** The transaction manager (`src/query/txn_manager.cpp`) issues a single `DB::GetSnapshot()` that covers all column families. All model reads during a transaction see the same consistent state.
- **Unified index manager:** `src/index/IndexManager` maintains a registry of `IIndex` implementations: `BTreeIndex`, `HnswVectorIndex`, `AdjacencyIndex`, `InvertedIndex`. The query planner selects indexes based on predicate type (range → B-tree; ANN → HNSW; graph hop → adjacency).
- **AQL (Algebraic Query Language):** `src/aql/` implements a unified parser, AST, logical planner, and physical planner. AQL expressions can reference relational predicates, vector distance functions (`NEAR(vec, query_vec, k)`), graph traversal (`TRAVERSE v IN 1..3`), and document paths (`doc.author.name`) within a single query.
- **Cross-model join operators:** The physical planner emits `VectorJoin`, `GraphHopJoin`, and `HashJoin` operators that the executor chains without IPC.

## Consequences

### Positive
- Three-model joins (e.g., graph + vector + document) execute in a single process with function-call overhead only — no network round-trips.
- Atomic cross-model writes eliminate partial-write consistency bugs that plague polyglot persistence architectures.
- Single deployment artifact simplifies container image management, monitoring (one metric namespace), and upgrade procedures.
- AQL's unified syntax reduces client-side query composition complexity compared to issuing separate API calls per model type.

### Negative / Trade-offs
- **Implementation surface:** Maintaining four model implementations in one engine is significantly more complex than specializing one model. *Mitigation: each model is implemented in its own sub-directory (`src/graph/`, `src/index/`, etc.) with isolated unit tests; shared utilities live in `src/core/`.*
- **Testing matrix:** Cross-model interaction bugs are harder to isolate. *Mitigation: integration tests in `tests/integration/multi_model/` exercise all 2- and 3-model join combinations.*
- **Query planner complexity:** A unified planner that reasons about vector distance, graph hops, and relational predicates simultaneously is substantially harder to optimize than a single-model planner. *Accepted because: this is ThemisDB's core differentiator.*
- **Graph and vector indexes not yet at parity with dedicated databases:** For pure-graph or pure-vector workloads, ArangoDB or Qdrant may outperform ThemisDB. *Accepted because: ThemisDB targets mixed workloads, not pure-model benchmarks.*

### Neutral
- Each model can be disabled at compile time (`-DTHEMIS_ENABLE_GRAPH=OFF`, etc.) for resource-constrained deployments.
- The `IDataModel` interface allows future model additions (e.g., time-series, geospatial) without changing the query planner core.

## Validation

- [x] Three-model join query (graph + vector + document) returns correct results in integration test
- [x] Atomic cross-model write verified with simulated crash injection
- [x] MVCC consistency verified across all four model types under concurrent load
- [x] AQL parser round-trip tested for all supported query forms
- [ ] Performance benchmark: cross-model join vs. equivalent polyglot (Postgres + Qdrant + Neo4j) pipeline (tracked: `benchmarks/multi_model/`)
- [ ] AQL specification document published to `docs/aql/`

## Follow-up Actions

- [ ] Publish AQL grammar (EBNF) to `docs/aql/grammar.ebnf` and reference documentation.
- [ ] Implement cost-based optimizer for cross-model join order selection (`src/query/optimizer/`).
- [ ] Add geospatial model as fifth data model using R-tree index (`src/geo/`).
- [ ] Document cross-model transaction boundaries in `src/query/README.md`.

## Related Decisions

- [ADR-001: HNSW over FAISS for ANN Vector Index](adr_001_hnsw_over_faiss_vector_index.md)
- [ADR-002: RocksDB as Primary Persistent Storage Backend](adr_002_rocksdb_storage_backend.md)
- [ADR-007: gRPC + Protobuf for Internal Service RPC](adr_007_grpc_for_internal_rpc.md)

---
**Last Updated:** 2026-04-06
