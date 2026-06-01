> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/rag/ARCHITECTURE.md -->

# RAG Module — Public Header Architecture

**Module Path:** `include/rag/`
**Implementation:** `../../src/rag/`
**Canonical architecture doc:** [`../../src/rag/ARCHITECTURE.md`](../../src/rag/ARCHITECTURE.md)

---

## 1. Overview

`include/rag/` defines the **public Retrieval-Augmented Generation pipeline, evaluation, and quality-control API contract** for ThemisDB. The 65 headers cover retrieval (hybrid, FLARE, RePLUG, TARG, streaming, multi-hop, multimodal, knowledge-graph, ontology-aware, LoRA-enhanced), evaluation (faithfulness, relevance, coherence, completeness, hallucination, bias, fairness, calibration, NLI, G-EVAL, delegate, distributed, batch), quality control, RAG ingestion, continuous learning, LLM judge integration, explainability, and safety.

For runtime composition — retriever orchestration, evaluation pipeline scheduling, continuous-learning loop, and LLM judge routing — see:
→ [`../../src/rag/ARCHITECTURE.md`](../../src/rag/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Retrieval

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hybrid_retriever.h` | `HybridRetriever` | Dense + sparse hybrid retrieval |
| `adaptive_retrieval.h` | `AdaptiveRetrieval` | Query-adaptive retrieval strategy selection |
| `flare_retrieval.h` | `FLARERetrieval` | Forward-Looking Active REtrieval (FLARE) |
| `replug_retriever.h` | `RePlugRetriever` | Retrieval-augmented LM re-plugging |
| `targ_retrieval.h` | `TARGRetrieval` | Task-Adaptive Retrieval Generation |
| `streaming_retriever.h` | `StreamingRetriever` | Streaming chunk delivery during generation |
| `multi_hop_reasoner.h` | `MultiHopReasoner` | Multi-hop retrieval and reasoning chain |
| `multimodal_rag.h` | `MultimodalRAG` | Text + image multi-modal retrieval |
| `knowledge_graph_retriever.h` | `KnowledgeGraphRetriever` | Graph-structured knowledge retrieval |
| `ontology_aware_retriever.h` | `OntologyAwareRetriever` | Ontology-guided retrieval expansion |
| `lora_enhanced_retriever.h` | `LoRAEnhancedRetriever` | LoRA-tuned retrieval re-ranker |

### 2.2 Vectorisation and Re-Ranking

| Header | Public Type | Purpose |
|--------|------------|---------|
| `dpr_vectorizer.h` | `DPRVectorizer` | Dense Passage Retrieval vectorisation |
| `reranker.h` | `Reranker` | Cross-encoder re-ranking over retrieved passages |
| `vectorizer_interface.h` | `IVectorizer` | Backend-agnostic vectoriser abstraction |

### 2.3 Generation and Context

| Header | Public Type | Purpose |
|--------|------------|---------|
| `rag_context_assembler.h` | `RAGContextAssembler` | Passage ranking and context-window assembly |
| `document_splitter.h` | `DocumentSplitter` | Chunking strategies for ingested documents |
| `document_summarizer.h` | `DocumentSummarizer` | Extractive and abstractive summarisation |
| `multi_step_rag.h` | `MultiStepRAG` | Iterative multi-step RAG orchestration |
| `agentic_rag.h` | `AgenticRAG` | Agent-driven retrieval with tool invocation |
| `prompt_templates.h` | `RAGPromptTemplates` | Curated prompt templates for RAG generation |

### 2.4 Evaluation — Faithfulness, Relevance, Coherence

| Header | Public Type | Purpose |
|--------|------------|---------|
| `faithfulness_evaluator.h` | `FaithfulnessEvaluator` | Attribution-based faithfulness scoring |
| `relevance_evaluator.h` | `RelevanceEvaluator` | Query-passage relevance scoring |
| `coherence_evaluator.h` | `CoherenceEvaluator` | Response coherence and fluency scoring |
| `completeness_evaluator.h` | `CompletenessEvaluator` | Coverage of expected answer elements |
| `cot_evaluator.h` | `CoTEvaluator` | Chain-of-thought evaluation |
| `nli_faithfulness_verifier.h` | `NLIFaithfulnessVerifier` | NLI-based faithfulness verification |

### 2.5 Evaluation — Hallucination, Bias, and Safety

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hallucination_dashboard.h` | `HallucinationDashboard` | Aggregated hallucination tracking and reporting |
| `bias_detector.h` | `BiasDetector` | Bias detection in retrieval and generation |
| `fairness_detector.h` | `FairnessDetector` | Fairness-metric assessment |
| `adversarial_tester.h` | `AdversarialTester` | Adversarial robustness testing for retrievers |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | Prompt-injection attack detection |
| `claim_extractor.h` | `ClaimExtractor` | Factual claim extraction for downstream verification |

### 2.6 Evaluation — G-EVAL and Ensemble

| Header | Public Type | Purpose |
|--------|------------|---------|
| `geval_evaluator.h` | `GEvalEvaluator` | GPT-based generalised evaluation (G-EVAL) |
| `judge_ensemble.h` | `JudgeEnsemble` | Ensemble of LLM judges with aggregated scoring |
| `judge_config.h` | `JudgeConfig` | Configuration for LLM judge selection and weighting |
| `rubric_evaluator.h` | `RubricEvaluator` | Rubric-based structured evaluation |
| `pairwise_comparator.h` | `PairwiseComparator` | Pairwise response preference comparison |
| `delegate_evaluator.h` | `DelegateEvaluator` | Delegated sub-evaluator composition |
| `distributed_rag_evaluator.h` | `DistributedRAGEvaluator` | Distributed evaluation across shards |
| `batch_evaluator.h` | `BatchEvaluator` | Batch-mode evaluation pipeline |

### 2.7 Quality Control and Reporting

| Header | Public Type | Purpose |
|--------|------------|---------|
| `quality_control_pipeline.h` | `QualityControlPipeline` | Composable evaluation pipeline orchestration |
| `quality_control_factory.h` | `QualityControlFactory` | Factory for evaluation pipeline assembly |
| `evaluation_cache.h` | `EvaluationCache` | Result caching for evaluation pipelines |
| `evaluation_report_exporter.h` | `EvaluationReportExporter` | Evaluation result export (JSON/HTML/Prometheus) |
| `calibration_manager.h` | `CalibrationManager` | Score calibration and normalisation |
| `explainability_reason_builder.h` | `ExplainabilityReasonBuilder` | Structured explanation generation |
| `citation_highlighter.h` | `CitationHighlighter` | Source-citation annotation and highlighting |

### 2.8 Learning and Optimisation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `continuous_learning_orchestrator.h` | `ContinuousLearningOrchestrator` | Continuous learning loop for retrieval and generation |
| `continuous_learning_client.h` | `ContinuousLearningClient` | Client for registering signal providers |
| `learning_metrics.h` | `LearningMetrics` | Learning-loop metrics and convergence tracking |
| `rlaif_trainer.h` | `RLAIFTrainer` | Reinforcement-learning from AI feedback trainer |
| `bayesian_optimizer.h` | `BayesianOptimizer` | Bayesian hyperparameter optimisation |
| `ab_testing_framework.h` | `ABTestingFramework` | A/B testing for retrieval and generation strategies |

### 2.9 LLM Integration and Ingestion

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llm_integration.h` | `LLMIntegration` | LLM provider integration for RAG generation steps |
| `llm_judge_client.h` | `LLMJudgeClient` | Client for LLM-as-judge evaluation calls |
| `llm_judge_integration.h` | `LLMJudgeIntegration` | Full LLM judge pipeline integration |
| `llm_meta_analyzer.h` | `LLMMetaAnalyzer` | Meta-analysis of LLM responses across evaluators |
| `rag_ingestion_bridge.h` | `RAGIngestionBridge` | Bridge for ingesting documents into the RAG store |
| `rag_integration_helpers.h` | `RAGIntegrationHelpers` | Helper utilities for RAG pipeline composition |
| `rag_judge.h` | `RAGJudge` | Composite judge for overall RAG quality assessment |
| `response_parser.h` | `ResponseParser` | Structured response parsing from LLM output |
| `onnx_model_loader.h` | `ONNXModelLoader` | ONNX model loading for evaluation and re-ranking |
| `http_metrics_client.h` | `HTTPMetricsClient` | HTTP-based metrics export for evaluation telemetry |
| `knowledge_gap_detector.h` | `KnowledgeGapDetector` | Knowledge-gap detection in retrieval coverage |
| `tensor_rag_pipeline.h` | `TensorRAGPipeline` | Tensor-layer RAG pipeline integration |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::rag` | All retrieval, generation, evaluation, and quality-control types |
| `themis::rag::eval` | Evaluation-specific sub-types and evaluator interfaces |

---

## 4. Public Contract Notes

- Retriever headers define stable retrieval-strategy contracts; index and vector-store internals are opaque.
- `IVectorizer` provides the public extension point for custom embedding backends.
- Evaluation headers model deterministic scoring contracts; LLM judge calls are async and require explicit timeout handling.
- Continuous-learning headers expose signal-provider registration and loop control; training triggers remain internal.
- Quality-control headers define composable pipeline contracts; evaluation ordering and caching are configurable.
- Safety and injection-detection headers must fail closed on unsupported or adversarial inputs.
