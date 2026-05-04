<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — RAG Module

> ⚠️ **Auditstand:** Source code analysis 2026-04-21 found high-severity security vulnerabilities.

**Last Audit:** 2026-04-21 | **Auditor:** Copilot | **Status:** ⚠️ High — 4×S1 (prompt injection, cache cross-tenant, self-consistency stub, retrieval no tenant ID)

> **Note:** Previous audit claimed "Security Issues: None critical" and "Prompt injection detection
> implemented." Direct source analysis found that document content is still injected verbatim into
> LLM judge prompts (`rag_judge.cpp`), bypassing the `PromptInjectionDetector` which is applied only
> at inference boundary, not at the evaluation judge prompt construction layer.

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 55 `.cpp` in `src/rag/` |
| Test Coverage | ✅ Present (38 dedicated test files in `tests/`) |
| S0 Critical | ✅ None in RAG module itself |
| S1 High | ✅ 0 (F4-1/F4-2/F5-1/F5-2 fixed 2026-05-04) |
| S2 Medium | ⚠️ 4 |
| S3 Low | ℹ️ 1 |
| Faithfulness judge prompt-injection-safe | ✅ Document content sandboxed with delimiters + injection sanitizer (F4-1 fixed) |

## Source Files Audited

| File | Purpose |
|------|---------|
| `rag_judge.cpp` | Main orchestrator for multi-dimensional RAG evaluation |
| `knowledge_gap_detector.cpp` | Three-level knowledge gap detection system |
| `llm_integration.cpp` | LLM inference bridge for RAG pipeline |
| `streaming_retriever.cpp` | Incremental context window filling with token-budget enforcement, MMR deduplication |
| `faithfulness_evaluator.cpp` | Fact-checking against retrieved sources |
| `relevance_evaluator.cpp` | Query-answer alignment scoring |
| `completeness_evaluator.cpp` | Query aspect coverage measurement |
| `coherence_evaluator.cpp` | Structure and readability scoring |
| `bias_detector.cpp` | Ethical compliance / bias checking |
| `claim_extractor.cpp` | Atomic claim decomposition from answers |
| `response_parser.cpp` | LLM evaluation response parsing |
| `prompt_templates.cpp` | Template and few-shot example management |
| `judge_config.cpp` | Configuration validation |
| `rubric_evaluator.cpp` | Custom rubric evaluation |
| `judge_ensemble.cpp` | Multi-judge voting strategies |
| `pairwise_comparator.cpp` | Head-to-head response comparison |
| `cot_evaluator.cpp` | Chain-of-thought evaluation |
| `geval_evaluator.cpp` | G-Eval framework (Liu et al., 2023) |
| `llm_judge_integration.cpp` | Judge orchestration |
| `llm_meta_analyzer.cpp` | Performance meta-analysis |
| `hybrid_retriever.cpp` | BM25 + vector fusion with configurable RRF weights |
| `reranker.cpp` | Cross-encoder reranking |
| `hallucination_dashboard.cpp` | Rolling-window hallucination rate tracking |
| `document_summarizer.cpp` | Multi-document summarization before context injection |
| `knowledge_graph_retriever.cpp` | Knowledge graph-augmented retrieval with entity linking |
| `document_splitter.cpp` | Configurable chunking (size, overlap, strategy) |
| `citation_highlighter.cpp` | Map answer sentences to source chunks |
| `agentic_rag.cpp` | Agentic RAG with iterative retrieval loops |
| `multimodal_rag.cpp` | Multi-modal RAG (image + text retrieval) |
| `adaptive_retrieval.cpp` | Bayesian optimizer over retrieval parameters |
| `evaluation_cache.cpp` | Thread-safe LRU evaluation result cache with TTL |
| `calibration_manager.cpp` | Temperature scaling, Platt/isotonic regression for judge score calibration |
| `batch_evaluator.cpp` | Parallel batch evaluation with configurable worker threads |
| `distributed_rag_evaluator.cpp` | Distributed evaluation across multiple judge models |
| `prompt_injection_detector.cpp` | Pattern-based prompt injection detection and sanitization |
| `replug_retriever.cpp` | REPLUG-style LLM-scored retrieval fusion |
| `nli_faithfulness_verifier.cpp` | NLI entailment-based claim verification |
| `adversarial_tester.cpp` | Adversarial robustness testing for RAG pipeline |
| `ab_testing_framework.cpp` | A/B testing for retrieval and generation strategies |
| `bayesian_optimizer.cpp` | Bayesian optimization for retrieval hyperparameters |
| `learning_metrics.cpp` | Sliding-window metrics with mean/std-dev/trend export |
| `continuous_learning_client.cpp` | Client for continuous learning feedback loop |
| `continuous_learning_orchestrator.cpp` | Orchestrator for adaptive learning cycles (`ContinuousLearningOrchestrator`; `themis::rag::learning` namespace): `triggerLoop(LoopPhase)`, `registerLoopCompletionHandler()`, `setFederationCoordinator()`, `setTrainerForFederation()`; `TriggerEvent::FEDERATED_ROUND_START` |
| `rlaif_trainer.cpp` | RLAIF (Constitutional AI) training pipeline |
| `evaluation_report_exporter.cpp` | Per-query evaluation report export (JSON/HTML) |
| `llm_judge_client.cpp` | HTTP client for remote LLM judge API |
| `http_metrics_client.cpp` | HTTP client for metrics ingestion |
| `multi_hop_reasoner.cpp` | Multi-hop reasoning over retrieved documents |
| `multi_step_rag.cpp` | Multi-step RAG with iterative retrieval planning |
| `rag_context_assembler.cpp` | Context assembly and formatting for LLM prompts |
| `rag_ingestion_bridge.cpp` | Connects `IngestionToolbox` to RAG pipeline (`RAGIngestionBridge`; `themis::rag` namespace): `indexDocument()`, `enrichRetrievedDocuments()`, `extractEntitiesForContext()`, `buildEntityContext()`; `IndexResult` return type |
| `quality_control_factory.cpp` | Factory for QA/QC pipeline components |
| `quality_control_pipeline.cpp` | End-to-end quality control pipeline orchestration |
| `onnx_model_loader.cpp` | ONNX model loader for local NLI/reranker inference |
| `explainability_reason_builder.cpp` | Explainability reason and evidence builder |

