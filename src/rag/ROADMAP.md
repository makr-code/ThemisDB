<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# RAG Module Roadmap

## Current Status
v1.x – Production-ready Retrieval-Augmented Generation system. 22 implementation files (~8,700 LOC) covering evaluation, knowledge gap detection, ethical compliance, multi-judge orchestration, streaming retrieval, cross-encoder re-ranking, and hybrid BM25+vector retrieval.

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

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Per-query evaluation report export (JSON / HTML) (Issue: #2240)

### Long-term (6-12 months)
- [P] Agentic RAG with iterative retrieval loops (Issue: #2241)
- [I] Multi-modal RAG (image + text retrieval) (Issue: #2243)
- [I] Online learning from evaluation feedback (adaptive retrieval) (Issue: #2244)
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

### Phase 3: Hybrid Retrieval & Citation Highlighting (Status: In Progress 🚧)
- [x] Hybrid retrieval (BM25 + vector) with configurable RRF weights
- [x] Citation highlighting (map answer sentences to source chunks)
- [x] Configurable chunk size and overlap for document splitting
- [x] Multi-document summarization before context injection
- [ ] Per-query evaluation report export (JSON / HTML)

### Phase 4: Agentic & Knowledge-Graph RAG (Status: Planned 📋)
- [P] Agentic RAG with iterative retrieval loops
- [x] Knowledge graph-augmented retrieval (entity linking)
- [ ] Multi-modal RAG (image + text retrieval)
- [ ] Online learning from evaluation feedback (adaptive retrieval)
- [ ] Distributed RAG evaluation across multiple judge models

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (streaming_retriever: 28 test cases; reranker: 30+ test cases; document_splitter: 37 test cases)
- [x] Unit tests coverage > 80% (streaming_retriever: 28 tests; reranker: 30+ tests; hybrid_retriever: 31 tests)
- [x] Unit tests for LearningMetrics (test_learning_metrics.cpp: recordEvaluation, computeMetrics, exportMetrics, printReport, window enforcement)
- [x] Unit tests for ClaimExtractor (test_claim_extractor.cpp: extract, verify, calculateFaithfulness, SelfConsistencyEvaluator)
- [x] Unit tests for CitationHighlighter (test_rag_citation_highlighter.cpp: comprehensive coverage; available in all build variants)
- [?] Integration tests (full pipeline: retrieve → generate → evaluate)
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
