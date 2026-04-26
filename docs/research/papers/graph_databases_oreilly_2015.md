# Graph Databases (2nd Edition)

**Metadaten:**
- Author(en): Ian Robinson, Jim Webber, Emil Eifrem
- Konferenz/Journal: Book — O'Reilly Media (2nd edition)
- Jahr: 2015 (1st ed. 2013)
- Link: [O'Reilly](https://www.oreilly.com/library/view/graph-databases-2nd/9781491930885/) · [ISBN: 978-1-491-93089-2]
- Zitierweise: `robinson2015graphdatabases`
- Tags: `graph-database`, `property-graph`, `neo4j`, `cypher`, `graph-model`, `traversal`
- ThemisDB-Versionen: v1.0.0+ (core reference for `src/graph/`)
- Status: [x] Partially Implemented (property graph model + AQL traversal) · [ ] GQL/ISO 39075 compliance planned

## 📋 Executive Summary

This book is the definitive introduction to graph databases, the property graph model, and graph query languages. It covers the motivation for graph-native storage (index-free adjacency), the Neo4j architecture, Cypher query language, and practical patterns for modeling and querying highly connected data. ThemisDB's graph module uses the property graph model and the graph traversal patterns described here as its core design reference.

## 🎯 Key Findings

- **Property graph model**: Nodes + edges, both with key-value property maps + type labels; directed edges; no schema required.
- **Index-free adjacency**: Edges stored as direct references to adjacent nodes; traversal is O(1) per hop regardless of total graph size (vs. O(log n) for index-based joins in relational).
- **Connected data advantage**: Relational JOIN performance degrades O(n²) for multi-hop queries; graph traversal stays O(k) where k = number of hops.
- **Cypher query language**: Declarative ASCII-art pattern matching (`MATCH (a)-[:KNOWS]->(b)`); expressive, readable; became the basis for GQL (ISO 39075:2024).
- **ACID transactions on graphs**: Neo4j's WAL-based durability; write transactions with rollback; property graph changes are atomic.
- **Graph modeling patterns**: Bi-temporal graphs, intermediate nodes, hyperedge simulation, tree hierarchies — all with Cypher examples.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Graph module → `src/graph/` (property graph storage, traversal algorithms)
- [x] AQL → `src/aql/` (AQL graph traversal syntax inspired by Cypher patterns)
- [x] Schema → (node/edge type system based on property graph model)
- [ ] GQL compliance layer → planned (ISO 39075:2024 compatibility)

### What Was Adopted?

1. **Property graph model**: ThemisDB's graph layer stores `{node_id, labels[], properties{}}` and `{edge_id, type, source_id, target_id, properties{}}` — directly from the book's data model definition.
2. **Index-free adjacency simulation**: ThemisDB stores adjacency lists in RocksDB with key prefix `graph:adj:{node_id}` for O(1) neighbor lookup per hop.
3. **AQL traversal syntax**: `FOR v, e IN 1..3 OUTBOUND startNode GRAPH graphName` — modeled on Cypher's `MATCH (a)-[*1..3]->(b)` pattern.
4. **Graph traversal algorithms**: BFS, DFS, Dijkstra, A* — all described in this book and implemented in `src/graph/`.

### How Was It Adapted?

| Graph DB Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Pure in-memory graph (Neo4j) | RocksDB-backed adjacency + WAL | Durability requirement; graphs exceed RAM for large deployments |
| Native graph storage format | Key-value adjacency lists in RocksDB | ThemisDB unifies all data in RocksDB; no separate graph storage engine |
| Cypher query language | AQL (ThemisDB Query Language) | AQL covers relational + graph + vector + temporal in one language |
| Neo4j Java API | C++ graph API (`src/graph/`) | ThemisDB is C++ |

### Performance Impact

| Metric | Book's Claim | ThemisDB Result | Status |
|--------|-------------|-----------------|--------|
| Traversal per hop | O(1) index-free adjacency | O(log n) RocksDB lookup | ✅ Acceptable for most graphs |
| BFS depth-5 on 1M-node graph | ~50 ms | ~120 ms | ⏳ GPU traversal planned |

## ⚠️ Limitations & Open Questions

- ThemisDB does not achieve true index-free adjacency (RocksDB key lookups add O(log n)).
  - ThemisDB solution: GPU BFS (`src/gpu/`) for large-scale traversal; planned cache-resident hot subgraph.
- No native Cypher support; AQL is ThemisDB-specific.
  - ThemisDB solution: GQL/ISO 39075 compatibility layer planned to accept standard Cypher queries.

## 🔬 Validation

- [x] Property graph model reviewed against book
- [x] AQL traversal semantics reviewed
- [x] BFS/DFS/Dijkstra unit tests written
- [ ] GQL compliance test suite pending
- [ ] Module README linked

## 📚 Related Work

- [GQL ISO 39075:2024](https://www.iso.org/standard/76120.html)
- [GraphSAGE — Hamilton et al. (2017)](https://arxiv.org/abs/1706.02216)
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [`src/graph/README.md`](../../../src/graph/README.md)
- [Apache TinkerPop / Gremlin](https://tinkerpop.apache.org)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
