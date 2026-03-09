# RAG Module

**Stand:** 9. März 2026
**Version:** 1.x
**Kategorie:** RAG (Retrieval-Augmented Generation)
**Validated:** 2026-03-09 (cea4844)
**Status:** current

---

## Übersicht

Das RAG-Modul implementiert die vollständige Retrieval-Augmented-Generation-Pipeline von ThemisDB:
Vektorselektion, hybride BM25+Vektor-Fusion, LLM-Integration, mehrdimensionale Antwortbewertung,
Halluzinationserkennung, Citation-Highlighting, kontinuierliches Lernen und agentenbasiertes Retrieval.

**Primäre Dokumentation:** [`src/rag/README.md`](../../../src/rag/README.md)
**Roadmap:** [`src/rag/ROADMAP.md`](../../../src/rag/ROADMAP.md)
**Fehlende Implementierungen:** [`missing-implementations.md`](missing-implementations.md)

---

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| RAGJudge | `rag_judge.h` | `rag_judge.cpp` | Haupt-Orchestrator für mehrdimensionale Bewertung |
| KnowledgeGapDetector | `knowledge_gap_detector.h` | `knowledge_gap_detector.cpp` | Dreistufige Wissenslückenerkennung |
| LLMIntegration | `llm_integration.h` | `llm_integration.cpp` | LLM-Brücke zum Inferenz-Engine |
| StreamingRetriever | `streaming_retriever.h` | `streaming_retriever.cpp` | Inkrementelles Kontextfenster-Füllen mit Token-Budget |
| HybridRetriever | `hybrid_retriever.h` | `hybrid_retriever.cpp` | BM25 + Vektor-Fusion mit konfigurierbaren RRF-Gewichten |
| KnowledgeGraphRetriever | `knowledge_graph_retriever.h` | `knowledge_graph_retriever.cpp` | Wissensgraph-augmentiertes Retrieval mit Entity-Linking |
| Reranker (CrossEncoder) | `reranker.h` | `reranker.cpp` | Cross-Encoder Re-Ranking mit heuristischem Scorer und ONNX |
| FaithfulnessEvaluator | `faithfulness_evaluator.h` | `faithfulness_evaluator.cpp` | Faktenprüfung gegen abgerufene Quellen |
| RelevanceEvaluator | `relevance_evaluator.h` | `relevance_evaluator.cpp` | Query-Antwort-Alignment-Bewertung |
| CompletenessEvaluator | `completeness_evaluator.h` | `completeness_evaluator.cpp` | Abdeckung aller Query-Aspekte |
| CoherenceEvaluator | `coherence_evaluator.h` | `coherence_evaluator.cpp` | Struktur- und Lesbarkeits-Score |
| BiasDetector | `bias_detector.h` | `bias_detector.cpp` | Ethik-Compliance-Prüfung |
| CoTEvaluator | `cot_evaluator.h` | `cot_evaluator.cpp` | Chain-of-Thought-Bewertung |
| GEvalEvaluator | `geval_evaluator.h` | `geval_evaluator.cpp` | G-Eval-Framework (Liu et al., 2023) |
| RubricEvaluator | `rubric_evaluator.h` | `rubric_evaluator.cpp` | Angepasste Rubrik-Bewertung |
| ClaimExtractor | `claim_extractor.h` | `claim_extractor.cpp` | Extraktion atomarer Behauptungen |
| NLIFaithfulnessVerifier | `nli_faithfulness_verifier.h` | `nli_faithfulness_verifier.cpp` | NLI-Entailment-basierte Claim-Verifikation |
| ResponseParser | `response_parser.h` | `response_parser.cpp` | LLM-Bewertungsantwort-Parsing |
| PromptTemplates | `prompt_templates.h` | `prompt_templates.cpp` | Template- und Few-Shot-Verwaltung |
| JudgeConfig | `judge_config.h` | `judge_config.cpp` | Konfigurations-Validierung |
| OnnxModelLoader | `onnx_model_loader.h` | `onnx_model_loader.cpp` | ONNX-Runtime-Modell-Loading (NLI/Cross-Encoder) |
| JudgeEnsemble | `judge_ensemble.h` | `judge_ensemble.cpp` | Multi-Judge-Abstimmungsstrategien |
| PairwiseComparator | `pairwise_comparator.h` | `pairwise_comparator.cpp` | Direkte Antwortvergleiche |
| LLMJudgeIntegration | `llm_judge_integration.h` | `llm_judge_integration.cpp` | Judge-Orchestrierung |
| LLMJudgeClient | `llm_judge_client.h` | `llm_judge_client.cpp` | HTTP-Client für externe LLM-Judge-Endpunkte |
| LLMMetaAnalyzer | `llm_meta_analyzer.h` | `llm_meta_analyzer.cpp` | Performance-Meta-Analyse |
| DocumentSplitter | `document_splitter.h` | `document_splitter.cpp` | Konfigurierbarer Chunk-Size, -Overlap, -Strategie |
| DocumentSummarizer | `document_summarizer.h` | `document_summarizer.cpp` | Mehrdokument-Zusammenfassung vor Kontext-Injektion |
| CitationHighlighter | `citation_highlighter.h` | `citation_highlighter.cpp` | Antwort-Sätze auf Quell-Chunks mappen |
| EvaluationReportExporter | `evaluation_report_exporter.h` | `evaluation_report_exporter.cpp` | Pro-Query-Evaluierungsbericht (JSON / HTML) |
| HallucinationDashboard | `hallucination_dashboard.h` | `hallucination_dashboard.cpp` | Gleitendes-Fenster-Halluzinationsrate-Tracking |
| HTTPMetricsClient | `http_metrics_client.h` | `http_metrics_client.cpp` | HTTP-Metriken-Export |
| ContinuousLearningOrchestrator | `continuous_learning_orchestrator.h` | `continuous_learning_orchestrator.cpp` | Adaptives Lernen aus Evaluierungsfeedback (LoRA-Retraining) |
| ContinuousLearningClient | `continuous_learning_client.h` | `continuous_learning_client.cpp` | Client-Interface für Lern-Orchestrator |
| BayesianOptimizer | `bayesian_optimizer.h` | `bayesian_optimizer.cpp` | Bayesianische Optimierung von Retrieval-Parametern |
| ABTestingFramework | `ab_testing_framework.h` | `ab_testing_framework.cpp` | Traffic-Splitting A/B-Tests mit statistischer Validierung |
| LearningMetrics | `learning_metrics.h` | `learning_metrics.cpp` | Gleitendes-Fenster-Metriken mit Mean/StdDev/Trend-Export |
| QualityControlPipeline | `quality_control_pipeline.h` | `quality_control_pipeline.cpp` | Automatisierte Qualitätskontroll-Pipeline |
| QualityControlFactory | `quality_control_factory.h` | `quality_control_factory.cpp` | Factory für Qualitätskontroll-Komponenten |
| AgenticRAG | `agentic_rag.h` | `agentic_rag.cpp` | Agentisches RAG mit iterativen Retrieval-Schleifen *(in Arbeit)* |
| MultimodalRAG | `multimodal_rag.h` | `multimodal_rag.cpp` | Multi-modales RAG (Bild + Text) *(in Arbeit)* |
| BatchEvaluator | `batch_evaluator.h` | *(header-only)* | Batch-Evaluierungs-Pipeline |
| CalibrationManager | `calibration_manager.h` | *(header-only)* | Evaluator-Score-Kalibrierung |
| EvaluationCache | `evaluation_cache.h` | *(header-only)* | Evaluierungs-Ergebnis-Caching |
| RAGIntegrationHelpers | `rag_integration_helpers.h` | *(header-only)* | Integration-Hilfsfunktionen |

