<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# RAG Module — Architecture Guide

## Overview

The rag module implements the full Retrieval-Augmented Generation (RAG) pipeline for ThemisDB: hybrid retrieval (BM25 + vector), cross-encoder reranking, contextual compression, citation tracking, hallucination detection, multi-modal RAG, agentic RAG, streaming SSE, and a comprehensive evaluation framework (RAGAS-compatible, LLM-as-judge, NLI faithfulness, pairwise comparison, distributed evaluation).

## Design Principles

- **Hybrid retrieval** — `HybridRetriever` combines BM25 sparse and dense vector retrieval with configurable fusion weights.
- **Evaluation-first** — every RAG pipeline variant exposes evaluation hooks; `QualityControlPipeline` enforces quality gates.
- **Multi-modal** — `MultimodalRag` supports text, image, and audio context injection.
- **Streaming** — `StreamingRetriever` delivers results via SSE for low-latency partial responses.
- **Adversarial safety** — `AdversarialTester`, `PromptInjectionDetector`, and `BiasDetector` are embedded in the quality pipeline.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `ab_testing_framework.h` | `AbTestingFramework` | A/B testing for RAG pipeline variants |
| `adversarial_tester.h` | `AdversarialTester` | Adversarial robustness testing |
| `agentic_rag.h` | `AgenticRag` | Multi-step agentic RAG with tool use |
| `batch_evaluator.h` | `BatchEvaluator` | Batch evaluation of RAG outputs |
| `bayesian_optimizer.h` | `BayesianOptimizer` | Bayesian hyperparameter optimization for RAG |
| `bias_detector.h` | `BiasDetector` | Bias detection in RAG outputs |
| `calibration_manager.h` | `CalibrationManager` | Score calibration for evaluator outputs |
| `citation_highlighter.h` | `CitationHighlighter` | Citation extraction and highlighting |
| `claim_extractor.h` | `ClaimExtractor` | Atomic claim extraction from LLM responses |
| `coherence_evaluator.h` | `CoherenceEvaluator` | Response coherence scoring |
| `completeness_evaluator.h` | `CompletenessEvaluator` | Answer completeness evaluation |
| `continuous_learning_client.h` | `ContinuousLearningClient` | Client for online learning feedback loop |
| `continuous_learning_orchestrator.h` | `ContinuousLearningOrchestrator` | Orchestrates online model updates |
| `cot_evaluator.h` | `CotEvaluator` | Chain-of-thought reasoning evaluation |
| `distributed_rag_evaluator.h` | `DistributedRagEvaluator` | Distributed multi-node evaluation coordination |
| `document_splitter.h` | `DocumentSplitter` | Chunk-based document splitting |
| `document_summarizer.h` | `DocumentSummarizer` | LLM-based document summarization |
| `evaluation_cache.h` | `EvaluationCache` | Cache for evaluation results |
| `evaluation_report_exporter.h` | `EvaluationReportExporter` | CSV/JSON evaluation report export |
| `faithfulness_evaluator.h` | `FaithfulnessEvaluator` | RAGAS-style faithfulness scoring |
| `geval_evaluator.h` | `GevalEvaluator` | G-Eval LLM-as-judge scoring |
| `hallucination_dashboard.h` | `HallucinationDashboard` | Hallucination rate visualization and alerting |
| `http_metrics_client.h` | `HttpMetricsClient` | HTTP client for Prometheus metrics push |
| `hybrid_retriever.h` | `HybridRetriever` | BM25 + vector hybrid retrieval with RRF fusion |
| `judge_config.h` | `JudgeConfig` | LLM judge model and prompt configuration |
| `judge_ensemble.h` | `JudgeEnsemble` | Ensemble of multiple LLM judges |
| `knowledge_gap_detector.h` | `KnowledgeGapDetector` | Detects missing knowledge in retrieval corpus |
| `knowledge_graph_retriever.h` | `KnowledgeGraphRetriever` | Graph-based knowledge retrieval |
| `learning_metrics.h` | `LearningMetrics` | Online learning performance metrics |
| `llm_integration.h` | `LlmIntegration` | LLM provider integration facade |
| `llm_judge_client.h` | `LlmJudgeClient` | HTTP client for LLM-as-judge API calls |
| `llm_judge_integration.h` | `LlmJudgeIntegration` | End-to-end LLM judge pipeline |
| `llm_meta_analyzer.h` | `LlmMetaAnalyzer` | Meta-analysis of LLM judge outputs |
| `multimodal_rag.h` | `MultimodalRag` | Text + image + audio RAG |
| `nli_faithfulness_verifier.h` | `NliFaithfulnessVerifier` | NLI-based faithfulness verification |
| `onnx_model_loader.h` | `OnnxModelLoader` | ONNX runtime model loading for local inference |
| `pairwise_comparator.h` | `PairwiseComparator` | Pairwise response comparison |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | RAG-specific prompt injection detection |
| `prompt_templates.h` | `PromptTemplates` | Pre-built RAG prompt templates |
| `quality_control_factory.h` | `QualityControlFactory` | Factory for quality control pipeline components |
| `quality_control_pipeline.h` | `QualityControlPipeline` | End-to-end quality gate pipeline |
| `rag_integration_helpers.h` | `RagIntegrationHelpers` | Utility helpers for RAG pipeline wiring |
| `rag_judge.h` | `RagJudge` | RAG-specific LLM judge |
| `relevance_evaluator.h` | `RelevanceEvaluator` | Retrieval relevance scoring |
| `reranker.h` | `Reranker` | Cross-encoder reranking |
| `response_parser.h` | `ResponseParser` | LLM response parsing and structured extraction |
| `rubric_evaluator.h` | `RubricEvaluator` | Rubric-based scoring |
| `streaming_retriever.h` | `StreamingRetriever` | SSE-based streaming retrieval |
| *(planned)* `explainability_reason_builder.h` | `ExplainabilityReasonBuilder`, `CausalChain` | Natural-language causal chain for every autonomous RAG decision (IMPL-B9) |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `prompt_engineering` | `PromptTemplates`, `PromptInjectionDetector` | RAG prompt construction and injection safety |
| `observability` | `HttpMetricsClient`, `HallucinationDashboard` | Metrics and hallucination alerting |
| `llm` | `LlmIntegration`, `LlmJudgeClient` | LLM provider calls |
| `index` | `HybridRetriever`, `KnowledgeGraphRetriever` | Vector and graph index access |
| `network` | `StreamingRetriever` | SSE streaming to clients |
| *(planned)* `distributed_knowledge` | `FederatedRAGMerger` | Cross-shard RAG result merge for Layer 11C (DK-4) |

> **Paper 1+2 additions (IMPL-A2, IMPL-A3, IMPL-B9):**
> - `ContinuousLearningOrchestrator`: explicit `triggerLoop1…4()` + `FEDERATED_ROUND_START` event (IMPL-A2/A3)
> - `ExplainabilityReasonBuilder` (IMPL-B9): generates `CausalChain` for every autonomous decision; writes `DecisionRecord` to `AIDecisionAuditor`

## Implementation

Implementation in `../../src/rag/`.
