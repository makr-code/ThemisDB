> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-08-24 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · PRODUCTION_REQUIREMENTS.md · MODULE_STATUS.md -->
<!-- Phase 6 Acceptance: CHANGELOG.md updated with all critical gap fixes from Batches 1-3 -->
<!-- Sprint 1 Phase B E2E: 2026-08-24 — BM25+/HNSW/RRF/LLM-Judge end-to-end activation -->
<!-- Issue References: makr-code/ThemisDB#5665 (tracking issue), makr-code/ThemisDB#5624 (parent epic) -->

# Changelog — RAG Module

All notable changes to the RAG module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [2.3.0] — 2026-08-24 – Sprint 1: Phase B Kern End-to-End Activation

### Phase B Retrieval Chain — Activated End-to-End

- **BM25+ scorer** (`WikiIndexStore::query`): production path confirmed active via
  `SecondaryIndexManager::scanFulltextWithScores` + `applyBm25PlusFloor(score, config_.bm25_delta)`.
  Parameters `bm25_k1=1.5`, `bm25_b=0.75`, `bm25_delta=0.5` validated in
  `WikiIndexConfig`. Gate `THEMIS_WIKI_PHASE_B` enabled by default.
- **HNSW index** (`WikiIndexStore` constructor): `VectorIndexManager::init` called with
  `hnsw_m=16`, `hnsw_ef_construction=200`, `Metric::COSINE`. Phase B gate guards init.
- **RRF fusion** (`HybridRetriever::fuseRRF`): `rrf_k=60.0`, equal BM25+vector weights
  by default. `WikiIndexStore::query` calls `retriever_.fuse(bm25_docs, vec_docs)`.
- **LLM-Judge real-mode** (`LLMJudgeIntegration`): production path via
  `ILLMInferenceEngine*` constructor injection confirmed. Mock fallback only reachable
  via explicit `allow_mock=true && use_mock_mode=true` (test-only). Gate
  `THEMIS_ENABLE_LLM_JUDGE` enabled by default.

### New Tests

- `tests/rag/test_rag_phase_b_e2e.cpp` — 7 focused integration tests:
  - PHASE-B-E2E-01: RRF fuses BM25 and HNSW candidates; shared-doc boosted to top rank.
  - PHASE-B-E2E-02: BM25-only mode (vector_weight=0): BM25-sourced doc outscores HNSW-only.
  - PHASE-B-E2E-03: Vector-only mode (bm25_weight=0): HNSW-sourced doc outscores BM25-only.
  - PHASE-B-E2E-04: RRF k=60 factory preset: all hybrid scores positive, strictly ordered.
  - PHASE-B-E2E-05: Real ILLMInferenceEngine injection: isMockMode()==false, engine called once.
  - PHASE-B-E2E-06: Gate disabled (enable_llm_judge=false): llm_unavailable returned, engine not called.
  - PHASE-B-E2E-07: Full Phase B chain — BM25 + HNSW → RRF → LLM-Judge — chunk-001 top-ranked and judged.

### Documentation

- `src/rag/ROADMAP.md`: BM25+, HNSW, RRF, Persistent Cache, LLM-Judge, Phase B acceptance boxes advanced from `[~]`/`[ ]` to `[x]` with source-aligned evidence citations.



All notable changes to the RAG module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [2.2.0] — 2026-08-18 – Phase 6 Documentation & Acceptance

### Thread Safety
- **RAGContextAssembler** thread-safety contract documented: all methods are const or operate on local state (no mutable shared state)
- **RAGIngestionBridge** thread-safety guarantees formalized: public methods thread-safe, no mutable state beyond constructor-injected shared pointers
- Added `@thread-safe` annotations to all synchronized methods in rag_context_assembler.h and rag_ingestion_bridge.h
- Concurrency model documented in ARCHITECTURE.md (retrieval/evaluation under concurrent load, caches/metrics coordinated)

