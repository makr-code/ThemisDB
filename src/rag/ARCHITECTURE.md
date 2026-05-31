# RAG Module - Architecture Guide

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Version: 1.0
Last Updated: 2026-05-31
Module Path: src/rag/

## 1. Overview

The RAG module implements retrieval, context construction, evaluation, and guardrail surfaces for retrieval-augmented generation workflows.

## 2. Architecture Surfaces

| Surface | Source files |
|---|---|
| Retrieval fusion and ranking | src/rag/hybrid_retriever.cpp, src/rag/reranker.cpp, src/rag/replug_retriever.cpp |
| Context assembly and orchestration | src/rag/streaming_retriever.cpp, src/rag/rag_context_assembler.cpp, src/rag/multi_step_rag.cpp |
| Evaluation and quality control | src/rag/rag_judge.cpp, src/rag/faithfulness_evaluator.cpp, src/rag/quality_control_pipeline.cpp |
| Adaptive and iterative retrieval | src/rag/adaptive_retrieval.cpp, src/rag/agentic_rag.cpp, src/rag/multi_hop_reasoner.cpp |
| Ingestion bridge and enrichment | src/rag/rag_ingestion_bridge.cpp, src/rag/document_splitter.cpp |
| Safety and sanitization | src/rag/prompt_injection_detector.cpp, src/rag/bias_detector.cpp |
| Metrics and reporting | src/rag/hallucination_dashboard.cpp, src/rag/evaluation_report_exporter.cpp |
| Reliability benchmarking | src/rag/delegate_evaluator.cpp, src/rag/batch_evaluator.cpp |

## 3. Runtime Control Flow

1. Query enters retrieval path.
2. Hybrid or adaptive retriever selects candidate chunks.
3. Context assembler builds bounded prompt context.
4. Generation and evaluation stages run with quality/safety checks.
5. Result, citations, and diagnostics are emitted to downstream handlers.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Used by | API handlers, orchestration layers, AI runtime features |
| Uses | llm module, index/search surfaces, ingestion inputs |
| Exposes | retrieval APIs, context assembly outputs, evaluation and safety signals |

## 5. Concurrency Model

- Retrieval and evaluation components run under concurrent request load.
- Shared caches and metrics paths are coordinated in component implementations.
- Iterative workflows enforce bounded loops and explicit stop conditions.

## 6. Known Limits

- retrieval quality and latency depend on configured backend and index state
- benchmark coverage for all deployment topologies is still evolving
- environment-dependent backend availability can alter runtime envelopes

## 7. Sourcecode Verification (Module: rag/architecture)

- Verified files:
  - src/rag/hybrid_retriever.cpp
  - src/rag/reranker.cpp
  - src/rag/streaming_retriever.cpp
  - src/rag/rag_context_assembler.cpp
  - src/rag/rag_judge.cpp
  - src/rag/quality_control_pipeline.cpp
  - src/rag/adaptive_retrieval.cpp
  - src/rag/rag_ingestion_bridge.cpp
  - src/rag/prompt_injection_detector.cpp
  - src/rag/delegate_evaluator.cpp
- Verified interfaces and behavior:
  - retrieval and ranking integration
  - context and evaluation orchestration
  - safety and reliability benchmark surfaces
