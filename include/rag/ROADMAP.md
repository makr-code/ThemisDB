---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/rag/ROADMAP.md -->

# RAG Module — Public Header Roadmap

**Module Path:** `include/rag/`
**Canonical implementation roadmap:** [`../../src/rag/ROADMAP.md`](../../src/rag/ROADMAP.md)

---

## Overview

Tracks public RAG API contract stability, header coverage, and future public entry points. Runtime retriever orchestration, evaluation scheduling, continuous-learning loop, and LLM judge routing work remain in:

→ [`../../src/rag/ROADMAP.md`](../../src/rag/ROADMAP.md)

---

## Current Status

All 104 top-level `include/rag/*.h` headers are present in this checkout. This
is a file inventory only; presence does not establish API stability, runtime
integration, or production readiness. See each header and its implementation
for current behavior.

---

## Completed ✅

- [x] Retrieval headers: `hybrid_retriever.h`, `adaptive_retrieval.h`, `flare_retrieval.h`, `replug_retriever.h`, `targ_retrieval.h`, `streaming_retriever.h`, `multi_hop_reasoner.h`, `multimodal_rag.h`, `knowledge_graph_retriever.h`, `ontology_aware_retriever.h`, `lora_enhanced_retriever.h`
- [x] Vectorisation and re-ranking: `dpr_vectorizer.h`, `reranker.h`, `vectorizer_interface.h`
- [x] Generation and context: `rag_context_assembler.h`, `document_splitter.h`, `document_summarizer.h`, `multi_step_rag.h`, `agentic_rag.h`, `prompt_templates.h`
- [x] Core evaluation: `faithfulness_evaluator.h`, `relevance_evaluator.h`, `coherence_evaluator.h`, `completeness_evaluator.h`, `cot_evaluator.h`, `nli_faithfulness_verifier.h`
- [x] Safety evaluation: `hallucination_dashboard.h`, `bias_detector.h`, `fairness_detector.h`, `adversarial_tester.h`, `prompt_injection_detector.h`, `claim_extractor.h`
- [x] G-EVAL and ensemble: `geval_evaluator.h`, `judge_ensemble.h`, `judge_config.h`, `rubric_evaluator.h`, `pairwise_comparator.h`, `delegate_evaluator.h`, `distributed_rag_evaluator.h`, `batch_evaluator.h`
- [x] Quality control: `quality_control_pipeline.h`, `quality_control_factory.h`, `evaluation_cache.h`, `evaluation_report_exporter.h`, `calibration_manager.h`, `explainability_reason_builder.h`, `citation_highlighter.h`
- [x] Learning: `continuous_learning_orchestrator.h`, `continuous_learning_client.h`, `learning_metrics.h`, `rlaif_trainer.h`, `bayesian_optimizer.h`, `ab_testing_framework.h`
- [x] LLM integration and ingestion: `llm_integration.h`, `llm_judge_client.h`, `llm_judge_integration.h`, `llm_meta_analyzer.h`, `rag_ingestion_bridge.h`, `rag_integration_helpers.h`, `rag_judge.h`, `response_parser.h`, `onnx_model_loader.h`, `http_metrics_client.h`, `knowledge_gap_detector.h`, `tensor_rag_pipeline.h`

---

## In Progress

- [ ] Align `IVectorizer` and `vectorizer_interface.h` with `include/llm/` embedding model contracts (Target: 2026-Q3)
- [ ] Document async timeout handling requirements for LLM judge client headers (Target: 2026-Q3)

---

## Planned

- [ ] `rag_policy.h` — per-pipeline retrieval resource and access-policy contract (Target: 2026-Q4)
- [ ] Add explicit stability annotations to experimental agentic and multimodal headers (Target: 2026-Q4)
- [ ] Expose benchmark precision/recall and latency targets for retrieval hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. RAG headers maintain backward compatibility within the active major line; evaluator API and retrieval-strategy changes require migration notes and changelog updates.
