# ThemisDB RAG Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The RAG module provides retrieval-augmented generation runtime surfaces for document retrieval, context assembly, quality evaluation, and safety checks.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| rag_judge.cpp | multi-dimensional evaluation orchestration |
| hybrid_retriever.cpp | BM25 plus vector fusion retrieval |
| streaming_retriever.cpp | token-budget-aware context streaming |
| rag_context_assembler.cpp | deterministic context assembly under token limits |
| rag_ingestion_bridge.cpp | ingestion-to-retrieval bridge and enrichment |
| multi_step_rag.cpp | iterative and map-reduce style multi-step retrieval |
| adaptive_retrieval.cpp | complexity-aware retrieval depth control |
| prompt_injection_detector.cpp | prompt-injection detection and sanitization |
| quality_control_pipeline.cpp | retrieval and generation quality gate pipeline |
| delegate_evaluator.cpp | round-trip corruption benchmark support |

## Scope

In scope:
- retrieval fusion, reranking, and context assembly
- RAG evaluation and quality gates
- ingestion bridge and retrieval enrichment
- prompt and retrieval safety controls
- RAG-specific performance and reliability controls

Out of scope:
- core LLM backend lifecycle and model loading
- storage engine internals outside RAG ingestion integration
- non-RAG HTTP transport and server bootstrap details

## Known Limitations

- runtime behavior varies with configured retriever, evaluator, and LLM backend combinations
- some cross-node production mixes still need broader benchmark evidence
- environment-dependent integrations may affect end-to-end latency envelopes

**Production Readiness Status (Batch 3 verified 2026-08-14):**
- **Ready for production:** Hybrid retrieval (BM25 + vector), streaming context assembly, retrieval quality gates, ingestion bridge
- **Production-ready with limits:** Multi-step RAG (iterative, map-reduce), adaptive retrieval depth control, prompt-injection detection
- **Not yet production-ready:** WikiIndexStore Phase B (RocksDB integration, BM25+, HNSW, RRF fusion) — Wave B target Q4 2026
- **Pending:** LLM-Judge real integration (currently mock), persistent embedding cache, per-query retrieval guardrails

---

## Sourcecode Verification (Module: rag/readme)

- Verified files:
  - src/rag/rag_judge.cpp
  - src/rag/hybrid_retriever.cpp
  - src/rag/streaming_retriever.cpp
  - src/rag/rag_context_assembler.cpp
  - src/rag/rag_ingestion_bridge.cpp
  - src/rag/multi_step_rag.cpp
  - src/rag/adaptive_retrieval.cpp
  - src/rag/prompt_injection_detector.cpp
  - src/rag/quality_control_pipeline.cpp
  - src/rag/delegate_evaluator.cpp
- Verified behavior surfaces:
  - retrieval and context assembly flow
  - quality and safety gate behavior
  - ingestion bridge and evaluation path integration
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - implementation history remains in CHANGELOG.md
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
