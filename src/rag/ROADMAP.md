<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# RAG Module Roadmap

## Current Status
v1.x – Production-ready Retrieval-Augmented Generation system. 25 implementation files (~9,900 LOC) covering evaluation, knowledge gap detection, ethical compliance, multi-judge orchestration, streaming retrieval, cross-encoder re-ranking, hybrid BM25+vector retrieval, batch evaluation, calibration, and LRU evaluation caching.

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

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [I] Distributed RAG evaluation across multiple judge models (Issue: #2245)

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
- [x] Agentic RAG with iterative retrieval loops (`agentic_rag.cpp`, Issue: #2241)
- [x] Knowledge graph-augmented retrieval (entity linking, `knowledge_graph_retriever.cpp`)
- [x] Multi-modal RAG (image + text retrieval, `multimodal_rag.cpp`, Issue: #2243)
- [x] Online learning from evaluation feedback (adaptive retrieval via Bayesian optimizer, `bayesian_optimizer.cpp`, Issue: #2244)
- [ ] Distributed RAG evaluation across multiple judge models (Issue: #2245, planned)

### Phase 5: Batch Evaluation, Calibration & Caching (Status: Completed ✅)
- [x] `EvaluationCache` – LRU cache with TTL, invalidation triggers, statistics, and warm-up API
- [x] `CalibrationManager` – temperature/Platt/isotonic regression; ECE, Brier score, inter-annotator agreement
- [x] `BatchEvaluator` – parallel workers, async futures, progress callbacks, stop/resume lifecycle

### Phase 6: Future (Planned 📋)
- [ ] Distributed RAG evaluation across multiple judge models (Issue: #2245)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (streaming_retriever: 28 test cases; reranker: 30+ test cases; document_splitter: 37 test cases)
- [x] Unit tests coverage > 80% (streaming_retriever: 28 tests; reranker: 30+ tests; hybrid_retriever: 31 tests)
- [x] Unit tests for LearningMetrics (test_learning_metrics.cpp: recordEvaluation, computeMetrics, exportMetrics, printReport, window enforcement)
- [x] Unit tests for ClaimExtractor (test_claim_extractor.cpp: extract, verify, calculateFaithfulness, SelfConsistencyEvaluator)
- [x] Unit tests for CitationHighlighter (test_rag_citation_highlighter.cpp: comprehensive coverage; available in all build variants)
- [x] Unit tests for EvaluationReportExporter (test_rag_evaluation_report_exporter.cpp: JSON/HTML export; file I/O; edge cases; factory; available in all build variants)
- [x] Unit tests for EvaluationCache (test_rag_evaluation_cache.cpp: LRU eviction, TTL expiry, invalidation, statistics, thread-safety; available in all build variants)
- [x] Unit tests for CalibrationManager (test_rag_calibration_manager.cpp: ECE, Brier, inter-annotator agreement, temperature scaling, model persistence; available in all build variants)
- [x] Unit tests for BatchEvaluator (test_rag_batch_evaluator.cpp: sync/async batch, progress callback, stop/resume, aggregated stats)
- [x] Integration tests (test_rag_pipeline_integration.cpp: split → retrieve → evaluate end-to-end, EvaluationCache hit/miss, BatchEvaluator consistency)
- [?] Performance benchmarks (recall@10, latency per mode)
- [?] Security audit (prompt injection in retrieved context)
- [x] Documentation complete (streaming_retriever.h, reranker.h, hybrid_retriever.h: full Doxygen API docs)
- [x] API stability guaranteed (streaming_retriever API: stable; CrossEncoderConfig: stable; HybridRetrieverConfig: stable)

## Known Issues & Limitations
- Evaluation accuracy depends on quality of the injected LLM judge model.
- Thorough mode (~2 s latency) is not suitable for real-time interactive use.
- No built-in document chunking strategy: now provided by `DocumentSplitter` (configurable chunk size, overlap, and strategy).

## Breaking Changes
- Evaluator scoring API (0–1 float range) is stable from v1.x.
- JudgeConfig fields may gain new optional parameters; backward-compatible.
