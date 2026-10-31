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

# RAG API Reference

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose and scope

This is a source-level reference for maintainers integrating the public C++
surfaces under `include/rag/`. It is not an HTTP endpoint specification or a
promise of binary compatibility. The included headers define parameter and
result types; implementation details and tests define observed edge behavior.

## Retrieval and ranking

### `themis::rag::HybridRetriever`

Header: `include/rag/hybrid_retriever.h`

- `fuse(bm25_candidates, vector_candidates)` accepts two lists of
  `judge::RetrievedDocument` and returns `HybridFusionResult`. Results are sorted
  by descending fused score; each returned document's `similarity_score` is
  replaced by that fused score. Either list may be empty.
- Default configuration uses weighted Reciprocal Rank Fusion (`rrf_k = 60`,
  equal weights, `top_k = 10`). `top_k == 0` returns all fused candidates.
- Set `use_rrf = false` for weighted linear score combination. Weights must be
  non-negative; `rrf_k` must be positive. Invalid constructor or setter
  configuration throws `std::invalid_argument`.
- `retrieveWithVectorizer(query, bm25_candidates)` throws
  `std::invalid_argument` for an empty query and `std::runtime_error` if the
  vectorizer is absent or uninitialized. It dense-scores only the supplied BM25
  candidates; it does not search a vector index.

Use the ranking test `tests/rag/test_rag_hybrid_retriever.cpp` when changing
fusion behavior.

### `themis::rag::CrossEncoderReranker`

Header: `include/rag/reranker.h`

`rerank(query, candidates, top_k)` returns sorted documents and per-document
scores. It can filter by `min_score_threshold`; empty inputs, oversized inputs,
or excessive candidate counts fail closed with an empty result. An empty
`model_path` uses the built-in heuristic scorer. In the current implementation,
`computeScore()` uses that heuristic even when `loadModel()` has set the
`model_loaded` flag; the ONNX forward pass remains a comment. Consequently,
`RerankResult::used_model` reports the flag, not proof that model inference
occurred. `loadModel()` validates a file and marks it loaded but does not create
an inference session. Do not treat this surface as learned cross-encoder
scoring until an actual model inference path is implemented and tested.

The loader checks a `.sha256` sidecar when present, rejects a mismatch, and
warns but proceeds when the sidecar is absent. `require_model_checksum` and
`expected_model_checksum` are not enforced by the current implementation;
callers requiring artifact integrity must verify it independently.

## Embeddings and context

### `themis::rag::DPRVectorizer`

Header: `include/rag/dpr_vectorizer.h`

Call `initialize()` before `encodeQuery()` or `encodePassage()`. Empty
single-text inputs are invalid; an empty `encodePassageBatch()` is valid and
returns an empty result. Query and passage models must emit compatible
dimensions and use compatible preprocessing. A vectorizer instance is not
thread-safe; synchronize concurrent access or use separate instances. This
class-specific contract conflicts with the general thread-safety statement in
`include/rag/vectorizer_interface.h`; follow the stricter DPR restriction until
the interface discrepancy is resolved. See [embedding details](./EMBEDDING.md)
for model evaluation and fallback limits.

### `themis::rag::RAGContextAssembler`

Header: `include/rag/rag_context_assembler.h`

`assemble(chunks, system_prompt, query)` returns `AssembledContext` with
selected chunks, estimated `tokens_used`, response tokens remaining, and a
`was_truncated` flag. Empty input and no available context are valid outcomes,
not exceptions. The configured response reservation is
`max(min_response_tokens, 20% of the model context window)`. Callers should
derive the generation maximum with `computeMaxTokens()` and their provider's
context-window budget rather than independently assuming all remaining tokens
are available.

## Ingestion and safety

### `themis::rag::RAGIngestionBridge`

Header: `include/rag/rag_ingestion_bridge.h`

Construct with a non-null `IngestionToolbox`; vector and graph writers are
optional. `indexDocument()` returns an `IndexResult` rather than throwing for
ordinary validation or sink failures. Check both `ok` and `vector_count`:
vector sink failure is logged and may produce a partial result with `ok == true`.
Graph writer errors are not represented in the result. Input limits and
deduplication behavior are detailed in
[implementation guidance](./IMPLEMENTATION.md).

### `themis::rag::security::PromptInjectionDetector` and
`PromptInjectionSanitizer`

Header: `include/rag/prompt_injection_detector.h`

`scan(text)` returns severity and findings; `is_suspicious()` means at least
MEDIUM and `is_blocked()` means HIGH or CRITICAL. Sanitization replaces
matched fragments at or above its configured threshold and can strip Unicode
direction overrides. These are pattern-based controls, not proof that arbitrary
input is safe. The caller chooses where to scan, block, or sanitize.

## Evaluation and monitoring

### `themis::rag::judge::QualityControlPipeline`

Header: `include/rag/quality_control_pipeline.h`

`runQualityControl(query, answer, documents)` produces stage scores, timings,
failure reasons, citation coverage, and one of `PASSED`, `FAILED`,
`RETRY_NEEDED`, or `ESCALATE`. Stages and thresholds are configurable. The
caller must treat non-passing results according to its response policy; this
class does not itself enforce every server response path.

### Freshness, SLO, and telemetry surfaces

- `IngestionLatencyMonitor` and `FreshnessSLAEnforcer` expose percentile,
  compliance, refresh, event, and fallback-shard methods in
  `include/rag/ingestion_latency_monitor.h` and
  `include/rag/freshness_sla_enforcer.h`.
- `RealtimeSLOTracker` accepts query metrics and returns compliance and health
  data; it is an in-memory component in `include/rag/realtime_slo_tracker.h`.
- `RagQualityMonitor` accepts layer metrics, emits gauge-formatted log lines,
  and checks selected rolling anomalies in
  `include/rag/rag_quality_monitor.h`.
- `OTELSpanEmitter` provides span attributes, events, and flush/stat methods in
  `include/rag/otel_span_emitter.h`. The current implementation does not perform
  an OTLP network export; see [operations](./OPERATIONS.md).

These APIs are separate components and are not implied to be wired into every
RAG request by their presence in the module.

## Source of truth and tests

The headers above are the contract for types and method signatures. Consult
their `.cpp` files for actual behavior and `tests/rag/CMakeLists.txt` for
registered focused tests. The RAG evaluation contract defines evaluation
acceptance criteria; it does not assert that a particular deployment has
achieved those metrics.

## References

- [Implementation guide](./IMPLEMENTATION.md)
- [Module guide](./MODULE_GUIDE.md)
- [Trust boundaries](./TRUST.md)
- [Operations](./OPERATIONS.md)
- `tests/rag/test_rag_hybrid_retriever.cpp`
- `tests/rag/test_rag_context_assembler.cpp`
- `tests/rag/test_rag_ingestion_bridge_hardening_focused.cpp`
- `tests/rag/test_rag_prompt_injection.cpp`
- `tests/rag/test_rag_judge.cpp`
