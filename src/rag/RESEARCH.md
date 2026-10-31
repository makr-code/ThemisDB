---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Research Notes

AI provenance note: Exact model backend not verifiable from final document text alone.

## Purpose and scope

This note records the implemented retrieval, ranking, embedding, and evaluation
methods in `src/rag`. It is for maintainers comparing retrieval behavior or
planning a quality evaluation. The implementation and tests are authoritative;
design specifications and performance targets are not evidence that a target was
measured or met.

## Problem and method

The RAG retrieval path combines sparse and dense candidate rankings, can apply
a reranking stage, and can evaluate the resulting evidence. The
`HybridRetriever` accepts BM25 and vector candidate lists and fuses them using
weighted Reciprocal Rank Fusion (RRF) by default, or normalized linear scores
when configured. RRF avoids adding incomparable raw BM25 and vector scores, but
its output depends on each input list's ordering and the configured weights and
constant.

Although the reranker API exposes an ONNX model path, the current
`CrossEncoderReranker::computeScore()` uses its heuristic scorer for every
candidate; the ONNX forward pass is not implemented. Model loading and the
`used_model` flag do not demonstrate learned scoring. Evaluation claims must
identify this as heuristic ranking unless a real inference path is added and
validated.

`DPRVectorizer` supplies separate query and passage encoders. The helper
`retrieveWithVectorizer()` encodes the query and the content of the supplied
BM25 candidates, calculates cosine scores, and fuses those candidates. It is not
a corpus-wide dense index search. Other retrieval and reranking paths are
implemented separately; consult their source before treating this helper as the
whole RAG pipeline.

The evaluation sources include retrieval and answer-quality metrics,
`RAGJudge`, result persistence, and benchmark harnesses. These methods evaluate
different stages; a retrieval score alone does not establish that a generated
answer is faithful to its context.

## Assumptions and limits

- Candidate IDs identify the same documents across sparse and dense lists;
  empty lists are allowed, but duplicate output IDs are not the intended result.
- RRF consumes ranks, whereas linear fusion consumes score values and relies on
  configured normalization and weights.
- DPR query and passage models must produce compatible vectors. The configured
  dimension, token-length cap, model files, tokenizer, and ONNX runtime affect
  the result.
- The vectorizer has a deterministic fallback path when an ONNX session or
  usable output is unavailable. Fallback vectors are not evidence of semantic
  embedding quality.
- Tests use small fixtures and injected vectorizers; they establish API,
  ordering, validation, and edge-case behavior, not relevance on a production
  corpus.
- `RESEARCH_EVAL_HARNESS_SPECIFICATION.md` describes an evaluation design.
  Dataset files named by the RAG evaluation contract were not present in the
  repository inventory, so those dataset-level results are not reproducible
  from this checkout.

## Evidence and reproduction

Relevant implementation and validation sources:

- `include/rag/hybrid_retriever.h` and `src/rag/hybrid_retriever.cpp`
- `include/rag/dpr_vectorizer.h` and `src/rag/dpr_vectorizer.cpp`
- `src/rag/reranker.cpp` and `src/rag/metric_computation.cpp`
- `tests/rag/test_rag_hybrid_retriever.cpp`
- `tests/rag/test_dpr_vectorizer.cpp`
- `tests/rag/test_metric_computation.cpp`
- `benchmarks/rag/bench_rag_evaluation.cpp`
- `benchmarks/rag/bench_rag_hybrid_retriever.cpp`

After configuring a test-enabled build, the focused tests can be run with:

```bash
cmake --build <build-dir> --target module_rag_test_rag_hybrid_retriever_focused module_rag_test_dpr_vectorizer_focused module_rag_test_metric_computation_focused
ctest --test-dir <build-dir> -R 'test_(rag_hybrid_retriever|dpr_vectorizer|metric_computation)_RagFocusedTests' --output-on-failure
```

The benchmark source files provide deterministic microbenchmark fixtures, not a
quality evaluation on the named external datasets. In particular,
`benchmarks/rag/CMakeLists.txt` currently registers only `bench_delegate_evaluator`
and `bench_fts_phase_b`; it does not register the two RAG benchmark source files
listed above as build targets. Do not report their declared performance targets
as measured results.

## References

- [Modeling](./MODELING.md)
- [Embedding](./EMBEDDING.md)
- [Evaluation contract](./EVALUATION_CONTRACT_V1.md)
- [Evaluation harness specification](./RESEARCH_EVAL_HARNESS_SPECIFICATION.md)
- [RAG architecture](./ARCHITECTURE.md)
