<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# RAG Module Roadmap

## Current Status

v2.0.0 — production. Full RAG pipeline with hybrid retrieval, cross-encoder reranking, citation tracking, hallucination detection, multi-modal RAG, agentic RAG, RAGAS-compatible evaluation, distributed evaluation, streaming SSE, online learning, REPLUG-style LLM-scored fusion, Constitutional AI / RLAIF training pipeline, and FLARE active retrieval with `VectorIndexManager` callback bridge are operational.

## Completed

- [x] Core RAG pipeline (document split, summarize, LLM integration)
- [x] Quality gate pipeline with factory
- [x] RAG-specific prompt injection detection
- [x] LLM judge pipeline (G-Eval, pairwise, rubric, ensemble)
- [x] Continuous learning (client + orchestrator)
- [x] Knowledge gap detection and graph retrieval
- [x] FLARE active retrieval — `RetrievalCallback` + `setRetrievalCallback()` (§17.30)
- [x] ONNX local inference support
- [x] A/B testing + Bayesian optimizer
- [x] NLI faithfulness verifier, bias detector, calibration manager
- [x] Evaluation cache and report exporter
- [x] HybridRetriever (BM25 + vector + RRF)
- [x] Cross-encoder reranking
- [x] Citation highlighting, streaming SSE
- [x] RAGAS-compatible evaluation suite
- [x] Distributed evaluation coordination
- [x] Hallucination dashboard
- [x] Agentic RAG with tool use
- [x] Multi-modal RAG (text + image + audio)
- [x] REPLUG-style LLM-scored retrieval fusion (`replug_retriever.h/.cpp`)
- [x] Constitutional AI / RLAIF training pipeline (`rlaif_trainer.h/.cpp`)

## Implementation Phases

### Phase 1 — Core Pipeline ✅
- [x] Document splitter, summarizer, LLM integration
- [x] Quality control pipeline and factory

### Phase 2 — Evaluation Framework ✅
- [x] Faithfulness, relevance, coherence, completeness evaluators
- [x] LLM judge pipeline (G-Eval, pairwise, rubric)
- [x] Batch evaluator, evaluation cache, report exporter

### Phase 3 — Advanced Retrieval ✅
- [x] HybridRetriever BM25 + vector + RRF
- [x] Cross-encoder reranking
- [x] Knowledge graph retriever

### Phase 4 — Safety & Quality ✅
- [x] Prompt injection detector (RAG-specific)
- [x] Adversarial tester, bias detector
- [x] Hallucination dashboard

### Phase 5 — Multi-modal & Agentic ✅
- [x] MultimodalRag text + image + audio
- [x] AgenticRag multi-step tool use
- [x] Streaming retriever SSE

### Phase 6 — REPLUG Co-Training & Constitutional AI / RLAIF ✅
- [x] `ReplugRetriever` — REPLUG-style LLM-scored fusion (Shi et al., 2023); ILLMScorer interface; HeuristicLLMScorer; λ interpolation; REPLUG-LSR weight update; factory helpers; 30 unit tests
- [x] `RLAIFTrainer` — Constitutional AI + RLAIF preference dataset generation (Bai et al., 2022; Lee et al., 2023); IAIJudge interface; HeuristicAIJudge; AIPrinciple registry; processBatch(); factory helpers; 30 unit tests

### Phase 7 — Advanced Retrieval & Reasoning ✅
- [x] `MultiHopReasoner` — multi-hop reasoning with query decomposition (`include/rag/multi_hop_reasoner.h`, `src/rag/multi_hop_reasoner.cpp`); heuristic + LLM-based decomposition; per-hop retrieval with context injection; answer composition; factory helpers; 15 unit tests
- [x] `AdaptiveRetrieval` — adaptive retrieval depth based on query complexity (`include/rag/adaptive_retrieval.h`, `src/rag/adaptive_retrieval.cpp`); QueryComplexity tiers (SIMPLE/MODERATE/COMPLEX/VERY_COMPLEX); heuristic + custom scorer; top_k and similarity_threshold scaling; factory helpers; 15 unit tests
- [ ] Video modality support in `MultimodalRag` (Target: Q4 2026)
- [ ] Federated RAG across isolated data silos (Target: Q4 2026)

### Phase 8 — FLARE Retrieval-Callback Bridge ✅
- [x] `RetrievalCallback` type alias + `KnowledgeGapDetector::setRetrievalCallback()` — wires FLARE loop to any retrieval back-end (`include/rag/knowledge_gap_detector.h`)
- [x] `performDynamicRetrieval()` implements callback invocation with exception guard and graceful empty-return fallback (`src/rag/knowledge_gap_detector.cpp`)
- [x] `LLMApiHandler::setVectorIndex(VectorIndexManager*, RocksDBWrapper*)` — `handleRAG()` performs real `embed()` + `searchKnn()` + `convertToRetrievedDocuments()` pipeline (`include/server/llm_api_handler.h`, `src/server/llm_api_handler.cpp`)
- [x] 7 unit tests in `tests/test_knowledge_gap_retrieval_callback.cpp` (KGD-CB-01…07); CMake target `test_knowledge_gap_retrieval_callback`
- [x] Reference documentation: `docs/flare_retrieval_callback_bridge.md`; Compendium §17.30

## Production Readiness Checklist

- [x] HybridRetriever validated on BEIR benchmark
- [x] Faithfulness evaluator correlation tested against human judgments
- [x] Agentic RAG tested with 10-step tool chains
- [x] ReplugRetriever: 30 unit tests (ILLMScorer, fusion, weight updates, factory)
- [x] RLAIFTrainer: 30 unit tests (IAIJudge, runTrainingStep, batch, dataset, stats)
- [x] MultiHopReasoner: 15 unit tests (config, heuristic + LLM decomposition, pipeline)
- [x] AdaptiveRetrieval: 15 unit tests (config, complexity analysis, param computation, custom scorer)
- [x] FLARE Retrieval-Callback Bridge: 7 unit tests (KGD-CB-01…07); `test_knowledge_gap_retrieval_callback`
- [ ] Video modality support (Target: Q4 2026)