### Performance & Complexity Analysis
- **RAGContextAssembler::assemble()** complexity documented as O(n log n) for sorting + O(n) for greedy fill = **O(n log n)** overall where n = chunk count
- **RAGContextAssembler::truncateContent()** complexity: O(n) where n = content length, deterministic truncation with marker
- **RAGIngestionBridge::indexDocument()** complexity: O(m * e) where m = document characters, e = entity extraction overhead (delegated to IngestionToolbox)
- **RAGIngestionBridge::enrichRetrievedDocuments()** complexity: O(d * e) where d = document count in retrieval result set
- Performance characteristics added to function documentation in both headers

### Resource Management & Bounds
- **Context Assembly Budget Limits**: model_context_tokens bounds enforced, response reservation = max(min_response_tokens, 20% window)
- **Ingestion Limits Enforced**:
  - Document size: kMaxDocumentChars = 5 MiB (prevents OOM)
  - Collection name: kMaxCollectionChars = 256 chars
  - Metadata values: kMaxMetadataValueChars = 16 KiB
  - Chunk snippet: kMaxChunkSnippetChars = 128 KiB
- All limits documented with rationale in implementation and headers

### Documentation Enhancements
- **API Documentation Review Complete**: all public functions in rag_context_assembler.h and rag_ingestion_bridge.h now have @brief, @param, @return, @throws documentation
- **Pre/Post Conditions Documented**:
  - `RAGContextAssembler::assemble()` @pre: config must be initialized, chunks must not be null
  - `RAGIngestionBridge::indexDocument()` @pre: toolbox must not be null; @post: either ok=true with doc_id, or ok=false with error message
- **Failure Modes Documented**:
  - Empty context/retrieval: returns valid empty context (not error)
  - Backend unavailable: fallback to vector-only or error signaling (configurable)
  - Malformed input: validation and boundary enforcement (fail-closed)
  - Missing metadata: handled with truncation/defaulting via boundedMetadataValue()

### Implementation Comments Added
- **Sorting comments** (rag_context_assembler.cpp line 101-113): Deterministic tie-breaking for relevance-equal chunks (id, source, content as secondary keys)
- **Budget computation comments** (rag_context_assembler.cpp line 70-82): Token budget lifecycle, response guard calculation
- **Greedy fill comments** (rag_context_assembler.cpp line 115-143): Chunk fit strategy, truncation decision logic
- **Entity extraction comments** (rag_ingestion_bridge.cpp): Delegation pattern to IngestionToolbox, deterministic hydration
- **Error recovery comments** (rag_ingestion_bridge.cpp): Optional graph writer fallback, partial indexing error signaling

### Test Coverage Verification (Phase 4 Evidence)
- **Budget Consistency Tests**: 20 tests in test_rag_budget_consistency_focused.cpp (Groups A-E: assembler determinism, propagation, truncation, response reservation)
- **Ingestion Bridge Hardening**: 19 tests in test_rag_ingestion_bridge_hardening_focused.cpp (Groups A-E: malformed input, missing metadata, empty retrieval, deterministic hydration, error recovery)
- **Error Handling & Edge Cases**: 23 tests in test_rag_error_handling_edge_cases_focused.cpp (Groups A-E: malformed context, invalid budget, partial failures, backend fallback, resource exhaustion)
- Total new focused tests: **62 tests** covering Phase 3 (error handling) and Phase 4 (testing) acceptance criteria

### Production Requirements Alignment
- **PRODUCTION_REQUIREMENTS.md synced** with current implementation state:
  - Prompt-injection-detector integration documented (active on all RAG paths)
  - Context assembly bounded-size enforcement documented
  - RAG judge and quality control pipeline documented as mandatory gates
  - Upstream authorization for retrieval scope documented
  - Bias detector calibration requirements documented
- Audit-fable minimal production check list verified (8/8 items mapped to code)

### Verification & Acceptance
- Maturity scores verified:
  - rag_context_assembler.h: 🟢 PRODUCTION-READY (100/100)
  - rag_ingestion_bridge.h: 🟢 PRODUCTION-READY (86/100)
  - rag_context_assembler.cpp: 🟢 PRODUCTION-READY (100/100)
  - rag_ingestion_bridge.cpp: 🟢 PRODUCTION-READY (84/100)