**Gesamt:** 45 Header, 41 Source-Dateien + 4 Header-only-Komponenten

---

## Evaluierungsmodi

| Modus | Latenz | Anwendungsfall |
|-------|--------|----------------|
| Fast | ~100 ms | Hochdurchsatz-Produktion |
| Balanced | ~500 ms | Standard-RAG-Pipeline |
| Thorough | ~2 s | Forschung, Benchmarking |

---

## Wichtige Schnittstellen

```cpp
// Haupt-Evaluierungs-API
RAGJudge judge(config);
EvaluationResult result = judge.evaluate(query, answer, documents);

// Hybrides Retrieval
HybridRetriever retriever(config);
auto docs = retriever.retrieve(query, top_k);

// Kontinuierliches Lernen
ContinuousLearningOrchestrator orchestrator(config);
orchestrator.recordFeedback(query_id, user_rating);
auto params = orchestrator.getOptimizedRetrievalParams();
```

---

## Verwandte Dokumentation

- **Primäre Docs:** [`src/rag/README.md`](../../../src/rag/README.md)
- **Roadmap:** [`src/rag/ROADMAP.md`](../../../src/rag/ROADMAP.md)
- **Kontinuierliches Lernen:** [`KONTINUIERLICHES_LERNEN.md`](KONTINUIERLICHES_LERNEN.md)
- **Fehlende Implementierungen:** [`missing-implementations.md`](missing-implementations.md)
