# Translating Embeddings for Modeling Multi-relational Data (TransE)

**Metadaten:**
- Author(en): Antoine Bordes, Nicolas Usunier, Alberto Garcia-Duran, Jason Weston, Oksana Yakhnenko
- Konferenz/Journal: Advances in Neural Information Processing Systems 26 (NeurIPS 2013)
- Jahr: 2013
- Link: [NeurIPS Proceedings](https://proceedings.neurips.cc/paper/2013/hash/1cecc7a77928ca8133fa24680a88d2f9-Abstract.html) · [Semantic Scholar](https://api.semanticscholar.org/CorpusID:14448775)
- Zitierweise: `bordes2013transe`
- Tags: `knowledge-graph`, `embeddings`, `link-prediction`, `kg-completion`, `translational-model`, `multi-relational`, `entity-resolution`
- ThemisDB-Versionen: v2.1.0+ (planned — `KnowledgeGraphReasoner`, LoRA-Plausibility-Scoring)
- Status: [ ] Not Started | [ ] Partially Implemented | [ ] Fully Implemented  
  *(Planned Q4 2026; existierende KGE-Research: `docs/research/KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md`)*

## 📋 Executive Summary

TransE ist das grundlegende Translational-Embedding-Modell für Knowledge Graphs. Entitäten (`h`, `t`) und Relationen (`r`) werden in einem niedrig-dimensionalen Vektorraum eingebettet, sodass `h + r ≈ t` für wahre Tripel `(h, r, t)`. Training minimiert Margin-basierenden Ranking-Loss mit negativem Sampling. TransE erzielt State-of-the-Art Link-Prediction auf FB15k und WN18 und ist der Ausgangspunkt für alle nachfolgenden KGE-Modelle (RotatE, QuatE, ComplEx, DistMult).

Für ThemisDB ist TransE relevant als **Soft-Plausibility-Scoring**-Methode im `KnowledgeGraphReasoner`: abgeleitete Inferenz-Kanten können via TransE-Score (oder LoRA-Adapter als neuronale Alternative) bewertet werden. Die Embedding-Nähe `||h + r - t||` liefert einen Konfidenzwert für Inferenz-Tripel.

## 🎯 Key Findings

- **Scoring-Funktion**: `f(h, r, t) = -||h + r - t||` — negierte L2-Distanz; höher = plausiblere Relation.
- **Link Prediction**: Für fehlende `t` bei gegebenen `(h, r)`: `argmax_t f(h, r, t)` via ANN-Suche.
- **Rank-basierte Metriken**: Mean Reciprocal Rank (MRR), Hits@K (K=1,3,10) als Standardmetriken; TransE: Hits@10 = 47.1% auf FB15k.
- **Skalierbarkeit**: Embedding-Dimension d=50–200; Training auf CPU für KGs mit ≤1M Tripeln möglich.
- **Limitations**: TransE kann keine symmetrischen Relationen (`spouse`) und 1-N/N-1-Relationen korrekt modellieren.
- **Nachfolger**: RotatE (2019) — Rotation in komplexem Raum; QuatE (2019) — Quaternionen; DistMult (2015) — bilinear; alle kompatibel mit `h + r ≈ t` Scoring-API.
- **Multi-Hop**: `h + r₁ + r₂ + ... + rₙ ≈ t` für Pfad-Komposition (Path-Embedding).

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] Graph module → `src/graph/` — `KnowledgeGraphReasoner::applyLoRAScore()` — TransE-Score als Soft-Plausibility-Baseline; LoRA-Adapter als lernbare Alternative
- [ ] Graph module → `src/graph/` — `KnowledgeGraphRetriever` Link-Prediction für Kanten-Vorschläge in `ScheduledGraphEdgeRefreshEngine`
- [ ] RAG module → `src/rag/` — `OntologyAwareRetriever` Entitäts-Ähnlichkeits-Suche via Embedding-Raum
- [ ] Analytics module → `src/analytics/` — `LoRAPatternClassifier` als neuronales Äquivalent zu KGE-Scoring

### What Was Adopted?

1. **Scoring-API-Konzept**: `KnowledgeGraphReasoner::applyLoRAScore(chain, adapter_id)` gibt Konfidenzwert 0.0–1.0 pro Inferenz-Kante zurück — konzeptuell analog zu `f(h, r, t)` von TransE.
2. **Link-Prediction-Basis**: `ScheduledGraphEdgeRefreshEngine` (existierend) nutzt Vektor-Cosine-Ähnlichkeit für Kanten-Kandidaten — TransE-Pfad-Komposition ist geplante Erweiterung (v2.2.0).
3. **Embedding-Dimensionen**: Graph-Node-Embeddings im Vector-Index (`src/vector/`) verwenden d=128–1024 (konsistent mit TransE-Empfehlung d=50–200 als Untergrenze).
4. **Negative Sampling**: `IncrementalLoRATrainer` verwendet Contrastive-Loss mit negativen Samples — strukturell analog zu TransE-Margin-Loss.
5. **MRR/Hits@K**: Geplante Evaluierungs-Metriken für `KnowledgeGraphReasoner`-Inferenzqualität (KGR-19..20).

### How Was It Adapted?

