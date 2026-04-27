# Speculative RAG: Enhancing Retrieval Augmented Generation through Drafting

**Metadaten:**
- Author(en): Zilong Wang, Zifeng Wang, Long Le, Huaixiu Steven Zheng, Swaroop Mishra, Vincent Perot, Yuwei Zhang, Anush Mattapalli, Ankur Taly, Jingbo Shang, Chen-Yu Lee, Tomas Pfister
- Konferenz/Journal: Google DeepMind / arXiv preprint; ICLR 2025 submission
- Jahr: 2024
- Link: [arXiv:2407.08223](https://arxiv.org/abs/2407.08223)
- Zitierweise: `wang2024speculativerag`
- Tags: `speculative-rag`, `draft-answer`, `near-realtime`, `rag-latency`, `small-lm`, `lora-specialist`, `parallel-inference`
- ThemisDB-Versionen: v2.1.0+ (geplant in `src/rag/`, `src/llm/`)
- Status: [ ] Not Started (geplant Q1 2027 als Near-Realtime-RAG-Optimierung)

## 📋 Executive Summary

Speculative RAG überträgt das Konzept des **Speculative Decoding** auf den gesamten RAG-Zyklus: Ein kleines, spezialisiertes **Draft-LLM** (oder LoRA-Adapter auf einem 3B-7B-Modell) generiert schnell eine vorläufige Antwort aus einem Teilkontext; ein größeres **Verifier-LLM** überprüft und verfeinert diese Antwort. Da der teure Verifier nur auf bereits vorgefiltertem Kontext arbeitet, sinkt die End-to-End-Latenz um 40–60% bei gleichem oder besserem Qualitätsniveau. Für ThemisDB ist dies besonders relevant, da der Fachdomänen-LoRA-Adapter (z.B. Deutsches Verwaltungsrecht) als Draft-Spezialist fungieren kann, während das Basis-Modell als Verifier dient — ohne separates Small-LM.

Direkt relevant für `src/rag/streaming_retriever.cpp`, `src/rag/agentic_rag.cpp` und die geplante Near-Realtime-RAG-Pipeline in `src/rag/FUTURE_ENHANCEMENTS.md`.

## 🎯 Key Findings

- **Draft + Verify Pipeline**: Das Draft-LLM generiert eine schnelle Antwort aus den Top-K/2 Dokumenten (halber Kontext); das Verifier-LLM bewertet die Draft-Antwort und generiert bei niedriger Qualität eine vollständige Antwort aus dem vollen Kontext.
- **Latenz-Einsparung**: 40–60% geringere End-to-End-Latenz auf TriviaQA, MuSiQue, PubHealth-Benchmarks bei gleichem oder besserem Exact-Match (EM) Score gegenüber Standard-RAG.
- **Qualitätsgewinn durch Spezialisierung**: Draft-Spezialist, der nur auf relevanten Dokumenten-Cluster fokussiert, erzeugt präzisere Antworten als ein generalistisches LLM auf vollem Kontext — weniger Kontext-Rauschen.
- **Parallelisierbarkeit**: Draft-Generierung und Kontext-Scoring für Verifier können parallel ablaufen → zusätzliche Latenzreduktion in Multi-GPU-Setups.
- **LoRA-Spezialist als Draft-Modell**: Ein LoRA-Adapter auf einem Basis-Modell kann als "Draft-Spezialist" für eine Fachdomäne dienen; kein separates kleines Modell erforderlich.
- **Adaptive Skipping**: Wenn die Draft-Antwort eine hohe Konfidenz aufweist (Verifier-Score ≥ θ), wird der vollständige Verifier-Pass übersprungen — spart weiteren Overhead.
- **Skalierungsverhalten**: Bei Queries mit hoher Dokumenten-Homogenität (ähnliche Chunks) ist der Gewinn am größten; bei sehr heterogenen Chunks bleibt Standard-RAG kompetitiv.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] RAG Streaming → `src/rag/streaming_retriever.cpp` (Draft-Phase nutzt halben Kontext; First-Token-Latenz ≤ 100 ms)
- [ ] Agentic RAG → `src/rag/agentic_rag.cpp` (Draft-Verify als neue Iteration-Strategie im AgenticRAG-Loop)
- [ ] LLM LoRA → `src/llm/lora/` (Domain-LoRA-Adapter als Draft-Spezialist; Basis-Modell als Verifier)
- [ ] RAG Judge → `src/rag/rag_judge.cpp` (Verifier-Qualitätsbewertung für Adaptive-Skipping-Entscheidung)
- [ ] Continuous Learning → `src/rag/continuous_learning_orchestrator.cpp` (Loop-1: HNSW-Trefferliste informiert Draft-Kontext-Selektion)

### What Was Adopted?

1. **Draft-Verify-Pipeline in StreamingRetriever**: `StreamingRetriever::stream()` wird um eine zwei-phasige Pipeline erweitert:
   - Phase 1 (Draft): Top-K/2 Chunks → Domain-LoRA-Adapter → Draft-Antwort (inkl. Konfidenz-Score via `RAGJudge::evaluate()` im FAST-Modus)
   - Phase 2 (Verify): Wenn `confidence < θ` (default: 0.8): Vollständiger Kontext → Basis-Modell → Finale Antwort