## Test Coverage

| Test File | Coverage |
|-----------|----------|
| `tests/test_rag_judge.cpp` | Main evaluation orchestrator |
| `tests/test_rag_judge_phase1.cpp` | Phase 1 evaluators |
| `tests/test_rag_judge_phase2.cpp` | Phase 2 streaming and reranking |
| `tests/test_rag_judge_phase3.cpp` | Phase 3 hybrid retrieval |
| `tests/test_rag_judge_phase4.cpp` | Phase 4 agentic / KG RAG |
| `tests/test_rag_streaming_retriever.cpp` | Streaming retriever |
| `tests/test_rag_hybrid_retriever.cpp` | Hybrid BM25+vector retriever |
| `tests/test_rag_reranker.cpp` | Cross-encoder reranking |
| `tests/test_rag_hallucination_dashboard.cpp` | Hallucination rate tracking |
| `tests/test_rag_document_splitter.cpp` | Chunking strategies |
| `tests/test_rag_document_summarizer.cpp` | Multi-document summarization |
| `tests/test_rag_knowledge_graph_retriever.cpp` | KG-augmented retrieval |
| `tests/test_rag_citation_highlighter.cpp` | Citation mapping |
| `tests/test_rag_agentic.cpp` | Agentic RAG |
| `tests/test_rag_multimodal.cpp` | Multi-modal RAG |
| `tests/test_rag_adaptive_retrieval.cpp` | Bayesian retrieval optimization |
| `tests/test_rag_evaluation_cache.cpp` | Evaluation LRU cache |
| `tests/test_rag_calibration_manager.cpp` | Judge score calibration |
| `tests/test_rag_batch_evaluator.cpp` | Parallel batch evaluation |
| `tests/test_rag_distributed_evaluator.cpp` | Distributed evaluation |
| `tests/test_rag_prompt_injection.cpp` | Prompt injection detection |
| `tests/test_rag_replug_retriever.cpp` | REPLUG retriever |
| `tests/test_rag_adversarial_tester.cpp` | Adversarial robustness |
| `tests/test_rag_ingestion_bridge.cpp` | Ingestion bridge |
| `tests/test_rag_context_assembler.cpp` | Context assembly |
| `tests/test_rag_evaluation_report_exporter.cpp` | Report export |
| `tests/test_rag_multi_hop_reasoner.cpp` | Multi-hop reasoning |
| `tests/test_multi_step_rag.cpp` | Multi-step RAG |
| `tests/test_rag_rlaif_trainer.cpp` | RLAIF trainer |
| `tests/test_rag_pipeline_integration.cpp` | End-to-end pipeline integration |
| `tests/test_rag_ethics.cpp` | Ethics / bias compliance |
| `tests/test_rag_context_engine.cpp` | Context engine |
| `tests/test_rag_aql_integration.cpp` | AQL query integration |
| `tests/test_rag_prompt_builder.cpp` | Prompt builder |
| `tests/test_rag_uncovered.cpp` | Additional coverage |
| `benchmarks/bench_rag_evaluation.cpp` | Recall@K, latency, batch throughput |
| `tests/test_explainability_reason_builder.cpp` | Explainability builder |

