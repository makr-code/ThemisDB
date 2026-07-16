# HippoRAG: Neurobiologically Inspired Long-Term Memory for Large Language Models

**Metadaten:**
- Author(en): Bernal Jiménez Gutierrez, Yiheng Shu, Yu Gu, Michihiro Yasunaga, Yu Su
- Konferenz/Journal: NeurIPS 2024; arXiv preprint
- Jahr: 2024
- Link: [arXiv:2405.14831](https://arxiv.org/abs/2405.14831)
- Zitierweise: `gutierrez2024hipporag`
- Tags: `graph-rag`, `personalized-pagerank`, `ppr`, `knowledge-graph`, `retrieval`, `long-term-memory`, `process-graph`
- ThemisDB-Versionen: v1.9.0+; planned implementation in `src/process/`
- Status: [~] In Progress (PPR scoring planned Q2 2026)

## 📋 Executive Summary

HippoRAG models long-term memory retrieval inspired by the human hippocampal-neocortical system. The neocortex (dense LLM knowledge) is complemented by the hippocampus (sparse associative graph index). Retrieval proceeds via Personalized PageRank (PPR) over a knowledge graph, seeded by query entities. PPR naturally propagates relevance across multi-hop paths, retrieving indirectly related knowledge that keyword or vector search would miss. This replaces BFS-based subgraph extraction in `ProcessGraphRag` with a principled probabilistic scoring method.

Directly referenced in `src/process/FUTURE_ENHANCEMENTS.md` (P2: PPR-basiertes GraphRAG Scoring, Target Q2 2026, Priority: High).

## 🎯 Key Findings

- **Personalized PageRank (PPR)**: Seed nodes from query entity matching; PPR score = relevance score for each graph node. Damping factor α = 0.85 (standard PageRank default).
- **Multi-hop retrieval**: A query like "Which documents were filed after the completeness check?" (3 hops) is answered correctly via PPR propagation; BFS with depth=2 would miss it.
- **Integration with embedding retrieval**: PPR seeds are chosen by semantic embedding similarity; combines vector search (for seed selection) with graph propagation (for multi-hop expansion).
- **Convergence**: Power iteration terminates when `||r_new − r_old||₁ < 1e-6`; typically <20 iterations for graphs with <1,000 nodes.
- **Outperforms naive RAG**: +10 pp F1 on multi-hop question-answering benchmarks (HotpotQA, MuSiQue) vs. naive dense retrieval.
- **Interpretable**: PPR scores provide a transparent ranking of nodes; auditors can trace *why* a node was included in the LLM context.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Process module → `src/process/` (`ProcessGraphRag::extractSubgraph()` → replace BFS with PPR)
- [x] Graph module → `src/graph/` (PPR computation over adjacency matrix)
- [x] RAG module → `src/rag/` (PPR-based context assembly)

### What Was Adopted?

1. **PPR scoring API** in `ProcessGraphRag`: `computePpr(graph, seed_node_ids, PprConfig)` returns `vector<pair<string, float>>` — top-k nodes by PPR score.
2. **PprConfig**: `damping=0.85`, `max_iterations=50`, `convergence_epsilon=1e-6`, `top_k_nodes=20` — directly from the paper's hyperparameters.
3. **Sparse adjacency matrix construction**: Build from `normalized.edges`; power iteration in-place for process graphs with ≤500 nodes.
4. **Backward compatibility**: BFS retained as fallback when process graph is a tree (PPR equivalent to BFS for trees).

### How Was It Adapted?

| HippoRAG Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Generic knowledge graph | BPMN 2.0 process graphs | Process nodes are typed; PPR adapts to edge-type-weighted adjacency |
| OpenIE entity extraction for seeds | HNSW ANN over pre-computed node embeddings | Avoid LLM calls for seed selection; faster for structured process data |
| Undirected graph | Directed process graph | Process flows are directed (preceding → following step); PPR uses directed random walk |
| Dense adjacency matrix | Sparse COO format | Process graphs are sparse (avg. degree ≈ 2.5); COO more memory-efficient |

### Performance Impact

| Metric | HippoRAG Claim | ThemisDB Target | Status |
|--------|----------------|-----------------|--------|
| F1 on multi-hop QA vs. naive RAG | +10 pp | +7 pp (conservative) | ⏳ Planned Q2 2026 |
| PPR computation (500-node graph) | Not reported | <20 ms | ⏳ Planned |
| Seed selection via HNSW | Not reported | <5 ms (p99) | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- PPR on very large graphs (>10,000 nodes) may be slow with dense adjacency.
  - ThemisDB solution: Apply PPR only within relevant subgraph (2-hop neighborhood of seeds) to bound computation.
- Directed graphs require careful handling (teleport probability distribution over reachable nodes only).
  - ThemisDB solution: Use out-degree normalized transition matrix; add self-loops for sink nodes.
- PPR is sensitive to graph topology; poorly connected graphs have flat score distributions.
  - ThemisDB solution: Fall back to BFS when PPR score variance < threshold.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written (PPR vs. BFS recall on 3-hop test graphs)
- [ ] Benchmark executed
- [ ] Documentation updated
- [ ] Module README linked (`src/process/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [LightRAG — Guo et al. (2024)](https://arxiv.org/abs/2410.05779)
- [RAG — Lewis et al. (2020)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#61-original-rag-framework)
- [`src/process/FUTURE_ENHANCEMENTS.md`](../../../src/process/FUTURE_ENHANCEMENTS.md) (P2)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
