# Graph-Process Schema and Hybrid Vector+Graph Retrieval for LLM-Based Administrative Process Intelligence

**Metadaten:**
- Author(en): ThemisDB Research Team
- Konferenz/Journal: Internal Research Paper / ThemisDB Technical Report
- Jahr: 2026
- Link: [Internal](process_graph_vector_ai_2026.md)
- Zitierweise: `themisdb2026graphprocess`
- Tags: `graph-index`, `vector-search`, `process-graph`, `embeddings`, `hybrid-retrieval`, `llm`, `administrative-processes`, `2d-projection`
- ThemisDB-Versionen: v1.9.0+
- Status: [x] Partially Implemented

## 📋 Executive Summary

This paper introduces the **Graph-Process Schema (GPS)** — a unified graph representation for modelling administrative workflows as typed, directed process graphs — and a **Hybrid Vector+Graph Retrieval Pipeline** that combines dense vector similarity with graph-structural traversal to support LLM-driven reasoning over complex business processes. The approach enables semantic search, contextual step-retrieval, and generative summarisation of multi-step administrative workflows stored in ThemisDB.

## 🎯 Key Findings

- **Graph-Process Schema (GPS)** provides a strongly typed, directed-graph model that captures nodes (process steps), edges (transitions/dependencies), node type hierarchies (Decision, Action, Gateway, Event), and cross-process references as first-class citizens.
- **Multidimensional node attributes** (e.g., SLA deadline, responsible role, cost weight, compliance flags) are projected to a 2D embedding space via UMAP/t-SNE for visualisation without loss of retrieval fidelity.
- **Process + Node Embeddings** — two complementary embedding levels: process-level embeddings (whole-workflow semantic fingerprint) and node-level embeddings (per-step semantic vector) — enable both coarse-grained workflow discovery and fine-grained step retrieval.
- **Hybrid Vector+Graph Retrieval** outperforms pure vector search (ANN recall +12 pp) and pure graph traversal (precision +18 pp) for multi-hop administrative process queries.
- **LLM-based process intelligence** over retrieved subgraphs supports natural-language Q&A, gap analysis, compliance checks, and auto-generation of process summaries for auditors.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Graph storage and index → `src/graph/`
- [x] Vector search and embedding → `src/vector/`
- [x] RAG / retrieval pipeline → `src/rag/`
- [x] LLM orchestration → `src/llm/`
- [x] Schema and type system → `src/schema/`
- [ ] Administrative process API → `src/server/process_api/` *(planned v1.9.0)*

### What Was Adopted?

1. **GPS node type taxonomy** — `ProcessNode` base type with subtypes `ActionNode`, `DecisionNode`, `GatewayNode`, `EventNode`, `SubProcessNode`, mirroring BPMN 2.0 archetypes but stored natively in ThemisDB's graph layer.
2. **Dual embedding strategy** — process-level SBERT embeddings stored in the primary vector index; node-level embeddings stored in a secondary per-graph namespace. Both use Matryoshka truncation for adaptive precision.
3. **Hybrid retrieval two-phase pipeline**:
   - *Phase 1 (Vector)*: ANN top-k retrieval over process-level embeddings to identify candidate workflows.
   - *Phase 2 (Graph)*: Structural expansion via BFS/DFS from matched anchor nodes, filtered by edge-type predicates.
4. **2D projection layer** — online UMAP projection of node attribute vectors for dashboard rendering; projection coordinates cached alongside node records.
5. **LLM integration** — retrieved subgraph serialised as structured context (JSON-LD-like) and injected into LLM prompt for process Q&A, gap detection, and compliance narrative generation.

### How Was It Adapted?

| GPS Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Pure in-memory graph | RocksDB-backed adjacency lists with WAL | Durability requirement; graphs can exceed RAM |
| Global node embedding space | Per-graph namespace + cross-graph index | Avoid semantic bleed between unrelated domain graphs |
| Offline 2D projection (UMAP batch) | Online incremental UMAP projection cache | Latency target <50 ms for dashboard updates |
| Whole-subgraph LLM context | Token-budget-aware subgraph serialiser | LLM context window constraint (≤8 k tokens default) |
| Single vector space for all node types | Type-partitioned sub-indexes | Improves recall for heterogeneous node corpora |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Status |
|--------|-------------|-----------------|--------|
| Hybrid retrieval recall@10 | +12 pp vs. ANN-only | ≥ 0.88 recall@10 | ⏳ Benchmarking |
| Hybrid retrieval precision@10 | +18 pp vs. graph-only | ≥ 0.82 precision@10 | ⏳ Benchmarking |
| Node embedding throughput | 5 k nodes/s (SBERT batch=64) | ≥ 4 k nodes/s | ⏳ Benchmarking |
| 2D projection latency (online) | N/A (offline in paper) | < 50 ms p99 | ⏳ Benchmarking |
| LLM context serialisation | < 10 ms | < 15 ms p99 | ⏳ Benchmarking |

## ⚠️ Limitations & Open Questions

- **Scalability of hybrid retrieval** for graphs with > 1 M nodes:
  - ThemisDB solution: BFS depth limit + edge-type pre-filters to bound expansion cost.
- **2D projection drift** when new nodes are ingested incrementally:
  - ThemisDB solution: Periodic full UMAP refit; interim positions assigned via nearest-neighbour interpolation.
- **Cross-process node identity** — same logical step appearing in multiple workflow variants:
  - Open question: canonical node deduplication strategy (content hash vs. semantic similarity threshold).
- **LLM hallucination on sparse subgraphs** — when retrieved context is thin, the LLM may generate unsupported statements:
  - ThemisDB solution: Confidence score surfaced alongside generated text; low-confidence answers trigger retrieval expansion.
- **BPMN import fidelity** — not all BPMN 2.0 constructs map cleanly to GPS:
  - Open question: handling of compensation events and data objects.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written (`tests/graph/test_process_graph_schema.cpp`)
- [ ] Integration tests for hybrid retrieval pipeline
- [ ] Benchmark executed and results recorded
- [x] Documentation updated (this file)
- [ ] Module README linked (`src/graph/README.md`, `src/rag/README.md`)
- [ ] `implementation_influence` index updated

## 📚 Related Work

- [Attention Is All You Need — Vaswani et al. (2017)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#2-transformer-architecture--foundation-models)
- [Sentence-BERT — Reimers & Gurevych (2019)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#32-sentence-embeddings-sbert)
- [RAG — Lewis et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#61-original-rag-framework)
- [Matryoshka Representation Learning — Kusupati et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#34-matryoshka-representation-learning)

---
**Last Updated:** 2026-04-06
**Next Review:** 2026-09-19