| TransE Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Trainierbare statische Embeddings | LoRA-Adapter-Scoring (dynamisch) | LoRA kann domänenspezifisch fine-getuned werden; kein separates KGE-Training nötig |
| `||h + r - t||` Distanz | LoRA-generierter Konfidenzwert 0.0–1.0 | LLM-basiertes Scoring ausdrucksstärker als lineare Translation |
| Offline-Training (Batch) | Online-incremental via `IncrementalLoRATrainer` | Produktionsbetrieb erfordert Online-Adaption |
| Nur Link-Prediction | Link-Prediction + Pfad-Plausibilität + Kanten-Typ-Validierung | ThemisDB-Anwendungsfall breiter als reines KG-Completion |
| FB15k / WN18 Benchmarks | ThemisDB domänenspezifische Benchmark-Tripel | Kein generisches KG; juristisch/prozessual strukturiert |

### Performance Impact

| Metric | TransE (FB15k) | ThemisDB Ziel | Status |
|--------|----------------|---------------|--------|
| Hits@10 (Link Prediction) | 47.1% | ≥ 40% (interne Benchmark-Tripel) | ⏳ Planned Q4 2026 |
| Embedding-Inference-Latenz | O(d) | ≤ 1 ms/Tripel | ⏳ Planned |
| LoRA-Scoring 1k Kanten | — | ≤ 500 ms | ⏳ Planned Q2 2027 |

## ⚠️ Limitations & Open Questions

- **Symmetrische Relationen**: TransE kann `spouse(A, B)` und `spouse(B, A)` nicht gleichzeitig korrekt repräsentieren.
  - ThemisDB Lösung: RotatE-Erweiterung (v2.2.0); interim: symmetrische Kanten werden als bidirektionale Einzel-Kanten gespeichert.
- **1-N Relationen**: `mother_of(A, B)`, `mother_of(A, C)` → `h + r` muss `B` und `C` gleichzeitig approximieren.
  - ThemisDB Lösung: LoRA-Scoring gibt Konfidenz pro Kandidat; kein Embeding-Vektor muss eindeutig sein.
- **Cold Start**: Neue Entitäten ohne Training-Tripel haben keine Embeddings.
  - ThemisDB Lösung: `OntologyManager` liefert Klassen-basiertes Fallback; LoRA-Adapter generiert Kontext-basiertes Scoring.
- **Skalierbarkeit auf ≥10M Tripel**: Naives TransE-Training wird prohibitiv teuer.
  - ThemisDB Lösung: ANN-Index (HNSW) für Embedding-Suche; Training nur auf domain-relevanten Teilgraphen.

## 🔬 Validation

- [ ] Konzept validiert gegen ThemisDB-Anwendungsfall (juristische KG-Tripel)
- [ ] LoRA-Score-Äquivalenz zu TransE-Baseline auf Beispiel-Tripeln geprüft
- [ ] Benchmark-Datensatz für Hits@K-Evaluierung definiert (KGR-19..20)
- [ ] Module README verlinkt (`src/graph/README.md`)
- [ ] implementation_influence index aktualisiert

## 📚 Related Work

- [OWL 2 / Description Logic Handbook](owl2_description_logics_2012.md) — Symbolische Wissensrepräsentation (Komplement zu TransE)
- [LoRA — Hu et al. (2022)](lora_low_rank_adaptation_2022.md) — LoRA-Adapter als lernbare KGE-Alternative
- [HippoRAG — Gutierrez et al. (2024)](hipporag_gutierrez_2024.md) — PPR-basiertes Multi-Hop-Retrieval
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md) — LLM-basiertes KG-Reasoning
- [`docs/research/KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md`](../KNOWLEDGE_GRAPH_EMBEDDINGS_RESEARCH.md) — RotatE, QuatE, ComplEx
- [`src/graph/FUTURE_ENHANCEMENTS.md`](../../../src/graph/FUTURE_ENHANCEMENTS.md) — KG Reasoning + LoRA Scoring
- [Best Practice: Multi-LoRA Adapter Routing](../best_practices/multi_lora_adapter_routing.md)

## 🔬 Weiterführende Quellen — KGE-Nachfolger

| Modell | Quelle | Verbesserung gegenüber TransE |
|--------|--------|-------------------------------|
| RotatE | Sun et al. (2019) — NeurIPS | Rotation in ℂ; symmetrische + antisymmetrische + inverse Relationen |
| QuatE | Zhang et al. (2019) — NeurIPS | Quaternionen; kompositionelle Relationen |
| DistMult | Yang et al. (2015) — ICLR | Bilinear; symmetrische Relationen; skalierbar |
| ComplEx | Trouillon et al. (2016) — ICML | Komplexe Embeddings; antisymmetrische Relationen |
| ConvE | Dettmers et al. (2018) — AAAI | CNN-basiertes Scoring; 1-N Relationen |
| TuckER | Balazevic et al. (2019) — EMNLP | Tucker-Zerlegung; verallgemeinert DistMult/ComplEx |
| KGBERT | Yao et al. (2019) — arXiv | BERT-basiertes KGE; kontextuelle Embeddings |

---
**Last Updated:** 2026-04-22  
**Next Review:** 2026-10-01
