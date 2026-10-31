> Author: ThemisDB Team
> Created: 2026-10-30
> Last Updated: 2026-10-31
> Status: active
> AI_Used: true
> AI_Models: unknown
> AI_Contributor: ThemisDB documentation orchestration specialist
> Human_Review: false
> Human_Author: ThemisDB Team
> Source level: level1
> SOT domain: module-behavior
> Canonical source references: [RotatE header](../../include/graph/rotate_completion.h), [RotatE implementation](rotate_completion.cpp), [RotatE tests](../../tests/graph/test_rotate_completion.cpp), [root roadmap](../../ROADMAP.md)
> Related issue: #7122
> Related milestone: DOC-WEEKLY-2026-44
>
> Exact model backend not verifiable from final document text alone.

# Graph embeddings: implemented surfaces and evidence

## RotatE link prediction

The concrete embedding implementation found in `src/graph/` is `RotatEModel` in [rotate_completion.cpp](rotate_completion.cpp), declared in [rotate_completion.h](../../include/graph/rotate_completion.h). It models an entity as a complex vector (real and imaginary components) and a relation as phase rotations. For a triple `(h, r, t)`, the implementation computes the L1 distance between the rotated head and tail embeddings; lower distance is ranked as more plausible.

The public API lets a caller register entities and relations, train on `KGTriple` values, score a triple, retrieve entity/phase vectors, and rank head or tail candidates. `KGCompletionEngine` can pass predictions to a supplied `KnowledgeGraphReasoner` when the configured score threshold accepts them. This is caller-directed fact injection; a ranking is not independently verified truth.

Implementation details visible in the current source:

- Entity and relation registries deduplicate repeated IDs.
- Training initializes parameters from a fixed-seed generator, uses negative entity sampling and approximate SGD/sub-gradient updates with a margin loss, normalizes entity vectors, and wraps relation phase values. The [graph changelog](CHANGELOG.md) identifies RTE-S01 as approximate SGD, not the full self-adversarial RotatE gradient.
- `entityEmbedding()` and `relationPhase()` return empty vectors before training. Scoring and ranking require a trained model; unknown IDs are reported as `std::out_of_range`.
- `entityEmbedding()` returns `2 * embedding_dim` interleaved real/imaginary floats. `relationPhase()` returns `embedding_dim` phase values.
- The implementation uses CPU `float` parameter arrays (RTE-S02 in the [graph changelog](CHANGELOG.md)); no GPU or external model backend is established by this source.

## What the tests establish

[test_rotate_completion.cpp](../../tests/graph/test_rotate_completion.cpp) checks entity/relation registry behavior, errors before training and for unknown IDs, successful training on a small fixture, embedding sizes, finite non-negative scores, stable repeated scoring, sorted top-k results, reasoner integration, and that changing epoch count changes a score.

[test_rotate_completion_acceptance.cpp](../../tests/graph/test_rotate_completion_acceptance.cpp) defines an in-repository deterministic fixture, a local lightweight TransE comparison, MRR ≥ 0.10 and Hits@10 ≥ 0.40 assertions, relative-to-TransE checks, a top-20 p99 < 50 ms assertion, and reasoner-integration assertions. These are test expectations, not captured benchmark outcomes. No test was run for this documentation change.

### Acceptance-gate discrepancy

The current acceptance test asserts MRR ≥ 0.10, Hits@10 ≥ 0.40, relative-to-TransE bounds, and top-20 p99 < 50 ms. The repository-level [ROADMAP.md](../../ROADMAP.md) lists MRR ≥ 0.35 and Hits@10 ≥ 0.55; those are not current test assertions and must not be presented as evaluation results. Reconcile that root roadmap entry with its owner; it is outside this scoped update.

## Similar names, different behavior

- [graph_embedding.h](../../include/graph/graph_embedding.h) declares `EmbeddingAlgorithm`, `GraphEmbeddingConfig`, and the `IGraphEmbeddingProvider` interface. The inspected module source inventory contains no `graph_embedding.cpp` provider implementation. The declared Node2Vec, GraphSAGE, GAT, GCN, LINE, DeepWalk, and `CUSTOM_LLM` options are therefore not evidence of working backend implementations.
- [TensorFingerprintGraph](../../include/graph/tensor_fingerprint_graph.h) creates tensor-train fingerprints and similarity edges. Its MinHash/core-norm signature is not the RotatE model and is not an embedding of graph nodes.
- `ScheduledGraphEdgeRefreshEngine` accepts a caller-provided node embedding function. That injection point does not itself specify or provide the embedding model: [scheduled_edge_refresh.h](../../include/graph/scheduled_edge_refresh.h).
- `GraphWatermark::embed()` adds watermark metadata/IDs to a graph snapshot; it is not a learned graph embedding: [graph_watermark.h](../../include/graph/graph_watermark.h).

## Evaluation and limitations

Report the exact dataset/fixture, entity and relation vocabulary, train/evaluation split, configuration, ranking protocol, baseline, and measured run output. Scores are model distances, not calibrated confidence values; the `inject_threshold` is a caller-selected cutoff. Do not reuse the small in-repository fixture's assertions as evidence of external generalization or production latency.

For research methods and benchmark case mapping, see [RESEARCH.md](RESEARCH.md) and [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md). For algorithm/data representation distinctions, see [MODELING.md](MODELING.md).

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
