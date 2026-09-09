# RAG Module - Architecture Guide

<!-- Status: current | validated: 2026-08-18 (Phase 6 Acceptance) -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · CHANGELOG.md · MODULE_STATUS.md -->
<!-- Phase 6: Documentation enhanced with thread-safety, complexity analysis, failure modes -->

Version: 1.1
Last Updated: 2026-09-09
Module Path: src/rag/

## 1. Overview

The RAG module implements retrieval, context construction, evaluation, and guardrail surfaces for retrieval-augmented generation workflows.

**Phase 6 Status:** Documentation and acceptance complete. All critical gap fixes from Batches 1-3 documented. API contracts frozen with production-ready Doxygen documentation.

## 2. Architecture Surfaces

| Surface | Source files | Maturity | Thread-Safe |
|---|---|---|---|
| Retrieval fusion and ranking | src/rag/hybrid_retriever.cpp, src/rag/reranker.cpp, src/rag/replug_retriever.cpp | PROD | ✅ |
| Context assembly and orchestration | src/rag/streaming_retriever.cpp, src/rag/rag_context_assembler.cpp, src/rag/multi_step_rag.cpp | PROD | ✅ |
| Ingestion bridge and enrichment | src/rag/rag_ingestion_bridge.cpp, src/rag/document_splitter.cpp | PROD | ✅ |
| Evaluation and quality control | src/rag/rag_judge.cpp, src/rag/faithfulness_evaluator.cpp, src/rag/quality_control_pipeline.cpp | PROD | ✅ |
| Adaptive and iterative retrieval | src/rag/adaptive_retrieval.cpp, src/rag/agentic_rag.cpp, src/rag/multi_hop_reasoner.cpp | PROD | ⚠️ |
| Safety and sanitization | src/rag/prompt_injection_detector.cpp, src/rag/bias_detector.cpp | PROD | ✅ |
| Metrics and reporting | src/rag/hallucination_dashboard.cpp, src/rag/evaluation_report_exporter.cpp | PROD | ⚠️ |
| Reliability benchmarking | src/rag/delegate_evaluator.cpp, src/rag/batch_evaluator.cpp | EVAL | ⚠️ |

**Legend:** PROD = Production-Ready, EVAL = Evaluation-Only, ✅ = Thread-Safe, ⚠️ = See concurrency model

## 3. Runtime Control Flow

1. Query enters retrieval path.
2. Hybrid or adaptive retriever selects candidate chunks (thread-safe under concurrent load).
3. Context assembler builds bounded prompt context (deterministic, O(n log n)).
4. Ingestion bridge enriches retrieved documents with NER entities (thread-safe, O(d*e)).
5. Generation and evaluation stages run with quality/safety checks.
6. Result, citations, and diagnostics are emitted to downstream handlers.

## 4. Integration Boundaries

| Direction | Integration | Status |
|---|---|---|
| Used by | API handlers, orchestration layers, AI runtime features | ✅ Documented |
| Uses | llm module (ContextWindowBudget), index/search surfaces, ingestion toolbox | ✅ Documented |
| Exposes | retrieval APIs, context assembly outputs (AssembledContext), evaluation signals | ✅ Documented |

## 5. Concurrency Model (Phase 6 Updated)

### Thread-Safe Components
- **RAGContextAssembler:** All methods are const or operate on local state. Safe for concurrent assemble() calls.
  - No mutable shared state beyond constructor-injected config_
  - Config changes via setConfig() must not race with concurrent assemble() calls
- **RAGIngestionBridge:** All public methods thread-safe.
  - Holds no mutable state beyond constructor-injected shared pointers (toolbox, vector_writer, graph_writer)
  - Safe for concurrent indexDocument(), enrichRetrievedDocuments(), extractEntitiesForContext() calls
- **Retrieval fusion & ranking:** Thread-safe under concurrent request load
  - Shared caches use coordinated access patterns
  - No unsynchronized reads during concurrent retrieval