2. **Adaptive-Skipping-Threshold θ**: Konfigurierbar in `rag_config.yaml` als `speculative_rag.draft_confidence_threshold: 0.8`. Senken auf 0.6 bei Latenz-kritischen Queries; erhöhen auf 0.9 bei Qualitäts-kritischen Queries.
3. **LoRA-als-Draft-Pattern**: `AdapterRegistry::getDomainAdapter(tenant_id, query_type)` gibt den spezialisierten Adapter zurück, der als Draft-Modell fungiert. Kein separates Small-LM in ThemisDB's Infrastruktur erforderlich.
4. **Parallel Draft + Score**: Draft-Generierung und HNSW-Scoring für verbleibende Dokumente (Verifier-Kontext-Aufbau) laufen via `std::async` parallel.

### How Was It Adapted?

| Speculative RAG Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Separates kleines Draft-LLM | Domain-LoRA-Adapter auf Basis-Modell als Draft | ThemisDB verwaltet bereits LoRA-Adapter-Infrastruktur; kein zweites Modell nötig |
| Top-K/2 Dokumente als Draft-Kontext | Top-K * `draft_context_ratio` (konfigurierbarer Faktor 0.3–0.7) | Optimaler Draft-Kontext-Anteil variiert je nach Domäne |
| Verifier: identisches großes LLM | Verifier: identisches Basis-Modell ohne LoRA-Adapter | LoRA-Adapter nur im Draft verwendet; Verifier = unbiasiertes Basis-Modell |
| Hard Threshold für Skip | Soft-Threshold mit `RAGJudge::evaluate(FAST)` Score | ThemisDB's Qualitäts-Framework bereits integriert; FAST-Modus ≤ 100 ms |
| TriviaQA / PubHealth Benchmarks | Deutsche Verwaltungsrecht-QA-Testset | Domänen-spezifische Eval-Sets für ThemisDB-Kerndomäne |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Status |
|--------|-------------|-----------------|--------|
| End-to-End-Latenz-Reduktion vs. Standard-RAG | 40–60% | 30–50% (konservativ) | ⏳ Planned Q1 2027 |
| First-Token-Latenz (Draft-Phase) | Nicht angegeben | ≤ 100 ms (Draft) | ⏳ Planned |
| EM-Score-Veränderung vs. Standard-RAG | ±0 bis +3 pp | ±0 bis +2 pp | ⏳ Planned |
| Skip-Rate (Verifier überspringen) | 30–50% der Queries | ≥ 25% | ⏳ Planned |
| VRAM-Overhead | Kein separates Modell | 0 zusätzlicher VRAM (LoRA-Draft) | ✅ By Design |

## ⚠️ Limitations & Open Questions

- Draft-Spezialist muss für die Fachdomäne ausreichend fine-getuned sein; generischer LoRA-Adapter erzeugt keine Qualitätsverbesserung.
  - ThemisDB-Lösung: Nur Domain-LoRA-Adapter (r=16, auf Verwaltungsrecht fine-getuned) werden als Draft-Spezialist eingesetzt; generische Adapter fallen auf Standard-RAG zurück.
- Verifier-LLM ist identisch mit Draft-Basis-Modell: wenn Basis-Modell halluziniert, nützt Verification wenig.
  - ThemisDB-Lösung: NLI-Faithfulness-Verifier (`nli_faithfulness_verifier.cpp`) als zusätzliche Verification-Stufe; unabhängig vom LLM.
- Adaptive Skipping setzt zuverlässige Konfidenz-Scores voraus; kalibrationsfehlerhafte Scores erhöhen Fehlerrate.
  - ThemisDB-Lösung: `CalibrationManager` in `src/rag/calibration_manager.cpp` kalibriert Judge-Scores regelmäßig; Skip-Threshold konservativ auf 0.85 gesetzt bis Kalibrierung validiert ist.
- Parallelisierung von Draft + Score erfordert ausreichend GPU-Speicher für simultanes Batching.
  - ThemisDB-Lösung: S-LoRA-Adapter-Paging stellt sicher, dass Draft-Adapter und Verifier-Modell im VRAM koexistieren können.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests: Draft-Verify-Pipeline (Mock-LLM mit kontrollierten Konfidenz-Scores)
- [ ] A/B-Test: Speculative RAG vs. Standard RAG auf deutschen Verwaltungsrecht-QA-Queries
- [ ] Benchmark: Latenz p50/p99, EM-Score, Skip-Rate
- [ ] Documentation updated (`src/rag/FUTURE_ENHANCEMENTS.md` Near-Realtime Section)
- [ ] Module README linked (`src/rag/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [S-LoRA — Sheng et al. (2023)](sheng_slora_concurrent_adapters_2023.md)
- [LoRA — Hu et al. (2022)](lora_low_rank_adaptation_2022.md)
- [HippoRAG — Gutierrez et al. (2024)](hipporag_gutierrez_2024.md)
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [Best Practice: S-LoRA Near-Realtime RAG Serving](../best_practices/slora_realtime_rag_serving.md)
- [Speculative Decoding — Chen et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#84-speculative-decoding)
- [PagedAttention — Kwon et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#85-pagedattention--continuous-batching)
- [`src/rag/streaming_retriever.cpp`](../../../src/rag/streaming_retriever.cpp)
- [`src/rag/agentic_rag.cpp`](../../../src/rag/agentic_rag.cpp)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-09-30
