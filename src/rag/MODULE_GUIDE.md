---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Module Developer Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

## Audience and module boundary

Use this guide to navigate the RAG implementation and select the right contract
or test before changing retrieval, context assembly, evaluation, ingestion, or
RAG-specific safety behavior. The RAG module composes with the LLM, search,
ingestion, and storage surfaces; it does not own model lifecycle, core storage,
or server transport.

The C++ ownership boundary is:

| Path | Role |
|---|---|
| `include/rag/` | Public C++ declarations and API contracts |
| `src/rag/` | RAG implementation and module-local engineering docs |
| `tests/rag/` | Focused tests registered by `tests/rag/CMakeLists.txt` |
| `benchmarks/rag/` | Benchmark and evaluation sources; inspect the local CMake registration before assuming a source is runnable |

## Recommended reading path

1. Start with [module overview](./README.md) and
   [architecture](./ARCHITECTURE.md) for boundaries and data flow.
2. Read [API](./API.md) and [implementation](./IMPLEMENTATION.md) for the
   component contract you intend to change.
3. Read [research](./RESEARCH.md), [modeling](./MODELING.md), and
   [embedding](./EMBEDDING.md) before changing retrieval ranking or vector
   behavior.
4. Read [trust boundaries](./TRUST.md) before changing document, query, model,
   or telemetry handling.
5. Use [operations](./OPERATIONS.md) for ongoing signals and known operational
   limits; use the [runbook](./RUNBOOK.md) for incident triage.
6. Check [ROADMAP.md](./ROADMAP.md) and
   [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) for current delivery
   tracking. Design specifications are not proof that proposed behavior is
   integrated.

## Change-to-test map

The test targets are generated from each `test_*.cpp` in
`tests/rag/CMakeLists.txt`; the standard focused target is
`module_rag_<test-file-stem>_focused`, registered as
`<test-file-stem>_RagFocusedTests`.

| Change area | Focused test source |
|---|---|
| Hybrid fusion | `tests/rag/test_rag_hybrid_retriever.cpp` |
| Query/passage embeddings | `tests/rag/test_dpr_vectorizer.cpp` |
| Context token budget and ordering | `tests/rag/test_rag_context_assembler.cpp`, `tests/rag/test_rag_budget_consistency_focused.cpp` |
| Ingestion validation and partial writes | `tests/rag/test_rag_ingestion_bridge.cpp`, `tests/rag/test_rag_ingestion_bridge_hardening_focused.cpp` |
| Prompt-injection detection and sanitization | `tests/rag/test_rag_prompt_injection.cpp` |
| Evaluation and failure handling | `tests/rag/test_rag_judge.cpp`, `tests/rag/test_rag_error_handling_edge_cases_focused.cpp` |
| Freshness and SLO monitoring | `tests/rag/test_ingestion_latency_monitor.cpp`, `tests/rag/test_freshness_sla_enforcer.cpp`, `tests/rag/test_realtime_slo_tracker.cpp` |
| Span handling | `tests/rag/test_otel_span_emitter.cpp` |

For an already configured build, for example:

```bash
cmake --build <build-dir> --target module_rag_test_rag_context_assembler_focused
ctest --test-dir <build-dir> -R '^test_rag_context_assembler_RagFocusedTests$' --output-on-failure
```

For relevance or model-quality claims, use held-out labeled data and report
the dataset, model artifacts, metric definition, and cutoff. Fixture-based unit
tests do not establish production retrieval quality.

## Engineering invariants

- Keep candidate generation, rank fusion, reranking, and context assembly
  distinct in both code and measurements. Reranking cannot recover candidates
  omitted by retrieval.
- Preserve the assembler's response-token reservation and deterministic
  selection behavior. Validate empty input, zero available budget, truncation,
  and tie cases.
- Keep authorization and tenant scope in the upstream retrieval path. A RAG
  `collection` string or a safety scan is not an access-control decision.
- Treat ingestion `IndexResult` as a partial-write report: `ok == true` alone
  does not certify vector or graph persistence.
- Do not silently convert missing model/runtime support into a successful
  semantic result. Explicitly identify fallback or heuristic behavior in
  results and tests.
- Keep operational targets separate from observed measurements and provide
  reproducible evidence for any claimed improvement.

## References

- [API contracts](./API.md)
- [Implementation](./IMPLEMENTATION.md)
- [Runbook](./RUNBOOK.md)
- [Operations](./OPERATIONS.md)
- [Trust model](./TRUST.md)
- [Research](./RESEARCH.md) · [Modeling](./MODELING.md) · [Embedding](./EMBEDDING.md)
- `tests/rag/CMakeLists.txt`
