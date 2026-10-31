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

# RAG Retrieval and Evaluation Modeling

AI provenance note: Exact model backend not verifiable from final document text alone.

## Scope

This document describes the ranking and evaluation assumptions represented by
the current RAG source. It does not prescribe a single end-to-end retriever for
all RAG entry points.

## Candidate fusion

`HybridRetriever::fuse()` combines two ranked inputs:

1. BM25/sparse candidates and dense-vector candidates are keyed by document ID.
2. With the default `use_rrf=true`, each list contributes a weighted reciprocal
   rank score. The default `rrf_k` is 60; a larger value reduces sensitivity to
   rank differences.
3. With `use_rrf=false`, the implementation uses configured non-negative
   weights and optionally normalizes raw scores before combining them.
4. Results are ordered by descending fused score and can be truncated by
   `top_k`. Either input list may be empty.

RRF is the default because sparse and dense raw scores need not share a scale.
Linear fusion retains score magnitude but makes ranking sensitive to score
normalization and the chosen weights. Neither mode guarantees improved
relevance; that requires a labeled evaluation on the target corpus.

`retrieveWithVectorizer()` is a narrower path: it encodes the query and only the
provided BM25 candidate contents, computes cosine similarity, then invokes the
same fusion. It does not independently retrieve candidates from a dense index.

## Reranking and context evaluation

The reranker scores a bounded candidate set and can reorder supplied
query-document pairs, but cannot recover relevant documents that candidate
generation omitted. In the current source, `CrossEncoderReranker::computeScore()`
always returns its built-in heuristic score; the ONNX forward pass is not
implemented. `loadModel()` validates the path and file, then sets a loaded flag;
`RerankResult::used_model` reflects that flag and is not evidence that model
inference occurred. The exposed `max_length`, `use_gpu`, and model-path settings
therefore do not establish learned scoring behavior. Treat all current
reranker scores as heuristic and evaluate them separately from model-based
cross-encoder results.

The metric implementation and RAG evaluation code expose separate measurements
for retrieval quality, evidence support, faithfulness, cost, and latency. Record
the metric definition, input set, and cutoff with each result. The checked-in
RAG evaluation contract contains thresholds, but those thresholds are
acceptance criteria, not measured baselines.

## Inputs, constraints, and failure cases

- Candidate IDs, relevance labels, and ranked-list positions must be
  unambiguous. Empty candidate sources are supported by fusion.
- Fusion weights must be non-negative and `rrf_k` must be greater than zero;
  invalid configuration is rejected.
- `top_k=0` means no output truncation in the fusion configuration.
- Missing, empty, oversized, or uninitialized embedding inputs are handled by
  the vectorizer's validation paths; incompatible model/vector settings can
  invalidate dense scoring.
- A reranker has a model and resource boundary: model loading can fail, and
  configured candidate/input limits can constrain the result.
- A high metric on synthetic or unit-test fixtures does not establish quality
  across domains, languages, or production traffic.

## Verification

The relevant behavior is covered by
`tests/rag/test_rag_hybrid_retriever.cpp`,
`tests/rag/test_rag_reranker.cpp`,
`tests/rag/test_reranker_budget_gate.cpp`, and
`tests/rag/test_metric_computation.cpp`. The corresponding focused CTest names
are registered in `tests/rag/CMakeLists.txt`.

For reproducible performance work, use the benchmark source matching the
behavior under study, retain the exact build configuration and model files,
record hardware and warm/cold cache state, and save raw benchmark output. The
RAG evaluation benchmark source uses fixed synthetic documents and relevant-ID
positions; the current local RAG benchmark CMake file does not register that
source as a target. See [research notes](./RESEARCH.md) for the asset and
registration limits. `HybridRetriever` declares targets of under 1 ms for 100
candidates and Recall@10 of at least 85% with balanced RRF weights; these are
targets in the header, not measured outcomes in this repository.

## References

- [Research notes](./RESEARCH.md)
- [Embedding path](./EMBEDDING.md)
- `include/rag/hybrid_retriever.h`
- `src/rag/hybrid_retriever.cpp`
- `include/rag/reranker.h`
- `src/rag/reranker.cpp`
- `src/rag/metric_computation.cpp`
