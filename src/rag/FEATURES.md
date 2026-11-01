---
Author: ThemisDB Contributors
Created: 2026-10-18
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Module Features

Exact model backend not verifiable from final document text alone.

This page summarizes component capabilities exposed by the current RAG headers.
It is not a claim that every component is connected to every request path or
that a performance or production-readiness target has been met.

## Current component surfaces

- **Candidate fusion:** `HybridRetriever::fuse()` combines supplied sparse and
  dense candidate lists using reciprocal-rank fusion by default. It does not
  query an index; `retrieveWithVectorizer()` scores only the supplied BM25 pool.
- **Context assembly:** `RAGContextAssembler` selects and, when configured,
  truncates chunks under a context budget while reserving response tokens.
- **Ingestion bridge:** `RAGIngestionBridge` validates documents and can pass
  them through the ingestion workflow and optional vector/graph sinks. Check
  `IndexResult` and independently verify persistence; partial sink failures may
  not make `ok` false.
- **Evaluation:** `QualityControlPipeline`, `RAGJudge`, and evaluator classes
  expose independent checks and result objects. Calling them is an integration
  choice; the module does not enforce a universal response gate.
- **Safety:** injection detection and sanitization are explicit APIs. Callers
  remain responsible for applying them to every relevant input path and for
  enforcing authorization before retrieval.
- **Monitoring:** freshness, SLO, quality-monitoring, and span components are
  available, but their existence does not establish durable metrics, external
  alert delivery, or OTLP network export.

## Build and focused validation

RAG test sources are registered by `tests/rag/CMakeLists.txt`; for example, the
focused CTest entries are `test_rag_hybrid_retriever_RagFocusedTests` and
`test_rag_context_assembler_RagFocusedTests`. The test registration and target
names are generated from the source stems. There is no `tests/rag` or
`benchmarks/rag` Markdown guide.

Benchmark source files and their actual CMake registration are documented in
[`RESEARCH.md`](RESEARCH.md) and [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Canonical references

- [Module overview and API examples](README.md)
- [Architecture and composition boundaries](ARCHITECTURE.md)
- [Public C++ contracts](API.md)
- [Implementation behavior](IMPLEMENTATION.md)
- [Test map](MODULE_GUIDE.md)
- [Roadmap and open work](ROADMAP.md)