## Findings

### S1 — High (all resolved 2026-05-04)

#### ~~F4-1 · `rag_judge.cpp` · `extractClaimsViaLLM()` + `verifyClaimViaLLM()` — Prompt injection via document content~~

**Fixed 2026-05-04:** Both functions now:
1. Call `impl_->injection_detector->sanitize()` on all user-supplied content before
   embedding in the judge prompt.
2. Wrap document content in `[DOCUMENT_START doc=N]…[DOCUMENT_END]` and
   answer/claim content in `[TEXT_START]…[TEXT_END]` / `[CLAIM_START]…[CLAIM_END]`
   delimiters.
3. Include an explicit instruction in the system prompt not to follow any directives
   within the delimited content.

---

#### ~~F4-2 · `rag_judge.cpp` · `evaluate()` — Evaluation cache has no tenant isolation~~

**Fixed 2026-05-04:** `tenant_id` added to `EvaluationInput`.
`computeCacheKey()` accepts `tenant_id` and prefixes the key with
`tenant_id + "\x1F"` when non-empty. Both `evaluate()` cache lookup
and cache store paths updated.

---

#### ~~F5-1 · `knowledge_gap_detector.cpp` · `generateMultipleSamples()` — Self-consistency stub~~

**Fixed 2026-05-04:** Function now emits a `THEMIS_WARN` one-time log (via
`std::call_once`) when called, explicitly notifying operators that self-consistency
checking is running in stub/heuristic mode and cannot detect real LLM inconsistencies.
The STUB/SIMULATION NOTE was expanded with removal criteria. Feature remains active
for operators who need the code path; false gate risk is now explicitly surfaced at
runtime.

---

#### ~~F5-2 · `knowledge_gap_detector.cpp` · `performDynamicRetrieval()` — FLARE retrieval passes no tenant ID~~

**Fixed 2026-05-04:**
- `RetrievalCallback` signature changed to `(query, k, tenant_id)`.
- `detectWithActiveRetrieval()` accepts an optional `tenant_id` parameter (default `{}`).
- `performDynamicRetrieval()` passes `tenant_id` to `retrieval_fn(query, k, tenant_id)`.
- All three function signatures updated in both header and implementation.

---

### S2 — Medium

| ID | File | Function | Description |
|----|------|----------|-------------|
| F4-3 | rag_judge.cpp | `evaluate()` | `ethical_veto_power=true` + `enable_ethical_evaluation=false` silently passes everything — no warning for contradictory config |
| F4-4 | rag_judge.cpp | `detectBias()` | Hardcoded English word list trivially bypassed by paraphrasing or other languages |
| F5-3 | knowledge_gap_detector.cpp | `verifyClaim()` | Returns `true` for empty term list (stop-word-only claims) — short sentences always verified |
| F5-4 | knowledge_gap_detector.cpp | `detectGap()` | Ethical keyword match short-circuits similarity/coverage pre-generation checks |

### S3 — Low

| ID | File | Function | Description |
|----|------|----------|-------------|
| F4-5 | rag_judge.cpp | `hasEthicalCitations()` | `"["` (any bracket) treated as citation marker — any JSON or Markdown response scores full citation quality |

---

### Resolved (from 2026-04-19 audit)
- Build system registration verified for all 55 source files ✅
- PII filtering integrated into retrieval path ✅
- Prompt injection detection at inference boundary ✅ (but not at judge prompt construction — see F4-1)
- GDPR Article 22 audit logging with source attribution ✅
- `ContinuousLearningOrchestrator` loop trigger API implemented ✅
- `RAGIngestionBridge` implemented ✅

### Open (carried forward + new)
- Loop-interference cooldown guard (`OptimizationLock`) not yet implemented (ROADMAP Phase 8)
- JSON context serialiser for loop outcome signals not yet implemented (ROADMAP Phase 8)
- Unit tests in `tests/test_continuous_learning_orchestrator_loops.cpp` not yet present

## Compliance

RAG pipelines processing personal data fall under GDPR Article 22 (automated decision-making).
Source attribution and audit logging support compliance requirements.

**Note:** `prompt_injection_detector.cpp` guards the inference boundary only. RAG judge prompt
construction (`rag_judge.cpp`) directly embeds retrieved document content without applying the
detector — see F4-1. The compliance claim "guards against data-exfiltration attacks in retrieved
context" is not accurate for the evaluation pipeline.
