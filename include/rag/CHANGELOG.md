<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — RAG Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/rag/CHANGELOG.md`.

## [1.5.0] — 2026-01

### Added
- `multimodal_rag.h` — `MultimodalRag` text + image + audio context injection.
- `hybrid_retriever.h` — `HybridRetriever` BM25 + vector with RRF fusion.
- `reranker.h` — `Reranker` cross-encoder reranking.
- `citation_highlighter.h` — `CitationHighlighter` citation extraction and highlighting.
- `streaming_retriever.h` — `StreamingRetriever` SSE-based streaming retrieval.
- RAG evaluation (RAGAS-compatible): `faithfulness_evaluator.h`, `relevance_evaluator.h`, `coherence_evaluator.h`, `completeness_evaluator.h`.
- `distributed_rag_evaluator.h` — `DistributedRagEvaluator` multi-node evaluation.
- `hallucination_dashboard.h` — `HallucinationDashboard` rate visualization.
- `agentic_rag.h` — `AgenticRag` multi-step agentic RAG with tool use.

## [1.3.0] — 2025-09

### Added
- `ab_testing_framework.h` — `AbTestingFramework` pipeline A/B testing.
- `bayesian_optimizer.h` — `BayesianOptimizer` hyperparameter optimization.
- `judge_ensemble.h` — `JudgeEnsemble` multi-judge ensemble.
- `geval_evaluator.h` — `GevalEvaluator` G-Eval LLM-as-judge.
- `pairwise_comparator.h` — `PairwiseComparator` pairwise response comparison.
- `rubric_evaluator.h` — `RubricEvaluator` rubric-based scoring.
- `nli_faithfulness_verifier.h` — `NliFaithfulnessVerifier` NLI verification.
- `adversarial_tester.h` — `AdversarialTester` robustness testing.
- `bias_detector.h` — `BiasDetector` output bias detection.
- `calibration_manager.h` — `CalibrationManager` score calibration.
- `evaluation_cache.h` / `evaluation_report_exporter.h` — Evaluation infrastructure.

## [1.1.0] — 2025-06

### Added
- `continuous_learning_client.h` / `continuous_learning_orchestrator.h` — Online learning feedback loop.
- `knowledge_gap_detector.h` — `KnowledgeGapDetector` corpus gap detection.
- `knowledge_graph_retriever.h` — `KnowledgeGraphRetriever` graph-based retrieval.
- `llm_judge_client.h` / `llm_judge_integration.h` / `llm_meta_analyzer.h` — LLM judge pipeline.
- `onnx_model_loader.h` — `OnnxModelLoader` local ONNX inference.
- `cot_evaluator.h` — `CotEvaluator` chain-of-thought evaluation.
- `batch_evaluator.h` — `BatchEvaluator` batch evaluation.
- `learning_metrics.h` — `LearningMetrics` online learning metrics.

## [1.0.0] — 2025-01

### Added
- Core RAG pipeline: `document_splitter.h`, `document_summarizer.h`, `llm_integration.h`.
- `rag_judge.h` — `RagJudge` RAG-specific LLM judge.
- `quality_control_pipeline.h` / `quality_control_factory.h` — Quality gate pipeline.
- `prompt_injection_detector.h` — RAG-specific injection detection.
- `prompt_templates.h` — Pre-built RAG prompt templates.
- `claim_extractor.h` / `response_parser.h` — Response analysis.
- `rag_integration_helpers.h` — Pipeline wiring utilities.
- `judge_config.h` — Judge model configuration.
- `http_metrics_client.h` — Prometheus metrics push client.