- Issue #5665 (RAG module phase 1-6) evidence update complete
- Issue #5624 (parent epic) tracking updated with Phase 1-4 completion status

## [Unreleased]

### Fixed
- `ContinuousLearningOrchestrator::wireLiveSignalProviders()`: passing `nullptr` for any dependency now calls `setXxxProvider({})` (empty function), correctly setting `signal_source = "fallback_missing"` instead of leaving the provider wired to a lambda that would throw on lock failure.
- `ContinuousLearningOrchestrator::triggerLoop()`: `LoopResult.success` now mirrors `guardrail_passed` for `LOOP_1_HNSW_QUERY`, `LOOP_2_WORKLOAD`, and `LOOP_4_RLAIF`; previously these loops always reported `success = true` regardless of guardrail outcome, blurring operational semantics. `LOOP_3_SCHEMA_INDEX` remains advisory and always succeeds.
- `LoopResult.metric_delta` is now `0.0` for guardrail-blocked runs, and completion-handler callbacks receive the correct `success`/`guardrail_passed` state.
### Added
- **B1 — Self-RAG** (`include/rag/self_rag.h`, `src/rag/self_rag.cpp`; `themis::rag`) — Wave B, issue #5039
  - `SelfRAGController` with injected retrieval + critic callbacks; `setRetrievalCallback()` / `setCriticCallback()` wiring hooks for `InferenceEngineEnhanced`.
  - `runRefinementLoop(query)` — iterative retrieval-and-critique loop (configurable `max_rounds`), cross-round deduplication, early-exit on Relevant grade saturation.
  - `SelfRAGResult` — `relevant_docs`, `rounds_used`, `retrieve_decided`.
  - 12 unit tests: SELF_RAG-01..12 (`tests/rag/test_self_rag.cpp`).
  - Stubs: SRG-S01 (threshold-heuristic retrieval controller); SRG-S02 (score-proxy critic).

### Changed
- Documentation governance sync: roadmap/future/audit/readme/architecture/security/performance docs aligned to source-verifiable statements; planning remains in roadmap/future and history remains in changelog.
- `ContinuousLearningOrchestrator::triggerLoop()` now snapshots loop stats (`current_accuracy`, `accuracy_7d_avg`, `lora_retraining_count`) under `impl_->mutex` before guardrail evaluation, eliminating unsynchronized stats reads during loop execution; fallback signal-provider error/invalid behavior is covered by new Loop-1/Loop-4 regression tests.
- Signal-source injection (`set{HnswMissRate,WorkloadDrift,FeedbackEntryCount}Provider`) is now fully wired at `HttpServer` bootstrap via `wireLiveSignalProviders(bao_optimizer_, workload_optimizer_, live_feedback_collector_)`; the "removal plan" stub note has been retired and replaced with an informational comment.

## [2.1.0] — 2026-04-16
### Added
- `RAGIngestionBridge` (`include/rag/rag_ingestion_bridge.h`, `src/rag/rag_ingestion_bridge.cpp`; `themis::rag` namespace):
  - `IndexResult` return type: `ok`, `doc_id`, `collection`, `entity_count`, `vector_count`, `error`
  - `indexDocument(text, collection)` — full ingestion workflow; writes to `IVectorWriter` and optional `IGraphWriter`; idempotent via content-hash `doc_id`
  - `enrichRetrievedDocuments(docs, query)` — attaches NER entities under `"_entities"` metadata key
  - `extractEntitiesForContext(text)` — delegates to `IngestionToolbox::extractEntities()`
  - `static buildEntityContext(entities)` — converts `BaseEntity` list to compact string for LLM prompt injection
  - Thread-safe: no mutable state beyond constructor-injected shared pointers