- **Quality control & judges:** Thread-safe
  - Evaluation stages run independently per request

### Components Requiring Synchronization
- **Adaptive retrieval (agentic_rag.cpp):** Maintains per-request state; use separate instances per concurrent request
- **Metrics & reporting (hallucination_dashboard.cpp):** Aggregates metrics under concurrent load; uses internal synchronization
- **Continuous learning orchestrator:** Signal providers must be thread-safe; see wireLiveSignalProviders()

## 6. Complexity Analysis (Phase 6 Documented)

| Component | Operation | Complexity | Notes |
|---|---|---|---|
| RAGContextAssembler | assemble() | O(n log n) | Sorting by relevance + greedy fill |
| RAGContextAssembler | truncateContent() | O(k) | k = content length, deterministic |
| RAGIngestionBridge | indexDocument() | O(m * e) | m = text size, e = extraction overhead |
| RAGIngestionBridge | enrichRetrievedDocuments() | O(d * e) | d = doc count, e = extraction per doc |
| RAGIngestionBridge | extractEntitiesForContext() | O(t) | t = text size, delegation to toolbox |
| RAGIngestionBridge | buildEntityContext() | O(n) | n = entity count, stateless formatting |

## 7. Known Limits

- Retrieval quality and latency depend on configured backend and index state
- Benchmark coverage for all deployment topologies is still evolving
- Environment-dependent backend availability can alter runtime envelopes
- Ingestion document size bounded at 5 MiB (kMaxDocumentChars) to prevent memory exhaustion
- Collection names bounded at 256 chars (kMaxCollectionChars)
- Metadata values bounded at 16 KiB (kMaxMetadataValueChars)

## 8. Resource Bounds Enforcement (Phase 6 Updated)

All bounds enforced with fail-closed validation:

| Bound | Limit | File | Validation |
|---|---|---|---|
| Document size | 5 MiB | rag_ingestion_bridge.cpp:32 | lines 123-129, returns IndexResult.ok=false |
| Collection name | 256 chars | rag_ingestion_bridge.cpp:33 | lines 130-137, fail-closed |
| MIME type | 128 chars | rag_ingestion_bridge.cpp:34 | lines 138-145, fail-closed |
| Filename | 512 chars | rag_ingestion_bridge.cpp:35 | lines 146-153, fail-closed |
| Chunk snippet | 128 KiB | rag_ingestion_bridge.cpp:36 | truncation in metadata injection |
| Metadata value | 16 KiB | rag_ingestion_bridge.cpp:37 | boundedMetadataValue() helper |
| Context window | Configurable | rag_context_assembler.h:60 | RAGContextAssemblerConfig.model_context_tokens |
| Response budget | max(min_response_tokens, 20% window) | rag_context_assembler.cpp:70-82 | ContextWindowBudget enforcement |

## 9. Sourcecode Verification (Module: rag/architecture, Phase 6 Enhanced)

### API Documentation Status (Phase 6 Complete)
All public APIs now have comprehensive Doxygen documentation:

**include/rag/rag_context_assembler.h**
- ✅ Class documentation: 🟢 PRODUCTION-READY (100/100 score)
- ✅ All public methods: @brief, @param, @return, @throws, @pre, @post, @thread-safe, @complexity
- ✅ Failure modes documented: empty input, zero budget, all chunks over-budget
- ✅ Response reservation guarantee documented: max(min_response_tokens, 20% window)

**include/rag/rag_ingestion_bridge.h**
- ✅ Class documentation: 🟢 PRODUCTION-READY (86/100 score)
- ✅ All public methods: @brief, @param, @return, @throws, @pre, @post, @thread-safe, @complexity
- ✅ Failure modes documented: empty text, size validation, workflow fallback, I/O errors
- ✅ Thread-safety contract: no mutable state beyond constructor-injected pointers

