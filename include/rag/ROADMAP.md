> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/rag/ROADMAP.md -->

# RAG Module — Public Header Roadmap

**Module Path:** `include/rag/`
**Canonical implementation roadmap:** [`../../src/rag/ROADMAP.md`](../../src/rag/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/rag/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/rag/ROADMAP.md`](../../src/rag/ROADMAP.md)

---

## Current Status

production RAG runtime with hybrid retrieval, context assembly, evaluation suite, quality control, continuous learning, and safety controls. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `hybrid_retriever.h` — retrieval and ranking contract
- [x] `reranker.h` — retrieval and ranking contract
- [x] `replug_retriever.h` — retrieval and ranking contract
- [x] `adaptive_retrieval.h` — retrieval and ranking contract
- [x] `flare_retrieval.h` — retrieval and ranking contract
- [x] `targ_retrieval.h` — retrieval and ranking contract
- [x] `lora_enhanced_retriever.h` — retrieval and ranking contract
- [x] `knowledge_graph_retriever.h` — retrieval and ranking contract
- [x] `ontology_aware_retriever.h` — retrieval and ranking contract
- [x] `dpr_vectorizer.h` — retrieval and ranking contract
- [x] `vectorizer_interface.h` — retrieval and ranking contract
- [x] `rag_context_assembler.h` — context assembly and orchestration contract
- [x] `streaming_retriever.h` — context assembly and orchestration contract
- [x] `multi_step_rag.h` — context assembly and orchestration contract
- [x] `agentic_rag.h` — context assembly and orchestration contract
- [x] `multi_hop_reasoner.h` — context assembly and orchestration contract
- [x] `multimodal_rag.h` — context assembly and orchestration contract
- [x] `tensor_rag_pipeline.h` — context assembly and orchestration contract
- [x] `document_splitter.h` — context assembly and orchestration contract
- [x] `document_summarizer.h` — context assembly and orchestration contract
- [x] `rag_judge.h` — evaluation and quality control contract
- [x] `faithfulness_evaluator.h` — evaluation and quality control contract
- [x] `quality_control_pipeline.h` — evaluation and quality control contract
- [x] `quality_control_factory.h` — evaluation and quality control contract
- [x] `batch_evaluator.h` — evaluation and quality control contract
- [x] `distributed_rag_evaluator.h` — evaluation and quality control contract
- [x] `judge_ensemble.h` — evaluation and quality control contract
- [x] `relevance_evaluator.h` — evaluation and quality control contract
- [x] `coherence_evaluator.h` — evaluation and quality control contract
- [x] `completeness_evaluator.h` — evaluation and quality control contract
- [x] `cot_evaluator.h` — evaluation and quality control contract
- [x] `geval_evaluator.h` — evaluation and quality control contract
- [x] `rubric_evaluator.h` — evaluation and quality control contract
- [x] `pairwise_comparator.h` — evaluation and quality control contract
- [x] `delegate_evaluator.h` — evaluation and quality control contract
- [x] `nli_faithfulness_verifier.h` — evaluation and quality control contract
- [x] `prompt_injection_detector.h` — safety, guardrails, and learning contract
- [x] `bias_detector.h` — safety, guardrails, and learning contract
- [x] `fairness_detector.h` — safety, guardrails, and learning contract
- [x] `adversarial_tester.h` — safety, guardrails, and learning contract
- [x] `hallucination_dashboard.h` — safety, guardrails, and learning contract
- [x] `continuous_learning_orchestrator.h` — safety, guardrails, and learning contract
- [x] `continuous_learning_client.h` — safety, guardrails, and learning contract
- [x] `rlaif_trainer.h` — safety, guardrails, and learning contract
- [x] `ab_testing_framework.h` — safety, guardrails, and learning contract
- [x] `bayesian_optimizer.h` — safety, guardrails, and learning contract
- [x] `calibration_manager.h` — safety, guardrails, and learning contract
- [x] `rag_ingestion_bridge.h` — integration and metrics contract
- [x] `rag_integration_helpers.h` — integration and metrics contract
- [x] `llm_integration.h` — integration and metrics contract
- [x] `llm_judge_client.h` — integration and metrics contract
- [x] `llm_judge_integration.h` — integration and metrics contract
- [x] `llm_meta_analyzer.h` — integration and metrics contract
- [x] `onnx_model_loader.h` — integration and metrics contract
- [x] `http_metrics_client.h` — integration and metrics contract
- [x] `learning_metrics.h` — integration and metrics contract
- [x] `evaluation_cache.h` — integration and metrics contract
- [x] `evaluation_report_exporter.h` — integration and metrics contract
- [x] `prompt_templates.h` — integration and metrics contract
- [x] `response_parser.h` — integration and metrics contract
- [x] `claim_extractor.h` — integration and metrics contract
- [x] `citation_highlighter.h` — integration and metrics contract
- [x] `knowledge_gap_detector.h` — integration and metrics contract
- [x] `explainability_reason_builder.h` — integration and metrics contract
- [x] `judge_config.h` — integration and metrics contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/rag/ROADMAP.md`](../../src/rag/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
