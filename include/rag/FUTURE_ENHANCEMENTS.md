> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/rag/FUTURE_ENHANCEMENTS.md -->

# RAG Module — Public Header Future Enhancements

**Module Path:** `include/rag/`
**Canonical implementation enhancements:** [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md)

---

## Scope

This document covers planned enhancements to the **public header contract** in `include/rag/` — new types, interface additions, deprecation removals, and header-level API improvements. Enhancements that touch both headers and implementation are tracked primarily in the canonical source-level document:

→ [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Headers must remain backward-compatible within a major version; new capabilities are added via new methods or versioned types.
- `[x]` `#pragma once` guard required on every header; no include-guard macros.
- `[x]` No implementation code in headers (exception: `constexpr` helpers, template bodies, and header-only utilities explicitly documented as such).
- `[x]` All factory functions and error-returning methods must be `[[nodiscard]]`.
- `[x]` Build-conditional headers must not be included unconditionally by other headers.

---

## Execution Plane Surface

- **Retrieval and ranking plane:** `hybrid_retriever.h`, `reranker.h`, `replug_retriever.h`, `adaptive_retrieval.h`, `flare_retrieval.h`, `targ_retrieval.h`, `lora_enhanced_retriever.h`, `knowledge_graph_retriever.h`, `ontology_aware_retriever.h`, `dpr_vectorizer.h`, `vectorizer_interface.h`
- **Context assembly and orchestration plane:** `rag_context_assembler.h`, `streaming_retriever.h`, `multi_step_rag.h`, `agentic_rag.h`, `multi_hop_reasoner.h`, `multimodal_rag.h`, `tensor_rag_pipeline.h`, `document_splitter.h`, `document_summarizer.h`
- **Evaluation and quality control plane:** `rag_judge.h`, `faithfulness_evaluator.h`, `quality_control_pipeline.h`, `quality_control_factory.h`, `batch_evaluator.h`, `distributed_rag_evaluator.h`, `judge_ensemble.h`, `relevance_evaluator.h`, `coherence_evaluator.h`, `completeness_evaluator.h`, `cot_evaluator.h`, `geval_evaluator.h`, `rubric_evaluator.h`, `pairwise_comparator.h`, `delegate_evaluator.h`, `nli_faithfulness_verifier.h`
- **Safety, guardrails, and learning plane:** `prompt_injection_detector.h`, `bias_detector.h`, `fairness_detector.h`, `adversarial_tester.h`, `hallucination_dashboard.h`, `continuous_learning_orchestrator.h`, `continuous_learning_client.h`, `rlaif_trainer.h`, `ab_testing_framework.h`, `bayesian_optimizer.h`, `calibration_manager.h`
- **Integration and metrics plane:** `rag_ingestion_bridge.h`, `rag_integration_helpers.h`, `llm_integration.h`, `llm_judge_client.h`, `llm_judge_integration.h`, `llm_meta_analyzer.h`, `onnx_model_loader.h`, `http_metrics_client.h`, `learning_metrics.h`, `evaluation_cache.h`, `evaluation_report_exporter.h`, `prompt_templates.h`, `response_parser.h`, `claim_extractor.h`, `citation_highlighter.h`, `knowledge_gap_detector.h`, `explainability_reason_builder.h`, `judge_config.h`

For the authoritative interface inventory and stability classification see [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md).

---

## Planned Header Enhancements

### 1. `[[nodiscard]]` Audit

**Priority:** Medium
**Target Version:** v2.1.0

Audit all public headers for factory functions and error-returning methods that are missing `[[nodiscard]]`. Apply missing annotations and add a CI compile-time check to prevent regressions.

---

### 2. Deprecated Symbol Cleanup

**Priority:** Low
**Target Version:** v2.1.0

Identify symbols that have been superseded in `v1.x` and annotate them with `[[deprecated("use X instead")]]`. Track removal in a subsequent major version.

---

### 3. Header Isolation Verification

**Priority:** Low
**Target Version:** v2.1.0

Verify that every header in `include/rag/` compiles in isolation (without implicit transitive includes). Add a CMake `check_headers` target for automated CI enforcement.

---

## Test Strategy

| Test Type | Target | Notes |
|---|---|---|
| Compile-time | All headers compile in isolation | CMake `check_headers` target (planned) |
| Unit | Key interface implementations | Tracked in module test suite |
| ABI | No unexpected virtual table changes between patch releases | ABI checker in CI |

---

## Security / Reliability

- `[x]` `[[nodiscard]]` applied to factory and error-returning methods.
- `[x]` No implementation code in public headers.
- `[x]` Build-conditional guards documented in `ARCHITECTURE.md`.

---

## References

- Canonical implementation enhancements: [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Module overview: [`README.md`](README.md)