### Implementation Comments (Phase 6 Added)
- ✅ src/rag/rag_context_assembler.cpp: complexity analysis, sorting logic, greedy fill algorithm
- ✅ src/rag/rag_ingestion_bridge.cpp: validation section, fallback path, error recovery, vector/graph writers

### Test Coverage Evidence (Phase 4 Complete, Phase 6 Documented)
- ✅ test_rag_budget_consistency_focused.cpp: 20 tests (budget determinism, propagation, truncation, response reservation)
- ✅ test_rag_ingestion_bridge_hardening_focused.cpp: 19 tests (malformed input, metadata, empty retrieval, determinism, error recovery)
- ✅ test_rag_error_handling_edge_cases_focused.cpp: 23 tests (malformed context, invalid budget, partial failures, backend fallback, resource exhaustion)
- Total: **62 new focused tests** + existing coverage

### Verified Components
- ✅ src/rag/hybrid_retriever.cpp (thread-safe, tested)
- ✅ src/rag/reranker.cpp (thread-safe, tested)
- ✅ src/rag/streaming_retriever.cpp (thread-safe, tested)
- ✅ src/rag/rag_context_assembler.cpp (thread-safe, O(n log n), 100% maturity)
- ✅ src/rag/rag_judge.cpp (thread-safe, tested)
- ✅ src/rag/quality_control_pipeline.cpp (thread-safe, mandatory gates)
- ✅ src/rag/adaptive_retrieval.cpp (tested, see concurrency notes)
- ✅ src/rag/rag_ingestion_bridge.cpp (thread-safe, 86% maturity, fail-closed validation)
- ✅ src/rag/prompt_injection_detector.cpp (thread-safe, safety-critical)
- ✅ src/rag/delegate_evaluator.cpp (evaluated)

### References
- **Issue Tracking:** 
  - Phases 1-4 complete: https://github.com/makr-code/ThemisDB/issues/5665
  - Parent epic: https://github.com/makr-code/ThemisDB/issues/5624
- **Governance:** DOCUMENTATION_GOVERNANCE.md (root governance file)
- **Status Report:** MODULE_STATUS.md (phases 1-6 evidence summary)
- **Production Checklist:** PRODUCTION_REQUIREMENTS.md (mandatory requirements + evidence)
- **Changelog:** CHANGELOG.md (version history, phase milestones)

## 10. Phase 6 Acceptance Sign-Off

| Criterion | Status | Evidence |
|---|---|---|
| API contracts frozen | ✅ COMPLETE | Doxygen maturity 100/100 (assembler), 86/100 (bridge) |
| Documentation synchronized | ✅ COMPLETE | CHANGELOG.md, ARCHITECTURE.md, PRODUCTION_REQUIREMENTS.md updated |
| Thread-safety guarantees documented | ✅ COMPLETE | @thread-safe annotations, concurrency model section |
| Complexity analysis documented | ✅ COMPLETE | @complexity tags, detailed comments in implementations |
| Failure modes documented | ✅ COMPLETE | @throws, @pre/@post conditions, error recovery patterns |
| Resource bounds enforced | ✅ COMPLETE | All 8 bounds with validation + documentation |
| Test coverage verified | ✅ COMPLETE | 62 focused tests + existing suite, all passing |
| Production requirements aligned | ✅ COMPLETE | PRODUCTION_REQUIREMENTS.md sync + evidence table |

**Phase 6 Status:** 🟢 COMPLETE – Ready for production deployment with comprehensive documentation and test coverage.

---

## Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| llm | `include/llm/inference_engine.h` | Generation inference calls via configured LLM backend |
| llm | `include/llm/context_window_budget.h` (`ContextWindowBudget`) | Token-budget computation for context assembly |
| llm | `include/llm/llm_plugin_interface.h` (`ILLMPlugin`) | Plugin-interface for swapping inference backends (llama.cpp, ONNX) |
| distributed_knowledge | `include/distributed_knowledge/` | Cross-node knowledge-shard retrieval |
| observability | `include/observability/` | Retrieval latency spans, recall metrics export |
| prompt_engineering | `include/prompt_engineering/` | Prompt template rendering and injection guards |
| training | `include/training/` | Continuous-learning signal wiring (`wireLiveSignalProviders()`) |
| security | `include/security/` | Prompt-injection detection at retrieval boundary |
| index | `include/index/ann_frontdoor.h`, `include/index/vector_index.h` (`IVectorIndex`) | ANN and vector candidate retrieval |
| storage | `include/storage/` (base entity) | Document and chunk persistence layer |
| graph | `include/graph/knowledge_graph_reasoner.h` | Graph-hop reasoning over knowledge graph |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| llm | `src/llm/` (context assembly budget) | LLM module triggers RAG context assembly before generation |
| api | API orchestration handlers | External RAG query and evaluation endpoints |
| search | `src/search/layered_retrieval_orchestrator.cpp` (Graph layer) | `LayeredRetrievalOrchestrator` invokes RAG knowledge-graph reasoning layer |

---

## Integration Points

### Critical Integration: ContextWindowBudget — LLM Budget Handoff
**Files:** `include/llm/context_window_budget.h` ↔ `src/rag/rag_context_assembler.cpp`
**Contract:** `ContextWindowBudget::compute()` determines maximum token allocation for retrieved context. `RAGContextAssembler::assemble()` is O(n log n) and greedy-fills within the computed budget. Response reservation hard-floor: `max(min_response_tokens, 20% window)`.
**Thread Safety:** `ContextWindowBudget` is stateless; `RAGContextAssembler::assemble()` is const / local-state only — safe for concurrent calls without external synchronisation.
**Failure Mode:** Zero-budget → returns empty `AssembledContext` with explicit error code; caller must handle before passing to generation.

### Critical Integration: ILLMPlugin — Inference Backend for Generation
**Files:** `include/llm/llm_plugin_interface.h` ↔ `src/rag/hybrid_retriever.cpp`, `src/rag/agentic_rag.cpp`
**Contract:** RAG generation steps call through `ILLMPlugin` to trigger inference. Plugin selection is delegated to `llm::LlmPluginManager`; RAG holds only a shared pointer to the active plugin.
**Thread Safety:** Plugin methods must be re-entrant; RAG holds read lock on plugin pointer.
**Failure Mode:** Plugin unavailable → `llm_unavailable` status; RAG returns partial result with `partial_result = true`.

### Critical Integration: IVectorIndex — ANN Candidate Retrieval
**Files:** `include/index/ann_frontdoor.h` ↔ `src/rag/hybrid_retriever.cpp`
**Contract:** `HybridRetriever` calls `IVectorIndex::search()` for dense candidates; results are fused with lexical results via RRF. The `ann_frontdoor.h` is the stable ABI entry-point (contract frozen at Wave B).
**Thread Safety:** `IVectorIndex` implementations must be thread-safe for concurrent `search()` calls; `HybridRetriever` does not add additional locking over the index.
**Failure Mode:** Index unavailable → degraded result with explicit `SearchStats::partial_result`; retrieval continues with available candidates.

### Critical Integration: Knowledge Graph Reasoner
**Files:** `include/graph/knowledge_graph_reasoner.h` ↔ `src/rag/multi_hop_reasoner.cpp`
**Contract:** `MultiHopReasoner` delegates graph traversal to `KnowledgeGraphReasoner`. Multi-hop depth is bounded by query config to prevent runaway traversal.
**Thread Safety:** `MultiHopReasoner` maintains per-request state; use separate instances per concurrent request.
**Failure Mode:** Graph traversal timeout → returns available hops; partial reasoning result flagged.

### Open: Recall@k Sign-Off + Wikipedia ABI Wiring
**Status:** PENDING — hardware p95/p99 latency gates and Wikipedia ABI wiring not yet signed off.
**Tracking:** RAG Phase completion: https://github.com/makr-code/ThemisDB/issues/5665

