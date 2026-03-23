<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# RAG Module Roadmap

## Current Status

v1.5.0 — production. Full RAG pipeline with hybrid retrieval, cross-encoder reranking, citation tracking, hallucination detection, multi-modal RAG, agentic RAG, RAGAS-compatible evaluation, distributed evaluation, streaming SSE, and online learning are operational.

## Completed

- [x] Core RAG pipeline (document split, summarize, LLM integration)
- [x] Quality gate pipeline with factory
- [x] RAG-specific prompt injection detection
- [x] LLM judge pipeline (G-Eval, pairwise, rubric, ensemble)
- [x] Continuous learning (client + orchestrator)
- [x] Knowledge gap detection and graph retrieval
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

### Phase 6 — Future Enhancements (Planned)
- [ ] Video modality support in `MultimodalRag` (Target: Q4 2026)
- [ ] Adaptive retrieval budget based on query complexity (Target: Q3 2026)
- [ ] Federated RAG across isolated data silos (Target: Q4 2026)

## Production Readiness Checklist

- [x] HybridRetriever validated on BEIR benchmark
- [x] Faithfulness evaluator correlation tested against human judgments
- [x] Agentic RAG tested with 10-step tool chains
- [ ] Video modality support (Target: Q4 2026)