## [2.0.1] — 2026-04-15
### Added
- `ContinuousLearningOrchestrator` (`include/rag/continuous_learning_orchestrator.h`, `src/rag/continuous_learning_orchestrator.cpp`; `themis::rag::learning` namespace):
  - `ContinuousLearningConfig` — trigger thresholds, learning rates, A/B testing config, data-selection integration, federation paths
  - `startLearningLoop()` / `stopLearningLoop()` — background learning loop
  - `triggerLearningIteration()` — manual trigger
  - `LoopPhase` enum: `IDLE`, `LOOP_1_HNSW_QUERY`, `LOOP_2_WORKLOAD`, `LOOP_3_SCHEMA_INDEX`, `LOOP_4_RLAIF`
  - `LoopResult` — `phase`, `success`, `guardrail_passed`, `adapter_version`, `metric_delta`
  - `triggerLoop(LoopPhase)` — explicit named loop trigger with `LoopResult` return
  - `registerLoopCompletionHandler(LoopPhase, handler)` — per-phase synchronous completion callback
  - `TriggerEvent::FEDERATED_ROUND_START` — fires after Loop-4 with `guardrail_passed == true`
  - `setFederationCoordinator(ILoRAFederationCoordinator*)` — DI setter for federated LoRA aggregation
  - `setTrainerForFederation(IncrementalLoRATrainer*)` — DI setter for gradient export

## [2.0.0] — 2026-03-24
### Added
- `ReplugRetriever` — REPLUG-style co-trained retrieval fusion (Shi et al., 2023, arXiv:2301.12652):
  - `ILLMScorer` pluggable interface for model-agnostic perplexity scoring
  - `HeuristicLLMScorer` — Jaccard-based PPL proxy (no LLM runtime required)
  - `ReplugConfig` — λ interpolation weight, top_k, temperature, min_retrieval_score, LSR learning rate
  - `ReplugFusionResult` / `ReplugScore` — per-document score breakdown
  - `updateRetrieverWeights()` — REPLUG-LSR gradient step via KL-divergence
  - `ReplugRetrieverFactory` — createBalanced/createLLMDominant/createRetrievalDominant/createLSR
- `RLAIFTrainer` — Constitutional AI / RLAIF training pipeline (Bai et al., 2022; Lee et al., 2023):
  - `IAIJudge` pluggable interface for AI preference labelling
  - `HeuristicAIJudge` — heuristic critique/revision/judge (no LLM runtime required)
  - `AIPrinciple` — configurable constitutional principles (harmlessness/helpfulness/honesty/fairness)
  - `PreferencePair` — (prompt, chosen, rejected) training example
  - `runTrainingStep()` — end-to-end CAI critique-revision + preference pair generation
  - `createPreferencePair()` — direct judge-based preference labelling
  - `processBatch()` — queue-based batch training
  - `RLAIFConfig` — revision iterations, quality threshold, max dataset size
  - `RLAIFTrainerFactory` — createDefault/createStrict/createFast/createWithJudge
- 30 unit tests for `ReplugRetriever` (`tests/test_rag_replug_retriever.cpp`)
- 30 unit tests for `RLAIFTrainer` (`tests/test_rag_rlaif_trainer.cpp`)
- Focused test executables: `ReplugRetrieverFocusedTests`, `RLAIFTrainerFocusedTests`

## [1.5.0] — 2026-03-12
### Added
- Multi-modal RAG pipeline with text, image, and audio retrieval
- Hybrid dense+sparse retrieval (BM25 + vector)
- Cross-encoder reranking for improved relevance scoring
- Contextual compression to reduce token usage
- Citation and source attribution in generated answers
- RAG evaluation framework with RAGAS metrics
- Streaming RAG responses via SSE

### Changed
- Improved chunking strategies (sentence-aware, recursive)
- Enhanced metadata filtering during retrieval

## [1.0.0] — 2024-06-01
### Added
- Initial RAG pipeline: document ingestion, chunking, embedding, retrieval
- Integration with LLM module for answer generation
- Vector store integration for semantic search

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
