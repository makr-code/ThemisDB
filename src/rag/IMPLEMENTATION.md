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

# RAG Implementation Guide

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose and scope

This document is for developers implementing or changing RAG behavior in
`src/rag`. It records the current component contracts and the points where a
caller must compose or enforce them. The implementation and public headers are
authoritative; phase specifications describe intent and must not be read as
proof that every proposed integration is active.

RAG is a set of composable retrieval, assembly, evaluation, ingestion, and
safety components. The module does not expose one mandatory end-to-end request
pipeline. The calling service is responsible for connecting these stages and
for handling their results.

## Request-side composition

1. **Retrieve candidates.** A search backend supplies `judge::RetrievedDocument`
   candidates to `HybridRetriever::fuse()`. The fusion method ranks the union of
   the BM25 and vector lists by document ID; it does not query an index.
2. **Optionally score a candidate pool densely.**
   `retrieveWithVectorizer()` encodes the query and each non-empty document in
   the supplied BM25 list, then fuses those scores with the sparse list. This
   helper cannot discover documents omitted from the BM25 pool.
3. **Optionally rerank.** `CrossEncoderReranker` reorders only the supplied
   candidates. The current `computeScore()` always uses its heuristic scorer;
   a successful `loadModel()` only sets a loaded flag and does not perform ONNX
   inference. Do not report its scores as learned cross-encoder output.
4. **Assemble bounded context.** `RAGContextAssembler::assemble()` sorts chunks
   deterministically by relevance and tie-break fields, fills the available
   context budget, and may truncate the last chunk. It reserves response tokens
   through `ContextWindowBudget`; callers should use the returned token
   accounting when setting the generation limit.
5. **Generate and evaluate.** The service invokes its selected LLM backend and
   may pass the answer, query, and evidence to `QualityControlPipeline` or
   `RAGJudge`. These are explicit evaluation components; calling retrieval or
   assembly alone does not establish answer faithfulness.
6. **Apply safety and authorization.** Prompt-injection scanning and
   sanitization are explicit APIs. The caller must decide where they run and
   must enforce access scope before data enters retrieval or context assembly.
   The presence of a detector in the module does not imply every path invokes it.

## Ingestion-side composition

`RAGIngestionBridge::indexDocument()` sends text through the
`IngestionToolbox` workflow when available, then optionally writes vector
records and graph entities through the injected sinks. Re-indexing identical
text in the same collection produces the same document ID; sink overwrite
semantics determine the resulting records.

Validation rejects empty text, text over 5 MiB, invalid collection/MIME/filename
values, and control characters in those metadata inputs. If the workflow is
unavailable or fails, the bridge builds a minimal entity set and one retrieval
chunk. A failed vector write is logged but does not make the returned
`IndexResult` fail; inspect `vector_count` and the logs before treating
`ok == true` as proof that vectors were persisted. Graph writes are also
best-effort and their sink results are not exposed in `IndexResult`.

## Invariants and edge cases

- Candidate IDs should be stable and unique within each ranked input. Fusion
  uses IDs as map keys; duplicate IDs can merge score contributions.
- RRF consumes input-list positions as ranks. Do not pass unsorted scores and
  expect RRF to sort by score first.
- Linear fusion consumes `similarity_score` values. It is sensitive to score
  scales and configured weights.
- Empty candidate lists are valid for `fuse()`. An empty BM25 pool makes
  `retrieveWithVectorizer()` return empty fusion output without dense search.
- Empty context input or an exhausted context budget returns a valid empty
  assembly result; callers must handle an empty evidence set.
- The DPR vectorizer requires initialization and non-empty single query/passage
  input. Its deterministic fallback is shape-preserving, not semantically
  equivalent to the configured model.
- `DPRVectorizer` documents that a single instance is not thread-safe.
  `RAGContextAssembler` allows concurrent assembly calls only while its
  configuration is not being mutated.
- RAG evaluation scores, test fixtures, and acceptance thresholds are not
  production relevance measurements. Record the dataset, labels, model
  artifacts, and metric cutoff for any quality claim.

## Validation path

The focused unit tests are registered from `tests/rag/CMakeLists.txt`. Useful
targets include:

```bash
cmake --build <build-dir> --target \
  module_rag_test_rag_hybrid_retriever_focused \
  module_rag_test_rag_context_assembler_focused \
  module_rag_test_rag_ingestion_bridge_hardening_focused

ctest --test-dir <build-dir> \
  -R '^(test_rag_hybrid_retriever|test_rag_context_assembler|test_rag_ingestion_bridge_hardening_focused)_RagFocusedTests$' \
  --output-on-failure
```

For changes to detector or sanitizer behavior, build
`module_rag_test_rag_prompt_injection_focused` and run
`ctest --test-dir <build-dir> -R '^test_rag_prompt_injection_RagFocusedTests$'`.
Use target-corpus evaluation for ranking quality; unit tests establish local
contract behavior, not corpus quality.

## References

- [Public API contracts](./API.md)
- [Module guide](./MODULE_GUIDE.md)
- [Architecture](./ARCHITECTURE.md)
- [Research evidence](./RESEARCH.md)
- [Modeling assumptions](./MODELING.md)
- [Embedding behavior](./EMBEDDING.md)
- [RAG production requirements](./PRODUCTION_REQUIREMENTS.md)
- `include/rag/hybrid_retriever.h`, `include/rag/rag_context_assembler.h`,
  `include/rag/rag_ingestion_bridge.h`
- `src/rag/hybrid_retriever.cpp`, `src/rag/rag_context_assembler.cpp`,
  `src/rag/rag_ingestion_bridge.cpp`, `tests/rag/CMakeLists.txt`
