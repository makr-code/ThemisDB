<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# RAG Module Roadmap

## Current Status
v1.x – Production-ready Retrieval-Augmented Generation system. 19 implementation files (~7,600 LOC) covering evaluation, knowledge gap detection, ethical compliance, and multi-judge orchestration.

## Completed ✅
- [x] RAGJudge – main orchestrator for multi-dimensional evaluation
- [x] KnowledgeGapDetector – three-level gap detection system
- [x] LLM integration bridge to InferenceEngineEnhanced
- [x] FaithfulnessEvaluator – fact-checking against retrieved sources
- [x] RelevanceEvaluator – query-answer alignment scoring
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

## In Progress 🚧
- [!] Streaming retrieval with incremental context window filling (Target: Q2 2026) (Issue: #2437)
- [x] Re-ranking layer with cross-encoder model integration (Target: Q2 2026) (Issue: #2247)
- [I] Hallucination rate tracking dashboard (Target: Q3 2026) (Issue: #2438)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Hybrid retrieval (BM25 + vector) with configurable RRF weights (Issue: #1968)
- [!] Citation highlighting (map answer sentences to source chunks) (Issue: #2436)
- [I] Configurable chunk size and overlap for document splitting (Issue: #2238)
- [I] Multi-document summarization before context injection (Issue: #2239)
- [I] Per-query evaluation report export (JSON / HTML) (Issue: #2240)

### Long-term (6-12 months)
- [I] Agentic RAG with iterative retrieval loops (Issue: #2241)
- [I] Knowledge graph-augmented retrieval (entity linking) (Issue: #2242)
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

### Phase 2: Streaming Retrieval & Re-Ranking (Status: In Progress 🚧)
- [~] Streaming retrieval with incremental context window filling
- [x] Re-ranking layer with cross-encoder model integration
- [~] Hallucination rate tracking dashboard

### Phase 3: Hybrid Retrieval & Citation Highlighting (Status: Planned 📋)
- [ ] Hybrid retrieval (BM25 + vector) with configurable RRF weights
- [ ] Citation highlighting (map answer sentences to source chunks)
- [ ] Configurable chunk size and overlap for document splitting
- [ ] Multi-document summarization before context injection
- [ ] Per-query evaluation report export (JSON / HTML)

### Phase 4: Agentic & Knowledge-Graph RAG (Status: Planned 📋)
- [ ] Agentic RAG with iterative retrieval loops
- [ ] Knowledge graph-augmented retrieval (entity linking)
- [ ] Multi-modal RAG (image + text retrieval)
- [ ] Online learning from evaluation feedback (adaptive retrieval)
- [ ] Distributed RAG evaluation across multiple judge models

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (full pipeline: retrieve → generate → evaluate)
- [?] Performance benchmarks (recall@10, latency per mode)
- [?] Security audit (prompt injection in retrieved context)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Evaluation accuracy depends on quality of the injected LLM judge model.
- Thorough mode (~2 s latency) is not suitable for real-time interactive use.
- No built-in document chunking strategy; callers manage chunk boundaries.

## Breaking Changes
- Evaluator scoring API (0–1 float range) is stable from v1.x.
- JudgeConfig fields may gain new optional parameters; backward-compatible.
