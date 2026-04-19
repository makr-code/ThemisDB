> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# RAG Module Roadmap

## Current Status
v2.0.0 – Production-ready Retrieval-Augmented Generation system. 27 implementation files covering evaluation, knowledge gap detection, ethical compliance, multi-judge orchestration, streaming retrieval, cross-encoder re-ranking, hybrid BM25+vector retrieval, batch evaluation, calibration, LRU evaluation caching, REPLUG-style LLM-scored fusion, and Constitutional AI / RLAIF training pipeline.

## Completed ✅
- [x] RAGJudge – main orchestrator for multi-dimensional evaluation
- [x] KnowledgeGapDetector – three-level gap detection system
- [x] LLM integration bridge to InferenceEngineEnhanced
- [x] FaithfulnessEvaluator – fact-checking against retrieved sources
- [x] RelevanceEvaluator – query-answer alignment scoring (TF-cosine semantic similarity)
- [x] CompletenessEvaluator – query aspect coverage measurement
- [x] CoherenceEvaluator – structure and readability scoring
- [x] BiasDetector – ethical compliance checking
- [x] ClaimExtractor – atomic claim decomposition
- [x] ResponseParser – LLM evaluation response parsing
- [x] PromptTemplates – template and few-shot example management
- [x] JudgeConfig – configuration validation
- [x] RubricEvaluator – custom rubric evaluation
- [x] JudgeEnsemble – multi-judge voting strategies
- [x] PairwiseComparator – head-to-head response comparison
- [x] CoTEvaluator – chain-of-thought evaluation
- [x] GEvalEvaluator – G-Eval framework (Liu et al., 2023)
- [x] LLMJudgeIntegration – judge orchestration
- [x] LLMMetaAnalyzer – performance meta-analysis
- [x] Fast (~100 ms), Balanced (~500 ms), and Thorough (~2 s) evaluation modes
- [x] StreamingRetriever – incremental context window filling (Issue: #2437)
- [x] CrossEncoderReranker – re-ranking with heuristic scorer and ONNX stub (Issue: #2247)
- [x] HallucinationDashboard – rolling-window hallucination rate tracking (Issue: #2438)
- [x] DocumentSummarizer – multi-document summarization before context injection (Issue: #2239)
- [x] KnowledgeGraphRetriever – knowledge graph-augmented retrieval with entity linking (Issue: #2242)
- [x] DocumentSplitter – configurable chunk size, overlap, and strategy for document splitting (Issue: #2238)
- [x] HybridRetriever – BM25 + vector fusion with configurable RRF weights (Issue: #1968)
- [x] RAGJudge::extractClaims() – LLM-first + heuristic fallback dispatch (Issue: #1296, Target: Q1 2026) — Inputs: answer text; Outputs: vector of claim strings; Errors: JSON parse failure falls back to heuristic; Tests: unit + LLM mock; Perf: <500ms for 1k-char input
- [x] RAGJudge::verifyClaimAgainstDocuments() – NLI → LLM → semantic fallback dispatch (Issue: #1296, Target: Q1 2026) — Inputs: claim + documents; Outputs: bool support decision; Errors: NLI/LLM failure cascades to term-overlap; Tests: unit + NLI mock; Perf: <200ms per claim
- [x] NLIFaithfulnessVerifier integrated into RAGJudge for entailment-based claim verification (Issue: #1296, Target: Q1 2026) — Member of RAGJudge::Impl; threshold: 0.7; graceful degradation when model not loaded
- [x] FaithfulnessEvaluator::extractClaims() – LLM-first + sentence-boundary fallback (Issue: #1296, Target: Q1 2026) — Inputs: answer text; Outputs: vector of Claim structs; Errors: JSON parse failure falls back to regex; LLM confidence: 0.9, heuristic confidence: 0.6
- [x] LearningMetrics – sliding-window metrics with mean/std-dev/trend export (Issue: #1296, Target: Q1 2026) — Tracks accuracy, faithfulness, relevance, completeness, coherence; CSV export; thread-safe with std::mutex
- [x] Citation highlighting (map answer sentences to source chunks) (Issue: #2436, #2000)
- [x] Online learning from evaluation feedback – adaptive retrieval via Bayesian optimization over `top_k` and `similarity_threshold`, driven by both user feedback and RAGJudge evaluation confidence scores; `getOptimizedRetrievalParams()` API (Issue: #2244)
- [x] EvaluationCache – thread-safe LRU cache with TTL expiry, invalidation triggers, and statistics tracking (`evaluation_cache.cpp`)
- [x] CalibrationManager – temperature scaling, Platt scaling, and isotonic regression to align judge scores with human annotations; ECE/Brier/correlation metrics (`calibration_manager.cpp`)
- [x] BatchEvaluator – parallel batch processing with configurable worker threads, async evaluation via futures/promises, and aggregated statistics (`batch_evaluator.cpp`)
- [x] `batchConvertToRetrievedDocuments` – implemented with `EmbeddingFunction` callback; sequential per-query K-NN search; no placeholder / DO NOT USE warning removed (`rag_integration_helpers.h`)
- [x] `RAGIngestionBridge` — connects `IngestionToolbox` to the RAG pipeline (`include/rag/rag_ingestion_bridge.h`, `src/rag/rag_ingestion_bridge.cpp`; `themis::rag` namespace): `indexDocument()`, `enrichRetrievedDocuments()`, `extractEntitiesForContext()`, `buildEntityContext()`; `IndexResult` return type; thread-safe (v0.1.0)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [x] Agentic RAG with iterative retrieval loops (`rag/agentic_rag.cpp`) (Issue: #2241)
- [x] Multi-modal RAG (image + text retrieval) (`rag/multimodal_rag.cpp`) (Issue: #2243)
- [x] Online learning from evaluation feedback (adaptive retrieval) (Issue: #2244)
- [x] Distributed RAG evaluation across multiple judge models (Issue: #2245) — `rag/distributed_rag_evaluator.h/.cpp`; thread-pool parallel dispatch; MEAN/WEIGHTED_MEAN/MAJORITY_VOTING/BEST_OF_N aggregation; inter-judge agreement metric; factory helpers
- [x] Performance benchmarks (recall@10, latency targets) — `benchmarks/bench_rag_evaluation.cpp`; recall@K harness; FAST/BALANCED/THOROUGH latency; batch throughput; DistributedRAGEvaluator benchmark; PromptInjectionDetector scan throughput; end-to-end pipeline
- [x] Security audit (prompt injection in retrieved context) — `rag/prompt_injection_detector.h/.cpp`; pattern-based detection (instruction-override, system-prompt-leak, delimiter-escape, role-injection, markup-injection, Unicode bidi); density threshold; PromptInjectionSanitizer; full unit test coverage

## Implementation Phases

### Phase 1: Evaluation Pipeline & Multi-Judge System (Status: Completed ✅)
- [x] `RAGJudge` – main orchestrator for multi-dimensional evaluation
- [x] `KnowledgeGapDetector` – three-level gap detection system
- [x] LLM integration bridge to `InferenceEngineEnhanced`
- [x] `FaithfulnessEvaluator`, `RelevanceEvaluator`, `CompletenessEvaluator`, `CoherenceEvaluator`
- [x] `BiasDetector` – ethical compliance checking
- [x] `ClaimExtractor`, `ResponseParser`, `PromptTemplates`, `JudgeConfig`
- [x] `RubricEvaluator`, `JudgeEnsemble`, `PairwiseComparator`
- [x] `CoTEvaluator`, `GEvalEvaluator` (Liu et al., 2023), `LLMJudgeIntegration`, `LLMMetaAnalyzer`
- [x] Fast (~100 ms), Balanced (~500 ms), and Thorough (~2 s) evaluation modes

### Phase 2: Streaming Retrieval & Re-Ranking (Status: Completed ✅)
- [x] Streaming retrieval with incremental context window filling
- [x] Re-ranking layer with cross-encoder model integration
- [x] Hallucination rate tracking dashboard

### Phase 3: Hybrid Retrieval & Citation Highlighting (Status: Completed ✅)
- [x] Hybrid retrieval (BM25 + vector) with configurable RRF weights
- [x] Citation highlighting (map answer sentences to source chunks)
- [x] Configurable chunk size and overlap for document splitting
- [x] Multi-document summarization before context injection
- [x] Per-query evaluation report export (JSON / HTML) (Issue: #2240)

### Phase 4: Agentic & Knowledge-Graph RAG (Status: Completed ✅)
- [x] Agentic RAG with iterative retrieval loops (`rag/agentic_rag.cpp`)
- [x] Knowledge graph-augmented retrieval (entity linking)
- [x] Multi-modal RAG (image + text retrieval) (`rag/multimodal_rag.cpp`)
- [x] Online learning from evaluation feedback (adaptive retrieval)

### Phase 5: Distributed Evaluation, Benchmarks & Security (Status: Completed ✅)
- [x] Distributed RAG evaluation across multiple judge models (`rag/distributed_rag_evaluator.h/.cpp`) (Issue: #2245) — thread-pool parallel dispatch; MEAN/WEIGHTED_MEAN/MAJORITY_VOTING/BEST_OF_N aggregation; factory helpers
- [x] Performance benchmark harness (`benchmarks/bench_rag_evaluation.cpp`) — recall@K (K=1/5/10/20/50); FAST/BALANCED/THOROUGH latency; batch throughput; end-to-end pipeline benchmark
- [x] Prompt injection detection and sanitization (`rag/prompt_injection_detector.h/.cpp`) — security audit for retrieved context; pattern-based heuristic detector; PromptInjectionSanitizer with configurable thresholds

### Phase 6: REPLUG Co-Training & Constitutional AI / RLAIF (Status: Completed ✅)
- [x] `ReplugRetriever` — REPLUG-style LLM-scored retrieval fusion (`rag/replug_retriever.h/.cpp`) (Target: Q1 2026) — Inputs: query + RetrievedDocument list; Outputs: ReplugFusionResult with fused scores; λ interpolation, softmax temperature, min_retrieval_score filter, REPLUG-LSR weight update via KL gradient; ILLMScorer plugin; HeuristicLLMScorer (Jaccard); 30 unit tests
- [x] `RLAIFTrainer` — Constitutional AI + RLAIF preference dataset generation (`rag/rlaif_trainer.h/.cpp`) (Target: Q1 2026) — Inputs: query + draft response; Outputs: PreferencePair (prompt, chosen, rejected); critique-revision loop; IAIJudge plugin; HeuristicAIJudge; AIPrinciple registry; processBatch(); RLAIFConfig; 30 unit tests

### Phase 7: Context-Window Management & Token Budget (Status: Completed ✅)
- [x] `ContextWindowBudget` — central token-budget model (`include/llm/context_window_budget.h`) (Target: Q2 2026) — Inputs: model_ctx, system_prompt, query, min_response; Outputs: available_context_tokens, reserved_response_tokens; heuristic estimator ceil(chars/3.5); 20% floor on response reservation; fallback 4096; 30 unit tests
- [x] `RAGContextAssembler` — budget-aware chunk selection (`include/rag/rag_context_assembler.h`, `src/rag/rag_context_assembler.cpp`) (Target: Q2 2026) — Greedy Fill with Response Guard; truncation with configurable marker; computeMaxTokens(); 30 unit tests
- [x] `MultiStepRAGOrchestrator` — Map-Reduce and Iterative strategies (`include/rag/multi_step_rag.h`, `src/rag/multi_step_rag.cpp`) (Target: Q2 2026) — Map: batch partitioning bounded by context budget; Reduce: partial-answer synthesis; Iterative: gap-detection loop, max_iterations guard, deduplication; factory helpers; 15 unit tests
- [x] `LlamaCppPlugin::loadModel()` reads `n_ctx`/`context_length` from config JSON → `ModelInfo::context_length`; fallback 4096 (Target: Q2 2026)
- [x] `LlamaCppPlugin::generateRAG()` replaced naive doc concat with `RAGContextAssembler`; `max_tokens` capped via `computeMaxTokens()` (Target: Q2 2026)
- [x] `RAGContext::max_context_tokens` set to 0 (dynamic fallback); `response_budget_tokens` field added (Target: Q2 2026)
- [x] `RAGPromptConfig::reserved_response_tokens` field added (default: 512) (Target: Q2 2026)
- [x] `MultiHopReasoner` — multi-hop reasoning with query decomposition (`include/rag/multi_hop_reasoner.h`, `src/rag/multi_hop_reasoner.cpp`) (Target: Q2 2026) — heuristic + LLM-based decomposition; per-hop retrieval + inference with context injection; answer composition; factory helpers (single-hop, balanced, deep-reasoning); 15 unit tests
- [x] `AdaptiveRetrieval` — adaptive retrieval depth based on query complexity (`include/rag/adaptive_retrieval.h`, `src/rag/adaptive_retrieval.cpp`) (Target: Q2 2026) — QueryComplexity tiers (SIMPLE/MODERATE/COMPLEX/VERY_COMPLEX); connective/question-word heuristic; IComplexityScorer plugin; top_k + similarity_threshold scaling; factory helpers (lightweight, balanced, high-recall); 15 unit tests

### Phase 8: Loop 1–4 Explicit Orchestration & Federated RLAIF — IMPL-A2 + IMPL-A3 (Status: Completed ✅)

> *Paper 1 — §4.4 The Four Self-Optimising Loops / §5.4 ContinuousLearningOrchestrator*
> Issues: [IMPL-A2](../../docs/issues/lora_loops/IMPL-A2-loop-orchestration.md) · [IMPL-A3](../../docs/issues/lora_loops/IMPL-A3-federation-hooks.md)

- [x] Expose explicit named loop-trigger methods on `ContinuousLearningOrchestrator` (Implemented: 2026-04-19):
  - `triggerLoop1QueryExecution(const QueryExecutionOutcome&)` (Loop 1 — ≤ 10 ms BaoOptimizer feedback)
  - `triggerLoop2WorkloadAdaptation()` (Loop 2 — 60 s interval, `WorkloadAdaptiveOptimizer` + HNSW)
  - `triggerLoop3IndexLifecycle()` (Loop 3 — hours/days, `IndexSuggestionEngine`)
  - `triggerLoop4AdapterImprovement()` (Loop 4 — weekly, `IncrementalLoRATrainer`)
- [x] Add `FEDERATED_ROUND_START` event type to `ContinuousLearningOrchestrator` (IMPL-A3)
  - Fired after Loop 4 completes; 24 h minimum interval guard
  - Invokes `ILoRAFederationCoordinator::startRound()` when coordinator is injected
- [x] Add `setFederationCoordinator(ILoRAFederationCoordinator*)` DI setter
- [x] Loop-interference cooldown guard: `setOptimizationCooldown(seconds)` + per-loop timestamp map (RQ10)
- [x] JSON context serialiser `serializeLoopContext()` → JSON ≤ 8 000 chars / ≈ 2 000 tokens
- [x] `RAGIngestionBridge::indexOptimizerLog()` extension: index optimizer-log documents for RAG retrieval
- [x] 14 unit tests in `tests/test_clo_loops.cpp` (`test_clo_loops_focused` CMake target):
  - `CLO-L1-01` … `CLO-L1-03`: Loop 1 trigger, outcome in context JSON, completion handler
  - `CLO-L2-01` … `CLO-L2-03`: Loop 2 trigger, context JSON, completion handler
  - `CLO-L3-01` … `CLO-L3-02`: Loop 3 advisory guardrail pass, context JSON
  - `CLO-L4-01` … `CLO-L4-02`: Loop 4 trigger, context JSON
  - `CLO-FED-01`: `FEDERATED_ROUND_START` fires after Loop 4 (no throw when coordinator absent)
  - `CLO-COOL-01`: 60 s cooldown blocks second trigger; different loop unaffected
  - `SerializeContext_EmptyBeforeTrigger`, `SerializeContext_MultipleLoopsPresent`
- [x] `LoopPhase` enum on `ContinuousLearningOrchestrator`: `LOOP_1_HNSW_QUERY`, `LOOP_2_WORKLOAD`, `LOOP_3_SCHEMA_INDEX`, `LOOP_4_RLAIF`  (`include/rag/continuous_learning_orchestrator.h:249`)
- [x] `triggerLoop(LoopPhase)` — explicitly trigger a named learning loop; returns `LoopResult` (`include/rag/continuous_learning_orchestrator.h:283`)
- [x] `registerLoopCompletionHandler(LoopPhase, handler)` — per-phase completion callback (`include/rag/continuous_learning_orchestrator.h:293`)
- [x] `TriggerEvent::FEDERATED_ROUND_START` — fired automatically after a successful Loop-4 run with `guardrail_passed == true` (`include/rag/continuous_learning_orchestrator.h:309`)
- [x] `setFederationCoordinator(ILoRAFederationCoordinator*)` DI setter (`include/rag/continuous_learning_orchestrator.h:326`)
- [x] `setTrainerForFederation(IncrementalLoRATrainer*)` DI setter (`include/rag/continuous_learning_orchestrator.h:342`)
- [ ] Loop-interference cooldown guard: shared `OptimizationLock` with per-resource cooldown (RQ10)
- [ ] JSON context serialiser for Loop 1–3 outcome signals → `≤ 2 000 tokens` context block
- [ ] `RAGIngestionBridge` extension: index optimizer-log documents for RAG retrieval
- [ ] 12 new unit tests in `tests/test_continuous_learning_orchestrator_loops.cpp`:
  - `CLO-L1-01` … `CLO-L1-03`: Loop 1 trigger updates BaoOptimizer hint
  - `CLO-L2-01` … `CLO-L2-03`: Loop 2 trigger updates WorkloadAdaptiveOptimizer
  - `CLO-L3-01` … `CLO-L3-02`: Loop 3 trigger calls IndexSuggestionEngine
  - `CLO-L4-01` … `CLO-L4-02`: Loop 4 trigger calls IncrementalLoRATrainer
  - `CLO-FED-01`: `FEDERATED_ROUND_START` fires after Loop 4 + 24 h guard respected
  - `CLO-COOL-01`: cooldown guard prevents concurrent loop interference

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (streaming_retriever: 28 test cases; reranker: 30+ test cases; document_splitter: 37 test cases)
- [x] Unit tests coverage > 80% (streaming_retriever: 28 tests; reranker: 30+ tests; hybrid_retriever: 31 tests)
- [x] Unit tests for LearningMetrics (test_learning_metrics.cpp: recordEvaluation, computeMetrics, exportMetrics, printReport, window enforcement)
- [x] Unit tests for ClaimExtractor (test_claim_extractor.cpp: extract, verify, calculateFaithfulness, SelfConsistencyEvaluator)
- [x] Unit tests for CitationHighlighter (test_rag_citation_highlighter.cpp: comprehensive coverage; available in all build variants)
- [x] Unit tests for EvaluationReportExporter (test_rag_evaluation_report_exporter.cpp: JSON/HTML export; file I/O; edge cases; factory; available in all build variants)
- [x] Unit tests for DistributedRAGEvaluator (test_rag_distributed_evaluator.cpp: construction validation, aggregation strategies, meta fields, factory helpers, batch evaluate)
- [x] Unit tests for PromptInjectionDetector and Sanitizer (test_rag_prompt_injection.cpp: benign pass-through, instruction override, system-prompt leak, delimiter escape, role injection, markup injection, Unicode bidi, sanitizer truncation/replacement)
- [x] Unit tests for ReplugRetriever (test_rag_replug_retriever.cpp: ILLMScorer, HeuristicLLMScorer, fuse(), top_k truncation, min_retrieval_score filtering, weight updates, factory helpers; 30 tests)
- [x] Unit tests for RLAIFTrainer (test_rag_rlaif_trainer.cpp: IAIJudge, HeuristicAIJudge, runTrainingStep(), createPreferencePair(), processBatch(), principle management, dataset access, stats; 30 tests)
- [x] Unit tests for ContextWindowBudget (test_context_window_budget.cpp: estimateTokens, tokensToChars, compute, reserved_response_tokens enforcement, available_context_tokens arithmetic, helpers; 30 tests)
- [x] Unit tests for RagContextAssembler (test_rag_context_assembler.cpp: empty edge cases, single chunk fit/truncation, greedy fill, response-guard, truncation marker, computeMaxTokens; 30 tests)
- [x] Unit tests for MultiStepRAGOrchestrator (test_multi_step_rag.cpp: map-reduce single-pass, multi-batch, iterative cap, factory helpers; 15 tests)
- [x] Unit tests for MultiHopReasoner (test_rag_multi_hop_reasoner.cpp: 15 tests — A config/factory, B decomposition heuristic+LLM, C pipeline single/multi/error cases)
- [x] Unit tests for AdaptiveRetrieval (test_rag_adaptive_retrieval.cpp: 15 tests — A config/factory, B complexity analysis, C params + custom scorer injection)
- [x] Performance benchmarks (benchmarks/bench_rag_evaluation.cpp: recall@K harness, FAST/BALANCED/THOROUGH latency, distributed evaluator, injection scan throughput, end-to-end pipeline)
- [x] Integration tests (full pipeline: retrieve → generate → evaluate) — `test_rag_pipeline_integration.cpp` (heuristic/FAST mode, no live LLM required)
- [x] Performance benchmarks (recall@10, latency per mode)
- [x] Security audit (prompt injection in retrieved context)
- [x] Documentation complete (streaming_retriever.h, reranker.h, hybrid_retriever.h: full Doxygen API docs)
- [x] API stability guaranteed (streaming_retriever API: stable; CrossEncoderConfig: stable; HybridRetrieverConfig: stable)

## Known Issues & Limitations
- Evaluation accuracy depends on quality of the injected LLM judge model.
- Thorough mode (~2 s latency) is not suitable for real-time interactive use.
- No built-in document chunking strategy: now provided by `DocumentSplitter` (configurable chunk size, overlap, and strategy).

## Breaking Changes
- Evaluator scoring API (0–1 float range) is stable from v1.x.
- JudgeConfig fields may gain new optional parameters; backward-compatible.
