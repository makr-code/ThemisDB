# From Local to Global: A Graph RAG Approach to Query-Focused Summarization

**Metadaten:**
- Author(en): Darren Edge, Ha Trinh, Newman Cheng, Joshua Bradley, Alex Chao, Apurva Mody, Steven Truitt, Jonathan Larson
- Konferenz/Journal: Microsoft Research Technical Report; arXiv preprint
- Jahr: 2024
- Link: [arXiv:2404.16130](https://arxiv.org/abs/2404.16130)
- Zitierweise: `edge2024graphrag`
- Tags: `graph-rag`, `community-detection`, `leiden-algorithm`, `knowledge-graph`, `llm`, `summarization`, `process-graph`
- ThemisDB-Versionen: v1.9.0+; planned implementation in `src/process/` and `src/rag/`
- Status: [ ] Not Started · [~] In Progress (planned Q3 2026)

## 📋 Executive Summary

GraphRAG introduces a graph-based Retrieval-Augmented Generation framework that operates at two levels: *local search* (entity-centric, traverses direct neighbors in a knowledge graph) and *global search* (community-centric, aggregates over hierarchical clusters produced by the Leiden algorithm). Unlike standard RAG, which retrieves flat document chunks, GraphRAG extracts entities and relationships from a corpus, builds a property graph, clusters it into communities, and pre-computes LLM summaries (Community Reports) per cluster. This architecture enables high-quality answers to global, abstractive questions such as "What are the main themes across all building permit processes?" that purely vector-based RAG cannot answer.

Directly referenced in `src/process/FUTURE_ENHANCEMENTS.md` (P4: Leiden-Community-Detection, Target Q3 2026).

## 🎯 Key Findings

- **Two-stage retrieval**: Local search navigates entity neighborhoods for specific fact lookup; Global search aggregates Community Reports for holistic summarization.
- **Leiden community detection**: Hierarchical clustering of graph nodes outperforms Louvain; modularity-based clusters with resolution parameter γ produce stable, semantically coherent communities.
- **Community Reports**: LLM-generated per-cluster summaries are pre-cached; global queries only read summaries (O(clusters), not O(nodes)) — dramatically reduces per-query LLM call count.
- **Entity extraction via LLM**: Relations and entities are extracted from source texts by the same LLM used for generation; enables fully automated graph construction.
- **Global query quality**: GraphRAG wins on abstractive reasoning (+15–25 pp on comprehensiveness) vs. baseline RAG; local queries are on par.
- **Token budget management**: Community Report selection respects configurable token budgets; lower-priority communities are dropped first.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Process module → `src/process/` (`ProcessGraphRag::extractSubgraph()` → replace BFS with Leiden+PPR)
- [x] RAG module → `src/rag/`
- [x] Graph module → `src/graph/` (community detection algorithms)
- [x] LLM module → `src/llm/` (Community Report generation)
- [ ] Analytics module → `src/analytics/` (planned: Leiden-based cluster analytics)

### What Was Adopted?

1. **Dual local/global retrieval pattern**: `ProcessGraphRag` adopts local (BFS from entity-matched nodes) and global (community report aggregation) retrieval modes, switchable via `ProcessRagConfig::retrieval_mode`.
2. **Leiden community detection**: `ProcessCommunityDetector` (planned `process_community_detector.cpp`) implements Leiden using the `src/graph/` module's graph layer; outputs `{cluster_id → [node_ids], community_report}`.
3. **Community Report pre-computation**: Background job pre-generates LLM summaries for each process community; summaries cached as `ProcessCommunityReport` objects in RocksDB.
4. **Token-budget-aware summary selection**: `ProcessGraphRag::buildAdminProcessingPrompt()` includes Community Reports up to `max_prompt_tokens`; excess communities dropped by `community_priority` score.

### How Was It Adapted?

| GraphRAG Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Generic knowledge graph | BPMN 2.0 / EPK / VCC-VPB process graphs | ThemisDB process graphs have typed nodes (Action, Decision, Gateway); entity extraction not needed — nodes are already structured |
| LLM entity extraction | FIM-Prozessbibliothek pre-built entities | German administrative processes have canonical entity taxonomy from FITKO |
| Global / local mode toggle | `ProcessRagConfig::retrieval_mode` enum | Process queries need deterministic mode selection, not automatic |
| Flat community reports | Hierarchical community reports (Leistung → Prozess → Schritt) | Matches the 3-layer FIM structure of German administrative processes |

### Performance Impact

| Metric | GraphRAG Claim | ThemisDB Target | Status |
|--------|----------------|-----------------|--------|
| Global query comprehensiveness vs. RAG | +15–25 pp | +10 pp (conservative) | ⏳ Planned Q3 2026 |
| Community detection runtime (500-node process graph) | ~2 s for 1M-node corpus | <100 ms for 500 nodes | ⏳ Planned |
| `ProcessGraphRag::retrieve()` latency | Not reported | <200 ms total (excl. LLM) | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- GraphRAG's LLM-based entity extraction is expensive; ThemisDB avoids this for structured process graphs.
  - ThemisDB solution: Skip extraction; use existing BPMN node metadata directly.
- Community Report pre-computation requires LLM calls at indexing time; adds index build latency.
  - ThemisDB solution: Async background generation; serve existing reports if not yet available.
- Leiden algorithm has non-deterministic output (random seed-dependent).
  - ThemisDB solution: Seed fixed per `model_id` for reproducibility; re-cluster on model update.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written
- [ ] Benchmark executed (query quality vs. BFS-only baseline)
- [ ] Documentation updated
- [ ] Module README linked (`src/process/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [HippoRAG — Gutierrez et al. (2024)](hipporag_gutierrez_2024.md)
- [LightRAG — Guo et al. (2024)](https://arxiv.org/abs/2410.05779)
- [Process Mining — van der Aalst (2012)](process_mining_van_der_aalst_2012.md)
- [`src/process/FUTURE_ENHANCEMENTS.md`](../../../src/process/FUTURE_ENHANCEMENTS.md)
- [`docs/de/process/STATE_OF_THE_ART.md`](../../de/process/STATE_OF_THE_ART.md)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
