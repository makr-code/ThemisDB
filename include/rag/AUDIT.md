<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — RAG Module

- **Last Audit:** 2026-04-19
- **Auditor:** Copilot
- **Status:** ✅ Pass

## Summary

| Metric | Count |
|---|---|
| Header files audited | 56 |
| Exported symbol groups | 48 |
| Open stubs | 0 |
| Critical findings | 0 |

## Header Files Audited

| File | Exported Symbols | Notes |
|---|---|---|
| `ab_testing_framework.h` | `AbTestingFramework` | A/B pipeline testing |
| `adversarial_tester.h` | `AdversarialTester` | Robustness testing |
| `agentic_rag.h` | `AgenticRag` | Multi-step agentic RAG |
| `batch_evaluator.h` | `BatchEvaluator` | Batch evaluation |
| `bayesian_optimizer.h` | `BayesianOptimizer` | Hyperparameter optimization |
| `bias_detector.h` | `BiasDetector` | Output bias detection |
| `calibration_manager.h` | `CalibrationManager` | Score calibration |
| `citation_highlighter.h` | `CitationHighlighter` | Citation extraction |
| `claim_extractor.h` | `ClaimExtractor` | Atomic claim extraction |
| `coherence_evaluator.h` | `CoherenceEvaluator` | Coherence scoring |
| `completeness_evaluator.h` | `CompletenessEvaluator` | Completeness evaluation |
| `continuous_learning_client.h` | `ContinuousLearningClient` | Online learning client |
| `continuous_learning_orchestrator.h` | `ContinuousLearningOrchestrator` | Online model update orchestration |
| `cot_evaluator.h` | `CotEvaluator` | CoT reasoning evaluation |
| `distributed_rag_evaluator.h` | `DistributedRagEvaluator` | Distributed evaluation coordination |
| `document_splitter.h` | `DocumentSplitter` | Chunk-based splitting |
| `document_summarizer.h` | `DocumentSummarizer` | LLM summarization |
| `evaluation_cache.h` | `EvaluationCache` | Evaluation result cache |
| `evaluation_report_exporter.h` | `EvaluationReportExporter` | CSV/JSON export |
| `faithfulness_evaluator.h` | `FaithfulnessEvaluator` | RAGAS faithfulness |
| `geval_evaluator.h` | `GevalEvaluator` | G-Eval LLM judge |
| `hallucination_dashboard.h` | `HallucinationDashboard` | Rate visualization/alerting |
| `http_metrics_client.h` | `HttpMetricsClient` | Prometheus metrics push |
| `hybrid_retriever.h` | `HybridRetriever` | BM25 + vector + RRF |
| `judge_config.h` | `JudgeConfig` | Judge model configuration |
| `judge_ensemble.h` | `JudgeEnsemble` | Multi-judge ensemble |
| `knowledge_gap_detector.h` | `KnowledgeGapDetector` | Corpus gap detection |
| `knowledge_graph_retriever.h` | `KnowledgeGraphRetriever` | Graph-based retrieval |
| `learning_metrics.h` | `LearningMetrics` | Online learning metrics |
| `llm_integration.h` | `LlmIntegration` | LLM provider facade |
| `llm_judge_client.h` | `LlmJudgeClient` | LLM judge HTTP client |
| `llm_judge_integration.h` | `LlmJudgeIntegration` | End-to-end judge pipeline |
| `llm_meta_analyzer.h` | `LlmMetaAnalyzer` | Judge meta-analysis |
| `multimodal_rag.h` | `MultimodalRag` | Text + image + audio |
| `nli_faithfulness_verifier.h` | `NliFaithfulnessVerifier` | NLI faithfulness check |
| `onnx_model_loader.h` | `OnnxModelLoader` | ONNX runtime loading |
| `pairwise_comparator.h` | `PairwiseComparator` | Pairwise comparison |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | RAG injection detection |
| `prompt_templates.h` | `PromptTemplates` | Pre-built templates |
| `quality_control_factory.h` | `QualityControlFactory` | QC component factory |
| `quality_control_pipeline.h` | `QualityControlPipeline` | Quality gate pipeline |
| `rag_integration_helpers.h` | `RagIntegrationHelpers` | Pipeline wiring utilities |
| `rag_judge.h` | `RagJudge` | RAG-specific LLM judge |
| `relevance_evaluator.h` | `RelevanceEvaluator` | Retrieval relevance |
| `reranker.h` | `Reranker` | Cross-encoder reranking |
| `response_parser.h` | `ResponseParser` | LLM response parsing |
| `rubric_evaluator.h` | `RubricEvaluator` | Rubric-based scoring |
| `streaming_retriever.h` | `StreamingRetriever` | SSE streaming retrieval |
| `adaptive_retrieval.h` | `AdaptiveRetrieval` | Adaptive retrieval strategy selection |
| `explainability_reason_builder.h` | `ExplainabilityReasonBuilder` | RAG explainability and reasoning builder |
| `multi_hop_reasoner.h` | `MultiHopReasoner` | Multi-hop reasoning over knowledge graphs |
| `multi_step_rag.h` | `MultiStepRag` | Multi-step RAG pipeline |
| `rag_context_assembler.h` | `RagContextAssembler` | RAG context assembly from retrieved chunks |
| `rag_ingestion_bridge.h` | `RagIngestionBridge` | Bridge between RAG and ingestion pipeline |
| `replug_retriever.h` | `ReplugRetriever` | REPLUG-style retrieval-augmented LM |
| `rlaif_trainer.h` | `RlaifTrainer` | RLAIF (RL from AI Feedback) trainer |

## Findings

### Resolved
- `PromptInjectionDetector` in rag module is specialized for RAG retrieval context injections.
- `HybridRetriever` RRF fusion weights are configurable and documented.

### Open
- None.
