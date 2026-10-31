---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - "include/index/vector_index.h"
  - "include/index/ann_frontdoor.h"
  - "include/index/ann_index.h"
  - "src/index/ann_frontdoor.cpp"
  - "src/index/ROADMAP.md"
  - "tests/index/test_ann_cpu_parity.cpp"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index retrieval models

## Purpose and reader

This document describes the data and decision models an engineer must keep
consistent when implementing or reviewing index retrieval. It is intended for
backend authors, query integrators, and test reviewers. It is a model of index
behavior, not a model of embedding generation or a claim that a particular
backend is available in every build.

## Indexed data and distance

`VectorIndexManager` accepts vector-valued fields on entities and is initialized
with a fixed dimension and a configured `Metric` (`L2`, `COSINE`, or `DOT`).
The query and indexed vectors must use compatible dimensions and a consistent
metric. The public result type carries a primary key and a distance; lower
distance is better. For cosine, the header documents distance as `1 - cosine`.
Do not compare distances across different metrics or assume vectors have been
normalized unless the producer contract explicitly guarantees that.

The distance implementation and its metric branches are in
[`vector_index.cpp`](vector_index.cpp), with the configuration and operations in
[`vector_index.h`](../../include/index/vector_index.h). HNSW, flat scan, and
optional advanced backends represent different candidate-search trade-offs:
approximate methods can reduce work but must be evaluated against an exact
baseline for the same input and metric.

## ANN routing model

`AnnQueryContext` carries caller hints: `dataset_size`, `hot_tier`,
`shard_aware`, `recall_target`, `latency_budget_ms`, `scope_id`, and correlation
and confidence-policy identifiers. The route planner may use tier metadata
when a tier manager is registered. Its implementation prefers an eligible
scope backend, can select distributed execution for shard-aware requests, and
otherwise uses configured size/tier/backend availability rules. Inspect
[`planRetrieval()`](ann_frontdoor.cpp) and
[`AnnFrontdoor::Config`](../../include/index/ann_frontdoor.h) before changing
those rules.

Important modeling boundary: `recall_target` and `latency_budget_ms` are
available in the context and route explanation, but the current routing code
does not implement a measured optimizer that guarantees either target. Its
recall estimates are informational constants. Treat selection as routing
policy, not as proof that a target was achieved.

Distributed retrieval executes selected shard backends, retries according to
configuration, merges duplicate IDs using the smallest distance, sorts by
distance and then ID, and reports shard counts and partial-result state.
Cost-aware selection currently derives estimates from fixed metadata in
`ann_frontdoor.cpp`; it is not a live cost-learning model.

## Approximation and compression trade-offs

ScaNN's interface describes partitioning, asymmetric hashing/PQ distance
estimation, and exact-distance reranking; its current documented metric is L2
([`ann_index.h`](../../include/index/ann_index.h)). Other advanced-vector
choices and quantizers are configuration-dependent. Compression or candidate
pruning may change recall, memory use, and ranking. Validate each selected
configuration against raw-vector or exact-flat results instead of inferring
quality from the algorithm name.

## Evaluation model

For a test query set, compare approximate top-k IDs with an exact result for the
same metric. Report at least recall@k or overlap, result cardinality, distance
ordering, and invalid-distance handling. For performance experiments, also
record the workload, index parameters, build flags, hardware/backend, memory,
and latency distribution. The deterministic CPU fixture in
[`test_ann_cpu_parity.cpp`](../../tests/index/test_ann_cpu_parity.cpp) checks
fixture-level parity and result invariants; it is not a large-corpus recall
benchmark.

## Edge conditions and operational effect

- Null query pointers and zero dimensions are rejected by the frontdoor with
  `std::invalid_argument`; a non-positive `k` uses the configured default, and
  an invalid default returns an empty result with configuration-error metadata.
- Frontdoor validation truncates overlarge candidate lists and removes NaN or
  negative distances. It does not establish that the remaining list is
  complete or finite.
- A missing backend can produce a flat-brute-force route label, but the
  current frontdoor helper does not implement a storage scan and returns no
  candidates. Confirm actual result source before relying on such a route.
- Partial shard failures can return marked partial results or clear candidates,
  depending on configuration. Consumers must inspect the result metadata.
- GPU numerical parity has not been validated on all representative hardware;
  GPU parity is a pending roadmap gate, not a modeled invariant.

## Canonical sources

- [Vector API and metric configuration](../../include/index/vector_index.h)
- [ANN routing input and result types](../../include/index/ann_frontdoor.h)
- [ANN backend interface](../../include/index/ann_index.h)
- [Routing and merge implementation](ann_frontdoor.cpp)
- [CPU fixture validation](../../tests/index/test_ann_cpu_parity.cpp)
- [Current readiness and open gates](ROADMAP.md)
